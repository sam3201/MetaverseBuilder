#include <stdio.h>
#include "NN.h"

int main(void) {
  printf("Hello World\n");

   unsigned int numInputs = 2;
    unsigned int numHidden = 3;
    unsigned int numOutput = 1;
    double learningRate = 0.1;
    double momentum = 0.9;

   double input[4] = {0.0, 0.0, 0.0, 1.0}; 
   double target[1] = {1.0}; 
   double hidden[2];
   double hiddenO[1];
   double output[2];


   NN_t* nn = NN_create(input, numHidden, numOutput, learningRate, momentum); 

   unsigned int numEpoch = INFINITY; 
   double * res = train(nn, input, target, 4, numEpoch);
   for (unsigned int i = 0; i < sizeof(res) / sizeof(res[0]); i++) {
     printf("%lf\n", res[i]);
   }

   for (unsigned int i = 0; i < sizeof(target) / sizeof(target[0]); i++) {
    printf("%lf\n", target[i]);
   }

  /*
  double *train(NN_t *nn, double *input, double *target, int num_samples, int num_epochs) {
  for (int epoch = 0; epoch < num_epochs; epoch++) {
    for (int i = 0; i < num_samples; i++) {
      forward(nn, input + i * nn->numInputs);
      backward(nn, target + i * nn->numOutput);
    }
    printf("Epoch %d: Error = %.6f\n", epoch, nn->error);
  }
  return nn->output;
  }
  */


   NN_destroy(nn);

  return 0;
}
