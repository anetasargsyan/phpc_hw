#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

pthread_barrier_t barrier;

void *routine(void *args) {

    int id = *(int *)args;

    // Stage 1
    
    printf("Thread %d: START stage --1--\n", id);
    sleep(rand()%5);
    printf("ready to pass \n");

    pthread_barrier_wait(&barrier);
    printf("Thread %d: END stage --1--\n", id);


    // Stage 2
    printf("Thread %d: START stage --2--\n", id);
    sleep(rand()%5);
    printf("ready to pass\n");

    pthread_barrier_wait(&barrier);
    printf("Thread %d: END stage --2--\n", id);


    // Stage 3
    printf("Thread %d: START stage --3--\n", id);
    sleep(rand()%5);
    printf("ready to pass\n");

    pthread_barrier_wait(&barrier);
    printf("Thread %d: END stage --3--\n", id);

    printf("Thread %d: DONE\n", id);

    return NULL;
}

int threads;
int main(void) {
    printf("Input thread count: ");
    scanf("%d", &threads);

    int ids[threads];
    for (int i = 0; i < threads; i++)
    {
        ids[i] = i+1;
    }
    
    pthread_t th[threads];
    pthread_barrier_init(&barrier, NULL, threads);

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&th[i], NULL, &routine, &ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < threads; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}
