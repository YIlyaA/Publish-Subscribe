#include "publish.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // For sleep()

void* thread_T1(void* arg) {
    TQueue* queue = (TQueue*)arg;
    int counter = 1;

    for (int i = 0; i < 10; i++) { // Add 10 messages
        sleep(3);
        char message[10];
        sprintf(message, "m%d", counter++);
        addMsg(queue, message);
        printf("T1: Put message '%s'\n", message);
    }

    return NULL;
}

void* thread_T2(void* arg) {
    TQueue* queue = (TQueue*)arg;
    pthread_t thread_id = pthread_self();
    subscribe(queue, &thread_id);
    printf("T2: Subscribed\n");

    for (int i = 0; i < 5; i++) { \
        char* message = (char*)getMsg(queue, &thread_id);
        if (message) {
            printf("T2: Received message: %s\n", message);
        }
        sleep(2);
    }

    unsubscribe(queue, &thread_id);
    printf("T2: Unsubscribed\n");

    return NULL;
}


void* thread_T3(void* arg) {
    TQueue* queue = (TQueue*)arg;
    pthread_t thread_id = pthread_self();
    subscribe(queue, &thread_id);
    printf("T3: Subscribed\n");

    for (int i = 0; i < 5; i++) { // Receive 5 messages
        char* message = (char*)getMsg(queue, &thread_id);
        if (message) {
            printf("T3: Received message: %s\n", message);
        }
        sleep(4);
    }

    unsubscribe(queue, &thread_id);
    printf("T3: Unsubscribed\n");

    return NULL;
}


void* thread_T4(void* arg) {
    TQueue* queue = (TQueue*)arg;
    pthread_t thread_id = pthread_self();
    subscribe(queue, &thread_id);
    printf("T4: Subscribed\n");

    char* message = (char*)getMsg(queue, &thread_id);
    if (message) {
        printf("T4: Received message: %s\n", message);
    }
    sleep(1);

    unsubscribe(queue, &thread_id);
    printf("T4: Unsubscribed\n");

    return NULL;
}


int main() {
    TQueue queue;
    int size = 2; // Initial queue size

    createQueue(&queue, &size);
    printf("Queue created with size %d\n", size);

    pthread_t t1, t2, t3, t4;
    // pthread_t t1, t2;

    // Create threads to simulate the example
    pthread_create(&t1, NULL, thread_T1, &queue);
    pthread_create(&t2, NULL, thread_T2, &queue);
    pthread_create(&t3, NULL, thread_T3, &queue);
    pthread_create(&t4, NULL, thread_T4, &queue);

    // Wait for threads to finish
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    printf("All threads finished. Destroying queue.\n");
    destroyQueue(&queue);

    return 0;
}
