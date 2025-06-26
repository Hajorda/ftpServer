#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 512
#define FILENAME_MAX_LEN 64

typedef struct
{
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t total_chunks;
    // TODO Handle type.
    uint32_t type;
    char filename[FILENAME_MAX_LEN];
} FileChunkHeader;

void progress_bar(int percent)
{
    const int length = 30;
    int filled = (percent * length) / 100;
    printf("\r[");
    for (int i = 0; i < filled; i++)
        printf("█");
    for (int i = filled; i < length; i++)
        printf("▒");
    printf("] %d%%", percent);
    fflush(stdout);
}

// Send Chunk
int sendChunk(int socked, FileChunkHeader *header, const uint8_t *buffer, int payload_size)
{
    //! Why we are taking the header as pointer?
    // Send header
    if (send(socked, header, sizeof(*header), 0) <= 0)
    {
        perror("Header send failed");
        return 0;
    }

    // Send payload
    if (send(socked, buffer, payload_size, 0) <= 0)
    {
        perror("Payload send failed");
        return 0;
    }

    return 1;
};

// Send file
void send_file(const char *filename, int sockfd)
{
    FILE *fp = fopen(filename, "rb");

    if (!fp)
    {
        perror("File opening failed. File not be exist check the name dir.");
        return;
    }

    // Find length of the file.
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    // Calculate the number of the chunks
    int total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    uint8_t buffer[CHUNK_SIZE];

    FileChunkHeader header;

    for (int i = 0; i < total_chunks; i++)
    {
        int bytes_read = fread(buffer, 1, CHUNK_SIZE, fp);
        if (bytes_read <= 0)
        {
            fprintf(stderr, "Error while reading file chunk %d\n", i);
            break;
        }

        header.chunk_id = htonl(i);
        header.chunk_size = htonl(bytes_read);
        header.total_chunks = htonl(total_chunks);

        if (i == 0)
        {
            // header.filename = filename;
            strncpy(header.filename, filename, FILENAME_MAX_LEN - 1);
            header.filename[FILENAME_MAX_LEN - 1] = '\0';
        }
        else
        {
            // todo IF not first chunk set file name to "0"
            memset(header.filename, 0, FILENAME_MAX_LEN);
        }
        if (!sendChunk(sockfd, &header, buffer, bytes_read))
        {
            fprintf(stderr, "Failed to send chunk %d . ABORT", i);
            fclose(fp);
            return;
        }
        int percent = (i + 1) * 100 / total_chunks;
        progress_bar(percent);
    }
    printf("\n");

    printf("File succesfully sent.");
}

// Function to check if the server sent an error message
int check_for_error(int sockfd)
{
    char buffer[256];
    // Peek at the first few bytes without removing them from the socket buffer
    int n = recv(sockfd, buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (n > 0)
    {
        buffer[n] = '\0';
        if (strstr(buffer, "ERROR:") || strstr(buffer, "SUCCESS:"))
        {
            // It's a text message, not file data
            receive_output(sockfd);
            return 1; // Error or success message found
        }
    }
    return 0; // No error message, proceed with file transfer
}

// Recive Fİle
void receive_file(int sockfd)
{
    FILE *fp = NULL;
    char filename[FILENAME_MAX_LEN];
    int total_chunks = 0, received = 0;

    // Initialize filename buffer
    memset(filename, 0, FILENAME_MAX_LEN);

    while (1)
    {
        FileChunkHeader header;
        int n = recv(sockfd, &header, sizeof(header), MSG_WAITALL);
        if (n <= 0)
        {
            if (fp)
                fclose(fp);
            perror("Failed to receive file header");
            return;
        }

        int chunk_id = ntohl(header.chunk_id);
        int chunk_size = ntohl(header.chunk_size);
        total_chunks = ntohl(header.total_chunks);

        // Validate chunk_size to prevent buffer overflow
        if (chunk_size > CHUNK_SIZE || chunk_size <= 0)
        {
            if (fp)
                fclose(fp);
            fprintf(stderr, "Invalid chunk size: %d\n", chunk_size);
            return;
        }

        // Validate total_chunks
        if (total_chunks <= 0 || total_chunks > 1000000)
        {
            if (fp)
                fclose(fp);
            fprintf(stderr, "Invalid total chunks: %d\n", total_chunks);
            return;
        }

        if (chunk_id == 0)
        {
            // Ensure header.filename is null-terminated
            header.filename[FILENAME_MAX_LEN - 1] = '\0';
            strncpy(filename, header.filename, FILENAME_MAX_LEN - 1);
            filename[FILENAME_MAX_LEN - 1] = '\0';
            fp = fopen(filename, "wb");
            if (!fp)
            {
                perror("Failed to open file for writing");
                return;
            }
        }

        uint8_t buffer[CHUNK_SIZE];
        int r = recv(sockfd, buffer, chunk_size, MSG_WAITALL);
        if (r <= 0)
        {
            if (fp)
                fclose(fp);
            perror("Failed to receive chunk data");
            return;
        }

        if (fp)
        {
            fwrite(buffer, 1, r, fp);
        }
        else
        {
            fprintf(stderr, "File not opened, cannot write chunk %d\n", chunk_id);
            return;
        }
        received++;

        int percent = received * 100 / total_chunks;
        progress_bar(percent);

        if (received >= total_chunks)
            break;
    }

    if (fp)
    {
        fclose(fp);
        printf("\nFile received: %s\n", filename);
    }
    else
    {
        printf("Failed to receive file.\n");
    }
}

void receive_output(int sockfd)
{
    char buffer[512];
    int n;
    printf("\n--- Server Response ---\n");

    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[n] = '\0';
        printf("%s", buffer);

        // Check for specific termination markers
        if (strstr(buffer, "END_OF_LIST") ||
            strstr(buffer, "ERROR:") ||
            strstr(buffer, "SUCCESS:") ||
            strstr(buffer, "OK:"))
        {
            break;
        }

        // For directory listing, check if we received a path ending with newline
        if (buffer[n - 1] == '\n' && strchr(buffer, '/') != NULL)
        {
            break;
        }
    }
    printf("\n-----------------------\n");
}