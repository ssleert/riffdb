#include "Queue.h"

#include <stdint.h>
#include <stdlib.h>

int
QueueIsEmpty(Queue* Self)
{
  return Self->Count == 0;
}

void*
QueuePeek(Queue* Self)
{
  if (QueueIsEmpty(Self)) {
    return NULL;
  }
  return Self->Items[Self->Head];
}

void
Enqueue(Queue* Self, void* Data)
{
  if (Self->Count == QueueCapacity) {
    return;
  }
  Self->Items[Self->Tail] = Data;
  Self->Tail = (Self->Tail + 1) % QueueCapacity;
  Self->Count++;
}

void*
Dequeue(Queue* Self)
{
  if (QueueIsEmpty(Self)) {
    return NULL;
  }
  void* Data = Self->Items[Self->Head];
  Self->Items[Self->Head] = NULL;
  Self->Head = (Self->Head + 1) % QueueCapacity;
  Self->Count--;
  return Data;
}

uint16_t
QueueCount(Queue* Self)
{
  return Self->Count;
}
