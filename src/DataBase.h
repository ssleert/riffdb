#ifndef DATABASE_H
#define DATABASE_H

#include <stdint.h>

#include <yyjson.h>
#include <sqlite3.h>

int32_t DataBaseCreateIfNotExists(const char Dir[]);

int32_t DataBaseBindJsonArgsToStmt(const yyjson_val* Args, sqlite3_stmt* Stmt);

#endif
