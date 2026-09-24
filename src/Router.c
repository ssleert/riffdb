#include "Router.h"
#include "Execute.h"
#include "HttpResponse.h"
#include "Query.h"
#include "Request.h"

#include <stdint.h>

// k&r style shit...
static uint32_t
Hash(const char* str)
{
  uint32_t hash = 0;
  char c = 0;

  do {
    c = *str++;
    hash += c;
  } while (c);

  return hash;
}

static uint32_t ExecuteRoute = 0;
static uint32_t QueryRoute = 0;
static uint32_t HealthRoute = 0;

void
RouterInit(void)
{
  ExecuteRoute = Hash("/execute");
  QueryRoute = Hash("/query");
  HealthRoute = Hash("/health");
}

void
RouterRoute(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;

  uint32_t Route = Hash(Req->State.Parser.Url);

  if (Route == ExecuteRoute) {
    Execute(Req);
    return;
  }
  if (Route == QueryRoute) {
    Query(Req);
    return;
  }
  if (Route == HealthRoute) {
    const char body[] = "health";
    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, sizeof(body) - 1, body);
    return;
  }

  const char body[] = "not found";
  HttpResponseStatusCode(Res, 404);
  HttpResponseBody(Res, sizeof(body) - 1, body);
}
