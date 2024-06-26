#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "NN.h"

Vector *vector_new(Matrix *Parent, size_t size, void *data) {
    Vector *vector = (Vector*)malloc(sizeof(Vector));
    vector->index = 0;
    vector->size = size;
    vector->parent = Parent;
    vector->data = (void**)malloc(size * sizeof(void *));
    
    for (size_t i = 0; i < size; i++) {
        vector->data[i] = data;
    }

    return vector;
}

Vector *vector_push(Vector *vector, void *data) {
    size_t new_size = vector->size + 1;
    vector->data = (void**)realloc(vector->data, new_size * sizeof(void*));
    vector->data[vector->size] = data;
    vector->size = new_size;

    return vector;
}

Vector *dynamic_vector_push(Vector *vector, unsigned int idx, void *data, float *argv) {
    if (idx >= vector->size) {
        size_t new_size = idx + 1;
        vector->data = (void**)realloc(vector->data, new_size * sizeof(void*));
        for (size_t i = vector->size; i < new_size; i++) {
            vector->data[i] = (argv && argv[i] != 0.0f) ? &argv[i] : NULL;
        }
        vector->size = new_size;
    }

    vector = vector_push(vector, data);
    return vector;
}

void *vector_pop(Vector *vector) {
    if (vector->size == 0) {
        return NULL;
    }
    vector->size--;
    void *data = vector->data[vector->size];
    vector->data[vector->size] = NULL;
    return data;
}

Vector *vector_free(Vector *vector) {
    free(vector->data);
    free(vector);
    return NULL;
}

Matrix *matrix_new(Vector **argv) {
    Matrix *matrix = (Matrix*)malloc(sizeof(Matrix));
    matrix->index = 0;
    matrix->size = 0;

    if (argv == NULL) {
        Vector *vector = vector_new(matrix, 0, NULL);
        vector->index = 0;
        vector->size = 0;
        vector->data = NULL;
        matrix->vectors = &vector;
        matrix->size = 1;
    } else {
        matrix->vectors = argv;
        while (argv[matrix->size] != NULL) {
            matrix->size++;
        }
    }

    return matrix;
}

Matrix *matrix_free(Matrix *matrix) {
    for (size_t i = 0; i < matrix->size; i++) {
        vector_free(matrix->vectors[i]);
    }
    free(matrix->vectors);
    free(matrix);
    return NULL;
}

Tensor *tensor_new(size_t size) {
    Tensor *tensor = (Tensor*)malloc(sizeof(Tensor));
    tensor->size = size;
    tensor->matrices = (Matrix**)malloc(size * sizeof(Matrix*));

    for (size_t i = 0; i < size; i++) {
        tensor->matrices[i] = matrix_new(NULL);
    }

    return tensor;
}

Tensor *tensor_free(Tensor *tensor) {
    for (size_t i = 0; i < tensor->size; i++) {
        matrix_free(tensor->matrices[i]);
    }
    free(tensor->matrices);
    free(tensor);
    return NULL;
}

void tensor_interact_with_environment(Tensor *tensor, void *environment) {
    Environment *env = (Environment*)environment;
    // Implement interaction logic
    // Propagate changes to matrices and vectors based on environment state
    for (size_t i = 0; i < tensor->size; i++) {
        for (size_t j = 0; j < tensor->matrices[i]->size; j++) {
            for (size_t k = 0; k < tensor->matrices[i]->vectors[j]->size; k++) {
                // Perform interaction logic here
            }
        }
    }
}

Tensor *tensor_procreate(Tensor *parent_a, Tensor *parent_b, Tensor *offspring) {
    offspring = tensor_new(parent_a->size);
    // Implement tensor procreation logic here
    for (size_t i = 0; i < parent_a->size; i++) {
        offspring->matrices[i] = matrix_new(NULL);
        for (size_t j = 0; j < parent_a->matrices[i]->size; j++) {
            Vector *vector_a = parent_a->matrices[i]->vectors[j];
            Vector *vector_b = parent_b->matrices[i]->vectors[j];
            Vector *vector_offspring = vector_new(offspring->matrices[i], vector_a->size, NULL);
            for (size_t k = 0; k < vector_a->size; k++) {
                // Combine data from parent_a and parent_b to form offspring
                vector_offspring->data[k] = vector_a->data[k];  
            }
            offspring->matrices[i]->vectors[j] = vector_offspring;
        }
    }
    return offspring;
}

float Force(Gravity *gravity, Element *element1, Element *element2) {
    float distance = sqrt(pow(element1->entity->cell.pos.x - element2->entity->cell.pos.x, 2) +
                          pow(element1->entity->cell.pos.y - element2->entity->cell.pos.y, 2));
    float force = (gravity->force * element1->mass * element2->mass) / pow(distance, 2);

    //element1->entity->cell.pos.x += force / element1->mass;
    //element2->entity->cell.pos.x -= force / element2->mass;
    //element1->entity->cell.pos.y += force / element1->mass;
    //element2->entity->cell.pos.y -= force / element2->mass;
    
  return force;
}

Entity *new_entity(Color color, float mass, float diameter, float weight) {
    Entity *entity = (Entity*)malloc(sizeof(Entity));
    entity->color = color;
    return entity;
}

Gravity *new_gravity(Entity *entity, float force) {
    Gravity *gravity = (Gravity*)malloc(sizeof(Gravity));
    gravity->affected = entity;
    gravity->force = force;
    return gravity;
}

Environment *new_environment(uint8_t width, uint8_t height) {
    Environment *env = (Environment*)malloc(sizeof(Environment));
    env->canvas = initCanvas(width, height, ' ');
    env->totalGravity = NULL;
    env->GravitionalCollapseFactor = 1.0;
    env->start = clock();
    env->end = clock();
    return env;
}

Element *interact_with_environment(Environment *env, Entity *entity) {
    Element *element = (Element*)malloc(sizeof(Element));
    element->index = 0;
    element->entity = entity;
    element->tensor = tensor_new(1);
    tensor_interact_with_environment(element->tensor, env);
    return element;
}

Element *create_canvas(Environment *env, Entity *entity) {
    Element *element = (Element*)malloc(sizeof(Element));
    element->index = 0;
    element->entity = entity;
    element->tensor = tensor_new(1);
    env->canvas->state.entities = (Entity**)realloc(env->canvas->state.entities, (env->canvas->state.entityCount + 1) * sizeof(Entity*));
    env->canvas->state.entities[env->canvas->state.entityCount] = entity;
    env->canvas->state.entityCount++;
    return element;
}

Element *create_entity(Environment *env, Entity *entity) {
    Element *element = (Element*)malloc(sizeof(Element));
    element->index = 0;
    element->entity = entity;
    element->tensor = tensor_new(1);
    return element;
}

Element *procreate(Element *a, Element *b) {
    Element *offspring = (Element*)malloc(sizeof(Element));
    offspring->index = 0;
    offspring->entity = new_entity(a->entity->color, a->mass, a->diameter, a->weight);
    offspring->tensor = tensor_procreate(a->tensor, b->tensor, offspring->tensor);
    return offspring;
}

Element *new_element(Canvas *canvas, Entity *entity) {
    Element *element = (Element*)malloc(sizeof(Element));
    element->index = 0;
    element->entity = entity;
    element->tensor = tensor_new(1);
    return element;
}

Element *new_attribute(Element *element, char *key, void *value) {
    // Add new attribute to element's tensor or other properties
    // Implementation is domain-specific, assuming key-value store in tensor for simplicity
    if (strcmp(key, "health") == 0) {
        element->entity->health = *((int*)value);
    }
    return element;
}

Element *new_property(Element *element, char *key, void *value) {
    // Add new property to element's tensor or other properties
    // Implementation is domain-specific, assuming key-value store in tensor for simplicity
    if (strcmp(key, "isAlive") == 0) {
        element->entity->isAlive = *((int*)value);
    }
    return element;
}


