#include "symtable.h"

/** Returns the next computed index for the two given hashes
 * @param H0 hash 0
 * @param H1 hash 1
 * @param I  iteration number
 * @param N  table size
 * @returns  index into hash table
 */
#define GET_INDEX(H0, H1, I, N)   ( ( (H0) + ((I) * (I)) * (H1) ) % (N) )


/* static symtable_t *symtable_grow(symtable_t *map); */
/* static symtable_t *symtable_shrink(symtable_t *map); */
/* static symtable_t *symtable_resize(symtable_t *map, unsigned int new_size); */

/** total number of possible hash table sizes */
#define MAX_TABLE_SIZE_OPTIONS  28
/**
 * Array of prime number hash table sizes.
 *
 * Numbers courtesy of:
 * http://planetmath.org/GoodHashTablePrimes.html
 */
static unsigned int table_sizes[] = {
    7, 17, 43, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741, 0
};


/** Creates and initializes a new symtable_t
 *
 * @returns new symtable_t
 */
symtable_t *symtable_create()
{
    symtable_t *st = alloc(sizeof(*st));

    return st;
}

void symtable_add(symtable_t* st, void* key, symbol_t* s)
{
}

/**
 * Determines whether a symtable_t contains a key
 *
 * @param m symtable_t
 * @param o object
 * @returns 1 if str contains k, 0 otherwise
 */
bool symtable_contains(symtable_t *st, void *k)
{
    return false;
}

/**
 * Destroys a symtable_t
 *
 * frees keys and values arrays
 *
 * @param st symtable_t
 */
void symtable_destroy(symtable_t *st)
{
    free(st);
}
