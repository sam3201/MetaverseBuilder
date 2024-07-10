#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stdint.h>

typedef struct {
    uint8_t id;
    void *data;
    pthread_t thread;
} Buffer_t;

typedef struct {
    uint8_t id;
    pthread_t thread;
    Buffer_t *in;
    Buffer_t *out;
    int socket;
    struct Server_t *server;
} Client_t;

typedef struct Server_t {
    uint8_t id;
    char *host;
    char *port;
    Buffer_t *in;
    Buffer_t *out;
    int socket;
    Client_t *clients;
    uint8_t clientCount;
} Server_t;

Server_t *init_server(char *host, char *port);
Client_t *init_client(Server_t *server, int socket);
void *handle_client(void *arg);
void start_server(Server_t *server);
void stop_server(Server_t *server);
void stop_client(Client_t *client);
Client_t *send_message(Client_t *client, char *msg);
#endif 

