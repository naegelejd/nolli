#include "symtable.h"

/** Returns the next computed index for the two given hashes
 * @param H0 hash 0
 * @param H1 hash 1
 * @param I  iteration number
 * @param N  table size
 * @returns  index into hash table
 */
#define GET_INDEX(H0, H1, I, N)   ( ( (H0) + ((I) * (I)) * (H1) ) % (N) )


static symtable_t *symtable_grow(symtable_t*);
static symtable_t *symtable_shrink(symtable_t*);
static symtable_t *symtable_resize(symtable_t*, unsigned int);
static unsigned int symtable_hash0(const char*);
static unsigned int symtable_hash1(const char*);

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

static symtable_t *symtable_grow(symtable_t *st)
{
    return symtable_resize(st, st->size_idx + 1);
}

static symtable_t *symtable_shrink(symtable_t *st)
{
    return symtable_resize(st, st->size_idx - 1);
}

static symtable_t *symtable_resize(symtable_t *st, unsigned int new_size_idx)
{
    assert(st);

    if (new_size_idx <= 0 || new_size_idx >= MAX_TABLE_SIZE_OPTIONS) {
        return st;
    }

    unsigned int old_size = st->size;
    const char** old_keys = st->keys;
    symbol_t** old_vals = st->vals;

    st->size_idx = new_size_idx;
    st->size = table_sizes[new_size_idx];
    st->count = 0;
    st->collisions = 0;

    st->keys = nalloc(st->size * sizeof(*st->keys));
    st->vals = nalloc(st->size * sizeof(*st->vals));

    unsigned int i;
    for (i = 0; i < old_size; i++) {
        if (old_keys[i] != NULL) {
            symtable_add(st, old_keys[i], old_vals[i]);
        }
    }

    free(old_keys);
    free(old_vals);

    return st;
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
static unsigned int symtable_hash0(const char* s)
{
    unsigned int h = 5381;
    int c;

    while ((c = *s++))
        h = ((h << 5) + h) + c;
    return h;
}

/* SDBM:
 *
 * hash(i) = hash(i - 1) * 65599 + str[i]
 */
static unsigned int symtable_hash1(const char* s)
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
static unsigned int symtable_hash_2(char *s)
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
static unsigned int symtable_hash_3(char *s)
{
    unsigned int h = 2166136261;    /* 14695981039346656037 for 64-bit hash */
    unsigned int c;
    while ((c = *s++)) {
        h ^= c;
        h *= 16777619;  /* 1099511628211 for 64-bit hash */
        h %= 32;        /* 64 for 64-bit hash */
    }
}

/** Creates and initializes a new symtable_t
 *
 * @returns new symtable_t
 */
symtable_t *symtable_create()
{
    symtable_t *st = nalloc(sizeof(*st));
    st->size_idx = 0;
    st->size = table_sizes[st->size_idx];

    st->keys = nalloc(st->size * sizeof(*st->keys));
    st->vals = nalloc(st->size * sizeof(*st->vals));

    return st;
}

void symtable_add(symtable_t* st, const char* key, symbol_t* val)
{
    assert(st);
    assert(key);

    if (st->count > (st->size * 0.60)) {
        symtable_grow(st);
    }

    uint32_t hash0 = symtable_hash0(key);
    uint32_t hash1 = symtable_hash1(key);

    unsigned int i = 0, idx = 0;
    for (i = 0; i < st->size; i++) {
        idx = GET_INDEX(hash0, hash1, i, st->size);
        const char* curkey = st->keys[idx];

        if (curkey == NULL) {
            printf("Adding symbol %s\n", key);
            st->keys[idx] = key;
            st->vals[idx] = val;
            st->count++;
            break;
        } else if (strcmp(curkey, key) == 0) {
            st->vals[idx] = val;
        } else {
            st->collisions++;
        }
    }
}

/**
 * Determines whether a symtable_t contains a key
 *
 * @param m symtable_t
 * @param o object
 * @returns 1 if str contains k, 0 otherwise
 */
bool symtable_contains(symtable_t *st, const char *key)
{
    assert(st);
    assert(key);

    printf("Checking for symbol %s\n", key);

    uint32_t hash0 = symtable_hash0(key);
    uint32_t hash1 = symtable_hash1(key);

    unsigned int i = 0, idx = 0;
    for (i = 0; i < st->size; i++) {
        idx = GET_INDEX(hash0, hash1, i, st->size);
        const char* curkey = st->keys[idx];

        if (st->keys[idx] == NULL) {
            printf("Symbol %s not in table\n", key);
            return false;
        }

        if (strcmp(curkey, key) == 0) {
            printf("Symbol %s IS in table\n", key);
            return true;
        }
    }

    printf("Symbol %s not in table\n", key);
    return false;
}

symbol_t* symtable_get(symtable_t* st, const char* key)
{
    assert(st);
    assert(key);

    uint32_t hash0 = symtable_hash0(key);
    uint32_t hash1 = symtable_hash1(key);

    unsigned int i = 0, idx = 0;
    for (i = 0; i < st->size; i++) {
        idx = GET_INDEX(hash0, hash1, i, st->size);
        const char* curkey = st->keys[idx];

        if (st->keys[idx] == NULL) {
            return NULL;
        }

        if (strcmp(curkey, key) == 0) {
            return st->vals[idx];
        }
    }
    return NULL;
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
