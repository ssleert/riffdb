#include "DataBase.h"
#include "Log.h"
#include "Utils.h"
#include "XMalloc.h"

#include "sqlite3.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"
#include "yyjson.h"
#include <stdint.h>

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

  char* DataBaseFile = XMalloc(strlen(Dir) + sizeof("/riff.db"));
  sprintf(DataBaseFile, "%s/riff.db", Dir);

  Rc = sqlite3_open_v2(
    DataBaseFile, &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  XFree(DataBaseFile);

  if (Rc != SQLITE_OK) {
    LogErr("Cant open sqlite3 file on %s/riff.db", Dir);
    sqlite3_free(Db);
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

int32_t
DataBaseBindJsonArgsToStmt(const yyjson_val* Args, sqlite3_stmt* Stmt)
{
  yyjson_val* Element = NULL;
  yyjson_arr_iter ArgsIter = yyjson_arr_iter_with(Args);

  while ((Element = yyjson_arr_iter_next(&ArgsIter))) {
    int32_t Idx = ArgsIter.idx;
    if (yyjson_is_str(Element)) {
      const char* Str = yyjson_get_str(Element);
      size_t Len = yyjson_get_len(Element);

      sqlite3_bind_text(Stmt, Idx, Str, Len, NULL);
      continue;
    }

    if (yyjson_is_int(Element)) {
      int64_t Int = yyjson_get_sint(Element);

      sqlite3_bind_int64(Stmt, Idx, Int);
      continue;
    }

    if (yyjson_is_real(Element)) {
      double Double = yyjson_get_real(Element);

      sqlite3_bind_double(Stmt, Idx, Double);
      continue;
    }

    if (yyjson_is_bool(Element)) {
      bool Bool = yyjson_get_bool(Element);

      sqlite3_bind_int(Stmt, Idx, (int32_t)Bool);
      continue;
    }

    if (yyjson_is_null(Element)) {
      sqlite3_bind_null(Stmt, Idx);
      continue;
    }
  }

  return 0;
}

sqlite3*
DataBaseOpen(const char Dir[], bool ReadOnly)
{
  sqlite3* Db = NULL;

  char* DataBaseFile = XMalloc(strlen(Dir) + sizeof("/riff.db"));
  sprintf(DataBaseFile, "%s/riff.db", Dir);

  int32_t Flags = SQLITE_OPEN_CREATE;
  if (ReadOnly) {
    Flags = Flags | SQLITE_OPEN_READONLY;
  } else {
    Flags = Flags | SQLITE_OPEN_READWRITE;
  }

  int32_t Rc = sqlite3_open_v2(DataBaseFile, &Db, Flags, NULL);
  XFree(DataBaseFile);

  if (Rc != 0) {
    LogErr("cant open sqlite connection for worker");
    return NULL;
  }

  return Db;
}
