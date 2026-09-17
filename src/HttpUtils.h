#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#include "HttpResponse.h"

void HttpUtilsResError(HttpResponse* Res, uint16_t Status, const char Err[]);

#endif
