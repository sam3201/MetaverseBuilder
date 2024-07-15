#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "../environment.h"

typedef struct Client_t {
    int socket;
    char *host;
    char *port;
    pthread_t thread;
    Canvas *canvas;
} Client_t;

Client_t *init_client(char *host, char *port);
void *receive_updates(void *arg);
void connect_to_server(Client_t *client);

#endif

