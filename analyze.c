#include "analyze.h"

struct analysis {
    struct symtable *packages;
    struct string *pkgname;
    struct symtable *curtable;
};

static struct symtable *new_scope(struct analysis *analysis)
{
    return symtable_create(analysis->curtable);
}


typedef struct type* (*analyzer) (struct ast*, struct analysis*);

static struct type *analyze(struct ast *root, struct analysis *analysis);


static struct type *analyze_bool_lit(struct ast *node, struct analysis *analysis)
{
    node->type = &bool_type;
    return node->type;
}

static struct type *analyze_char_lit(struct ast *node, struct analysis *analysis)
{
    node->type = &char_type;
    return node->type;
}

static struct type *analyze_int_num(struct ast *node, struct analysis *analysis)
{
    node->type = &int_type;
    return node->type;
}

static struct type *analyze_real_num(struct ast *node, struct analysis *analysis)
{
    node->type = &real_type;
    return node->type;
}

static struct type *analyze_str_lit(struct ast *node, struct analysis *analysis)
{
    node->type = &str_type;
    return node->type;
}

static struct type *analyze_ident(struct ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_tmpl_type(struct ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct type *analyze_qual_type(struct ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct type *analyze_func_type(struct ast *node, struct analysis *analysis)
{
    if (node->func_type.ret_type) {
        analyze(node->func_type.ret_type, analysis);
    }
    if (node->func_type.params) {
        analyze(node->func_type.params, analysis);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_decl(struct ast *node, struct analysis *analysis)
{
    analyze(node->decl.type, analysis);
    if (node->decl.rhs) {
        analyze(node->decl.rhs, analysis);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_init(struct ast *node, struct analysis *analysis)
{
    analyze(node->init.ident, analysis);
    analyze(node->init.expr, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_unexpr(struct ast *node, struct analysis *analysis)
{
    analyze(node->unexpr.expr, analysis);

    node->type = node->unexpr.expr->type;
    return node->type;    /* FIXME */
}

static struct type *analyze_binexpr(struct ast *node, struct analysis *analysis)
{
    struct type *rhs = analyze(node->binexpr.lhs, analysis);
    struct type *lhs = analyze(node->binexpr.rhs, analysis);
    if (node->binexpr.lhs->type != node->binexpr.rhs->type) {
        NOLLI_ERRORF("Type mismatch in binary expression on line %d", node->lineno);
    }

    if (node->binexpr.op == TOK_EQ || node->binexpr.op == TOK_NEQ ||
            (node->binexpr.op >= TOK_LT && node->binexpr.op <= TOK_AND)) {
        node->type = &bool_type;
    } else {
        node->type = node->binexpr.lhs->type;
    }

    return node->type;
}

static struct type *analyze_keyval(struct ast *node, struct analysis *analysis)
{
    analyze(node->keyval.key, analysis);
    analyze(node->keyval.val, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_lookup(struct ast *node, struct analysis *analysis)
{
    analyze(node->lookup.container, analysis);
    analyze(node->lookup.index, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_package_ref(struct ast *node, struct analysis *analysis)
{

    return NULL;    /* FIXME */
}

static struct type *analyze_selector(struct ast *node, struct analysis *analysis)
{
    if (node->selector.parent->tag == AST_IDENT) {
        printf("selector: %s\n", node->selector.parent->s->str);
    }

    /* lookup type of the selector parent (e.g. package, class) */
    analyze(node->selector.parent, analysis);

    /* lookup type of child in parent */
    analyze(node->selector.child, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_bind(struct ast *node, struct analysis *analysis)
{
    analyze(node->bind.ident, analysis);
    analyze(node->bind.expr, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_assign(struct ast *node, struct analysis *analysis)
{
    analyze(node->assignment.lhs, analysis);
    analyze(node->assignment.expr, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_ifelse(struct ast *node, struct analysis *analysis)
{
    struct ast *cond = node->ifelse.cond;
    analyze(cond, analysis);
    if (cond->type != &bool_type) {
        NOLLI_ERRORF("Conditional expression must be boolean on line %d", cond->lineno);
    }

    analyze(node->ifelse.if_body, analysis);
    if (node->ifelse.else_body) {
        analyze(node->ifelse.else_body, analysis);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_while(struct ast *node, struct analysis *analysis)
{
    analyze(node->while_loop.cond, analysis);
    analyze(node->while_loop.body, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_for(struct ast *node, struct analysis *analysis)
{
    analyze(node->for_loop.var, analysis);
    analyze(node->for_loop.range, analysis);
    analyze(node->for_loop.body, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_call(struct ast *node, struct analysis *analysis)
{
    /* get type of callee */
    struct type *ft = analyze(node->call.func, analysis);

    /* error if not a function type */

    /* compare types of params to args */
    analyze(node->call.args, analysis);

    /* return the return-type of the callee */
    return NULL;    /* FIXME */
}

static struct type *analyze_function(struct ast *node, struct analysis *analysis)
{
    struct type *ft = analyze(node->function.type, analysis);

    /* check if function is an un-named function literal */
    if (node->function.name) {
        add_symbol(analysis->curtable, node->function.name->s->str, ft);
    }

    struct symtable *functable = new_scope(analysis);
    analysis->curtable = functable;
    analyze(node->function.body, analysis);
    analysis->curtable = functable->parent;

    return ft;
}

static struct type *analyze_classlit(struct ast *node, struct analysis *analysis)
{
    analyze(node->classlit.name, analysis);
    if (node->classlit.tmpl) {
        analyze(node->classlit.tmpl, analysis);
    }
    analyze(node->classlit.items, analysis);
    return NULL;    /* FIXME */
}

static struct type *analyze_return(struct ast *node, struct analysis *analysis)
{
    if (node->ret.expr) {
        analyze(node->ret.expr, analysis);
    }

    return NULL;    /* FIXME */
}

static struct type *analyze_break(struct ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_continue(struct ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct type *analyze_class(struct ast *node, struct analysis *analysis)
{
    assert(node->classdef.name->tag == AST_IDENT);

    /* create a new type */
    struct type *ct = type_new_class(node->classdef.name->s->str);

    analyze(node->classdef.name, analysis);

    if (node->classdef.tmpl) {
        analyze(node->classdef.tmpl, analysis);
    }

    analyze(node->classdef.members, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_interface(struct ast *node, struct analysis *analysis)
{
    analyze(node->interface.name, analysis);
    analyze(node->interface.methods, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_alias(struct ast *node, struct analysis *analysis)
{
    analyze(node->alias.type, analysis);
    analyze(node->alias.name, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_using(struct ast *node, struct analysis *analysis)
{
    assert(node->usings.names);
    analyze(node->usings.names, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_unit(struct ast *node, struct analysis *analysis)
{
    struct ast *pkg = node->unit.package;
    assert(pkg->tag == AST_IDENT);
    analysis->pkgname = pkg->s;

    struct symtable *package_table = new_scope(analysis);
    /* Add this package symbol to the top-level "package table" */
    add_symbol(analysis->packages, pkg->s->str, package_table);
    analysis->curtable = package_table;

    analyze(node->unit.globals, analysis);

    return NULL;    /* FIXME */
}

static struct type *analyze_list(struct ast *node, struct analysis *analysis)
{
    struct ast *elem = node->list.head;
    while (elem) {
        struct type *tp = analyze(elem, analysis);
        elem = elem->next;
    }

    return NULL;    /* FIXME */
}

static struct type* analyze_listlit(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_maplit(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_globals(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_usings(struct ast *node, struct analysis *analysis)
{
    struct ast *pkg = node->list.head;
    while (pkg) {
        assert(pkg->tag == AST_IDENT);

        /* DELETE THIS CRAP */
        char fname[1024];
        memset(fname, 0, 1024);
        strcat(fname, pkg->s->str);
        strcat(fname, ".nl");
        printf("LOADING pkg %s\n", fname);
        struct ast *root = compile_file(fname);

        pkg = pkg->next;
    }

    return NULL;    /* FIXME? */
}

static struct type* analyze_members(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_statements(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_idents(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_types(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_methods(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_method_decls(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_decls(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_class_inits(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_params(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct type* analyze_args(struct ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static char *ast_name(struct ast* node)
{
    static char *names[] = {
        "FanalysisT",

        "BOOL_LIT",
        "CHAR_LIT",
        "INT_NUM",
        "REAL_NUM",
        "STR_LIT",

        "IDENT",

        "TMPL_TYPE",
        "QUAL_TYPE",
        "FUNC_TYPE",

        "DECL",
        "INIT",

        "UNEXPR",
        "BINEXPR",

        "KEYVAL",
        "LOOKUP",
        "SELECTOR",
        "PACKAGEREF",

        "BIND",
        "ASSIGN",
        "IFELSE",
        "WHILE",
        "FOR",
        "CALL",
        "FUNCTION",
        "CLASSLIT",

        "RETURN",
        "BREAK",
        "CONTINUE",

        "CLASS",
        "INTERFACE",
        "ALIAS",
        "USING",
        "UNIT",

        "LIST_SENTINEL",

        "LIST_LISTLIT",
        "LIST_MAPLIT",
        "LIST_GLOBALS",
        "LIST_USINGS",
        "LIST_MEMBERS",
        "LIST_STATEMENTS",
        "LIST_IDENTS",
        "LIST_METHODS",
        "LIST_METHOD_DECLS",
        "LIST_DECLS",
        "LIST_CLASS_INITS",
        "LIST_PARAMS",
        "LIST_ARGS",

        "LAST"
    };

    return names[node->tag];
}

static struct type *analyze(struct ast *root, struct analysis *analysis)
{
    static analyzer analyzers[] = {
        NULL, /* not a valid AST node */
        analyze_bool_lit,
        analyze_char_lit,
        analyze_int_num,
        analyze_real_num,
        analyze_str_lit,

        analyze_ident,

        analyze_tmpl_type,
        analyze_qual_type,
        analyze_func_type,

        analyze_decl,
        analyze_init,

        analyze_unexpr,
        analyze_binexpr,

        analyze_keyval,
        analyze_lookup,
        analyze_selector,
        analyze_package_ref,

        analyze_bind,
        analyze_assign,
        analyze_ifelse,
        analyze_while,
        analyze_for,
        analyze_call,
        analyze_function,
        analyze_classlit,

        analyze_return,
        analyze_break,
        analyze_continue,

        analyze_class,
        analyze_interface,
        analyze_alias,
        analyze_using,
        analyze_unit,

        NULL,

        analyze_listlit,
        analyze_maplit,
        analyze_globals,
        analyze_usings,
        analyze_members,
        analyze_statements,
        analyze_idents,
        analyze_types,
        analyze_methods,
        analyze_method_decls,
        analyze_decls,
        analyze_class_inits,
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
    return w(root, analysis);
}

void analyze_ast(struct ast *root)
{
    assert(root);

    struct analysis analysis;
    analysis.packages = symtable_create(NULL);
    /* analysis.curtable = analysis.packages; */
    analyze(root, &analysis);
}
