#ifndef NOLLI_SYMTABLE_H
#define NOLLI_SYMTABLE_H

#include "nolli.h"

#define INIT_SYMTABLE_SIZE 8     /**< initial allocated size of a map */

/* TODO?: maybe use a single array of key-val pairs instead of two separate
 * arrays. This should decrease cache misses */
struct symtable {
    struct symtable *parent;
    char **keys;
    void **vals;
    unsigned int size_idx;      /**< identifier for current size of table */
    unsigned int collisions;    /**< number of hash collisions */
    unsigned int count;         /**< current number of key/value pairs */
    unsigned int size;          /**< current count of allocated pairs*/
};

enum {SYMTABLE_SEARCH = 0, SYMTABLE_INSERT = 1};

void *check_symbol(struct symtable *, const char *);
void *add_symbol(struct symtable *, const char *, void *);
struct symtable* symtable_create(struct symtable *parent);
void symtable_destroy(struct symtable *st);

#endif /* NOLLI_SYMTABLE_H */
