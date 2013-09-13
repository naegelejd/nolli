#ifndef NOLLI_SYMTABLE_H
#define NOLLI_SYMTABLE_H

#include "nolli.h"

#define INIT_SYMTABLE_SIZE 8     /**< initial allocated size of a map */

typedef struct symbol {
    int type;
} symbol_t;

typedef struct symtable {
    const char** keys;  /**< array of pointers to keys */
    symbol_t** vals;  /**< array of pointers to values */
    unsigned int size_idx;  /**< identifier for current size of table */
    unsigned int collisions;    /**< number of hash collisions */
    unsigned int count; /**< current number of key/value pairs */
    unsigned int size;  /**< current count of allocated pairs*/
} symtable_t;

symtable_t* symtable_create(void);
void symtable_add(symtable_t* st, const char* key, symbol_t *val);
bool symtable_contains(symtable_t* st, const char* key);
symbol_t* symtable_get(symtable_t* st, const char* key);
void symtable_destroy(symtable_t* st);

#endif
