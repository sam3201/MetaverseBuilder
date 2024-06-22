#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "utils/environment.h"

int gameLoop() {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    uint8_t frameRate = 30;

    uint8_t rows = 20;
    uint8_t cols = 50;
    Canvas *canvas = initCanvas(rows, cols, ' ');

    Color playerColor = {75, 2, 90};
    Entity *player = createEntity(PLAYER, 'o', cols / 2, rows / 2, 100, playerColor);
    addEntity(canvas, player);

    Color enemyColor = {0, 255, 0};
    uint8_t entityCount = 3;
    for (int i = 0; i < entityCount; i++) {
        Entity *enemy = createEntity(ENEMY, 'X', rand() % cols, rand() % rows, 3, enemyColor);
        addEntity(canvas, enemy);
    }

    char *text = "WASD: Move, Q: Quit";
    Color textColor = {0, 255, 255};
    size_t textEntityCount = strlen(text);
    Entity **textEntities = createText(text, 0, 0, textColor, &textEntityCount);
    for (size_t i = 0; i < textEntityCount; i++) {
        addEntity(canvas, textEntities[i]);
    }

    setRawMode(1);
    setupFrameTimer(frameRate);

    initializeEntityThreads(canvas, frameRate);

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

    return 0;
}

int main() {
    gameLoop();

    return 0;
}

