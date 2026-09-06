#include "HttpParser.h"
#include "Log.h"
#include "Options.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "Worker.h"

#include <string.h>
#include <unistd.h>

#define PROGRAM_NAME "riffdb"
#define PROGRAM_VERSION "0.0.1"

static ThreadPool GPool = { 0 };

static void
OnConnect(TcpServer* Server, int32_t ClientFd, void** ClientData)
{
  LogTrace("Client connected: fd=%d", ClientFd);

  *ClientData = malloc(sizeof(Request));
  if (*ClientData == NULL) {
    LogFatal("allocation failure");
  }

  *((Request*)(*ClientData)) = (Request){
    .ClientFd = ClientFd,
  };

  HttpParserInit(&((Request*)*ClientData)->State.Parser);
}

static int16_t
OnReadable(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  Request* Req = ClientData;

  char Buffer[8192];
  ssize_t N = read(ClientFd, Buffer, sizeof(Buffer));

  if (N == 0) {
    return TcpServerErrorEmptyRead;
  } else if (N < 0) {
    return TcpServerErrorRead;
  }

  HttpParserError rc =
    HttpParserParse(&Req->State.Parser, N, Buffer);
  if (rc < 0) {
    LogFatal("body parsing fucked up");
  }

  if (Req->State.Parser.State != HttpParserStateBody &&
      Req->State.Parser.State != HttpParserStateComplete) {
    return 0;
  }

  rc = HttpParserParseBody(&Req->State.Parser, N, Buffer);
  if (rc < 0) {
    free(Req);
    LogFatal("body parsing fucked up");
  }

  ThreadPoolProcess(&GPool, Req);

  return 0;
}

static void
OnDisconnect(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  LogTrace("Client disconnected: fd=%d", ClientFd);

  Request* Req = ClientData;
  HttpParserFree(&Req->State.Parser);

  Req->Cancel = true;
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

  TcpServer Server = { 0 };

  if (TcpServerCreate(&Server, GOptions.Port, 4096) != 0) {
    perror("Failed to create server");
    return 1;
  }

  if (ThreadPoolStart(
        &GPool, GOptions.Threads, (int (*)(void*))WorkerHandler) != 0) {
    LogErr("Failed to start thread pool");
    return 1;
  }

  TcpServerSetCallbacks(&Server, OnConnect, OnReadable, OnDisconnect, NULL);

  LogInfo("Listening on port %d...", GOptions.Port);
  TcpServerRun(&Server);

  ThreadPoolStop(&GPool);

  TcpServerDestroy(&Server);

  return EXIT_SUCCESS;
}
