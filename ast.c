#include "ast.h"



/* Convenience function for allocating AST node
 * and setting its type.
 */
static void *make_node(size_t size, int type)
{
    struct ast *node = nalloc(size);
    node->type = type;
    return (void*)node;
}

struct ast* ast_make_bool_lit(bool b)
{
    struct ast_bool *node = make_node(sizeof(*node), AST_BOOL_LIT);
    node->b = b;
    return (struct ast*)node;
}

struct ast* ast_make_char_lit(char c)
{
    struct ast_char *node = make_node(sizeof(*node), AST_CHAR_LIT);
    node->c = c;
    return (struct ast*)node;
}

struct ast* ast_make_int_num(long l)
{
    struct ast_int *node = make_node(sizeof(*node), AST_INT_NUM);
    node->l = l;
    return (struct ast*)node;
}

struct ast* ast_make_real_num(double d)
{
    struct ast_real *node = make_node(sizeof(*node), AST_REAL_NUM);
    node->d = d;
    return (struct ast*)node;
}

struct ast* ast_make_str_lit(const char *s)
{
    assert(s);

    struct ast_str *node = make_node(sizeof(*node), AST_STR_LIT);
    node->s = strdup(s);
    return (struct ast*)node;
}

struct ast* ast_make_ident(const char *s)
{
    assert(s);

    struct ast_ident *ident = make_node(sizeof(*ident), AST_IDENT);
    ident->s = strdup(s);
    return (struct ast*)ident;
}

struct ast* ast_make_import(struct ast* from, struct ast* modules)
{
    struct ast_import* import = make_node(sizeof(*import), AST_IMPORT);
    import->from = from;
    import->modules = modules;
    return (struct ast*)import;
}

struct ast* ast_make_alias(struct ast* type, struct ast* name)
{
    struct ast_alias* alias = make_node(sizeof(*alias), AST_ALIAS);
    alias->type = type;
    alias->name = name;
    return (struct ast*)alias;
}

struct ast* ast_make_list_type(struct ast *name)
{
    struct ast_list_type *type = make_node(sizeof(*type), AST_LIST_TYPE);
    type->name = name;
    return (struct ast*)type;
}

struct ast* ast_make_map_type(struct ast *keyname, struct ast *valname)
{
    struct ast_map_type *type = make_node(sizeof(*type), AST_MAP_TYPE);
    type->keyname = keyname;
    type->valname = valname;
    return (struct ast*)type;
}

struct ast* ast_make_func_type(struct ast *ret_type, struct ast *param_types)
{
    struct ast_func_type *type = make_node(sizeof(*type), AST_FUNC_TYPE);
    type->ret_type = ret_type;
    type->param_types = param_types;
    return (struct ast*)type;
}

struct ast* ast_make_struct_type(struct ast *name, struct ast *members)
{
    struct ast_struct_type *type = make_node(sizeof(*type), AST_STRUCT_TYPE);
    type->name = name;
    type->members = members;
    return (struct ast*)type;
}

struct ast* ast_make_iface_type(struct ast *name, struct ast *methods)
{
    struct ast_iface_type *type = make_node(sizeof(*type), AST_IFACE_TYPE);
    type->name = name;
    type->methods = methods;
    return (struct ast*)type;
}

struct ast* ast_make_decl(int tp, struct ast* type, struct ast* name_s)
{
    struct ast_decl *decl = make_node(sizeof(*decl), AST_DECL);
    decl->type = type;
    decl->name_s = name_s;
    decl->tp = tp;
    return (struct ast*)decl;
}

struct ast* ast_make_initialization(struct ast *ident, struct ast *expr)
{
    struct ast_init *init = make_node(sizeof(*init), AST_INIT);
    init->ident = ident;
    init->expr = expr;
    return (struct ast*)init;
}

struct ast* ast_make_unexpr(int op, struct ast* expr)
{
    assert(expr);

    struct ast_unexpr* unexpr = make_node(sizeof(*unexpr), AST_UNEXPR);
    unexpr->op = op;
    unexpr->expr = expr;
    return (struct ast*)unexpr;
}

struct ast* ast_make_binexpr(struct ast* lhs, int op, struct ast* rhs)
{
    assert(lhs);
    assert(rhs);

    struct ast_binexpr* binexpr = make_node(sizeof(*binexpr), AST_BINEXPR);
    binexpr->op = op;
    binexpr->lhs = lhs;
    binexpr->rhs = rhs;
    return (struct ast*)binexpr;
}

struct ast *ast_make_list(int type)
{
    struct ast_list *list = make_node(sizeof(*list), AST_LIST);
    list->type = type;
    /* TODO: this could differ based on list type..
     * e.g. statement lists allocate for 8 statements
     *      list literals only allocate for 4, etc.
     */
    list->alloc = 8;
    list->count = 0;
    list->items = nalloc(list->alloc * sizeof(*list->items));

    return (struct ast*)list;
}

struct ast *ast_list_append(struct ast* node, struct ast* item)
{
    assert(node);
    assert(item);
    struct ast_list* list = (struct ast_list*)node;
    if (list->count >= list->alloc) {
        list->alloc *= 2;
        list->items = nrealloc(list->items,
                list->alloc * sizeof(*list->items));
    }
    list->items[list->count++] = item;

    return (struct ast*)list;
}

struct ast* ast_make_keyval(struct ast* key, struct ast* val)
{
    struct ast_keyval* keyval = make_node(sizeof(*keyval), AST_KEYVAL);
    keyval->key = key;
    keyval->val = val;
    return (struct ast*)keyval;
}

struct ast* ast_make_contaccess(struct ast* ident, struct ast* index)
{
    assert(ident);
    assert(index);

    struct ast_contaccess* contaccess = make_node(
            sizeof(*contaccess), AST_CONTACCESS);
    contaccess->ident = ident;
    contaccess->index = index;

    return (struct ast*)contaccess;
}

struct ast *ast_make_short_decl(struct ast *ident, struct ast *expr)
{
    assert(ident);
    assert(expr);

    struct ast_short_decl* short_decl = make_node(
            sizeof(*short_decl), AST_SHORT_DECL);
    short_decl->ident = ident;
    short_decl->expr = expr;

    return (struct ast*)short_decl;
}

struct ast* ast_make_assignment(struct ast* ident, int op, struct ast* expr)
{
    assert(ident);
    assert(expr);

    struct ast_assignment* assignment = make_node(sizeof(*assignment), AST_ASSIGN);
    assignment->ident = ident;
    assignment->expr = expr;
    assignment->op = op;

    return (struct ast*)assignment;
}

struct ast* ast_make_ifelse(struct ast* cond,
        struct ast* if_body, struct ast* else_body)
{
    struct ast_ifelse* ifelse = make_node(sizeof(*ifelse), AST_IFELSE);
    ifelse->cond = cond;
    ifelse->if_body = if_body;
    ifelse->else_body = else_body;
    return (struct ast*)ifelse;
}

struct ast* ast_make_while(struct ast* cond, struct ast* body)
{
    struct ast_while* wile = make_node(sizeof(*wile), AST_WHILE);
    wile->cond = cond;
    wile->body = body;
    return (struct ast*)wile;
}

struct ast* ast_make_for(struct ast* var, struct ast* range, struct ast* body)
{
    struct ast_for* fore = make_node(sizeof(*fore), AST_FOR);
    fore->var = var;
    fore->range = range;
    fore->body = body;
    return (struct ast*)fore;
}

struct ast* ast_make_call(struct ast* func, struct ast* args)
{
    assert(func);
    assert(args);

    struct ast_call *call = make_node(sizeof(*call), AST_CALL);
    call->func = func;
    call->args = args;
    return (struct ast*)call;
}

struct ast* ast_make_return(struct ast* expr)
{
    struct ast_return *ret = make_node(sizeof(*ret), AST_RETURN);
    ret->expr = expr;
    return (struct ast*)ret;
}

struct ast* ast_make_break(void)
{
    struct ast_break *brk = make_node(sizeof(*brk), AST_BREAK);
    return (struct ast*)brk;
}

struct ast* ast_make_continue(void)
{
    struct ast_cont *cont = make_node(sizeof(*cont), AST_CONTINUE);
    return (struct ast*)cont;
}

struct ast* ast_make_funclit(struct ast *ret_type, struct ast *params,
        struct ast *body)
{
    struct ast_funclit *f = make_node(sizeof(*f), AST_FUNCLIT);
    f->ret_type = ret_type;
    f->params = params;
    f->body = body;

    return (struct ast*)f;
}
