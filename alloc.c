#include "alloc.h"

void *nalloc(size_t bytes)
{
    assert(bytes);
    void* block = calloc(1, bytes);
    if (block == NULL) {
        NOLLI_DIE("nalloc failed");
    }

    return block;
}

void *nrealloc(void* block, size_t bytes)
{
    assert(block);
    assert(bytes);

    void* reblock = realloc(block, bytes);
    if (reblock == NULL) {
        NOLLI_DIE("nrealloc failed");
    }

    return reblock;
}
