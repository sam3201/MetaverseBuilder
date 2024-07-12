#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../utils/environment.h"

int run(uint8_t frameRate, uint8_t rows, uint8_t cols, uint8_t entityCount, Color playerColor) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));
    
  
    Canvas *canvas = initCanvas(rows, cols, ' ');
    if (!canvas) {
        fprintf(stderr, "Failed to initialize canvas\n");
        return 1;
    }

    for (int i = 0; i < entityCount; i++) {
        Entity *enemy = createEntity((TYPE){"ENEMY"}, 'X', (rand() % (cols - 2)) + 1, (rand() % (rows - 2)) + 1, 3, (Color){255, 255, 255}, moveEnemy);
        if (enemy) {
            addEntity(canvas, enemy);
        } else {
            fprintf(stderr, "Failed to create enemy entity\n");
        }
    }
    
    Entity *player = createEntity((TYPE){"PLAYER"}, 'O', 1, 1, 1, playerColor, NULL);
    if (player) {
        addEntity(canvas, player);
    } else {
        fprintf(stderr, "Failed to create player entity\n");
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
    initializeEntityThreads((TYPE){"ENEMY"}, canvas, frameRate);
    initializeEntityThreads((TYPE){"SNAKE"}, canvas, frameRate);

    while (1) {
        if (frameFlag) {
            frameFlag = 0;
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;
                }
                movePlayer(canvas, player, c);
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
  return run(30, 70, 150, 3, (Color){95, 144, 255}); 
}

