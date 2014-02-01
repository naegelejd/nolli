#include "symtable.h"

/** Returns the next computed index for the two given hashes
 * @param H0 hash 0
 * @param H1 hash 1
 * @param I  iteration number
 * @param N  table size
 * @returns  index into hash table
 */
/* #define GET_INDEX(H0, H1, I, N)   ( ( (H0) + ((I) * (I)) * (H1) ) % (N) ) */
#define GET_INDEX(H0, I, N)   (((H0) + (I)) % (N))


static struct symtable *symtable_grow(struct symtable*);
static struct symtable *symtable_shrink(struct symtable*);
static struct symtable *symtable_resize(struct symtable*, unsigned int);

static void *symtable_do(struct symtable *table,
        const char *key, void *val, int what);

static unsigned int string_hash0(const char*);
static unsigned int string_hash1(const char*);
static unsigned int string_hash2(const char*);
static unsigned int string_hash3(const char*);

/** total number of possible hash table sizes */
const unsigned int MAX_TABLE_SIZE_OPTIONS = 28;

const unsigned int SYMTABLE_MAX_KEY_LEN = 512;

/**
 * Array of prime number hash table sizes.
 *
 * Numbers courtesy of:
 * http://planetmath.org/GoodHashTablePrimes.html
 */
static unsigned int SYMTABLE_SIZES[] = {
    7, 17, 43, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741, 0
};

static struct symtable *symtable_grow(struct symtable *st)
{
    return symtable_resize(st, st->size_idx + 1);
}

static struct symtable *symtable_shrink(struct symtable *st)
{
    return symtable_resize(st, st->size_idx - 1);
}

static struct symtable *symtable_resize(struct symtable *st, unsigned int new_size_idx)
{
    assert(st);

    if (new_size_idx <= 0 || new_size_idx >= MAX_TABLE_SIZE_OPTIONS) {
        return st;
    }

    unsigned int old_size = st->size;
    char** old_keys = st->keys;
    void** old_vals = st->vals;

    st->size_idx = new_size_idx;
    st->size = SYMTABLE_SIZES[new_size_idx];
    st->count = 0;
    st->collisions = 0;

    st->keys = nalloc(st->size * sizeof(*st->keys));
    st->vals = nalloc(st->size * sizeof(*st->vals));

    unsigned int i;
    for (i = 0; i < old_size; i++) {
        if (old_keys[i] != NULL) {
            symtable_do(st, old_keys[i], old_vals[i], SYMTABLE_INSERT);
        }
    }

    free(old_keys);
    free(old_vals);

    return st;
}

/** Creates and initializes a new struct symtable
 *
 * @returns new struct symtable
 */
struct symtable *symtable_create()
{
    struct symtable *st = nalloc(sizeof(*st));
    st->size_idx = 0;
    st->size = SYMTABLE_SIZES[st->size_idx];

    st->keys = nalloc(st->size * sizeof(*st->keys));
    st->vals = nalloc(st->size * sizeof(*st->vals));

    return st;
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
static void *symtable_do(struct symtable *table, const char *key, void *val, int what)
{
    if (table->count > (table->size * 0.60)) {
        symtable_grow(table);
    }

    unsigned int hash0 = string_hash0(key);

    unsigned int i = 0;
    for (i = 0; i < table->size; i++) {
        unsigned int idx = GET_INDEX(hash0, i, table->size);
        const char *curkey = table->keys[idx];

        if (curkey == NULL) {
            if (what == SYMTABLE_INSERT) {
                table->keys[idx] = (char *)key;
                table->vals[idx] = val;
                table->count++;
                return table->vals[idx];
            } else {
                return NULL;
            }
        } else if (strncmp(curkey, key, SYMTABLE_MAX_KEY_LEN) == 0) {
            return table->vals[idx];
        }
    }

    return NULL;
}

void *check_symbol(struct symtable *symtable, const char *name)
{
    return symtable_do(symtable, name, NULL, SYMTABLE_SEARCH);
}

void *add_symbol(struct symtable *symtable, const char *name, void *value)
{
    char *name_copy = strndup(name, TYPENAME_MAXLEN);
    void *val = symtable_do(symtable, name_copy, value, SYMTABLE_INSERT);

    return val;
}

/**
 * Destroys a struct symtable
 *
 * frees keys and values arrays
 *
 * @param st struct symtable
 */
void symtable_destroy(struct symtable *st)
{
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
