#ifndef NN_H
#define NN_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef double (*ActivationFunction)(double);

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
  ActivationFunction *hiddenActivations;
  ActivationFunction *outputActivations;
  ActivationFunction *hiddenActivationDerivatives;
  ActivationFunction *outputActivationDerivatives;
} NN_t;

NN_t *NN_create(unsigned int numInputs, unsigned int numHidden, unsigned int numOutput,ActivationFunction *hiddenActivations, ActivationFunction *outputActivations, ActivationFunction *hiddenActivationDerivatives, ActivationFunction *outputActivationDerivatives, double learningRate, double momentum);

void NN_destroy(NN_t *nn);

void forward(NN_t *nn, double *input);
void backprop(NN_t *nn, double *target);
double *train(NN_t *nn, double *input, double *target, int num_samples, int num_epochs); 
void test(NN_t *nn, double *inputs, double *targets, int num_samples); 

double sigmoid(double x);
double sigmoid_derivative(double x);
double relu(double x);
double relu_derivative(double x);
double tanh_activation(double x);
double tanh_derivative(double x);

double mean_squared_error(double *target, double *output, int num_samples);
double mean_squared_error_derivative(double *target, double *output, int num_samples);
double cross_entropy(double *target, double *output, int num_samples);
double cross_entropy_derivative(double *target, double *output, int num_samples);

#endif 
