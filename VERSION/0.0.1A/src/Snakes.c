#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include "../utils/environment.h"
#include "../utils/NNS/NN.h"

typedef struct {
   NN_t *nn;
   Entity *entity;
   Pos direction;
   unsigned int score;
} Snake;

typedef struct {
  Entity *apple; 
} Apple;

Apple *createApple(Canvas *canvas) {
  Apple *apple = malloc(sizeof(Apple));
  apple->apple = createEntity((TYPE){"APPLE"}, 'O', canvas->numCols/2, canvas->numRows/2, 1, (Color){0,255,0}, NULL);
  apple->apple->cell.pos.x = rand() % canvas->numCols;
  apple->apple->cell.pos.y = rand() % canvas->numRows;
  addEntity(canvas, apple->apple);
  printf("Apple created at: (%d, %d)\n", apple->apple->cell.pos.x, apple->apple->cell.pos.y); 
  return apple;
}

Apple *destroyApple(Apple *apple) {
  free(apple);
  return NULL;
}

Snake *createSnake(Canvas *canvas, Entity *entity, Pos direction) {
   Snake *snake = malloc(sizeof(Snake));
   ActivationFunction hiddenFunctions[12];
   ActivationFunction hiddenDerivatives[12];
   for (size_t i = 0; i < 12; i++) {
     hiddenFunctions[i] = sigmoid;
     hiddenDerivatives[i] = sigmoid_derivative;
   }
   ActivationFunction outputFunction = sigmoid;
   ActivationFunction outputDerivative = sigmoid_derivative;

   snake->nn = NN_create(10, 12, 1, hiddenFunctions, hiddenDerivatives, &outputFunction, &outputDerivative, 0.1, 0.1);
   snake->entity = entity;
   snake->direction = direction;
   snake->score = 0;
   addEntity(canvas, entity);
   return snake;
}

Snake *destroySnake(Snake *snake) {
  free(snake);
  free(snake->nn);
  free(snake->entity);
  return NULL;
}

double *calcFitness(Snake *snake, Entity *target) {
  double fitness = sqrt(pow(snake->entity->cell.pos.x - target->cell.pos.x, 2) + pow(snake->entity->cell.pos.y - target->cell.pos.y, 2));
  return &fitness;
}

void updateSnake(Canvas *canvas, Snake *snake, Entity *target) {
  size_t numCells = sizeof(canvas->state.cells)/sizeof(Cell);
  double inputs[10] = {
    snake->entity->cell.pos.x,
    snake->entity->cell.pos.y,
    target->cell.pos.x,
    target->cell.pos.y, 
    numCells, 
    canvas->numRows,
    canvas->numCols,
    snake->direction.x,
    snake->direction.y,
    snake->score
  };

  forward(snake->nn, inputs);
  if (snake->nn->output[0] == 0.25) {
    snake->direction = (Pos){1, 0};
  } else if (snake->nn->output[0] == 0.5) {
    snake->direction = (Pos){-1, 0};
  } else if (snake->nn->output[0] == 0.75) {
    snake->direction = (Pos){0, 1};
  } else if (snake->nn->output[0] == 1) {
    snake->direction = (Pos){0, -1};
  }
  moveEntity(canvas, snake->entity, snake->direction);
  if (snake->entity->cell.pos.x < 0) {
    snake->entity->cell.pos.x = canvas->numCols - 1;
  } else if (snake->entity->cell.pos.x >= canvas->numCols) {
    snake->entity->cell.pos.x = 0;
  }

  if (snake->entity->cell.pos.y < 0) {
    snake->entity->cell.pos.y = canvas->numRows - 1;
  } else if (snake->entity->cell.pos.y >= canvas->numRows) {
    snake->entity->cell.pos.y = 0;
  }

  if (sqrt(snake->entity->cell.pos.x - target->cell.pos.x) + sqrt(snake->entity->cell.pos.y - target->cell.pos.y) <= 1) {
    target->cell.pos.x = rand() % canvas->numCols;
    target->cell.pos.y = rand() % canvas->numRows;
    snake->score++;
  }

  backprop(snake->nn, calcFitness(snake, target));
}

int run(uint8_t frameRate, uint8_t rows, uint8_t cols) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    Canvas *canvas = initCanvas(rows, cols, ' ');
    if (!canvas) {
        fprintf(stderr, "Failed to initialize canvas\n");
        return 1;
    }

    Snake *snake1 = createSnake(canvas, createEntity((TYPE){"SNAKE1"}, 'O', 0, 0, 1, (Color){255,0,0}, NULL), (Pos){1, 0});
    Snake *snake2 = createSnake(canvas, createEntity((TYPE){"SNAKE2"}, 'O', canvas->numCols - 1, canvas->numRows - 1, 1, (Color){0,0,255}, NULL), (Pos){-1, 0});

    Clock *clock = createClock();
    initClock(clock, 60, 60); 

    setRawMode(1);
    setupFrameTimer(frameRate);

    while (1) {
        if (kbhit()) {
            char c = getchar();
            if (c == 'q') {
                break;
            }
        }

        if (frameFlag) {
            frameFlag = 0;

            clearCanvas(canvas);
            updateClock(clock);
            updateSnake(canvas, snake1, snake2->entity);
            updateSnake(canvas, snake2, snake1->entity);
            drawBorder(canvas);
            printf("\033[H");
            printCanvas(canvas);
        }
    }

    setRawMode(0);

    freeCanvas(canvas);
    destroyClock(clock);
    destroySnake(snake1);
    destroySnake(snake2);

    return 0;
}

int main(void) {
  return run(60, 66, 90); 
}

