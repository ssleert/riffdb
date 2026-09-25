#include "DataBase.h"
#include "Log.h"
#include "Utils.h"
#include "XMalloc.h"

#include "Options.h"
#include "sqlite3.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

int32_t
DataBaseCreateIfNotExists(void)
{
  int32_t Rc = 0;
  sqlite3* Db = NULL;
  char* Err = NULL;

  Rc = sqlite3_open_v2(
    "./riff.db", &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  if (Rc != SQLITE_OK) {
    LogErr("Cant open sqlite3 file on %s/riff.db", GOptions.Directory);
    sqlite3_free(Db);
    return -1;
  }

  Rc = sqlite3_exec(Db,
                    "PRAGMA journal_mode = WAL;"
                    "PRAGMA synchronous = normal;"
                    "PRAGMA temp_store = memory;",
                    NULL,
                    NULL,
                    &Err);
  if (Rc != SQLITE_OK) {
    LogErr("Cant execute pragmas: %s", Err);
    sqlite3_free(Err);
    sqlite3_close(Db);
    return -1;
  }

  Rc = sqlite3_exec(Db,
                    "CREATE TABLE IF NOT EXISTS _riff ("
                    "    id INTEGER PRIMARY KEY,"
                    "    version TEXT NOT NULL"
                    ");",
                    NULL,
                    NULL,
                    &Err);
  if (Rc != SQLITE_OK) {
    LogErr("Cant execute meta table creation: %s", Err);
    sqlite3_free(Err);
    sqlite3_close(Db);
    return -1;
  }

  sqlite3_close(Db);
  return 0;
}

sqlite3*
DataBaseOpen(bool ReadOnly)
{
  sqlite3* Db = NULL;

  int32_t Flags = SQLITE_OPEN_CREATE;
  if (ReadOnly) {
    Flags = Flags | SQLITE_OPEN_READONLY;
  } else {
    Flags = Flags | SQLITE_OPEN_READWRITE;
  }

  int32_t Rc = sqlite3_open_v2("./riff.db", &Db, Flags, NULL);

  sqlite3_busy_timeout(Db, 5000);

  if (Rc != SQLITE_OK) {
    LogErr("cant open sqlite connection for worker: Rc = %d", Rc);
    return NULL;
  }

  return Db;
}
