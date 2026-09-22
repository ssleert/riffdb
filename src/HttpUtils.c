#include "HttpUtils.h"
#include "HttpResponse.h"
#include "string.h"
#include <stdint.h>

void
HttpUtilsResError(HttpResponse* Res, uint16_t Status, const char Err[])
{
  HttpResponseStatusCode(Res, Status);
  HttpResponseBody(Res, strlen(Err), Err);
}
