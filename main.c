#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "utils/environment.h"

int main() {
    signal(SIGINT, handleSignal);
    srand((unsigned) time(NULL));

    uint8_t frameRate = 30;

    uint8_t rows = 20;
    uint8_t cols = 50;
    Canvas *canvas = initCanvas(rows, cols, ' ');

    Color playerColor = {75, 2, 90};
    Entity *player = createEntity(PLAYER, 'o', cols / 2, rows / 2, 100, playerColor);
    addEntity(canvas, player);

    Color enemyColor = {0, 255, 0};
    for (int i = 0; i < 10; i++) {
        Entity *enemy = createEntity(ENEMY, 'X', rand() % cols, rand() % rows, 3, enemyColor);
        addEntity(canvas, enemy);
    }

    Color textColor = {0, 255, 255};
    size_t textEntityCount;
    Entity **textEntities = createText("WASD: Move, Q: Quit", 0, 0, textColor, &textEntityCount);
    for (size_t i = 0; i < textEntityCount; i++) {
        addEntity(canvas, textEntities[i]);
    }

    setRawMode(1);
    setupFrameTimer(frameRate);

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

            for (size_t i = 1; i < canvas->state.entityCount; i++) {
                Entity *entity = canvas->state.entities[i];
                if (entity->type == ENEMY) {
                    moveEnemy(canvas, entity);
                }
            }

            drawEntities(canvas);
            printf("\033[H\033[J");
            printCanvas(canvas);
        }
    }

    setRawMode(0);
    return 0;
}

