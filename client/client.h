#ifndef CLIENT_H
#define CLIENT_H

int create_connection(const char *ip, int port);
void send_file(const char *filename, int sockfd);
void receive_file(int sockfd);
void receive_output(int sockfd);

#endif