#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 100000000
#define BINS 256

static unsigned char A[N];

void fill_array(void) {
    for (int i = 0; i < N; i++) {
        A[i] = rand() % BINS;
    }
}

void clear_hist(long long hist[BINS]) {
    for (int i = 0; i < BINS; i++) {
        hist[i] = 0;
    }
}

long long check(long long hist[BINS]){
        long long sum;
        for(int i =0; i < BINS; i++){
                sum += hist[i];
        }

        return sum;
}

int main(void) {
    long long hist_naive[BINS];
    long long hist_critical[BINS];
    long long hist_reduction[BINS];
    double start, end;

    srand(42);
    fill_array();

    clear_hist(hist_naive);
    start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        hist_naive[A[i]]++;
    }
    end = omp_get_wtime();
    printf("Naive: %.4f sec\n", end - start);
        printf("Naive check: %lld\n", check(hist_naive));

    clear_hist(hist_critical);
    start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        hist_critical[A[i]]++;
    }
    end = omp_get_wtime();
    printf("Critical: %.4f sec\n", end - start);
 printf("Critical check: %lld\n", check(hist_critical));
    clear_hist(hist_reduction);
    start = omp_get_wtime();
    #pragma omp parallel for reduction(+:hist_reduction[:BINS]) //had to google this one :)
    for (int i = 0; i < N; i++) {
        hist_reduction[A[i]]++;
    }
    end = omp_get_wtime();
    printf("Reduction: %.4f sec\n", end - start);
 printf("Reduction check: %lld\n", check(hist_reduction));
    return 0;
}