#ifndef REQUEST_H
#define REQUEST_H

#include "HttpParser.h"
#include "HttpResponse.h"

#include <stdint.h>

typedef struct {
  struct {
    HttpParser Parser; 
    HttpResponse Response;
  } State;

  int32_t ClientFd;
  _Atomic(bool) Cancel;
} Request;

#endif
