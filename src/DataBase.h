#ifndef DATABASE_H
#define DATABASE_H

#include <stdbool.h>
#include <stdint.h>

#include <sqlite3.h>
#include <yyjson.h>

int32_t DataBaseCreateIfNotExists(void);
sqlite3* DataBaseOpen(bool ReadOnly);

#endif
