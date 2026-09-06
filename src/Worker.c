#include "Worker.h"
#include "Channel.h"
#include "Log.h"
#include "Request.h"
#include "Router.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int32_t
WorkerHandler(ThreadPoolWorker* Self)
{
  while (Self->Pool->Working) {
    const Request* Req = ChannelRecv(Self->MailBox);
    if (Req->Cancel) {
      continue;
    }
    
    RouterRoute(Req);
  }

  free(Self);
  return 0;
}
