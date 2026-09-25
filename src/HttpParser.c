#include "HttpParser.h"
#include "XMalloc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t
HttpParserSetContentLength(HttpParser* Parser)
{
  // TODO: ssleert - add check for any method except POST or PUT
  //       and return early

  for (uint16_t i = 0; i < Parser->HeadersLen; ++i) {
    if (Parser->Headers[i].KeyLen == 14 &&
        (strncmp(Parser->Headers[i].Key, "Content-Length", 14) == 0 ||
         strncmp(Parser->Headers[i].Key, "content-length", 14) == 0)) {
      Parser->ContentLength =
        (uint32_t)strtoul(Parser->Headers[i].Value, NULL, 10);
      return Parser->ContentLength;
    }
  }

  return 0;
}

HttpParserError
HttpParserInit(HttpParser* Self)
{
  *Self = (HttpParser){ 0 };

  for (size_t i = 0; i < HttpParserHeaderSize; ++i) {
    Self->Headers[i].Value = XMalloc(HttpParserHeaderValueSize);
  }

  Self->Body = XMalloc(HttpParserBodySize);

  Self->BodyCap = HttpParserBodySize;

  return 0;
}

HttpParserError
HttpParserZero(HttpParser* Self)
{
  Self->State = HttpParserStateMethod;
  Self->SawCr = false;
  Self->SawDoubleDot = false;
  Self->MethodLen = 0;
  Self->UrlLen = 0;
  Self->ContentLength = 0;
  Self->ConsumedBody = 0;
  Self->ContentLength = 0;

  for (uint32_t i = 0; i < Self->HeadersLen; ++i) {
    Self->Headers[i].KeyLen = 0;
    Self->Headers[i].ValueLen = 0;
  }

  Self->HeadersLen = 0;

  return 0;
}

HttpParserError
HttpParserParse(HttpParser* Self, size_t Len, const char Data[Len])
{
  if (Self->State == HttpParserStateBody) {
    return (HttpParserError)0;
  }

  if (Self->State == HttpParserStateComplete) {
    HttpParserZero(Self);
  }

  for (size_t i = 0; i < Len; ++i) {
    const char Byte = Data[i];

    switch (Self->State) {
      case HttpParserStateMethod:
        if (Byte == ' ') {
          Self->Method[Self->MethodLen] = '\0';
          Self->State = HttpParserStateUrl;
          break;
        }

        if (Self->MethodLen >= HttpParserMethodSize - 1) {
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

        if (Self->UrlLen >= HttpParserUrlSize - 1) {
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
          Self->Headers[Self->HeadersLen]
            .Key[Self->Headers[Self->HeadersLen].KeyLen] = '\0';
          Self->State = HttpParserStateHeaderValue;
          Self->SawDoubleDot = false;
          break;
        }

        if (Byte == '\r') {
          Self->SawCr = true;
          break;
        }

        if (Byte == '\n' && Self->SawCr) {
          Self->SawCr = false;

          HttpParserSetContentLength(Self);
          if (Self->ContentLength == 0) {
            Self->State = HttpParserStateComplete;
            return 0;
          }

          Self->State = HttpParserStateBody;
          Self->BodyStart = i + 1;
          return 0;
        }

        if (Self->HeadersLen >= HttpParserHeaderSize - 1) {
          break;
        }

        if (Self->Headers[Self->HeadersLen].KeyLen >=
            HttpParserHeaderKeySize - 1) {
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
            HttpParserHeaderValueSize - 1) {
          break;
        }

        Self->Headers[Self->HeadersLen]
          .Value[Self->Headers[Self->HeadersLen].ValueLen] = Byte;

        Self->Headers[Self->HeadersLen].ValueLen++;

        break;
      case HttpParserStateBody:
        return 0;
      default:
        return HttpParserErrorIncorrectState;
    }
  }

  return 0;
}

HttpParserError
HttpParserParseBody(HttpParser* Self, size_t Len, const char Data[Len])
{
  if (Self->State != HttpParserStateBody) {
    return 0;
  }

  if (Self->ConsumedBody == 0) {
    Data = &Data[Self->BodyStart];
    Len -= Self->BodyStart;
  }

  if (Self->BodyCap < Self->ContentLength) {
    Self->Body = XRealloc(Self->Body, Self->ContentLength);
    Self->BodyCap = Self->ContentLength;
  }

  if (Data != NULL) {
    memcpy(&Self->Body[Self->ConsumedBody], Data, Len);
  }

  Self->ConsumedBody += Len;

  if (Self->ConsumedBody == Self->ContentLength) {
    Self->State = HttpParserStateComplete;
  }

  return 0;
}

void
HttpParserFree(HttpParser* Self)
{
  for (size_t i = 0; i < HttpParserHeaderSize; ++i) {
    XFree(Self->Headers[i].Value);
  }

  XFree(Self->Body);
}
