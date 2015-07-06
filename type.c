#include "type.h"
#include "nolli.h"

#include <stdlib.h>

struct nl_type nl_bool_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "bool",
    .tag = NL_TYPE_BOOL,
    .n = 0
};

struct nl_type nl_char_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "char",
    .tag = NL_TYPE_CHAR,
    .n = 0
};

struct nl_type nl_int_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "int",
    .tag = NL_TYPE_INT,
    .n = 0
};

struct nl_type nl_real_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "real",
    .tag = NL_TYPE_REAL,
    .n = 0
};

struct nl_type nl_str_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "str",
    .tag = NL_TYPE_STR,
    .n = 0
};

struct nl_type nl_tmpl_placeholder_type = {
    .func = {NULL},
    .next = NULL,
    .repr = "tmpl_placeholder",
    .tag = NL_TYPE_TMPL_PLACEHOLDER,
    .n = 0
};

struct nl_type* nl_type_new_func(struct nl_context* ctx, struct nl_type *ret_type,
        struct nl_type *param_types_head, unsigned int count)
{
    struct nl_type* func = nl_alloc(ctx, sizeof(*func));
    func->tag = NL_TYPE_FUNC;

    func->func.ret_type = ret_type;
    func->func.param_types_head = param_types_head;
    func->func.param_count = count;

    return func;
}

struct nl_type* nl_type_new_class(struct nl_context* ctx, const char *name,
        struct nl_symtable *tmpls, struct nl_symtable *members,
        struct nl_symtable *methods)
{
    struct nl_type* user_type = nl_alloc(ctx, sizeof(*user_type));
    user_type->tag = NL_TYPE_CLASS;
    user_type->repr = name;

    user_type->clss.tmpl_types = tmpls;
    user_type->clss.member_types = members;
    user_type->clss.method_types = methods;

    return user_type;
}

struct nl_type* nl_type_new_interface(struct nl_context* ctx,
        const char *name, struct nl_symtable *methods)
{
    struct nl_type* user_type = nl_alloc(ctx, sizeof(*user_type));
    user_type->tag = NL_TYPE_INTERFACE;
    user_type->repr = name;

    user_type->interface.method_types = methods;

    return user_type;
}

struct nl_type* nl_type_new_reference(struct nl_context* ctx,
        nl_string_t package_name, nl_string_t type_name)
{
    struct nl_type* tp = nl_alloc(ctx, sizeof(*tp));
    tp->tag = NL_TYPE_REFERENCE;
    tp->repr = "reference";  /* FIXME */

    tp->reference.package_name = package_name;
    tp->reference.type_name = type_name;

    return tp;
}

bool nl_types_equal(struct nl_type *tp1, struct nl_type *tp2)
{
    /* FIXME: actually compare types */
    if (tp1 == tp2) {
        return true;
    }
    return false;
}
