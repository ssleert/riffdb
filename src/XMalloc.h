#ifndef XMALLOC_H_
#define XMALLOC_H_

#include <stddef.h>

void* XMalloc(size_t Size);
void* XCalloc(size_t Count, size_t Size);
void* XRealloc(void* Ptr, size_t NewSize);
void  XFree(void* Ptr);

#endif /* XMALLOC_H_ */
