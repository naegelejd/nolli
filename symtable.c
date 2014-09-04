#include "symtable.h"
#include "nolli.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> /* for dumping table */

static struct nl_symbol *new_symbol(const nl_string_t name, const void *value)
{
    struct nl_symbol *sym = nl_alloc(NULL, sizeof(*sym));
    *sym = (struct nl_symbol){.name=name, .value=value};
    return sym;
}

struct nl_symtable *nl_symtable_create(const struct nl_symtable *parent)
{
    struct nl_symtable *tab = nl_alloc(NULL, sizeof(*tab));
    tab->parent = parent;
    return tab;
}

void *nl_symtable_search(const struct nl_symtable *tab, const nl_string_t name)
{
    const struct nl_symtable *cur = tab;
    while (cur != NULL) {
        void *value = nl_symtable_get(cur, name);
        if (value != NULL) {
            return value;
        }
        cur = cur->parent;
    }
    return NULL;
}

void *nl_symtable_get(const struct nl_symtable *tab, const nl_string_t name)
{
    const struct nl_symbol *sym = tab->head;
    while (sym != NULL) {
        if (name == sym->name) {
            return (void*)sym->value;
        }
        sym = sym->next;
    }
    return NULL;
}

void *nl_symtable_add(struct nl_symtable *tab, const nl_string_t name, const void *value)
{
    struct nl_symbol **symp = &tab->head;
    while (*symp != NULL) {
        struct nl_symbol *sym = *symp;
        if (sym->name == name) {
            assert(strcmp(sym->name, name) == 0);
            /* Symbol already exists in table */
            const void *old_value = sym->value;
            sym->value = value;
            return (void*)old_value;
        }
        symp = (struct nl_symbol **)&sym->next;
    }

    /* append symbol to end of table */
    (*symp) = new_symbol(name, value);
    tab->count++;

    return (void*)value;
}

void nl_symtable_dump(const struct nl_symtable *tab)
{
    const struct nl_symtable *curtab = tab;
    while (curtab != NULL) {
        const struct nl_symbol *sym = curtab->head;
        while (sym != NULL) {
            printf("%s: %p\n", sym->name, sym->value);
            sym = sym->next;
        }
        curtab = curtab->parent;
        printf("parent: %p\n", curtab);
    }
    printf("\n");
}