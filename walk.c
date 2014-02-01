#include "walk.h"

struct irstate {
    struct symtable *symtable;
};

typedef void (*visitor) (struct ast*);
typedef void (*walker) (struct ast*, visitor);

static void walk_bool_lit(struct ast *node, visitor v);
static void walk_char_lit(struct ast *node, visitor v);
static void walk_int_num(struct ast *node, visitor v);
static void walk_real_num(struct ast *node, visitor v);
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

static void walk_ast(struct ast *root, struct irstate *state);

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

static void visit(struct ast* node)
{
    printf("%s\n", ast_name(node));
}

void walk(struct ast* root)
{
    struct irstate state;
    state.symtable = symtable_create();
    walk_ast(root, &state);
}

static void walk_ast(struct ast *root, struct irstate *state)
{
    static walker walkers[] = {
        NULL, /* not a valid AST node */
        walk_bool_lit,
        walk_char_lit,
        walk_int_num,
        walk_real_num,
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

        walk_return,
        walk_break,
        walk_continue,
    };

    visitor v = visit;

    walker w = walkers[root->type];
    if (w) w(root, v);
}


static void walk_bool_lit(struct ast *node, visitor v)
{
    v(node);
}

static void walk_char_lit(struct ast *node, visitor v)
{
    v(node);
}

static void walk_int_num(struct ast *node, visitor v)
{
    v(node);
}

static void walk_real_num(struct ast *node, visitor v)
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
