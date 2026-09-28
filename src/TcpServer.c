#include "TcpServer.h"
#include "XMalloc.h"

#include "Log.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

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
  int32_t Fd = Server->PollFds[Index + 2].fd;
  void* ClientData = Server->ClientsData[Index];

  Server->OnDisconnect(Server, Fd, ClientData);

  close(Fd);

  uint16_t Last = Server->ClientCount - 1;
  if (Index != Last) {
    Server->ClientsData[Index] = Server->ClientsData[Last];
    Server->PollFds[Index + 2] = Server->PollFds[Last + 2];
  }

  Server->ClientCount--;
}

int32_t
TcpServerCreate(TcpServer* Server, uint16_t Port, uint16_t MaxClients)
{
  if (!Server || MaxClients == 0) {
    return -1;
  }

  int32_t ListenFd = -1;
  int32_t Opt = 1;

  {
    *Server = (TcpServer){ 0 };
    Server->MaxClients = MaxClients;
    Server->ListenFd = -1;
    Server->SelfPipe[0] = -1;
    Server->SelfPipe[1] = -1;

    Server->PollFds = XCalloc((size_t)MaxClients + 2, sizeof(struct pollfd));
    Server->ClientsData =
      (void**)XCalloc(MaxClients, sizeof(Server->ClientsData[0]));

    if (pipe(Server->SelfPipe) < 0) {
      goto error;
    }
    if (SetNonBlocking(Server->SelfPipe[0]) < 0 ||
        SetNonBlocking(Server->SelfPipe[1]) < 0) {
      goto error;
    }

    ListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (ListenFd < 0) {
      goto error;
    }

    setsockopt(ListenFd, SOL_SOCKET, SO_REUSEADDR, &Opt, sizeof(Opt));
    setsockopt(ListenFd, SOL_SOCKET, 15, &Opt, sizeof(Opt));

    if (SetNonBlocking(ListenFd) < 0) {
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
      goto error;
    }

    if (listen(ListenFd, 128) < 0) {
      goto error;
    }
  }

  Server->ListenFd = ListenFd;
  Server->PollFds[0].fd = ListenFd;
  Server->PollFds[0].events = POLLIN;
  Server->PollFds[1].fd = Server->SelfPipe[0];
  Server->PollFds[1].events = POLLIN;
  Server->Running = false;

  return 0;

error:
  if (ListenFd > 0) {
    close(ListenFd);
  }
  if (Server->SelfPipe[0] >= 0) {
    close(Server->SelfPipe[0]);
  }
  if (Server->SelfPipe[1] >= 0) {
    close(Server->SelfPipe[1]);
  }
  XFree(Server->PollFds);
  XFree((void*)Server->ClientsData);

  return -1;
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

  if (Server->SelfPipe[0] >= 0) {
    close(Server->SelfPipe[0]);
  }
  if (Server->SelfPipe[1] >= 0) {
    close(Server->SelfPipe[1]);
  }

  XFree(Server->PollFds);
  XFree((void*)Server->ClientsData);
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

  if (Server->SelfPipe[1] >= 0) {
    char byte = 0;
    write(Server->SelfPipe[1], &byte, 1);
  }
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
int32_t
TcpServerRun(TcpServer* Server)
{
  if (!Server || Server->ListenFd < 0) {
    return -1;
  }

  Server->Running = true;

  char Buf[64] = { 0 };
  do {
    int32_t Nfds = Server->ClientCount + 2;
    int32_t Ready = poll(Server->PollFds, (nfds_t)Nfds, -1);

    if (Ready < 0) {
      if (errno == EINTR) {
        LogWarn("interrupt");
        continue;
      }
      return -1;
    }

    if (Ready == 0) {
      continue;
    }

    if (Server->PollFds[1].revents & (POLLIN | POLLERR | POLLHUP)) {
      for (;;) {
        ssize_t n = read(Server->SelfPipe[0], &Buf, sizeof(Buf));
        if (n < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }
          break;
        }
        if (n == 0) {
          break;
        }
      }
      break;
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

        if (Server->ClientCount >= Server->MaxClients-2) {
          close(ClientFd);
          continue;
        }

        if (SetNonBlocking(ClientFd) < 0) {
          close(ClientFd);
          continue;
        }

        uint16_t Idx = Server->ClientCount;

        Server->ClientsData[Idx] = NULL;
        Server->PollFds[Idx + 2].fd = ClientFd;
        Server->PollFds[Idx + 2].events = POLLIN;
        Server->PollFds[Idx + 2].revents = 0;

        Server->ClientCount++;

        Server->OnConnect(Server, ClientFd, &Server->ClientsData[Idx]);
      }
    }

    for (int16_t i = (int16_t)Server->ClientCount - 1; i >= 0; i--) {
      short Rev = Server->PollFds[i + 2].revents;
      if (Rev == 0) {
        continue;
      }

      int32_t Fd = Server->PollFds[i + 2].fd;
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
          }
          if (rc == TcpServerErrorRead) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            }
            RemoveClient(Server, i);
            break;
          }
        }
      }
    }
  } while (Server->Running);

  return 0;
}
// NOLINTEND(readability-function-cognitive-complexity)
