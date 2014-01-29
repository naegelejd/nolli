#include "nolli.h"

static struct ast* create_node(ast_type_t type)
{
    struct ast* a = nalloc(sizeof(*a));
    a->type = type;
    return a;
}

/* Convenience function for allocating AST node
 * and setting its type.
 */
static void *make_node(size_t size, ast_type_t type)
{
    struct ast *node = nalloc(size);
    node->type = type;
    return (void*)node;
}

struct ast* ast_make_bool_lit(bool b)
{
    struct ast* bn = create_node(AST_BOOL_LIT);
    return bn;
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

struct ast* ast_make_typedef(struct ast* type, struct ast* alias)
{
    struct ast_typedef* tdef = make_node(sizeof(*tdef), AST_TYPEDEF);
    tdef->type = type;
    tdef->alias = alias;
    return (struct ast*)tdef;
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

struct ast* ast_make_decl(decl_type_t tp, struct ast* type, struct ast* name_s)
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

struct ast* ast_make_unexpr(expr_op_t op, struct ast* expr)
{
    assert(expr);

    struct ast_unexpr* unexpr = make_node(sizeof(*unexpr), AST_UNEXPR);
    unexpr->op = op;
    unexpr->expr = expr;
    return (struct ast*)unexpr;
}

struct ast* ast_make_binexpr(struct ast* lhs, expr_op_t op, struct ast* rhs)
{
    assert(lhs);
    assert(rhs);

    struct ast_binexpr* binexpr = make_node(sizeof(*binexpr), AST_BINEXPR);
    binexpr->op = op;
    binexpr->lhs = lhs;
    binexpr->rhs = rhs;
    return (struct ast*)binexpr;
}

struct ast *ast_make_list(list_type_t type)
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

struct ast* ast_make_assignment(struct ast* ident, assign_op_t op, struct ast* expr)
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

struct ast* ast_make_funcdef(struct ast *ret_type, struct ast *name,
        struct ast *params, struct ast *body)
{
    struct ast_funcdef *f = make_node(sizeof(*f), AST_FUNCDEF);
    f->ret_type = ret_type;
    f->name = name;
    f->params = params;
    f->body = body;

    return (struct ast*)f;
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

static char *ast_name(struct ast* node)
{
    static char *names[] = {
        "BAD_TYPE",
        "BOOL_LIT",
        "CHAR_LIT",
        "INT_NUM",
        "REAL_NUM",
        "STR_LIT",
        "IDENT",
        "IMPORT",
        "TYPEDEF",
        "LIST_TYPE",
        "MAP_TYPE",
        "FUNC_TYPE",
        "STRUCT_TYPE",
        "IFACE_TYPE",
        "DECL",
        "INIT",
        "UNEXPR",
        "BINEXPR",
        "LIST",
        "KEYVAL",
        "CONTACCESS",
        "ASSIGN",
        "IFELSE",
        "WHILE",
        "FOR",
        "CALL",
        "FUNCLIT",
        "FUNCDEF",
        "STRUCT",
        "RETURN",
        "BREAK",
        "CONTINUE",
    };

    static char *list_names[] = {
        "LIST_DECLS",
        "LIST_ARGS",
        "LIST_PARAMS",
        "LIST_TYPES",
        "LIST_LITERAL",
        "LIST_IMPORTS",
        "LIST_MAP_ITEMS",
        "LIST_SELECTORS",
        "LIST_MEMBERS",
        "LIST_METHODS",
        "LIST_STATEMENTS",
    };

    if (node->type == AST_LIST) {
        struct ast_list* list = (struct ast_list*)node;
        return list_names[list->type];
    }
    return names[node->type];
}

typedef void (*visitor) (struct ast*);
typedef void (*walker) (struct ast*, visitor);

void visit(struct ast* node)
{
    printf("%s\n", ast_name(node));
}

static void walk_int_num(struct ast *node, visitor v);
static void walk_str_lit(struct ast *node, visitor v);
static void walk_ident(struct ast *node, visitor v);
static void walk_unexpr(struct ast *node, visitor v);
static void walk_binexpr(struct ast *node, visitor v);
static void walk_list(struct ast *node, visitor v);
static void walk_keyval(struct ast *node, visitor v);
static void walk_assign(struct ast *node, visitor v);
static void walk_call(struct ast *node, visitor v);
static void walk_import(struct ast *node, visitor v);
static void walk_list_type(struct ast *node, visitor v);
static void walk_map_type(struct ast *node, visitor v);
static void walk_func_type(struct ast *node, visitor v);
static void walk_struct_type(struct ast *node, visitor v);
static void walk_iface_type(struct ast *node, visitor v);
static void walk_decl(struct ast *node, visitor v);
static void walk_initialization(struct ast *node, visitor v);
static void walk_ifelse(struct ast *node, visitor v);
static void walk_while(struct ast *node, visitor v);
static void walk_for(struct ast *node, visitor v);
static void walk_typedef(struct ast *node, visitor v);
static void walk_return(struct ast *node, visitor v);
static void walk_break(struct ast *node, visitor v);
static void walk_continue(struct ast *node, visitor v);
static void walk_contaccess(struct ast *node, visitor v);
static void walk_funclit(struct ast *node, visitor v);
static void walk_funcdef(struct ast *node, visitor v);

void walk(struct ast* root)
{
    static walker walkers[] = {
        NULL, /* not a valid AST node */
        NULL, /* walk_bool_lit, */
        NULL, /* walk_char_lit, */
        walk_int_num,
        NULL, /* walk_real_num, */
        walk_str_lit,

        walk_ident,

        walk_import,
        walk_typedef,

        walk_list_type,
        walk_map_type,
        walk_func_type,
        walk_struct_type,
        walk_iface_type,

        walk_decl,
        walk_initialization,

        walk_unexpr,
        walk_binexpr,
        walk_list,

        walk_keyval,
        walk_contaccess,

        walk_assign,
        walk_ifelse,
        walk_while,
        walk_for,
        walk_call,
        walk_funclit,
        walk_funcdef,
        NULL, /* walk_struct, */

        walk_return,
        walk_break,
        walk_continue,
    };

    visitor v = visit;

    walker w = walkers[root->type];
    if (w) w(root, v);
}


static void walk_int_num(struct ast *node, visitor v)
{
    v(node);
}

static void walk_str_lit(struct ast *node, visitor v)
{
    v(node);
}

static void walk_ident(struct ast *node, visitor v)
{
    v(node);
}

static void walk_unexpr(struct ast *node, visitor v)
{
    struct ast_unexpr* unexpr = (struct ast_unexpr*)node;
    walk(unexpr->expr);
    v(node);
}

static void walk_binexpr(struct ast *node, visitor v)
{
    struct ast_binexpr* binexpr = (struct ast_binexpr*)node;
    walk(binexpr->lhs);
    walk(binexpr->rhs);
    v(node);
}

static void walk_list(struct ast *node, visitor v)
{
    struct ast_list* list = (struct ast_list*)node;
    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i]);
    }
    v(node);
}

static void walk_keyval(struct ast *node, visitor v)
{
    struct ast_keyval* keyval = (struct ast_keyval*)node;
    walk(keyval->key);
    walk(keyval->val);
    v(node);
}

static void walk_assign(struct ast *node, visitor v)
{
    struct ast_assignment* assignment = (struct ast_assignment*)node;
    walk(assignment->ident);
    walk(assignment->expr);
    v(node);
}

static void walk_call(struct ast *node, visitor v)
{
    struct ast_call* call = (struct ast_call*)node;
    walk(call->func);
    walk(call->args);
    v(node);
}

static void walk_import(struct ast *node, visitor v)
{
    struct ast_import* import = (struct ast_import*)node;
    if (import->from) {
        assert(import->from);
        walk(import->from);
    }

    assert(import->modules);
    walk(import->modules);

    v(node);
}

static void walk_list_type(struct ast *node, visitor v)
{
    struct ast_list_type *type = (struct ast_list_type*)node;
    walk(type->name);
    v(node);
}

static void walk_map_type(struct ast *node, visitor v)
{
    struct ast_map_type *type = (struct ast_map_type*)node;
    walk(type->keyname);
    walk(type->valname);
    v(node);
}

static void walk_func_type(struct ast *node, visitor v)
{
    struct ast_func_type *type = (struct ast_func_type*)node;
    if (type->ret_type) {
        walk(type->ret_type);
    }
    if (type->param_types) {
        walk(type->param_types);
    }
    v(node);
}

static void walk_struct_type(struct ast *node, visitor v)
{
    struct ast_struct_type *type = (struct ast_struct_type*)node;
    walk(type->name);
    walk(type->members);
    v(node);
}

static void walk_iface_type(struct ast *node, visitor v)
{
    struct ast_iface_type *type = (struct ast_iface_type*)node;
    walk(type->name);
    walk(type->methods);
    v(node);
}

static void walk_decl(struct ast *node, visitor v)
{
    struct ast_decl *decl = (struct ast_decl *)node;
    walk(decl->type);
    walk(decl->name_s);
    v(node);
}

static void walk_initialization(struct ast *node, visitor v)
{
    struct ast_init *init = (struct ast_init *)node;
    walk(init->ident);
    walk(init->expr);
    v(node);
}

static void walk_ifelse(struct ast *node, visitor v)
{
    struct ast_ifelse *ifelse = (struct ast_ifelse *)node;
    walk(ifelse->cond);
    walk(ifelse->if_body);
    if (ifelse->else_body) {
        walk(ifelse->else_body);
    }
    v(node);
}

static void walk_while(struct ast *node, visitor v)
{
    struct ast_while *wile = (struct ast_while *)node;
    walk(wile->cond);
    walk(wile->body);
    v(node);
}

static void walk_for(struct ast *node, visitor v)
{
    struct ast_for *fore = (struct ast_for *)node;
    walk(fore->var);
    walk(fore->range);
    walk(fore->body);
    v(node);
}

static void walk_typedef(struct ast *node, visitor v)
{
    struct ast_typedef *tdef = (struct ast_typedef*)node;
    walk(tdef->type);
    walk(tdef->alias);
    v(node);
}

static void walk_return(struct ast *node, visitor v)
{
    struct ast_return *ret = (struct ast_return*)node;
    if (ret->expr) {
        walk(ret->expr);
    }
    v(node);
}

static void walk_break(struct ast *node, visitor v)
{
    v(node);
}

static void walk_continue(struct ast *node, visitor v)
{
    v(node);
}

static void walk_contaccess(struct ast *node, visitor v)
{
    struct ast_contaccess *ca = (struct ast_contaccess*)node;
    walk(ca->ident);
    walk(ca->index);
    v(node);
}

static void walk_funclit(struct ast *node, visitor v)
{
    struct ast_funclit *f = (struct ast_funclit*)node;

    if (f->ret_type) {
        walk(f->ret_type);
    }
    if (f->params) {
        walk(f->params);
    }
    walk(f->body);
    v(node);
}

static void walk_funcdef(struct ast *node, visitor v)
{
    struct ast_funcdef *f = (struct ast_funcdef*)node;

    if (f->ret_type) {
        walk(f->ret_type);
    }

    walk(f->name);

    if (f->params) {
        walk(f->params);
    }
    walk(f->body);
    v(node);
}
