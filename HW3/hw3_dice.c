#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int players = 0;
int rounds = 0;
int *results = NULL;
int *round_results = NULL;

pthread_barrier_t barrier;

void *routine(void *args) {
    int player_id = *(int *)args;
    round_results[player_id] = rand() % 6 + 1;

    printf("Player %d rolled %d\n", player_id + 1, round_results[player_id]);
    pthread_barrier_wait(&barrier);
    printf("Player %d passed the barrier\n", player_id + 1);

    return NULL;
}

int main(void) {
    printf("Input the player count: ");
    scanf("%d", &players);

    printf("Input the round count: ");
    scanf("%d", &rounds);

    results = malloc(sizeof(int) * players);
    round_results = malloc(players * sizeof(int));
    int ids[players];

    for(int i  = 0; i < players; i ++){
        results[i]=round_results[i]=0;
        ids[i] = i;
    }


    pthread_t th[players];
    pthread_barrier_init(&barrier, NULL, players);

    for (int r = 1; r <= rounds; r++) {
        printf("\nRound %d\n", r);

        for (int i = 0; i < players; i++) {
            if (pthread_create(&th[i], NULL, &routine, &ids[i]) != 0) {
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

        int winner_id = 0;
        int max_roll = round_results[0];

        for (int i = 1; i < players; i++) {
            if (round_results[i] > max_roll) {
                max_roll = round_results[i];
                winner_id = i;
            }
        }

        printf("Winner for round %d/%d is player %d with %d 🥳\n", r, rounds, winner_id + 1, max_roll);
        results[winner_id]++;

        for (int i = 0; i < players; i++) {
            round_results[i] = 0;
        }
    }

    int winner_id = 0;
    int max_wins = results[0];

    for (int i = 1; i < players; i++) {
        if (results[i] > max_wins) {
            max_wins = results[i];
            winner_id = i;
        }
    }

    printf("\nOverall winner is player %d with %d wins 🥳\n", winner_id + 1, max_wins);

    pthread_barrier_destroy(&barrier);
    free(results);
    free(round_results);
    return 0;
}