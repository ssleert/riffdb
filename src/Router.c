#include "Router.h"
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

void RouterInit(void) {
  ExecuteRoute = Hash("/execute");
  HealthRoute = Hash("/health");
}

int
RouterRoute(const Request* Req)
{
  uint32_t Route = Hash(Req->State.Parser.Url);

  if (Route == ExecuteRoute) { 
    const char res[] = "HTTP/1.1 200 OK\n\
Date: Sun, 16 Aug 2026 07:25:00 GMT\n\
Server: ExampleServer/1.0\n\
Content-Type: text/html\n\
Content-Length: 59\n\
Connection: keep-alive\n\
\n\
<html>\n\
<body>\n\
<h1>Execute    HTTP/1.0</h1>\n\
</body>\n\
</html>\n";
    size_t resLen = strlen(res);
    // LogInfo(
    //   "response: %zd bytes written to clientfd = %d", resLen, Req->ClientFd);
    write(Req->ClientFd, res, resLen); // echo
  } else if (Route == HealthRoute) {
    const char res[] = "HTTP/1.1 200 OK\n\
Date: Sun, 16 Aug 2026 07:25:00 GMT\n\
Server: ExampleServer/1.0\n\
Content-Type: text/html\n\
Content-Length: 59\n\
Connection: keep-alive\n\
\n\
<html>\n\
<body>\n\
<h1>Health     HTTP/1.0</h1>\n\
</body>\n\
</html>\n";
    size_t resLen = strlen(res);
    // LogInfo(
    //   "response: %zd bytes written to clientfd = %d", resLen, Req->ClientFd);
    write(Req->ClientFd, res, resLen); // echo
  }

    //         Self->Pool->CurrentWorker,
    //         Req->ClientFd,
    //         Req->BufferLen);
    // LogInfo("method = '%.*s', url = '%.*s'",
    //         Req->State.Parser.MethodLen,
    //         Req->State.Parser.Method,
    //         Req->State.Parser.UrlLen,
    //         Req->State.Parser.Url);
    // LogInfo("headers = %d", Req->State.Parser.HeadersLen);
    // for (uint32_t h = 0; h < Req->State.Parser.HeadersLen; ++h) {
    //   LogInfo("  header[%d] = '%.*s': '%.*s'",
    //           h,
    //           Req->State.Parser.Headers[h].KeyLen,
    //           Req->State.Parser.Headers[h].Key,
    //           Req->State.Parser.Headers[h].ValueLen,
    //           Req->State.Parser.Headers[h].Value);
    // }


  return 0;
}
