#include "type.h"

struct type bool_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "bool",
    .tag = TYPE_BOOL,
    .n = 0
};

struct type char_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "char",
    .tag = TYPE_CHAR,
    .n = 0
};

struct type int_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "int",
    .tag = TYPE_INT,
    .n = 0
};

struct type real_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "real",
    .tag = TYPE_REAL,
    .n = 0
};

struct type str_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "str",
    .tag = TYPE_STR,
    .n = 0
};

struct type* type_new_func(struct type *ret_type, struct type *param_types_head)
{
    struct type* func = nalloc(sizeof(*func));
    func->tag = TYPE_FUNC;

    return func;
}

struct type* type_new_class(const char *name)
{
    struct type* user_type = nalloc(sizeof(*user_type));
    user_type->tag = TYPE_CLASS;
    user_type->repr = name;

    return user_type;
}
