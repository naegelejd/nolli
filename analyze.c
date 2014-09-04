#include "nolli.h"
#include "ast.h"
#include "type.h"
#include "strtab.h"
#include "symtable.h"
#include "debug.h"

/* FIXME: need lexer.h to look up tokens */
#include "lexer.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Semantic Analysis
 *
 * Need:
 *     map of package names to package table
 *     for each package:
 *         map of typenames to types
 *         map of symbols to types (including functions)
 *         map of class names to class table (methods + members)
 *         map of interface names to interface table (methods)
 *
 * A package's types, classes and interfaces are constant
 * A package's symbols are variable across scopes
 *
 * Make global "package" table
 * For each unit:
 *     Make package table
 *     Collect class and interface names into typetable
 *     Collect alias into typetable
 *     Collect class/interface contents in class/interface tables
 *     Collect function signatures in symbol table
 *     Collect global declarations in symbol table
 *     Analyze global initializations
 *     Analyze function/method bodies
 *
 */

/**
 * While collecting global types and symbols:
 *
 *  - access to nl_context and other package types/symbols
 *  - access to current AST node
 *  - access to current package's types/symbols
 *
 * While analyzing functions:
 *
 *  - access to nl_context and other packages types/symbols
 *  - access to current AST node
 *  - access to current packages's types
 *  - access to current function's symbols
 *
 * While analyzing individual statements:
 *
 *  - access to nl_context and other packages types/symbols
 *  - access to current AST node
 *  - access to current packages's types
 *  - access to symbols in current scope
 *  - access to return type of current function and
 *    whether the current statement is inside a loop
 *
 * While analyzing individual expressions:
 *
 *  - access to nl_context and other packages types/symbols
 *  - access to current AST node
 *  - access to current packages's types
 *  - access to current scope's symbols
 */

/** Analysis State */
struct analysis {
    struct nl_context *ctx;
    struct nl_symtable *packages;
};

/** Collection of types/symbols for a package */
struct pkgtable {
    struct nl_symtable *type_names;     /**< Typenames -> types */
    struct nl_symtable *type_tables;    /**< Typenames -> typetable */
    struct nl_symtable *symbols;        /**< Symbols -> types */
};

struct func_info {
    struct nl_type *ret_type;
    bool inloop;
};

const char *NL_GLOBAL_PACKAGE_NAME = "";

#define ANALYSIS_ERRORF(A, n, fmt, ...) \
    NL_ERRORF((A)->ctx, NL_ERR_ANALYZE, fmt " near line %d", __VA_ARGS__, (n)->lineno)
#define ANALYSIS_ERROR(A, n, ...) ANALYSIS_ERRORF(A, n, "%s", __VA_ARGS__)


static struct nl_type *expr_set_type(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis);
static void analyze_statement(struct nl_ast *stmt,
        struct nl_symtable *symbols, struct nl_symtable *types,
        struct func_info *func_info, struct analysis *analysis);


static int analysis_init(struct analysis *analysis, struct nl_context *ctx)
{
    assert(analysis != NULL);
    assert(ctx != NULL);

    memset(analysis, 0, sizeof(*analysis));

    analysis->ctx = ctx;
    analysis->packages = nl_symtable_create(NULL);

    return NL_NO_ERR;
}


static struct nl_type *expr_get_type_bool_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return &nl_bool_type;
}

static struct nl_type *expr_get_type_char_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return &nl_char_type;
}

static struct nl_type *expr_get_type_int_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return &nl_int_type;
}

static struct nl_type *expr_get_type_real_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return &nl_real_type;
}

static struct nl_type *expr_get_type_str_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return &nl_str_type;
}

static struct nl_type* expr_get_type_list_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type* expr_get_type_map_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *expr_get_type_class_lit(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    // analyze(node->class_lit.type, analysis);
    if (node->class_lit.tmpl) {
        // analyze(node->class_lit.tmpl, analysis);
    }
    // analyze(node->class_lit.items, analysis);
    return NULL;    /* FIXME */
}


static struct nl_type *expr_get_type_ident(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    struct nl_type *tp = nl_symtable_search(symbols, node->s);
    if (NULL == tp) {
        ANALYSIS_ERRORF(analysis, node, "Unknown symbol %s", node->s);
        return NULL;
    }
    return tp;
}

static struct nl_type *expr_get_type_unexpr(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return expr_set_type(node->unexpr.expr, symbols, types, analysis);
}

static struct nl_type *expr_get_type_binexpr(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    struct nl_type *lhs_type = expr_set_type(node->binexpr.lhs, symbols, types, analysis);
    struct nl_type *rhs_type = expr_set_type(node->binexpr.rhs, symbols, types, analysis);

    if (!nl_types_equal(lhs_type, rhs_type)) {
        ANALYSIS_ERROR(analysis, node->binexpr.lhs, "Type mismatch in binary expression");
        return NULL;
    }

    struct nl_type *tp = NULL;
    if (TOK_EQ == node->binexpr.op || TOK_NEQ == node->binexpr.op ||
            (TOK_LT >= node->binexpr.op && TOK_AND <= node->binexpr.op)) {
        tp = &nl_bool_type;
    } else {
        tp = lhs_type;
    }
    return tp;
}

static struct nl_type *expr_get_type_call(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    assert(NL_AST_CALL == node->tag);

    struct nl_type *tp = expr_set_type(node->call.func, symbols, types, analysis);
    if (NULL == tp || tp->tag != NL_TYPE_FUNC) {
        /* TODO: invalid function in "call" */
    } else {
        /* TODO: compare argument types with parameter types */

        tp = tp->func.ret_type;
    }
    return tp;
}

static struct nl_type *expr_get_type_keyval(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *expr_get_type_lookup(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *expr_get_type_selector(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *expr_get_type_packageref(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *expr_get_type_function(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *expr_set_type(struct nl_ast *node,
        struct nl_symtable *symbols, struct nl_symtable *types, struct analysis *analysis)
{
    assert(node != NULL);

    typedef struct nl_type* (*expression_typer)(struct nl_ast*, struct nl_symtable*,
            struct nl_symtable*, struct analysis*);

    static expression_typer typers[] = {
        expr_get_type_bool_lit,
        expr_get_type_char_lit,
        expr_get_type_int_lit,
        expr_get_type_real_lit,
        expr_get_type_str_lit,
        expr_get_type_list_lit,
        expr_get_type_map_lit,
        expr_get_type_class_lit,
        expr_get_type_ident,
        expr_get_type_unexpr,
        expr_get_type_binexpr,
        expr_get_type_call,
        expr_get_type_keyval,
        expr_get_type_lookup,
        expr_get_type_selector,
        expr_get_type_packageref,
        expr_get_type_function,
    };
    assert(sizeof(typers) / sizeof(*typers) == NL_AST_FUNCTION - NL_AST_BOOL_LIT + 1);

    struct nl_type *tp = NULL;

    tp = typers[node->tag - NL_AST_BOOL_LIT](node, symbols, types, analysis);

    if (tp == NULL) {
        ANALYSIS_ERROR(analysis, node, "Bad type in expression");
    }

    node->type = tp;
    return tp;
}

static struct nl_type *set_type(struct nl_ast *node,
        struct nl_symtable *types, struct analysis *analysis)
{
    assert(node != NULL);
    assert(types != NULL);

    assert(node != NULL);
    struct nl_type *tp = NULL;

    switch(node->tag) {
        case NL_AST_IDENT:
            tp = nl_symtable_search(types, node->s);
            if (NULL == tp) {
                ANALYSIS_ERRORF(analysis, node, "Unknown type %s", node->s);
            }
            break;
        case NL_AST_TMPL_TYPE:
            tp = &nl_tmpl_placeholder_type;  /* FIXME */
            break;
        case NL_AST_QUAL_TYPE: {
            struct nl_ast *pkgname = node->package_ref.package;
            assert(NL_AST_IDENT == pkgname->tag);
            struct nl_ast *name = node->package_ref.name;
            assert(NL_AST_IDENT == name->tag);

            struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, pkgname->s);
            if (NULL == pkgtable) {
                ANALYSIS_ERRORF(analysis, node, "Unknown package %s", pkgname->s);
                /* FIXME */
            } else {
                tp = nl_symtable_search(types, name->s);
                if (NULL == tp) {
                    printf("Making new type reference %s:%s\n", pkgname->s, name->s);
                    tp = nl_type_new_reference(pkgname->s, name->s);
                }
            }
            break;
        }
        case NL_AST_FUNC_TYPE:
            tp = nl_type_new_func(NULL, NULL);  /* FIXME! */
            break;
        case NL_AST_CLASS:
            tp = nl_type_new_class(node->classdef.name->s, NULL, NULL, NULL);  /* FIXME! */
            break;
        case NL_AST_INTERFACE:
            tp = nl_type_new_interface(node->interface.name->s, NULL);  /* FIXME! */
            break;
        default:
            printf("%s is not yet handled in %s\n", nl_ast_name(node), __func__);
            assert(false);
            break;
    }

    node->type = tp;
    return tp;
}

static void analyze_decl(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_DECL == node->tag);

    /* TODO: handle kind of decl - var/const */
    struct nl_type *tp = set_type(node->decl.type, types, analysis);

    struct nl_ast *rhs = node->decl.rhs;
    assert(NL_AST_IDENT == rhs->tag);   /* FIXME: handle declaration list */

    /* printf("adding decl %s:%p to %p\n", rhs->s, tp, parent_symbols); */
    nl_symtable_add(parent_symbols, rhs->s, tp);
}

static void analyze_init(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
}

static void analyze_bind(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_BIND == node->tag);
    struct nl_ast *name = node->bind.ident;
    assert(NL_AST_IDENT == name->tag);
    struct nl_ast *expr = node->bind.expr;

    /* Check if name is in current scope's symbol table.
        If so, it is multiply defined. */
    struct nl_type *tp = nl_symtable_search(parent_symbols, name->s);
    if (tp != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-bound symbol %s", name->s);
    } else {
        struct nl_type *tp = expr_set_type(expr, parent_symbols, types, analysis);
        nl_symtable_add(parent_symbols, name->s, tp);
    }
}

static void analyze_assign(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_ASSIGN == node->tag);
    struct nl_ast *lhs = node->assignment.lhs;
    /* TODO: handle assignments to containers */
    assert(NL_AST_IDENT == lhs->tag);

    struct nl_ast *expr = node->assignment.expr;

    /* Check if name is in current scope's symbol table.
        If NOT, it can't be assigned to! */
    struct nl_type *expr_type = nl_symtable_search(parent_symbols, lhs->s);
    if (NULL == expr_type) {
        ANALYSIS_ERRORF(analysis, lhs, "Can't assign to undeclared symbol %s", lhs->s);
    } else {
        struct nl_type *existing_type = expr_set_type(expr, parent_symbols, types, analysis);
        if (!nl_types_equal(existing_type, expr_type)) {
            ANALYSIS_ERROR(analysis, node, "Mismatch of types in assignment");
        }
    }
}

static void analyze_while(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_WHILE == node->tag);
    struct nl_ast *cond = node->while_loop.cond;
    struct nl_ast *body = node->while_loop.body;
    assert(cond != NULL);
    assert(body != NULL);
    assert(NL_AST_LIST_STATEMENTS == body->tag);

    struct nl_type *cond_type = expr_set_type(cond, parent_symbols, types, analysis);
    if (cond_type != &nl_bool_type) {
        ANALYSIS_ERROR(analysis, cond, "While-loop requires boolean conditional expression");
        return;
    }

    struct nl_symtable *symbols = nl_symtable_create(parent_symbols);

    func_info->inloop = true;
    struct nl_ast *stmt = body->list.head;
    while (stmt) {
        analyze_statement(stmt, symbols, types, func_info, analysis);
        stmt = stmt->next;
    }
    func_info->inloop = false;
}

static void analyze_for(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_FOR == node->tag);
    struct nl_ast *var = node->for_loop.var;
    struct nl_ast *range = node->for_loop.range;
    struct nl_ast *body = node->for_loop.body;
    assert(var != NULL);
    assert(range != NULL);
    assert(body != NULL);
    assert(NL_AST_IDENT == var->tag);
    assert(NL_AST_LIST_STATEMENTS == body->tag);

    /* printf("checking table %p for symbol %s\n", parent_symbols, range->s); */
    struct nl_type *range_type = expr_set_type(range, parent_symbols, types, analysis);
    /* TODO: check that range type is a container?? */

    struct nl_symtable *symbols = nl_symtable_create(parent_symbols);
    nl_symtable_add(symbols, var->s, range_type);

    func_info->inloop = true;
    struct nl_ast *stmt = body->list.head;
    while (stmt) {
        analyze_statement(stmt, symbols, types, func_info, analysis);
        stmt = stmt->next;
    }
    func_info->inloop = false;
}

static void analyze_ifelse(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_IFELSE == node->tag);
    struct nl_ast *cond = node->ifelse.cond;
    struct nl_ast *if_body = node->ifelse.if_body;
    struct nl_ast *else_body = node->ifelse.else_body;
    assert(cond != NULL);
    assert(if_body != NULL);
    assert(else_body != NULL);
    assert(NL_AST_LIST_STATEMENTS == if_body->tag);
    assert(NL_AST_LIST_STATEMENTS == else_body->tag);

    struct nl_type *cond_type = expr_set_type(cond, parent_symbols, types, analysis);
    if (cond_type != &nl_bool_type) {
        ANALYSIS_ERROR(analysis, cond, "If statement requires boolean conditional expression");
        return;
    }

    struct nl_symtable *if_symbols = nl_symtable_create(parent_symbols);
    struct nl_ast *true_stmt = if_body->list.head;
    while (true_stmt) {
        analyze_statement(true_stmt, if_symbols, types, func_info, analysis);
        true_stmt = true_stmt->next;
    }

    struct nl_symtable *else_symbols = nl_symtable_create(parent_symbols);
    struct nl_ast *false_stmt = if_body->list.head;
    while (false_stmt) {
        analyze_statement(false_stmt, else_symbols, types, func_info, analysis);
        false_stmt = false_stmt->next;
    }
}

static void analyze_call_stmt(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(NL_AST_CALL_STMT == node->tag);
    struct nl_ast *func = node->call.func;
    struct nl_ast *args = node->call.args;
    assert(func != NULL);
    assert(args != NULL);
    assert(NL_AST_LIST_ARGS == args->tag);

    struct nl_type *callee_type = expr_set_type(func, parent_symbols, types, analysis);
    if (callee_type->tag != NL_TYPE_FUNC) {
        ANALYSIS_ERROR(analysis, func, "Attempt to call something that isn't a function");
        return;
    }

    /* TODO: ensure the number of args matches the number of parameters */
    struct nl_ast *arg = args->list.head;
    struct nl_type *param_type = callee_type->func.param_types_head;
    while (arg && param_type) {
        struct nl_type *arg_type = expr_set_type(arg, parent_symbols, types, analysis);
        if (!nl_types_equal(arg_type, param_type)) {
            ANALYSIS_ERROR(analysis, node, "Mismatch of types in return");
        }

        arg = arg->next;
        param_type = param_type->next;
    }
}

static void analyze_return(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    if (node->ret.expr != NULL) {
        struct nl_type *tp = expr_set_type(node->ret.expr, parent_symbols, types, analysis);
        if (!nl_types_equal(tp, func_info->ret_type)) {
            ANALYSIS_ERROR(analysis, node, "Mismatch of types in return");
        }
    }
}

static void analyze_break(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    if (!func_info->inloop) {
        ANALYSIS_ERROR(analysis, node, "Cannot `break` outside of a loop");
    }
}

static void analyze_continue(struct nl_ast *node, struct nl_symtable *parent_symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    if (!func_info->inloop) {
        ANALYSIS_ERROR(analysis, node, "Cannot `continue` outside of a loop");
    }
}

static void analyze_statement(struct nl_ast *stmt, struct nl_symtable *symbols,
        struct nl_symtable *types, struct func_info *func_info, struct analysis *analysis)
{
    assert(stmt != NULL);
    assert(symbols != NULL);
    assert(types != NULL);
    assert(func_info != NULL);

    typedef void (*statement_analyzer)(struct nl_ast*, struct nl_symtable*,
            struct nl_symtable*, struct func_info*, struct analysis*);

    static statement_analyzer analyzers[] = {
        analyze_decl,
        analyze_init,
        analyze_bind,
        analyze_assign,
        analyze_ifelse,
        analyze_while,
        analyze_for,
        analyze_call_stmt,
        analyze_return,
        analyze_break,
        analyze_continue
    };
    assert(sizeof(analyzers) / sizeof(*analyzers) == (NL_AST_CONTINUE - NL_AST_DECL + 1));

    assert(stmt->tag >= NL_AST_DECL && stmt->tag <= NL_AST_RETURN);
    size_t idx = stmt->tag - NL_AST_DECL;
    analyzers[idx](stmt, symbols, types, func_info, analysis);
}

#if 0
static struct nl_type *analyze_using(struct nl_ast *node, struct analysis *analysis)
{
    assert(node->usings.names);
    // analyze(node->usings.names, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type* analyze_usings(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *pkg = node->list.head;
    while (pkg) {
        assert(NL_AST_IDENT == pkg->tag);
        printf("USING pkg %s\n", pkg->s);
        pkg = pkg->next;
    }

    return NULL;    /* FIXME? */
}
#endif

static struct pkgtable *make_package_table(nl_string_t name,
        struct pkgtable *parent, struct analysis *analysis)
{
    assert(name != NULL);
    struct pkgtable *tab = nl_symtable_get(analysis->packages, name);
    assert(NULL == tab);
    NL_DEBUGF(analysis->ctx, "Making package table %s", name);
    tab = nl_alloc(analysis->ctx, sizeof(*tab));
    if (parent != NULL) {
        tab->type_names = nl_symtable_create(parent->type_names);
        tab->type_tables = nl_symtable_create(parent->type_tables);
        tab->symbols = nl_symtable_create(parent->symbols);
    } else {
        tab->type_names = nl_symtable_create(NULL);
        tab->type_tables = nl_symtable_create(NULL);
        tab->symbols = nl_symtable_create(NULL);
    }
    nl_symtable_add(analysis->packages, name, tab);
    return tab;
}

static void collect_class_type(struct nl_ast_class *classdef,
        struct nl_symtable *typetable, struct analysis *analysis)
{
    assert(classdef != NULL);
    assert(typetable != NULL);
    struct nl_ast *name = classdef->name;
    assert(NL_AST_IDENT == name->tag);
    if (nl_symtable_get(typetable, name->s) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined class %s", name->s);
        /* FIXME */
    } else {
        struct nl_type *tp = nl_type_new_class(name->s, NULL, NULL, NULL);
        nl_symtable_add(typetable, name->s, tp); /* FIXME */
    }
}

static void collect_interface_type(struct nl_ast_class *interface,
        struct nl_symtable *typetable, struct analysis *analysis)
{
    assert(interface != NULL);
    assert(typetable != NULL);
    struct nl_ast *name = interface->name;
    assert(NL_AST_IDENT == name->tag);
    if (nl_symtable_get(typetable, name->s) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined interface %s", name->s);
        /* FIXME */
    } else {
        struct nl_type *tp = nl_type_new_interface(name->s, NULL);
        nl_symtable_add(typetable, name->s, tp); /* FIXME */
    }
}

static void collect_types(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);
    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        switch (global->tag) {
            case NL_AST_CLASS:
                collect_class_type(&global->classdef, pkgtable->type_names, analysis);
                break;
            case NL_AST_INTERFACE:
                collect_interface_type(&global->classdef, pkgtable->type_names, analysis);
                break;
            default: break;
        }
        global = global->next;
    }
}

static void collect_alias(struct nl_ast_alias *alias,
        struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    assert(pkgtable->type_names != NULL);

    assert(NL_AST_IDENT == alias->name->tag);

    struct nl_ast *name = alias->name;
    assert(NL_AST_IDENT == name->tag);

    if (nl_symtable_get(pkgtable->type_names, name->s) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined alias %s", name->s);
        /* FIXME */
    } else {
        struct nl_type *tp = set_type(alias->type, pkgtable->type_names, analysis);
        if (NULL == tp) {
            ANALYSIS_ERRORF(analysis, name, "Invalid type in alias %s", name->s);
        }
        nl_symtable_add(pkgtable->type_names, name->s, tp);
    }
}

static void collect_aliases(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_ALIAS == global->tag) {
            collect_alias(&global->alias, pkgtable, analysis);
        }
        global = global->next;
    }
}

static void collect_class_definition(struct nl_ast_class *classdef,
        struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *type_tables = pkgtable->type_tables;
    assert(type_tables != NULL);

    struct nl_ast *classname = classdef->name;
    assert(NL_AST_IDENT == classname->tag);
    if (nl_symtable_get(type_tables, classname->s) != NULL) {
        ANALYSIS_ERRORF(analysis, classname, "Multiply defined class %s", classname->s);
        /* FIXME */
        return;
    }
    struct nl_symtable *class_symbols = nl_symtable_create(NULL);
    nl_symtable_add(type_tables, classname->s, class_symbols); /* FIXME */

    if (classdef->tmpl != NULL) {
        struct nl_ast *tmpl = classdef->tmpl->list.head;
        while (tmpl) {
            assert(NL_AST_IDENT == tmpl->tag);
            /* TODO: handle templates on class */
            tmpl = tmpl->next;
        }
    }

    struct nl_ast *member = classdef->members->list.head;
    while (member) {
        assert(NL_AST_DECL == member->tag);
        struct nl_ast *decltype = member->decl.type;
        struct nl_type *tp = set_type(decltype, pkgtable->type_names, analysis);

        struct nl_ast *rhs = member->decl.rhs;
        if (NL_AST_IDENT == rhs->tag) {
            if (nl_symtable_get(class_symbols, rhs->s) != NULL) {
                ANALYSIS_ERRORF(analysis, rhs, "Re-defined member %s in class %s",
                        rhs->s, classname->s);
            } else {
                nl_symtable_add(class_symbols, rhs->s, tp);
            }
        } else if (NL_AST_LIST_IDENTS == rhs->tag) {
            struct nl_ast *item = rhs->list.head;
            while (item) {
                assert(NL_AST_IDENT == item->tag);
                if (nl_symtable_get(class_symbols, item->s) != NULL) {
                    ANALYSIS_ERRORF(analysis, item,
                            "Re-defined member %s in class %s", item->s, classname->s);
                } else {
                    nl_symtable_add(class_symbols, item->s, tp);
                }
                item = item->next;
            }
        } else {
            fprintf(stdout, "%s\n", nl_ast_name(rhs));
            assert(false); /* FIXME - ERROR */
        }

        member = member->next;
    }

    struct nl_ast *method = classdef->methods->list.head;
    while (method) {
        assert(NL_AST_FUNCTION == method->tag);
        struct nl_ast *name = method->function.name;
        assert(NL_AST_IDENT == name->tag);
        struct nl_type *tp = set_type(method->function.type, pkgtable->type_names, analysis);
        if (nl_symtable_get(class_symbols, name->s) != NULL) {
            ANALYSIS_ERRORF(analysis, name,
                    "Re-defined method %s in class %s", name->s, classname->s);
            /* FIXME */
        } else {
            nl_symtable_add(class_symbols, name->s, tp);
        }
        method = method->next;
    }
}

static void collect_interface_definition(struct nl_ast_interface *interface,
        struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *type_tables = pkgtable->type_tables;
    assert(type_tables != NULL);

    struct nl_ast *interface_name = interface->name;
    assert(NL_AST_IDENT == interface_name->tag);
    if (nl_symtable_get(type_tables, interface_name->s) != NULL) {
        ANALYSIS_ERRORF(analysis, interface_name,
                "Multiply defined interface %s near line %d",
                interface_name->s, interface_name->lineno);
        /* FIXME */
        return;
    }

    struct nl_symtable *interface_symbols = nl_symtable_create(NULL);
    nl_symtable_add(type_tables, interface_name->s, interface_symbols); /* FIXME */

    struct nl_ast *methdecl = interface->methods->list.head;
    while (methdecl) {
        assert(NL_AST_DECL == methdecl->tag);
        struct nl_ast *name = methdecl->decl.rhs;
        assert(NL_AST_IDENT == name->tag);
        struct nl_type *tp = set_type(methdecl->decl.type, pkgtable->type_names, analysis);
        if (nl_symtable_get(interface_symbols, name->s) != NULL) {
            ANALYSIS_ERRORF(analysis, name,
                    "Re-declared method %s in interface %s",
                    name->s, interface_name->s);
            /* FIXME */
        } else {
            nl_symtable_add(interface_symbols, name->s, tp);
        }
        methdecl = methdecl->next;
    }
}

static void collect_type_definitions(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        switch (global->tag) {
            case NL_AST_CLASS:
                collect_class_definition(&global->classdef, pkgtable, analysis);
                break;
            case NL_AST_INTERFACE:
                collect_interface_definition(&global->interface, pkgtable, analysis);
                break;
            default: break;
        }
        global = global->next;
    }
}

static void collect_function_signature(struct nl_ast_function *func,
    struct pkgtable *pkgtable, struct analysis *analysis)
{
    struct nl_ast *name = func->name;
    assert(NL_AST_IDENT == name->tag);

    assert(pkgtable != NULL);
    assert(pkgtable->type_names != NULL);
    assert(pkgtable->symbols != NULL);

    if (nl_symtable_get(pkgtable->symbols, name->s) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-definition of function %s", name->s);
        /* FIXME */
    } else {
        struct nl_ast *ft = func->type;
        assert(NL_AST_FUNC_TYPE == ft->tag);
        struct nl_type *rt = set_type(ft->func_type.ret_type, pkgtable->type_names, analysis);

        /* struct nl_ast *params = ft->func_type.params; */
        /* TODO: convert params to list of types */

        struct nl_type *functype = nl_type_new_func(rt, NULL);    /* FIXME: pass list of types */
        nl_symtable_add(pkgtable->symbols, name->s, functype);
    }
}

static void collect_function_signatures(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_FUNCTION == global->tag) {
            collect_function_signature(&global->function, pkgtable, analysis);
        }
        global = global->next;
    }
}

static void collect_global_declaration(struct nl_ast_decl *decl,
    struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *symbols = pkgtable->symbols;
    struct nl_symtable *types = pkgtable->type_names;
    assert(symbols != NULL);
    assert(types != NULL);

    struct nl_type *tp = set_type(decl->type, types, analysis);
    struct nl_ast *rhs = decl->rhs;

    if (NL_AST_IDENT == rhs->tag) {
        if (nl_symtable_get(symbols, rhs->s) != NULL) {
            ANALYSIS_ERRORF(analysis, rhs, "Re-defined symbol %s", rhs->s);
        } else {
            nl_symtable_add(symbols, rhs->s, tp);
        }
    } else if (NL_AST_INIT == rhs->tag) {
        struct nl_ast *sym = rhs->init.ident;
        assert(NL_AST_IDENT == sym->tag);
        if (nl_symtable_get(symbols, sym->s)) {
            ANALYSIS_ERRORF(analysis, sym, "Re-defined symbol %s", sym->s);
        } else {
            nl_symtable_add(symbols, sym->s, tp);
        }
    } else {
        printf("%s\n", nl_ast_name(rhs));
        assert(false);
    }
}

static void collect_global_declarations(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_DECL == global->tag) {
            collect_global_declaration(&global->decl, pkgtable, analysis);
        }
        global = global->next;
    }
}

static void resolve_references(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_symtable *type_names = pkgtable->type_names;
    assert(type_names != NULL);

    struct nl_symbol *sym = type_names->head;
    while (sym != NULL) {
        nl_string_t name = sym->name;
        struct nl_type *ref_type = (struct nl_type*)sym->value;
        if (ref_type != NULL && NL_TYPE_REFERENCE == ref_type->tag) {
            nl_string_t package_name = ref_type->reference.package_name;
            nl_string_t type_name = ref_type->reference.type_name;
            printf("Resolving type %s from %s::%s\n", name, package_name, type_name);
            struct pkgtable *ref_pkgtable = nl_symtable_get(analysis->packages, package_name);
            assert(ref_pkgtable != NULL);   /* FIXME: if NULL then package doesn't exist */
            struct nl_type *tp = nl_symtable_search(ref_pkgtable->type_names, type_name);
            /* FIXME: resolve indirect reference etc. */
            assert(tp != NULL && NL_TYPE_REFERENCE != tp->tag);
            /* TODO: cleanup reference */
            nl_symtable_add(pkgtable->type_names, name, tp);
        }
        sym = (struct nl_symbol *)sym->next;
    }
}

static void analyze_global_initialization(struct nl_ast_decl *decl,
    struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *symbols = pkgtable->symbols;
    struct nl_symtable *types = pkgtable->type_names;
    assert(symbols != NULL);
    assert(types != NULL);

    struct nl_ast *rhs = decl->rhs;
    if (NL_AST_INIT == rhs->tag) {

        struct nl_ast *name = rhs->init.ident;
        assert(NL_AST_IDENT == name->tag);

        struct nl_type *tp = nl_symtable_search(symbols, name->s);
        assert(tp != NULL);

        /* Analyze the entire right-hand-side of the initialization */
        struct nl_type *rhs_tp = expr_set_type(rhs->init.expr, symbols, types, analysis);

        if (tp != rhs_tp) {
            ANALYSIS_ERRORF(analysis, name, "Type mismatch in initialization of %s", name->s);
        /* FIXME */
        }
    }
}

static void analyze_global_initializations(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_DECL == global->tag) {
            analyze_global_initialization(&global->decl, pkgtable, analysis);
        }
        global = global->next;
    }
}

static void analyze_function(struct nl_ast_function *func,
    struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    assert(pkgtable->symbols != NULL);

    /* TODO: create new symbol table for function scope */
    struct nl_symtable *symbols = nl_symtable_create(pkgtable->symbols);
    struct nl_symtable *types = pkgtable->type_names;

    struct nl_ast *ft = func->type;
    assert(NL_AST_FUNC_TYPE == ft->tag);

    struct nl_type *ret_type = set_type(ft->func_type.ret_type, pkgtable->type_names, analysis);
    struct func_info func_info = {.ret_type=ret_type, .inloop=false};

    assert(ft->func_type.params != NULL);
    struct nl_ast *param = ft->func_type.params->list.head;
    while (param) {
        assert(NL_AST_DECL == param->tag);
        struct nl_ast *rhs = param->decl.rhs;
        assert(NL_AST_IDENT == rhs->tag);   /* FIXME - parameters can be "init"s too */
        struct nl_type *tp = set_type(param->decl.type, pkgtable->type_names, analysis);
        /* printf("adding symbol %s to table %p for function %s\n", rhs->s, symbols, func->name->s); */
        nl_symtable_add(symbols, rhs->s, tp);
        param = param->next;
    }

    struct nl_ast *body = func->body;
    assert(NL_AST_LIST_STATEMENTS == body->tag);
    struct nl_ast *stmt = body->list.head;
    while (stmt) {
        analyze_statement(stmt, symbols, types, &func_info, analysis);
        stmt = stmt->next;
    }
}

static void analyze_class_methods(struct nl_ast_class *classdef,
    struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *symbols = pkgtable->symbols;
    struct nl_symtable *types = pkgtable->type_names;
    assert(symbols != NULL);
    assert(types != NULL);

    /* Make scope for function... its symbols should include the class's members/methods. */
}

static void analyze_methods_and_functions(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_get(analysis->packages, name->s);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        switch (global->tag) {
            case NL_AST_FUNCTION:
                analyze_function(&global->function, pkgtable, analysis);
                break;
            case NL_AST_CLASS:
                analyze_class_methods(&global->classdef, pkgtable, analysis);
                break;
            default:break;
        }
        global = global->next;
    }
}

static void join_packages(struct nl_ast *packages, struct analysis *analysis)
{
    assert(NL_AST_LIST_PACKAGES == packages->tag);

    /* Join packages of the same name */
    struct nl_ast *pkg = packages->list.head;
    while (pkg && pkg->next) {
        struct nl_ast *pkg_name = pkg->package.name;
        assert(NL_AST_IDENT == pkg_name->tag);
        struct nl_ast *pkg_globals = pkg->package.globals;
        assert(NL_AST_LIST_GLOBALS == pkg_globals->tag);

        struct nl_ast *prev = pkg;
        struct nl_ast *cur = pkg->next;
        while (cur) {
            struct nl_ast *cur_name = cur->package.name;
            assert(NL_AST_IDENT == cur_name->tag);
            if (pkg_name->s == cur_name->s) {
                NL_DEBUGF(analysis->ctx, "Joining package %s", pkg_name->s);
                struct nl_ast *cur_globals = cur->package.globals;
                assert(NL_AST_LIST_GLOBALS == cur_globals->tag);
                if (cur_globals->list.head != NULL) {
                    nl_ast_list_append(pkg_globals, cur_globals->list.head);
                    /* TODO: cleanup next_pkg->package.globals */
                }

                prev->next = cur->next;
                /* TODO: cleanup cur */
                cur = prev->next;
            } else {
                prev = cur;
                cur = cur->next;
            }
        }
        pkg = pkg->next;
    }
}

static struct nl_ast *find_global_package(struct nl_ast *packages)
{
    assert(NL_AST_LIST_PACKAGES == packages->tag);

    struct nl_ast *cur = packages->list.head;
    struct nl_ast *ret = NULL;
    while (cur != NULL) {
        struct nl_ast *cur_name = cur->package.name;
        assert(NL_AST_IDENT == cur_name->tag);

        if (strcmp(cur_name->s, NL_GLOBAL_PACKAGE_NAME) == 0) {
            ret = cur;
            break;
        }
        cur = cur->next;
    }
    return ret;
}

static void analyze(struct nl_ast *node, struct analysis *analysis)
{
    /* Collect all globals and packages from each unit */
    struct nl_ast *packages = nl_ast_make_list(NL_AST_LIST_PACKAGES, -1);

    assert(NL_AST_LIST_UNITS == node->tag);

    struct nl_ast *unit = node->list.head;
    while (unit) {
        struct nl_ast *ps = unit->unit.packages;
        assert(ps != NULL);
        assert(NL_AST_LIST_PACKAGES == ps->tag);

        struct nl_ast *p = ps->list.head;
        packages = nl_ast_list_append(packages, p);

        /* TODO: cleanup unit->unit.packages */
        unit->unit.packages = NULL;
        unit = unit->next;
    }

    join_packages(packages, analysis);

    struct nl_ast *gpkg = find_global_package(packages);
    struct pkgtable *gpkgtable = NULL;
    assert(gpkg != NULL);
    assert(NL_AST_PACKAGE == gpkg->tag);
    struct nl_ast *name = gpkg->package.name;
    assert(NL_AST_IDENT == name->tag);
    gpkgtable = make_package_table(name->s, NULL, analysis);
    /* Add builtin types to the global package table */
    NL_DEBUG(analysis->ctx, "Adding builtin types");
    struct nl_symtable *builtin_types = gpkgtable->type_names;
    assert(builtin_types != NULL);
    nl_symtable_add(builtin_types, nl_strtab_wrap(analysis->ctx->strtab, "bool"), &nl_bool_type);
    nl_symtable_add(builtin_types, nl_strtab_wrap(analysis->ctx->strtab, "char"), &nl_char_type);
    nl_symtable_add(builtin_types, nl_strtab_wrap(analysis->ctx->strtab, "int"), &nl_int_type);
    nl_symtable_add(builtin_types, nl_strtab_wrap(analysis->ctx->strtab, "real"), &nl_real_type);
    nl_symtable_add(builtin_types, nl_strtab_wrap(analysis->ctx->strtab, "str"), &nl_str_type);

    /*  Make remaining package tables */
    struct nl_ast *pkg = packages->list.head;
    while (pkg) {
        if (pkg != gpkg) {
            struct nl_ast *name = pkg->package.name;
            assert(NL_AST_IDENT == name->tag);
            make_package_table(name->s, gpkgtable, analysis);
        }
        pkg = pkg->next;
    }

    /* Collect classes, interfaces, aliases, function signatures, then declarations */
    pkg = packages->list.head;
    while (pkg) {
        collect_types(pkg, analysis);
        collect_aliases(pkg, analysis);
        collect_type_definitions(pkg, analysis);
        collect_function_signatures(pkg, analysis);
        collect_global_declarations(pkg, analysis);
        pkg = pkg->next;
    }

    /* Resolve package references */
    pkg = packages->list.head;
    while (pkg) {
        resolve_references(pkg, analysis);
        pkg = pkg->next;
    }

    /* Analyze code */
    pkg = packages->list.head;
    while (pkg) {
        analyze_global_initializations(pkg, analysis);
        analyze_methods_and_functions(pkg, analysis);
        pkg = pkg->next;
    }
}

int nl_analyze(struct nl_context *ctx)
{
    assert(ctx);

    struct analysis analysis;
    int err = analysis_init(&analysis, ctx);
    if (err) {
        return err;
    }

    analyze(ctx->ast_list, &analysis);

    return NL_NO_ERR;
}