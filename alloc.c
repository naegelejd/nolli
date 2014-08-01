#include "nolli.h"

void *nl_alloc(size_t bytes)
{
    assert(bytes);
    void* block = calloc(1, bytes);
    if (block == NULL) {
        NOLLI_FATAL("nalloc failed");
    }

    return block;
}

void *nl_realloc(void* block, size_t bytes)
{
    assert(block);
    assert(bytes);

    void* reblock = realloc(block, bytes);
    if (reblock == NULL) {
        NOLLI_FATAL("nrealloc failed");
    }

    return reblock;
}
