#include "TcpServer.h"
#include "XMalloc.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

static int32_t
SetNonBlocking(int32_t Fd)
{
  int32_t Flags = fcntl(Fd, F_GETFL, 0);
  if (Flags < 0) {
    return -1;
  }
  if (fcntl(Fd, F_SETFL, Flags | O_NONBLOCK) < 0) {
    return -1;
  }
  return 0;
}

static void
RemoveClient(TcpServer* Server, uint16_t Index)
{
  int32_t Fd = Server->PollFds[Index + 1].fd;
  void* ClientData = Server->ClientsData[Index];

  Server->OnDisconnect(Server, Fd, ClientData);

  close(Fd);

  uint16_t Last = Server->ClientCount - 1;
  if (Index != Last) {
    Server->ClientsData[Index] = Server->ClientsData[Last];
    Server->PollFds[Index + 1] = Server->PollFds[Last + 1];
  }

  Server->ClientCount--;
}

int32_t
TcpServerCreate(TcpServer* Server, uint16_t Port, uint16_t MaxClients)
{
  if (!Server || MaxClients == 0) {
    return -1;
  }

  bool isError = false;
  int32_t ListenFd = -1;
  int32_t Opt = 1;

  {
    memset(Server, 0, sizeof(*Server));
    Server->MaxClients = MaxClients;
    Server->ListenFd = -1;

    Server->PollFds = XCalloc((size_t)MaxClients + 1, sizeof(struct pollfd));
    Server->ClientsData = XCalloc(MaxClients, sizeof(Server->ClientsData[0]));

    ListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (ListenFd < 0) {
      isError = true;
      goto error;
    }

    setsockopt(ListenFd, SOL_SOCKET, SO_REUSEADDR, &Opt, sizeof(Opt));
    setsockopt(ListenFd, SOL_SOCKET, 15, &Opt, sizeof(Opt));

    if (SetNonBlocking(ListenFd) < 0) {
      isError = true;
      goto error;
    }

    struct sockaddr_in Addr = {
      .sin_family = AF_INET,
      .sin_addr = {
        .s_addr = htonl(INADDR_ANY),
      },
      .sin_port = htons(Port),
    };

    if (bind(ListenFd, (struct sockaddr*)&Addr, sizeof(Addr)) < 0) {
      isError = true;
      goto error;
    }

    if (listen(ListenFd, 128) < 0) {
      isError = true;
      goto error;
    }
  }
error:
  if (isError) {
    if (ListenFd > 0) {
      close(ListenFd);
    }
    XFree(Server->PollFds);
    XFree(Server->ClientsData);

    return -1;
  }

  Server->ListenFd = ListenFd;
  Server->PollFds[0].fd = ListenFd;
  Server->PollFds[0].events = POLLIN;
  Server->Running = false;

  return 0;
}

void
TcpServerDestroy(TcpServer* Server)
{
  if (!Server) {
    return;
  }

  TcpServerStop(Server);

  for (uint16_t i = 0; i < Server->ClientCount; i++) {
    close(Server->PollFds[i + 1].fd);
  }

  if (Server->ListenFd >= 0) {
    close(Server->ListenFd);
    Server->ListenFd = -1;
  }

  XFree(Server->PollFds);
  XFree(Server->ClientsData);
  Server->PollFds = NULL;
  Server->ClientsData = NULL;
  Server->ClientCount = 0;
}

void
TcpServerSetCallbacks(TcpServer* Server,
                      TcpServerOnConnect OnConnect,
                      TcpServerOnReadable OnReadable,
                      TcpServerOnDisconnect OnDisconnect,
                      void* UserData)
{
  if (!Server) {
    return;
  }
  Server->OnConnect = OnConnect;
  Server->OnReadable = OnReadable;
  Server->OnDisconnect = OnDisconnect;
  Server->UserData = UserData;
}

void
TcpServerStop(TcpServer* Server)
{
  if (Server) {
    Server->Running = false;
  }
}

int32_t
TcpServerRun(TcpServer* Server)
{
  if (!Server || Server->ListenFd < 0) {
    return -1;
  }

  Server->Running = true;

  while (Server->Running) {
    int32_t Nfds = Server->ClientCount + 1;
    int32_t Ready = poll(Server->PollFds, (nfds_t)Nfds, -1);

    if (Ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }

    if (Ready == 0) {
      continue;
    }

    if (Server->PollFds[0].revents & (POLLIN | POLLERR | POLLHUP)) {
      for (;;) {
        struct sockaddr_in ClientAddr;
        socklen_t AddrLen = sizeof(ClientAddr);
        int32_t ClientFd =
          accept(Server->ListenFd, (struct sockaddr*)&ClientAddr, &AddrLen);

        if (ClientFd < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }
          break;
        }

        if (Server->ClientCount >= Server->MaxClients) {
          close(ClientFd);
          continue;
        }

        if (SetNonBlocking(ClientFd) < 0) {
          close(ClientFd);
          continue;
        }

        uint16_t Idx = Server->ClientCount;

        Server->ClientsData[Idx] = NULL;
        Server->PollFds[Idx + 1].fd = ClientFd;
        Server->PollFds[Idx + 1].events = POLLIN;
        Server->PollFds[Idx + 1].revents = 0;

        Server->ClientCount++;

        Server->OnConnect(Server, ClientFd, &Server->ClientsData[Idx]);
      }
    }

    for (int16_t i = (int16_t)Server->ClientCount - 1; i >= 0; i--) {
      short Rev = Server->PollFds[i + 1].revents;
      if (Rev == 0) {
        continue;
      }

      int32_t Fd = Server->PollFds[i + 1].fd;
      void* ClientData = Server->ClientsData[i];

      if (Rev & (POLLERR | POLLHUP | POLLNVAL)) {
        RemoveClient(Server, i);
        continue;
      }

      if (Rev & POLLIN) {
        for (;;) {
          int16_t rc = Server->OnReadable(Server, Fd, ClientData);
          if (rc == TcpServerErrorEmptyRead) {
            RemoveClient(Server, i);
            break;
          } else if (rc == TcpServerErrorRead) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            }
            RemoveClient(Server, i);
            break;
          }
        }
      }
    }
  }

  return 0;
}
