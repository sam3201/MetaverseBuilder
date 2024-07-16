#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../utils/environment.h"
#include "../utils/NNS/NN.h"

#define MAX_SNAKES 1

typedef struct {
  NN_t nn;
  Entity entity;
} Snake;


Snake *create_snake() {
  Snake *snake = malloc(sizeof(Snake));
  if (snake == NULL) {
    return NULL;
  }

  snake->nn = NN_create(5, 20, 4, 0, 0);
  if (snake->nn == NULL) {
    free(snake);
    return NULL;
  }

  snake->entity.cell.pos.x = 5;
  snake->entity.cell.pos.y = 5;

  return snake;
}

