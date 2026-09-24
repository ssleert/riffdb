#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>

void* XMalloc(size_t Size);
void* XCalloc(size_t Count, size_t Size);
void* XRealloc(void* Ptr, size_t NewSize);
void  XFree(void* Ptr);

#endif
