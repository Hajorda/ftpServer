#include "client.h"
#include "colors.h"
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
        // Todo look and support arguments in the future
        if (!fgets(command, sizeof(command), stdin))
            break;
        command[strcspn(command, "\n")] = '\0'; // Remove newline
        // get
        if (strncmp(command, "get ", 4) == 0)
        {
            const char *filename = command + 4; // Extract filename
            if (strlen(filename) == 0)
            {
                printf(RED "Error: 'get' command requires a filename.\n" RESET);
                continue;
            }
            printf(GREEN "Getting the file: %s\n" RESET, filename);
            send(sock, command, strlen(command), 0);
            send(sock, "\n", 1, 0);
            if (!check_for_error(sock))
            {
                receive_file(sock);
            }
        }
        else if (strcmp(command, "get") == 0)
        {
            printf(RED "Error: 'get' command requires a filename.\n" RESET);
            continue;
        }
        // Upload
        else if (strncmp(command, "send ", 5) == 0)
        {
            const char *filename = command + 5; // Extract filename
            if (strlen(filename) == 0)
            {
                printf(RED "Error: 'send' command requires a filename.\n" RESET);
                continue;
            }
            printf(GREEN "Sending the file: %s\n" RESET, filename);
            send(sock, "upload", 6, 0);
            send(sock, "\n", 1, 0);
            send_file(filename, sock);
        }
        else if (strcmp(command, "send") == 0)
        {
            printf(RED "Error: 'send' command requires a filename.\n" RESET);
            continue;
        }
        // List files
        else if (strcmp(command, "list") == 0)
        {
            printf(BLUE "Listing files on the server...\n" RESET);
            send(sock, "ls", 2, 0);
            send(sock, "\n", 1, 0);
            receive_output(sock);
        }
        // Print working directory
        else if (strcmp(command, "pwd") == 0)
        {
            printf(BLUE "Getting current working directory...\n" RESET);
            send(sock, command, strlen(command), 0);
            send(sock, "\n", 1, 0);
            receive_output(sock);
        }
        // Change directory
        else if (strncmp(command, "cd ", 3) == 0)
        {
            const char *path = command + 3; // Extract path
            if (strlen(path) == 0)
            {
                printf(RED "Error: 'cd' command requires a directory path.\n" RESET);
                continue;
            }
            printf(GREEN "Changing directory to: %s\n" RESET, path);
            send(sock, command, strlen(command), 0);
            send(sock, "\n", 1, 0);
            receive_output(sock);
        }
        else if (strcmp(command, "cd") == 0)
        {
            printf(RED "Error: 'cd' command requires a directory path.\n" RESET);
            continue;
        }
        // Delete file
        else if (strncmp(command, "del ", 4) == 0)
        {
            const char *filename = command + 4; // Extract filename
            if (strlen(filename) == 0)
            {
                printf(RED "Error: 'del' command requires a filename.\n" RESET);
                continue;
            }
            printf(GREEN "Deleting file: %s\n" RESET, filename);
            send(sock, command, strlen(command), 0);
            send(sock, "\n", 1, 0);
            receive_output(sock);
        }
        else if (strcmp(command, "del") == 0)
        {
            printf(RED "Error: 'del' command requires a filename.\n" RESET);
            continue;
        }
        // help
        else if (strncmp(command, "help", 4) == 0)
        {
            printf(YELLOW "Available commands:\n" RESET);
            printf(CYAN "  get <filename> - Download a file from the server\n" RESET);
            printf(CYAN "  send <filename> - Upload a file to the server\n" RESET);
            printf(CYAN "  list - List files on the server\n" RESET);
            printf(CYAN "  pwd - Print current working directory on the server\n" RESET);
            printf(CYAN "  cd <directory> - Change directory on the server\n" RESET);
            printf(CYAN "  del <filename> - Delete a file on the server\n" RESET);
            printf(CYAN "  help - Show this help message\n" RESET);
            printf(CYAN "  clear - Clear the console\n" RESET);
            printf(CYAN "  exit - Exit the client\n" RESET);
        }
        // Clear
        else if (strcmp(command, "clear") == 0)
        {
            // Clear the console
            printf("\033[H\033[J");
        }
        // Exit
        else if (strcmp(command, "exit") == 0)
        {
            break;
        }
        else if (strcmp(command, "") == 0)
        {
            // Do nothing, just prompt again
            continue;
        }
        else
        {
            //  send(sock, command, strlen(command), 0);
            // send(sock, "\n", 1, 0);
            // receive_output(sock);
            printf("Unknown command: \"%s\". Use 'help' for a list of commands.\n", command);
        }
    }

    close(sock);
    return 0;
}