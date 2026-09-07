#include "Worker.h"
#include "Channel.h"
#include "HttpResponse.h"
#include "Log.h"
#include "Request.h"
#include "Router.h"

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

int32_t
WorkerHandler(ThreadPoolWorker* Self)
{
  while (Self->Pool->Working) {
    Request* Req = ChannelRecv(Self->MailBox);
    if (Req->Cancel) {
      LogWarn("Request Canceled");
      continue;
    }

    RouterRoute(Req);

    write(Req->ClientFd, Req->State.Response.Buf, Req->State.Response.Len);
    HttpResponseZero(&Req->State.Response);
  }

  free(Self);
  return 0;
}
