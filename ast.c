#include "ast.h"
#include "nolli.h"

#include <assert.h>

/* Convenience function for allocating nl_ast node
 * and setting its type.
 */
static void *make_node(struct nl_context* ctx, int tag, int lineno)
{
    struct nl_ast *node = nl_alloc(ctx, sizeof(*node));
    node->tag = tag;
    node->lineno = lineno;
    return node;
}

struct nl_ast *nl_ast_make_bool_lit(struct nl_context* ctx, bool b, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_BOOL_LIT, lineno);
    node->b = b;
    return node;
}

struct nl_ast *nl_ast_make_char_lit(struct nl_context* ctx, char c, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_CHAR_LIT, lineno);
    node->c = c;
    return node;
}

struct nl_ast *nl_ast_make_int_lit(struct nl_context* ctx, long l, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_INT_LIT, lineno);
    node->l = l;
    return node;
}

struct nl_ast *nl_ast_make_real_lit(struct nl_context* ctx, double d, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_REAL_LIT, lineno);
    node->d = d;
    return node;
}

struct nl_ast *nl_ast_make_str_lit(struct nl_context* ctx, nl_string_t s, int lineno)
{
    assert(s);
    struct nl_ast *node = make_node(ctx, NL_AST_STR_LIT, lineno);
    node->s = s;
    return node;
}

struct nl_ast *nl_ast_make_ident(struct nl_context* ctx, nl_string_t s, int lineno)
{
    assert(s);
    struct nl_ast *node = make_node(ctx, NL_AST_IDENT, lineno);
    node->s = s;
    return node;
}

struct nl_ast *nl_ast_make_using(struct nl_context* ctx, struct nl_ast *names, int lineno)
{
    assert(names);
    struct nl_ast *node = make_node(ctx, NL_AST_USING, lineno);
    node->usings.names = names;
    return node;
}

struct nl_ast *nl_ast_make_alias(struct nl_context* ctx, struct nl_ast *type, struct nl_ast *name, int lineno)
{
    assert(type);
    assert(name);
    struct nl_ast *node = make_node(ctx, NL_AST_ALIAS, lineno);
    node->alias.type = type;
    node->alias.name = name;

    return node;
}

struct nl_ast *nl_ast_make_tmpl_type(struct nl_context* ctx, struct nl_ast *name, struct nl_ast *tmpls, int lineno)
{
    assert(name);
    assert(tmpls);
    struct nl_ast *node = make_node(ctx, NL_AST_TMPL_TYPE, lineno);
    node->tmpl_type.name = name;
    node->tmpl_type.tmpls = tmpls;
    return node;
}

struct nl_ast *nl_ast_make_qual_type(struct nl_context* ctx, struct nl_ast *package, struct nl_ast *name, int lineno)
{
    assert(package);
    assert(name);
    struct nl_ast *node = make_node(ctx, NL_AST_QUAL_TYPE, lineno);
    node->qual_type.package = package;
    node->qual_type.name = name;
    return node;
}

struct nl_ast *nl_ast_make_func_type(struct nl_context* ctx, struct nl_ast *tmpl, struct nl_ast *ret_type,
        struct nl_ast *params, int lineno)
{
    assert(params);
    struct nl_ast *node = make_node(ctx, NL_AST_FUNC_TYPE, lineno);
    node->func_type.tmpl = tmpl;
    node->func_type.ret_type = ret_type;
    node->func_type.params = params;
    return node;
}

struct nl_ast *nl_ast_make_class(struct nl_context* ctx, struct nl_ast *name, struct nl_ast *tmpl,
        struct nl_ast *members, struct nl_ast *methods, int lineno)
{
    assert(name);
    assert(members);
    assert(methods);
    struct nl_ast *node = make_node(ctx, NL_AST_CLASS, lineno);
    node->classdef.name = name;
    node->classdef.tmpl = tmpl;
    node->classdef.members = members;
    node->classdef.methods = methods;
    return node;
}

struct nl_ast *nl_ast_make_interface(struct nl_context* ctx, struct nl_ast *name, struct nl_ast *methods, int lineno)
{
    assert(name);
    assert(methods);
    struct nl_ast *node = make_node(ctx, NL_AST_INTERFACE, lineno);
    node->interface.name = name;
    node->interface.methods = methods;
    return node;
}

struct nl_ast *nl_ast_make_decl(struct nl_context* ctx, int tp, struct nl_ast *type, struct nl_ast *rhs, int lineno)
{
    assert(type);

    struct nl_ast *node = make_node(ctx, NL_AST_DECL, lineno);
    node->decl.type = type;
    node->decl.rhs = rhs;
    node->decl.tp = tp;
    return node;
}

struct nl_ast *nl_ast_make_initialization(struct nl_context* ctx, struct nl_ast *ident, struct nl_ast *expr, int lineno)
{
    assert(ident);
    assert(expr);
    struct nl_ast *node = make_node(ctx, NL_AST_INIT, lineno);
    node->init.ident = ident;
    node->init.expr = expr;
    return node;
}

struct nl_ast *nl_ast_make_unexpr(struct nl_context* ctx, int op, struct nl_ast *expr, int lineno)
{
    assert(expr);
    struct nl_ast *node = make_node(ctx, NL_AST_UNEXPR, lineno);
    node->unexpr.op = op;
    node->unexpr.expr = expr;
    return node;
}

struct nl_ast *nl_ast_make_binexpr(struct nl_context* ctx, struct nl_ast *lhs, int op, struct nl_ast *rhs, int lineno)
{
    assert(lhs);
    assert(rhs);
    struct nl_ast *node = make_node(ctx, NL_AST_BINEXPR, lineno);
    node->binexpr.op = op;
    node->binexpr.lhs = lhs;
    node->binexpr.rhs = rhs;
    return node;
}

struct nl_ast *nl_ast_make_list(struct nl_context* ctx, int type, int lineno)
{
    struct nl_ast *node = make_node(ctx, type, lineno);
    node->list.head = NULL;
    node->list.tail = NULL;
    return node;
}

struct nl_ast *nl_ast_list_append(struct nl_ast *node, struct nl_ast *elem)
{
    assert(node);
    assert(elem);

    if (node->list.head == NULL || node->list.tail == NULL) {
        node->list.head = elem;

        /* in the case that `elem` is a list itself, make `tail` the tail of `elem` */
        struct nl_ast *prev = elem;
        while (elem) {
            prev = elem;
            elem = elem->next;
        }
        node->list.tail = prev;
    } else {
        assert(node->tag > NL_AST_LIST_SENTINEL ||
                NL_AST_LIST_LIT == node->tag || NL_AST_MAP_LIT == node->tag);
        assert(node->tag < NL_AST_LAST);
        node->list.tail->next = elem;
        node->list.tail = elem;
    }
    node->list.count++;
    return node;
}

struct nl_ast *nl_ast_make_keyval(struct nl_context* ctx, struct nl_ast *key, struct nl_ast *val, int lineno)
{
    assert(key);
    assert(val);
    struct nl_ast *node = make_node(ctx, NL_AST_KEYVAL, lineno);
    node->keyval.key = key;
    node->keyval.val = val;
    return node;
}

struct nl_ast *nl_ast_make_lookup(struct nl_context* ctx, struct nl_ast *container, struct nl_ast *index, int lineno)
{
    assert(container);
    assert(index);
    struct nl_ast *node = make_node(ctx, NL_AST_LOOKUP, lineno);
    node->lookup.container = container;
    node->lookup.index = index;
    return node;
}

struct nl_ast *nl_ast_make_bind(struct nl_context* ctx, struct nl_ast *ident, struct nl_ast *expr, int lineno)
{
    assert(ident);
    assert(expr);
    struct nl_ast *node = make_node(ctx, NL_AST_BIND, lineno);
    node->bind.ident = ident;
    node->bind.expr = expr;
    return node;
}

struct nl_ast *nl_ast_make_assignment(struct nl_context* ctx, struct nl_ast *lhs, int op, struct nl_ast *expr, int lineno)
{
    assert(lhs);
    assert(expr);

    struct nl_ast *node = make_node(ctx, NL_AST_ASSIGN, lineno);
    node->assignment.lhs = lhs;
    node->assignment.expr = expr;
    node->assignment.op = op;

    return node;
}

struct nl_ast *nl_ast_make_ifelse(struct nl_context* ctx, struct nl_ast *cond,
        struct nl_ast *if_body, struct nl_ast *else_body, int lineno)
{
    assert(cond);
    assert(if_body);

    struct nl_ast *node = make_node(ctx, NL_AST_IFELSE, lineno);
    node->ifelse.cond = cond;
    node->ifelse.if_body = if_body;
    node->ifelse.else_body = else_body;
    return node;
}

struct nl_ast *nl_ast_make_while(struct nl_context* ctx, struct nl_ast *cond, struct nl_ast *body, int lineno)
{
    assert(cond);
    assert(body);

    struct nl_ast *node = make_node(ctx, NL_AST_WHILE, lineno);
    node->while_loop.cond = cond;
    node->while_loop.body = body;
    return node;
}

struct nl_ast *nl_ast_make_for(struct nl_context* ctx, struct nl_ast *var, struct nl_ast *range, struct nl_ast *body, int lineno)
{
    assert(var);
    assert(range);
    assert(body);

    struct nl_ast *node = make_node(ctx, NL_AST_FOR, lineno);
    node->for_loop.var = var;
    node->for_loop.range = range;
    node->for_loop.body = body;
    return node;
}

struct nl_ast *nl_ast_make_call(struct nl_context* ctx, struct nl_ast *func, struct nl_ast *args, int lineno)
{
    assert(func);
    assert(args);

    struct nl_ast *node = make_node(ctx, NL_AST_CALL, lineno);
    node->call.func = func;
    node->call.args = args;
    return node;
}

struct nl_ast *nl_ast_make_return(struct nl_context* ctx, struct nl_ast *expr, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_RETURN, lineno);
    node->ret.expr = expr;
    return node;
}

struct nl_ast *nl_ast_make_break(struct nl_context* ctx, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_BREAK, lineno);
    return node;
}

struct nl_ast *nl_ast_make_continue(struct nl_context* ctx, int lineno)
{
    struct nl_ast *node = make_node(ctx, NL_AST_CONTINUE, lineno);
    return node;
}

struct nl_ast *nl_ast_make_function(struct nl_context* ctx, struct nl_ast *name, struct nl_ast *type,
        struct nl_ast *body, int lineno)
{
    assert(type);
    assert(body);

    struct nl_ast *node = make_node(ctx, NL_AST_FUNCTION, lineno);
    node->function.name = name;
    node->function.type = type;
    node->function.body = body;

    return node;
}

struct nl_ast *nl_ast_make_classlit(struct nl_context* ctx, struct nl_ast *tp, struct nl_ast *tmpl, struct nl_ast *items, int lineno)
{
    assert(tp);
    assert(items);

    struct nl_ast *node = make_node(ctx, NL_AST_CLASS_LIT, lineno);
    node->class_lit.type = tp;
    node->class_lit.tmpl = tmpl;
    node->class_lit.items = items;
    return node;
}

struct nl_ast *nl_ast_make_selector(struct nl_context* ctx, struct nl_ast *parent, struct nl_ast *child, int lineno)
{
    assert(parent);
    assert(child);

    struct nl_ast *node = make_node(ctx, NL_AST_SELECTOR, lineno);
    node->selector.parent = parent;
    node->selector.child = child;
    return node;
}

struct nl_ast *nl_ast_make_package_ref(struct nl_context* ctx, struct nl_ast *package, struct nl_ast *name, int lineno)
{
    assert(package);
    assert(name);
    struct nl_ast *node = make_node(ctx, NL_AST_PACKAGE_REF, lineno);
    node->package_ref.package = package;
    node->package_ref.name = name;
    return node;
}

struct nl_ast *nl_ast_make_package(struct nl_context* ctx, struct nl_ast *name, struct nl_ast *globals, int lineno)
{
    assert(name);
    assert(globals);

    struct nl_ast *node = make_node(ctx, NL_AST_PACKAGE, lineno);
    node->package.name = name;
    node->package.globals = globals;
    return node;
}

struct nl_ast *nl_ast_make_unit(struct nl_context* ctx, struct nl_ast *packages, int lineno)
{
    assert(packages);

    struct nl_ast *node = make_node(ctx, NL_AST_UNIT, lineno);
    node->unit.packages = packages;
    return node;
}

char *nl_ast_name(const struct nl_ast* node)
{
    static char *ast_names[] = {
        "first",

        "bool_lit",
        "char_lit",
        "int_lit",
        "real_lit",
        "str_lit",
        "list_lit",
        "map_lit",
        "class_lit",
        "ident",
        "unexpr",
        "binexpr",
        "call",
        "keyval",
        "lookup",
        "selector",
        "packageref",
        "function",

        "tmpl_type",
        "qual_type",
        "func_type",

        "decl",
        "init",
        "bind",
        "assign",
        "ifelse",
        "while",
        "for",
        "call_statement",
        "return",
        "break",
        "continue",

        "alias",
        "using",

        "class",
        "interface",
        "package",
        "unit",

        "list_sentinel",

        "list_idents",
        "list_types",
        "list_params",
        "list_args",
        "list_members",
        "list_methods",
        "list_method_decls",
        "list_class_inits",
        "list_decls",
        "list_statements",
        "list_usings",
        "list_globals",
        "list_packages",
        "list_units",

        "last"
    };

    assert(sizeof(ast_names) / sizeof(*ast_names) == NL_AST_LAST + 1);

    return ast_names[node->tag];
}
