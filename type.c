#include "type.h"


int check_type(struct typetable *typetable, const char *name);
int add_type(struct typetable *typetable, const char *name);
int readd_type(struct typetable *typetable, const char *name, int id);

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
static unsigned int string_hash_2(char *s)
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

//#define GET_INDEX(H0, H1, I, N)   ( ( (H0) + ((I) * (I)) * (H1) ) % (N) )
#define GET_INDEX(H0, I, N)   (((H0) + (I)) % (N))

/** total number of possible hash table sizes */
#define MAX_TABLE_SIZE_OPTIONS  28
/**
 * Array of prime number hash table sizes.
 *
 * Numbers courtesy of:
 * http://planetmath.org/GoodHashTablePrimes.html
 */
static unsigned int TYPETABLE_SIZES[] = {
    7, 17, 43, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741, 0
};

static int typetable_do(struct typetable *table,
        const char *key, int val, int what);

static struct typetable *typetable_resize(struct typetable *tt, unsigned int new_size_idx)
{
    assert(tt);

    if (new_size_idx <= 0 || new_size_idx >= MAX_TABLE_SIZE_OPTIONS) {
        return tt;
    }

    unsigned int old_size = tt->size;
    char **old_names = tt->names;
    int *old_ids = tt->ids;

    tt->size_idx = new_size_idx;
    tt->size = TYPETABLE_SIZES[new_size_idx];
    tt->count = 0;

    tt->names = nalloc(tt->size * sizeof(*tt->names));
    tt->ids = nalloc(tt->size * sizeof(*tt->ids));

    unsigned int i;
    for (i = 0; i < old_size; i++) {
        if (old_names[i] != NULL) {
            typetable_do(tt, old_names[i], old_ids[i], TYPETABLE_INSERT);
        }
    }

    free(old_names);
    free(old_ids);

    return tt;
}

static struct typetable *typetable_grow(struct typetable *tt)
{
    return typetable_resize(tt, tt->size_idx + 1);
}

static struct typetable *typetable_shrink(struct typetable *tt)
{
    return typetable_resize(tt, tt->size_idx - 1);
}

static struct typetable *new_typetable(void)
{
    struct typetable *table = nalloc(sizeof(*table));
    table->count = 0;
    table->size_idx = 1;    /* start off with 17 slots in table */
    table->size = TYPETABLE_SIZES[table->size_idx];
    table->names = nalloc(table->size * sizeof(*table->names));
    table->ids = nalloc(table->size * sizeof(*table->ids));

    const char *typenames[] = {
        "bool", "char", "int", "real", "str", "file"
    };
    unsigned int tidx = 0;
    for (tidx = 0; tidx < sizeof(typenames) / sizeof(*typenames); tidx++) {
        typetable_do(table, typenames[tidx], -1, TYPETABLE_INSERT);
    }

    return table;
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
static int typetable_do(struct typetable *table, const char *key, int val, int what)
{
    if (table->count > (table->size * 0.60)) {
        typetable_grow(table);
    }

    unsigned int hash0 = string_hash0(key);

    unsigned int i = 0;
    for (i = 0; i < table->size; i++) {
        unsigned int idx = GET_INDEX(hash0, i, table->size);
        const char *curkey = table->names[idx];

        if (curkey == NULL) {
            if (what == TYPETABLE_INSERT) {
                table->names[idx] = (char *)key;
                if (val < 0) {
                    table->ids[idx] = table->count;
                } else {
                    table->ids[idx] = val;
                }
                table->count++;
                return table->ids[idx];
            } else {
                return -1;
            }
        } else if (strncmp(curkey, key, TYPENAME_MAXLEN) == 0) {
            /* Once a type is defined, it cannot be changed */
            return table->ids[idx];
        }
    }

    return -1;
}

int check_type(struct typetable *typetable, const char *name)
{
    return typetable_do(typetable, name, -1, TYPETABLE_SEARCH);
}

int add_type(struct typetable *typetable, const char *name)
{
    char *name_copy = strndup(name, TYPENAME_MAXLEN);
    int id = typetable_do(typetable, name_copy, -1, TYPETABLE_INSERT);

    return id;
    /* return typetable_do(typetable, name, -1, TYPETABLE_INSERT); */
}

type_t bool_type = {
    TYPE_BOOL,
    "bool",
    0,
    0
};

type_t char_type = {
    TYPE_CHAR,
    "char",
    0,
    0
};

type_t int_type = {
    TYPE_INT,
    "int",
    0,
    0
};

type_t real_type = {
    TYPE_REAL,
    "real",
    0,
    0
};

type_t str_type = {
    TYPE_STR,
    "str",
    0,
    0
};

type_t file_type = {
    TYPE_FILE,
    "file",
    0,
    0
};

type_t* new_list_type(type_t* tp)
{
    type_t* list_type = nalloc(sizeof(*list_type));
    list_type->id = TYPE_LIST;
    list_type->name = strdup("list");
    list_type->n = 1;
    list_type->kinds = nalloc(list_type->n * sizeof(*list_type->kinds));
    *list_type->kinds = tp;

    return list_type;
}

type_t* new_map_type(type_t* ktp, type_t* vtp)
{
    type_t* map_type = nalloc(sizeof(*map_type));
    map_type->id = TYPE_MAP;
    map_type->name = strdup("map");
    map_type->n = 2;
    map_type->kinds = nalloc(map_type->n * sizeof(*map_type->kinds));
    map_type->kinds[0] = ktp;
    map_type->kinds[1] = vtp;

    return map_type;
}

type_t* new_user_type(char *name)
{
    type_t* user_type = nalloc(sizeof(*user_type));
    user_type->id = TYPE_USER;
    user_type->name = name;

    return user_type;
}
