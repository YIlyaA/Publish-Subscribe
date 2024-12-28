#ifndef PUBLISH_H
#define PUBLISH_H

#include <pthread.h>

typedef struct SubscriberNode {
    pthread_t thread_id; // ID wątku subskrybenta
    struct SubscriberNode* next; // Następny subskrybent w liście
} SubscriberNode;

typedef struct TQueue {
    void** messages;     // Tablica wskaźników na wiadomości
    int size;            // Maksymalna pojemność kolejki
    int count;           // Aktualna liczba wiadomości w kolejce
    int head;            // Indeks początku kolejki (dla bufora cyklicznego)
    int tail;            // Indeks końca kolejki (dla bufora cyklicznego)

    SubscriberNode* subscribers; // Lista subskrybentów
    int** delivery_map;          // Mapa dostarczania wiadomości

    pthread_mutex_t mutex;       // Mutex do synchronizacji
    pthread_cond_t not_full;     // Zmienna warunkowa dla pełnej kolejki
    pthread_cond_t not_empty;    // Zmienna warunkowa dla pustej kolejki
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


// help functions
int countSubscribers(TQueue *queue);

// void printDeliveryMap(TQueue* queue);

#endif