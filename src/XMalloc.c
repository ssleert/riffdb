#include "XMalloc.h"

#include "Log.h"

#include <stdlib.h>

void*
XMalloc(size_t Size)
{
  void* Ptr = malloc(Size);
  if (Ptr == NULL) {
    LogFatal("malloc failed (requested %zu bytes)", Size);
  }
  return Ptr;
}

void*
XCalloc(size_t Count, size_t Size)
{
  void* Ptr = calloc(Count, Size);
  if (Ptr == NULL) {
    LogFatal("calloc failed (%zu × %zu bytes)", Count, Size);
  }
  return Ptr;
}

void*
XRealloc(void* Ptr, size_t NewSize)
{
  void* NewPtr = realloc(Ptr, NewSize);
  if (NewPtr == NULL) {
    LogFatal("realloc failed (requested %zu bytes)", NewSize);
  }
  return NewPtr;
}

void
XFree(void* Ptr)
{
  free(Ptr);
}
