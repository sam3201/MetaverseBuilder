#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../utils/environment.h"

typedef struct Snake Snake;

typedef struct Snake {
    Entity *segments;
    size_t length;
    void (*moveFunc)(Canvas* canvas, Entity *entity, char c);
    Snake *(*createSnake)(Canvas *canvas, TYPE type, char c, uint8_t x, uint8_t y, unsigned int health, Color color, TYPE parts[], size_t length, void (*moveFunc)(Canvas* canvas, Entity *entity, char c));
    TYPE parts[];
} Snake;

Snake *createSnake(Canvas *canvas, TYPE type, char c, uint8_t x, uint8_t y, unsigned int health, Color color, TYPE parts[], size_t length, void (*moveFunc)(Canvas* canvas, Entity *entity, char c)) {
    if (length <= 0) {
        fprintf(stderr, "Error: Length must be greater than 0\n");
        return NULL;
    }

    Snake *snake = (Snake *)malloc(sizeof(Snake));
    if (!snake) {
        fprintf(stderr, "Failed to allocate memory for snake\n");
        return NULL;
    }

    snake->segments = (Entity *)malloc(length * sizeof(Entity));
    if (!snake->segments) {
        fprintf(stderr, "Failed to allocate memory for snake segments\n");
        free(snake);
        return NULL;
    }

    snake->length = length;
    for (size_t i = 0; i < length; i++) {
      snake->parts[i] = parts[i];
    }
    snake->moveFunc = moveFunc;
    snake->createSnake = createSnake;

    for (size_t i = 0; i < length; i++) {
        snake->segments[i] = *createEntity(parts[i], c, x, y, health, color, (void *)snake->moveFunc);
        addEntity(canvas, &snake->segments[i]);
    }

    return snake;
}

void moveSnake(Canvas *canvas, Entity *entity, char c) {
    Pos vel = {0, 0};
    switch (c) {
        case 'w':
            vel.x = 0;
            vel.y = -1;
            break;
        case 'a':
            vel.x = -1;
            vel.y = 0;
            break;
        case 's':
            vel.x = 0;
            vel.y = 1;
            break;
        case 'd':
            vel.x = 1;
            vel.y = 0;
            break;
        default:
            vel.x = 0;
            vel.y = 0;
            break;

      moveEntity(canvas, entity, vel);
    }
}

int run(uint8_t frameRate, uint8_t rows, uint8_t cols, uint8_t entityCount, Color playerColor) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    TYPE SNAKE = addType("SNAKE");
    TYPE HEAD = addType("HEAD"); 
    TYPE BODY = addType("BODY");
    TYPE TAIL = addType("TAIL");
    TYPE ENEMY = addType("ENEMY");
  
    Canvas *canvas = initCanvas(rows, cols, ' ');
    if (!canvas) {
        fprintf(stderr, "Failed to initialize canvas\n");
        return 1;
    }

    for (int i = 0; i < entityCount; i++) {
        Entity *enemy = createEntity(ENEMY, 'X', (rand() % (cols - 2)) + 1, (rand() % (rows - 2)) + 1, 3, (Color){rand() % 256, rand() % 256, rand() % 256}, moveEnemy);
        if (enemy) {
            addEntity(canvas, enemy);
        } else {
            fprintf(stderr, "Failed to create enemy entity\n");
        }
    }

    Snake *snake = createSnake(canvas, SNAKE, '0', 30, 30, 1, (Color){30, 144, 255}, (TYPE[]){HEAD, BODY, TAIL}, 3, moveSnake);
    if (!snake) {
        fprintf(stderr, "Failed to create snake\n");
        freeCanvas(canvas);
        return 1;
    }

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
    initializeEntityThreads(ENEMY, canvas, frameRate);
    initializeEntityThreads(SNAKE, canvas, frameRate);

    while (1) {
        if (frameFlag) {
            frameFlag = 0;
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;
                }
                moveSnake(canvas, &snake->segments[0], c);
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

int main(void) {
  return run(30, 40, 50, 3, (Color){95, 144, 255}); 
}

