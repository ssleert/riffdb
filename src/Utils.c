#include "Utils.h"
#include "XMalloc.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

int32_t
MkdirIfNotExists(const char Path[])
{
  if (mkdir(Path, 0755) == 0) {
    return 0;
  }

  if (errno == EEXIST) {
    return 0;
  }

  return -1;
}

char*
XStrdup(const char Str[])
{
  size_t Len = strlen(Str) + 1;
  char* Copy = XMalloc(Len);
  (void)memcpy(Copy, Str, Len);
  return Copy;
}
