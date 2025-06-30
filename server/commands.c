#include "commands.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 512
#define FILENAME_MAX_LEN 64

typedef struct
{
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t total_chunks;
    uint32_t type;
    char filename[FILENAME_MAX_LEN];
} FileChunkHeader;

ssize_t read_line(int sock, char *buf, size_t max_len)
{
    size_t i = 0;
    char c;
    while (i < max_len - 1)
    {
        ssize_t n = recv(sock, &c, 1, 0);
        if (n <= 0)
            return n;
        if (c == '\n')
            break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    if (i == max_len - 1)
    {
        // Ensure no buffer overflow
        while (recv(sock, &c, 1, 0) > 0 && c != '\n')
            ;
    }
    return i;
}

void receive_file(int sock)
{
    FILE *fp = NULL;
    int total_chunks = 0;
    int received_chunks = 0;
    char filename[FILENAME_MAX_LEN] = {0};

    mkdir("saved", 0777);

    while (1)
    {
        FileChunkHeader header;
        int header_bytes = recv(sock, &header, sizeof(header), MSG_WAITALL);
        if (header_bytes <= 0)
            break;

        int chunk_id = ntohl(header.chunk_id);
        int chunk_size = ntohl(header.chunk_size);
        total_chunks = ntohl(header.total_chunks);

        if (chunk_id == 0)
        {
            strncpy(filename, header.filename, FILENAME_MAX_LEN - 1);
            filename[FILENAME_MAX_LEN - 1] = '\0'; // Ensure null-termination
            // save on 'saved' directory
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "saved/%s", filename);
            fp = fopen(full_path, "wb");
            if (!fp)
            {
                printf(RED "Error: Cannot create file '%s'\n" RESET, full_path);
                send(sock, "ERROR: Cannot create file\n", 26, 0);
                perror("fopen");
                return;
            }
        }

        uint8_t buffer[CHUNK_SIZE];
        ssize_t data_bytes = recv(sock, buffer, chunk_size, MSG_WAITALL);
        if (data_bytes != chunk_size)
            break;

        if (fp)
            fwrite(buffer, 1, chunk_size, fp);

        received_chunks++;
        if (received_chunks >= total_chunks)
            break;
    }

    if (fp)
    {
        fclose(fp);
        printf(GREEN "File received successfully: %s\n" RESET, filename);
        send(sock, "SUCCESS: File uploaded\n", 23, 0);
    }
    else
    {
        printf(RED "Error: File transfer failed\n" RESET);
        send(sock, "ERROR: File transfer failed\n", 28, 0);
    }
}

void send_file(int sock, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        printf(RED "Error: Cannot open file '%s' for reading\n" RESET, filename);
        send(sock, "ERROR: File not found\n", 22, 0);
        return;
    }

    printf(BLUE "Sending file: %s\n" RESET, filename);

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    int total_chunks = (fsize + CHUNK_SIZE - 1) / CHUNK_SIZE;

    for (int i = 0; i < total_chunks; i++)
    {
        uint8_t buffer[CHUNK_SIZE] = {0};
        int read_bytes = fread(buffer, 1, CHUNK_SIZE, fp);

        FileChunkHeader header = {
            .chunk_id = htonl(i),
            .chunk_size = htonl(read_bytes),
            .total_chunks = htonl(total_chunks),
            .type = htonl(0)};
        strncpy(header.filename, filename, FILENAME_MAX_LEN - 1);
        header.filename[FILENAME_MAX_LEN - 1] = '\0';
        send(sock, &header, sizeof(header), 0);

        send(sock, buffer, read_bytes, 0);
    }

    fclose(fp);
    printf(GREEN "File sent successfully: %s\n" RESET, filename);
}

void send_list(int sock)
{
    DIR *d = opendir(".");
    if (!d)
    {
        printf(RED "Error: Cannot open current directory\n" RESET);
        send(sock, "ERROR: Cannot list directory\n", 29, 0);
        return;
    }

    printf(BLUE "Listing directory contents\n" RESET);
    struct dirent *dir;
    char buffer[512] = {0};
    while ((dir = readdir(d)) != NULL)
    {
        // Skip the current and parent directory entries
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        struct stat st;
        // Get file type and name
        if (strlen(dir->d_name) >= FILENAME_MAX_LEN)
        {
            printf(RED "Error: Filename '%s' is too long\n" RESET, dir->d_name);
            snprintf(buffer, sizeof(buffer), "%s (Error: Filename too long)\n", dir->d_name);
            send(sock, buffer, strlen(buffer), 0);
            continue;
        }

        // TODO LOOK ??
        if (stat(dir->d_name, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                snprintf(buffer, sizeof(buffer), "- 📁 %s (Directory)\n", dir->d_name);
            }
            else if (S_ISREG(st.st_mode))
            {
                snprintf(buffer, sizeof(buffer), "- 📄 %s (File)\n", dir->d_name);
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "%s (Other)\n", dir->d_name);
            }
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%s (Error getting type)\n", dir->d_name);
        }

        send(sock, buffer, strlen(buffer), 0);
        printf(CYAN "Sent: %s" RESET, buffer);

        // Clear buffer for next entry
        memset(buffer, 0, sizeof(buffer));
    }
    closedir(d);
    send(sock, "END_OF_LIST\n", 12, 0);
}

void send_pwd(int sock)
{
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)))
    {
        printf(BLUE "Current directory: %s\n" RESET, cwd);
        strcat(cwd, "\n");
        send(sock, cwd, strlen(cwd), 0);
    }
    else
    {
        printf(RED "Error: Cannot get current directory\n" RESET);
        send(sock, "ERROR: Cannot get current directory\n", 36, 0);
    }
}

void change_dir(int sock, const char *path)
{
    if (chdir(path) == 0)
    {
        printf(GREEN "Changed directory to: %s\n" RESET, path);
        send(sock, "OK: Directory changed\n", 22, 0);
    }
    else
    {
        printf(RED "Error: Cannot change to directory '%s'\n" RESET, path);
        send(sock, "ERROR: Cannot change directory\n", 31, 0);
    }
}

void handle_client(int sock)
{
    char command[128];
    printf(GREEN "Client handler started\n" RESET);

    while (1)
    {
        memset(command, 0, sizeof(command));
        ssize_t bytes_read = read_line(sock, command, sizeof(command));

        if (bytes_read <= 0)
        {
            printf(YELLOW "Client disconnected or read error\n" RESET);
            break;
        }

        printf(CYAN "Received command: '%s'\n" RESET, command);

        if (strncmp(command, "upload", 6) == 0)
        {
            receive_file(sock);
        }
        else if (strncmp(command, "get ", 4) == 0)
        {
            send_file(sock, command + 4);
        }
        else if (strcmp(command, "ls") == 0)
        {
            send_list(sock);
        }
        else if (strcmp(command, "pwd") == 0)
        {
            send_pwd(sock);
        }
        else if (strncmp(command, "cd ", 3) == 0)
        {
            change_dir(sock, command + 3);
        }
        else
        {
            printf(YELLOW "Unknown command: '%s'\n" RESET, command);
            send(sock, "ERROR: Unknown command\n", 23, 0);
        }
    }

    printf(YELLOW "Client handler finished\n" RESET);
}