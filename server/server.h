#ifndef SERVER_H
#define SERVER_H

void start_server(int port);
void send_error_to_client(int client_sock, const char *error_msg);

#endif