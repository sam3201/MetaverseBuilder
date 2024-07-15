#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../utils/environment.h"

#define MAX_PARTICLE 300

typedef struct Particle Particle;

typedef struct Particle {
    Entity *entity;
    void (*destroy)(Particle *);
    Pos velocity;
} Particle;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    Particle *particles;
    size_t maxParticles;
    size_t currentParticle;
} System;

#define GRAVITY_STRENGTH 1.0 
#define GRAVITY_MIN_DISTANCE 1.0
#define DOWNWARD_VELOCITY 1.0 
#define MOUSE_REPULSION_STRENGTH 10.0 
#define MOUSE_MIN_DISTANCE 2.0 

double *distance(Particle *A, Particle *B) {
    double dx = A->entity->cell.pos.x - B->entity->cell.pos.x;
    double dy = A->entity->cell.pos.y - B->entity->cell.pos.y;
    double *dist = malloc(sizeof(double) * 2);
    dist[0] = dx;
    dist[1] = dy;
    return dist;
}

void destroyParticle(Particle *particle) {
    deleteEntity(particle->entity);
    free(particle);
}

Particle *createParticle(Canvas *canvas, char particleChar, Color color) {
    Particle *particle = malloc(sizeof(Particle));
    if (!particle) {
        fprintf(stderr, "Failed to allocate memory for particle\n");
        return NULL;
    }

    Entity *entity = createEntity((TYPE){"PARTICLE"}, particleChar, rand() % canvas->numCols, rand() % canvas->numRows, 1, color, NULL);
    if (!entity) {
        fprintf(stderr, "Failed to create entity for particle\n");
        free(particle);
        return NULL;
    }
    addEntity(canvas, entity);

    particle->entity = entity;
    particle->destroy = destroyParticle;
    particle->velocity.x = 0.0;
    particle->velocity.y = 0.0;

    return particle;
}

System *createSystem(unsigned int numParticles, Canvas *canvas) {
    System *system = malloc(sizeof(System));
    if (!system) {
        fprintf(stderr, "Failed to allocate memory for particle system\n");
        return NULL;
    }

    system->maxParticles = numParticles > MAX_PARTICLE ? MAX_PARTICLE : numParticles;
    system->currentParticle = 0;
    system->particles = malloc(sizeof(Particle) * system->maxParticles);
    if (!system->particles) {
        fprintf(stderr, "Failed to allocate memory for particles in system\n");
        free(system);
        return NULL;
    }

    for (size_t i = 0; i < system->maxParticles; i++) {
        Color color = {
            .r = rand() % 256,
            .g = rand() % 256,
            .b = rand() % 256};
        
        Particle *particle = createParticle(canvas, '*', color);
        if (particle) {
            system->particles[i] = *particle;
            system->currentParticle++;
        } else {
            fprintf(stderr, "Failed to create particle %zu\n", i + 1);
        }
    }

    pthread_mutex_init(&system->mutex, NULL);
    pthread_cond_init(&system->cond, NULL);

    return system;
}

void destroySystem(System *system) {
    for (size_t i = 0; i < system->currentParticle; i++) {
        destroyParticle(&system->particles[i]);
    }
    free(system->particles);
    free(system);
}

void applyGravity(Canvas *canvas, Particle *particle, Particle *otherParticle) {
    double *dist = distance(particle, otherParticle);
    double distanceSquared = dist[0] * dist[0] + dist[1] * dist[1];
    if (distanceSquared > GRAVITY_MIN_DISTANCE) {
        double gravityStrength = GRAVITY_STRENGTH / distanceSquared;
        double gravity_dx = dist[0] * gravityStrength;
        double gravity_dy = dist[1] * gravityStrength;
        particle->velocity.x += gravity_dx;
        particle->velocity.y += gravity_dy;
    }
    free(dist);
}

void applyMouseRepulsion(Canvas *canvas, Particle *particle, Pos mousePos) {
    double dx = particle->entity->cell.pos.x - mousePos.x;
    double dy = particle->entity->cell.pos.y - mousePos.y;
    double distanceSquared = dx * dx + dy * dy;
    if (distanceSquared > MOUSE_MIN_DISTANCE) {
        double repulsionStrength = MOUSE_REPULSION_STRENGTH / distanceSquared;
        double repulsion_dx = dx * repulsionStrength;
        double repulsion_dy = dy * repulsionStrength;
        particle->velocity.x += repulsion_dx;
        particle->velocity.y += repulsion_dy;
    }
}

void moveParticles(Canvas *canvas, System *system, Clock *clock) {
    Pos mousePos = getMousePos();
    printf("mouse pos: %d, %d\n", mousePos.x, mousePos.y);

    for (size_t i = 0; i < system->maxParticles; i++) {
        Particle *particle = &system->particles[i];

        for (size_t j = 0; j < system->maxParticles; j++) {
            if (i != j) {
                applyGravity(canvas, particle, &system->particles[j]);
            }
        }

        applyMouseRepulsion(canvas, particle, mousePos);

        if (particle->entity->cell.pos.y >= canvas->numRows - 2) {
            particle->entity->cell.pos.y = canvas->numRows - 2;  
        } else {
            particle->velocity.y += DOWNWARD_VELOCITY;

            int dx = (int) particle->velocity.x;
            int dy = (int) particle->velocity.y;

            moveEntity(canvas, particle->entity, (Pos) {dx, dy});
        }
    }
}

int run(uint8_t frameRate, uint8_t rows, uint8_t cols, unsigned int numParticles, Color particleColor) {
    signal(SIGINT, handleSignal);
    srand((unsigned)time(NULL));

    Canvas *canvas = initCanvas(rows, cols, ' ');
    if (!canvas) {
        fprintf(stderr, "Failed to initialize canvas\n");
        return 1;
    }

    System *particleSystem = createSystem(numParticles, canvas);
    if (!particleSystem) {
        fprintf(stderr, "Failed to create particle system\n");
        freeCanvas(canvas);
        return 1;
    }

    Clock *clock = createClock();
    if (!clock) {
        fprintf(stderr, "Failed to create clock\n");
        destroySystem(particleSystem);
        freeCanvas(canvas);
        return 1;
    }

    initClock(clock, 60, 60); 

    char *title = "Particle System";
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

    while (1) {
        if (frameFlag) {
            frameFlag = 0;
            if (kbhit()) {
                char c = getchar();
                if (c == 'q') {
                    break;
                }
            }

            updateClock(clock);  

            moveParticles(canvas, particleSystem, clock);

            drawEntities(canvas);
            drawBorder(canvas);
            printf("\033[H");
            printCanvas(canvas);
        }
    }

    setRawMode(0);

    destroyClock(clock);
    destroySystem(particleSystem);
    freeCanvas(canvas);

    return 0;
}

int main(void) {
    return run(60, 40, 50, 300, (Color){95, 144, 255});
}

