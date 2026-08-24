#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

enum {
  QueueCapacity = 1024,
};

typedef struct {
    void* Items[QueueCapacity];
    uint16_t Head;
    uint16_t Tail;
    uint16_t Count;
} Queue;

int QueueIsEmpty(Queue* Self);
void* QueuePeek(Queue* Self);
void Enqueue(Queue* Self, void* Data);
void* Dequeue(Queue* Self);
uint16_t QueueCount(Queue* Self);

#endif
