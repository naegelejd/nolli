#include "strtab.h"
#include "nolli.h"
#include "debug.h"

#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

enum {
    NL_STRTAB_WRAP,
    NL_STRTAB_REWRAP
};

static unsigned int string_hash0(const char*);
static struct nl_strtab *nl_strtab_grow(struct nl_context* ctx,
        struct nl_strtab *tab);
static nl_string_t nl_strtab_rewrap(struct nl_context* ctx,
        struct nl_strtab *tab, nl_string_t key);
static nl_string_t nl_strtab_do(struct nl_context* ctx,
        struct nl_strtab *tab, const char *key, int action);

/** total number of possible hash table sizes */
const unsigned int NL_MAX_STRTABLE_SIZE_OPTIONS = 28;

/**
 * Array of prime number hash table sizes.
 *
 * Numbers courtesy of:
 * http://planetmath.org/GoodHashTablePrimes.html
 */
static unsigned int NL_STRTAB_SIZES[] = {
    7, 17, 43, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741, 0
};

int nl_strtab_init(struct nl_context* ctx, struct nl_strtab *tab)
{
    tab->size_idx = 0;
    tab->size = NL_STRTAB_SIZES[tab->size_idx];

    tab->strings = nl_alloc(ctx, tab->size * sizeof(*tab->strings));

    return NL_NO_ERR;
}

static struct nl_strtab *nl_strtab_grow(struct nl_context* ctx, struct nl_strtab *tab)
{
    assert(tab);
    unsigned int old_size = tab->size;
    nl_string_t *old_keys = tab->strings;

    tab->size_idx++;
    if (tab->size_idx >= NL_MAX_STRTABLE_SIZE_OPTIONS) {
        printf("%s\n", "Cannot grow stringtable any further!");     /* FIXME */
        return tab;
    }

    tab->size = NL_STRTAB_SIZES[tab->size_idx];
    tab->count = 0;
    tab->collisions = 0;

    tab->strings = nl_alloc(ctx, tab->size * sizeof(*tab->strings));

    unsigned int i;
    for (i = 0; i < old_size; i++) {
        if (old_keys[i] != NULL) {
            nl_strtab_rewrap(ctx, tab, old_keys[i]);
        }
    }

    nl_free(ctx, old_keys);

    return tab;
}

nl_string_t nl_strtab_wrap(struct nl_context* ctx,
        struct nl_strtab *tab, const char *key)
{
    return nl_strtab_do(ctx, tab, key, NL_STRTAB_WRAP);
}

static nl_string_t nl_strtab_rewrap(struct nl_context* ctx,
        struct nl_strtab *tab, nl_string_t key)
{
    return nl_strtab_do(ctx, tab, key, NL_STRTAB_REWRAP);
}

static nl_string_t nl_strtab_do(struct nl_context* ctx,
        struct nl_strtab *tab, const char *key, int action)
{
    assert(tab != NULL);

    if (tab->count > (tab->size * 0.60)) {
        tab = nl_strtab_grow(ctx, tab);
    }

    unsigned int hash0 = string_hash0(key);

    unsigned int i = 0;
    for (i = 0; i < tab->size; i++) {
        unsigned int idx = (hash0 + i) % tab->size;
        nl_string_t curkey = tab->strings[idx];


        if (NULL == curkey) {
            nl_string_t ret = NULL;
            if (action == NL_STRTAB_REWRAP) {
                /* add previously created nl_string to the table */
                ret = (nl_string_t)key;
            } else {
                /* add a new nl_string to the table */
                ret = strdup(key);
                if (ret == NULL) {
                    fprintf(stderr, "failed to wrap string %s\n", ret); /* FIXME */
                    return NULL;
                }
            }
            tab->strings[idx] = ret;
            tab->count++;
            return ret;
        } else if (strcmp(curkey, key) == 0) {
            /* return previously added nl_string from table */
            return curkey;
        }
    }

    nl_strtab_dump(tab);
    /* Should never reach here because the hashtable should always have
     * enough room to add a new string */
    assert(0);
    return NULL;
}

void nl_strtab_dump(struct nl_strtab *tab)
{
    unsigned int i = 0;
    for (i = 0; i < tab->size; i++) {
        const nl_string_t key = tab->strings[i];
        if (NULL != key) {
            printf("%s\n", key);
        }
    }
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
