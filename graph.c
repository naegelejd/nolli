#include "graph.h"

typedef int (*ast_grapher) (struct ast*, FILE *, int id);

static int graph(struct ast *root, FILE *, int id);

static int graph_bool_lit(struct ast *node, FILE *fp, int id)
{
    static char *bs[] = {"false", "true"};
    fprintf(fp, "%d [label=\"bool: %s\"]\n", id, bs[node->b]);
    return id;
}

static int graph_char_lit(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"char: %c\"]\n", id, node->c);
    return id;
}

static int graph_int_num(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"int: %ld\"]\n", id, node->l);
    return id;
}

static int graph_real_num(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"real: %g\"]\n", id, node->d);
    return id;
}

static int graph_str_lit(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"str: \\\"%s\\\"\"]\n", id, node->s->str);
    return id;
}

static int graph_ident(struct ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"ident: %s\"]\n", id, node->s->str);
    return id;
}

static int graph_unexpr(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=[\"unexpr\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->unexpr.expr, fp, id);

    return id;
}

static int graph_binexpr(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, get_tok_name(node->binexpr.op));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->binexpr.lhs, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->binexpr.rhs, fp, id);

    return id;
}

static int graph_list(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"list\"]\n", rID);

    struct ast *item = node->list.head;
    while (item) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(item, fp, id);
        item = item->next;
    }
    return id;
}

static int graph_selector(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"selector\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->selector.parent, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->selector.child, fp, id);

    return id;
}

static int graph_keyval(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"keyval\"]\n", rID);

    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->keyval.key, fp, id);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->keyval.val, fp, id);

    return id;
}

static int graph_short_decl(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"short decl\"]\n", rID);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->short_decl.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->short_decl.expr, fp, id);

    return id;
}

static int graph_assign(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"assignment\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->assignment.lhs, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->assignment.expr, fp, id);

    return id;
}

static int graph_call(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"call\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->call.func, fp, id);

    struct ast* args = node->call.args;
    struct ast *arg = args->list.head;
    while (arg) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(arg, fp, id);
        arg = arg->next;
    }

    return id;
}

static int graph_list_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"list-type\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->list_type.name, fp, id);

    return id;
}

static int graph_map_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"map-type\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->map_type.keytype, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->map_type.valtype, fp, id);

    return id;
}

static int graph_func_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"functype\"]\n", rID);

    if (node->func_type.ret_type) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->func_type.ret_type, fp, id);
    }
    if (node->func_type.params) {
        struct ast *params = node->func_type.params;
        struct ast *item = params->list.head;
        while (item) {
            fprintf(fp, "%d -> %d\n", rID, ++id);
            id = graph(item, fp, id);
            item = item->next;
        }
    }

    return id;
}

static int graph_init(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"init\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->init.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->init.expr, fp, id);

    return id;
}

static int graph_ifelse(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"if-else\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->ifelse.cond, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->ifelse.if_body, fp, id);

    if (node->ifelse.else_body) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->ifelse.else_body, fp, id);
    }

    return id;
}

static int graph_while(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"while\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->while_loop.cond, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->while_loop.body, fp, id);

    return id;
}

static int graph_for(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"for\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->for_loop.var, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->for_loop.range, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->for_loop.body, fp, id);

    return id;
}

static int graph_return(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"return\"]\n", rID);

    if (node->ret.expr) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->ret.expr, fp, id);
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

static int graph_lookup(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"lookup\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->lookup.container, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->lookup.index, fp, id);

    return id;
}

static int graph_function(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"function\"]\n", rID);

    if (node->function.name) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->function.name, fp, id);
    }

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->function.type, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);

    struct ast *body = node->function.body;
    struct ast *stmt = body->list.head;
    while (stmt) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(stmt, fp, id);
        stmt = stmt->next;
    }

    return id;
}

static int graph_datalit(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"struclit\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->datalit.name, fp, id);

    struct ast *list = node->datalit.items;
    struct ast *item = list->list.head;
    while (item) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(item, fp, id);
        item = item->next;
    }

    return id;
}

static int graph_decl(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"decl\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->decl.type, fp, id);

    struct ast *rhs = node->decl.rhs;
    if (rhs) {
        if (rhs->tag == AST_LIST) {
            struct ast *item = rhs->list.head;
            while (item) {
                fprintf(fp, "%d -> %d\n", rID, ++id);
                id = graph(item, fp, id);
                item = item->next;
            }
        } else {
            fprintf(fp, "%d -> %d\n", rID, ++id);
            id = graph(node->decl.rhs, fp, id);
        }
    }

    return id;
}

static int graph_methods(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"methods\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->methods.name, fp, id);

    struct ast *meths = node->methods.methods;
    struct ast *meth = meths->list.head;
    while (meth) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(meth, fp, id);
        meth = meth->next;
    }

    return id;
}

static int graph_interface(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"interface\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.name, fp, id);

    struct ast *list = node->interface.methods;
    struct ast *item = list->list.head;
    while (item) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(item, fp, id);
        item = item->next;
    }

    return id;
}

static int graph_data(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"struct\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->data.name, fp, id);

    struct ast *list = node->data.members;
    struct ast *item = list->list.head;
    while (item) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(item, fp, id);
        item = item->next;
    }

    return id;
}

static int graph_alias(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"alias\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->alias.type, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->alias.name, fp, id);

    return id;
}

static int graph_import(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"import\"]\n", rID);

    if (node->import.from) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->import.from, fp, id);
    }

    assert(node->import.modules);
    struct ast *modules = node->import.modules;
    struct ast *mod = modules->list.head;
    while (mod) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(mod, fp, id);
        mod = mod->next;
    }

    return id;
}

static int graph_program(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"program\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->program.package, fp, id);

    struct ast *defs = node->program.definitions;
    struct ast *def = defs->list.head;
    while (def) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(def, fp, id);
        def = def->next;
    }

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

        graph_list_type,
        graph_map_type,
        graph_func_type,

        graph_decl,
        graph_init,

        graph_unexpr,
        graph_binexpr,

        graph_list,

        graph_keyval,
        graph_lookup,
        graph_selector,

        graph_short_decl,
        graph_assign,
        graph_ifelse,
        graph_while,
        graph_for,
        graph_call,
        graph_function,
        graph_datalit,

        graph_return,
        graph_break,
        graph_continue,

        graph_methods,
        graph_data,
        graph_interface,
        graph_alias,
        graph_import,
        graph_program
    };

    ast_grapher g = ast_graphers[root->tag];
    assert(g);
    return g(root, fp, id);
}

void dump_ast_graph(struct ast *root)
{
    assert(root);

    FILE *fp = NULL;

    fp = fopen("astdump.dot", "w");

    fputs("digraph hierarchy {\nnode [color=Green,fontcolor=Blue]", fp);
    graph(root, fp, 0);
    fputs("}", fp);

    fclose(fp);
}
