#ifndef NOLLI_STRTAB_H
#define NOLLI_STRTAB_H

typedef char* nl_string_t;

struct nl_strtab {
    nl_string_t *strings;
    unsigned int size_idx;      /**< identifier for current size of table */
    unsigned int collisions;    /**< number of hash collisions */
    unsigned int count;         /**< current number of key/value pairs */
    unsigned int size;          /**< current count of allocated pairs*/
};

int nl_strtab_init(struct nl_strtab *tab);
/**
 * Creates and stores and returns a string wrapper of the `char*`
 * or returns the existing wrapper in the table */
nl_string_t nl_strtab_wrap(struct nl_strtab *tab, const char *key);

void nl_strtab_dump(struct nl_strtab *tab);

#endif /* NOLLI_STRTAB_H */