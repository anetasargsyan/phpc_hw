#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ARRAY_SIZE 50000000

int array[ARRAY_SIZE];
long long partial_sums[5];
int chunk_size = 0;

void *sum_thread(void *arg) {

    int thread_id = *((int*)arg);

    int start = thread_id * chunk_size;
    int end = thread_id * chunk_size + chunk_size;

    long long sum = 0;
    for (int i = start; i < end; i++) {
        sum += array[i];
    }
    partial_sums[thread_id] = sum;
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main()
{
    pthread_t threads[5];
    int thread_ids[5];


    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = rand()%454;
    }

    long long seq_sum = 0;
    double start_time = get_time();

    for (int i = 0; i < ARRAY_SIZE; i++) {
        seq_sum += array[i];
    }
    double end_time = get_time();

    printf("Sequential sum %lld\n", seq_sum);

    printf("Sequential time: %.4f seconds\n", end_time - start_time);
    printf("\n");

    start_time = get_time();
    chunk_size = ARRAY_SIZE / 5;
    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i;
        partial_sums[i] = 0;
        if (pthread_create(&threads[i], NULL, sum_thread, (void*)&thread_ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    long long par_sum = 0;
    if (pthread_join(threads[0], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_sum += partial_sums[0];

    if (pthread_join(threads[1], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_sum += partial_sums[1];

    if (pthread_join(threads[2], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_sum += partial_sums[2];

    if (pthread_join(threads[3], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_sum += partial_sums[3];

    if (pthread_join(threads[4], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_sum += partial_sums[4];
    end_time = get_time();

    printf("Parallel sum (5 threads): %lld\n", par_sum);
    printf("Parallel time: %.4f seconds\n", end_time - start_time);

    return 0;
}