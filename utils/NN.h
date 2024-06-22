#ifndef NN_H
#define NN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Matrix Matrix;

typedef struct {
    int index; 
    size_t size; 
    void **data;
    Matrix *parent; 
} Vector;

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
    vector->data[vector->size] = NULL; // Ensure the popped element is set to NULL
    return data;
}

Vector *vector_free(Vector *vector) {
    free(vector->data);
    free(vector);
    return NULL;
}

typedef struct Matrix {
    int index;
    size_t size; 
    Vector **vectors;
} Matrix;

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
        // Assuming argv is NULL terminated for the example
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

typedef struct {
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
    // Implementation of interaction logic
    // This function should handle how the tensor interacts with its environment
    // and propagate changes to its matrices and vectors
}

void tensor_procreate(Tensor *parent_a, Tensor *parent_b, Tensor **offspring) {
    // Implementation of tensor procreation logic
    // This function should create new tensors based on the genetic combination
    // of parent_a and parent_b
}

#endif

