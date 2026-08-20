#include "Log.h"
#include "Options.h"
#include "TcpServer.h"

#include <string.h>

#include <aio.h>
#include <unistd.h>

#define PROGRAM_NAME "riffdb"
#define PROGRAM_VERSION "0.0.1"

static void
OnConnect(TcpServer* Server, int32_t ClientFd, void** ClientData)
{
  (void)Server;
  printf("Client connected: fd=%d\n", ClientFd);

  *ClientData = calloc(4096, sizeof(uint8_t));
}

static int16_t
OnReadable(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  (void)Server;

  for (;;) {
    ssize_t N = read(ClientFd, ClientData, 4096);

    if (N > 0) {
      const char res[] = "HTTP/1.1 200 OK\n\
Date: Sun, 16 Aug 2026 07:25:00 GMT\n\
Server: ExampleServer/1.0\n\
Content-Type: text/html\n\
Content-Length: 59\n\
Connection: keep-alive\n\
\n\
<html>\n\
<body>\n\
<h1>Hello from HTTP/1.0</h1>\n\
</body>\n\
</html>\n";

      write(ClientFd, res, (size_t)strlen(res)); // echo
    } else if (N == 0) {
      return TcpServerErrorEmptyRead;
    } else {
      return TcpServerErrorRead;
    }
  }
}

static void
OnDisconnect(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  (void)Server;
  printf("Client disconnected: fd=%d\n", ClientFd);

  free(ClientData);
}

int
main(int32_t Argc, char* Argv[])
{
  if (ParseOptions(Argc, Argv) != 0) {
    PrintUsage(PROGRAM_NAME);
    FreeOptions();
    return EXIT_FAILURE;
  }

  if (GOptions.ShowHelp) {
    PrintUsage(PROGRAM_NAME);
    FreeOptions();
    return EXIT_SUCCESS;
  }

  if (GOptions.ShowVersion) {
    PrintVersion(PROGRAM_NAME, PROGRAM_VERSION);
    FreeOptions();
    return EXIT_SUCCESS;
  }

  if (strcmp(GOptions.Directory, ".") != 0) {
    if (chdir(GOptions.Directory) != 0) {
      perror("chdir");
      FreeOptions();
      return EXIT_FAILURE;
    }
  }

  LogInfo(PROGRAM_NAME " " PROGRAM_VERSION);
  LogInfo("Port: %d", GOptions.Port);
  LogInfo("Directory: %s", GOptions.Directory);
  LogInfo("Threads: %d", GOptions.Threads);

  TcpServer Server;

  if (TcpServerCreate(&Server, 8081, 4096) != 0) {
    perror("Failed to create server");
    return 1;
  }

  TcpServerSetCallbacks(&Server, OnConnect, OnReadable, OnDisconnect, NULL);

  printf("Listening on port 8080...\n");
  TcpServerRun(&Server);

  TcpServerDestroy(&Server);

  return EXIT_SUCCESS;
}
