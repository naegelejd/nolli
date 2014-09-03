#ifndef NOLLI_SYMTABLE_H
#define NOLLI_SYMTABLE_H

#define INIT_SYMTABLE_SIZE 8     /**< initial allocated size of a map */
#define SYMBOL_MAXLEN 1024

#include "strtab.h"

struct nl_symbol {
    nl_string_t name;
    void *value;
    struct nl_symbol *next;
};

/* TODO?: maybe use a single array of key-val pairs instead of two separate
 * arrays. This could decrease cache misses */
struct nl_symtable {
    struct nl_symtable *parent;
    struct nl_symbol *head;
    unsigned int count;         /**< current number of key/value pairs */
};

struct nl_symtable* nl_symtable_create(struct nl_symtable *parent);
void *nl_symtable_check(struct nl_symtable *, nl_string_t);
void *nl_symtable_add(struct nl_symtable *, nl_string_t, void *);
void nl_symtable_dump(struct nl_symtable *tab);

#endif /* NOLLI_SYMTABLE_H */
