#ifndef NOLLI_TYPE_H
#define NOLLI_TYPE_H

#include "nolli.h"

enum {TYPENAME_MAXLEN = 32};

enum {
    TYPE_start,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_REAL,
    TYPE_STR,
    TYPE_FUNC,
    TYPE_CLASS,
    TYPE_end
};

struct type_func {
    struct type *ret_type;
    struct type *param_type_head;
};

struct type_class {
    struct type *tmpls_head;
};

struct type {
    union {
        struct type_func func;
        struct type_class clss;
    };

    struct type *next;
    const char* repr;

    int tag;
    unsigned int n;
};

extern struct type bool_type;
extern struct type char_type;
extern struct type int_type;
extern struct type real_type;
extern struct type str_type;

/* TODO: all types should be hashed when they are first parsed! */
struct type* type_new_func(struct type *ret_type, struct type *param_types_head);
struct type* type_new_class(const char *name);


#endif /* NOLLI_TYPE_H */
