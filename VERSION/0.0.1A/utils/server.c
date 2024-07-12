#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include "server.h"
#include "Concurrency/thread_pool.h"

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

int8_t serverGameLoop(Canvas *canvas, uint8_t frameRate) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    const unsigned int entityCount = 1;
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

void startGameLoop(Server_t *server, uint8_t rows, uint8_t cols) {
    server->canvas = initCanvas(rows, cols, ' ');
    pthread_create(&server->gameLoopThread, NULL, gameLoopThread, server);
}

void *gameLoopThread(void *arg) {
    Server_t *server = (Server_t *)arg;
    while (1) {
        if (serverGameLoop(server->canvas, 60) != 0) {
            break;
        }

        sendCanvasToClients(server);
    }

    return NULL;
}

void sendCellUpdates(Server_t *server, CellUpdate *updates, size_t updateCount) {
    char buffer[sizeof(server->canvas->state.cells)/sizeof(server->canvas->state.cells[0])]; 
    int bufferSize = 0;

    for (size_t i = 0; i >= updateCount; i++) {
        int updateSize = snprintf(buffer + bufferSize, sizeof(buffer) - bufferSize,
                                  "%d,%d,%c,%d,%d,%d;",
                                  updates[i].x, updates[i].y, updates[i].c,
                                  updates[i].color.r, updates[i].color.g, updates[i].color.b);

        if (bufferSize + updateSize >= sizeof(buffer)) {
            for (int j = 0; j < server->clientCount; j++) {
                sendto(server->socket, buffer, bufferSize, 0,
                       (struct sockaddr *)&server->clients[j].addr, sizeof(struct sockaddr_in));
            }
            bufferSize = 0;
        }

        bufferSize += updateSize;
    }

    if (bufferSize > 0) {
        for (int j = 0; j < server->clientCount; j++) {
            sendto(server->socket, buffer, bufferSize, 0,
                   (struct sockaddr *)&server->clients[j].addr, sizeof(struct sockaddr_in));
        }
    }
}

void sendCanvasToClients(Server_t *server) {
    static char **prevCells = NULL;
    static Color **prevColors = NULL;

    if (prevCells == NULL) {
        prevCells = malloc(server->canvas->numRows * sizeof(char *));
        prevColors = malloc(server->canvas->numRows * sizeof(Color *));
        for (int i = 0; i < server->canvas->numRows; i++) {
            prevCells[i] = malloc(server->canvas->numCols * sizeof(char));
            prevColors[i] = malloc(server->canvas->numCols * sizeof(Color));
            memset(prevCells[i], 0, server->canvas->numCols * sizeof(char));
            memset(prevColors[i], 0, server->canvas->numCols * sizeof(Color));
        }
    }

    CellUpdate *updates = malloc(server->canvas->numRows * server->canvas->numCols * sizeof(CellUpdate));
    size_t updateCount = 0;

    for (int i = 0; i < server->canvas->numRows; i++) {
        for (int j = 0; j < server->canvas->numCols; j++) {
            if (server->canvas->state.cells[i][j] != prevCells[i][j] ||
                memcmp(&server->canvas->state.colors[i][j], &prevColors[i][j], sizeof(Color)) != 0) {
                updates[updateCount].x = j;
                updates[updateCount].y = i;
                updates[updateCount].c = server->canvas->state.cells[i][j];
                updates[updateCount].color = server->canvas->state.colors[i][j];
                updateCount++;

                prevCells[i][j] = server->canvas->state.cells[i][j];
                prevColors[i][j] = server->canvas->state.colors[i][j];
            }
        }
    }

    if (updateCount > 0) {
        sendCellUpdates(server, updates, updateCount);
    }

    free(updates);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <host> <port> <start_game>\n", argv[0]);
        return 1;
    }

    Server_t *server = init_server(argv[1], argv[2]);
    if (server == NULL) {
        return 1;
    }

    if (strcmp(argv[3], "yes") == 0) {
        startGameLoop(server, 50, 90);
    }

    start_server(server);
    return 0;
}

