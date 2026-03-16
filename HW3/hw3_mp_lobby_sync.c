#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int players = 0;
int rounds = 0;
int *results = NULL;
int *round_results = NULL;

pthread_barrier_t barrier;

void *routine(void *args) {

    printf("I just joined. Gettiing ready now!\n");
    sleep(rand()%20);

    time_t now = time(NULL);

    printf("I am ready at %s waiting for others \n", ctime(&now));
    pthread_barrier_wait(&barrier);
    printf("All are ready, starting the game\n");

    return NULL;
}

int main(void) {
    printf("Input the player count: ");
    scanf("%d", &players);

    pthread_t th[players];
    pthread_barrier_init(&barrier, NULL, players);

    for (int i = 0; i < players; i++) {
        if (pthread_create(&th[i], NULL, &routine, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < players; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}