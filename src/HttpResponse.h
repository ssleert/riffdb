#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <stdint.h>

enum {
  HttpResponseBufferCapacity = 4096
};

typedef struct {
  char* Buf;
  uint32_t Len;
  uint32_t Cap;
} HttpResponse;

void HttpResponseInit(HttpResponse* Self);
void HttpResponseZero(HttpResponse* Self);
void HttpResponseStatusCode(HttpResponse* Self, uint16_t Status);
void HttpResponseHeader(HttpResponse* Self, const char Key[], const char Value[]);
void HttpResponseBody(HttpResponse* Self, uint32_t Len, const char Body[Len]);

void HttpResponseFree(HttpResponse* Self);

#endif
