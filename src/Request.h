#ifndef REQUEST_H
#define REQUEST_H

#include "HttpParser.h"

#include <stdint.h>

typedef struct {
  struct {
    HttpParser Parser; 
  } State;

  int32_t ClientFd;
  _Atomic(bool) Cancel;
} Request;

#endif
