#include "publish.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// Create a new queue
void createQueue(TQueue *queue, int *size) {
    queue->size = *size;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->messages = malloc(sizeof(void*) * queue->size);
    if (queue->messages == NULL || queue->size <= 0) {
        fprintf(stderr, "Error: malloc memory 'createQuery' for messages");
        return;
    }

    queue->delivery_map = malloc(sizeof(int*) * queue->size);
    if (queue->delivery_map == NULL) {
        fprintf(stderr, "Error: malloc memory 'createQuery' for delivery_map");
        return;
    }
    for (int i = 0; i < queue->size; i++) {
        queue->delivery_map[i] = NULL;
    }

    queue->subscribers = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
}

// Destroy the queue
void destroyQueue(TQueue *queue) {
    for (int i = 0; i < queue->size; i++) {
        if (queue->delivery_map[i]) {
            free(queue->delivery_map[i]);
        }
    }
    free(queue->delivery_map);
    free(queue->messages);

    SubscriberNode *current = queue->subscribers;
    while (current) {
        SubscriberNode *temp = current;
        current = current->next;
        free(temp);
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
}

// Subscribe a thread
void subscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    // Ensure thread is not already subscribed
    SubscriberNode *current = queue->subscribers;
    while (current) {
        if (pthread_equal(current->thread_id, *thread)) {
            pthread_mutex_unlock(&queue->mutex);
            return; // Thread already subscribed
        }
        current = current->next;
    }

    // Add the new subscriber
    SubscriberNode *new_node = malloc(sizeof(SubscriberNode));
    new_node->thread_id = *thread;
    new_node->next = queue->subscribers;
    queue->subscribers = new_node;

    // Update delivery_map for existing messages
    for (int i = 0; i < queue->count; i++) {
        int index = (queue->head + i) % queue->size;
        int subscriber_count = countSubscribers(queue);
        queue->delivery_map[index] = realloc(queue->delivery_map[index], sizeof(int) * subscriber_count);
        queue->delivery_map[index][subscriber_count - 1] = 0; // Not delivered to new subscriber
    }

    pthread_mutex_unlock(&queue->mutex);
}

// Unsubscribe a thread
void unsubscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    SubscriberNode **current = &queue->subscribers;
    int subscriber_index = 0;

    // Find and remove the subscriber
    while (*current) {
        if (pthread_equal((*current)->thread_id, *thread)) {
            SubscriberNode *temp = *current;
            *current = (*current)->next;
            free(temp);

            // Mark all messages as delivered for the unsubscribed thread
            for (int i = 0; i < queue->count; i++) {
                int index = (queue->head + i) % queue->size;
                queue->delivery_map[index][subscriber_index] = 1;
            }

            break;
        }
        current = &(*current)->next;
        subscriber_index++;
    }

    pthread_mutex_unlock(&queue->mutex);
}

// Add a message to the queue
void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->mutex);

    // Remove message if no subscribers
    if (queue->subscribers == NULL) {
        printf("No subscribers. Message '%s' discarded.\n", (char*)msg);
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    // Wait until there is space in the queue
    while (queue->count == queue->size) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    // Add message to the queue
    queue->messages[queue->tail] = msg;

    // Initialize delivery map
    int subscriber_count = countSubscribers(queue);
    queue->delivery_map[queue->tail] = malloc(sizeof(int) * subscriber_count);
    for (int i = 0; i < subscriber_count; i++) {
        queue->delivery_map[queue->tail][i] = 0; // Not delivered to any subscriber
    }

    // printDeliveryMap(queue);   // print map 

    queue->tail = (queue->tail + 1) % queue->size;
    queue->count++;

    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

void* getMsg(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    SubscriberNode *subscriber = queue->subscribers;
    int subscriber_index = 0;
    while (subscriber) {
        if (pthread_equal(subscriber->thread_id, *thread)) {
            break;
        }
        subscriber = subscriber->next;
        subscriber_index++;
    }

    if (!subscriber) {
        printf("Thread is not subscribed. Message not available.\n");
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    int index = queue->head;

    if (queue->delivery_map[index][subscriber_index] == 1) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    queue->delivery_map[index][subscriber_index] = 1;

    int all_delivered = 1;
    SubscriberNode *current = queue->subscribers;
    int i = 0;
    while (current) {
        if (queue->delivery_map[index][i] == 0) {
            all_delivered = 0;
            break;
        }
        current = current->next;
        i++;
    }

    if (all_delivered) {
        free(queue->delivery_map[index]);
        queue->delivery_map[index] = NULL;
        queue->head = (queue->head + 1) % queue->size;
        queue->count--;
        pthread_cond_signal(&queue->not_full);
    }

    void *msg = queue->messages[index];
    pthread_mutex_unlock(&queue->mutex);
    return msg;
}

void getAvailable(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    SubscriberNode *subscriber = queue->subscribers;
    int subscriber_index = 0;
    while (subscriber) {
        if (pthread_equal(subscriber->thread_id, *thread)) {
            break;
        }
        subscriber = subscriber->next;
        subscriber_index++;
    }

    if (!subscriber) {
        printf("Thread is not subscribed. No messages available.\n");
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    int available = 0;
    for (int i = 0; i < queue->count; i++) {
        int index = (queue->head + i) % queue->size;
        if (queue->delivery_map[index][subscriber_index] == 0) {
            available++;
        }
    }

    printf("Messages available for thread: %d\n", available);
    pthread_mutex_unlock(&queue->mutex);
}


// Count the number of subscribers
int countSubscribers(TQueue *queue) {
    int count = 0;
    SubscriberNode *current = queue->subscribers;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}


// void printDeliveryMap(TQueue* queue) {
//     printf("Delivery map:\n");
//     for (int i = 0; i < queue->size; i++) {
//         if (queue->delivery_map[i] != NULL) {
//             printf("Message at index %d: [", i);
//             int subscriber_count = countSubscribers(queue);
//             for (int j = 0; j < subscriber_count; j++) {
//                 printf("%d", queue->delivery_map[i][j]);
//                 if (j < subscriber_count - 1) printf(", ");
//             }
//             printf("]\n");
//         } else {
//             printf("Message at index %d: NULL\n", i);
//         }
//     }
// }
