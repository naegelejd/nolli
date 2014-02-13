#include "walk.h"

struct irstate {
    struct symtable *symtable;
};

typedef struct type* (*walker) (struct ast*, struct irstate*);

static struct type *walk_bool_lit(struct ast*, struct irstate*);
static struct type *walk_char_lit(struct ast*, struct irstate*);
static struct type *walk_int_num(struct ast*, struct irstate*);
static struct type *walk_real_num(struct ast*, struct irstate*);
static struct type *walk_str_lit(struct ast*, struct irstate*);
static struct type *walk_ident(struct ast*, struct irstate*);
static struct type *walk_unexpr(struct ast*, struct irstate*);
static struct type *walk_binexpr(struct ast*, struct irstate*);
static struct type *walk_list(struct ast*, struct irstate*);
static struct type *walk_keyval(struct ast*, struct irstate*);
static struct type *walk_short_decl(struct ast*, struct irstate*);
static struct type *walk_assign(struct ast*, struct irstate*);
static struct type *walk_call(struct ast*, struct irstate*);
static struct type *walk_import(struct ast*, struct irstate*);
static struct type *walk_list_type(struct ast*, struct irstate*);
static struct type *walk_map_type(struct ast*, struct irstate*);
static struct type *walk_func_type(struct ast*, struct irstate*);
static struct type *walk_struct_type(struct ast*, struct irstate*);
static struct type *walk_iface_type(struct ast*, struct irstate*);
static struct type *walk_decl(struct ast*, struct irstate*);
static struct type *walk_initialization(struct ast*, struct irstate*);
static struct type *walk_ifelse(struct ast*, struct irstate*);
static struct type *walk_while(struct ast*, struct irstate*);
static struct type *walk_for(struct ast*, struct irstate*);
static struct type *walk_alias(struct ast*, struct irstate*);
static struct type *walk_return(struct ast*, struct irstate*);
static struct type *walk_break(struct ast*, struct irstate*);
static struct type *walk_continue(struct ast*, struct irstate*);
static struct type *walk_contaccess(struct ast*, struct irstate*);
static struct type *walk_funclit(struct ast*, struct irstate*);

static struct type *walk_decl_list(struct ast *node, struct irstate *irs);
static struct type *walk_arg_list(struct ast *node, struct irstate *irs);
static struct type *walk_param_list(struct ast *node, struct irstate *irs);
static struct type *walk_type_list(struct ast *node, struct irstate *irs);
static struct type *walk_literal_list(struct ast *node, struct irstate *irs);
static struct type *walk_import_list(struct ast *node, struct irstate *irs);
static struct type *walk_map_item_list(struct ast *node, struct irstate *irs);
static struct type *walk_selector_list(struct ast *node, struct irstate *irs);
static struct type *walk_member_list(struct ast *node, struct irstate *irs);
static struct type *walk_method_list(struct ast *node, struct irstate *irs);
static struct type *walk_statement_list(struct ast *node, struct irstate *irs);

static struct type *walk(struct ast *root, struct irstate *irs);

void type_check(struct ast* root)
{
    struct irstate state;
    state.symtable = symtable_create();
    walk(root, &state);
}

static struct type *walk(struct ast *root, struct irstate *irs)
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
        walk_alias,

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

        walk_short_decl,
        walk_assign,
        walk_ifelse,
        walk_while,
        walk_for,
        walk_call,
        walk_funclit,

        walk_return,
        walk_break,
        walk_continue,
    };

    walker w = walkers[root->type];
    assert(w);
    return w(root, irs);
}


static struct type *walk_bool_lit(struct ast *node, struct irstate *irs)
{

    return &bool_type;
}

static struct type *walk_char_lit(struct ast *node, struct irstate *irs)
{

    return &char_type;
}

static struct type *walk_int_num(struct ast *node, struct irstate *irs)
{

    return &int_type;
}

static struct type *walk_real_num(struct ast *node, struct irstate *irs)
{

    return &real_type;
}

static struct type *walk_str_lit(struct ast *node, struct irstate *irs)
{

    return &str_type;
}

static struct type *walk_ident(struct ast *node, struct irstate *irs)
{

    return NULL;    /* FIXME */
}

static struct type *walk_unexpr(struct ast *node, struct irstate *irs)
{
    struct ast_unexpr* unexpr = (struct ast_unexpr*)node;
    walk(unexpr->expr, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_binexpr(struct ast *node, struct irstate *irs)
{
    struct ast_binexpr* binexpr = (struct ast_binexpr*)node;
    struct type *rhs = walk(binexpr->lhs, irs);
    struct type *lhs = walk(binexpr->rhs, irs);
    if (rhs != lhs) {
        NOLLI_DIE("Type mismatch in binary expression");
    }

    return NULL;    /* FIXME */
}

static struct type *walk_decl_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_arg_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_param_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_type_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_literal_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_import_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_map_item_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_selector_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_member_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_method_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_statement_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        walk(list->items[i], irs);
    }
    return NULL;
}

static struct type *walk_list(struct ast *node, struct irstate *irs)
{
    struct ast_list* list = (struct ast_list*)node;

    static walker walkers[] = {
        walk_decl_list,
        walk_arg_list,
        walk_param_list,
        walk_type_list,
        walk_literal_list,
        walk_import_list,
        walk_map_item_list,
        walk_selector_list,
        walk_member_list,
        walk_method_list,
        walk_statement_list,
    };

    walker w = walkers[list->type];
    assert(w);
    return w(node, irs);
}

static struct type *walk_keyval(struct ast *node, struct irstate *irs)
{
    struct ast_keyval* keyval = (struct ast_keyval*)node;
    walk(keyval->key, irs);
    walk(keyval->val, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_short_decl(struct ast *node, struct irstate *irs)
{
    struct ast_short_decl* short_decl = (struct ast_short_decl*)node;
    walk(short_decl->ident, irs);
    walk(short_decl->expr, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_assign(struct ast *node, struct irstate *irs)
{
    struct ast_assignment* assignment = (struct ast_assignment*)node;
    walk(assignment->ident, irs);
    walk(assignment->expr, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_call(struct ast *node, struct irstate *irs)
{
    struct ast_call* call = (struct ast_call*)node;
    walk(call->func, irs);
    walk(call->args, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_import(struct ast *node, struct irstate *irs)
{
    struct ast_import* import = (struct ast_import*)node;
    if (import->from) {
        /* import->from */
    }

    assert(import->modules);
    walk(import->modules, irs);


    return NULL;    /* FIXME */
}

static struct type *walk_list_type(struct ast *node, struct irstate *irs)
{
    struct ast_list_type *type = (struct ast_list_type*)node;
    walk(type->name, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_map_type(struct ast *node, struct irstate *irs)
{
    struct ast_map_type *type = (struct ast_map_type*)node;
    walk(type->keyname, irs);
    walk(type->valname, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_func_type(struct ast *node, struct irstate *irs)
{
    struct ast_func_type *type = (struct ast_func_type*)node;
    if (type->ret_type) {
        walk(type->ret_type, irs);
    }
    if (type->param_types) {
        walk(type->param_types, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *walk_struct_type(struct ast *node, struct irstate *irs)
{
    struct ast_struct_type *type = (struct ast_struct_type*)node;
    /* type->name */
    walk(type->members, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_iface_type(struct ast *node, struct irstate *irs)
{
    struct ast_iface_type *type = (struct ast_iface_type*)node;
    /* type->name */
    walk(type->methods, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_decl(struct ast *node, struct irstate *irs)
{
    struct ast_decl *decl = (struct ast_decl *)node;
    walk(decl->type, irs);
    walk(decl->name_s, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_initialization(struct ast *node, struct irstate *irs)
{
    struct ast_init *init = (struct ast_init *)node;
    walk(init->ident, irs);
    walk(init->expr, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_ifelse(struct ast *node, struct irstate *irs)
{
    struct ast_ifelse *ifelse = (struct ast_ifelse *)node;
    walk(ifelse->cond, irs);
    walk(ifelse->if_body, irs);
    if (ifelse->else_body) {
        walk(ifelse->else_body, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *walk_while(struct ast *node, struct irstate *irs)
{
    struct ast_while *wile = (struct ast_while *)node;
    walk(wile->cond, irs);
    walk(wile->body, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_for(struct ast *node, struct irstate *irs)
{
    struct ast_for *fore = (struct ast_for *)node;
    walk(fore->var, irs);
    walk(fore->range, irs);
    walk(fore->body, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_alias(struct ast *node, struct irstate *irs)
{
    struct ast_alias *alias = (struct ast_alias*)node;
    walk(alias->type, irs);
    /* alias->name */

    return NULL;    /* FIXME */
}

static struct type *walk_return(struct ast *node, struct irstate *irs)
{
    struct ast_return *ret = (struct ast_return*)node;
    if (ret->expr) {
        walk(ret->expr, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *walk_break(struct ast *node, struct irstate *irs)
{

    return NULL;    /* FIXME */
}

static struct type *walk_continue(struct ast *node, struct irstate *irs)
{

    return NULL;    /* FIXME */
}

static struct type *walk_contaccess(struct ast *node, struct irstate *irs)
{
    struct ast_contaccess *ca = (struct ast_contaccess*)node;
    walk(ca->ident, irs);
    walk(ca->index, irs);

    return NULL;    /* FIXME */
}

static struct type *walk_funclit(struct ast *node, struct irstate *irs)
{
    struct ast_funclit *f = (struct ast_funclit*)node;

    if (f->ret_type) {
        walk(f->ret_type, irs);
    }
    if (f->params) {
        walk(f->params, irs);
    }
    walk(f->body, irs);

    return NULL;    /* FIXME */
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
        "ALIAS",
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
        "SHORT_DECL",
        "ASSIGN",
        "IFELSE",
        "WHILE",
        "FOR",
        "CALL",
        "FUNCLIT",
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
