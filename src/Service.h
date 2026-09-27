#ifndef SERVICE_H
#define SERVICE_H

#include "sqlite3.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  ServiceOK = 0,
  ServiceErrorIncorrectJSON = -1,
  ServiceErrorQueryEmpty = -2,
  ServiceErrorQueryLen = -3,
  ServiceErrorProtocol = -4,
  ServiceErrorSqlite = -5,
  ServiceErrorCantCreateJSON = -6,
  ServiceErrorCancel = -7,
} ServiceError;

typedef struct { 
  _Atomic(bool)* Cancel;
  sqlite3 *Db;
  const char* Payload;
  const uint32_t PayloadLen;

  uint32_t ResSize; 
  const char* Res;
  uint16_t Status;
} ServiceState;

ServiceError ServiceExecute(
  ServiceState* Self
);

ServiceError ServiceQuery(
  ServiceState* Self
);

#endif
