#include "main.h"
#include "DataBase.h"
#include "Greeting.h"
#include "HttpParser.h"
#include "HttpResponse.h"
#include "Log.h"
#include "Options.h"
#include "Request.h"
#include "Router.h"
#include "SocketActions.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "Utils.h"
#include "Worker.h"
#include "XMalloc.h"

#include <sqlite3.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int
main(int32_t Argc, char* Argv[])
{
#ifdef NTRACE
  LogMaxVerbosity = LOG_VERBOSITY_Info;
#endif

  if (getenv("NO_COLOR") != NULL) { // NOLINT(concurrency-mt-unsafe)
    LogColored = false;
  }

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
    PrintVersion("wolfssl", wolfSSL_lib_version());
    PrintVersion("yyjson", YYJSON_VERSION_STRING);
    PrintVersion("cwpack", "commit 833fec9");
    FreeOptions();
    return EXIT_SUCCESS;
  }

  Greeting();
  LogInfo(PROGRAM_NAME " " PROGRAM_VERSION);
  LogInfo("Port: %d", GOptions.Port);
  LogInfo("Directory: %s", GOptions.Directory);
  LogInfo("Threads: %d", GOptions.Threads);

  if (strcmp(GOptions.Directory, ".") != 0) {
    int32_t Rc = MkdirIfNotExists(GOptions.Directory);
    if (Rc < 0) {
      perror("chdir");
      FreeOptions();
      return EXIT_FAILURE;
    }
    if (chdir(GOptions.Directory) != 0) {
      perror("chdir");
      FreeOptions();
      return EXIT_FAILURE;
    }
  }

  if (sqlite3_initialize()) {
    LogErr("cant init sqlite");
    return 1;
  }

  if (wolfSSL_Init() != SSL_SUCCESS) {
    LogErr("cant init wolfssl");
    return 1;
  }

  if (DataBaseCreateIfNotExists()) {
    return 1;
  }

  TcpServer Server = { 0 };

  if (TcpServerCreate(&Server, GOptions.Port, 1024) != 0) {
    perror("Failed to create server");
    return 1;
  }

  ThreadPool Pool = { 0 };
  if (ThreadPoolStart(
        &Pool, GOptions.Threads, (int (*)(void*))WorkerHandler) != 0) {
    LogErr("Failed to start thread pool");
    return 1;
  }

  TcpServerSetCallbacks(&Server,
                        SocketActionsOnConnect,
                        SocketActionsOnReadable,
                        SocketActionsOnDisconnect,
                        &Pool);

  LogInfo("Listening on port %d...", GOptions.Port);
  TcpServerRun(&Server);

  ThreadPoolStop(&Pool);

  TcpServerDestroy(&Server);

  return EXIT_SUCCESS;
}
