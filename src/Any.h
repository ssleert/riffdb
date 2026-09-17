#ifndef ANY_H
#define ANY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  AnyTypeNull,
  AnyTypeInt,
  AnyTypeDouble,
  AnyTypeBool,
  AnyTypeStr,
} AnyType;

typedef struct {
  AnyType Type;
  union {
    int64_t Int;
    double  Double;
    bool    Bool;
    char*   Str;
  };
} Any;

#define AnyNull (Any){.Type = AnyTypeNull}

#endif
