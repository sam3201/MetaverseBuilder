#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include "client.h"
#include "../Concurrency/thread_pool.h"

#define MAX_BUFFER 1024

ThreadPool *pool;

Client_t *init_client(char *host, char *port) {
    Client_t *client = malloc(sizeof(Client_t));
    client->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket < 0) {
        perror("Socket creation failed");
        free(client);
        return NULL;
    }
    client->host = host;
    client->port = port;

    pool = threadPoolCreate(4, 10);

    return client;
}

void *receive_updates(void *arg) {
    Client_t *client = (Client_t *)arg;
    char buffer[MAX_BUFFER];
    int bytesRead;

    while (1) {
        bytesRead = recv(client->socket, buffer, MAX_BUFFER - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            printf("\033[H\033[J"); 
            printf("%s", buffer); 
        } else if (bytesRead == 0) {
            printf("Server disconnected.\n");
            break;
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("Failed to receive updates");
            break;
        }
    }

    close(client->socket);
    return NULL;
}

void connect_to_server(Client_t *client) {
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(client->port));
    server_addr.sin_addr.s_addr = inet_addr(client->host);

    if (connect(client->socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client->socket);
        free(client);
        exit(1);
    }

    int flags = fcntl(client->socket, F_GETFL, 0);
    fcntl(client->socket, F_SETFL, flags | O_NONBLOCK);

    printf("Connected to server %s:%s\n", client->host, client->port);

    pthread_create(&client->thread, NULL, receive_updates, (void *)client);

    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

    char input;
    while (1) {
        int result = read(STDIN_FILENO, &input, 1);
        if (result == 1) {
            if (input == 'q') {
                break;
            }

            if (send(client->socket, &input, 1, 0) < 0) {
                perror("Failed to send message");
                break;
            }
        }
        usleep(10000); 
    }

    pthread_cancel(client->thread);
    close(client->socket);
    free(client);

    printf("Disconnected from server.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }

    char *host = argv[1];
    char *port = argv[2];

    Client_t *client = init_client(host, port);
    if (!client) {
        fprintf(stderr, "Failed to initialize client\n");
        exit(1);
    }

    connect_to_server(client);

    return 0;
}

