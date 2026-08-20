#ifndef QUEUE_H
#define QUEUE_H

typedef struct QueueNode {
    void* Data;
    struct QueueNode* Next;
} QueueNode;

typedef struct {
    QueueNode* Front;
    QueueNode* Rear;
} Queue;

QueueNode* QueueNodeNew(void* Data);

int QueueIsEmpty(Queue* Queue);
void Enqueue(Queue* Queue, void* Data);
void* Dequeue(Queue* Queue);

#endif
