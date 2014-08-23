#ifndef NOLLI_SYMTABLE_H
#define NOLLI_SYMTABLE_H

#define INIT_SYMTABLE_SIZE 8     /**< initial allocated size of a map */
#define SYMBOL_MAXLEN 1024

/* TODO?: maybe use a single array of key-val pairs instead of two separate
 * arrays. This could decrease cache misses */
struct nl_symtable {
    struct nl_symtable *parent;
    char **keys;
    void **vals;
    unsigned int size_idx;      /**< identifier for current size of table */
    unsigned int collisions;    /**< number of hash collisions */
    unsigned int count;         /**< current number of key/value pairs */
    unsigned int size;          /**< current count of allocated pairs*/
};

enum {NL_SYMTABLE_SEARCH = 0, NL_SYMTABLE_INSERT = 1};

struct nl_symtable* nl_symtable_create(struct nl_symtable *parent);
void nl_symtable_destroy(struct nl_symtable *st);
void *nl_symtable_check(struct nl_symtable *, const char *);
void *nl_symtable_add(struct nl_symtable *, const char *, void *);
void nl_symtable_dump(struct nl_symtable *tab);

#endif /* NOLLI_SYMTABLE_H */
