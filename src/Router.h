#ifndef ROUTER_H
#define ROUTER_H

#include "Request.h"

void RouterInit(void);
int RouterRoute(const Request* Req);

#endif
