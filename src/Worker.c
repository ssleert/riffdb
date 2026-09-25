#include "Worker.h"
#include "Channel.h"
#include "DataBase.h"
#include "HttpResponse.h"
#include "Log.h"
#include "Options.h"
#include "Request.h"
#include "Router.h"
#include "ThreadPool.h"
#include "XMalloc.h"
#include "sqlite3.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

int32_t
WorkerHandler(ThreadPoolWorker* Self)
{
  sqlite3* Db = DataBaseOpen(GOptions.Directory, false);
  if (Db == NULL) {
    Self->Pool->Working = false;
    return 0;
  }

  while (Self->Pool->Working) {
    Request* Req = ChannelRecv(Self->MailBox);
    if (Req->Cancel) {
      LogWarn("Request Canceled");
      continue;
    }

    Req->Worker.Db = Db;

    // Full parser dump
    LogTrace("=== HttpParser dump ===");
    LogTrace("  State          = %u", Req->State.Parser.State);
    LogTrace("  SawCr          = %d", Req->State.Parser.SawCr);
    LogTrace("  SawDoubleDot   = %d", Req->State.Parser.SawDoubleDot);
    LogTrace("  Method         = %.*s (len=%u)",
             Req->State.Parser.MethodLen,
             Req->State.Parser.Method,
             Req->State.Parser.MethodLen);
    LogTrace("  Url            = %.*s (len=%u)",
             Req->State.Parser.UrlLen,
             Req->State.Parser.Url,
             Req->State.Parser.UrlLen);
    LogTrace("  HeadersLen     = %u", Req->State.Parser.HeadersLen);
    for (uint8_t i = 0; i < Req->State.Parser.HeadersLen; i++) {
      HttpHeader* H = &Req->State.Parser.Headers[i];
      LogTrace("  Header[%u]      = %.*s: %.*s",
               i,
               H->KeyLen,
               H->Key,
               H->ValueLen,
               H->Value);
    }
    LogTrace("  BodyStart      = %u", Req->State.Parser.BodyStart);
    LogTrace("  BodyCap        = %u", Req->State.Parser.BodyCap);
    LogTrace("  ConsumedBody   = %u", Req->State.Parser.ConsumedBody);
    LogTrace("  ContentLength  = %u", Req->State.Parser.ContentLength);
    if (Req->State.Parser.Body != NULL && Req->State.Parser.ContentLength > 0) {
      LogTrace("  Body           = %.*s",
               Req->State.Parser.ContentLength,
               Req->State.Parser.Body);
    } else {
      LogTrace("  Body           = (null or empty)");
    }
    LogTrace("=== end HttpParser dump ===");

    RouterRoute(Req);

    LogTrace("=== HttpResponse dump ===");
    LogTrace("\n%.*s", Req->State.Response.Len, Req->State.Response.Buf);
    LogTrace("=== end HttpResponse dump ===");

    int Rc = send(Req->ClientFd,
                  Req->State.Response.Buf,
                  Req->State.Response.Len,
                  MSG_NOSIGNAL);
    if (Rc < 0) {
      LogWarn("Cant send data to client: Rc = %d, errno = %d", Rc, errno);
    }
    HttpResponseZero(&Req->State.Response);
  }

  sqlite3_close_v2(Db);
  XFree(Self);
  return 0;
}
