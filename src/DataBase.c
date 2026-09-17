#include "DataBase.h"
#include "Log.h"
#include "Utils.h"

#include "sqlite3.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

int32_t
DataBaseCreateIfNotExists(const char Dir[])
{
  int32_t Rc = MkdirIfNotExists(Dir);
  if (Rc < 0) {
    LogErr("Cant create dir.");
    return -1;
  }

  sqlite3* Db = NULL;
  char* Err = NULL;

  char* DataBaseFile = malloc(strlen(Dir) + sizeof("/riff.db"));
  sprintf(DataBaseFile, "%s/riff.db", Dir);

  Rc = sqlite3_open_v2(
    DataBaseFile, &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  free(DataBaseFile);

  if (Rc != SQLITE_OK) {
    LogErr("Cant open sqlite3 file on %s/riff.db", Dir);
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
