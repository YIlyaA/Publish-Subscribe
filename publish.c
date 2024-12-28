#include "publish.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// Create a new queue
void createQueue(TQueue *queue, int *size) {
    // Initialize queue parameters
    queue->size = *size;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    // Allocate memory for the message buffer
    queue->messages = malloc(sizeof(void*) * queue->size);
    if (queue->messages == NULL || queue->size <= 0) {
        fprintf(stderr, "Error: malloc memory 'createQueue' for messages");
        return;
    }

    // Allocate memory for the delivery map
    queue->delivery_map = malloc(sizeof(int*) * queue->size);
    if (queue->delivery_map == NULL) {
        fprintf(stderr, "Error: malloc memory 'createQueue' for delivery_map");
        return;
    }

    // Initialize the delivery map to NULL for each message slot
    for (int i = 0; i < queue->size; i++) {
        queue->delivery_map[i] = NULL;
    }

    // Initialize subscribers list as empty
    queue->subscribers = NULL;

    // Initialize mutex and condition variables
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
}

// Destroy the queue
void destroyQueue(TQueue *queue) {
    // Free the memory allocated for the delivery map
    for (int i = 0; i < queue->size; i++) {
        if (queue->delivery_map[i]) {
            free(queue->delivery_map[i]);
        }
    }
    free(queue->delivery_map);

    // Free the memory allocated for the messages buffer
    free(queue->messages);

    // Free all subscriber nodes in the linked list
    SubscriberNode *current = queue->subscribers;
    while (current) {
        SubscriberNode *temp = current;
        current = current->next;
        free(temp);
    }

    // Destroy mutex and condition variables
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
}

// Subscribe a thread
void subscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    // Check if the thread is already subscribed
    SubscriberNode *current = queue->subscribers;
    while (current) {
        if (pthread_equal(current->thread_id, *thread)) {
            pthread_mutex_unlock(&queue->mutex);
            return; // Thread already subscribed
        }
        current = current->next;
    }

    // Add the new subscriber to the list
    SubscriberNode *new_node = malloc(sizeof(SubscriberNode));
    new_node->thread_id = *thread;
    new_node->next = queue->subscribers;
    queue->subscribers = new_node;

    // Update delivery map for existing messages
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

    // Find and remove the subscriber from the list
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

    // Discard the message if there are no subscribers
    if (queue->subscribers == NULL) {
        printf("No subscribers. Message '%s' discarded.\n", (char*)msg);
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    // Wait until there is space in the queue
    while (queue->count == queue->size) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    // Add the message to the queue
    queue->messages[queue->tail] = msg;

    // Initialize the delivery map for the message
    int subscriber_count = countSubscribers(queue);
    queue->delivery_map[queue->tail] = malloc(sizeof(int) * subscriber_count);
    for (int i = 0; i < subscriber_count; i++) {
        queue->delivery_map[queue->tail][i] = 0; // Not delivered to any subscriber
    }

    // Update queue state
    queue->tail = (queue->tail + 1) % queue->size;
    queue->count++;

    // Signal that the queue is not empty
    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

// Get a message for a specific thread
void* getMsg(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    // Wait until there are messages in the queue
    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    // Find the subscriber's index
    SubscriberNode *subscriber = queue->subscribers;
    int subscriber_index = 0;
    while (subscriber) {
        if (pthread_equal(subscriber->thread_id, *thread)) {
            break;
        }
        subscriber = subscriber->next;
        subscriber_index++;
    }

    // If thread is not subscribed, return NULL
    if (!subscriber) {
        printf("Thread is not subscribed. Message not available.\n");
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    // Get the next message for the thread
    int index = queue->head;

    // Check if the message has already been delivered to the thread
    if (queue->delivery_map[index][subscriber_index] == 1) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    // Mark the message as delivered to the thread
    queue->delivery_map[index][subscriber_index] = 1;

    // Check if the message has been delivered to all subscribers
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

    // If all subscribers have received the message, remove it from the queue
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

// Print the number of available messages for a thread
void getAvailable(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->mutex);

    // Find the subscriber's index
    SubscriberNode *subscriber = queue->subscribers;
    int subscriber_index = 0;
    while (subscriber) {
        if (pthread_equal(subscriber->thread_id, *thread)) {
            break;
        }
        subscriber = subscriber->next;
        subscriber_index++;
    }

    // If the thread is not subscribed, print an error
    if (!subscriber) {
        printf("Thread is not subscribed. No messages available.\n");
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    // Count the number of messages available for the thread
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