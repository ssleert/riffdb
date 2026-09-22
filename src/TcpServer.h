#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum TcpServerError {
  TcpServerErrorEmptyRead = -1,
  TcpServerErrorRead = -2,
} TcpServerError;

typedef struct TcpServer TcpServer;

typedef void (*TcpServerOnConnect)(TcpServer *Server, int32_t ClientFd, void** ClientData);
typedef int16_t (*TcpServerOnReadable)(TcpServer *Server, int32_t ClientFd, void* ClientData);
typedef void (*TcpServerOnDisconnect)(TcpServer *Server, int32_t ClientFd, void* ClientData);

typedef struct TcpClient {
  int32_t Fd;
} TcpClient;

typedef struct TcpServer {
  int32_t                 ListenFd;
  struct pollfd*          PollFds;
  void**                  ClientsData;
  uint16_t                MaxClients;
  uint16_t                ClientCount;
  _Atomic(bool)           Running;
  TcpServerOnConnect      OnConnect;
  TcpServerOnReadable     OnReadable;
  TcpServerOnDisconnect   OnDisconnect;
  void*                   UserData;
} TcpServer;

int32_t TcpServerCreate(TcpServer *Server, uint16_t Port, uint16_t MaxClients);
void    TcpServerDestroy(TcpServer *Server);
void    TcpServerSetCallbacks(TcpServer *Server,
                              TcpServerOnConnect OnConnect,
                              TcpServerOnReadable OnReadable,
                              TcpServerOnDisconnect OnDisconnect,
                              void *UserData);
int32_t TcpServerRun(TcpServer *Server);
void    TcpServerStop(TcpServer *Server);

#endif /* TCPSERVER_H */
