#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>

// Memory Allocator starter code

void* xmalloc(size_t bytes);
void  xfree(void* ptr);
void* xrealloc(void* prev, size_t bytes);

#endif
