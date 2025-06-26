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
        printf("#");
    for (int i = filled; i < length; i++)
        printf(" ");
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

// Recive Fİle
void receive_file(int sockfd)
{
    FILE *fp = NULL;
    char filename[FILENAME_MAX_LEN];
    int total_chunks = 0, received = 0;

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

        if (chunk_id == 0)
        {
            strncpy(filename, header.filename, FILENAME_MAX_LEN);
            fp = fopen(filename, "wb");
            if (!fp)
                break;
        }

        uint8_t buffer[CHUNK_SIZE];
        int r = recv(sockfd, buffer, chunk_size, MSG_WAITALL);
        if (r <= 0)
            break;

        fwrite(buffer, 1, r, fp);
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
    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT)) > 0)
    {
        buffer[n] = '\0';
        printf("%s", buffer);
        if (n < (int)(sizeof(buffer) - 1))
            break;
    }
    printf("\n-----------------------\n");
}