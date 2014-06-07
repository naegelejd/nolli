#include "ast.h"


/* Convenience function for allocating AST node
 * and setting its type.
 */
static void *make_node(int tag)
{
    struct ast *node = nalloc(sizeof(*node));
    node->tag = tag;
    return node;
}

struct ast* ast_make_bool_lit(bool b)
{
    struct ast *node = make_node(AST_BOOL_LIT);
    node->b = b;
    return node;
}

struct ast* ast_make_char_lit(char c)
{
    struct ast *node = make_node(AST_CHAR_LIT);
    node->c = c;
    return node;
}

struct ast* ast_make_int_num(long l)
{
    struct ast *node = make_node(AST_INT_NUM);
    node->l = l;
    return node;
}

struct ast* ast_make_real_num(double d)
{
    struct ast *node = make_node(AST_REAL_NUM);
    node->d = d;
    return node;
}

struct ast* ast_make_str_lit(const char *s)
{
    struct ast *node = make_node(AST_STR_LIT);
    node->s = strdup(s);
    return node;
}

struct ast* ast_make_ident(const char *s)
{
    struct ast *node = make_node(AST_IDENT);
    node->s = strdup(s);
    return node;
}

struct ast* ast_make_import(struct ast* from, struct ast* modules)
{
    struct ast *node = make_node(AST_IMPORT);
    node->import.from = from;
    node->import.modules = modules;
    return node;
}

struct ast* ast_make_alias(struct ast* type, struct ast *name)
{
    struct ast *node = make_node(AST_ALIAS);
    node->alias.type = type;
    node->alias.name = name;
    return node;
}

struct ast* ast_make_list_type(struct ast *name)
{
    struct ast *node = make_node(AST_LIST_TYPE);
    node->list_type.name = name;
    return node;
}

struct ast* ast_make_map_type(struct ast *keyname, struct ast *valname)
{
    struct ast *node = make_node(AST_MAP_TYPE);
    node->map_type.keyname = keyname;
    node->map_type.valname = valname;
    return node;
}

struct ast* ast_make_func_type(struct ast *ret_type, struct ast *params)
{
    struct ast *node = make_node(AST_FUNC_TYPE);
    node->func_type.ret_type = ret_type;
    node->func_type.params = params;
    return node;
}

struct ast* ast_make_data(struct ast *name, struct ast *members)
{
    struct ast *node = make_node(AST_DATA);
    node->data.name = name;
    node->data.members = members;
    return node;
}

struct ast* ast_make_interface(struct ast *name, struct ast *methods)
{
    struct ast *node = make_node(AST_INTERFACE);
    node->interface.name = name;
    node->interface.methods = methods;
    return node;
}

struct ast* ast_make_decl(int tp, struct ast* type, struct ast* name_s)
{
    struct ast *node = make_node(AST_DECL);
    node->decl.type = type;
    node->decl.name_s = name_s;
    node->decl.tp = tp;
    return node;
}

struct ast* ast_make_initialization(struct ast *ident, struct ast *expr)
{
    struct ast *node = make_node(AST_INIT);
    node->init.ident = ident;
    node->init.expr = expr;
    return node;
}

struct ast* ast_make_unexpr(int op, struct ast* expr)
{
    struct ast *node = make_node(AST_UNEXPR);
    node->unexpr.op = op;
    node->unexpr.expr = expr;
    return node;
}

struct ast* ast_make_binexpr(struct ast* lhs, int op, struct ast* rhs)
{
    struct ast *node = make_node(AST_BINEXPR);
    node->binexpr.op = op;
    node->binexpr.lhs = lhs;
    node->binexpr.rhs = rhs;
    return node;
}

struct ast *ast_make_list(int type)
{
    struct ast *node = make_node(AST_LIST);
    node->list.type = type;
    node->list.head = NULL;
    node->list.tail = NULL;
    return node;
}

struct ast *ast_list_append(struct ast* node, struct ast* elem)
{
    assert(node != NULL);

    if (node->list.head == NULL || node->list.tail == NULL) {
        node->list.head = elem;
        node->list.tail = elem;
    } else {
        assert(node->tag == AST_LIST);
        node->list.tail->next = elem;
        node->list.tail = elem;
    }
    node->list.count++;
    return node;
}

struct ast* ast_make_keyval(struct ast* key, struct ast* val)
{
    struct ast *node = make_node(AST_KEYVAL);
    node->keyval.key = key;
    node->keyval.val = val;
    return node;
}

struct ast* ast_make_lookup(struct ast* container, struct ast* index)
{
    struct ast* node = make_node(AST_LOOKUP);
    node->lookup.container = container;
    node->lookup.index = index;
    return node;
}

struct ast *ast_make_short_decl(struct ast *ident, struct ast *expr)
{
    struct ast* node = make_node(AST_SHORT_DECL);
    node->short_decl.ident = ident;
    node->short_decl.expr = expr;
    return node;
}

struct ast* ast_make_assignment(struct ast* ident, int op, struct ast* expr)
{
    struct ast *node = make_node(AST_ASSIGN);
    node->assignment.ident = ident;
    node->assignment.expr = expr;
    node->assignment.op = op;

    return node;
}

struct ast* ast_make_ifelse(struct ast* cond,
        struct ast* if_body, struct ast* else_body)
{
    struct ast *node = make_node(AST_IFELSE);
    node->ifelse.cond = cond;
    node->ifelse.if_body = if_body;
    node->ifelse.else_body = else_body;
    return node;
}

struct ast* ast_make_while(struct ast* cond, struct ast* body)
{
    struct ast *node = make_node(AST_WHILE);
    node->while_loop.cond = cond;
    node->while_loop.body = body;
    return node;
}

struct ast* ast_make_for(struct ast* var, struct ast* range, struct ast* body)
{
    struct ast *node = make_node(AST_FOR);
    node->for_loop.var = var;
    node->for_loop.range = range;
    node->for_loop.body = body;
    return node;
}

struct ast* ast_make_call(struct ast* func, struct ast* args)
{
    struct ast *node = make_node(AST_CALL);
    node->call.func = func;
    node->call.args = args;
    return node;
}

struct ast* ast_make_return(struct ast* expr)
{
    struct ast *node = make_node(AST_RETURN);
    node->ret.expr = expr;
    return node;
}

struct ast* ast_make_break(void)
{
    struct ast *node = make_node(AST_BREAK);
    return node;
}

struct ast* ast_make_continue(void)
{
    struct ast *node = make_node(AST_CONTINUE);
    return node;
}

struct ast* ast_make_function(struct ast *name, struct ast *type,
        struct ast *body)
{
    struct ast *node = make_node(AST_FUNCTION);
    node->function.name = name;
    node->function.type = type;
    node->function.body = body;

    return node;
}

struct ast* ast_make_datalit(struct ast *name, struct ast *items)
{
    struct ast *node = make_node(AST_DATALIT);
    node->datalit.name = name;
    node->datalit.items = items;
    return node;
}

struct ast* ast_make_selector(struct ast *parent, struct ast *child)
{
    struct ast *node = make_node(AST_SELECTOR);
    node->selector.parent = parent;
    node->selector.child = child;
    return node;
}

struct ast* ast_make_methods(struct ast *name, struct ast *methods)
{
    struct ast *node = make_node(AST_METHODS);
    node->methods.name = name;
    node->methods.methods = methods;
    return node;
}

struct ast* ast_make_program(struct ast *package, struct ast *definitions)
{
    struct ast *node = make_node(AST_PROGRAM);
    node->program.package = package;
    node->program.definitions = definitions;
    return node;
}
