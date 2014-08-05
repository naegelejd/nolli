#ifndef NOLLI_STRING_TABLE_H
#define NOLLI_STRING_TABLE_H

/**
 * Placeholder string table.
 *
 * Currently a singly-linked list of wrappers around `char*`.
 * A tree or hashtable would obviously be more ideal.
 */

struct string {
    char const * str;
    struct string *next;
};

struct stringtable {
    struct string *head;
    /* struct string *tail; */
};

/**
 * Creates and stores and returns a string wrapper of the `char*`
 * or returns the existing wrapper in the table */
struct string *stringtable_wrap(struct stringtable*, char*);

#include <stdio.h>
void stringtable_dump(struct stringtable *tab, FILE *out);

#endif /* NOLLI_STRING_TABLE_H */
