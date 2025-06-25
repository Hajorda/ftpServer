#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

// Lib's for socket
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// DEFINE
#define CHUNK_SIZE 512
#define FILENAME_MAX_LEN 64
#define MAX_RETRIES 5
#define ACK_BUF_SIZE 4

typedef struct
{
    __uint32_t chunk_id;
    __uint32_t chunk_size;
    __uint32_t total_chunks;
    char filename[FILENAME_MAX_LEN];
} FileChunkHeader; // ! __attribute__((packed)) FileChunkHeader; It's not portable.

// PRogress bar
const int progressBarLength = 30;

void progressBar(int doneTime)
{
    int numChar = doneTime * progressBarLength / 100;

    printf("\r[");
    for (int i = 0; i < numChar; i++)
    {
        printf("#");
    }
    for (int j = 0; j < progressBarLength - numChar; j++)
    {
        printf(" ");
    }
    printf("] %d%% done.", doneTime);
    // TODO WHat is this?
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
void sendFile(const char *filename, int sockfd)
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
        progressBar(percent);
        // ! REMOVE REMOVE REMOVE
        usleep(20000);
    }
    printf("\n");

    printf("File succesfully sent.");
}

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        perror("ERROR: Socket creation failed.");
        exit(EXIT_FAILURE);
    };

    //! TODO Look
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to server failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Send file using Client Socket

    sendFile("cat.jpeg", clientSocket);
    printf("FINISH");
    close(clientSocket);
    return 0;
}