#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = create_connection("127.0.0.1", 8080);

    if (sock < 0)
    {
        fprintf(stderr, "Failed to connect to server.\n");
        return 1;
    }

    char command[128];
    while (1)
    {
        printf("ftp> ");
        fflush(stdout);
        // Todo look
        if (!fgets(command, sizeof(command), stdin))
            break;
        command[strcspn(command, "\n")] = '\0'; // Remove newline
        // get
        if (strncmp(command, "get ", 4) == 0)
        {
            send(sock, command, strlen(command), 0);
            receive_file(sock);
        }
        // Upload
        else if (strncmp(command, "upload ", 7) == 0)
        {
            send(sock, command, strlen(command), 0);
            const char *filename = command + 7;
            send_file(filename, sock);
        }
        // help
        else if (strcmp(command, "help") == 0)
        {
            printf("Available commands:\n");
            printf("  get <filename> - Download a file from the server\n");
            printf("  upload <filename> - Upload a file to the server\n");
            printf("  list - List files on the server\n");
            printf("  pwd - Print current working directory on the server\n");
            printf("  cd <directory> - Change directory on the server\n");
            printf("  del <filename> - Delete a file on the server\n");
            printf("  exit - Exit the client\n");
        }
        // Exit
        else if (strcmp(command, "exit") == 0)
        {
            break;
        }
        else
        {
            send(sock, command, strlen(command), 0);
            send(sock, "\n", 1, 0);
            receive_output(sock);
        }
    }

    close(sock);
    return 0;
}