#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <omp.h>

#define N 50000000
static double A[N];

void fill_array(void) {
    for (int i = 0; i < N; i++) {
        A[i] = ((double)rand()/RAND_MAX) * 500;
    }
}

int main(void) {
    double min_diff = 501;
    double start, end;

    srand(42);
    fill_array();

    start = omp_get_wtime();
    #pragma omp parallel for reduction(min:min_diff)
    for (int i = 1; i < N; i++) {
        double diff = fabs(A[i] - A[i - 1]);
        if (diff < min_diff) {
            min_diff = diff;
        }
    }
    end = omp_get_wtime();

    printf("Min: %.20e\n", min_diff);
    printf("Time: %.4f sec\n", end - start);

    return 0;
}