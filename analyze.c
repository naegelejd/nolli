#include "analyze.h"

struct irstate {
    struct symtable *symtable;
};

typedef struct type* (*analyzer) (struct ast*, struct irstate*);

static struct type *analyze(struct ast *root, struct irstate *irs);


static struct type *analyze_bool_lit(struct ast *node, struct irstate *irs)
{
    return &bool_type;
}

static struct type *analyze_char_lit(struct ast *node, struct irstate *irs)
{
    return &char_type;
}

static struct type *analyze_int_num(struct ast *node, struct irstate *irs)
{
    return &int_type;
}

static struct type *analyze_real_num(struct ast *node, struct irstate *irs)
{
    return &real_type;
}

static struct type *analyze_str_lit(struct ast *node, struct irstate *irs)
{
    return &str_type;
}

static struct type *analyze_ident(struct ast *node, struct irstate *irs)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_qualified(struct ast *node, struct irstate *irs)
{
    return NULL;
}

static struct type *analyze_list_type(struct ast *node, struct irstate *irs)
{
    analyze(node->list_type.name, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_map_type(struct ast *node, struct irstate *irs)
{
    analyze(node->map_type.keytype, irs);
    analyze(node->map_type.valtype, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_func_type(struct ast *node, struct irstate *irs)
{
    if (node->func_type.ret_type) {
        analyze(node->func_type.ret_type, irs);
    }
    if (node->func_type.params) {
        analyze(node->func_type.params, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_decl(struct ast *node, struct irstate *irs)
{
    analyze(node->decl.type, irs);
    if (node->decl.rhs) {
        analyze(node->decl.rhs, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_init(struct ast *node, struct irstate *irs)
{
    analyze(node->init.ident, irs);
    analyze(node->init.expr, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_unexpr(struct ast *node, struct irstate *irs)
{
    analyze(node->unexpr.expr, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_binexpr(struct ast *node, struct irstate *irs)
{
    struct type *rhs = analyze(node->binexpr.lhs, irs);
    struct type *lhs = analyze(node->binexpr.rhs, irs);
    if (rhs != lhs) {
        NOLLI_DIEF("Type mismatch in binary expression on line %d", node->lineno);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_keyval(struct ast *node, struct irstate *irs)
{
    analyze(node->keyval.key, irs);
    analyze(node->keyval.val, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_lookup(struct ast *node, struct irstate *irs)
{
    analyze(node->lookup.container, irs);
    analyze(node->lookup.index, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_selector(struct ast *node, struct irstate *irs)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_bind(struct ast *node, struct irstate *irs)
{
    analyze(node->bind.ident, irs);
    analyze(node->bind.expr, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_assign(struct ast *node, struct irstate *irs)
{
    analyze(node->assignment.lhs, irs);
    analyze(node->assignment.expr, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_ifelse(struct ast *node, struct irstate *irs)
{
    analyze(node->ifelse.cond, irs);
    analyze(node->ifelse.if_body, irs);
    if (node->ifelse.else_body) {
        analyze(node->ifelse.else_body, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_while(struct ast *node, struct irstate *irs)
{
    analyze(node->while_loop.cond, irs);
    analyze(node->while_loop.body, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_for(struct ast *node, struct irstate *irs)
{
    analyze(node->for_loop.var, irs);
    analyze(node->for_loop.range, irs);
    analyze(node->for_loop.body, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_call(struct ast *node, struct irstate *irs)
{
    analyze(node->call.func, irs);
    analyze(node->call.args, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_function(struct ast *node, struct irstate *irs)
{
    /* check if function is an un-named function literal */
    if (node->function.name) {
        analyze(node->function.name, irs);
    }
    analyze(node->function.type, irs);
    analyze(node->function.body, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_datalit(struct ast *node, struct irstate *irs)
{
    analyze(node->datalit.name, irs);
    analyze(node->datalit.items, irs);
    return NULL;    /* FIXME */
}

static struct type *analyze_return(struct ast *node, struct irstate *irs)
{
    if (node->ret.expr) {
        analyze(node->ret.expr, irs);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_break(struct ast *node, struct irstate *irs)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_continue(struct ast *node, struct irstate *irs)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_impl(struct ast *node, struct irstate *irs)
{
    analyze(node->impl.name, irs);
    analyze(node->impl.methods, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_data(struct ast *node, struct irstate *irs)
{
    analyze(node->data.name, irs);
    analyze(node->data.members, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_interface(struct ast *node, struct irstate *irs)
{
    analyze(node->interface.name, irs);
    analyze(node->interface.methods, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_alias(struct ast *node, struct irstate *irs)
{
    analyze(node->alias.type, irs);
    analyze(node->alias.name, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_import(struct ast *node, struct irstate *irs)
{
    if (node->import.from) {
        analyze(node->import.from, irs);
    }

    assert(node->import.modules);
    analyze(node->import.modules, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_program(struct ast *node, struct irstate *irs)
{
    analyze(node->program.package, irs);
    analyze(node->program.globals, irs);

    return NULL;    /* FIXME */
}

static struct type *analyze_list(struct ast *node, struct irstate *irs)
{
    struct ast *elem = node->list.head;
    while (elem) {
        struct type *tp = analyze(elem, irs);
        elem = elem->next;
    }

    return NULL;    /* FIXME */
}

static struct type* analyze_listlit(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_maplit(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_globals(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_imports(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_members(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_statements(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_idents(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_methods(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_method_decls(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_decls(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_data_inits(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_params(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static struct type* analyze_args(struct ast *node, struct irstate *irs)
{
    return analyze_list(node, irs);
}

static char *ast_name(struct ast* node)
{
    static char *names[] = {
        "FIRST",

        "BOOL_LIT",
        "CHAR_LIT",
        "INT_NUM",
        "REAL_NUM",
        "STR_LIT",

        "IDENT",

        "QUALIFIED",
        "LIST_TYPE",
        "MAP_TYPE",
        "FUNC_TYPE",

        "DECL",
        "INIT",

        "UNEXPR",
        "BINEXPR",

        "KEYVAL",
        "LOOKUP",
        "SELECTOR",

        "BIND",
        "ASSIGN",
        "IFELSE",
        "WHILE",
        "FOR",
        "CALL",
        "FUNCTION",
        "STRUCTLIT",

        "RETURN",
        "BREAK",
        "CONTINUE",

        "IMPORT",
        "DATA",
        "INTERFACE",
        "ALIAS",
        "IMPORT",
        "PROGRAM",

        "LIST_SENTINEL",

        "LIST_LISTLIT",
        "LIST_MAPLIT",
        "LIST_GLOBALS",
        "LIST_IMPORTS",
        "LIST_MEMBERS",
        "LIST_STATEMENTS",
        "LIST_IDENTS",
        "LIST_METHODS",
        "LIST_METHOD_DECLS",
        "LIST_DECLS",
        "LIST_DATA_INITS",
        "LIST_PARAMS",
        "LIST_ARGS",

        "LAST"
    };

    return names[node->tag];
}

static struct type *analyze(struct ast *root, struct irstate *irs)
{
    static analyzer analyzers[] = {
        NULL, /* not a valid AST node */
        analyze_bool_lit,
        analyze_char_lit,
        analyze_int_num,
        analyze_real_num,
        analyze_str_lit,

        analyze_ident,

        analyze_qualified,
        analyze_list_type,
        analyze_map_type,
        analyze_func_type,

        analyze_decl,
        analyze_init,

        analyze_unexpr,
        analyze_binexpr,

        analyze_keyval,
        analyze_lookup,
        analyze_selector,

        analyze_bind,
        analyze_assign,
        analyze_ifelse,
        analyze_while,
        analyze_for,
        analyze_call,
        analyze_function,
        analyze_datalit,

        analyze_return,
        analyze_break,
        analyze_continue,

        analyze_impl,
        analyze_data,
        analyze_interface,
        analyze_alias,
        analyze_import,
        analyze_program,

        NULL,

        analyze_listlit,
        analyze_maplit,
        analyze_globals,
        analyze_imports,
        analyze_members,
        analyze_statements,
        analyze_idents,
        analyze_methods,
        analyze_method_decls,
        analyze_decls,
        analyze_data_inits,
        analyze_params,
        analyze_args,

        NULL
    };

    /* Check that there are as many analyzers as AST node types */
    assert(sizeof(analyzers) / sizeof(*analyzers) == AST_LAST + 1);

    assert(root);
    /* printf("%s\n", ast_name(root)); */

    analyzer w = analyzers[root->tag];
    assert(w);
    return w(root, irs);
}

void analyze_ast(struct ast *root)
{
    assert(root);

    struct irstate state;
    state.symtable = symtable_create();
    analyze(root, &state);
}
