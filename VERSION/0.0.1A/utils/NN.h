#ifndef NN_H
#define NN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "environment.h"

typedef struct Matrix Matrix;
typedef struct Tensor Tensor;

typedef struct {
    int index; 
    size_t size; 
    double *data;
    Matrix *parent; 
} Vector;

typedef struct Matrix {
    unsigned int index;
    size_t size; 
    Vector **vectors;
    Tensor *parent;
} Matrix;

typedef enum {
    RULE_NONE = 0,
    RULE_ADD = 1,
    RULE_SUB = 2,
    RULE_RES = -1,
    RULE_ERR = -2
} RULES;

typedef struct {
    int8_t *buffer;
    unsigned int index;
    size_t size;
    int *references;
    RULES *rule;
} MetaOperands;

typedef struct Tensor {
    unsigned int index;
    size_t size;
    Matrix **matrices; 
    MetaOperands *operands;
} Tensor;

typedef struct {
  Entity **affected; 
  double totalForce; 
  size_t entityCount;
} Gravity;

typedef struct {
  Canvas *canvas;
  Gravity *gravity;
  double GravitionalCollapseFactor;
  double totalForce; 
  clock_t start, end;
} Environment;  

typedef struct Element {
  Entity *bounding;
  Entity **entities;
  float mass;
  float weight;
  uint8_t diameter;
  Tensor *tensor;

} Element; 

Vector *vector_new(Matrix *Parent, size_t size, double *data); 
static size_t next_power_of_two(size_t x);
void vector_push(Vector *vector, double data);
void dynamic_vector_push(Vector *vector, unsigned int idx, double *data); 
void vector_pop(Vector *vector);
void vector_free(Vector *vector);

Matrix *matrix_new(unsigned int numVectors, ...); 
void matrix_free(Matrix *matrix); 

Tensor *tensor_new(unsigned int numMatrices, ...);
void tensor_free(Tensor *tensor); 

double *calc_force(Element *element1, Element *element2);
void apply_force(Element *element, double* force_vector, double dt);

Gravity *gravity_new(Entity **entities, size_t entityCount, double initial_force); 

void gravity_free(Gravity *gravity);

Environment *environment_new(uint8_t width, uint8_t height);
void environment_free(Environment *env);
void interact_with_environment(Environment *env, Element *element);
void apply_force_all(Environment *env);
void update_gravity_affected(Environment *env);

Element *create_canvas(Environment *env, Entity *entity);
Element *create_entity(Environment *env, Entity *entity);

Element *element_new(Canvas *canvas, char c, uint8_t x, uint8_t y, Color color, void (*moveFunc)(Canvas *canvas, Entity *entity)); 
void element_free(Element *element);
void element_move(Canvas *canvas, Entity *entity);

Element *attribute_new(Element *element, char *key, void *value);
Element *property_new(Element *element, char *key, void *value);
double *element_sum(Element *element);
Element *element_update(Element *element);
Element *procreate(Element *a, Element *b); 

#endif

