#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <omp.h>

#define N 50000000
static double A[N];

void fill_array(void) {
    for (int i = 0; i < N; i++) {
        A[i] = ((double)rand()/RAND_MAX) * 500;
    }
}

int main(void) {
    double max = -DBL_MAX;

    srand(42);
    fill_array();

    #pragma omp parallel for reduction(max:max)
    for (int i = 0; i < N; i++) {
        if (A[i] > max) {
            max = A[i];
        }
    }
    double t = 0.8 * max;

    double sum = 0.0;

    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < N; i++) {
        if (A[i] > t) {
            sum += A[i];
        }
    }

    printf("Max: %.8f\n", max);
    printf("Sum: %.8f\n", sum);

    return 0;
}