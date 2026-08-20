#include "HttpParser.h"
#include <stdlib.h>

HttpParserError
HttpParserNew(HttpParser* Self)
{
  *Self = (HttpParser){ 0 };

  for (size_t i = 0; i < HttpParserHeaderSize; ++i) {
    Self->Headers[i].Value = HTTP_PARSER_MALLOC_FUNC(HttpParserHeaderValueSize);
    if (Self->Headers[i].Value == NULL) {
      return HttpParserErrorAlloc;
    }
  }

  return 0;
}

HttpParserError
HttpParserParse(HttpParser* Self, size_t Len, const char Text[Len])
{
  if (Self->State == HttpParserStateComplete) {
    Self->State = HttpParserStateMethod;
    Self->SawCr = false;
    Self->SawDoubleDot = false;
    Self->MethodLen = 0;
    Self->UrlLen = 0;
    Self->HeadersLen = 0;
  }

  for (size_t i = 0; i < Len; ++Len) {
    const char Byte = Text[i];

    switch (Self->State) {
      case HttpParserStateMethod:
        if (Byte == ' ') {
          Self->Method[Self->MethodLen] = '\0';
          Self->State = HttpParserStateUrl;
          break;
        }

        if (Self->MethodLen >= HttpParserMethodSize) {
          break;
        }

        Self->Method[Self->MethodLen] = Byte;
        Self->MethodLen++;

        break;
      case HttpParserStateUrl:
        if (Byte == ' ') {
          Self->Url[Self->UrlLen] = '\0';
          Self->State = HttpParserStateVersion;
          break;
        }

        if (Self->UrlLen >= HttpParserUrlSize) {
          break;
        }

        Self->Url[Self->UrlLen] = Byte;
        Self->UrlLen++;

        break;
      case HttpParserStateVersion:
        if (Byte == '\r') {
          Self->SawCr = true;
          break;
        }

        if (Byte == '\n' && Self->SawCr) {
          Self->State = HttpParserStateHeaderKey;
          Self->SawCr = false;
          break;
        }

        // i dont care about version of http

        break;
      case HttpParserStateHeaderKey:
        if (Byte == ':') {
          Self->SawDoubleDot = true;
          break;
        }

        if (Byte == ' ' && Self->SawDoubleDot) {
          Self->State = HttpParserStateHeaderValue;
          Self->SawDoubleDot = false;
          break;
        }

        if (Byte == '\r') {
          Self->SawCr = true;
          break;
        }

        if (Byte == '\n' && Self->SawCr) {
          Self->State = HttpParserStateComplete;
          return i;
        }

        if (Self->HeadersLen > HttpParserHeaderSize) {
          break;
        }

        if (Self->Headers[Self->HeadersLen].KeyLen >= HttpParserHeaderKeySize) {
          break;
        }

        Self->Headers[Self->HeadersLen]
          .Key[Self->Headers[Self->HeadersLen].KeyLen] = Byte;

        Self->Headers[Self->HeadersLen].KeyLen++;

        break;
      case HttpParserStateHeaderValue:
        if (Byte == '\r') {
          Self->SawCr = true;
          break;
        }

        if (Byte == '\n' && Self->SawCr) {
          Self->Headers[Self->HeadersLen]
            .Value[Self->Headers[Self->HeadersLen].ValueLen] = '\0';
          Self->State = HttpParserStateHeaderKey;
          Self->SawCr = false;
          Self->HeadersLen++;
          break;
        }

        if (Self->Headers[Self->HeadersLen].ValueLen >=
            HttpParserHeaderValueSize) {
          break;
        }

        Self->Headers[Self->HeadersLen]
          .Value[Self->Headers[Self->HeadersLen].ValueLen] = Byte;

        Self->Headers[Self->HeadersLen].ValueLen++;

        break;
      default:
        return HttpParserErrorIncorrectState;
    }
  }

  return 0;
}
