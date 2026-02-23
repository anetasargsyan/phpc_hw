#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RANGE 20000000

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

long long prime_counts[4];
int chunk_size = 0;

int is_prime(int n) {
    if (n < 2) {
        return 0;
    }
    if (n == 2) {
        return 1;
    }
    if (n % 2 == 0) {
        return 0;
    }
    for (int i = 3; (long long)i * i <= n; i += 2) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

void *prime_thread(void *arg) {
    int thread_id = *(int*)arg;
    int start = thread_id * chunk_size;
    int end = thread_id * chunk_size + chunk_size;
    long long count = 0;
    for (int n = start; n < end; n++) {
        if (is_prime(n)) {
            count++;
        }
    }
    prime_counts[thread_id] = count;
    return NULL;
}

int main() {
    pthread_t threads[4];
    int thread_ids[4];

    long long seq_count = 0;
    double start_time = get_time();
    for (int n = 2; n <= RANGE; n++) {
        if (is_prime(n)) {
            seq_count++;
        }
    }
    double end_time = get_time();

    printf("Sequential primes: %lld\n", seq_count);
    printf("Sequential time: %.4f seconds\n", end_time - start_time);
    printf("\n");

    chunk_size = RANGE / 4;
    start_time = get_time();
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i;
        prime_counts[i] = 0;
        if (pthread_create(&threads[i], NULL, prime_thread, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    long long par_count = 0;
    if (pthread_join(threads[0], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_count += prime_counts[0];

    if (pthread_join(threads[1], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_count += prime_counts[1];

    if (pthread_join(threads[2], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_count += prime_counts[2];

    if (pthread_join(threads[3], NULL) != 0) {
        perror("Failed to join thread");
        return 1;
    }
    par_count += prime_counts[3];
    end_time = get_time();

    printf("Parallel primes (%d threads): %lld\n", 4, par_count);
    printf("Parallel time:   %.4f seconds\n", end_time - start_time);

    return 0;
}
