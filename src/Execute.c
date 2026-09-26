#include "Execute.h"
#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>
#include <yyjson.h>

#include "HttpResponse.h"
#include "HttpUtils.h"
#include "Log.h"
#include "Protocol.h"
#include "Request.h"
#include "Service.h"
#include "XMalloc.h"

void
Execute(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;

  ServiceState State = {
    .Db = Req->Worker.Db,
    .Payload = Req->State.Parser.Body,
    .PayloadLen = Req->State.Parser.ContentLength,
  };

  ServiceError Rc = ServiceExecute(&State);
  if (Rc != ServiceOK) {
    HttpResponseStatusCode(Res, State.Status);
    HttpResponseBody(Res, State.ResSize, State.Res);

    if (Rc == ServiceErrorSqlite) {
      XFree((void*)State.Res);
    }
  }

  if (Req->Cancel) {
    return;
  }

  HttpResponseStatusCode(Res, State.Status);
  HttpResponseBody(Res, State.ResSize, State.Res);
}
