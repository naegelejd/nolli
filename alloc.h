#ifndef NOLLI_ALLOC_H
#define NOLLI_ALLOC_H

#include <stddef.h>

void *nl_alloc(size_t bytes);
void *nl_realloc(void* block, size_t bytes);

#endif /* NOLLI_ALLOC_H */
