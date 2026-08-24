#include "Channel.h"
#include <stdlib.h>

int8_t
ChannelInit(Channel* Self)
{
  Self->Q = (Queue){ 0 };
  if (mtx_init(&Self->Mutex, mtx_plain) != thrd_success) {
    free(Self);
    return -1;
  }

  if (cnd_init(&Self->Cond) != thrd_success) {
    mtx_destroy(&Self->Mutex);
    free(Self);
    return -1;
  }

  return 0;
}

void
ChannelDestroy(Channel* Self)
{
  mtx_lock(&Self->Mutex);

  while (!QueueIsEmpty(&Self->Q)) {
    free(Dequeue(&Self->Q));
  }

  mtx_unlock(&Self->Mutex);

  cnd_destroy(&Self->Cond);
  mtx_destroy(&Self->Mutex);
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
