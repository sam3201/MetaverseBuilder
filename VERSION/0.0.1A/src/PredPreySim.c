#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include "../utils/environment.h"
#include "../utils/NNs/NN.h"

#define MAX_AGENTS 10
#define MAX_PREDATORS 5
#define MAX_PREY 5
#define MAX_FOOD 15
#define FPS 60
#define INITIAL_HEALTH 100
#define HEALTH_DECAY_RATE 0.1
#define CATCH_DISTANCE 1 
#define PREDATOR_GAIN 100 
#define PREY_GAIN 50
#define FOOD_RESPAWN_RATE 0.02

typedef struct {
    NN_t *nn;
    Entity *entity;
    double fitness;
    int is_predator;
    double health;
    int time_alive;
} Agent;

typedef struct {
    Entity *entity;
} Food;

typedef struct {
    Agent *predators[MAX_PREDATORS];
    Agent *preys[MAX_PREY];
    Food *foods[MAX_FOOD];
    int numPredators;
    int numPreys;
    int numFoods;
} Simulation;

double distance(Pos A, Pos B) {
    return sqrt(pow(A.x - B.x, 2) + pow(A.y - B.y, 2));
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

Agent *createAgent(Canvas *canvas, const char *type, char symbol, int is_predator) {
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
    agent->fitness = 0;
    agent->is_predator = is_predator;
    agent->health = INITIAL_HEALTH;
    agent->time_alive = 0;

    // Initialize neural network
    agent->nn = NN_create(5, 6, 2, NULL, NULL, NULL, NULL, 0.01, 0.9);
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

void updateAgent(Agent *agent, Canvas *canvas, Simulation *simulation) {
    // Get inputs for neural network
    double inputs[5] = {0};
    Agent *closest_agent = NULL;
    Food *closest_food = NULL;
    double min_agent_distance = INFINITY;
    double min_food_distance = INFINITY;

    // Find the closest agent of the opposite type and closest food
    for (int i = 0; i < (agent->is_predator ? simulation->numPreys : simulation->numPredators); i++) {
        Agent *other = agent->is_predator ? simulation->preys[i] : simulation->predators[i];
        double dist = distance(agent->entity->cell.pos, other->entity->cell.pos);
        if (dist < min_agent_distance) {
            min_agent_distance = dist;
            closest_agent = other;
        }
    }

    for (int i = 0; i < simulation->numFoods; i++) {
        Food *food = simulation->foods[i];
        double dist = distance(agent->entity->cell.pos, food->entity->cell.pos);
        if (dist < min_food_distance) {
            min_food_distance = dist;
            closest_food = food;
        }
    }

    if (closest_agent) {
        inputs[0] = (closest_agent->entity->cell.pos.x - agent->entity->cell.pos.x) / canvas->numCols;
        inputs[1] = (closest_agent->entity->cell.pos.y - agent->entity->cell.pos.y) / canvas->numRows;
        inputs[2] = min_agent_distance / sqrt(canvas->numCols * canvas->numCols + canvas->numRows * canvas->numRows);
    }
    if (closest_food) {
        inputs[3] = (closest_food->entity->cell.pos.x - agent->entity->cell.pos.x) / canvas->numCols;
        inputs[4] = (closest_food->entity->cell.pos.y - agent->entity->cell.pos.y) / canvas->numRows;
    }

    // Forward pass through neural network
    forward(agent->nn, inputs);

    // Use output to determine movement
    int dx = round(agent->nn->output[0] * 2 - 1);
    int dy = round(agent->nn->output[1] * 2 - 1);

    agent->entity->cell.pos.x += dx;
    agent->entity->cell.pos.y += dy;

    // Ensure agent stays within canvas bounds
    if (agent->entity->cell.pos.x < 0) agent->entity->cell.pos.x = 0;
    if (agent->entity->cell.pos.x >= canvas->numCols) agent->entity->cell.pos.x = canvas->numCols - 1;
    if (agent->entity->cell.pos.y < 0) agent->entity->cell.pos.y = 0;
    if (agent->entity->cell.pos.y >= canvas->numRows) agent->entity->cell.pos.y = canvas->numRows - 1;

    // Update health and time alive
    agent->health -= HEALTH_DECAY_RATE;
    agent->time_alive++;

    // Check for predator-prey interaction
    if (agent->is_predator && closest_agent && min_agent_distance < CATCH_DISTANCE) {
        agent->health += PREDATOR_GAIN;
        closest_agent->health -= PREDATOR_GAIN;
    }

    // Check for prey-food interaction
    if (!agent->is_predator && closest_food && min_food_distance < CATCH_DISTANCE) {
        agent->health += PREY_GAIN;
        // Remove eaten food
        for (int i = 0; i < simulation->numFoods; i++) {
            if (simulation->foods[i] == closest_food) {
                destroyFood(closest_food);
                simulation->foods[i] = simulation->foods[--simulation->numFoods];
                break;
            }
        }
    }

    // Update fitness (multi-objective)
    agent->fitness = agent->health + agent->time_alive;
}

void updateSimulation(Simulation *simulation, Canvas *canvas) {
    for (int i = 0; i < simulation->numPredators; i++) {
        updateAgent(simulation->predators[i], canvas, simulation);
    }
    for (int i = 0; i < simulation->numPreys; i++) {
        updateAgent(simulation->preys[i], canvas, simulation);
    }

    // Respawn food
    if ((double)rand() / RAND_MAX < FOOD_RESPAWN_RATE && simulation->numFoods < MAX_FOOD) {
        Food *newFood = createFood(canvas);
        if (newFood) {
            simulation->foods[simulation->numFoods++] = newFood;
        }
    }
}

Simulation* createSimulation(Canvas *canvas) {
    Simulation *simulation = malloc(sizeof(Simulation));
    if (!simulation) {
        fprintf(stderr, "Failed to allocate memory for simulation\n");
        return NULL;
    }

    simulation->numPredators = MAX_PREDATORS;
    simulation->numPreys = MAX_PREY;
    simulation->numFoods = MAX_FOOD / 2;  // Start with half the maximum food

    for (int i = 0; i < MAX_PREDATORS; i++) {
        simulation->predators[i] = createAgent(canvas, "PREDATOR", 'X', 1);
    }
    for (int i = 0; i < MAX_PREY; i++) {
        simulation->preys[i] = createAgent(canvas, "PREY", 'O', 0);
    }
    for (int i = 0; i < simulation->numFoods; i++) {
        simulation->foods[i] = createFood(canvas);
    }

    return simulation;
}

void destroySimulation(Simulation *simulation) {
    for (int i = 0; i < simulation->numPredators; i++) {
        destroyAgent(simulation->predators[i]);
    }
    for (int i = 0; i < simulation->numPreys; i++) {
        destroyAgent(simulation->preys[i]);
    }
    for (int i = 0; i < simulation->numFoods; i++) {
        destroyFood(simulation->foods[i]);
    }
    free(simulation);
}

void drawSimulation(Canvas *canvas, Simulation *simulation) {
    for (int i = 0; i < simulation->numPredators; i++) {
        Entity *entity = simulation->predators[i]->entity;
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
    for (int i = 0; i < simulation->numPreys; i++) {
        Entity *entity = simulation->preys[i]->entity;
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
    for (int i = 0; i < simulation->numFoods; i++) {
        Entity *entity = simulation->foods[i]->entity;
        canvas->state.cells[entity->cell.pos.y][entity->cell.pos.x] = entity->cell.c;
        canvas->state.colors[entity->cell.pos.y][entity->cell.pos.x] = entity->color;
    }
}

int checkAliveEntities(Simulation *simulation) {
    for (int i = 0; i < simulation->numPredators; i++) {
        if (simulation->predators[i]->entity->health > 0) return 1;
    }
    for (int i = 0; i < simulation->numPreys; i++) {
        if (simulation->preys[i]->entity->health > 0) return 1;
    }
    return 0;
}

void restartSimulation(Simulation *simulation, Canvas *canvas) {
    // Reset predators
    for (int i = 0; i < simulation->numPredators; i++) {
        destroyAgent(simulation->predators[i]);
        simulation->predators[i] = createAgent(canvas, "PREDATOR", 'X', 1);
    }
    
    // Reset preys
    for (int i = 0; i < simulation->numPreys; i++) {
        destroyAgent(simulation->preys[i]);
        simulation->preys[i] = createAgent(canvas, "PREY", 'O', 0);
    }
    
    // Reset foods
    for (int i = 0; i < simulation->numFoods; i++) {
        destroyFood(simulation->foods[i]);
    }
    simulation->numFoods = MAX_FOOD / 2;
    for (int i = 0; i < simulation->numFoods; i++) {
        simulation->foods[i] = createFood(canvas);
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

            updateSimulation(simulation, canvas);
            drawSimulation(canvas, simulation);
            drawBorder(canvas);
            printf("\033[H");
            printCanvas(canvas);

            if (!checkAliveEntities(simulation)) {
                printf("All entities have died. Restarting simulation...\n");
                sleep(5); 
                restartSimulation(simulation, canvas);
            }
        }
    }

    setRawMode(0);

    destroySimulation(simulation);
    freeCanvas(canvas);

    return 0;
}

int main(void) {
    return run(FPS, 40, 50);
}
