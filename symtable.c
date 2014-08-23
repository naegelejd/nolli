#include "symtable.h"
#include "nolli.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> /* for dumping table */


/** Returns the next computed index for the two given hashes
 * @param H0 hash 0
 * @param H1 hash 1
 * @param I  iteration number
 * @param N  table size
 * @returns  index into hash table
 */
/* #define GET_INDEX(H0, H1, I, N)   ( ( (H0) + ((I) * (I)) * (H1) ) % (N) ) */


static struct nl_symtable *nl_symtable_grow(struct nl_symtable*);
static struct nl_symtable *nl_symtable_shrink(struct nl_symtable*);
static struct nl_symtable *nl_symtable_resize(struct nl_symtable*, unsigned int);

static void *nl_symtable_do(struct nl_symtable *table,
        const char *key, void *val, int what);

static unsigned int string_hash0(const char*);

/** total number of possible hash table sizes */
const unsigned int NL_MAX_TABLE_SIZE_OPTIONS = 28;

const unsigned int NL_SYMTABLE_MAX_KEY_LEN = 512;

/**
 * Array of prime number hash table sizes.
 *
 * Numbers courtesy of:
 * http://planetmath.org/GoodHashTablePrimes.html
 */
static unsigned int nl_symtable_SIZES[] = {
    7, 17, 43, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741, 0
};

static struct nl_symtable *nl_symtable_grow(struct nl_symtable *st)
{
    return nl_symtable_resize(st, st->size_idx + 1);
}

static struct nl_symtable *nl_symtable_shrink(struct nl_symtable *st)
{
    return nl_symtable_resize(st, st->size_idx - 1);
}

static struct nl_symtable *nl_symtable_resize(struct nl_symtable *st, unsigned int new_size_idx)
{
    assert(st);

    if (new_size_idx <= 0 || new_size_idx >= NL_MAX_TABLE_SIZE_OPTIONS) {
        return st;
    }

    unsigned int old_size = st->size;
    char** old_keys = st->keys;
    void** old_vals = st->vals;

    st->size_idx = new_size_idx;
    st->size = nl_symtable_SIZES[new_size_idx];
    st->count = 0;
    st->collisions = 0;

    st->keys = nl_alloc(NULL, st->size * sizeof(*st->keys));
    st->vals = nl_alloc(NULL, st->size * sizeof(*st->vals));

    unsigned int i;
    for (i = 0; i < old_size; i++) {
        if (old_keys[i] != NULL) {
            nl_symtable_do(st, old_keys[i], old_vals[i], NL_SYMTABLE_INSERT);
        }
    }

    free(old_keys);
    free(old_vals);

    return st;
}

/** Creates and initializes a new struct nl_symtable
 *
 * @returns new struct nl_symtable
 */
struct nl_symtable *nl_symtable_create(struct nl_symtable *parent)
{
    struct nl_symtable *st = nl_alloc(NULL, sizeof(*st));
    st->size_idx = 0;
    st->size = nl_symtable_SIZES[st->size_idx];

    st->parent = parent;
    st->keys = nl_alloc(NULL, st->size * sizeof(*st->keys));
    st->vals = nl_alloc(NULL, st->size * sizeof(*st->vals));

    return st;
}

void nl_symtable_dump(struct nl_symtable *tab)
{
    unsigned int i = 0;
    while (tab != NULL) {
        for (i = 0; i < tab->size; i++) {
            if (tab->keys[i] != NULL) {
                printf("%s -> %p\n", tab->keys[i], tab->vals[i]);
            }
        }
        tab = tab->parent;
    }
}
/*
    SEARCH
    NULL            key missing, return 0
    strcmp = 0      key found, return
    else            loop
    ...             not found, return 0

    INSERT
    NULL            insert it, return
    strcmp = 0      key exists, update value, return
    else            loop
    ...             should never get here if table dynamically expands

    GET
    NULL            key missing, return 0
    strcmp = 0      found key, return val
    else            loop
    ...             not found, return 0
*/
static void *nl_symtable_do(struct nl_symtable *table, const char *key, void *val, int what)
{
    assert(table != NULL);
    assert(key != NULL);

    if (table->count > (table->size * 0.60)) {
        nl_symtable_grow(table);
    } else if (table->count < (table->size * 0.20)) {
        nl_symtable_shrink(table);
    }

    unsigned int hash0 = string_hash0(key);

    unsigned int i = 0;
    for (i = 0; i < table->size; i++) {
        unsigned int idx = (hash0 + i) % table->size;
        const char *curkey = table->keys[idx];

        if (curkey == NULL) {
            if (what == NL_SYMTABLE_INSERT) {
                table->keys[idx] = (char *)key;
                table->vals[idx] = val;
                table->count++;
                return table->vals[idx];
            } else if (table->parent != NULL) {
                /* printf("Searching for %s in parent\n", key); */
                return nl_symtable_do(table->parent, key, val, what);
            } else {
                return NULL;
            }
        } else if (strncmp(curkey, key, NL_SYMTABLE_MAX_KEY_LEN) == 0) {
            return table->vals[idx];
        }
    }

    return NULL;
}

void *nl_symtable_check(struct nl_symtable *nl_symtable, const char *name)
{
    return nl_symtable_do(nl_symtable, name, NULL, NL_SYMTABLE_SEARCH);
}

void *nl_symtable_add(struct nl_symtable *nl_symtable, const char *name, void *value)
{
    char *name_copy = strndup(name, SYMBOL_MAXLEN);
    void *val = nl_symtable_do(nl_symtable, name_copy, value, NL_SYMTABLE_INSERT);

    return val;
}

/**
 * Destroys a struct nl_symtable
 *
 * frees keys and values arrays
 *
 * @param st struct nl_symtable
 */
void nl_symtable_destroy(struct nl_symtable *st)
{
    free(st->keys);
    free(st->vals);
    free(st);
}


/* djb2 (Daniel J. Bernstein):
 *
 * hash(i) = hash(i - 1) * 33 + str[i]
 *
 * Magic Constant 5381:
 *  1. odd number
 *  2. prime number
 *  3. deficient number
 *  4. 001/010/100/000/101 b
 *
 */
static unsigned int string_hash0(const char* s)
{
    unsigned int h = 5381;
    int c;

    while ((c = *s++))
        h = ((h << 5) + h) + c;
    return h;
}

#if 0
/* SDBM:
 *
 * hash(i) = hash(i - 1) * 65599 + str[i]
 */
static unsigned int string_hash1(const char* s)
{
    unsigned int h = 0;
    int c;
    while ((c = *s++))
        h = c + (h << 6) + (h << 16) - h;
    return h;
}


/* One-at-a-time (Bob Jenkins)
 *
 * @param s LuciStringObj to hash
 * @returns unsigned integer hash
 */
static unsigned int string_hash2(const char *s)
{
    unsigned int h = 0;
    int c;
    while ((c = *s++)) {
        h += c;
        h += h << 10;
        h ^= h >> 6;
    }
    h += h << 3;
    h ^= h >> 11;
    h += h << 15;
    return h;
}


/* FNV-1a:
 *
 * hash = offset_basis
 * for each octet_of_data to hash:
 *     hash = hash xor octet_of_data
 *     hash = hash * FNV_prime
 * return hash
 */
static unsigned int string_hash3(const char *s)
{
    unsigned int h = 2166136261;    /* 14695981039346656037 for 64-bit hash */
    unsigned int c;
    while ((c = *s++)) {
        h ^= c;
        h *= 16777619;  /* 1099511628211 for 64-bit hash */
        h %= 32;        /* 64 for 64-bit hash */
    }
    return h;
}
#endif
