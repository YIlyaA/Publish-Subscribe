#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "publish.h"  // Include your publish-subscribe implementation header

// Function declarations for thread behaviors
void* thread_t1(void* arg);
void* thread_t2(void* arg);
void* thread_t3(void* arg);
void* thread_t4(void* arg);

// Shared queue
TQueue queue;

int main() {
    int queue_size = 3;  // Initial size of the queue
    createQueue(&queue, &queue_size);

    // Create threads T1, T2, T3, and T4
    pthread_t t1, t2, t3, t4;
    pthread_create(&t1, NULL, thread_t1, NULL);
    pthread_create(&t2, NULL, thread_t2, NULL);
    pthread_create(&t3, NULL, thread_t3, NULL);
    pthread_create(&t4, NULL, thread_t4, NULL);

    // Wait for threads to finish
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    // Destroy the queue
    destroyQueue(&queue);

    return 0;
}

// Thread T1: Producer and resize operations
void* thread_t1(void* arg) {
    int m1 = 1, m2 = 2, m3 = 3, m4 = 4, m5 = 5, m6 = 6, m7 = 7;

    addMsg(&queue, &m1);  // Put m1
    printf("T1: Put m1\n");

    addMsg(&queue, &m2);  // Put m2
    printf("T1: Put m2\n");

    addMsg(&queue, &m3);  // Put m3
    printf("T1: Put m3\n");

    int new_size = 2;  // Resize queue
    setSize(&queue, &new_size);
    printf("T1: Resized queue to size %d\n", new_size);

    addMsg(&queue, &m4);  // Put m4
    printf("T1: Put m4\n");

    addMsg(&queue, &m5);  // Put m5
    printf("T1: Put m5\n");

    addMsg(&queue, &m6);  // Put m6
    printf("T1: Put m6\n");

    addMsg(&queue, &m7);  // Put m7
    printf("T1: Put m7\n");

    return NULL;
}

// Thread T2: Subscriber consuming messages
void* thread_t2(void* arg) {
    pthread_t self = pthread_self();
    subscribe(&queue, &self);
    printf("T2: Subscribed\n");

    sleep(1);  // Simulate delay

    printf("T2: Available messages: %d\n", queue.current_size);
    int* msg = (int*)getMsg(&queue, &self);
    if (msg) printf("T2: Retrieved message %d\n", *msg);

    unsubscribe(&queue, &self);
    printf("T2: Unsubscribed\n");

    return NULL;
}

// Thread T3: Subscriber consuming messages
void* thread_t3(void* arg) {
    pthread_t self = pthread_self();
    subscribe(&queue, &self);
    printf("T3: Subscribed\n");

    sleep(2);  // Simulate delay

    int* msg = (int*)getMsg(&queue, &self);
    if (msg) printf("T3: Retrieved message %d\n", *msg);

    unsubscribe(&queue, &self);
    printf("T3: Unsubscribed\n");

    return NULL;
}

// Thread T4: Subscriber checking availability and unsubscribing
void* thread_t4(void* arg) {
    pthread_t self = pthread_self();
    subscribe(&queue, &self);
    printf("T4: Subscribed\n");

    sleep(3);  // Simulate delay

    printf("T4: Available messages: %d\n", queue.current_size);
    unsubscribe(&queue, &self);
    printf("T4: Unsubscribed\n");

    return NULL;
}
