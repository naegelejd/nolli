#ifndef NOLLI_ALLOC_H
#define NOLLI_ALLOC_H

#include "nolli.h"

void *nalloc(size_t bytes);
void *nrealloc(void* block, size_t bytes);

#endif /* NOLLI_ALLOC_H */
