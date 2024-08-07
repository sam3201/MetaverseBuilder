#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include "../utils/environment.h"
#include "../utils/NNs/NN.h"

#define FPS 120 
#define MAX_AGENTS 10
#define MAX_PREDATORS 5
#define MAX_PREY 10 
#define MAX_FOOD 15
#define INITIAL_HEALTH 1000
#define HEALTH_DECAY_RATE 0 
#define CATCH_DISTANCE 1 
#define PREDATOR_GAIN 100 
#define PREY_GAIN 100 
#define FOOD_RESPAWN_RATE 0.02
#define MUTATION_RATE 1 
#define CROSSOVER_RATE 0.1 

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NUM_DIRECTIONS
} Direction;

Direction Directions[NUM_DIRECTIONS] = {UP, DOWN, LEFT, RIGHT};

typedef struct {
    NN_t *nn;
    Entity *entity;
    double fitness;
    size_t is_predator;
    size_t time_alive;
    Direction dir;
} Agent;

typedef struct {
    Entity *entity;
} Food;

typedef struct {
    Agent *predators[MAX_PREDATORS];
    Agent *preys[MAX_PREY];
    Food *foods[MAX_FOOD];
    size_t numPredators;
    size_t numPreys;
    size_t numFoods;
} Simulation;

double distance(Pos A, Pos B) {
    return sqrt(pow(A.x - B.x, 2) + pow(A.y - B.y, 2));
}
double calculatePredatorFitness(Agent *predator) {
    return 1 / 1 + predator->entity->health + predator->time_alive;
}

double calculatePreyFitness(Agent *prey) {
    return 1 / 1 + prey->entity->health + prey->time_alive; 
}

double *one_hot_encode(Simulation *simulation, Canvas *canvas) {
    size_t canvas_size = canvas->numRows * canvas->numCols;

    double *one_hot_encoded = malloc(canvas_size * sizeof(double)); 

    memset(one_hot_encoded, 0, canvas_size * sizeof(double)); 
  
    for (size_t i = 0; i < simulation->numPredators; i++) {
        one_hot_encoded[canvas_size - simulation->predators[i]->entity->cell.pos.y * canvas->numCols + simulation->predators[i]->entity->cell.pos.x] = 1;
    }
  
    for (size_t i = 0; i < simulation->numPreys; i++) {
        one_hot_encoded[canvas_size - simulation->preys[i]->entity->cell.pos.y * canvas->numCols + simulation->preys[i]->entity->cell.pos.x] = 3;
    }
  
    for (size_t i = 0; i < simulation->numFoods; i++) {
        one_hot_encoded[canvas_size - simulation->foods[i]->entity->cell.pos.y * canvas->numCols + simulation->foods[i]->entity->cell.pos.x] = 2;
    }
    
    return one_hot_encoded;
}

void destroyAgent(Agent *agent) {
    NN_destroy(agent->nn);
    deleteEntity(agent->entity);
    free(agent);
}

void destroyFood(Food *food) {
    deleteEntity(food->entity);
    free(food);
}

void updateAgent(Agent *agent, Canvas *canvas, Simulation *simulation) {
    size_t inputSize = canvas->numRows * canvas->numCols;
    double *inputs = malloc(sizeof(double) * inputSize);
    inputs = one_hot_encode(simulation, canvas);

    forward(agent->nn, inputs);
    if (agent->nn->output[0] != 0.25 && agent->nn->output[0] != 0.75 && agent->nn->output[0] != 1.0) {
        agent->dir = Directions[rand() % NUM_DIRECTIONS];
    }

    if (agent->nn->output[0] == 0.25) {
        agent->dir = UP;
    } else if (agent->nn->output[0] == 0.25) {
        agent->dir = DOWN;
    } else if (agent->nn->output[0] == 0.75) {
        agent->dir = LEFT;  
    } else if (agent->nn->output[0] == 1.0) {
        agent->dir = RIGHT;
    }

    if (agent->dir == UP) {
        agent->entity->cell.pos.y--;
    } else if (agent->dir == DOWN) {
        agent->entity->cell.pos.y++;
    } else if (agent->dir == LEFT) {
        agent->entity->cell.pos.x--;
    } else if (agent->dir == RIGHT) {
        agent->entity->cell.pos.x++;
    }
     
    if (agent->entity->cell.pos.x < 1) {
        agent->entity->cell.pos.x = canvas->numCols - 2;
    } else if (agent->entity->cell.pos.x >= canvas->numCols) {
        agent->entity->cell.pos.x = 1;
    }
  
    if (agent->entity->cell.pos.y < 1) {
        agent->entity->cell.pos.y = canvas->numRows - 2;
    } else if (agent->entity->cell.pos.y >= canvas->numRows) {
        agent->entity->cell.pos.y = 1;
    }

    agent->entity->health -= HEALTH_DECAY_RATE;
    agent->time_alive++;

    if (agent->is_predator) {
        for (size_t i = 0; i < simulation->numPreys; i++) {
            double dist = distance(agent->entity->cell.pos, simulation->preys[i]->entity->cell.pos);
            if (dist < CATCH_DISTANCE) {
                agent->entity->health += PREDATOR_GAIN;
                simulation->preys[i]->entity->health -= PREDATOR_GAIN;
            }
        }
    } else {
        for (size_t i = 0; i < simulation->numFoods; i++) {
            double dist = distance(agent->entity->cell.pos, simulation->foods[i]->entity->cell.pos);
            if (dist < CATCH_DISTANCE) {
                agent->entity->health += PREY_GAIN;
                destroyFood(simulation->foods[i]);
                simulation->foods[i] = simulation->foods[--simulation->numFoods];
                break;
            }
        }
    }

    if (agent->entity->cell.c == 'X') {
        agent->fitness = calculatePredatorFitness(agent);
        backprop(agent->nn, &agent->fitness);
    } else if (agent->entity->cell.c == 'O') {
        agent->fitness = calculatePreyFitness(agent);
        backprop(agent->nn, &agent->fitness);
    }

    free(inputs);
}

Agent *createAgent(Canvas *canvas, const char *type, char symbol, size_t is_predator) {
    Agent *agent = malloc(sizeof(Agent)); 
    if (!agent) {
        fprintf(stderr, "Failed to allocate memory for agent\n");
        return NULL;
    }

    Color color = is_predator ? (Color){255,0,0} : (Color){0,255,0};
    Entity *entity = createEntity((TYPE){type}, symbol, rand() % canvas->numCols, rand() % canvas->numRows, 1, color, NULL);
    if (!entity) {
        fprintf(stderr, "Failed to create entity for agent\n");
        destroyAgent(agent); 
        return NULL;
    }
    addEntity(canvas, entity);

    agent->entity = entity;
    agent->dir = Directions[rand() % NUM_DIRECTIONS];
    agent->fitness = 0;
    agent->is_predator = is_predator;
    agent->entity->health = INITIAL_HEALTH;
    agent->time_alive = 0;
    agent->nn = NULL;
    ActivationFunction hidden_activations[20];
    ActivationFunction hidden_derivatives[20];
    for (size_t i = 0; i < 20; i++) {
        hidden_activations[i] = sigmoid;
        hidden_derivatives[i] = sigmoid_derivative;
    }
    ActivationFunction output_activations = tanh;
    ActivationFunction output_derivatives = tanh_derivative;

    agent->nn = NN_create(10, 20, 1, hidden_activations, hidden_derivatives, &output_activations, &output_derivatives, 1, 1);
    if (!agent->nn) {
        fprintf(stderr, "Failed to create neural network for agent\n");
        destroyAgent(agent);
        return NULL;
    }

    return agent;
}

Food *createFood(Canvas *canvas) {
    Food *food = malloc(sizeof(Food));
    if (!food) {
        fprintf(stderr, "Failed to allocate memory for food\n");
        return NULL;
    }

    Entity *entity = createEntity((TYPE){"FOOD"}, '*', rand() % canvas->numCols, rand() % canvas->numRows, 1, (Color){0,255,255}, NULL);
    if (!entity) {
        fprintf(stderr, "Failed to create entity for food\n");
        free(food);
        return NULL;
    }
    addEntity(canvas, entity);

    food->entity = entity;
    return food;
}

size_t checkAliveEntities(Simulation *simulation) {
    for (size_t i = 0; i < simulation->numPredators; i++) {
        if (simulation->predators[i]->entity->health > 0) return 1;
    }
    for (size_t i = 0; i < simulation->numPreys; i++) {
        if (simulation->preys[i]->entity->health > 0) return 1;
    }
    return 0;
}

void mutate(NN_t *nn) {
    for (size_t i = 0; i < nn->numWeights; i++) {
        if ((double)rand() / RAND_MAX < MUTATION_RATE) {
            nn->weights[i] += ((double)rand() / RAND_MAX - 0.5) * 0.1;
        }
    }
}


void crossover(NN_t *parent1, NN_t *parent2, NN_t *child) {
    for (size_t i = 0; i < parent1->numWeights; i++) {
        if ((double)rand() / RAND_MAX < CROSSOVER_RATE) {
            child->weights[i] = parent1->weights[i];
        } else {
            child->weights[i] = parent2->weights[i];
        }
    }
}

int compareFitness(const void *a, const void *b) {
    Agent *agentA = *(Agent**)a;
    Agent *agentB = *(Agent**)b;
    return (agentB->fitness > agentA->fitness) - (agentB->fitness < agentA->fitness);
}
void evolvePopulation(Simulation *simulation) {
    qsort(simulation->predators, simulation->numPredators, sizeof(Agent*), compareFitness);
    qsort(simulation->preys, simulation->numPreys, sizeof(Agent*), compareFitness);

    for (size_t i = MAX_PREDATORS / 2; i < MAX_PREDATORS; i++) {
        size_t parent1 = rand() % (MAX_PREDATORS / 2);
        size_t parent2 = rand() % (MAX_PREDATORS / 2);
        crossover(simulation->predators[parent1]->nn, simulation->predators[parent2]->nn, simulation->predators[i]->nn);
        mutate(simulation->predators[i]->nn);
    }

    for (size_t i = MAX_PREY / 2; i < MAX_PREY; i++) {
        size_t parent1 = rand() % (MAX_PREY / 2);
        size_t parent2 = rand() % (MAX_PREY / 2);
        crossover(simulation->preys[parent1]->nn, simulation->preys[parent2]->nn, simulation->preys[i]->nn);
        mutate(simulation->preys[i]->nn);
    }
}

void updateSimulation(Simulation *simulation, Canvas *canvas) {
    for (size_t i = 0; i < simulation->numPredators; i++) {
        updateAgent(simulation->predators[i], canvas, simulation);
    }
    for (size_t i = 0; i < simulation->numPreys; i++) {
        updateAgent(simulation->preys[i], canvas, simulation);
    }

    if ((double)rand() / RAND_MAX < FOOD_RESPAWN_RATE && simulation->numFoods < MAX_FOOD) {
        Food *newFood = createFood(canvas);
        if (newFood) {
            simulation->foods[simulation->numFoods++] = newFood;
        }
    }

    //evolvePopulation(simulation);
}

void destroySimulation(Simulation *simulation) {
    for (size_t i = 0; i < simulation->numPredators; i++) {
        destroyAgent(simulation->predators[i]);
    }
    for (size_t i = 0; i < simulation->numPreys; i++) {
        destroyAgent(simulation->preys[i]);
    }
    for (size_t i = 0; i < simulation->numFoods; i++) {
        destroyFood(simulation->foods[i]);
    }
    free(simulation);
}

Simulation* createSimulation(Canvas *canvas) {
    Simulation *simulation = malloc(sizeof(Simulation));
    if (!simulation) {
        fprintf(stderr, "Failed to allocate memory for simulation\n");
        return NULL;
    }

    simulation->numPredators = MAX_PREDATORS;
    simulation->numPreys = MAX_PREY;
    simulation->numFoods = MAX_FOOD / 2;  

    for (size_t i = 0; i < MAX_PREDATORS; i++) {
        simulation->predators[i] = createAgent(canvas, "PREDATOR", 'X', 1);
        if (!simulation->predators[i]) {
            fprintf(stderr, "Failed to create predator\n");
            destroySimulation(simulation);
            return NULL;
        }
    }
    for (size_t i = 0; i < MAX_PREY; i++) {
        simulation->preys[i] = createAgent(canvas, "PREY", 'O', 0);
        if (!simulation->preys[i]) {
            fprintf(stderr, "Failed to create prey\n");
            destroySimulation(simulation);
            return NULL;
        }
    }
    for (size_t i = 0; i < simulation->numFoods; i++) {
        simulation->foods[i] = createFood(canvas);
        if (!simulation->foods[i]) {
            fprintf(stderr, "Failed to create food\n");
            destroySimulation(simulation);
            return NULL;
        }
    }

    return simulation;
}

void restartSimulation(Simulation *simulation, Canvas *canvas) {
    for (size_t i = 0; i < simulation->numPredators; i++) {
        destroyAgent(simulation->predators[i]);
        simulation->predators[i] = createAgent(canvas, "PREDATOR", 'X', 1);
    }
    
    for (size_t i = 0; i < simulation->numPreys; i++) {
        destroyAgent(simulation->preys[i]);
        simulation->preys[i] = createAgent(canvas, "PREY", 'O', 0);
    }
    
    for (size_t i = 0; i < simulation->numFoods; i++) {
        destroyFood(simulation->foods[i]);
    }
    simulation->numFoods = MAX_FOOD / 2;
    for (size_t i = 0; i < simulation->numFoods; i++) {
        simulation->foods[i] = createFood(canvas);
    }
}

void drawSimulation(Canvas *canvas, Simulation *simulation) {
    for (size_t i = 0; i < simulation->numPredators; i++) {
        Entity *entity = simulation->predators[i]->entity;
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
    for (size_t i = 0; i < simulation->numPreys; i++) {
        Entity *entity = simulation->preys[i]->entity;
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
    for (size_t i = 0; i < simulation->numFoods; i++) {
        Entity *entity = simulation->foods[i]->entity;
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
}

int run(uint8_t frameRate, uint8_t rows, uint8_t cols) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    Canvas *canvas = initCanvas(rows, cols, ' ');
    if (!canvas) {
        fprintf(stderr, "Failed to initialize canvas\n");
        return 1;
    }

    Simulation *simulation = createSimulation(canvas);
    if (!simulation) {
        fprintf(stderr, "Failed to create simulation\n");
        freeCanvas(canvas);
        return 1;
    }

    Clock *clock = createClock();
    initClock(clock, frameRate, frameRate);

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
            updateClock(clock);
            
            while (fixedUpdateReady(clock)) {

            clearCanvas(canvas);

            updateSimulation(simulation, canvas);
            drawSimulation(canvas, simulation);
            drawBorder(canvas);
            printf("\033[H");
            printCanvas(canvas);

            if (!checkAliveEntities(simulation)) {
                printf("All entities have died. Restarting simulation...\n");
                sleep(10); 
                restartSimulation(simulation, canvas);
            }
        }
    }
  }
    setRawMode(0);

    destroySimulation(simulation);
    destroyClock(clock);
    freeCanvas(canvas);

    return 0;
}

int main(void) {
    return run(FPS, 45, 155);
}

