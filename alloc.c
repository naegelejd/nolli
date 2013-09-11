#include "alloc.h"

void* alloc(size_t bytes)
{
    void* ret = calloc(1, bytes);
    if (ret == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    return ret;
}
