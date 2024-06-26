#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "server.h"

Server_t *init_server(char *host, char *port) {
  Server_t *server = malloc(sizeof(Server_t));
  server->host = host;
  server->port = port;
  server->clientCount = 0;
  server->id = 1;

  for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
    server->clients[i] = NULL;
  }

  return server;
}

void *handle_client(void *arg) {
  Client_t *client = (Client_t *)arg;
  char buffer[MAX_BUFFER_SIZE];
  int bytesRead;

  printf("Client %d connected.\n", client->id);

  while ((bytesRead = recv(client->socket, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
    buffer[bytesRead] = '\0';
    printf("Received from client %d: %s\n", client->id, buffer);

    send(client->socket, buffer, bytesRead, 0);
  }

  printf("Client %d disconnected.\n", client->id);
  close(client->socket);
  free(client);

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
    int client_socket;
    if ((client_socket = accept(server->socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
      perror("Accept failed");
      continue;
    }

    if (server->clientCount >= MAX_CLIENTS) {
      printf("Server is full\n");
      close(client_socket);
      continue;
    }

    Client_t *client = malloc(sizeof(Client_t));
    client->id = server->clientCount + 1;
    client->socket = client_socket;
    client->server = server;
    server->clients[server->clientCount++] = client;

    pthread_create(&client->thread, NULL, handle_client, (void *)client);
  }

  close(server->socket);
}

int main() {
  Server_t *server = init_server("127.0.0.1", "8080");
  start_server(server);

  return 0;
}
