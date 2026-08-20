#include "Queue.h"

#include <stdlib.h>

QueueNode*
QueueNodeNew(void* Data)
{
  QueueNode* NewNode = (QueueNode*)malloc(sizeof(QueueNode));
  if (!NewNode) {
    return NULL;
  }
  NewNode->Data = Data;
  NewNode->Next = NULL;
  return NewNode;
}

int
QueueIsEmpty(Queue* Self)
{
  return Self->Front == NULL;
}

void
Enqueue(Queue* Self, void* Data)
{
  QueueNode* NewNode = QueueNodeNew(Data);
  if (Self->Rear == NULL) {
    Self->Front = Self->Rear = NewNode;
    return;
  }
  Self->Rear->Next = NewNode;
  Self->Rear = NewNode;
}

void*
Dequeue(Queue* Self)
{
  if (QueueIsEmpty(Self)) {
    return NULL;
  }
  QueueNode* Temp = Self->Front;
  void* Data = Temp->Data;
  Self->Front = Self->Front->Next;
  if (Self->Front == NULL)
    Self->Rear = NULL;
  free(Temp);
  return Data;
}
