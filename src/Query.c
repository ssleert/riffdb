#include "Query.h"
#include "HttpResponse.h"
#include "HttpUtils.h"
#include "Log.h"
#include "Protocol.h"
#include "Request.h"
#include "Service.h"
#include "XMalloc.h"
#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>
#include <yyjson.h>

void
Query(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;

  ServiceState State = {
    .Db = Req->Worker.Db,
    .Payload = Req->State.Parser.Body,
    .PayloadLen = Req->State.Parser.ContentLength,
  };

  ServiceError Rc = ServiceQuery(&State);
  if (Rc != ServiceOK) {
    HttpUtilsResError(Res, State.Status, State.Res);

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
