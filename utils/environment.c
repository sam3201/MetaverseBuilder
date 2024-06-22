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
    return entity;
}

void deleteEntity(Entity *entity) {
    free(entity);
}

void addEntity(Canvas *canvas, Entity *entity) {
    canvas->state.entities = realloc(canvas->state.entities, (canvas->state.entityCount + 1) * sizeof(Entity *));
    canvas->state.entities[canvas->state.entityCount++] = entity;
}

void drawEntities(Canvas *canvas) {
    for (uint8_t i = 1; i < canvas->numRows - 1; i++) {
        for (uint8_t j = 1; j < canvas->numCols - 1; j++) {
            canvas->state.cells[i][j] = ' ';
            canvas->state.colors[i][j] = (Color){255, 255, 255};
        }
    }

    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        Entity *entity = canvas->state.entities[i];
        if (entity->isAlive) {
            canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
            canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.color;
        }
    }
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
    return bytesWaiting > 0;
}

void handleSignal(int signum) {
    setRawMode(0);
    printf("\nExiting...\n");
    exit(signum);
}

void handleFrameUpdate(int signum) {
    frameFlag = 1;
}

void setupFrameTimer(int frameRate) {
    struct sigaction sa;
    struct itimerval timer;

    sa.sa_handler = &handleFrameUpdate;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000 / frameRate;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / frameRate;

    setitimer(ITIMER_REAL, &timer, NULL);
}

void updateEntity(Entity *entity, Pos pos, Pos vel) {
    entity->cell.pos.x += vel.x;
    entity->cell.pos.y += vel.y;
}

void movePlayer(Canvas *canvas, Entity *player, char dir) {
    Pos newPos = player->cell.pos;

    switch (dir) {
        case 'w':
            newPos.y -= 1;
            break;
        case 'a':
            newPos.x -= 1;
            break;
        case 's':
            newPos.y += 1;
            break;
        case 'd':
            newPos.x += 1;
            break;
    }

    if (newPos.x < 1 || newPos.x >= canvas->numCols - 1 || newPos.y < 1 || newPos.y >= canvas->numRows - 1) {
        return;
    }

    player->cell.pos = newPos;
}

void moveEnemy(Canvas *canvas, Entity *enemy) {
    uint8_t choice = rand() % 4;
    switch (choice) {
        case 0:
            if (enemy->cell.pos.x + 1 < canvas->numCols - 1 && enemy->cell.pos.y + 1 < canvas->numRows - 1) {
                enemy->cell.pos.x++;
                enemy->cell.pos.y++;
            }
            break;
        case 1:
            if (enemy->cell.pos.x + 1 < canvas->numCols - 1 && enemy->cell.pos.y - 1 > 0) {
                enemy->cell.pos.x++;
                enemy->cell.pos.y--;
            }
            break;
        case 2:
            if (enemy->cell.pos.x - 1 > 0 && enemy->cell.pos.y - 1 > 0) {
                enemy->cell.pos.x--;
                enemy->cell.pos.y--;
            }
            break;
        case 3:
            if (enemy->cell.pos.x - 1 > 0 && enemy->cell.pos.y + 1 < canvas->numRows - 1) {
                enemy->cell.pos.x--;
                enemy->cell.pos.y++;
            }
            break;
    }
}

