#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../utils/NN.h"

int run(uint8_t frameRate, uint8_t rows, uint8_t cols) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    Environment *env = environment_new(rows, cols);
    if (!env) {
        fprintf(stderr, "Failed to initialize environment\n");
        return 1;
    }

    Canvas *canvas = env->canvas;
    drawBorder(canvas);

    Element *element1 = element_new(canvas, 'o', (rand() % (cols - 2)) + 1, (rand() % (rows - 2)) + 1, (Color){(uint8_t)(rand() % 256), (uint8_t)(rand() % 256), (uint8_t)(rand() % 256)}, element_move);
    addEntity(canvas, element1->bounding);
    Element *element2 = element_new(canvas, 'o', (rand() % (cols - 2)) + 1, (rand() % (rows - 2)) + 1, (Color){(uint8_t)(rand() % 256), (uint8_t)(rand() % 256), (uint8_t)(rand() % 256)}, element_move);
    addEntity(canvas, element2->bounding);
      
    char *title = "Sim Q: QUIT";
    Color titleColor = {0, 255, 255};
    size_t titleLength = strlen(title);
    Entity **titleText = createText(title, 1, 1, titleColor, &titleLength);
    for (size_t i = 0; i < titleLength; i++) {
        addEntity(canvas, titleText[i]);
    }
    free(titleText);

    setRawMode(1);
    setupFrameTimer(frameRate);
    initializeEntityThreads(ELEMENT, canvas, frameRate);

    env->start = clock();

    while (1) {
        if (frameFlag) {
            frameFlag = 0;
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;
                }
            }

            env->end = clock();
            update_gravity_affected(env);
            double deltaTime = (double)(env->end - env->start) / CLOCKS_PER_SEC;
            printf("deltaTime: %f\n", deltaTime);

            apply_force(element1, calc_force(element1, element2), deltaTime);
            apply_force(element2, calc_force(element2, element1), deltaTime);
            
            drawEntities(canvas);
            drawBorder(canvas);
            printf("\033[H");  
            printCanvas(canvas);

            env->start = clock(); 
        }
    }
    
    setRawMode(0);
    for (size_t i = 0; i < canvas->state.entityCount; i++) {
        canvas->state.entities[i]->isAlive = 0;
        pthread_join(canvas->state.entities[i]->thread, NULL);
    }
    environment_free(env);

    return 0;
}

int main() {
  return run(60, 40, 50);
}
