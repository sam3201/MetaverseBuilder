#include <stdio.h>
#include "NN.h"

int main(void) {
    printf("Hello World\n");

    unsigned int numInputs = 5;
    unsigned int numHidden = 20;
    unsigned int numOutput = 4;
    double learningRate = 0.1;
    double momentum = 0.9;

    double input[2] = {0.0, 0.0}; 
    double target[1] = {1.0};      

    unsigned int numEpochs = 10000; 

    ActivationFunction sigmoid_activations[20];
    ActivationFunction sigmoid_derivatives[20];
    ActivationFunction tanh_activations[4];
    ActivationFunction tanh_derivatives[4];

    for (int i = 0; i < 20; i++) {
        sigmoid_activations[i] = sigmoid;
        sigmoid_derivatives[i] = sigmoid_derivative;
    }
    for (int i = 0; i < 4; i++) {
        tanh_activations[i] = tanh;
        tanh_derivatives[i] = tanh_derivative;
    }

NN_t *nn = NN_create(numInputs, numHidden, numOutput, 
                     sigmoid_activations, sigmoid_derivatives, 
                     tanh_activations, tanh_derivatives, 
                     learningRate, momentum);
if (!nn) {
    fprintf(stderr, "Failed to create neural network\n");
    return 1;
};
    for (int epoch = 0; epoch < numEpochs; epoch++) {
        forward(nn, input);
        backprop(nn, target);
        
    }
    printf("Error = %.6f\n", nn->error);
    NN_destroy(nn);

    return 0;
}

