#include "Router.h"

#include "HttpResponse.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// k&r style shit...
static uint32_t
Hash(const char* str)
{
  uint32_t hash = 0;
  char c;

  do {
    c = *str++;
    hash += c;
  } while (c);

  return hash;
}

uint32_t ExecuteRoute = 0;
uint32_t HealthRoute = 0;

void
RouterInit(void)
{
  ExecuteRoute = Hash("/execute");
  HealthRoute = Hash("/health");
}

void
RouterRoute(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;

  uint32_t Route = Hash(Req->State.Parser.Url);

  if (Route == ExecuteRoute) {
    const char body[] = "execute";
    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, sizeof(body)-1, body);
  } else if (Route == HealthRoute) {
    const char body[] = "health";
    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, sizeof(body)-1, body);
  } else { 
    const char body[] = "not found";
    HttpResponseStatusCode(Res, 404);
    HttpResponseBody(Res, sizeof(body)-1, body);
  }
}
