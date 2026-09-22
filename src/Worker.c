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

    RouterRoute(Req);

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
