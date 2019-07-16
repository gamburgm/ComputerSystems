

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "xmalloc.h"

// The challenge: beat the system allocator

void*
xmalloc(size_t bytes)
{
    return malloc(bytes);
}

void
xfree(void* ptr)
{
    free(ptr);
}

void*
xrealloc(void* prev, size_t bytes)
{
    return realloc(prev, bytes);
}

