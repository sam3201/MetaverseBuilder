#include <math.h>
#include <stdlib.h>
#include "NN.h"

NN_t *NN_create(unsigned int numInputs, unsigned int numHidden, unsigned int numOutput,
                ActivationFunction *hiddenActivations, ActivationFunction *hiddenActivationDerivatives,
                ActivationFunction *outputActivations, ActivationFunction *outputActivationDerivatives,
                double learningRate, double momentum) {
    
    NN_t *nn = (NN_t *)malloc(sizeof(NN_t));
    if (!nn) return NULL;

    nn->numInputs = numInputs;
    nn->numHidden = numHidden;
    nn->numOutput = numOutput;
    nn->learningRate = learningRate;
    nn->momentum = momentum;

    nn->inputs = (double *)calloc(numInputs, sizeof(double));
    nn->hidden = (double *)calloc(numHidden, sizeof(double));
    nn->output = (double *)calloc(numOutput, sizeof(double));

    nn->numWeights = numInputs * numHidden + numHidden * numOutput;
    nn->weights = (double *)malloc(sizeof(double) * nn->numWeights);
    nn->weightsO = (double *)calloc(nn->numWeights, sizeof(double));

    nn->numBiases = numHidden + numOutput;
    nn->biases = (double *)malloc(sizeof(double) * nn->numBiases);
    nn->biasesO = (double *)calloc(nn->numBiases, sizeof(double));

    nn->gradient = (double *)calloc(nn->numWeights, sizeof(double));
    nn->gradientO = (double *)calloc(nn->numWeights, sizeof(double));

    for (unsigned int i = 0; i < nn->numWeights; i++) {
        nn->weights[i] = ((double)rand() / RAND_MAX) * 2 - 1;
    }
    for (unsigned int i = 0; i < nn->numBiases; i++) {
        nn->biases[i] = ((double)rand() / RAND_MAX) * 2 - 1;  
    }

    nn->hiddenActivations = (ActivationFunction *)malloc(numHidden * sizeof(ActivationFunction));
    nn->outputActivations = (ActivationFunction *)malloc(numOutput * sizeof(ActivationFunction));
    nn->hiddenActivationDerivatives = (ActivationFunction *)malloc(numHidden * sizeof(ActivationFunction));
    nn->outputActivationDerivatives = (ActivationFunction *)malloc(numOutput * sizeof(ActivationFunction));

    if (!nn->hiddenActivations || !nn->outputActivations || !nn->hiddenActivationDerivatives || !nn->outputActivationDerivatives) {
        NN_destroy(nn);
        return NULL;
    }

    for (unsigned int i = 0; i < numHidden; i++) {
        nn->hiddenActivations[i] = hiddenActivations[i];
        nn->hiddenActivationDerivatives[i] = hiddenActivationDerivatives[i];
    }
    for (unsigned int i = 0; i < numOutput; i++) {
        nn->outputActivations[i] = outputActivations[i];
        nn->outputActivationDerivatives[i] = outputActivationDerivatives[i];
    }

    return nn;
}

void NN_destroy(NN_t *nn) {
  free(nn->inputs);
  free(nn->hidden);
  free(nn->output);
  free(nn->weights);
  free(nn->weightsO);
  free(nn->biases);
  free(nn->biasesO);
  free(nn->gradient);
  free(nn->gradientO);
  free(nn);
}


void add_matrices(double* C, double* A, double* B, unsigned int n) {
    for (int i = 0; i < n * n; i++) {
        C[i] = A[i] + B[i];
    }
}

void subtract_matrices(double* C, double* A, double* B, unsigned int n) {
    for (int i = 0; i < n * n; i++) {
        C[i] = A[i] - B[i];
    }
}

double* strassen_matmul(double* A, double* B, double* bias, unsigned int n) {
    if (n <= 64) {  
        double* C = (double*)calloc(n * n, sizeof(double));
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                for (int k = 0; k < n; k++) {
                    C[i*n + j] += A[i*n + k] * B[k*n + j];
                }
                C[i*n + j] += bias[j];  
            }
        }
        return C;
    }

    int new_n = n / 2;
    double* A11 = (double*)malloc(new_n * new_n * sizeof(double));
    double* A12 = (double*)malloc(new_n * new_n * sizeof(double));
    double* A21 = (double*)malloc(new_n * new_n * sizeof(double));
    double* A22 = (double*)malloc(new_n * new_n * sizeof(double));
    double* B11 = (double*)malloc(new_n * new_n * sizeof(double));
    double* B12 = (double*)malloc(new_n * new_n * sizeof(double));
    double* B21 = (double*)malloc(new_n * new_n * sizeof(double));
    double* B22 = (double*)malloc(new_n * new_n * sizeof(double));

    for (int i = 0; i < new_n; i++) {
        for (int j = 0; j < new_n; j++) {
            A11[i*new_n + j] = A[i*n + j];
            A12[i*new_n + j] = A[i*n + (j + new_n)];
            A21[i*new_n + j] = A[(i + new_n)*n + j];
            A22[i*new_n + j] = A[(i + new_n)*n + (j + new_n)];
            B11[i*new_n + j] = B[i*n + j];
            B12[i*new_n + j] = B[i*n + (j + new_n)];
            B21[i*new_n + j] = B[(i + new_n)*n + j];
            B22[i*new_n + j] = B[(i + new_n)*n + (j + new_n)];
        }
    }

    double* S1 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S2 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S3 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S4 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S5 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S6 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S7 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S8 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S9 = (double*)malloc(new_n * new_n * sizeof(double));
    double* S10 = (double*)malloc(new_n * new_n * sizeof(double));

    subtract_matrices(S1, B12, B22, new_n);
    add_matrices(S2, A11, A12, new_n);
    add_matrices(S3, A21, A22, new_n);
    subtract_matrices(S4, B21, B11, new_n);
    add_matrices(S5, A11, A22, new_n);
    add_matrices(S6, B11, B22, new_n);
    subtract_matrices(S7, A12, A22, new_n);
    add_matrices(S8, B21, B22, new_n);
    subtract_matrices(S9, A11, A21, new_n);
    add_matrices(S10, B11, B12, new_n);

    double* M1 = strassen_matmul(A11, S1, bias, new_n);
    double* M2 = strassen_matmul(S2, B22, bias, new_n);
    double* M3 = strassen_matmul(S3, B11, bias, new_n);
    double* M4 = strassen_matmul(A22, S4, bias, new_n);
    double* M5 = strassen_matmul(S5, S6, bias, new_n);
    double* M6 = strassen_matmul(S7, S8, bias, new_n);
    double* M7 = strassen_matmul(S9, S10, bias, new_n);

    double* C11 = (double*)malloc(new_n * new_n * sizeof(double));
    double* C12 = (double*)malloc(new_n * new_n * sizeof(double));
    double* C21 = (double*)malloc(new_n * new_n * sizeof(double));
    double* C22 = (double*)malloc(new_n * new_n * sizeof(double));

    add_matrices(C11, M5, M4, new_n);
    subtract_matrices(C11, C11, M2, new_n);
    add_matrices(C11, C11, M6, new_n);

    add_matrices(C12, M1, M2, new_n);

    add_matrices(C21, M3, M4, new_n);

    add_matrices(C22, M5, M1, new_n);
    subtract_matrices(C22, C22, M3, new_n);
    subtract_matrices(C22, C22, M7, new_n);

    double* C = (double*)malloc(n * n * sizeof(double));
    for (int i = 0; i < new_n; i++) {
        for (int j = 0; j < new_n; j++) {
            C[i*n + j] = C11[i*new_n + j];
            C[i*n + (j + new_n)] = C12[i*new_n + j];
            C[(i + new_n)*n + j] = C21[i*new_n + j];
            C[(i + new_n)*n + (j + new_n)] = C22[i*new_n + j];
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i*n + j] += bias[j];
        }
    }

    free(A11); free(A12); free(A21); free(A22);
    free(B11); free(B12); free(B21); free(B22);
    free(S1); free(S2); free(S3); free(S4); free(S5);
    free(S6); free(S7); free(S8); free(S9); free(S10);
    free(M1); free(M2); free(M3); free(M4); free(M5); free(M6); free(M7);
    free(C11); free(C12); free(C21); free(C22);

    return C;
}

double *matmul(double *A1, double *A2, double *A3, unsigned int n) {
  double *C = (double *)calloc(n * n, sizeof(double));

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      for (int k = 0; k < n; k++) {
        C[i * n + j] += A1[i * n + k] * A2[k * n + j];
      }
      C[i * n + j] += A3[i * n + j];
    }
  }

  return C;
}

void forward(NN_t *nn, double *input) {
    for (int i = 0; i < nn->numInputs; i++) {
        nn->inputs[i] = input[i];
    }
    double *hidden_input = matmul(nn->inputs, nn->weights, nn->biases, nn->numHidden);
    for (int i = 0; i < nn->numHidden; i++) {
        nn->hidden[i] = nn->hiddenActivations[i](hidden_input[i]);  
    }
    free(hidden_input);

    double *output_input = matmul(nn->hidden, &nn->weights[nn->numInputs * nn->numHidden], &nn->biases[nn->numHidden], nn->numOutput);
    for (int i = 0; i < nn->numOutput; i++) {
        nn->output[i] = nn->outputActivations[i](output_input[i]);  
    }
    free(output_input);
}

void backprop(NN_t *nn, double *target) {
    double *output_error = (double *)malloc(sizeof(double) * nn->numOutput);
    for (int i = 0; i < nn->numOutput; i++) {
        double derivative = nn->outputActivationDerivatives[i](nn->output[i]);
        output_error[i] = (nn->output[i] - target[i]) * derivative;
    }

    double *hidden_error = (double *)malloc(sizeof(double) * nn->numHidden);
    for (int i = 0; i < nn->numHidden; i++) {
        hidden_error[i] = 0.0;
        for (int j = 0; j < nn->numOutput; j++) {
            hidden_error[i] += output_error[j] * nn->weights[nn->numInputs * nn->numHidden + j * nn->numHidden + i];
        }
        double derivative = nn->hiddenActivationDerivatives[i](nn->hidden[i]);
        hidden_error[i] *= derivative;
    }

    for (int i = 0; i < nn->numOutput; i++) {
        for (int j = 0; j < nn->numHidden; j++) {
            int index = nn->numInputs * nn->numHidden + i * nn->numHidden + j;
            double delta = -nn->learningRate * output_error[i] * nn->hidden[j];
            nn->weights[index] += delta + nn->momentum * nn->weightsO[index];
            nn->weightsO[index] = delta;
        }
        nn->biases[nn->numHidden + i] += -nn->learningRate * output_error[i];
    }

    for (int i = 0; i < nn->numHidden; i++) {
        for (int j = 0; j < nn->numInputs; j++) {
            int index = i * nn->numInputs + j;
            double delta = -nn->learningRate * hidden_error[i] * nn->inputs[j];
            nn->weights[index] += delta + nn->momentum * nn->weightsO[index];
            nn->weightsO[index] = delta;
        }
        nn->biases[i] += -nn->learningRate * hidden_error[i];
    }

    free(output_error);
    free(hidden_error);
}

double sigmoid(double x) {
  return 1.0 / (1.0 + exp(-x));
}

double sigmoid_derivative(double x) {
  return x * (1.0 - x);
}

double tanh(double x) {
  return exp(x) - exp(-x) / exp(x) + exp(-x);; 
}

double tanh_derivative(double x) {
  return 1 - x * x;
}

double relu(double x) {
  return x > 0 ? x : 0;
}

double relu_derivative(double x) {
  return x > 0 ? 1 : 0;
}

double softmax(double x, double *xs) {
  double sum = 0.0;
  for (size_t i = 0; i < sizeof(xs)/sizeof(xs[0]); i++) {
    sum += exp(xs[i]);
  }
  return exp(x) / sum;
}

double softmax_derivative(double x, double *xs) {
  double sum = 0.0;
  for (size_t i = 0; i < sizeof(xs)/sizeof(xs[0]); i++) {
    sum += exp(xs[i]);
  }
  return exp(x) * sum;
}

double cross_entropy(double *target, double *output, int num_samples) {
  double error = 0.0;
  for (int i = 0; i < num_samples; i++) {
    error -= target[i] * log(output[i]);
  }
  return error;
}

double cross_entropy_derivative(double *target, double *output, int num_samples) {
  double error = 0.0;
  for (int i = 0; i < num_samples; i++) {
    error -= target[i] / output[i];
  }
  return error;
}

double mean_squared_error(double *target, double *output, int num_samples) {
  double error = 0.0;
  for (int i = 0; i < num_samples; i++) {
    error += (target[i] - output[i]) * (target[i] - output[i]);
  }
  return error / num_samples;
}

double mean_squared_error_derivative(double *target, double *output, int num_samples) {
  double error = 0.0;
  for (int i = 0; i < num_samples; i++) {
    error += 2 * (target[i] - output[i]);
  }
  return error / num_samples;
}

double *train(NN_t *nn, double *input, double *target, int num_samples, int num_epochs) {
  for (int epoch = 0; epoch < num_epochs; epoch++) {
    for (int i = 0; i < num_samples; i++) {
      forward(nn, input + i * nn->numInputs);
      backprop(nn, target + i * nn->numOutput);
    }
    printf("Epoch %d: Error = %.6f\n", epoch, nn->error);
  }
  return nn->output;
}

