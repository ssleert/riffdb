#include "HttpResponse.h"
#include "XMalloc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HTTP_PROTOCOL_STR "HTTP/1.1 "

static inline void
HttpResponseAppend(HttpResponse* Self,
                   uint32_t BufferSize,
                   const char Buffer[BufferSize])
{
  if (Self->Len + BufferSize > Self->Cap) {
    Self->Cap = Self->Cap * 2;
    Self->Buf = XRealloc(Self->Buf, Self->Cap);
  }

  memcpy(&Self->Buf[Self->Len], Buffer, BufferSize);

  Self->Len += BufferSize;
}

void
HttpResponseInit(HttpResponse* Self)
{
  *Self = (HttpResponse){
    .Buf = XMalloc(HttpResponseBufferCapacity),
    .Cap = HttpResponseBufferCapacity,
  };
}

void
HttpResponseZero(HttpResponse* Self)
{
  Self->Len = 0;
}

void
HttpResponseStatusCode(HttpResponse* Self, uint16_t Status)
{
  char StatusStr[6] = { 0 };
  uint8_t N = sprintf(StatusStr, "%d", Status);

  HttpResponseAppend(Self, sizeof(HTTP_PROTOCOL_STR) - 1, HTTP_PROTOCOL_STR);
  HttpResponseAppend(Self, N, StatusStr);
  HttpResponseAppend(Self, 2, "\r\n");
}

void
HttpResponseHeader(HttpResponse* Self, const char Key[], const char Value[])
{
  HttpResponseAppend(Self, strlen(Key), Key);
  HttpResponseAppend(Self, 2, ": ");
  HttpResponseAppend(Self, strlen(Value), Value);
  HttpResponseAppend(Self, 2, "\r\n");
}

void
HttpResponseBody(HttpResponse* Self, uint32_t Len, const char Body[Len])
{
  char LenStr[11] = { 0 };
  sprintf(LenStr, "%d", Len);

  HttpResponseHeader(Self, "Content-Length", LenStr);
  HttpResponseAppend(Self, 2, "\r\n");
  HttpResponseAppend(Self, Len, Body);
}

void
HttpResponseFree(HttpResponse* Self)
{
  XFree(Self->Buf);
}
