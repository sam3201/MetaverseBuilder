#include "environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

int pipe_fd[2];
volatile sig_atomic_t frameFlag = 0;

Canvas *initCanvas(uint8_t numRows, uint8_t numCols, char empty) {
    Canvas *canvas = malloc(sizeof(Canvas));
    canvas->numRows = numRows;
    canvas->numCols = numCols;
    pthread_mutex_init(&canvas->state.lock, NULL);  

    if (!empty || empty == '\n' || empty == '\r' || empty == '\0') {
        empty = ' ';
    }

    canvas->state.cells = malloc(numRows * sizeof(char *));
    canvas->state.colors = malloc(numRows * sizeof(Color *));
    for (uint8_t i = 0; i < numRows; i++) {
        canvas->state.cells[i] = malloc(numCols * sizeof(char));
        canvas->state.colors[i] = malloc(numCols * sizeof(Color));
        for (uint8_t j = 0; j < numCols; j++) {
            canvas->state.cells[i][j] = empty;
            canvas->state.colors[i][j] = (Color){255, 255, 255};
        }
    }

    for (uint8_t i = 0; i < numCols; i++) {
        canvas->state.cells[0][i] = ' ';
        canvas->state.cells[numRows - 1][i] = ' ';
    }
    for (uint8_t i = 0; i < numRows; i++) {
        canvas->state.cells[i][0] = '|';
        canvas->state.cells[i][numCols - 1] = '|';
    }

    canvas->state.cells[0][0] = '+';
    canvas->state.cells[0][numCols - 1] = '+';
    canvas->state.cells[numRows - 1][0] = '+';
    canvas->state.cells[numRows - 1][numCols - 1] = '+';
    for (uint8_t i = 1; i < numCols - 1; i++) {
        canvas->state.cells[0][i] = '-';
    }
    for (uint8_t i = 1; i < numCols - 1; i++) {
        canvas->state.cells[numRows - 1][i] = '-';
    }

    canvas->state.entities = NULL;
    canvas->state.entityCount = 0;

    return canvas;
}

Entity *createEntity(Type type, char c, uint8_t x, uint8_t y, uint8_t health, Color color) {
    Entity *entity = malloc(sizeof(Entity));
    entity->type = type;
    entity->cell.c = c;
    entity->cell.pos.x = x;
    entity->cell.pos.y = y;
    entity->cell.color = color;
    entity->health = health;
    entity->isAlive = 1;
    return entity;
}

Entity **createText(char *text, uint8_t startX, uint8_t startY, Color color, size_t *entityCount) {
    size_t len = strlen(text);
    Entity **entities = malloc(len * sizeof(Entity *));
    for (size_t i = 0; i < len; i++) {
        entities[i] = createEntity(TEXT, text[i], startX + i, startY, 0, color);
    }
    *entityCount = len;
    return entities;
}

Entity *createButton(char c, uint8_t x, uint8_t y) {
    Color defaultColor = {255, 255, 255};
    Entity *entity = createEntity(BUTTON, c, x, y, 0, defaultColor);
    entity->isAlive = 1;

    return entity;
}

void deleteEntity(Entity *entity) {
    free(entity);
}

void addEntity(Canvas *canvas, Entity *entity) {
    pthread_mutex_lock(&canvas->state.lock);
    canvas->state.entities = realloc(canvas->state.entities, (canvas->state.entityCount + 1) * sizeof(Entity *));
    canvas->state.entities[canvas->state.entityCount++] = entity;
    pthread_mutex_unlock(&canvas->state.lock);
}

void drawEntities(Canvas *canvas) {
    for (uint8_t i = 1; i < canvas->numRows - 1; i++) {
        for (uint8_t j = 1; j < canvas->numCols - 1; j++) {
            canvas->state.cells[i][j] = ' ';
            canvas->state.colors[i][j] = (Color){255, 255, 255};
        }
    }

    pthread_mutex_lock(&canvas->state.lock);
    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        Entity *entity = canvas->state.entities[i];
        if (entity->isAlive) {
            canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
            canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.color;
        }
    }
    pthread_mutex_unlock(&canvas->state.lock);
}

void printCanvas(Canvas *canvas) {
    for (uint8_t i = 0; i < canvas->numRows; i++) {
        for (uint8_t j = 0; j < canvas->numCols; j++) {
            Color color = canvas->state.colors[i][j];
            printf("\033[38;2;%d;%d;%dm%c", color.r, color.g, color.b, canvas->state.cells[i][j]);
        }
        printf("\033[0m\n");
    }
}

void setRawMode(int enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

int kbhit(void) {
    int bytesWaiting;
    ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

void handleSignal(int signum) {
    if (signum == SIGINT) {
        exit(0);
    }
}

void handleFrameUpdate(int signum) {
    if (signum == SIGALRM) {
        frameFlag = 1;
    }
}

void setupFrameTimer(int frameRate) {
    struct itimerval timer;
    signal(SIGALRM, handleFrameUpdate);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000 / frameRate;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / frameRate;
    setitimer(ITIMER_REAL, &timer, NULL);
}

void updateEntity(Entity *entity, Pos pos, Pos vel) {
    entity->cell.pos.x = pos.x;
    entity->cell.pos.y = pos.y;
}

void movePlayer(Canvas *canvas, Entity *player, char dir) {
    Pos newPos = player->cell.pos;
    if (dir == 'w' && newPos.y > 1) {
        newPos.y--;
    } else if (dir == 's' && newPos.y < canvas->numRows - 2) {
        newPos.y++;
    } else if (dir == 'a' && newPos.x > 1) {
        newPos.x--;
    } else if (dir == 'd' && newPos.x < canvas->numCols - 2) {
        newPos.x++;
    }
    updateEntity(player, newPos, (Pos){0, 0});
}

void moveEntity(Canvas *canvas, Entity *entity, Pos vel) {
    if (!entity->isAlive) {
        return;
    }

    Pos oldPos = entity->cell.pos;
    oldPos.x += vel.x;
    oldPos.y += vel.y;
}

void moveEnemy(Canvas *canvas, Entity *enemy) {
    if (!enemy->isAlive) {
        return;
    }

    Pos newPos = enemy->cell.pos;
    int8_t dx = (rand() % 3) - 1; 
    int8_t dy = (rand() % 3) - 1;

    newPos.x += dx;
    newPos.y += dy;

    if (newPos.x > 0 && newPos.x < canvas->numCols - 1) {
        enemy->cell.pos.x = newPos.x;
    }
    if (newPos.y > 0 && newPos.y < canvas->numRows - 1) {
        enemy->cell.pos.y = newPos.y;
    }
}

void* entityThreadFunc(void* arg) {
    EntityThreadArgs *args = (EntityThreadArgs*)arg;
    uint8_t frameRate = (uint8_t)(uintptr_t)args->entity->buff.data;
    Canvas *canvas = args->canvas;
    Entity *entity = args->entity;

    while (entity->isAlive) {
        if (entity->type == ENEMY) {
            moveEnemy(canvas, entity); 
        }
        usleep(1000000 / frameRate);  
    }

    free(args);  
    return NULL;
}

void initializeEntityThreads(Canvas *canvas, uint8_t frameRate) {
    pthread_mutex_lock(&canvas->state.lock);
    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        Entity *entity = canvas->state.entities[i];
        if (entity->type == ENEMY) {
            EntityThreadArgs *args = malloc(sizeof(EntityThreadArgs));
            args->canvas = canvas;
            args->entity = entity;
            args->entity->buff.data = (void *)(uintptr_t)frameRate; 
            pthread_create(&args->entity->buff.thread, NULL, entityThreadFunc, (void *)args);
        }
    }
    pthread_mutex_unlock(&canvas->state.lock);
}

