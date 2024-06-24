#ifndef NN_H
#define NN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "environment.h"

typedef struct Matrix Matrix;

typedef struct {
    int index; 
    size_t size; 
    void **data;
    Matrix *parent; 
} Vector;

typedef struct Matrix {
    int index;
    size_t size; 
    Vector **vectors;
} Matrix;

typedef struct {
    int index;
    size_t size;
    Matrix **matrices; 
    union {
        int8_t *buffer;
        unsigned int index;
        int *references;
        enum {
            RULE_NONE = 0,
            RULE_ADD = 1,
            RULE_MUL = 2,
            RULE_RES = -1,
            RULE_ERR = -2
        } RULES;
    } MetaOperands;
} Tensor;


typedef struct {
  Entity *affected; 
  float force; 

} Gravity;

typedef struct {
  Canvas *canvas;
  Gravity **totalGravity;
  float GravitionalCollapseFactor;
  clock_t start, end;
} Environment;  

typedef struct {
  unsigned int index;
  Entity *entity;
  float mass;
  float weight;
  uint8_t diameter;
  Tensor *tensor;
   
} Element; 

Vector *vector_new(Matrix *Parent, size_t size, void *data); 
Vector *vector_push(Vector *vector, void *data);
Vector *dynamic_vector_push(Vector *vector, unsigned int idx, void *data, float *argv); 
void *vector_pop(Vector *vector);
Matrix *matrix_new(Vector **argv);
Matrix *matrix_free(Matrix *matrix); 
Tensor *tensor_new(size_t size);
Tensor *tensor_free(Tensor *tensor); 
Tensor *Procreate(Tensor *a, Tensor *b, Tensor *offspring);
float Force(Gravity *gravity, Element *element1, Element *element2); 
Entity *new_entity(Color color, float mass, float diameter, float weight);
Gravity *new_gravity(Entity *entity, float force);
Environment *new_environment(uint8_t width, uint8_t height);
Element *interact_with_environment(Environment *env, Entity *entity);
Element *create_canvas(Environment *env, Entity *entity);
Element *create_entity(Environment *env, Entity *entity);
Element *procreate(Element *a, Element *b); 
Element *new_element(Canvas *canvas, Entity *entity);
Element *new_attribute(Element *element, char *key, void *value);
Element *new_property(Element *element, char *key, void *value);
double *sum_element(Element *element);
Element *update(Element *element);

#endif

