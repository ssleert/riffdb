#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "Channel.h"

#include <threads.h>
#include <stdbool.h>

typedef struct {
  Channel* MailBoxes; 
  thrd_t*  Workers;

  uint8_t  MailBoxesSize;
  uint8_t  WorkersSize;

  uint8_t  CurrentWorker;

  _Atomic(bool) Working;
} ThreadPool;

typedef struct {
  ThreadPool* Pool;
  Channel* MailBox;
} ThreadPoolWorker;

int8_t ThreadPoolStart(ThreadPool* Self, uint8_t WorkersAmount, int (*Worker)(void*));

void ThreadPoolProcess(ThreadPool* Self, void* Message);

void ThreadPoolStop(ThreadPool* Self);

#endif
