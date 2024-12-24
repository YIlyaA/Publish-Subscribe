#include <stdio.h>
#include <stdlib.h>
#include "publish.h"

void createQueue(TQueue *queue, int *size) {
    queue->messages = (void **)malloc(sizeof(void *) * (*size));
    queue->max_size = *size;
    queue->current_size = 0;
    queue->head = 0;
    queue->tail = 0;

    queue->subscribers = NULL;
    queue->num_subscribers = 0;

    queue->read_status = (int **)malloc(sizeof(int *) * (*size));
    for (int i = 0; i < *size; i++) {
        queue->read_status[i] = NULL; // Initialize as NULL
    }

    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
}

void destroyQueue(TQueue *queue) {
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);

    for (int i = 0; i < queue->max_size; i++) {
        if (queue->read_status[i]) free(queue->read_status[i]);
    }

    free(queue->read_status);
    free(queue->messages);
    free(queue->subscribers);
}

void subscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->lock);

    queue->subscribers = realloc(queue->subscribers, sizeof(pthread_t) * (queue->num_subscribers + 1));
    queue->subscribers[queue->num_subscribers] = *thread;

    for (int i = 0; i < queue->max_size; i++) {
        if (queue->read_status[i] == NULL) {
            queue->read_status[i] = calloc(queue->num_subscribers + 1, sizeof(int));
        } else {
            queue->read_status[i] = realloc(queue->read_status[i], sizeof(int) * (queue->num_subscribers + 1));
            queue->read_status[i][queue->num_subscribers] = 0;
        }
    }

    queue->num_subscribers++;
    pthread_mutex_unlock(&queue->lock);
}

void unsubscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->lock);

    for (int i = 0; i < queue->num_subscribers; i++) {
        if (pthread_equal(queue->subscribers[i], *thread)) {
            queue->subscribers[i] = queue->subscribers[--queue->num_subscribers];
            break;
        }
    }

    pthread_mutex_unlock(&queue->lock);
}

void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->lock);

    while (queue->current_size == queue->max_size) {
        pthread_cond_wait(&queue->not_full, &queue->lock);
    }

    queue->messages[queue->tail] = msg;
    queue->read_status[queue->tail] = calloc(queue->num_subscribers, sizeof(int));
    queue->tail = (queue->tail + 1) % queue->max_size;
    queue->current_size++;

    if (queue->num_subscribers == 0) {
        free(queue->read_status[queue->head]);
        queue->read_status[queue->head] = NULL;
        queue->head = (queue->head + 1) % queue->max_size;
        queue->current_size--;
    }

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
}

void* getMsg(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->lock);

    while (queue->current_size == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    int subscriber_index = -1;
    for (int i = 0; i < queue->num_subscribers; i++) {
        if (pthread_equal(queue->subscribers[i], *thread)) {
            subscriber_index = i;
            break;
        }
    }

    if (subscriber_index == -1) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    while (queue->read_status[queue->head][subscriber_index] == 1) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    void *msg = queue->messages[queue->head];
    queue->read_status[queue->head][subscriber_index] = 1;

    int all_read = 1;
    for (int i = 0; i < queue->num_subscribers; i++) {
        if (queue->read_status[queue->head][i] == 0) {
            all_read = 0;
            break;
        }
    }

    if (all_read) {
        free(queue->read_status[queue->head]);
        queue->read_status[queue->head] = NULL;
        queue->head = (queue->head + 1) % queue->max_size;
        queue->current_size--;
        pthread_cond_signal(&queue->not_full);
    }

    pthread_mutex_unlock(&queue->lock);
    return msg;
}

void getAvailable(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->lock);

    int subscriber_index = -1;
    for (int i = 0; i < queue->num_subscribers; i++) {
        if (pthread_equal(queue->subscribers[i], *thread)) {
            subscriber_index = i;
            break;
        }
    }

    if (subscriber_index == -1) {
        pthread_mutex_unlock(&queue->lock);
        return;
    }

    int unread_count = 0;
    for (int i = 0; i < queue->current_size; i++) {
        int index = (queue->head + i) % queue->max_size;
        if (queue->read_status[index][subscriber_index] == 0) unread_count++;
    }

    printf("Available messages for thread %ld: %d\n", (long)*thread, unread_count);
    pthread_mutex_unlock(&queue->lock);
}

void setSize(TQueue *queue, int *size) {
    pthread_mutex_lock(&queue->lock);

    if (*size < queue->current_size) {
        int diff = queue->current_size - *size;
        for (int i = 0; i < diff; i++) {
            free(queue->read_status[queue->head]);
            queue->read_status[queue->head] = NULL;
            queue->head = (queue->head + 1) % queue->max_size;
        }
        queue->current_size = *size;
    }

    queue->messages = realloc(queue->messages, sizeof(void *) * (*size));
    queue->read_status = realloc(queue->read_status, sizeof(int *) * (*size));
    queue->max_size = *size;

    pthread_mutex_unlock(&queue->lock);
}
