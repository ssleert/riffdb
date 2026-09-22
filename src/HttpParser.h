#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  HttpParserErrorAlloc = -1,
  HttpParserErrorIncorrectState = -2,
} HttpParserError;

enum {
  HttpParserMethodSize      = 9,
  HttpParserUrlSize         = 65,
  HttpParserHeaderSize      = 25,
  HttpParserHeaderKeySize   = 65,
  HttpParserHeaderValueSize = 8193,
  HttpParserBodySize        = 2049,
};

typedef enum {
  HttpParserStateMethod,
  HttpParserStateUrl,
  HttpParserStateVersion,
  HttpParserStateHeaderKey,
  HttpParserStateHeaderValue,
  HttpParserStateBody,
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

  char*      Body;
  uint32_t   BodyStart;
  uint32_t   BodyCap;
  uint32_t   ConsumedBody;
  uint32_t   ContentLength;
} HttpParser;

HttpParserError HttpParserInit(HttpParser* Self);
HttpParserError HttpParserZero(HttpParser* Self);

HttpParserError HttpParserParse(HttpParser* Self, size_t Len, const char Data[Len]);
HttpParserError HttpParserParseBody(HttpParser* Self, size_t Len, const char Data[Len]);

void HttpParserFree(HttpParser* Self);

uint32_t HttpParserSetContentLength(HttpParser* Parser);

#endif
