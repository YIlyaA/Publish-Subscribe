#ifndef PUBLISH_H
#define PUBLISH_H

#include <pthread.h>

typedef struct {
    void **messages;          // Array of message pointers
    int max_size;             // Maximum capacity of the queue
    int current_size;         // Current number of messages in the queue
    int head;                 // Index of the first message
    int tail;                 // Index of the next available slot

    pthread_mutex_t lock;     // Mutex for thread safety
    pthread_cond_t not_full;  // Condition variable: queue not full
    pthread_cond_t not_empty; // Condition variable: queue not empty

    pthread_t *subscribers;   // Array of subscriber thread IDs
    int num_subscribers;      // Number of current subscribers

    int **read_status;        // 2D array: tracks if each subscriber has read a message
} TQueue;



void createQueue(TQueue *queue, int *size);

void destroyQueue(TQueue *queue);

void subscribe(TQueue *queue, pthread_t *thread);

void unsubscribe(TQueue *queue, pthread_t *thread);

void addMsg(TQueue *queue, void *msg);

void* getMsg(TQueue *queue, pthread_t *thread);

void getAvailable(TQueue *queue, pthread_t *thread);

void removeMsg(TQueue *queue, void *msg);

void setSize(TQueue *queue, int *size);


#endif