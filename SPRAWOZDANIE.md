---
title: Publish-subscribe
subtitle: Programowanie systemowe i współbieżne
author: Yanukovich Illia 159788 <illia.yanukovich@student.put.poznan.pl>
date: v1.0, 2024-12-28
lang: pl-PL
---

Projekt jest dostępny w repozytorium pod adresem:  
<https://github.com/YIlyaA/Publish-Subscribe>.


# Struktury danych

1. Struktura `SubscriberNode`:

    Reprezentuje pojedynczego subskrybenta w systemie. Wykorzystuje ona listę jednokierunkową, aby umożliwić dynamiczne zarządzanie subskrybentami.
    ```C
    typedef struct SubscriberNode {
        pthread_t thread_id;
        struct SubscriberNode* next;
    } SubscriberNode;
   ```

  - `thread_id`: Przechowuje identyfikator wątku subskrybenta (`pthread_t`)
  - `next`: Wskaźnik na kolejnego subskrybenta w liście.


2. Struktura `TQueue`:

    Reprezentuje kolejkę wiadomości w systemie oraz zarządza listą subskrybentów i mechanizmami synchronizacji.
    ```C
    typedef struct TQueue {
        void** messages;
        int size;
        int count;
        int head;
        int tail;
        SubscriberNode* subscribers;
        int** delivery_map;
        pthread_mutex_t mutex;
        pthread_cond_t not_full;
        pthread_cond_t not_empty;
    } TQueue;
   ```

  - `messages`: Tablica wskaźników przechowująca wiadomości do dostarczenia subskrybentom.
  - `size`: Maksymalna liczba wiadomości, które mogą znajdować się w kolejce.
  - `count`: Aktualna liczba wiadomości w kolejce.
  - `head`: Indeks wskazujący początek kolejki (bufor cykliczny).
  - `tail`: Indeks wskazujący koniec kolejki (bufor cykliczny).
  - `subscribers`: Wskaźnik na początek listy `SubscriberNode`.
  - `delivery_map`: Tablica dwuwymiarowa przechowująca informacje o dostarczeniu wiadomości. Dla każdego subskrybenta określa, czy otrzymał daną wiadomość.
  - `mutex`: Mutex zapewniający wzajemne wykluczanie w operacjach na kolejce. 
  - `not_full`: Zmienna warunkowa, sygnalizująca, że kolejka nie jest pełna i można dodać wiadomość.
  - `not_empty`: Zmienna warunkowa, sygnalizująca, że kolejka nie jest pusta i można pobrać wiadomość.

# Funkcje

1. `int countSubscribers(TQueue *queue)` -- funkcja zliczająca liczbę subskrybentów w kolejce queue.


# Algorytm / dodatkowy opis

```
  Publisher                                Subscriber
  +-------------+                         +-------------+
  |    Start    |                         |    Start    |
  +-------------+                         +-------------+
        |                                         |
        v                                         v
  +-------------------+                  +-------------------+
  | Czy kolejka jest  |                  | Czy są wiadomości |
  |       pełna?      |                  |  w kolejce?       |
  +-------------------+                  +-------------------+
  NIE |        | TAK                      NIE |        | TAK
      |        v                              |        v
      |   +----------+                   +----------+  |
      |   |  Czekaj  |                   |  Czekaj  |  |
      |   +----------+                   +----------+  |
      |        |                              |        |
      +--------v                              +--------v
  +-------------------+                  +-------------------+
  | Dodaj wiadomość   |                  | Pobierz wiadomość |
  |  do kolejki       |                  |  z kolejki        |
  +-------------------+                  +-------------------+
        |                                         |
        v                                         v
  +-------------------+                  +-------------------+
  | Powiadom          |                  | Oznacz wiadomość  |
  | subskrybentów     |                  | jako odebraną     |
  +-------------------+                  +-------------------+
        |                                         |
        v                                         v
  +-------------+                         +--------------------+
  |    Koniec   |                         | Czy wszyscy        |
  +-------------+                         | odebrali wiadomość?|
                                          +--------------------+
                                            NIE |        | TAK
                                                |        v
                                                |   +-----------------+
                                                |   | Usuń wiadomość  |
                                                |   | z kolejki       |
                                                |   +-----------------+
                                                |        |
                                                +--------v
                                          +-------------+
                                          |    Koniec   |
                                          +-------------+
```


### Odporność na sytuacje skrajne
- `Pusty bufor:` Wątki próbujące pobrać wiadomość z pustej kolejki są wstrzymywane (pthread_cond_wait na not_empty).
- `Pełny bufor`: Wątki publikujące wiadomości są wstrzymywane, gdy kolejka jest pełna (pthread_cond_wait na not_full).
- `Zakleszczenie`: Synchronizacja za pomocą mutexów i zmiennych warunkowych zapobiega zakleszczeniom. Wszystkie wątki odblokowują mutexy po zakończeniu operacji.
- `Głodzenie`: Mapy dostarczania (delivery_map) gwarantują, że każda wiadomość jest dostarczana do wszystkich subskrybentów przed jej usunięciem.


# Przykład użycia

### Dane wejściowe:
- Rozmiar kolejki: `int size = 2;` .
- Publisher: `T1`.
   - Generuje wiadomości w pętli, tworząc kolejne nazwy wiadomości `(m1, m2, ...`
- Subskrybenci: `T2, T3, T4`.

### Dane wyjściowe:
```
Queue created with size 2
T2: Subscribed
T4: Subscribed
T3: Subscribed
T1: Put message 'm1'
T3: Received message: m1
T2: Received message: m1
T4: Received message: m1
T4: Unsubscribed
T1: Put message 'm2'
T2: Received message: m2
T3: Received message: m2
T1: Put message 'm3'
T2: Received message: m3
T3: Received message: m3
T1: Put message 'm4'
T2: Received message: m4
T3: Received message: m4
T1: Put message 'm5'
T2: Unsubscribed
T1: Put message 'm6'
T3: Received message: m6
T1: Put message 'm7'
T3: Unsubscribed
No subscribers. Message 'm8' discarded.
T1: Put message 'm8'
No subscribers. Message 'm9' discarded.
T1: Put message 'm9'
No subscribers. Message 'm10' discarded.
T1: Put message 'm10'
All threads finished. Destroying queue.
```