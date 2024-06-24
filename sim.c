#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "utils/NN.h"
#include "utils/environment.c"

#define MAX_WIDTH 20
#define MAX_HEIGHT 50

int run(uint8_t frameRate, uint8_t rows, uint8_t cols, uint8_t entityCount, Color enemyColor) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    Canvas *canvas = initCanvas(rows, cols, ' ');

    for (int i = 0; i < entityCount; i++) {
        Entity *enemy = createEntity(ENEMY, 'O', rand() % cols, rand() % rows, 3, enemyColor);
        addEntity(canvas, enemy);
    }

    char *title = "Sim";
    Color titleColor = {0, 255, 255};
    size_t titleLength = strlen(title);
    Entity **titleText = createText(title, 0, 0, titleColor, &titleLength); 
    for (size_t i = 0; i < titleLength; i++) {
        addEntity(canvas, titleText[i]);
    }

    setRawMode(1);
    setupFrameTimer(frameRate);

    initializeEntityThreads(canvas, frameRate);


    Tensor *tensors = tensor_new(2);

    while (1) {
        if (frameFlag) {
            frameFlag = 0;

            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;
                }
            }
            
            drawEntities(canvas);
            printf("\033[H\033[J");
            printCanvas(canvas);
        }
    }

    setRawMode(0);
    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        canvas->state.entities[i]->isAlive = 0;
        pthread_join(canvas->state.entities[i]->buff.thread, NULL);
        free(canvas->state.entities[i]);
    }
    free(canvas);
    free(titleText);

    return 0;
}

int main(int argc, char **argv) {
  run(60, MAX_WIDTH, MAX_HEIGHT, 5, (Color){255, 255, 0});
  return 0;
}



