#include "type.h"

struct type bool_type = {
    TYPE_BOOL,
    "bool",
    0,
    0
};

struct type char_type = {
    TYPE_CHAR,
    "char",
    0,
    0
};

struct type int_type = {
    TYPE_INT,
    "int",
    0,
    0
};

struct type real_type = {
    TYPE_REAL,
    "real",
    0,
    0
};

struct type str_type = {
    TYPE_STR,
    "str",
    0,
    0
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
