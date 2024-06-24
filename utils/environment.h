#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdint.h>
#include <pthread.h>
#include <signal.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

typedef struct {
    uint8_t x;
    uint8_t y;
} Pos;

typedef struct {
    char c;
    Pos pos;
    Color color;
} Cell;

typedef struct {
    int fd;
    int read;
    int write;
    int execute;
    int pos;
    int end;
} Channel;

typedef struct {
    void *data;
    pthread_t thread;
    Channel *in;
    Channel *out;
    Channel *err;
} Buffer;

typedef enum {
    PLAYER = 0,
    ENEMY = 1,
    TENSOR = -1,
    MATRIX = -2,
    VECTOR = -3,
    BUTTON = 2,
    TEXT = 3,
} Type;

typedef struct {
    Type type;
    uint8_t health;
    int8_t isAlive;
    Cell cell;
    Buffer buff;
    Color color;
} Entity;

typedef struct {
    char **cells;
    Color **colors;
    Entity **entities;
    size_t entityCount;
    pthread_mutex_t lock; 
} State;

typedef struct {
    uint8_t numRows;
    uint8_t numCols;
    State state;
} Canvas;

typedef struct {
    Canvas *canvas;
    Entity *entity;
} EntityThreadArgs;

Canvas *initCanvas(uint8_t numRows, uint8_t numCols, char empty);
Entity *createEntity(Type type, char c, uint8_t x, uint8_t y, uint8_t health, Color color);
Entity **createText(char *text, uint8_t startX, uint8_t startY, Color color, size_t *entityCount);
Entity *createButton(char c, uint8_t x, uint8_t y);
void deleteEntity(Entity *entity);
void addEntity(Canvas *canvas, Entity *entity);
void drawEntities(Canvas *canvas);
void printCanvas(Canvas *canvas);
void setRawMode(int enable);
int kbhit(void);
void handleSignal(int signum);
void handleFrameUpdate(int signum);
void setupFrameTimer(int frameRate);
void updateEntity(Entity *entity, Pos pos, Pos vel);
void moveEntity(Canvas *canvas, Entity *entity, Pos vel);
void movePlayer(Canvas *canvas, Entity *player, char dir);
void moveEnemy(Canvas *canvas, Entity *enemy);
void* entityThreadFunc(void* arg); 
void initializeEntityThreads(Canvas *canvas, uint8_t frameRate); 

extern int pipe_fd[2];
extern volatile sig_atomic_t frameFlag;

#endif

