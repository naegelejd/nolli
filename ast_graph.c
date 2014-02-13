#include "ast_graph.h"

typedef int (*ast_grapher) (struct ast*, FILE *, int id);

static int graph(struct ast *root, FILE *, int id);

static int graph_bool_lit(struct ast *node, FILE *fp, int id)
{
    struct ast_bool *x = (struct ast_bool*)node;
    static char *bs[] = {"false", "true"};
    fprintf(fp, "%d [label=\"bool: %s\"]\n", id, bs[x->b]);
    return id;
}

static int graph_char_lit(struct ast *node, FILE *fp, int id)
{
    struct ast_char *x = (struct ast_char*)node;
    fprintf(fp, "%d [label=\"char: %c\"]\n", id, x->c);
    return id;
}

static int graph_int_num(struct ast *node, FILE *fp, int id)
{
    struct ast_int *x = (struct ast_int*)node;
    fprintf(fp, "%d [label=\"int: %ld\"]\n", id, x->l);
    return id;
}

static int graph_real_num(struct ast *node, FILE *fp, int id)
{
    struct ast_real *x = (struct ast_real*)node;
    fprintf(fp, "%d [label=\"real: %g\"]\n", id, x->d);
    return id;
}

static int graph_str_lit(struct ast *node, FILE *fp, int id)
{
    struct ast_str *x = (struct ast_str*)node;
    fprintf(fp, "%d [label=\"str: \\\"%s\\\"\"]\n", id, x->s);
    return id;
}

static int graph_ident(struct ast *node, FILE *fp, int id)
{
    struct ast_ident *name = (struct ast_ident*)node;
    fprintf(fp, "%d [label=\"ident: %s\"]\n", id, name->s);
    return id;
}

static int graph_unexpr(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_unexpr* unexpr = (struct ast_unexpr*)node;
    fprintf(fp, "%d [label=[\"unexpr\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(unexpr->expr, fp, id);

    return id;
}

static int graph_binexpr(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_binexpr* binexpr = (struct ast_binexpr*)node;

    fprintf(fp, "%d [label=\"binexpr\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(binexpr->lhs, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(binexpr->rhs, fp, id);

    return id;
}

static int graph_literal_list(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_list* list = (struct ast_list*)node;

    fprintf(fp, "%d [label=\"list-lit\"]\n", rID);

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(list->items[i], fp, id);
    }
    return id;
}

static int graph_map_item_list(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_list* list = (struct ast_list*)node;

    fprintf(fp, "%d [label=\"map-lit\"]\n", rID);

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(list->items[i], fp, id);
    }
    return id;
}

static int graph_selector_list(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_list* list = (struct ast_list*)node;

    fprintf(fp, "%d [label=\"selectors\"]\n", rID);

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(list->items[i], fp, id);
    }
    return id;
}

static int graph_statement_list(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_list* list = (struct ast_list*)node;

    fprintf(fp, "%d [label=\"statements\"]\n", rID);

    unsigned int i = 0;
    for (i = 0; i < list->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(list->items[i], fp, id);
    }
    return id;
}

static int graph_list(struct ast *node, FILE *fp, int id)
{
    struct ast_list* list = (struct ast_list*)node;

    static ast_grapher graphers[] = {
        NULL,NULL,NULL,NULL,
        graph_literal_list,
        NULL,
        graph_map_item_list,
        graph_selector_list,
        NULL,NULL,
        graph_statement_list,
    };

    ast_grapher g = graphers[list->type];

    /* assert that the graph function is not NULL, since the NULL functions
     * the ast_list* types that should be handled in their corresponding
     * parent node graph function... e.g.
     * LIST_ARG ast_list types are handled in the graph_call function */
    assert(g);

    return g(node, fp, id);
}

static int graph_keyval(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_keyval* keyval = (struct ast_keyval*)node;
    fprintf(fp, "%d [label=\"keyval\"]\n", rID);

    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(keyval->key, fp, id);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(keyval->val, fp, id);

    return id;
}

static int graph_short_decl(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_short_decl* short_decl = (struct ast_short_decl*)node;

    fprintf(fp, "%d [label=\"short decl\"]\n", rID);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(short_decl->ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(short_decl->expr, fp, id);

    return id;
}

static int graph_assign(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_assignment* assignment = (struct ast_assignment*)node;

    fprintf(fp, "%d [label=\"assignment\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(assignment->ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(assignment->expr, fp, id);

    return id;
}

static int graph_call(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_call* call = (struct ast_call*)node;

    fprintf(fp, "%d [label=\"call\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(call->func, fp, id);

    struct ast_list *args = (struct ast_list *)call->args;

    unsigned int i = 0;
    for (i = 0; i < args->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(args->items[i], fp, id);
    }

    return id;
}

static int graph_import(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_import* import = (struct ast_import*)node;
    fprintf(fp, "%d [label=\"import\"]\n", rID);

    if (import->from) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(import->from, fp, id);
    }

    assert(import->modules);
    struct ast_list *modules = (struct ast_list *)import->modules;

    unsigned int i = 0;
    for (i = 0; i < modules->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(modules->items[i], fp, id);
    }

    return id;
}

static int graph_list_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_list_type *type = (struct ast_list_type*)node;

    fprintf(fp, "%d [label=\"list-type\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(type->name, fp, id);

    return id;
}

static int graph_map_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_map_type *type = (struct ast_map_type*)node;

    fprintf(fp, "%d [label=\"map-type\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(type->keyname, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(type->valname, fp, id);

    return id;
}

static int graph_func_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_func_type *type = (struct ast_func_type*)node;

    fprintf(fp, "%d [label=\"func-type\"]\n", rID);

    if (type->ret_type) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(type->ret_type, fp, id);
    }
    if (type->param_types) {
        struct ast_list *params = (struct ast_list*)type->param_types;
        unsigned int i;
        for (i = 0; i < params->count; i++) {
            fprintf(fp, "%d -> %d\n", rID, ++id);
            id = graph(params->items[i], fp, id);
        }
    }

    return id;
}

static int graph_struct_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_struct_type *type = (struct ast_struct_type*)node;

    fprintf(fp, "%d [label=\"struct\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(type->name, fp, id);

    struct ast_list *members = (struct ast_list*)type->members;
    unsigned int i;
    for (i = 0; i < members->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(members->items[i], fp, id);
    }

    return id;
}

static int graph_iface_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_iface_type *type = (struct ast_iface_type*)node;

    fprintf(fp, "%d [label=\"iface\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(type->name, fp, id);


    struct ast_list *methods = (struct ast_list*)type->methods;
    unsigned int i;
    for (i = 0; i < methods->count; i++) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(methods->items[i], fp, id);
    }

    return id;
}

static int graph_decl(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_decl *decl = (struct ast_decl *)node;

    fprintf(fp, "%d [label=\"decl\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(decl->type, fp, id);

    if (decl->name_s->type == AST_LIST) {
        struct ast_list *names = (struct ast_list*)decl->name_s;
        unsigned int i;
        for (i = 0; i < names->count; i++) {
            fprintf(fp, "%d -> %d\n", rID, ++id);
            id = graph(names->items[i], fp, id);
        }

    } else {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(decl->name_s, fp, id);
    }

    return id;
}

static int graph_initialization(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_init *init = (struct ast_init *)node;

    fprintf(fp, "%d [label=\"init\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(init->ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(init->expr, fp, id);

    return id;
}

static int graph_ifelse(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_ifelse *ifelse = (struct ast_ifelse *)node;

    fprintf(fp, "%d [label=\"if-else\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(ifelse->cond, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(ifelse->if_body, fp, id);

    if (ifelse->else_body) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(ifelse->else_body, fp, id);
    }

    return id;
}

static int graph_while(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_while *wile = (struct ast_while *)node;

    fprintf(fp, "%d [label=\"while\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(wile->cond, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(wile->body, fp, id);

    return id;
}

static int graph_for(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_for *fore = (struct ast_for *)node;

    fprintf(fp, "%d [label=\"for\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(fore->var, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(fore->range, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(fore->body, fp, id);

    return id;
}

static int graph_alias(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_alias *alias = (struct ast_alias*)node;

    fprintf(fp, "%d [label=\"alias\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(alias->type, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(alias->name, fp, id);

    return id;
}

static int graph_return(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_return *ret = (struct ast_return*)node;
    fprintf(fp, "%d [label=\"return\"]\n", rID);

    if (ret->expr) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(ret->expr, fp, id);
    }

    return id;
}

static int graph_break(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"break\"]\n", id);
    return id;
}

static int graph_continue(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"continue\"]\n", id);
    return id;
}

static int graph_contaccess(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_contaccess *ca = (struct ast_contaccess*)node;

    fprintf(fp, "%d [label=\"contaccess\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(ca->ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(ca->index, fp, id);

    return id;
}

static int graph_funclit(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    struct ast_funclit *f = (struct ast_funclit*)node;
    fprintf(fp, "%d [label=\"funclit\"]\n", rID);

    if (f->ret_type) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(f->ret_type, fp, id);
    }
    if (f->params) {
        struct ast_list *params = (struct ast_list*)f->params;
        unsigned int i;
        for (i = 0; i < params->count; i++) {
            fprintf(fp, "%d -> %d\n", rID, ++id);
            id = graph(params->items[i], fp, id);
        }
    }

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(f->body, fp, id);

    return id;
}

static int graph(struct ast *root, FILE *fp, int id)
{
    assert(root);

    static ast_grapher ast_graphers[] = {
        NULL, /* not a valid AST node */
        graph_bool_lit,
        graph_char_lit,
        graph_int_num,
        graph_real_num,
        graph_str_lit,

        graph_ident,

        graph_import,
        graph_alias,

        graph_list_type,
        graph_map_type,
        graph_func_type,
        graph_struct_type,
        graph_iface_type,

        graph_decl,
        graph_initialization,

        graph_unexpr,
        graph_binexpr,
        graph_list,

        graph_keyval,
        graph_contaccess,

        graph_short_decl,
        graph_assign,
        graph_ifelse,
        graph_while,
        graph_for,
        graph_call,
        graph_funclit,

        graph_return,
        graph_break,
        graph_continue,
    };

    ast_grapher g = ast_graphers[root->type];
    assert(g);
    return g(root, fp, id);
}

void dump_ast_graph(struct ast* root)
{
    FILE *fp = NULL;

    fp = fopen("astdump.dot", "w");

    fputs("digraph hierarchy {\nnode [color=Green,fontcolor=Blue]", fp);
    graph(root, fp, 0);
    fputs("}", fp);

    fclose(fp);
}
