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

const char *NL_GLOBAL_PACKAGE_NAME = "";

#define ANALYSIS_ERRORF(A, n, fmt, ...) \
    NL_ERRORF((A)->ctx, NL_ERR_ANALYZE, fmt " near line %d", __VA_ARGS__, (n)->lineno)
#define ANALYSIS_ERROR(A, n, ...) ANALYSIS_ERRORF(A, n, "%s", __VA_ARGS__)

static int analysis_init(struct analysis *analysis, struct nl_context *ctx)
{
    assert(analysis != NULL);
    assert(ctx != NULL);

    memset(analysis, 0, sizeof(*analysis));

    analysis->ctx = ctx;
    analysis->packages = nl_symtable_create(NULL);

    return NL_NO_ERR;
}

typedef struct nl_type* (*analyzer) (struct nl_ast*, struct analysis*);


static struct nl_type *set_type(struct nl_ast *node,
        struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *symbols = pkgtable->symbols;
    struct nl_symtable *type_names = pkgtable->type_names;
    assert(symbols != NULL);
    assert(type_names != NULL);

    assert(node != NULL);
    struct nl_type *tp = NULL;

    switch(node->tag) {
        case NL_AST_BOOL_LIT:
            tp = &nl_bool_type;
            break;
        case NL_AST_CHAR_LIT:
            tp = &nl_char_type;
            break;
        case NL_AST_INT_NUM:
            tp = &nl_int_type;
            break;
        case NL_AST_REAL_NUM:
            tp = &nl_real_type;
            break;
        case NL_AST_STR_LIT:
            tp = &nl_str_type;
            break;
        case NL_AST_IDENT:
            /* TODO: check symbols or type_names first? */
            tp = nl_symtable_check(symbols, node->s->str);
            if (NULL == tp) {
                tp = nl_symtable_check(type_names, node->s->str);
                if (NULL == tp) {
                    ANALYSIS_ERRORF(analysis, node, "Unknown type %s", node->s->str);
                }
            }
            break;
        case NL_AST_QUAL_TYPE: {
            struct nl_ast *pkgname = node->package_ref.package;
            assert(NL_AST_IDENT == pkgname->tag);
            struct nl_ast *name = node->package_ref.name;
            assert(NL_AST_IDENT == name->tag);

            struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, pkgname->s->str);
            if (NULL == pkgtable) {
                ANALYSIS_ERRORF(analysis, node, "Unknown package %s", pkgname->s->str);
                /* FIXME */
            } else {
                tp = nl_symtable_check(pkgtable->type_names, name->s->str);
                if (NULL == tp) {
                    printf("Making new type reference %s:%s\n", pkgname->s->str, name->s->str);
                    tp = nl_type_new_reference(pkgname->s, name->s);
                }
            }
            break;
        }
        case NL_AST_FUNC_TYPE:
            tp = nl_type_new_func(NULL, NULL);  /* FIXME! */
            break;
        case NL_AST_CLASS:
            tp = nl_type_new_class(node->classdef.name->s->str, NULL, NULL, NULL);  /* FIXME! */
            break;
        case NL_AST_INTERFACE:
            tp = nl_type_new_interface(node->interface.name->s->str, NULL);  /* FIXME! */
            break;
        default:
            printf("%s is not yet handled in set_type\n", nl_ast_name(node));
            assert(false);
            break;
    }

    node->type = tp;
    return tp;
}



static struct nl_type *analyze_unexpr(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *analyze_binexpr(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_type *rhs = set_type(node->binexpr.lhs, NULL, analysis);
    struct nl_type *lhs = set_type(node->binexpr.rhs, NULL, analysis);
    if (lhs != rhs) {
        ANALYSIS_ERROR(analysis, node, "Type mismatch in binary expression");
    }

    if (TOK_EQ == node->binexpr.op || TOK_NEQ == node->binexpr.op ||
            (TOK_LT >= node->binexpr.op && TOK_AND <= node->binexpr.op)) {
        node->type = &nl_bool_type;
    } else {
        node->type = node->binexpr.lhs->type;
    }

    return node->type;
}

static struct nl_type *analyze_assign(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *analyze_while(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *analyze_for(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type *analyze_ifelse(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *cond = node->ifelse.cond;
    struct nl_type *tp = set_type(cond, NULL, analysis);
    if (tp != &nl_bool_type) {
        ANALYSIS_ERROR(analysis, cond, "Conditional expression must be boolean");
    }

    // analyze(node->ifelse.if_body, analysis);
    // if (node->ifelse.else_body) {
    //     analyze(node->ifelse.else_body, analysis);
    // }

    return NULL;    /* FIXME */
}

static struct nl_type *analyze_call(struct nl_ast *node, struct analysis *analysis)
{
    /* get type of callee */
    // struct nl_type *ft = analyze(node->call.func, analysis);
    // (void)ft; /* FIXME - avoid warning */

    /* error if not a function type */

    /* compare types of params to args */
    // analyze(node->call.args, analysis);

    /* return the return-type of the callee */
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_classlit(struct nl_ast *node, struct analysis *analysis)
{
    // analyze(node->classlit.type, analysis);
    if (node->classlit.tmpl) {
        // analyze(node->classlit.tmpl, analysis);
    }
    // analyze(node->classlit.items, analysis);
    return NULL;    /* FIXME */
}

static struct nl_type *analyze_return(struct nl_ast *node, struct analysis *analysis)
{
    if (node->ret.expr) {
        // analyze(node->ret.expr, analysis);
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

static struct nl_type *analyze_using(struct nl_ast *node, struct analysis *analysis)
{
    assert(node->usings.names);
    // analyze(node->usings.names, analysis);

    return NULL;    /* FIXME */
}

static struct nl_type* analyze_listlit(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type* analyze_maplit(struct nl_ast *node, struct analysis *analysis)
{
    return NULL;
}

static struct nl_type* analyze_usings(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *pkg = node->list.head;
    while (pkg) {
        assert(NL_AST_IDENT == pkg->tag);
        printf("USING pkg %s\n", pkg->s->str);
        pkg = pkg->next;
    }

    return NULL;    /* FIXME? */
}

static struct pkgtable *make_package_table(struct nl_string *name,
        struct pkgtable *parent, struct analysis *analysis)
{
    assert(name != NULL);
    struct pkgtable *tab = nl_symtable_check(analysis->packages, name->str);
    assert(NULL == tab);
    NL_DEBUGF(analysis->ctx, "Making package table %s", name->str);
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
    nl_symtable_add(analysis->packages, name->str, tab);
    return tab;
}

static void collect_class_type(struct nl_ast_class *classdef,
        struct nl_symtable *typetable, struct analysis *analysis)
{
    assert(classdef != NULL);
    assert(typetable != NULL);
    struct nl_ast *name = classdef->name;
    assert(NL_AST_IDENT == name->tag);
    /* TODO: this will check global types table as well, which is incorrect */
    if (nl_symtable_check(typetable, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined class %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_type *tp = nl_type_new_class(name->s->str, NULL, NULL, NULL);
        nl_symtable_add(typetable, name->s->str, tp); /* FIXME */
    }
}

static void collect_interface_type(struct nl_ast_class *interface,
        struct nl_symtable *typetable, struct analysis *analysis)
{
    assert(interface != NULL);
    assert(typetable != NULL);
    struct nl_ast *name = interface->name;
    assert(NL_AST_IDENT == name->tag);
    /* TODO: this will check global types table as well, which is incorrect */
    if (nl_symtable_check(typetable, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined interface %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_type *tp = nl_type_new_interface(name->s->str, NULL);
        nl_symtable_add(typetable, name->s->str, tp); /* FIXME */
    }
}

static void collect_types(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);
    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
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

    /* TODO: this will check global types table as well, which is incorrect */
    if (nl_symtable_check(pkgtable->type_names, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined alias %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_type *tp = set_type(alias->type, pkgtable, analysis);
        if (NULL == tp) {
            ANALYSIS_ERRORF(analysis, name, "Invalid type in alias %s", name->s->str);
        }
        nl_symtable_add(pkgtable->type_names, name->s->str, tp);
    }
}

static void collect_aliases(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
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
    if (nl_symtable_check(type_tables, classname->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, classname, "Multiply defined class %s", classname->s->str);
        /* FIXME */
        return;
    }
    struct nl_symtable *class_symbols = nl_symtable_create(NULL);
    nl_symtable_add(type_tables, classname->s->str, class_symbols); /* FIXME */

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
        struct nl_type *tp = set_type(decltype, pkgtable, analysis);

        struct nl_ast *rhs = member->decl.rhs;
        if (NL_AST_IDENT == rhs->tag) {
            if (nl_symtable_check(class_symbols, rhs->s->str) != NULL) {
                ANALYSIS_ERRORF(analysis, rhs, "Re-defined member %s in class %s",
                        rhs->s->str, classname->s->str);
            } else {
                nl_symtable_add(class_symbols, rhs->s->str, tp);
            }
        } else if (NL_AST_LIST_IDENTS == rhs->tag) {
            struct nl_ast *item = rhs->list.head;
            while (item) {
                assert(NL_AST_IDENT == item->tag);
                if (nl_symtable_check(class_symbols, item->s->str) != NULL) {
                    ANALYSIS_ERRORF(analysis, item,
                            "Re-defined member %s in class %s", item->s->str, classname->s->str);
                } else {
                    nl_symtable_add(class_symbols, item->s->str, tp);
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
        struct nl_type *tp = set_type(method->function.type, pkgtable, analysis);
        if (nl_symtable_check(class_symbols, name->s->str) != NULL) {
            ANALYSIS_ERRORF(analysis, name,
                    "Re-defined method %s in class %s", name->s->str, classname->s->str);
            /* FIXME */
        } else {
            nl_symtable_add(class_symbols, name->s->str, tp);
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
    if (nl_symtable_check(type_tables, interface_name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, interface_name,
                "Multiply defined interface %s near line %d",
                interface_name->s->str, interface_name->lineno);
        /* FIXME */
        return;
    }

    struct nl_symtable *interface_symbols = nl_symtable_create(NULL);
    nl_symtable_add(type_tables, interface_name->s->str, interface_symbols); /* FIXME */

    struct nl_ast *methdecl = interface->methods->list.head;
    while (methdecl) {
        assert(NL_AST_DECL == methdecl->tag);
        struct nl_ast *name = methdecl->decl.rhs;
        assert(NL_AST_IDENT == name->tag);
        struct nl_type *tp = set_type(methdecl->decl.type, pkgtable, analysis);
        if (nl_symtable_check(interface_symbols, name->s->str) != NULL) {
            ANALYSIS_ERRORF(analysis, name,
                    "Re-declared method %s in interface %s",
                    name->s->str, interface_name->s->str);
            /* FIXME */
        } else {
            nl_symtable_add(interface_symbols, name->s->str, tp);
        }
        methdecl = methdecl->next;
    }
}

static void collect_type_definitions(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
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
    struct nl_symtable *symbols, struct analysis *analysis)
{
    struct nl_ast *name = func->name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *nopkg = nl_symtable_check(analysis->packages, NL_GLOBAL_PACKAGE_NAME);
    assert(nopkg != NULL);
    assert(nopkg->type_names != NULL);
    assert(nopkg->symbols != NULL);

    if (nl_symtable_check(nopkg->type_names, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-definition of function %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_ast *ft = func->type;
        assert(NL_AST_FUNC_TYPE == ft->tag);
        struct nl_ast *rt = ft->func_type.ret_type;
        struct nl_ast *params = ft->func_type.params;
        struct nl_type *functype = nl_type_new_func(NULL, NULL);    /* FIXME! */
        nl_symtable_add(nopkg->type_names, name->s->str, functype);
    }
}

static void collect_function_signatures(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
    assert(pkgtable != NULL);

    struct nl_ast *globals = node->package.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_FUNCTION == global->tag) {
            collect_function_signature(&global->function, pkgtable->symbols, analysis);
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

    struct nl_type *tp = set_type(decl->type, pkgtable, analysis);
    struct nl_ast *rhs = decl->rhs;

    if (NL_AST_IDENT == rhs->tag) {
        if (nl_symtable_check(symbols, rhs->s->str) != NULL) {
            ANALYSIS_ERRORF(analysis, rhs, "Re-defined symbol %s", rhs->s->str);
        } else {
            nl_symtable_add(symbols, rhs->s->str, tp);
        }
    } else if (NL_AST_INIT == rhs->tag) {
        struct nl_ast *sym = rhs->init.ident;
        assert(NL_AST_IDENT == sym->tag);
        if (nl_symtable_check(symbols, sym->s->str)) {
            ANALYSIS_ERRORF(analysis, sym, "Re-defined symbol %s", sym->s->str);
        } else {
            nl_symtable_add(symbols, sym->s->str, tp);
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

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
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

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
    assert(pkgtable != NULL);

    struct nl_symtable *type_names = pkgtable->type_names;
    assert(type_names != NULL);

    unsigned int i;
    for (i = 0; i < type_names->size; i++) {
        if (type_names->keys[i] != NULL) {
            char *name = type_names->keys[i];
            if (type_names->vals[i] != NULL) {
                struct nl_type *ref_type = type_names->vals[i];
                if (NL_TYPE_REFERENCE == ref_type->tag) {
                    struct nl_string *package_name = ref_type->reference.package_name;
                    struct nl_string *type_name = ref_type->reference.type_name;
                    printf("Resolving type %s from %s::%s\n", name, package_name->str, type_name->str);
                    struct pkgtable *ref_pkgtable = nl_symtable_check(analysis->packages, package_name->str);
                    assert(ref_pkgtable != NULL);   /* FIXME: if NULL then package doesn't exist */
                    struct nl_type *tp = nl_symtable_check(ref_pkgtable->type_names, type_name->str);
                    /* FIXME: resolve indirect reference etc. */
                    assert(tp != NULL && NL_TYPE_REFERENCE != tp->tag);
                    /* TODO: cleanup reference */
                    nl_symtable_add(pkgtable->type_names, name, tp);
                }
            }
        }
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

        struct nl_type *tp = nl_symtable_check(symbols, name->s->str);
        assert(tp != NULL);

        /* Analyze the entire right-hand-side of the initialization */
        struct nl_type *rhs_tp = set_type(rhs->init.expr, pkgtable, analysis);

        if (tp != rhs_tp) {
            ANALYSIS_ERRORF(analysis, name, "Type mismatch in initialization of %s", name->s->str);
        /* FIXME */
        }
    }
}

static void analyze_global_initializations(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_PACKAGE == node->tag);

    struct nl_ast *name = node->package.name;
    assert(NL_AST_IDENT == name->tag);

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
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

static void analyze_statement(struct nl_ast *stmt,
        struct pkgtable *pkgtable, struct analysis *analysis)
{

}

static void analyze_function(struct nl_ast_function *func,
    struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    assert(pkgtable->symbols != NULL);

    /* create symbol table for function scope */
    struct nl_symtable *symbols = nl_symtable_create(pkgtable->symbols);

    struct nl_ast *body = func->body;
    assert(NL_AST_LIST_STATEMENTS == body->tag);
    struct nl_ast *stmt = body->list.head;
    while (stmt) {
        analyze_statement(stmt, pkgtable, analysis);
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

    struct pkgtable *pkgtable = nl_symtable_check(analysis->packages, name->s->str);
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
                NL_DEBUGF(analysis->ctx, "Joining package %s", pkg_name->s->str);
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

        if (strcmp(cur_name->s->str, NL_GLOBAL_PACKAGE_NAME) == 0) {
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
    nl_symtable_add(builtin_types, "bool", &nl_bool_type);
    nl_symtable_add(builtin_types, "char", &nl_char_type);
    nl_symtable_add(builtin_types, "int", &nl_int_type);
    nl_symtable_add(builtin_types, "real", &nl_real_type);
    nl_symtable_add(builtin_types, "str", &nl_str_type);

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

    static analyzer analyzers[] = {
        NULL, /* not a valid nl_ast node */
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

        analyze_unexpr,
        analyze_binexpr,

        NULL,
        NULL,
        NULL,
        NULL,

        NULL,
        analyze_assign,
        analyze_ifelse,
        analyze_while,
        analyze_for,
        analyze_call,
        NULL,
        analyze_classlit,

        analyze_return,
        analyze_break,
        analyze_continue,

        NULL, NULL, NULL,
        analyze_using,
        NULL, NULL, NULL,

        analyze_listlit,
        analyze_maplit,
        NULL,
        analyze_usings,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL
    };

    /* Check that there are as many analyzers as nl_ast node types */
    assert(sizeof(analyzers) / sizeof(*analyzers) == NL_AST_LAST + 1);
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