#ifndef WORKER_H
#define WORKER_H

#include "HttpParser.h"
#include "ThreadPool.h"
#include <stdint.h>

typedef struct {
  struct {
    HttpParser Parser; 
  } State;

  int32_t ClientFd;
  _Atomic(bool) Cancel;
} Request;

int32_t WorkerHandler(ThreadPoolWorker* Self);

#endif
