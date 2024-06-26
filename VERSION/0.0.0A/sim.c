#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "utils/NN.h"
#include "utils/environment.h"

void moveElement(Canvas *canvas, Entity *entity) {
    Pos vel = { (rand() % 3) - 1, (rand() % 3) - 1 };
    moveEntity(canvas, entity, vel);
}

int run(uint8_t frameRate, uint8_t rows, uint8_t cols, uint8_t entityCount, Color enemyColor, Color elementColor) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));
    
    Canvas *canvas = initCanvas(rows, cols, ' ');
    if (!canvas) {
        fprintf(stderr, "Failed to initialize canvas\n");
        return 1;
    }
    drawBorder(canvas);

    for (int i = 0; i < entityCount; i++) {
        Entity *enemy = createEntity(ENEMY, 'O', (rand() % (cols - 2)) + 1, (rand() % (rows - 2)) + 1, 3, enemyColor, moveEnemy);
        if (enemy) {
            addEntity(canvas, enemy);
        } else {
            fprintf(stderr, "Failed to create enemy entity\n");
        }
    }

    for (int i = 0; i < entityCount; i++) {
        Entity *element = createEntity(ELEMENT, '*', (rand() % (cols - 2)) + 1, (rand() % (rows - 2)) + 1, 1, elementColor, moveElement);
        if (element) {
            addEntity(canvas, element);
            Element *elementObj = new_element(canvas, element);
            // add attributes or properties to the element 
            // new_attribute(elementObj, "someKey", someValue);
            // new_property(elementObj, "someProperty", someValue);
        } else {
            fprintf(stderr, "Failed to create element entity\n");
        }
    }

    char *title = "Sim Q: QUIT";
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
    initializeEntityThreads(ELEMENT, canvas, frameRate);

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
            drawBorder(canvas);
            printf("\033[H");  
            printCanvas(canvas);
        }
        usleep(1000 / frameRate);  
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
    return run(60, 40, 50, 5, (Color){255, 0, 0}, (Color){255, 255, 0}); 
}
