#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#if defined(__linux__)
#include <linux/input.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#endif


#include "type_system/type_system.h"

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

typedef struct Canvas Canvas;
typedef struct Entity Entity;

typedef struct Entity {
    TYPE type;
    unsigned int health;
    int8_t isAlive;
    Cell cell;
    Color color;
    pthread_t thread;
    void (*moveFunc)(Canvas *canvas, Entity *entity);
} Entity;

typedef struct {
    char **cells;
    Color **colors;
    Entity **entities;
    size_t entityCount;
    pthread_mutex_t lock;
} State;

typedef struct Canvas {
    uint8_t numRows;
    uint8_t numCols;
    State state;
} Canvas;

typedef struct EntityThreadArgs {
    Canvas *canvas;
    Entity *entity;
    void (*moveFunc)(Canvas *canvas, Entity *entity);
    uint8_t frameRate;
} EntityThreadArgs;

typedef struct {
  struct timespec lastUpdate;
  double deltaTime;
  double fixed_deltaTime;
  unsigned int frameCount;
  uint8_t fps;
} Clock;


void setRawMode(int8_t enable);
int kbhit(void);
void handleSignal(int signum);
void handleFrameUpdate(int signum);
void setupFrameTimer(int frameRate);

Canvas *initCanvas(uint8_t numRows, uint8_t numCols, char empty);
void drawBorder(Canvas *canvas);
void setColor(Color color);
void resetColor();
void clearCanvas(Canvas *canvas);
Canvas *resetCanvas(Canvas *canvas);

void printCanvas(Canvas *canvas);

Clock *createClock();
void initClock(Clock *clock, double fixed_update_rate, uint8_t fps);
void updateClock(Clock *clock);
int8_t fixedUpdateReady(Clock *clock);
void destroyClock(Clock *clock);

Entity *createEntity(TYPE type, char c, uint8_t x, uint8_t y, uint8_t health, Color color, void (*moveFunc)(Canvas *canvas, Entity *entity));
#define GENERATE_TYPE_MACROS(types) \
    do { \
        for (size_t i = 0; i < sizeof(types) / sizeof(TYPE); i++) { \
            printf("#define %s (%s)\n", types[i].name, types[i].name); \
        } \
    } while(0);

Entity **createText(char *text, uint8_t startX, uint8_t startY, Color color, size_t *entityCount);
Entity *createButton(char c, uint8_t x, uint8_t y);
void deleteEntity(Entity *entity);
void addEntity(Canvas *canvas, Entity *entity);
void drawEntities(Canvas *canvas);

void updateEntity(Entity *entity, Pos pos, Pos vel);

void moveEntity(Canvas *canvas, Entity *entity, Pos vel);
void movePlayer(Canvas *canvas, Entity *player, char dir);
void moveEnemy(Canvas *canvas, Entity *enemy);

void entityThreadFunc(EntityThreadArgs* args);
void initializeEntityThreads(TYPE type, Canvas *canvas, uint8_t frameRate);
void freeCanvas(Canvas *canvas);

int8_t GameLoop(int8_t addPlayer, uint8_t numRows, uint8_t numCols, double fixed_update_rate, uint8_t frameRate); 
void handleMouseEvents(Canvas *canvas);
Pos getMousePos();

extern int pipe_fd[2];
extern volatile sig_atomic_t frameFlag;

#endif

