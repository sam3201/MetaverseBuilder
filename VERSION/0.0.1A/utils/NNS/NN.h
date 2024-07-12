#include <math.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  unsigned int numInputs; 
  double *inputs;
  unsigned int numHidden;
  double *hidden;
  unsigned int numOutput;
  double *output;
  unsigned int numWeights;
  double *weights;
  double *weightsO;
  unsigned int numBiases;
  double *biases;
  double *biasesO;
  double learningRate;
  double momentum;
  double error;
  double *gradient;
  double *gradientO;
} NN_t;


NN_t *NN_create(double *inputs, unsigned int numHidden, unsigned int numOutput, double learningRate, double momentum); 
void NN_destroy(NN_t *nn);
void add_matrices(double *C, double *A, double *B, unsigned int n);
void subtract_matrices(double* C, double* A, double* B, unsigned int n);
double* strassen_matmul(double* A, double* B, double* bias, unsigned int n); 
double *matmul(double *A, double *B, double *C, unsigned int n);
void forward(NN_t *nn, double *input); 
void backward(NN_t *nn, double *target); 
double sigmoid(double x);
double sigmoid_derivative(double x);
double mean_squared_error(double *target, double *output, int num_samples); 
double *train(NN_t *nn, double *input, double *target, int num_samples, int num_epochs); 

