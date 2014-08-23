#include "strtab.h"
#include "nolli.h"
#include "debug.h"

#include <string.h>

#include <stdio.h>
#include <assert.h>

struct nl_string *nl_strtab_wrap(struct nl_strtab *tab, const char *s)
{
    assert(tab != NULL);
    assert(s != NULL);

    struct nl_string **ptr = &tab->head;
    while (*ptr != NULL) {
        assert((*ptr)->str != NULL);
        if (strcmp(s, (*ptr)->str) == 0) {
            return *ptr;
        }
        ptr = &(*ptr)->next;
    }

    *ptr = nl_alloc(NULL, sizeof(**ptr));

    (*ptr)->str = strdup(s);
    if ((*ptr)->str == NULL) {
        fprintf(stderr, "failed to wrap string"); /* FIXME */
        return NULL;
    }

    return *ptr;
}

void nl_strtab_dump(struct nl_strtab *tab, FILE *out)
{
    struct nl_string *cur = tab->head;
    while (cur != NULL) {
        fprintf(out, "%s\n", cur->str);
        cur = cur->next;
    }
}
