#include "ast.h"


/* Convenience function for allocating AST node
 * and setting its type.
 */
static void *make_node(int tag, int lineno)
{
    struct ast *node = nalloc(sizeof(*node));
    node->tag = tag;
    node->lineno = lineno;
    return node;
}

struct ast* ast_make_bool_lit(bool b, int lineno)
{
    struct ast *node = make_node(AST_BOOL_LIT, lineno);
    node->b = b;
    return node;
}

struct ast* ast_make_char_lit(char c, int lineno)
{
    struct ast *node = make_node(AST_CHAR_LIT, lineno);
    node->c = c;
    return node;
}

struct ast* ast_make_int_num(long l, int lineno)
{
    struct ast *node = make_node(AST_INT_NUM, lineno);
    node->l = l;
    return node;
}

struct ast* ast_make_real_num(double d, int lineno)
{
    struct ast *node = make_node(AST_REAL_NUM, lineno);
    node->d = d;
    return node;
}

struct ast* ast_make_str_lit(struct string *s, int lineno)
{
    assert(s);
    struct ast *node = make_node(AST_STR_LIT, lineno);
    node->s = s;
    return node;
}

struct ast* ast_make_ident(struct string *s, int lineno)
{
    assert(s);
    struct ast *node = make_node(AST_IDENT, lineno);
    node->s = s;
    return node;
}

struct ast* ast_make_import(struct ast* from, struct ast* modules, int lineno)
{
    assert(modules);
    struct ast *node = make_node(AST_IMPORT, lineno);
    node->import.from = from;
    node->import.modules = modules;
    return node;
}

struct ast* ast_make_alias(struct ast* type, struct ast *name, int lineno)
{
    assert(type);
    assert(name);
    struct ast *node = make_node(AST_ALIAS, lineno);
    node->alias.type = type;
    node->alias.name = name;
    return node;
}

struct ast *ast_make_tmpl_type(struct ast *name, struct ast *tmpls, int lineno)
{
    assert(name);
    assert(tmpls);
    struct ast *node = make_node(AST_TMPL_TYPE, lineno);
    node->tmpl_type.name = name;
    node->tmpl_type.tmpls = tmpls;
    return node;
}

struct ast *ast_make_qual_type(struct ast *package, struct ast *name, int lineno)
{
    assert(package);
    assert(name);
    struct ast *node = make_node(AST_QUAL_TYPE, lineno);
    node->qual_type.package = package;
    node->qual_type.name = name;
    return node;
}

struct ast* ast_make_func_type(struct ast *ret_type, struct ast *params, int lineno)
{
    assert(params);
    struct ast *node = make_node(AST_FUNC_TYPE, lineno);
    node->func_type.ret_type = ret_type;
    node->func_type.params = params;
    return node;
}

struct ast* ast_make_class(struct ast *name, struct ast *tmpl, struct ast *members, struct ast *methods, int lineno)
{
    assert(name);
    assert(members);
    assert(methods);
    struct ast *node = make_node(AST_CLASS, lineno);
    node->classdef.name = name;
    node->classdef.tmpl = tmpl;
    node->classdef.members = members;
    node->classdef.methods = methods;
    return node;
}

struct ast* ast_make_interface(struct ast *name, struct ast *methods, int lineno)
{
    assert(name);
    assert(methods);
    struct ast *node = make_node(AST_INTERFACE, lineno);
    node->interface.name = name;
    node->interface.methods = methods;
    return node;
}

struct ast* ast_make_decl(int tp, struct ast* type, struct ast* rhs, int lineno)
{
    assert(type);

    struct ast *node = make_node(AST_DECL, lineno);
    node->decl.type = type;
    node->decl.rhs = rhs;
    node->decl.tp = tp;
    return node;
}

struct ast* ast_make_initialization(struct ast *ident, struct ast *expr, int lineno)
{
    assert(ident);
    assert(expr);
    struct ast *node = make_node(AST_INIT, lineno);
    node->init.ident = ident;
    node->init.expr = expr;
    return node;
}

struct ast* ast_make_unexpr(int op, struct ast* expr, int lineno)
{
    assert(expr);
    struct ast *node = make_node(AST_UNEXPR, lineno);
    node->unexpr.op = op;
    node->unexpr.expr = expr;
    return node;
}

struct ast* ast_make_binexpr(struct ast* lhs, int op, struct ast* rhs, int lineno)
{
    assert(lhs);
    assert(rhs);
    struct ast *node = make_node(AST_BINEXPR, lineno);
    node->binexpr.op = op;
    node->binexpr.lhs = lhs;
    node->binexpr.rhs = rhs;
    return node;
}

struct ast *ast_make_list(int type, int lineno)
{
    struct ast *node = make_node(type, lineno);
    node->list.head = NULL;
    node->list.tail = NULL;
    return node;
}

struct ast *ast_list_append(struct ast* node, struct ast* elem)
{
    assert(node);
    assert(elem);

    if (node->list.head == NULL || node->list.tail == NULL) {
        node->list.head = elem;
        node->list.tail = elem;
    } else {
        assert(node->tag > AST_LIST_SENTINEL);
        node->list.tail->next = elem;
        node->list.tail = elem;
    }
    node->list.count++;
    return node;
}

struct ast* ast_make_keyval(struct ast* key, struct ast* val, int lineno)
{
    assert(key);
    assert(val);
    struct ast *node = make_node(AST_KEYVAL, lineno);
    node->keyval.key = key;
    node->keyval.val = val;
    return node;
}

struct ast* ast_make_lookup(struct ast* container, struct ast* index, int lineno)
{
    assert(container);
    assert(index);
    struct ast* node = make_node(AST_LOOKUP, lineno);
    node->lookup.container = container;
    node->lookup.index = index;
    return node;
}

struct ast *ast_make_bind(struct ast *ident, struct ast *expr, int lineno)
{
    assert(ident);
    assert(expr);
    struct ast* node = make_node(AST_BIND, lineno);
    node->bind.ident = ident;
    node->bind.expr = expr;
    return node;
}

struct ast* ast_make_assignment(struct ast* lhs, int op, struct ast* expr, int lineno)
{
    assert(lhs);
    assert(expr);

    struct ast *node = make_node(AST_ASSIGN, lineno);
    node->assignment.lhs = lhs;
    node->assignment.expr = expr;
    node->assignment.op = op;

    return node;
}

struct ast* ast_make_ifelse(struct ast* cond,
        struct ast* if_body, struct ast* else_body, int lineno)
{
    assert(cond);
    assert(if_body);

    struct ast *node = make_node(AST_IFELSE, lineno);
    node->ifelse.cond = cond;
    node->ifelse.if_body = if_body;
    node->ifelse.else_body = else_body;
    return node;
}

struct ast* ast_make_while(struct ast* cond, struct ast* body, int lineno)
{
    assert(cond);
    assert(body);

    struct ast *node = make_node(AST_WHILE, lineno);
    node->while_loop.cond = cond;
    node->while_loop.body = body;
    return node;
}

struct ast* ast_make_for(struct ast* var, struct ast* range, struct ast* body, int lineno)
{
    assert(var);
    assert(range);
    assert(body);

    struct ast *node = make_node(AST_FOR, lineno);
    node->for_loop.var = var;
    node->for_loop.range = range;
    node->for_loop.body = body;
    return node;
}

struct ast* ast_make_call(struct ast* func, struct ast* args, int lineno)
{
    assert(func);
    assert(args);

    struct ast *node = make_node(AST_CALL, lineno);
    node->call.func = func;
    node->call.args = args;
    return node;
}

struct ast* ast_make_return(struct ast* expr, int lineno)
{
    struct ast *node = make_node(AST_RETURN, lineno);
    node->ret.expr = expr;
    return node;
}

struct ast* ast_make_break(int lineno)
{
    struct ast *node = make_node(AST_BREAK, lineno);
    return node;
}

struct ast* ast_make_continue(int lineno)
{
    struct ast *node = make_node(AST_CONTINUE, lineno);
    return node;
}

struct ast* ast_make_function(struct ast *name, struct ast *type,
        struct ast *body, int lineno)
{
    assert(type);
    assert(body);

    struct ast *node = make_node(AST_FUNCTION, lineno);
    node->function.name = name;
    node->function.type = type;
    node->function.body = body;

    return node;
}

struct ast* ast_make_classlit(struct ast *name, struct ast *tmpl, struct ast *items, int lineno)
{
    assert(name);
    assert(items);

    struct ast *node = make_node(AST_CLASSLIT, lineno);
    node->classlit.name = name;
    node->classlit.tmpl = tmpl;
    node->classlit.items = items;
    return node;
}

struct ast* ast_make_selector(struct ast *parent, struct ast *child, int lineno)
{
    assert(parent);
    assert(child);

    struct ast *node = make_node(AST_SELECTOR, lineno);
    node->selector.parent = parent;
    node->selector.child = child;
    return node;
}

struct ast *ast_make_package_ref(struct ast *package, struct ast *expr, int lineno)
{
    assert(package);
    assert(expr);
    struct ast *node = make_node(AST_PACKAGE_REF, lineno);
    node->package_ref.package = package;
    node->package_ref.expr = expr;
    return node;
}

struct ast* ast_make_unit(struct ast *package, struct ast *globals, int lineno)
{
    assert(package);
    assert(globals);

    struct ast *node = make_node(AST_UNIT, lineno);
    node->unit.package = package;
    node->unit.globals = globals;
    return node;
}
