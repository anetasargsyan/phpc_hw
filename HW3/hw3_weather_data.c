#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int sensors = 0;
long results = 0;
pthread_barrier_t barrier;
pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;

void *routine(void *args) {

    int id = *(int *)args;

    printf("Called to sense data!\n");
    sleep(rand()%5);

    time_t now = time(NULL);
    long temp = 15+ rand()%12;

    printf("I am ready at %s the temp is %ld\n", ctime(&now), temp);
    
    pthread_mutex_lock(&results_mutex);
    results += temp;
    pthread_mutex_unlock(&results_mutex);
    pthread_barrier_wait(&barrier);
    printf("Done!\n");

    return NULL;
}

int main(void) {
    printf("Input the sensor count: ");
    scanf("%d", &sensors);

    pthread_t th[sensors];
    pthread_barrier_init(&barrier, NULL, sensors);
    pthread_mutex_init(&results_mutex, NULL);

    int ids[sensors];
    for (int i = 0; i < sensors; i++) {
        ids[i] = i;
    }


    for (int i = 0; i < sensors; i++) {
        if (pthread_create(&th[i], NULL, &routine, &ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < sensors; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

float avg = results/sensors;
    printf ("The avg is: %.2f\n", avg);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&results_mutex);
    return 0;
}