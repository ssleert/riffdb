#include "Channel.h"
#include <stdlib.h>

Channel*
ChannelNew(void)
{
  Channel* Self = (Channel*)malloc(sizeof(Channel));
  if (Self == NULL) {
    return NULL;
  }

  Self->Q = (Queue){ 0 };
  if (mtx_init(&Self->Mutex, mtx_plain) != thrd_success) {
    free(Self);
    return NULL;
  }

  if (cnd_init(&Self->Cond) != thrd_success) {
    mtx_destroy(&Self->Mutex);
    free(Self);
    return NULL;
  }

  return Self;
}

void
ChannelFree(Channel* Self)
{
  if (Self == NULL) {
    return;
  }

  mtx_lock(&Self->Mutex);

  while (!QueueIsEmpty(&Self->Q)) {
    QueueNode* Temp = Self->Q.Front;
    Self->Q.Front = Self->Q.Front->Next;
    free(Temp);
  }

  mtx_unlock(&Self->Mutex);

  cnd_destroy(&Self->Cond);
  mtx_destroy(&Self->Mutex);
  free(Self);
}

void
ChannelSend(Channel* Self, void* Data)
{
  mtx_lock(&Self->Mutex);
  Enqueue(&Self->Q, Data);
  cnd_signal(&Self->Cond);
  mtx_unlock(&Self->Mutex);
}

void*
ChannelRecv(Channel* Self)
{
  mtx_lock(&Self->Mutex);

  while (QueueIsEmpty(&Self->Q)) {
    cnd_wait(&Self->Cond, &Self->Mutex);
  }

  void* Data = Dequeue(&Self->Q);
  mtx_unlock(&Self->Mutex);
  return Data;
}

void*
ChannelTryRecv(Channel* Self)
{
  mtx_lock(&Self->Mutex);

  if (QueueIsEmpty(&Self->Q)) {
    mtx_unlock(&Self->Mutex);
    return NULL;
  }

  void* Data = Dequeue(&Self->Q);
  mtx_unlock(&Self->Mutex);
  return Data;
}
