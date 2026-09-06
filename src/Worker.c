#include "Worker.h"
#include "Channel.h"
#include "Log.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int32_t
WorkerHandler(ThreadPoolWorker* Self)
{
  while (Self->Pool->Working) {
    Request* Req = ChannelRecv(Self->MailBox);
    if (Req->Cancel) {
      free(Req);
      continue;
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

    const char res[] = "HTTP/1.1 200 OK\n\
Date: Sun, 16 Aug 2026 07:25:00 GMT\n\
Server: ExampleServer/1.0\n\
Content-Type: text/html\n\
Content-Length: 59\n\
Connection: keep-alive\n\
\n\
<html>\n\
<body>\n\
<h1>Hello from HTTP/1.0</h1>\n\
</body>\n\
</html>\n";
    size_t resLen = strlen(res);
    // LogInfo(
    //   "response: %zd bytes written to clientfd = %d", resLen, Req->ClientFd);
    write(Req->ClientFd, res, resLen); // echo
  }

  free(Self);
  return 0;
}
