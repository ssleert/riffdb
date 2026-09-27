#include "SocketActions.h"
#include "HttpParser.h"
#include "HttpResponse.h"
#include "Log.h"
#include "Request.h"
#include "ThreadPool.h"
#include "XMalloc.h"
#include "main.h"

#include <sqlite3.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

void
SocketActionsOnConnect(TcpServer* Server, int32_t ClientFd, void** ClientData)
{
  (void)Server;

  LogTrace("Client connected: fd=%d", ClientFd);

  *ClientData = XMalloc(sizeof(Request));

  *((Request*)(*ClientData)) = (Request){
    .ClientFd = ClientFd,
  };

  HttpParserInit(&((Request*)*ClientData)->State.Parser);
  HttpResponseInit(&((Request*)*ClientData)->State.Response);
}

int16_t
SocketActionsOnReadable(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  Request* Req = ClientData;

  char Buffer[8192];
  ssize_t N = read(ClientFd, Buffer, sizeof(Buffer));

  if (N == 0) {
    return TcpServerErrorEmptyRead;
  }
  if (N < 0) {
    return TcpServerErrorRead;
  }

  HttpParserError rc = HttpParserParse(&Req->State.Parser, N, Buffer);
  if (rc < 0) {
    LogErr("body parsing fucked up %d", rc);
  }

  if (Req->State.Parser.State != HttpParserStateBody &&
      Req->State.Parser.State != HttpParserStateComplete) {
    return 0;
  }

  rc = HttpParserParseBody(&Req->State.Parser, N, Buffer);
  if (rc < 0) {
    XFree(Req);
    LogErr("body parsing fucked up %d", rc);
  }

  ThreadPoolProcess((ThreadPool*)Server->UserData, Req);

  return 0;
}

void
SocketActionsOnDisconnect(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  (void)Server;

  LogTrace("Client disconnected: fd=%d", ClientFd);

  Request* Req = ClientData;
  HttpParserFree(&Req->State.Parser);
  HttpResponseFree(&Req->State.Response);

  Req->Cancel = true;
}
