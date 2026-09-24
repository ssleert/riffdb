#ifndef DATABASE_H
#define DATABASE_H

#include <stdbool.h>
#include <stdint.h>

#include <sqlite3.h>
#include <yyjson.h>

int32_t DataBaseCreateIfNotExists(const char Dir[]);
sqlite3* DataBaseOpen(const char Dir[], bool ReadOnly);

#endif
