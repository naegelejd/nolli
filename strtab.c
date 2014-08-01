#include "strtab.h"

struct string *stringtable_wrap(struct stringtable *tab, char *str)
{
    struct string **ptr = &tab->head;
    struct string *cur = NULL;
    while ((cur = *ptr) != NULL) {
        if (strcmp(str, cur->str) == 0) {
            return cur;
        }
        ptr = &cur->next;
    }

    *ptr = nl_alloc(sizeof(*ptr));
    (*ptr)->str = strdup(str);
    return *ptr;
}

void stringtable_dump(struct stringtable *tab, FILE *out)
{
    struct string *cur = tab->head;
    while (cur != NULL) {
        fprintf(out, "%s\n", cur->str);
        cur = cur->next;
    }
}
