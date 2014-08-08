#include "nolli.h"
#include "ast.h"
#include "type.h"
#include "strtab.h"
#include "symtable.h"
#include "debug.h"

/* FIXME: need lexer.h to look up tokens */
#include "lexer.h"

#include <string.h>
#include <assert.h>

/** Analysis State:
 *
 *  all_symbols:
 *     package name -> symbols in package
 *  pkgname:
 *     current package name
 *  local_symbols:
 *      symbol table for current scope
 */
struct analysis {
    struct nl_symtable *all_symbols;
    struct string *pkgname;
    /* Convenience pointers to all_symbols[pkgname] */
    struct nl_symtable *local_symbols;
};

int analysis_init(struct analysis *analysis)
{
    analysis->all_symbols = nl_symtable_create(NULL);
    analysis->pkgname = NULL;
    analysis->local_symbols = NULL;

    return NL_NO_ERR;
}

typedef struct nl_type* (*analyzer) (struct nl_ast*, struct analysis*);
typedef void (*collector) (struct nl_ast*, struct analysis*);

static struct nl_type *analyze(struct nl_ast *root, struct analysis *analysis);
static void collect_types(struct nl_ast *root, struct analysis *analysis);

static void collect_types_ident(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_tmpl_type(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_qual_type(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_func_type(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_decl(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_init(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_function(struct nl_ast *node, struct analysis *analysis)
{

}

static void collect_types_class(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast_class *classdef = &node->classdef;

    struct nl_ast *name = classdef->name;

    struct nl_symtable *tmpls = NULL;
    if (classdef->tmpl != NULL) {
        tmpls = nl_symtable_create(NULL);
        struct nl_ast *tmpl = classdef->tmpl->list.head;
        while (tmpl) {
            /* add template to templates symtable */
            tmpl = tmpl->next;
        }
    }

    struct nl_symtable *members = nl_symtable_create(NULL);
    struct nl_ast *mem = classdef->members->list.head;
    while (mem) {
        /* add member to members symtable */
        mem = mem->next;
    }

    struct nl_symtable *methods = nl_symtable_create(NULL);
    struct nl_ast *meth = classdef->methods->list.head;
    while (meth) {
        /* add method to methods symtable */
        meth = meth->next;
    }

    struct nl_type *t = nl_type_new_class(name->s->str, tmpls, members, methods);

    nl_add_symbol(analysis->local_symbols, name->s->str, t);
}

static void collect_types_interface(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast_interface *interface = &node->interface;

    struct nl_ast *name = interface->name;
    /* TODO: template parameters */

    struct nl_symtable *methods = nl_symtable_create(NULL);
    struct nl_ast *meth = interface->methods->list.head;
    while (meth) {
        /* add method to methods symtable */
        meth = meth->next;
    }

    struct nl_type *t = nl_type_new_interface(name->s->str, methods);

    nl_add_symbol(analysis->local_symbols, name->s->str, t);

}

static void collect_types_alias(struct nl_ast *node, struct analysis *analysis)
{
    
}

static void collect_types_using(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_package(struct nl_ast *node, struct analysis *analysis)
{
    collect_types(node->package.globals, analysis);
}

static void collect_types_unit(struct nl_ast *node, struct analysis *analysis)
{
    collect_types(node->unit.globals, analysis);
}

static void collect_types_listlit(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_maplit(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_globals(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *global = node->list.head;
    while (global) {
        collect_types(global, analysis);
        global = global->next;
    }
}

static void collect_types_usings(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_members(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_statements(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_idents(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_types(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_methods(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_method_decls(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_decls(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_class_inits(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_params(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_args(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_packages(struct nl_ast *node, struct analysis *analysis)
{
}

static void collect_types_units(struct nl_ast *node, struct analysis *analysis)
{
}

static struct nl_type *analyze_bool_lit(struct nl_ast *node, struct analysis *analysis)
{
    node->type = &nl_bool_type;
    return node->type;
}

static struct nl_type *analyze_char_lit(struct nl_ast *node, struct analysis *analysis)
{
    node->type = &nl_char_type;
    return node->type;
}

static struct nl_type *analyze_int_num(struct nl_ast *node, struct analysis *analysis)
{
    node->type = &nl_int_type;
    return node->type;
}

static struct nl_type *analyze_real_num(struct nl_ast *node, struct analysis *analysis)
{
    node->type = &nl_real_type;
    return node->type;
}

static struct nl_type *analyze_str_lit(struct nl_ast *node, struct analysis *analysis)
{
    node->type = &nl_str_type;
    return node->type;
}

static struct nl_type *analyze_ident(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_tmpl_type(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *analyze_qual_type(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *analyze_func_type(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *ret = node->func_type.ret_type;
    struct nl_ast *params = node->func_type.params;

    struct nl_type *ret_type = NULL;
    struct nl_type *param_types = NULL;

    if (ret) {
        ret_type = analyze(ret, analysis);
    }

    if (params) {
        struct nl_ast *param = params->list.head;
        struct nl_type *cur = param_types;
        while (param) {
            struct nl_type *t = analyze(params, analysis);

            if (cur == NULL) {
                cur = t;
            } else {
                cur->next = t;
                cur = cur->next;
            }

            param = param->next;
        }
    }

    node->type = nl_type_new_func(ret_type, param_types);
    return node->type;
}

static struct nl_type *analyze_decl(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->decl.type, analysis);
    if (node->decl.rhs) {
        analyze(node->decl.rhs, analysis);
    }

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_init(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->init.ident, analysis);
    analyze(node->init.expr, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_unexpr(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->unexpr.expr, analysis);

    node->type = node->unexpr.expr->type;
    return node->type;    /* FIXME */
}

static struct nl_type *analyze_binexpr(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_type *rhs = analyze(node->binexpr.lhs, analysis);
    struct nl_type *lhs = analyze(node->binexpr.rhs, analysis);
    if (lhs != rhs) {
        NOLLI_ERRORF("Type mismatch in binary expression on line %d", node->lineno);
    }

    if (node->binexpr.op == TOK_EQ || node->binexpr.op == TOK_NEQ ||
            (node->binexpr.op >= TOK_LT && node->binexpr.op <= TOK_AND)) {
        node->type = &nl_bool_type;
    } else {
        node->type = node->binexpr.lhs->type;
    }

    return node->type;
}

static struct nl_type *analyze_keyval(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->keyval.key, analysis);
    analyze(node->keyval.val, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_lookup(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->lookup.container, analysis);
    analyze(node->lookup.index, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_package_ref(struct nl_ast *node, struct analysis *analysis)
{

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_selector(struct nl_ast *node, struct analysis *analysis)
{
    if (node->selector.parent->tag == NL_AST_IDENT) {
    }

    /* lookup type of the selector parent (e.g. package, class) */
    analyze(node->selector.parent, analysis);

    /* lookup type of child in parent */
    analyze(node->selector.child, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_bind(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->bind.ident, analysis);
    analyze(node->bind.expr, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_assign(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->assignment.lhs, analysis);
    analyze(node->assignment.expr, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_ifelse(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *cond = node->ifelse.cond;
    analyze(cond, analysis);
    if (cond->type != &nl_bool_type) {
        NOLLI_ERRORF("Conditional expression must be boolean on line %d", cond->lineno);
    }

    analyze(node->ifelse.if_body, analysis);
    if (node->ifelse.else_body) {
        analyze(node->ifelse.else_body, analysis);
    }

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_while(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->while_loop.cond, analysis);
    analyze(node->while_loop.body, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_for(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->for_loop.var, analysis);
    analyze(node->for_loop.range, analysis);
    analyze(node->for_loop.body, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_call(struct nl_ast *node, struct analysis *analysis)
{
    /* get type of callee */
    struct nl_type *ft = analyze(node->call.func, analysis);
    (void)ft; /* FIXME - avoid warning */

    /* error if not a function type */

    /* compare types of params to args */
    analyze(node->call.args, analysis);

    /* return the return-type of the callee */
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_function(struct nl_ast *node, struct analysis *analysis)
{
    assert(analysis);
    assert(node);

    struct nl_type *ft = analyze(node->function.type, analysis);
    assert(ft);

    /* check if function is an un-named function literal */
    struct nl_ast *name = node->function.name;
    if (name) {
        assert(name->s->str);
        nl_add_symbol(analysis->local_symbols, name->s->str, ft);
    }

    struct nl_symtable *functable = nl_symtable_create(analysis->local_symbols);
    analysis->local_symbols = functable;

    analyze(node->function.body, analysis);
    analysis->local_symbols = functable->parent;

    return ft;
}

static struct nl_type *analyze_classlit(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->classlit.type, analysis);
    if (node->classlit.tmpl) {
        analyze(node->classlit.tmpl, analysis);
    }
    analyze(node->classlit.items, analysis);
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_return(struct nl_ast *node, struct analysis *analysis)
{
    if (node->ret.expr) {
        analyze(node->ret.expr, analysis);
    }

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_break(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_continue(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_class(struct nl_ast *node, struct analysis *analysis)
{
    assert(node->classdef.name->tag == NL_AST_IDENT);

    /* create a new type */
    /* struct nl_type *ct = nl_type_new_class(node->classdef.name->s->str); */

    analyze(node->classdef.name, analysis);

    if (node->classdef.tmpl) {
        analyze(node->classdef.tmpl, analysis);
    }

    analyze(node->classdef.members, analysis);

    analyze(node->classdef.methods, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_interface(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->interface.name, analysis);
    analyze(node->interface.methods, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_alias(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->alias.type, analysis);
    analyze(node->alias.name, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_using(struct nl_ast *node, struct analysis *analysis)
{
    assert(node->usings.names);
    analyze(node->usings.names, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_package(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *name = node->package.name;

    if (name != NULL) {
        assert(name->tag == NL_AST_IDENT);
        analysis->pkgname = name->s;

        struct nl_symtable *local_symbols = nl_check_symbol(analysis->all_symbols,
                name->s->str);
        if (local_symbols == NULL) {
            local_symbols = nl_symtable_create(NULL);
            nl_add_symbol(analysis->all_symbols, name->s->str, local_symbols);
        }
        analysis->local_symbols = local_symbols;
    }

    analyze(node->package.globals, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_unit(struct nl_ast *node, struct analysis *analysis)
{
    analyze(node->unit.globals, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_list(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *elem = node->list.head;
    while (elem) {
        struct nl_type *tp = analyze(elem, analysis);
        (void)tp; /* FIXME - avoid warning */
        elem = elem->next;
    }

    return NULL;    /* FIXME */
}

static struct nl_type* analyze_listlit(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_maplit(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_globals(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *global = node->list.head;
    while (global) {
        analyze(global, analysis);
        global = global->next;
    }

    return NULL; /* FIXME? */
}

static struct nl_type* analyze_usings(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *pkg = node->list.head;
    while (pkg) {
        assert(pkg->tag == NL_AST_IDENT);
        printf("USING pkg %s\n", pkg->s->str);
        pkg = pkg->next;
    }

    return NULL;    /* FIXME? */
}

static struct nl_type* analyze_members(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_statements(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_idents(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_types(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_methods(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_method_decls(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_decls(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_class_inits(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_params(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_args(struct nl_ast *node, struct analysis *analysis)
{
    return analyze_list(node, analysis);
}

static struct nl_type* analyze_packages(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;    /* FIXME */
}

static struct nl_type* analyze_units(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *unit = NULL;

    /*  Make symbol & type tables */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Collect class & interface names */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Collect type aliases */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Collect class & interface types (template, member, method types) */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Collect function signatures */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Collect global declarations */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Analyze global initializations */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    /* Analyze class methods & function bodies */
    unit = node->list.head;
    while (unit) {
        unit = unit->next;
    }

    return NULL;
}

static struct nl_type *analyze(struct nl_ast *root, struct analysis *analysis)
{
    static analyzer analyzers[] = {
        NULL, /* not a valid nl_ast node */
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
        analyze_package,
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
        analyze_packages,
        analyze_units,

        NULL
    };

    /* Check that there are as many analyzers as nl_ast node types */
    assert(sizeof(analyzers) / sizeof(*analyzers) == NL_AST_LAST + 1);

    assert(root);
    /* printf("%s\n", nl_ast_name(root)); */

    analyzer a = analyzers[root->tag];
    assert(a);
    return a(root, analysis);
}

static void collect_types(struct nl_ast *root, struct analysis *analysis)
{
    static collector collectors[] = {
        NULL, /* not a valid nl_ast node */
        NULL, /* collect_types_bool_lit, */
        NULL, /* collect_types_char_lit, */
        NULL, /* collect_types_int_num, */
        NULL, /* collect_types_real_num, */
        NULL, /* collect_types_str_lit, */

        collect_types_ident,

        collect_types_tmpl_type,
        collect_types_qual_type,
        collect_types_func_type,

        collect_types_decl,
        collect_types_init,

        NULL, /* collect_types_unexpr, */
        NULL, /* collect_types_binexpr, */

        NULL, /* collect_types_keyval, */
        NULL, /* collect_types_lookup, */
        NULL, /* collect_types_selector, */
        NULL, /* collect_types_package_ref, */

        NULL, /* collect_types_bind, */
        NULL, /* collect_types_assign, */
        NULL, /* collect_types_ifelse, */
        NULL, /* collect_types_while, */
        NULL, /* collect_types_for, */
        NULL, /* collect_types_call, */
        collect_types_function,
        NULL, /* collect_types_classlit, */

        NULL, /* collect_types_return, */
        NULL, /* collect_types_break, */
        NULL, /* collect_types_continue, */

        collect_types_class,
        collect_types_interface,
        collect_types_alias,
        collect_types_using,
        collect_types_package,
        collect_types_unit,

        NULL,

        collect_types_listlit,
        collect_types_maplit,
        collect_types_globals,
        collect_types_usings,
        collect_types_members,
        collect_types_statements,
        collect_types_idents,
        collect_types_types,
        collect_types_methods,
        collect_types_method_decls,
        collect_types_decls,
        collect_types_class_inits,
        collect_types_params,
        collect_types_args,
        collect_types_packages,
        collect_types_units,

        NULL
    };

    /* Check that there are as many type collectors as nl_ast node types */
    assert(sizeof(collectors) / sizeof(*collectors) == NL_AST_LAST + 1);

    assert(root);
    /* printf("%s\n", nl_ast_name(root)); */

    collector c = collectors[root->tag];
    if (c != NULL) {
        c(root, analysis);
    }
}

int nl_analyze(struct nl_context *ctx)
{
    assert(ctx);

    struct analysis analysis;
    int err = analysis_init(&analysis);
    if (err) {
        return err;
    }

    analyze(ctx->ast_list, &analysis);

    return NL_NO_ERR;
}
