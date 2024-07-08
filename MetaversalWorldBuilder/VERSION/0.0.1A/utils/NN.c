#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <math.h>
#include "NN.h"

Vector *vector_new(Matrix *Parent, size_t size, double *data) { 
  Vector *vector = (Vector*)malloc(sizeof(Vector));
  vector->index = 0;
  vector->size = size;
  vector->parent = Parent;
  vector->data = (double*)malloc(sizeof(double) * size);

  if (data == NULL) {
    double default_data[1] = {0.0};
    data = default_data;
    size_t default_size = 1;
    size = default_size;
  }

  for (size_t i = 0; i < size; i++) {
    vector->data[i] = data[i];
  }

  return vector;
}

static size_t next_power_of_two(size_t x) {
  if (x == 0) return 1;
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;  
  return x+1;
}

void vector_push(Vector *vector, double data) {
  size_t new_size = vector->size + 1;
  vector->data = (double *)realloc(vector->data, new_size * sizeof(double));
  if (vector->data == NULL) {
    fprintf(stderr, "Error: Memory allocation failed in vector_push\n");
    return;
  }
  vector->data[vector->size] = data;
  vector->size = new_size;
}

void dynamic_vector_push(Vector *vector, unsigned int idx, double *data) {
  size_t absolute_diff = (idx > vector->size) ? idx - vector->size : vector->size - idx;

  size_t new_size = idx + 1 + absolute_diff;

  if (new_size > vector->size) {
    size_t full_size = next_power_of_two(new_size); 
    vector->data = (double*)realloc(vector->data, full_size * sizeof(double));
    vector->size = full_size;
  }

  if (idx > vector->size) {
    memset(&vector->data[vector->size], 0, (idx - vector->size) * sizeof(double));
  }

    memcpy(&vector->data[idx], data, sizeof(double));
}

void vector_pop(Vector *vector) {
  if (vector->size == 0) {
    return;
  }
  vector->size--;
  double data = vector->data[vector->size];
  free(vector->data);
  vector->data = NULL;
}

void vector_free(Vector *vector) {
  free(vector->data);
  free(vector);
}

Matrix *matrix_new(unsigned int numVectors,...) {  
  Matrix *matrix = (Matrix*)malloc(sizeof(Matrix));  
  matrix->index = 0;  
  matrix->size = numVectors;  

  va_list args;  
  va_start(args, numVectors);  

  for (unsigned int i = 0; i < numVectors; i++) {  
    Vector *vector;  
    if (va_arg(args, Vector*)!= NULL) {  
      vector = va_arg(args, Vector*);  
    } else {  
      double data[1] = {0.0};  
      vector = vector_new(matrix, 1, data);  
    }  
    matrix->vectors[i] = vector;  
  }  

  va_end(args);  
  return matrix;  
}  

void matrix_free(Matrix *matrix) {
  for (size_t i = 0; i < matrix->size; i++) {
    vector_free(matrix->vectors[i]);
  }
  free(matrix->vectors);
  free(matrix);
}

Tensor *tensor_new(unsigned int numMatrices, ...) {
  Tensor *tensor = (Tensor*)malloc(sizeof(Tensor));  
  tensor->index = 0;
  tensor->size = numMatrices;
  tensor->matrices = (Matrix**)malloc(sizeof(Matrix*) * numMatrices); 

  va_list args;
  va_start(args, numMatrices);

  for (size_t i = 0; i < numMatrices; i++) {
    Matrix *matrix;
    if (va_arg(args, Matrix*)!= NULL) {
      matrix = va_arg(args, Matrix*);
    } else {
      double data[1] = {0.0};
      matrix = matrix_new(1, data); 
    }
    tensor->matrices[i] = matrix;
  }

  va_end(args);

  tensor->operands = (MetaOperands*)malloc(sizeof(MetaOperands)); 
  tensor->operands->rule = (RULES*)malloc(sizeof(RULES));
  tensor->operands->buffer = NULL; 
  tensor->operands->size = 1;  
  tensor->operands->index = 0;
  tensor->operands->references = NULL;
  tensor->operands->rule = (RULES *)RULE_NONE; 

  return tensor;
}

void tensor_free(Tensor *tensor) {
  for (size_t i = 0; i < tensor->size; i++) {
    matrix_free(tensor->matrices[i]);
  }
  free(tensor->matrices);
  free(tensor);
}

double *calc_force(Element *element1, Element *element2) {
    double *force_vector = (double*)malloc(sizeof(double) * 2);
    force_vector[0] = 0;
    force_vector[1] = 0;

    if (element1 == NULL) {
      printf("Error: Element1 has not been freed\n");
      element_free(element1);
      return force_vector;
    } 
    if (element2 == NULL) {
      printf("Error: Element2 has not been freed\n");
      element_free(element2);
      return force_vector;
    }
    
    double dx = element1->bounding->cell.pos.x - element2->bounding->cell.pos.x;
    double dy = element1->bounding->cell.pos.y - element2->bounding->cell.pos.y;

    double dist = sqrtl((dx * dx) + (dy * dy));

    if (dist == 0) return force_vector;    

    double force = (element1->mass * element2->mass) / (dist * dist);
    double force_x = force * dx / dist;
    double force_y = force * dy / dist;

    force_vector[0] = force_x;
    force_vector[1] = force_y;

    return force_vector; 
}

void apply_force(Element *element, double *force_vector, double dt) {
  double accel_x = force_vector[0] / element->mass;
  double accel_y = force_vector[1] / element->mass;

  double dx = accel_x * dt;
  double dy = accel_y * dt;

  element->bounding->cell.pos.x += dx; 
  element->bounding->cell.pos.y += dy; 

  free(force_vector);
}

Gravity *gravity_new(Entity **entities, size_t entityCount, double initial_force) {
    Gravity *gravity = (Gravity*)malloc(sizeof(Gravity));
    if (!gravity) {
        fprintf(stderr, "Error: Failed to allocate memory for Gravity\n");
        return NULL;
    }

    gravity->affected = (Entity**)malloc(entityCount * sizeof(Entity*));
    if (!gravity->affected) {
        fprintf(stderr, "Error: Failed to allocate memory for affected entities\n");
        free(gravity);
        return NULL;
    }

    for (size_t i = 0; i < entityCount; i++) {
        gravity->affected[i] = entities[i];
    }

    gravity->totalForce = initial_force;
    gravity->entityCount = entityCount; 

    return gravity;
}

void gravity_free(Gravity *gravity) {
  free(gravity);
}

Environment *environment_new(uint8_t width, uint8_t height) {
  Environment *env = (Environment*)malloc(sizeof(Environment));
  if (!env) {
    fprintf(stderr, "Error: Failed to allocate memory for Environment\n");
    return NULL;
  }

  env->canvas = initCanvas(width, height, ' ');
  if (!env->canvas) {
    fprintf(stderr, "Error: Failed to initialize canvas\n");
    free(env);
    return NULL;
  }

  env->gravity = gravity_new(env->canvas->state.entities, env->canvas->state.entityCount, 0.0);
  if (!env->gravity) {
    fprintf(stderr, "Error: Failed to initialize gravity\n");
    freeCanvas(env->canvas);
    free(env);
    return NULL;
  }

  env->GravitionalCollapseFactor = 1.0;
  env->totalForce = env->gravity->totalForce; 
  env->start = clock();
  env->end = clock();

  return env;
}

void environment_free(Environment *env) {
  freeCanvas(env->canvas);
  gravity_free(env->gravity);
  free(env);
}

void update_gravity_affected(Environment *env) {
    if (!env || !env->canvas || !env->gravity) {
      free(env->gravity->affected);
    }

    env->gravity->affected = (Entity**)malloc(env->canvas->state.entityCount * sizeof(Entity*));

    for (size_t i = 0; i < env->canvas->state.entityCount; i++) {
        env->gravity->affected[i] = env->canvas->state.entities[i];
    }

    env->gravity->entityCount = env->canvas->state.entityCount;
}

void apply_force_all(Environment *env) {
  double dt = (double)(env->end - env->start) / CLOCKS_PER_SEC;
  for (size_t i = 0; i < env->canvas->state.entityCount; i++) {
      if (!env->canvas->state.entities[i] || !env->gravity->affected[i]) {
          continue;
      }
      double *totalForce = calc_force((Element *)env->canvas->state.entities[i], (Element *)env->gravity->affected[i]);
      if (totalForce) {
          apply_force((Element *)env->canvas->state.entities[i], totalForce, dt);
          free(totalForce); 
      }
  }
}

void interact_with_environment(Environment *env, Element* element) {
  //TODO: Implement
  //calc force
  //apply force/return force_x, force_y  
  //derive tensor from element
  //sum tensors's matrices's vectors  
  //mutate matrices using newtonian iteration 
  //aggregate new rgb values

  return;
}

/*
Element *procreate(Element *a, Element *b) {
    Element *offspring = (Element*)malloc(sizeof(Element));
    offspring->entities = entity_new(a->entities->color, a->mass, a->diameter, a->weight);
    offspring->tensor = tensor_procreate(a->tensor, b->tensor, offspring->tensor);
    return offspring;
  return;
}
*/

/*
Entity *init_element(Canvas *canvas, int x, int y, int dx, int dy, uint8_t diameter, Color color) {
  Entity *entity = entity_new(canvas, color, 1, diameter, 1);
  entity->cell.pos.x = x;
  entity->cell.pos.y = y;
  return entity;
}
*/

//use newtonian iteration to allow element to move
void element_free(Element *element) {
  free(element);
  free(element->tensor);
  free(element->bounding);
  for (size_t i = 0; i < sizeof(element->entities)/sizeof(Entity*); i++) {
    free(element->entities[i]);
  }

  for (unsigned int i = 0; i < element->tensor->size; i++) {
       free(element->tensor->matrices[i]);
       for (unsigned int j = 0; j < element->tensor->matrices[i]->size; j++) {
          free(element->tensor->matrices[i]->vectors[j]);
       }
  }
}

void element_move(Canvas *canvas, Entity *entity) {
  moveEntity(canvas, entity, (Pos){0, 0});
  return;
}

Element *element_new(Canvas *canvas, char c, uint8_t x, uint8_t y, Color color, void (*moveFunc)(Canvas *canvas, Entity *entity)) {
  Element *element = (Element*)malloc(sizeof(Element));

  element->bounding = createEntity(ELEMENT, c, x, y, 1, color, moveFunc); 
  element->mass = element->bounding->cell.color.r + element->bounding->cell.color.g + element->bounding->cell.color.b;
  element->diameter = 1;
  element->weight = element->mass; 
  element->tensor = tensor_new(1);

  return element;
}

Element *attribute_new(Element *element, char *key, void *value) {
  // Add new attribute to element's tensor or other properties
  // Implementation is domain-specific, assuming key-value store in tensor for simplicity
  if (strcmp(key, "health") == 0) {
    element->bounding->health = *((int*)value);
  }
  return element;
}

Element *new_property(Element *element, char *key, void *value) {
  // Add new property to element's tensor or other properties
  // Implementation is domain-specific, assuming key-value store in tensor for simplicity
  if (strcmp(key, "isAlive") == 0) {
    element->bounding->isAlive = *((int*)value);
  }
  return element;
}


