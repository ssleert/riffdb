#include "HttpUtils.h"
#include "string.h"

void HttpUtilsResError(HttpResponse* Res, uint16_t Status, const char Err[]) {
  HttpResponseStatusCode(Res, Status);
  HttpResponseBody(Res, strlen(Err), Err);
}
