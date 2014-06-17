#include "graph.h"

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

static int graph_tmpl_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"type\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->tmpl_type.name, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->tmpl_type.tmpls, fp, id);

    return id;
}

static int graph_qual_type(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"qualified\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->qual_type.package, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->qual_type.name, fp, id);

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
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->func_type.params, fp, id);
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

static int graph_bind(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"bind\"]\n", rID);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->bind.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->bind.expr, fp, id);

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

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->call.args, fp, id);

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
    id = graph(node->function.body, fp, id);

    return id;
}

static int graph_classlit(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"struclit\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->classlit.name, fp, id);

    if (node->classlit.tmpl) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->classlit.tmpl, fp, id);
    }

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->classlit.items, fp, id);

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
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->decl.rhs, fp, id);
    }

    return id;
}

static int graph_interface(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"interface\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.name, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.methods, fp, id);

    return id;
}

static int graph_class(struct ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"class\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->classdef.name, fp, id);

    if (node->classlit.tmpl) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->classlit.tmpl, fp, id);
    }

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->classdef.members, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->classdef.methods, fp, id);

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

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->import.modules, fp, id);

    return id;
}

static int graph_program(struct ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"program\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->program.package, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->program.globals, fp, id);

    return id;
}

static int graph_list(struct ast *node, FILE *fp, int id, const char *name)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, name);

    struct ast *elem = node->list.head;
    while (elem) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(elem, fp, id);
        elem = elem->next;
    }

    return id;
}

static int graph_listlit(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "list literal");
}

static int graph_maplit(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "map literal");
}

static int graph_globals(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "globals");
}

static int graph_imports(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "imports");
}

static int graph_members(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "members");
}

static int graph_statements(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "statements");
}

static int graph_idents(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "idents");
}

static int graph_types(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "types");
}

static int graph_methods(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "methods");
}

static int graph_method_decls(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "method decls");
}

static int graph_decls(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "declarations");
}

static int graph_class_inits(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "class literal inits");
}

static int graph_params(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "params");
}

static int graph_args(struct ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "args");
}


typedef int (*grapher) (struct ast*, FILE *, int id);

static int graph(struct ast *root, FILE *fp, int id)
{
    assert(root);

    static grapher graphers[] = {
        NULL,   /* sentinel */

        graph_bool_lit,
        graph_char_lit,
        graph_int_num,
        graph_real_num,
        graph_str_lit,

        graph_ident,

        graph_tmpl_type,
        graph_qual_type,
        graph_func_type,

        graph_decl,
        graph_init,

        graph_unexpr,
        graph_binexpr,

        graph_keyval,
        graph_lookup,
        graph_selector,

        graph_bind,
        graph_assign,
        graph_ifelse,
        graph_while,
        graph_for,
        graph_call,
        graph_function,
        graph_classlit,

        graph_return,
        graph_break,
        graph_continue,

        graph_class,
        graph_interface,
        graph_alias,
        graph_import,
        graph_program,

        NULL,   /* sentinel separator */

        graph_listlit,
        graph_maplit,
        graph_globals,
        graph_imports,
        graph_members,
        graph_statements,
        graph_idents,
        graph_types,
        graph_methods,
        graph_method_decls,
        graph_decls,
        graph_class_inits,
        graph_params,
        graph_args,

        NULL, /* sentinel */
    };

    /* Check that there are as many graphers as AST node types */
    assert(sizeof(graphers) / sizeof(*graphers) == AST_LAST + 1);

    grapher g = graphers[root->tag];
    assert(g);
    return g(root, fp, id);
}

void graph_ast(struct ast *root)
{
    assert(root);

    FILE *fp = NULL;

    fp = fopen("astdump.dot", "w");

    fputs("digraph hierarchy {\nnode [color=Green,fontcolor=Blue]", fp);
    graph(root, fp, 0);
    fputs("}", fp);

    fclose(fp);
}
