#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <unistd.h>

// Lib's for socket
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 512
#define FILENAME_MAX_LEN 64

#include <sys/stat.h>
#define SAVE_DIR "saved"

typedef struct
{
    __uint32_t chunk_id;
    __uint32_t chunk_size;
    __uint32_t total_chunks;
    char filename[FILENAME_MAX_LEN];
} FileChunkHeader; // ! __attribute__((packed)) FileChunkHeader; It's not portable.

void receiveFile(int client_sock)
{
    FILE *fp = NULL;
    int total_chunks = 0;
    int received_chunks = 0;
    char filename[FILENAME_MAX_LEN] = {0};

    // Creates saves dir if not exist
    mkdir(SAVE_DIR, 0777);

    while (1)
    {
        FileChunkHeader header;
        int header_bytes = recv(client_sock, &header, sizeof(header), MSG_WAITALL);
        if (header_bytes <= 0)
        {
            perror("Header receive failed");
            break;
        }

        // Convert from network to host byte order
        int chunk_id = ntohl(header.chunk_id);
        int chunk_size = ntohl(header.chunk_size);
        total_chunks = ntohl(header.total_chunks);

        if (chunk_id == 0)
        {
            strncpy(filename, header.filename, FILENAME_MAX_LEN - 1);
            filename[FILENAME_MAX_LEN - 1] = '\0';
            // char full_path[FILENAME_MAX_LEN + sizeof(SAVE_DIR) + 2];
            // snprintf(full_path, sizeof(full_path), "%s/%s", SAVE_DIR, filename);
            fp = fopen(filename, "wb");
            if (!fp)
            {
                perror("File open failed");
                return;
            }
        }

        uint8_t buffer[CHUNK_SIZE];
        ssize_t data_bytes = recv(client_sock, buffer, chunk_size, MSG_WAITALL);
        if (data_bytes != chunk_size)
        {
            fprintf(stderr, "Failed to receive chunk data\n");
            break;
        }

        if (fp)
        {
            fwrite(buffer, 1, chunk_size, fp);
        }

        received_chunks++;
        int percent = (received_chunks * 100) / total_chunks;
        printf("\rReceiving: %d%%", percent);
        fflush(stdout);

        if (received_chunks >= total_chunks)
        {
            break;
        }
    }

    if (fp)
    {
        fclose(fp);
        printf("\nFile received and saved as '%s'.\n", filename);
    }
}

int main()
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 1) < 0)
    {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0)
    {
        perror("Accept failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Client connected from IP: %s\n", client_ip);

    receiveFile(client_sock);

    close(client_sock);
    close(server_sock);
    return 0;
}