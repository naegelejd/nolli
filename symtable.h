#ifndef NOLLI_SYMTABLE_H
#define NOLLI_SYMTABLE_H

#include "strtab.h"

struct nl_symbol {
    const nl_string_t name;
    const void *value;
    const struct nl_symbol *next;
};

struct nl_symtable {
    const struct nl_symtable *parent;
    struct nl_symbol *head;
    unsigned int count;         /**< current number of key/value pairs */
};

struct nl_symtable* nl_symtable_create(struct nl_context* ctx,
        const struct nl_symtable *parent);
struct nl_symtable* nl_symtable_destroy(struct nl_context* ctx,
        const struct nl_symtable*);
void *nl_symtable_get(const struct nl_symtable *, const nl_string_t);
void *nl_symtable_search(const struct nl_symtable *, const nl_string_t);
void *nl_symtable_add(struct nl_context* ctx,
        struct nl_symtable *, const nl_string_t, const void *);
void nl_symtable_dump(const struct nl_symtable *tab);

#endif /* NOLLI_SYMTABLE_H */
