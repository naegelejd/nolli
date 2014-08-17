#ifndef NOLLI_STRTAB_H
#define NOLLI_STRTAB_H

/**
 * Placeholder string table.
 *
 * Currently a singly-linked list of wrappers around `char*`.
 * A tree or hashtable would obviously be more ideal.
 */

struct nl_string {
    char const * str;
    struct nl_string *next;
};

struct nl_strtab {
    struct nl_string *head;
    /* struct string *tail; */
};

/**
 * Creates and stores and returns a string wrapper of the `char*`
 * or returns the existing wrapper in the table */
struct nl_string *nl_strtab_wrap(struct nl_strtab*, char*);

#include <stdio.h>
void nl_strtab_dump(struct nl_strtab *tab, FILE *out);

#endif /* NOLLI_STRTAB_H */
