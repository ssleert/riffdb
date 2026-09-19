#include "Execute.h"
#include <yyjson.h>

#include "DataBase.h"

void
Execute(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;  

  {
    const char body[] = "execute";
    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, sizeof(body) - 1, body);
  }
}
