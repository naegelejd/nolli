#include "nolli.h"
#include "ast.h"
/* FIXME: needed for get_tok_name: */
#include "lexer.h"

static int graph(struct nl_ast *root, FILE *, int id);

static int graph_bool_lit(struct nl_ast *node, FILE *fp, int id)
{
    static char *bs[] = {"false", "true"};
    fprintf(fp, "%d [label=\"bool: %s\"]\n", id, bs[node->b]);
    return id;
}

static int graph_char_lit(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"char: %c\"]\n", id, node->c);
    return id;
}

static int graph_int_num(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"int: %ld\"]\n", id, node->l);
    return id;
}

static int graph_real_num(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"real: %g\"]\n", id, node->d);
    return id;
}

static int graph_str_lit(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"str: \\\"%s\\\"\"]\n", id, node->s->str);
    return id;
}

static int graph_ident(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"ident: %s\"]\n", id, node->s->str);
    return id;
}

static int graph_unexpr(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=[\"unexpr\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->unexpr.expr, fp, id);

    return id;
}

static int graph_binexpr(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_get_tok_name(node->binexpr.op));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->binexpr.lhs, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->binexpr.rhs, fp, id);

    return id;
}

static int graph_tmpl_type(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"type\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->tmpl_type.name, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->tmpl_type.tmpls, fp, id);

    return id;
}

static int graph_qual_type(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"qualified\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->qual_type.package, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->qual_type.name, fp, id);

    return id;
}

static int graph_func_type(struct nl_ast *node, FILE *fp, int id)
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

static int graph_package_ref(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"package ref\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->package_ref.package, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->package_ref.expr, fp, id);

    return id;
}

static int graph_selector(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"selector\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->selector.parent, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->selector.child, fp, id);

    return id;
}

static int graph_keyval(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"keyval\"]\n", rID);

    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->keyval.key, fp, id);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->keyval.val, fp, id);

    return id;
}

static int graph_bind(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"bind\"]\n", rID);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->bind.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->bind.expr, fp, id);

    return id;
}

static int graph_assign(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"assignment\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->assignment.lhs, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->assignment.expr, fp, id);

    return id;
}

static int graph_call(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"call\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->call.func, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->call.args, fp, id);

    return id;
}

static int graph_init(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"init\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->init.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->init.expr, fp, id);

    return id;
}

static int graph_ifelse(struct nl_ast *node, FILE *fp, int id)
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

static int graph_while(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"while\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->while_loop.cond, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->while_loop.body, fp, id);

    return id;
}

static int graph_for(struct nl_ast *node, FILE *fp, int id)
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

static int graph_return(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"return\"]\n", rID);

    if (node->ret.expr) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->ret.expr, fp, id);
    }

    return id;
}

static int graph_break(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"break\"]\n", id);
    return id;
}

static int graph_continue(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"continue\"]\n", id);
    return id;
}

static int graph_lookup(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"lookup\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->lookup.container, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->lookup.index, fp, id);

    return id;
}

static int graph_function(struct nl_ast *node, FILE *fp, int id)
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

static int graph_classlit(struct nl_ast *node, FILE *fp, int id)
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

static int graph_decl(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"decl\"]\n", rID);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->decl.type, fp, id);

    struct nl_ast *rhs = node->decl.rhs;
    if (rhs) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->decl.rhs, fp, id);
    }

    return id;
}

static int graph_interface(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"interface\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.name, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.methods, fp, id);

    return id;
}

static int graph_class(struct nl_ast *node, FILE *fp, int id)
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

static int graph_alias(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"alias\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->alias.type, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->alias.name, fp, id);

    return id;
}

static int graph_using(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"using\"]\n", rID);

    assert(node->usings.names);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->usings.names, fp, id);

    return id;
}

static int graph_unit(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"unit\"]\n", rID);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->unit.package, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->unit.globals, fp, id);

    return id;
}

static int graph_list(struct nl_ast *node, FILE *fp, int id, const char *name)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, name);

    struct nl_ast *elem = node->list.head;
    while (elem) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(elem, fp, id);
        elem = elem->next;
    }

    return id;
}

static int graph_listlit(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "list literal");
}

static int graph_maplit(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "map literal");
}

static int graph_globals(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "globals");
}

static int graph_usings(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "usings");
}

static int graph_members(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "members");
}

static int graph_statements(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "statements");
}

static int graph_idents(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "idents");
}

static int graph_types(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "types");
}

static int graph_methods(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "methods");
}

static int graph_method_decls(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "method decls");
}

static int graph_decls(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "declarations");
}

static int graph_class_inits(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "class literal inits");
}

static int graph_params(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "params");
}

static int graph_args(struct nl_ast *node, FILE *fp, int id)
{
    return graph_list(node, fp, id, "args");
}


typedef int (*grapher) (struct nl_ast*, FILE *, int id);

static int graph(struct nl_ast *root, FILE *fp, int id)
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
        graph_package_ref,

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
        graph_using,
        graph_unit,

        NULL,   /* sentinel separator */

        graph_listlit,
        graph_maplit,
        graph_globals,
        graph_usings,
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

    /* Check that there are as many graphers as nl_ast node types */
    assert(sizeof(graphers) / sizeof(*graphers) == NL_AST_LAST + 1);

    grapher g = graphers[root->tag];
    assert(g);
    return g(root, fp, id);
}

int nl_graph_ast(struct nl_context *ctx)
{
    assert(ctx);

    FILE *fp = NULL;

    fp = fopen("astdump.dot", "w");
    if (fp == NULL) {
        NOLLI_ERROR("Failed to open a new graph file");
        return NL_ERR_IO;
    }

    struct nl_ast *root = ctx->ast_head;
    fputs("digraph hierarchy {\nnode [color=Green,fontcolor=Blue]", fp);

    int id = 0;
    fprintf(fp, "%d [label=\"All Units\"]\n", 0);
    while (root) {
        fprintf(fp, "%d -> %d\n", 0, ++id);
        id = graph(root, fp, id);
        root = root->next;
    }
    fputs("}", fp);

    if ((fclose(fp)) == EOF) {
        NOLLI_ERROR("Failed to close graph file");
        return NL_ERR_IO;
    }

    return NL_NO_ERR;
}
