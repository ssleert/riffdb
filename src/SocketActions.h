#ifndef SOCKETACTIONS_H
#define SOCKETACTIONS_H

#include "TcpServer.h"

void
SocketActionsOnConnect(TcpServer* Server, int32_t ClientFd, void** ClientData);

int16_t
SocketActionsOnReadable(TcpServer* Server, int32_t ClientFd, void* ClientData);

void
SocketActionsOnDisconnect(TcpServer* Server, int32_t ClientFd, void* ClientData);

#endif
