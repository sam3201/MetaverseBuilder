#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stdint.h>

#define MAX_CLIENTS 1
#define MAX_BUFFER_SIZE 1024

typedef struct {
    uint8_t id;
    void *data;
    pthread_t thread;
} Buffer_t;

typedef struct {
    void *val;
} Data_t;

typedef struct {
    uint8_t id;
    pthread_t thread;
    Data_t *data;
    Buffer_t *in;
    Buffer_t *out;
    int socket;
    struct Server_t *server;
} Client_t;

typedef struct {
    uint8_t id;
    char *host;
    char *port;
    Data_t *data;
    Buffer_t *in;
    Buffer_t *out;
    int socket;
    Client_t *clients[MAX_CLIENTS];
    uint8_t clientCount;
} Server_t;

Server_t *init_server(char *host, char *port);
Client_t *init_client(char *host, char *port);
void *handle_client(void *arg);
void start_server(Server_t *server);
void stop_server(Server_t *server);
void stop_client(Client_t *client);

#endif 

