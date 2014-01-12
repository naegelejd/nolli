#include "nolli.h"

static struct ast* create_node(ast_type_t type)
{
    struct ast* a = nalloc(sizeof(*a));
    a->type = type;
    return a;
}

struct ast* ast_make_bool_lit(bool b)
{
    struct ast* bn = create_node(AST_BOOL_LIT);
    return bn;
}

struct ast* ast_make_char_lit(char c)
{
    struct ast_char *node = nalloc(sizeof(*node));
    node->HEAD.type = AST_CHAR_LIT;
    node->c = c;
    return (struct ast*)node;
}

struct ast* ast_make_int_num(long l)
{
    struct ast_int *node = nalloc(sizeof(*node));
    node->HEAD.type = AST_INT_NUM;
    node->l = l;
    return (struct ast*)node;
}

struct ast* ast_make_real_num(double d)
{
    struct ast_real *node = nalloc(sizeof(*node));
    node->HEAD.type = AST_REAL_NUM;
    node->d = d;
    return (struct ast*)node;
}

struct ast* ast_make_str_lit(const char *s)
{
    assert(s);

    struct ast_str *node = nalloc(sizeof(*node));
    node->HEAD.type = AST_STR_LIT;
    node->s = strdup(s);
    return (struct ast*)node;
}

struct ast* ast_make_ident(const char *s)
{
    assert(s);

    struct ast_ident *ident = nalloc(sizeof(*ident));
    ident->HEAD.type = AST_IDENT;
    ident->s = strdup(s);
    return (struct ast*)ident;
}

struct ast* ast_make_import(struct ast* parent, struct ast* names)
{
    struct ast* node = create_node(AST_IMPORT);
    return node;
}

struct ast* ast_make_typedef(type_t* t, struct ast* id)
{
    struct ast* node = create_node(AST_TYPEDEF);
    return node;
}

struct ast* ast_make_decl(type_t* t, struct ast* id)
{
    struct ast* dn = create_node(AST_DECL);
    return dn;
}


struct ast* ast_make_unexpr(expr_op_t op, struct ast* expr)
{
    assert(expr);

    struct ast_unexpr* unexpr = nalloc(sizeof(*unexpr));
    unexpr->HEAD.type = AST_UNEXPR;
    unexpr->op = op;
    unexpr->expr = expr;
    return (struct ast*)unexpr;
}

struct ast* ast_make_binexpr(struct ast* lhs, expr_op_t op, struct ast* rhs)
{
    assert(lhs);
    assert(rhs);

    struct ast_binexpr* binexpr = nalloc(sizeof(*binexpr));
    binexpr->HEAD.type = AST_BINEXPR;
    binexpr->op = op;
    binexpr->lhs = lhs;
    binexpr->rhs = rhs;
    return (struct ast*)binexpr;
}

struct ast *ast_make_list(list_type_t type)
{
    struct ast_list *list = nalloc(sizeof(*list));
    list->HEAD.type = AST_LIST;
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
    if (list->count > list->alloc) {
        list->alloc *= 2;
        list->items = nrealloc(list->items,
                list->alloc * sizeof(*list->items));
    }
    list->items[list->count++] = item;

    return (struct ast*)list;
}

struct ast* ast_make_keyval(struct ast* key, struct ast* val)
{
    struct ast_keyval* keyval = nalloc(sizeof(*keyval));
    keyval->HEAD.type = AST_KEYVAL;
    keyval->key = key;
    keyval->val = val;
    return (struct ast*)keyval;
}

struct ast* ast_make_contaccess(struct ast* cont, struct ast* idx)
{
    struct ast* node = create_node(AST_CONTACCESS);
    return node;
}

struct ast* ast_make_assignment(struct ast* ident, assign_op_t op, struct ast* expr)
{
    assert(ident);
    assert(expr);

    struct ast_assignment* assignment = nalloc(sizeof(*assignment));
    assignment->HEAD.type = AST_ASSIGN;

    assignment->ident = ident;
    assignment->expr = expr;

    return (struct ast*)assignment;
}

struct ast* ast_make_contassign(struct ast* cont, struct ast* idx,
        assign_op_t op, struct ast* item)
{
    struct ast* node = create_node(AST_CONTASSIGN);
    return node;
}

struct ast* ast_make_ifelse(struct ast* cond, struct ast* t, struct ast* f)
{
    struct ast* node = create_node(AST_IFELSE);
    return node;
}

struct ast* ast_make_while(struct ast* cond, struct ast* s)
{
    struct ast* node = create_node(AST_WHILE);
    return node;
}

struct ast* ast_make_until(struct ast* cond, struct ast* s)
{
    struct ast* node = create_node(AST_UNTIL);
    return node;
}

struct ast* ast_make_for(struct ast* id, struct ast* iter, struct ast* s)
{
    struct ast* node = create_node(AST_FOR);
    return node;
}

struct ast* ast_make_call(struct ast* func, struct ast* args)
{
    assert(func);
    assert(args);

    struct ast_call *call = nalloc(sizeof(*call));
    call->HEAD.type = AST_CALL;
    call->func = func;
    call->args = args;
    return (struct ast*)call;
}

struct ast* ast_make_member(struct ast* parent, struct ast* child)
{
    struct ast* node = create_node(AST_MEMBER);
    return node;
}

struct ast* ast_make_return(struct ast* expr)
{
    struct ast* node = create_node(AST_RETURN);
    return node;
}

struct ast* ast_make_break(void)
{
    struct ast* node = create_node(AST_BREAK);
    return node;
}

struct ast* ast_make_continue(void)
{
    struct ast* node = create_node(AST_CONTINUE);
    return node;
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
        "DECL",
        "UNEXPR",
        "BINEXPR",
        "LIST",
        "KEYVAL",
        "CONTACCESS",
        "ASSIGN",
        "CONTASSIGN",
        "IFELSE",
        "WHILE",
        "UNTIL",
        "FOR",
        "CALL",
        "FUNC_DEF",
        "STRUCT",
        "MEMBER",
        "RETURN",
        "BREAK",
        "CONTINUE",
    };

    static char *list_names[] = {
        "LIST_ARGS",
        "LIST_LITERAL",
        "LIST_MAP_ITEMS",
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
static void walk_ident(struct ast *node, visitor v);
static void walk_unexpr(struct ast *node, visitor v);
static void walk_binexpr(struct ast *node, visitor v);
static void walk_list(struct ast *node, visitor v);
static void walk_assign(struct ast *node, visitor v);
static void walk_call(struct ast *node, visitor v);

void walk(struct ast* root)
{
    static walker walkers[] = {
        NULL, /* not a valid AST node */
        NULL, /* walk_bool_lit, */
        NULL, /* walk_char_lit, */
        walk_int_num,
        NULL, /* walk_real_num, */
        NULL, /* walk_str_lit, */

        walk_ident,

        NULL, /* walk_import, */
        NULL, /* walk_typedef, */
        NULL, /* walk_decl, */

        walk_unexpr,
        walk_binexpr,
        walk_list,

        NULL, /* walk_keyval, */
        NULL, /* walk_contaccess, */

        walk_assign,
        NULL, /* walk_contassign, */
        NULL, /* walk_ifelse, */
        NULL, /* walk_while, */
        NULL, /* walk_until, */
        NULL, /* walk_for, */
        walk_call,
        NULL, /* walk_func_def, */
        NULL, /* walk_struct, */
        NULL, /* walk_member, */

        NULL, /* walk_return, */
        NULL, /* walk_break, */
        NULL, /* walk_continue, */
    };

    visitor v = visit;

    walkers[root->type](root, v);
}


static void walk_int_num(struct ast *node, visitor v)
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
