#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "server.h"
#include "../Concurrency/thread_pool.h"

#define MAX_BUFFER 1024
#define MAX_CLIENTS 10

ThreadPool *pool;

Server_t *init_server(char *host, char *port) {
    Server_t *server = malloc(sizeof(Server_t));
    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket < 0) {
        perror("Socket creation failed");
        free(server);
        return NULL;
    }
    server->host = host;
    server->port = port;
    server->clientCount = 0;
    server->id = 1;
    server->clients = malloc(sizeof(Client_t) * MAX_CLIENTS);

    pool = threadPoolCreate(4, 10);

    return server;
}

Entity *create_player(Server_t *server, Color color) {
    Entity *player = malloc(sizeof(Entity));
    player->type = (TYPE){"PLAYER"};
    player->cell.pos.x = rand() % server->canvas->numCols;
    player->cell.pos.y = rand() % server->canvas->numRows;
    player->color = color;
    addEntity(server->canvas, player);
    return player;
}

Client_t *init_client(Server_t *server, int socket, struct sockaddr_in *client_addr) {
    Client_t *client = &server->clients[server->clientCount];
    client->player = create_player(server, (Color){rand() % 256, rand() % 256, rand() % 256});
    client->socket = socket;
    client->server = server;
    client->addr = client_addr;
    client->id = server->clientCount + 1;
    server->clientCount++;
    return client;
}

Client_t *send_message(Client_t *client, char *msg) {
    if (send(client->socket, msg, strlen(msg), 0) < 0) {
        perror("Failed to send message");
    }
    return client;
}

char *canvasToString(Canvas *canvas) {
    size_t bufferSize = canvas->numRows * (canvas->numCols + 1) + 1; 
    char *buffer = malloc(bufferSize);
    if (!buffer) {
        perror("Failed to allocate canvas buffer");
        return NULL;
    }

    char *ptr = buffer;
    for (int y = 0; y < canvas->numRows; y++) {
        for (int x = 0; x < canvas->numCols; x++) {
            *ptr++ = canvas->state.cells[y][x];
        }
        *ptr++ = '\n'; 
    }
    *ptr = '\0'; 

    return buffer;
}

void sendCanvasToClients(Server_t *server) {
    char *canvasStr = canvasToString(server->canvas); 
    for (int i = 0; i < server->clientCount; i++) {
        if (server->clients[i].socket != -1) {
            if (send(server->clients[i].socket, canvasStr, strlen(canvasStr), 0) < 0) {
                perror("Failed to send canvas");
            }
        }
    }
    free(canvasStr); 
}

void *handle_client(void *arg) {
    Client_t *client = (Client_t *)arg;
    char buffer[MAX_BUFFER];
    int bytesRead;

    printf("Client %d connected.\n", client->id);

    while ((bytesRead = recv(client->socket, buffer, MAX_BUFFER - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        printf("Client %d: %s\n", client->id, buffer);

        if (strcmp(buffer, "q") == 0) {
            close(client->socket);
            client->socket = -1;
            return NULL;
        }

        if (client->id == 1 && strcmp(buffer, "stop") == 0) {
            stop_server(client->server);
            break;
        }

        if (client->player) {
            if (strcmp(buffer, "w") == 0) {
                client->player->cell.pos.y--;
            } else if (strcmp(buffer, "s") == 0) {
                client->player->cell.pos.y++;
            } else if (strcmp(buffer, "a") == 0) {
                client->player->cell.pos.x--;
            } else if (strcmp(buffer, "d") == 0) {
                client->player->cell.pos.x++;
            }
        }

        sendCanvasToClients(client->server);
    }

    printf("Client %d disconnected.\n", client->id);
    close(client->socket);
    client->socket = -1;
    return NULL;
}

int8_t serverGameLoop(Canvas *canvas, uint8_t frameRate) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    Clock *clock = createClock();
     
    const unsigned int entityCount = 7;
    for (int i = 0; i < entityCount; i++) {
        Entity *enemy = createEntity((TYPE){"ENEMY"}, 'X', (rand() % (canvas->numCols - 2)) + 1, (rand() % (canvas->numRows - 2)) + 1, 3, (Color){255, 255, 255}, moveEnemy);
        if (enemy) {
            addEntity(canvas, enemy);
        } else {
            fprintf(stderr, "Failed to create enemy entity\n");
        }
    }

    Entity *p = createEntity((TYPE){"P"}, 'O', 5, 5, 1, (Color){255, 0, 0}, NULL);
    addEntity(canvas, p);

    char *title = "Game - WASD to move, Q to quit";
    Color titleColor = {0, 255, 255};
    size_t titleLength = strlen(title);
    Entity **titleText = createText(title, 1, 1, titleColor, &titleLength);
    if (titleText) {
        for (size_t i = 0; i < titleLength; i++) {
            addEntity(canvas, titleText[i]);
        }
        free(titleText);
    } else {
        fprintf(stderr, "Failed to create title text\n");
    }

    setRawMode(1);
    setupFrameTimer(frameRate);
    initializeEntityThreads((TYPE){"ENEMY"}, canvas, frameRate);

    while (1) {
        if (frameFlag) {
            frameFlag = 0;
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;
                }

                movePlayer(canvas, p, c);
            }
          
      
            updateClock(clock);

            drawEntities(canvas);
            drawBorder(canvas);
            printf("\033[H");
            printCanvas(canvas);
        }
    }

    setRawMode(0);

    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        canvas->state.entities[i]->isAlive = 0;
        pthread_join(canvas->state.entities[i]->thread, NULL);
    }

    freeCanvas(canvas);

    return 0;
}

void *gameLoopWrapper(void *arg) {
    Server_t *server = (Server_t *)arg;
    while (1) {
        if (serverGameLoop(server->canvas, 60) != 0) {
            break;
        }

        sendCanvasToClients(server);
    }

    return NULL;
}

void startGameLoop(Server_t *server, uint8_t rows, uint8_t cols) {
    server->canvas = initCanvas(rows, cols, ' ');
    pthread_create(&server->gameLoopThread, NULL, gameLoopWrapper, server);
}

void start_server(Server_t *server) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

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

    pthread_t gameLoopThread;
    pthread_create(&gameLoopThread, NULL, gameLoopWrapper, (void *)server);

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

        Client_t *client = init_client(server, client_socket, &client_addr);
        pthread_create(&client->thread, NULL, handle_client, (void *)client);
    }
}

void stop_server(Server_t *server) {
    close(server->socket);
    threadPoolDestroy(pool);
    exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    exit(1);
  }  

  Server_t *server = init_server(argv[1], argv[2]);
  if (!server) {
      exit(1);
  }

  startGameLoop(server, 20, 50);
  start_server(server);

  return 0;
}
