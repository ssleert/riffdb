#ifndef CHANNEL_H
#define CHANNEL_H

#include "Queue.h"
#include <stdint.h>
#include <threads.h>

typedef struct {
  Queue Q;
  mtx_t Mutex;
  cnd_t Cond;
} Channel;

Channel* ChannelNew(void);
void ChannelFree(Channel* Self);

void ChannelSend(Channel* Self, void* Data);
void* ChannelRecv(Channel* Self);

void* ChannelTryRecv(Channel* Self);

#endif
