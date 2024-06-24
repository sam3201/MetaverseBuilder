#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include "environment.h"

Canvas *initCanvas(uint8_t numRows, uint8_t numCols, char empty) {
    Canvas *canvas = (Canvas *)malloc(sizeof(Canvas));
    canvas->numRows = numRows;
    canvas->numCols = numCols;
    canvas->state.entityCount = 0;
    pthread_mutex_init(&canvas->state.lock, NULL); 

    canvas->state.cells = (char **)malloc(numRows * sizeof(char *));
    canvas->state.colors = (Color **)malloc(numRows * sizeof(Color *));
    canvas->state.entities = NULL;

    for (uint8_t i = 0; i < numRows; i++) {
        canvas->state.cells[i] = (char *)malloc(numCols * sizeof(char));
        canvas->state.colors[i] = (Color *)malloc(numCols * sizeof(Color));
        for (uint8_t j = 0; j < numCols; j++) {
            canvas->state.cells[i][j] = empty;
            canvas->state.colors[i][j] = (Color){255, 255, 255};
        }
    }

    return canvas;
}

Entity *createEntity(Type type, char c, uint8_t x, uint8_t y, uint8_t health, Color color) {
    Entity *entity = (Entity *)malloc(sizeof(Entity));
    entity->type = type;
    entity->cell.c = c;
    entity->cell.pos.x = x;
    entity->cell.pos.y = y;
    entity->health = health;
    entity->isAlive = 1;
    entity->color = color;

    return entity;
}

Entity **createText(char *text, uint8_t startX, uint8_t startY, Color color, size_t *entityCount) {
    size_t len = strlen(text);
    Entity **textEntities = (Entity **)malloc(len * sizeof(Entity *));
    *entityCount = len;

    for (size_t i = 0; i < len; i++) {
        textEntities[i] = createEntity(TEXT, text[i], startX + i, startY, 1, color);
    }

    return textEntities;
}

Entity *createButton(char c, uint8_t x, uint8_t y) {
    return createEntity(BUTTON, c, x, y, 1, (Color){0, 0, 0});
}

void deleteEntity(Entity *entity) {
    free(entity);
}

void addEntity(Canvas *canvas, Entity *entity) {
    pthread_mutex_lock(&canvas->state.lock); 
    canvas->state.entities = (Entity **)realloc(canvas->state.entities, (canvas->state.entityCount + 1) * sizeof(Entity *));
    canvas->state.entities[canvas->state.entityCount] = entity;
    canvas->state.entityCount++;
    pthread_mutex_unlock(&canvas->state.lock); 
}

void drawEntities(Canvas *canvas) {
    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        Entity *entity = canvas->state.entities[i];
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
}

void printCanvas(Canvas *canvas) {
    system("clear");
    for (uint8_t i = 0; i < canvas->numRows; i++) {
        for (uint8_t j = 0; j < canvas->numCols; j++) {
            printf("%c", canvas->state.cells[i][j]);
        }
        printf("\n");
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
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void handleSignal(int signum) {
    if (signum == SIGALRM) {
        frameFlag = 1;
    }
}

void handleFrameUpdate(int signum) {
    frameFlag = 1;
}

void setupFrameTimer(int frameRate) {
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000 / frameRate;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / frameRate;

    signal(SIGALRM, handleFrameUpdate);
    setitimer(ITIMER_REAL, &timer, NULL);
}

void updateEntity(Entity *entity, Pos pos, Pos vel) {
    entity->cell.pos.x += vel.x;
    entity->cell.pos.y += vel.y;
}

void moveEntity(Canvas *canvas, Entity *entity, Pos vel) {
    pthread_mutex_lock(&canvas->state.lock); 
    canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = ' ';
    entity->cell.pos.x += vel.x;
    entity->cell.pos.y += vel.y;
    canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
    pthread_mutex_unlock(&canvas->state.lock); 
}

void movePlayer(Canvas *canvas, Entity *player, char dir) {
    Pos vel = {0, 0};
    switch (dir) {
        case 'w': vel.y = -1; break;
        case 's': vel.y = 1; break;
        case 'a': vel.x = -1; break;
        case 'd': vel.x = 1; break;
    }

    moveEntity(canvas, player, vel);
}

void moveEnemy(Canvas *canvas, Entity *enemy) {
    Pos vel = { (rand() % 3) - 1, (rand() % 3) - 1 };
    moveEntity(canvas, enemy, vel);
}

void* entityThreadFunc(void* arg) {
    EntityThreadArgs *args = (EntityThreadArgs *)arg;
    Canvas *canvas = args->canvas;
    Entity *entity = args->entity;

    while (entity->isAlive) {
        if (entity->type == ENEMY) {
            moveEnemy(canvas, entity);
        }
        usleep(100000);
    }

    return NULL;
}

void initializeEntityThreads(Canvas *canvas, uint8_t frameRate) {
    pthread_t *threads = (pthread_t *)malloc(canvas->state.entityCount * sizeof(pthread_t));

    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        EntityThreadArgs *args = (EntityThreadArgs *)malloc(sizeof(EntityThreadArgs));
        args->canvas = canvas;
        args->entity = canvas->state.entities[i];

        pthread_create(&threads[i], NULL, entityThreadFunc, args);
    }
}

