#include "nolli.h"
#include "ast.h"
#include "strtab.h"
#include "debug.h"

/* FIXME: needed for get_tok_name: */
#include "lexer.h"

#include <stdio.h>
#include <assert.h>

static int graph(struct nl_ast *root, FILE *, int id);

static int graph_bool_lit(struct nl_ast *node, FILE *fp, int id)
{
    static char *bs[] = {"false", "true"};
    fprintf(fp, "%d [label=\"%s: %s\"]\n", id, nl_ast_name(node), bs[node->b]);
    return id;
}

static int graph_char_lit(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s: %c\"]\n", id, nl_ast_name(node), node->c);
    return id;
}

static int graph_int_num(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s: %ld\"]\n", id, nl_ast_name(node), node->l);
    return id;
}

static int graph_real_num(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s: %g\"]\n", id, nl_ast_name(node), node->d);
    return id;
}

static int graph_str_lit(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s: \\\"%s\\\"\"]\n", id, nl_ast_name(node), node->s->str);
    return id;
}

static int graph_ident(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s: %s\"]\n", id, nl_ast_name(node), node->s->str);
    return id;
}

static int graph_unexpr(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
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

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->tmpl_type.name, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->tmpl_type.tmpls, fp, id);

    return id;
}

static int graph_qual_type(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->qual_type.package, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->qual_type.name, fp, id);

    return id;
}

static int graph_func_type(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

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

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->package_ref.package, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->package_ref.expr, fp, id);

    return id;
}

static int graph_selector(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->selector.parent, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->selector.child, fp, id);

    return id;
}

static int graph_keyval(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->keyval.key, fp, id);
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->keyval.val, fp, id);

    return id;
}

static int graph_bind(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp,"%d -> %d\n", rID, ++id);
    id = graph(node->bind.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->bind.expr, fp, id);

    return id;
}

static int graph_assign(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->assignment.lhs, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->assignment.expr, fp, id);

    return id;
}

static int graph_call(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->call.func, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->call.args, fp, id);

    return id;
}

static int graph_init(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->init.ident, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->init.expr, fp, id);

    return id;
}

static int graph_ifelse(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
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

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->while_loop.cond, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->while_loop.body, fp, id);

    return id;
}

static int graph_for(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
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
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    if (node->ret.expr) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(node->ret.expr, fp, id);
    }

    return id;
}

static int graph_break(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s\"]\n", id, nl_ast_name(node));
    return id;
}

static int graph_continue(struct nl_ast *node, FILE *fp, int id)
{
    fprintf(fp, "%d [label=\"%s\"]\n", id, nl_ast_name(node));
    return id;
}

static int graph_lookup(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->lookup.container, fp, id);
    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->lookup.index, fp, id);

    return id;
}

static int graph_function(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

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
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->classlit.type, fp, id);

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

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));
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

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.name, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->interface.methods, fp, id);

    return id;
}

static int graph_class(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

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

    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->alias.type, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->alias.name, fp, id);

    return id;
}

static int graph_using(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    assert(node->usings.names);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->usings.names, fp, id);

    return id;
}

static int graph_package(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->package.name, fp, id);

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->package.globals, fp, id);

    return id;
}

static int graph_unit(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    fprintf(fp, "%d -> %d\n", rID, ++id);
    id = graph(node->unit.packages, fp, id);

    return id;
}

static int graph_list(struct nl_ast *node, FILE *fp, int id)
{
    int rID = id;
    fprintf(fp, "%d [label=\"%s\"]\n", rID, nl_ast_name(node));

    struct nl_ast *elem = node->list.head;
    while (elem) {
        fprintf(fp, "%d -> %d\n", rID, ++id);
        id = graph(elem, fp, id);
        elem = elem->next;
    }

    return id;
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
        graph_package,
        graph_unit,

        NULL,   /* sentinel separator */

        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,
        graph_list,

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
        NL_ERROR(ctx, NL_ERR_GRAPH, "Failed to open a new graph file");
        return NL_ERR_IO;
    }

    fputs("digraph hierarchy {\nnode [color=Green,fontcolor=Blue]", fp);
    graph(ctx->ast_list, fp, 0);
    fputs("}\n", fp);

    if ((fclose(fp)) == EOF) {
        NL_ERROR(ctx, NL_ERR_GRAPH, "Failed to close graph file");
        return NL_ERR_IO;
    }

    return NL_NO_ERR;
}
