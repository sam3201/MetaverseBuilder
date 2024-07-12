#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stdint.h>
#include "environment.h"
#include <netinet/in.h>

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
    struct sockaddr_in *addr;
    struct Server_t *server;
    Entity *player;
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
    Canvas *canvas;
    pthread_t gameLoopThread; 
} Server_t;

typedef struct {
  uint8_t x;
  uint8_t y;
  char c;
  Color color;
} CellUpdate;

Server_t *init_server(char *host, char *port);
Client_t *init_client(Server_t *server, int socket, struct sockaddr_in *addr);
Entity *create_player(Server_t *server, Color color);

void *handle_client(void *arg);
void start_server(Server_t *server);
void stop_server(Server_t *server);
void stop_client(Client_t *client);
Client_t *send_message(Client_t *client, char *msg);

void *gameLoopThread(void *arg);
void startGameLoop(Server_t *server, uint8_t rows, uint8_t cols);

void sendCanvasToClients(Server_t *server);
void sendCellUpdatesToClients(Server_t *server, CellUpdate *updates, size_t updateCount);

#endif

