#include "main.h"
#include "DataBase.h"
#include "Greeting.h"
#include "HttpParser.h"
#include "HttpResponse.h"
#include "Log.h"
#include "Options.h"
#include "Request.h"
#include "Router.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "Worker.h"
#include "XMalloc.h"

#include <sqlite3.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static ThreadPool GPool = { 0 };

static void
OnConnect(TcpServer* Server, int32_t ClientFd, void** ClientData)
{
  (void)Server;

  LogTrace("Client connected: fd=%d", ClientFd);

  *ClientData = XMalloc(sizeof(Request));

  *((Request*)(*ClientData)) = (Request){
    .ClientFd = ClientFd,
  };

  HttpParserInit(&((Request*)*ClientData)->State.Parser);
  HttpResponseInit(&((Request*)*ClientData)->State.Response);
}

static int16_t
OnReadable(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  (void)Server;

  Request* Req = ClientData;

  char Buffer[8192];
  ssize_t N = read(ClientFd, Buffer, sizeof(Buffer));

  if (N == 0) {
    return TcpServerErrorEmptyRead;
  }
  if (N < 0) {
    return TcpServerErrorRead;
  }

  HttpParserError rc = HttpParserParse(&Req->State.Parser, N, Buffer);
  if (rc < 0) {
    LogErr("body parsing fucked up %d", rc);
  }

  if (Req->State.Parser.State != HttpParserStateBody &&
      Req->State.Parser.State != HttpParserStateComplete) {
    return 0;
  }

  rc = HttpParserParseBody(&Req->State.Parser, N, Buffer);
  if (rc < 0) {
    XFree(Req);
    LogErr("body parsing fucked up %d", rc);
  }

  ThreadPoolProcess(&GPool, Req);

  return 0;
}

static void
OnDisconnect(TcpServer* Server, int32_t ClientFd, void* ClientData)
{
  (void)Server;

  LogTrace("Client disconnected: fd=%d", ClientFd);

  Request* Req = ClientData;
  HttpParserFree(&Req->State.Parser);
  HttpResponseFree(&Req->State.Response);

  Req->Cancel = true;
}

int
main(int32_t Argc, char* Argv[])
{
#ifdef NTRACE
  LogMaxVerbosity = LOG_VERBOSITY_Info;
#endif

  RouterInit();

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
    PrintVersion("sqlite", sqlite3_version);
    FreeOptions();
    return EXIT_SUCCESS;
  }

  Greeting();
  LogInfo(PROGRAM_NAME " " PROGRAM_VERSION);
  LogInfo("Port: %d", GOptions.Port);
  LogInfo("Directory: %s", GOptions.Directory);
  LogInfo("Threads: %d", GOptions.Threads);

  if (DataBaseCreateIfNotExists(GOptions.Directory)) {
    return 1;
  }
  if (strcmp(GOptions.Directory, ".") != 0) {
    if (chdir(GOptions.Directory) != 0) {
      perror("chdir");
      FreeOptions();
      return EXIT_FAILURE;
    }
  }

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
