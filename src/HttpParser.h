#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef HTTP_PARSER_MALLOC_FUNC
#define HTTP_PARSER_MALLOC_FUNC malloc
#endif

typedef enum {
  HttpParserErrorAlloc = -1,
  HttpParserErrorIncorrectState = -2,
} HttpParserError;

enum {
  HttpParserMethodSize      = 8,
  HttpParserUrlSize         = 64,
  HttpParserHeaderSize      = 24,
  HttpParserHeaderKeySize   = 64,
  HttpParserHeaderValueSize = 8192,
};

typedef enum {
  HttpParserStateMethod,
  HttpParserStateUrl,
  HttpParserStateVersion,
  HttpParserStateHeaderKey,
  HttpParserStateHeaderValue,
  HttpParserStateComplete,
} HttpParserState;

typedef struct {
  char     Key[HttpParserHeaderKeySize];
  char*    Value;
  uint16_t ValueLen;
  uint8_t  KeyLen;
} HttpHeader;

typedef struct {
  uint8_t    State;
  bool       SawCr;
  bool       SawDoubleDot;

  char       Method[HttpParserMethodSize];
  uint8_t    MethodLen;

  char       Url[HttpParserUrlSize]; 
  uint8_t    UrlLen;

  uint8_t    HeadersLen;
  HttpHeader Headers[HttpParserHeaderSize];
} HttpParser;

HttpParserError HttpParserNew(HttpParser* Self);
HttpParserError HttpParserParse(HttpParser* Self, size_t Len, const char Text[Len]);

#endif
