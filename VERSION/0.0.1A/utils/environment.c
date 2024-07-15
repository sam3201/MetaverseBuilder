#include "environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#if defined(__linux__)
#includwe <linux/input.h>
#elif defined(__APPLE__) 
#include <ApplicationServices/ApplicationServices.h>
#endif

int pipe_fd[2];

Canvas *initCanvas(uint8_t rows, uint8_t cols, char defaultChar) {
  Canvas *canvas = (Canvas *)malloc(sizeof(Canvas));
  if (!canvas) return NULL;

  canvas->numRows = rows;
  canvas->numCols = cols;

  canvas->state.cells = (char **)malloc(rows * sizeof(char *));
  canvas->state.colors = (Color **)malloc(rows * sizeof(Color *));
  if (!canvas->state.cells || !canvas->state.colors) {
    freeCanvas(canvas);
    return NULL;
  }

  for (uint8_t i = 0; i < rows; i++) {
    canvas->state.cells[i] = (char *)malloc(cols * sizeof(char));
    canvas->state.colors[i] = (Color *)malloc(cols * sizeof(Color));
    if (!canvas->state.cells[i] || !canvas->state.colors[i]) {
      freeCanvas(canvas);
      return NULL;
    }
    for (uint8_t j = 0; j < cols; j++) {
      canvas->state.cells[i][j] = defaultChar;
      canvas->state.colors[i][j] = (Color){0, 0, 0};
    }
  }

  canvas->state.entityCount = 0;
  canvas->state.entities = NULL;
  pthread_mutex_init(&canvas->state.lock, NULL);

  return canvas;
}

void freeCanvas(Canvas *canvas) {
  if (!canvas) return;

  pthread_mutex_destroy(&canvas->state.lock);

  if (canvas->state.cells) {
    for (uint8_t i = 0; i < canvas->numRows; i++) {
      free(canvas->state.cells[i]);
    }
    free(canvas->state.cells);
  }

  if (canvas->state.colors) {
    for (uint8_t i = 0; i < canvas->numRows; i++) {
      free(canvas->state.colors[i]);
    }
    free(canvas->state.colors);
  }

  if (canvas->state.entities) {
    for (size_t i = 0; i < canvas->state.entityCount; i++) {
      free(canvas->state.entities[i]);
    }
    free(canvas->state.entities);
  }

  free(canvas);
}

void initClock(Clock *clock, double fixed_update_rate, uint8_t fps) {
  clock_gettime(CLOCK_MONOTONIC, &clock->lastUpdate);
  clock->deltaTime = 0.0f;
  clock->fixed_deltaTime = 1.0f / fixed_update_rate;
  clock->frameCount = 1.0f / fixed_update_rate;
  clock->fps = fps;
}

Clock *createClock() {
  Clock *clock = (Clock *)malloc(sizeof(Clock));
  if (!clock) return NULL;

  clock->fps = 0;
  return clock;
}

void destroyClock(Clock *clock) {
  free(clock);
}

void updateClock(Clock *clock) {
  struct timespec currentTime;
  clock_gettime(CLOCK_MONOTONIC, &currentTime);

  clock->deltaTime = (currentTime.tv_sec - clock->lastUpdate.tv_sec) + (currentTime.tv_nsec - clock->lastUpdate.tv_nsec) / 1e9;

  clock->lastUpdate = currentTime;
  clock->frameCount++;
}

int8_t fixedUpdateReady(Clock *clock) {
  static double accumlator = 0.0f;
  accumlator += clock->deltaTime;

  if (accumlator >= clock->fixed_deltaTime) {
    accumlator -= clock->fixed_deltaTime;
    return 1;
  }

  return 0;
}
void drawBorder(Canvas *canvas) {
  for (int i = 0; i < canvas->numCols; i++) {
    canvas->state.cells[0][i] = '-';
    canvas->state.cells[canvas->numRows - 1][i] = '-';
  }
  for (int i = 0; i < canvas->numRows; i++) {
    canvas->state.cells[i][0] = '|';
    canvas->state.cells[i][canvas->numCols - 1] = '|';
  }
}

void setColor(Color color) {
  printf("\033[38;2;%d;%d;%dm", color.r, color.g, color.b);
}

void resetColor() {
  printf("\033[0m");
}

void printCanvas(Canvas *canvas) {
  printf("\033[2J");
  uint8_t i, j;
  for (i = 0; i < canvas->numRows; i++) {
    for (j = 0; j < canvas->numCols; j++) {
      setColor(canvas->state.colors[i][j]);
      printf("%c", canvas->state.cells[i][j]);
      resetColor();
    }
    printf("\n");
  }
}

void clearCanvas(Canvas *canvas) {
  for (uint8_t i = 0; i < canvas->numRows; i++) {
    for (uint8_t j = 0; j < canvas->numCols; j++) {
      canvas->state.cells[i][j] = ' ';
      canvas->state.colors[i][j] = (Color){0, 0, 0};
    }
  }
}

Canvas *resetCanvas(Canvas *canvas) {
  for (uint8_t i = 0; i < canvas->numRows; i++) {
    for (uint8_t j = 0; j < canvas->numCols; j++) {
      canvas->state.cells[i][j] = ' ';
    }
  }
  drawBorder(canvas);
  return canvas;
}

Entity *createEntity(TYPE type, char c, uint8_t x, uint8_t y, uint8_t health, Color color, void (*moveFunc)(Canvas *canvas, Entity *entity)) {
  Entity *entity = (Entity *)malloc(sizeof(Entity));

  entity->type = type;
  entity->cell.c = c;
  entity->cell.pos.x = x;
  entity->cell.pos.y = y;
  entity->health = health;
  entity->isAlive = 1;
  entity->color = color;
  entity->moveFunc = moveFunc;

  return entity;
}

Entity **createText(char *text, uint8_t startX, uint8_t startY, Color color, size_t *entityCount) {
  size_t len = strlen(text);
  Entity **textEntities = (Entity **)malloc(len * sizeof(Entity *));
  *entityCount = len;

  for (size_t i = 0; i < len; i++) {
    textEntities[i] = createEntity((TYPE){"TEXT"}, text[i], startX + i, startY, 1, color, NULL);
  }

  return textEntities;
}

Entity *createButton(char c, uint8_t x, uint8_t y) {
  return createEntity((TYPE){"BUTTON"}, c, x, y, 1, (Color){0, 0, 0}, NULL);
}

void deleteEntity(Entity *entity) {
  free(entity);
}

void addEntity(Canvas *canvas, Entity *entity) {
  pthread_mutex_lock(&canvas->state.lock);
  Entity **newEntities = (Entity **)realloc(canvas->state.entities, (canvas->state.entityCount + 1) * sizeof(Entity *));
  if (newEntities) {
    canvas->state.entities = newEntities;
    canvas->state.entities[canvas->state.entityCount] = entity;
    canvas->state.entityCount++;
  }
  pthread_mutex_unlock(&canvas->state.lock);
}

void drawEntities(Canvas *canvas) {
  for (size_t i = 0; i < canvas->state.entityCount; i++) {
    Entity *entity = canvas->state.entities[i];
    canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
    canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
  }
}

void setRawMode(int8_t enable) {
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

volatile sig_atomic_t frameFlag = 0;

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

    if (entity->cell.pos.x > 0 && entity->cell.pos.x < canvas->numCols - 1 &&
        entity->cell.pos.y > 0 && entity->cell.pos.y < canvas->numRows - 1) {
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = ' ';
    }
    
    entity->cell.pos.x += vel.x;
    entity->cell.pos.y += vel.y;
    
    entity->cell.pos.x = ((entity->cell.pos.x - 1 + (canvas->numCols - 2)) % (canvas->numCols - 2)) + 1;
    entity->cell.pos.y = ((entity->cell.pos.y - 1 + (canvas->numRows - 2)) % (canvas->numRows - 2)) + 1;
    
    canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
    canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    
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
  moveEntity(canvas, enemy, (Pos){(rand() % 3) - 1, (rand() % 3) - 1}); 
}

void entityThreadFunc(EntityThreadArgs *args) {
  Canvas *canvas = args->canvas;
  Entity *entity = args->entity;
  uint8_t frameRate = args->frameRate;

  while (entity->isAlive) {
    entity->moveFunc(canvas, entity);
    usleep(1000000/frameRate);
  }

  free(args);
}

void initializeEntityThreads(TYPE type, Canvas *canvas, uint8_t frameRate) { 
  for (size_t i = 0; i < canvas->state.entityCount; i++) {
    if (canvas->state.entities[i]->type.name == type.name) {
      EntityThreadArgs *args = (EntityThreadArgs *)malloc(sizeof(EntityThreadArgs));
      args->canvas = canvas;
      args->entity = canvas->state.entities[i];
      args->frameRate = frameRate;
      args->moveFunc = canvas->state.entities[i]->moveFunc;
      pthread_create(&canvas->state.entities[i]->thread, NULL, (void *)entityThreadFunc, args);
    }
  }
}

int8_t GameLoop(int8_t addPlayer, uint8_t numRows, uint8_t numCols, double fixed_update_rate, uint8_t frameRate) {
    setupFrameTimer(frameRate);
    Canvas *canvas = initCanvas(numRows, numCols, ' '); 
    Clock *clock = createClock();
    initClock(clock, fixed_update_rate, frameRate);

    Entity *player = NULL;
    if (addPlayer) {
    Entity *player = createEntity((TYPE){"PLAYER"}, 'O', rand() & canvas->numCols, rand() & canvas->numRows, 1, (Color){255, 0, 255}, NULL); 
    addEntity(canvas, player);
    }

    while (1) {
        if (frameFlag) {
            frameFlag = 0;
            updateClock(clock);
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;  
                
                }
                  if (player != NULL) {
                    movePlayer(canvas, player, c);
                }

              
            }

              if (fixedUpdateReady(clock)) {

            for (size_t i = 0; i < canvas->state.entityCount; i++) {
                Entity *entity = canvas->state.entities[i];
                if (entity->moveFunc) {
                    entity->moveFunc(canvas, entity);  
                }
            }
      }

            pthread_mutex_lock(&canvas->state.lock);  
            drawEntities(canvas);
            drawBorder(canvas);
            pthread_mutex_unlock(&canvas->state.lock);  

            printf("\033[H\033[J");  
            printCanvas(canvas);
        }
    }

    return 0;  
}

#if defined(__linux__)
void handleMouseEvents(Canvas *canvas) {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Opening device");
        return;
    }

    struct input_event ie;
    Pos mousePos = {0, 0};

    while (read(fd, &ie, sizeof(struct input_event))) {
        if (ie.type == EV_REL) {
            if (ie.code == REL_X) mousePos.x += ie.value;
            if (ie.code == REL_Y) mousePos.y += ie.value;
        } else if (ie.type == EV_KEY && ie.code == BTN_LEFT && ie.value == 1) {
            printf("Mouse click at (%d, %d)\n", mousePos.x, mousePos.y);
        }
    }

    close(fd);
}

Pos getMousePos() {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Opening device");
        return (Pos){0, 0};
    }

    struct input_event ie;
    Pos mousePos = {0, 0};

    while (read(fd, &ie, sizeof(struct input_event))) {
        if (ie.type == EV_REL) {
            if (ie.code == REL_X) mousePos.x += ie.value;
            if (ie.code == REL_Y) mousePos.y += ie.value;
        }
    }

    close(fd);
    return mousePos;
}
#elif defined(__APPLE__)
void handleMouseEvents(Canvas *canvas) {
    CGEventRef event = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(event);
    printf("Mouse position: (%.0f, %.0f)\n", cursor.x, cursor.y);
    CFRelease(event);
}

Pos getMousePos() {
    CGEventRef event = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(event);
    Pos pos = {cursor.x, cursor.y};
    CFRelease(event);
    return pos;
}
#endif
