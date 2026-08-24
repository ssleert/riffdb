#include "HttpUtils.h"

#include <stdlib.h>
#include <string.h>

uint32_t HttpUtilsGetContentLength(HttpParser* Parser) {
  for (uint16_t i = 0; i < Parser->HeadersLen; ++i) {
    if (Parser->Headers[i].KeyLen == 14 &&
        strncmp(Parser->Headers[i].Key, "Content-Length", 14) == 0) {
      return (uint32_t)strtoul(Parser->Headers[i].Value, NULL, 10);
    }
  }

  return 0;
}
