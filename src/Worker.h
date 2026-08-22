#ifndef WORKER_H
#define WORKER_H

#include "HttpParser.h"
#include "ThreadPool.h"
#include <stdint.h>

typedef struct {
  struct {
    HttpParser Parser; 
  } State;

  char* Buffer;
  char* BufferLen;
  int32_t ClientFd;
} Request;

int32_t WorkerHandler(ThreadPoolWorker* Self);

#endif
