#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define ARRAY_SIZE 50000000

int array[ARRAY_SIZE];
int local_maxes[4];
int chunk_size = 0;

void *max_thread(void *arg) {

    int thread_id = *(int*)arg;
    int start = thread_id * chunk_size;
    int end = start + chunk_size;
    int max_val = 0;
    for (int i = start; i < end; i++) {
        if (array[i] > max_val) {
            max_val = array[i];
        }
    }
    local_maxes[thread_id] = max_val;
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
    pthread_t threads[4];
    int thread_ids[4];

    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = rand() % 643;
    }

    int seq_max = 0;
    double start_time = get_time();
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] > seq_max) {
            seq_max = array[i];
        }
    }
    double end_time = get_time();

    printf("Sequential max: %d\n", seq_max);
    printf("Sequential time: %.4f seconds\n", end_time - start_time);
    printf("\n");

    chunk_size = ARRAY_SIZE / 4;
    start_time = get_time();
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i;
        local_maxes[i] = 0;
        if (pthread_create(&threads[i], NULL, max_thread, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    int par_max = 0;
    if (pthread_join(threads[0], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    if (local_maxes[0] > par_max) {
        par_max = local_maxes[0];
    }

    if (pthread_join(threads[1], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    if (local_maxes[1] > par_max) {
        par_max = local_maxes[1];
    }

    if (pthread_join(threads[2], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    if (local_maxes[2] > par_max) {
        par_max = local_maxes[2];
    }

    if (pthread_join(threads[3], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    if (local_maxes[3] > par_max) {
        par_max = local_maxes[3];
    }
    end_time = get_time();

    printf("Parallel max (%d threads): %d\n", 4, par_max);
    printf("Parallel time:   %.4f seconds\n", end_time - start_time);

    return 0;
}
