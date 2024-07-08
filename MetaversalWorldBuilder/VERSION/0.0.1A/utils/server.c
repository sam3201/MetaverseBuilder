#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "server.h"

#define MAX_BUFFER 1024
#define MAX_CLIENTS 10

Server_t *init_server(char *host, char *port) {
    Server_t *server = malloc(sizeof(Server_t));
    server->host = host;
    server->port = port;
    server->clientCount = 0;
    server->id = 1;
    server->clients = malloc(sizeof(Client_t) * MAX_CLIENTS);
    return server;
}

Client_t *init_client(Server_t *server, int socket) {
    Client_t *client = &server->clients[server->clientCount];
    client->socket = socket;
    client->server = server;
    client->id = server->clientCount + 1;
    return client;
}

Client_t *send_message(Client_t *client, char *msg) {
    if (send(client->socket, msg, strlen(msg), 0) < 0) {
        perror("Failed to send message");
    }
    return client;
}

void *handle_client(void *arg) {
    Client_t *client = (Client_t *)arg;
    char buffer[MAX_BUFFER];
    int bytesRead;

    printf("Client %d connected.\n", client->id);

    while ((bytesRead = recv(client->socket, buffer, MAX_BUFFER - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        printf("Client %d: %s\n", client->id, buffer);

        if (strcmp(buffer, "quit") == 0) {
            close(client->socket);
            client->socket = -1;
            free(client);
            return NULL;
        }
      
        if (client->id == 1) {
          if (strcmp(buffer, "stop") == 0) {
              stop_server(client->server);
              break; 
            }
           
           unsigned int kick_id; 
           if (sscanf(buffer, "kick %d", &kick_id) == 1) {
              for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
                if (client->server->clients[i].id == kick_id) {
                  close(client->server->clients[i].socket);
                  client->server->clients[i].socket = -1;
                  free(&client->server->clients[i]);
                  printf("Client %d kicked.\n", client->id);
                  if (kick_id == client->id) {
                    stop_server(client->server); 
                    return NULL;
                  }
                  break;
              }
            }

          } 
        }

        send_message(client, buffer);
    }

    printf("Client %d disconnected.\n", client->id);
    close(client->socket);
    client->socket = -1; 
    return NULL;
}

void start_server(Server_t *server) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(server->port));
    server_addr.sin_addr.s_addr = inet_addr(server->host);

    if (bind(server->socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server->socket);
        exit(1);
    }

    if (listen(server->socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server->socket);
        exit(1);
    }

    printf("Server started on %s:%s\n", server->host, server->port);

    while (1) {
        int client_socket = accept(server->socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        if (server->clientCount >= MAX_CLIENTS) {
            printf("Server is full\n");
            close(client_socket);
            continue;
        }

        Client_t *client = init_client(server, client_socket);
        server->clientCount++;

        if (pthread_create(&client->thread, NULL, handle_client, (void *)client) != 0) {
            perror("Failed to create client thread");
            close(client_socket);
            server->clientCount--;
            continue;
        }

        pthread_detach(client->thread);
    }
  stop_server(server);
}

void stop_server(Server_t *server) {
    printf("Stopping Server...\n");
    for (int i = 0; i < server->clientCount; i++) {
        if (server->clients[i].socket != -1) {
            close(server->clients[i].socket);
            printf("Client %d disconnected.\n", server->clients[i].id);
            pthread_cancel(server->clients[i].thread);
        }
    }
    close(server->socket);
    free(server->clients);
    free(server);

  printf("Server stopped\n");
}

int main(int argc, char **argv) {
  Server_t *server = init_server(argv[1], argv[2]);
  start_server(server);
  return 0;
}


