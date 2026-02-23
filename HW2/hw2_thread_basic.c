#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* print_thread_info(void* arg) {
    int thread_id = *((int*)arg);
    printf("Thread %d is running\n", thread_id);
    return NULL;
}

int main() {
    pthread_t thread1, thread2, thread3;
    int id1 = 1;
    int id2 = 2;
    int id3 = 3;

    if (pthread_create(&thread1, NULL, print_thread_info, &id1) != 0) {
        perror("Failed to create thread 1");
        return 1;
    }
    if (pthread_create(&thread2, NULL, print_thread_info, &id2) != 0) {
        perror("Failed to create thread 2");
        return 1;
    }
    if (pthread_create(&thread3, NULL, print_thread_info, &id3) != 0) {
        perror("Failed to create thread 3");
        return 1;
    }

    if (pthread_join(thread1, NULL) != 0) {
        perror("Failed to join thread 1");
        return 1;
    }

    if (pthread_join(thread2, NULL) != 0) {
        perror("Failed to join thread 2");
        return 1;
    }

    if (pthread_join(thread3, NULL) != 0) {
        perror("Failed to join thread 3");
        return 1;
    }

    return 0;
}
