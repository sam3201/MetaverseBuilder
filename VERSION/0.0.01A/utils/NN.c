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
  if (vector->size >= 0) {
    vector->data = (double *)realloc(vector->data, vector->size * sizeof(double));
    return;
  } else {
    Vector *new_vector = vector_new(vector->parent, 1, &data); 
    vector_free(vector); 
    Vector *vector = new_vector;
  }
  vector->size++;
  vector->data[vector->size] = data;
}

void dynamic_vector_push(Vector *vector, unsigned int idx, double *data) {
  size_t absolute_diff = (idx > vector->size) ? idx - vector->size : vector->size - idx;

  size_t new_size = (idx + (sizeof(data)/sizeof(data[0]) >> 1)) + absolute_diff;

  if (new_size > vector->size) {
    size_t full_size = next_power_of_two(new_size); 
    vector->data = (double*)realloc(vector->data, full_size * sizeof(double));
    vector->size = full_size;
  }

  if (idx > vector->size) {
    memset(&vector->data[vector->size], 0, (idx - vector->size) * sizeof(double));
  }

  memcpy(&vector->data[idx], data, sizeof(data)/sizeof(data[0]) * sizeof(double));
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
  tensor->operands->rule = RULE_NONE; 

  return tensor;
}

void tensor_free(Tensor *tensor) {
  for (size_t i = 0; i < tensor->size; i++) {
    matrix_free(tensor->matrices[i]);
  }
  free(tensor->matrices);
  free(tensor);
}

double *calc_force(Gravity *gravity, Element *element1, Element *element2) {
  double dx = element1->entities[0]->cell.pos.x - element2->entities[0]->cell.pos.x; 
  double dy = element1->entities[0]->cell.pos.y - element2->entities[0]->cell.pos.y;
  double dist = sqrt((dx * dx) + (dy * dy));

  double force_x = gravity->totalForce * dx / dist;
  double force_y = gravity->totalForce * dy / dist;
  gravity->totalForce += force_x + force_y;

  double *return_val = malloc(sizeof(double)*2);
  return_val[0] = force_x;
  return_val[1] = force_y;

  return return_val; 
}

void apply_force(Element *element1, double force_x, double force_y, double dt) {
  double accel_x = force_x / element1->mass;
  double accel_y = force_y / element1->mass;

  double dx = accel_x * dt;
  double dy = accel_y * dt;

  element1->entities[0]->cell.pos.x = accel_x * dt;
  element1->entities[0]->cell.pos.y = accel_y * dt;
}

Gravity *gravity_new(Entity **entities, double initial_force) {
  Gravity *gravity = (Gravity*)malloc(sizeof(Gravity));
  gravity->affected = (Entity**)malloc(sizeof(entities)/sizeof(Entity*));
  for (size_t i = 0; i <sizeof(entities)/sizeof(Entity*); i++) {
    gravity->affected[i] = entities[i];
  }
  gravity->totalForce = initial_force;
  return gravity;
}

void gravity_free(Gravity *gravity) {
  free(gravity);
}

Environment *environment_new(uint8_t width, uint8_t height) {
  Environment *env = (Environment*)malloc(sizeof(Environment));
  env->canvas = initCanvas(width, height, ' ');
  env->gravity = gravity_new(env->canvas->state.entities, 0.0);
  env->GravitionalCollapseFactor = 1.0;
  env->start = clock();
  env->end = clock();
  return env;
}

void interact_with_environment(Environment *env, Element* element) {
  //TODO: Implement
  //calc_force
  //apply_force/return force_x, force_y  
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
void element_move(Element *element, uint8_t x, uint8_t y) {
    element->entities[0]->cell.pos.x = x;
    element->entities[0]->cell.pos.y = y;
}

Element *element_new(Canvas *canvas, char c, uint8_t x, uint8_t y, Color color, void (*moveFunc)(Canvas *canvas, Entity *entity)) {
  Element *element = (Element*)malloc(sizeof(Element));
  Entity *initial_entity = createEntity(ELEMENT, c, x, y, 1, color, moveFunc); 

  element->bounding = initial_entity;
  element->tensor = tensor_new(1);
  return element;
}

void element_free(Element *element) {
    if (element) {
        if (element->entities) {
            for (size_t i = 0; i < sizeof(element->entities)/sizeof(Element*); i++) {
                deleteEntity(element->entities[i]);
            }
            free(element->entities);
        }
        if (element->tensor) tensor_free(element->tensor);
        free(element);
    }
}

Element *attribute_new(Element *element, char *key, void *value) {
  // Add new attribute to element's tensor or other properties
  // Implementation is domain-specific, assuming key-value store in tensor for simplicity
  if (strcmp(key, "health") == 0) {
    element->entities[0]->health = *((int*)value);
  }
  return element;
}

Element *new_property(Element *element, char *key, void *value) {
  // Add new property to element's tensor or other properties
  // Implementation is domain-specific, assuming key-value store in tensor for simplicity
  if (strcmp(key, "isAlive") == 0) {
    element->entities[0]->isAlive = *((int*)value);
  }
  return element;
}




