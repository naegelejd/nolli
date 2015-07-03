#ifndef NOLLI_TYPE_H
#define NOLLI_TYPE_H

#include "strtab.h"

#include <stdbool.h>

enum {
    NL_TYPE_start,
    NL_TYPE_BOOL,
    NL_TYPE_CHAR,
    NL_TYPE_INT,
    NL_TYPE_REAL,
    NL_TYPE_STR,
    NL_TYPE_FUNC,
    NL_TYPE_CLASS,
    NL_TYPE_INTERFACE,
    NL_TYPE_REFERENCE,

    NL_TYPE_TMPL_PLACEHOLDER,

    NL_TYPE_end
};

struct nl_type_func {
    struct nl_type *ret_type;
    struct nl_type *param_types_head;
    unsigned int param_count;
};

struct nl_type_class {
    struct nl_symtable *tmpl_types;
    struct nl_symtable *member_types;
    struct nl_symtable *method_types;
};

struct nl_type_interface {
    struct nl_symtable *method_types;
};

struct nl_type_reference {
    nl_string_t package_name, type_name;
};

struct nl_type {
    union {
        struct nl_type_func func;
        struct nl_type_class clss;
        struct nl_type_interface interface;
        struct nl_type_reference reference;
    };

    struct nl_type *next;
    const char* repr;

    int tag;
    unsigned int n;
};

extern struct nl_type nl_bool_type;
extern struct nl_type nl_char_type;
extern struct nl_type nl_int_type;
extern struct nl_type nl_real_type;
extern struct nl_type nl_str_type;
extern struct nl_type nl_tmpl_placeholder_type;

/* TODO: all types should be hashed when they are first parsed! */
struct nl_type* nl_type_new_func(struct nl_type *ret_type,
        struct nl_type *param_types_head, unsigned int count);
struct nl_type* nl_type_new_class(const char *name, struct nl_symtable *tmpls,
        struct nl_symtable *members, struct nl_symtable *methods);
struct nl_type* nl_type_new_interface(const char *name, struct nl_symtable *methods);
struct nl_type* nl_type_new_reference(nl_string_t package_name, nl_string_t type_name);

bool nl_types_equal(struct nl_type *tp1, struct nl_type *tp2);

#endif /* NOLLI_TYPE_H */
