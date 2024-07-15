#ifndef CITE_H
#define CITE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>

void init_cite_server(const char* host, const char* port);
void start_cite_server();
void handle_client(int client_socket);
char* base64_encode(const unsigned char* input, int length);

#endif
