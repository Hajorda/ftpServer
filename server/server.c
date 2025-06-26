#include "server.h"
#include "commands.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_error_to_client(int client_sock, const char *error_msg)
{
    if (client_sock >= 0)
    {
        char full_msg[256];
        snprintf(full_msg, sizeof(full_msg), "SERVER_ERROR: %s\n", error_msg);
        send(client_sock, full_msg, strlen(full_msg), 0);
    }
}

void start_server(int port)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        printf(RED "Server socket creation failed\n" RESET);
        perror("socket");
        exit(1);
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        printf(YELLOW "Warning: Could not set SO_REUSEADDR\n" RESET);
        perror("setsockopt");
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf(RED "Server socket binding failed on port %d\n" RESET, port);
        perror("bind");
        close(server_sock);
        exit(1);
    }

    if (listen(server_sock, 5) < 0)
    {
        printf(RED "Server socket listening failed\n" RESET);
        perror("listen");
        close(server_sock);
        exit(1);
    }

    printf(GREEN "Server listening on port %d...\n" RESET, port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        if (client_sock < 0)
        {
            printf(RED "Server socket accept failed\n" RESET);
            perror("accept");
            continue; // Continue accepting other clients instead of exiting
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf(BLUE "Client connected from IP: %s\n" RESET, client_ip);

        // Handle client in a separate process or thread (for now, handle sequentially)
        handle_client(client_sock);

        printf(YELLOW "Client %s disconnected\n" RESET, client_ip);
        close(client_sock);
    }

    close(server_sock);
}