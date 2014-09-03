#include "symtable.h"
#include "nolli.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> /* for dumping table */

static struct nl_symbol *new_symbol(nl_string_t name, void *value)
{
    struct nl_symbol *sym = nl_alloc(NULL, sizeof(*sym));
    *sym = (struct nl_symbol){.name=name, .value=value};
    return sym;
}

struct nl_symtable *nl_symtable_create(struct nl_symtable *parent)
{
    struct nl_symtable *tab = nl_alloc(NULL, sizeof(*tab));
    tab->parent = parent;

    return tab;
}

void *nl_symtable_check(struct nl_symtable *tab, nl_string_t name)
{
    struct nl_symtable *curtab = tab;
    while (curtab != NULL) {
        struct nl_symbol *sym = curtab->head;
        while (sym != NULL) {
            if (sym->name == name) {
                return sym->value;
            }
            sym = sym->next;
        }
        curtab = curtab->parent;
    }
    return NULL;
}

void *nl_symtable_add(struct nl_symtable *tab, nl_string_t name, void *value)
{
    struct nl_symbol **symp = &tab->head;
    while (*symp != NULL) {
        struct nl_symbol *sym = *symp;
        if (sym->name == name) {
            assert(strcmp(sym->name, name) == 0);
            /* Symbol already exists in table */
            void *old_value = sym->value;
            sym->value = value;
            return old_value;
        }
        symp = &(*symp)->next;
    }

    /* append symbol to end of table */
    (*symp) = new_symbol(name, value);
    tab->count++;

    return value;
}