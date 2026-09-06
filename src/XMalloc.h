#ifndef _XMALLOC_H_
#define _XMALLOC_H_

#include <stddef.h>

void* XMalloc(size_t Size);
void* XCalloc(size_t Count, size_t Size);
void* XRealloc(void* Ptr, size_t NewSize);
void  XFree(void* Ptr);

#endif /* _XMALLOC_H_ */
