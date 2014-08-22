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
 * Global Package Table:
 *   Package Name -> Package Table
 *   Package Table: 
 *     Type Name -> Type
 *     Package-level Symbol -> Symbol Table
 *     Symbol Table:
 *       Class members
 *       Interface methods
 *       Functions
 */

/** Analysis State:
 *
 *  packages:
 *     package name -> symbols in package
 *  pkgname:
 *     current package name
 *  local_symbols:
 *      symbol table for current scope
 */
struct analysis {
    struct nl_context *ctx;
    struct nl_symtable *packages;
    struct pkgtable *packages_;
    /* Convenience pointers */
    struct nl_string *curpkg;
    struct nl_symtable *local_symbols;
};

struct pkgtable {
    struct nl_symtable *types;
    struct nl_symtable *definitions;
};

const char *NOPKG_NAME = "";

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
    analysis->curpkg = NULL;
    analysis->local_symbols = NULL;

    return NL_NO_ERR;
}

typedef struct nl_type* (*analyzer) (struct nl_ast*, struct analysis*);
typedef void (*collector) (struct nl_ast*, struct analysis*);

static struct nl_type *analyze(struct nl_ast *root, struct analysis *analysis);


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

static void collect_types_globals(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *global = node->list.head;
    while (global) {
        /* collect_types(global, analysis); */
        global = global->next;
    }
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

            if (NULL == cur) {
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
    if (NL_AST_IDENT == node->selector.parent->tag) {
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
        ANALYSIS_ERROR(analysis, cond, "Conditional expression must be boolean");
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
        assert(NL_AST_IDENT == name->tag);
        analysis->curpkg = name->s;

        struct nl_symtable *local_symbols = nl_check_symbol(analysis->packages,
                name->s->str);
        if (NULL == local_symbols) {
            local_symbols = nl_symtable_create(NULL);
            nl_add_symbol(analysis->packages, name->s->str, local_symbols);
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
        assert(NL_AST_IDENT == pkg->tag);
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

static struct pkgtable *make_package_table(struct analysis *analysis,
        const char *name, struct pkgtable *parent)
{
    struct pkgtable *tab = nl_check_symbol(analysis->packages, name);
    if (NULL == tab) {
        tab = nl_alloc(analysis->ctx, sizeof(*tab));
        if (parent != NULL) {
            tab->types = nl_symtable_create(parent->types);
            tab->definitions = nl_symtable_create(parent->definitions);
        } else {
            tab->types = nl_symtable_create(NULL);
            tab->definitions = nl_symtable_create(NULL);
        }
        nl_add_symbol(analysis->packages, name, tab);
    }
    return tab;
}

static void make_package_tables(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = make_package_table(analysis, NOPKG_NAME, NULL);

    /* Add builtin types to the global package table */
    struct nl_symtable *builtin_types = nopkg->types;
    assert(builtin_types != NULL);
    nl_add_symbol(builtin_types, "bool", &nl_bool_type);
    nl_add_symbol(builtin_types, "char", &nl_char_type);
    nl_add_symbol(builtin_types, "int", &nl_int_type);
    nl_add_symbol(builtin_types, "real", &nl_real_type);
    nl_add_symbol(builtin_types, "str", &nl_str_type);

    struct nl_ast *packages = node->unit.packages;
    struct nl_ast *pkg = packages->list.head;
    while (pkg) {
        assert(NL_AST_PACKAGE == pkg->tag);
        struct nl_ast *name = pkg->package.name;
        assert(NL_AST_IDENT == name->tag);
        struct nl_string *str = name->s;

        make_package_table(analysis, str->str, nopkg);

        pkg = pkg->next;
    }
}

static struct nl_type *set_type(struct nl_ast *node,
        struct nl_symtable *types, struct analysis *analysis)
{
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
            tp = nl_check_symbol(types, node->s->str);
            if (NULL == tp) {
                ANALYSIS_ERRORF(analysis, node, "Unknown type %s", node->s->str);
            }
            break;
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
            assert(false);
            break;
    }

    node->type = tp;
    return tp;
}

static void collect_class_type(struct nl_ast_class *classdef,
        struct nl_symtable *typetable, struct analysis *analysis)
{
    struct nl_ast *name = classdef->name;
    assert(NL_AST_IDENT == name->tag);
    if (nl_check_symbol(typetable, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined class %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_type *tp = nl_type_new_class(name->s->str, NULL, NULL, NULL);
        nl_add_symbol(typetable, name->s->str, tp); /* FIXME */
    }
}

static void collect_interface_type(struct nl_ast_class *interface,
        struct nl_symtable *typetable, struct analysis *analysis)
{
    struct nl_ast *name = interface->name;
    assert(NL_AST_IDENT == name->tag);
    if (nl_check_symbol(typetable, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined interface %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_type *tp = nl_type_new_interface(name->s->str, NULL);
        nl_add_symbol(typetable, name->s->str, tp); /* FIXME */
    }
}

static void collect_types(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        switch (global->tag) {
            case NL_AST_CLASS:
                collect_class_type(&global->classdef, nopkg->types, analysis);
                break;
            case NL_AST_INTERFACE:
                collect_interface_type(&global->classdef, nopkg->types, analysis);
                break;
            default: break;
        }
        global = global->next;
    }

    struct nl_ast *packages = node->unit.packages;
    struct nl_ast *pkg = packages->list.head;
    while (pkg) {
        assert(NL_AST_PACKAGE == pkg->tag);
        struct nl_ast *name = pkg->package.name;
        assert(NL_AST_IDENT == name->tag);

        struct pkgtable *pkgtable = nl_check_symbol(analysis->packages, name->s->str);
        assert(pkgtable != NULL);

        struct nl_ast *globals = node->package.globals;
        struct nl_ast *global = globals->list.head;
        while (global) {
            switch (global->tag) {
                case NL_AST_CLASS:
                    collect_class_type(&global->classdef, pkgtable->types, analysis);
                    break;
                case NL_AST_INTERFACE:
                    collect_interface_type(&global->classdef, pkgtable->types, analysis);
                    break;
                default: break;
            }
            global = global->next;
        }

        pkg = pkg->next;
    }
}

static void collect_alias(struct nl_ast_alias *alias,
        struct nl_symtable *types, struct analysis *analysis)
{
    assert(NL_AST_IDENT == alias->name->tag);

    struct nl_ast *name = alias->name;
    assert(NL_AST_IDENT == name->tag);

    if (nl_check_symbol(types, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-defined alias %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_type *tp = set_type(alias->type, types, analysis);
        nl_add_symbol(types, name->s->str, tp);
    }
}

static void collect_aliases(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);
    assert(nopkg->types != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_ALIAS == global->tag) {
            collect_alias(&global->alias, nopkg->types, analysis);
        }
        global = global->next;
    }
}

static void collect_class_definition(struct nl_ast_class *classdef,
        struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *symbols = pkgtable->definitions;
    struct nl_symtable *types = pkgtable->types;
    assert(symbols != NULL);
    assert(types != NULL);

    struct nl_ast *classname = classdef->name;
    assert(NL_AST_IDENT == classname->tag);
    if (nl_check_symbol(symbols, classname->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, classname, "Multiply defined class %s", classname->s->str);
        /* FIXME */
    } else {
        struct nl_symtable *class_symbols = nl_symtable_create(NULL);
        nl_add_symbol(symbols, classname->s->str, class_symbols); /* FIXME */

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
            struct nl_type *tp = set_type(decltype, types, analysis);

            struct nl_ast *rhs = member->decl.rhs;
            if (NL_AST_IDENT == rhs->tag) {
                if (nl_check_symbol(class_symbols, rhs->s->str) != NULL) {
                    ANALYSIS_ERRORF(analysis, rhs, "Re-defined member %s in class %s",
                            rhs->s->str, classname->s->str);
                } else {
                    nl_add_symbol(class_symbols, rhs->s->str, tp);
                }
            } else if (NL_AST_LIST_IDENTS == rhs->tag) {
                struct nl_ast *item = rhs->list.head;
                while (item) {
                    assert(NL_AST_IDENT == item->tag);
                    if (nl_check_symbol(class_symbols, item->s->str) != NULL) {
                        ANALYSIS_ERRORF(analysis, item,
                                "Re-defined member %s in class %s", item->s->str, classname->s->str);
                    } else {
                        nl_add_symbol(class_symbols, item->s->str, tp);
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
            struct nl_type *tp = set_type(method->function.type, types, analysis);
            if (nl_check_symbol(class_symbols, name->s->str) != NULL) {
                ANALYSIS_ERRORF(analysis, name,
                        "Re-defined method %s in class %s", name->s->str, classname->s->str);
                /* FIXME */
            } else {
                nl_add_symbol(class_symbols, name->s->str, tp);
            }
            method = method->next;
        }
    }
}

static void collect_interface_definition(struct nl_ast_interface *interface,
        struct pkgtable *pkgtable, struct analysis *analysis)
{
    assert(pkgtable != NULL);
    struct nl_symtable *symbols = pkgtable->definitions;
    struct nl_symtable *types = pkgtable->types;
    assert(symbols != NULL);
    assert(types != NULL);

    struct nl_ast *interface_name = interface->name;
    assert(NL_AST_IDENT == interface_name->tag);
    if (nl_check_symbol(symbols, interface_name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, interface_name,
                "Multiply defined interface %s near line %d",
                interface_name->s->str, interface_name->lineno);
        /* FIXME */
    } else {
        struct nl_symtable *interface_symbols = nl_symtable_create(NULL);
        nl_add_symbol(symbols, interface_name->s->str, interface_symbols); /* FIXME */

        struct nl_ast *methdecl = interface->methods->list.head;
        while (methdecl) {
            assert(NL_AST_DECL == methdecl->tag);
            struct nl_ast *name = methdecl->decl.rhs;
            assert(NL_AST_IDENT == name->tag);
            struct nl_type *tp = set_type(methdecl->decl.type, types, analysis);
            if (nl_check_symbol(interface_symbols, name->s->str) != NULL) {
                ANALYSIS_ERRORF(analysis, name,
                        "Re-declared method %s in interface %s",
                        name->s->str, interface_name->s->str);
                /* FIXME */
            } else {
                nl_add_symbol(interface_symbols, name->s->str, tp);
            }
            methdecl = methdecl->next;
        }
    }
}

static void collect_type_definitions(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        switch (global->tag) {
            case NL_AST_CLASS:
                collect_class_definition(&global->classdef, nopkg, analysis);
                break;
            case NL_AST_INTERFACE:
                collect_interface_definition(&global->interface, nopkg, analysis);
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

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);
    assert(nopkg->types != NULL);
    assert(nopkg->definitions != NULL);

    if (nl_check_symbol(nopkg->types, name->s->str) != NULL) {
        ANALYSIS_ERRORF(analysis, name, "Re-definition of function %s", name->s->str);
        /* FIXME */
    } else {
        struct nl_ast *ft = func->type;
        assert(NL_AST_FUNC_TYPE == ft->tag);
        struct nl_ast *rt = ft->func_type.ret_type;
        struct nl_ast *params = ft->func_type.params;
        struct nl_type *functype = nl_type_new_func(NULL, NULL);    /* FIXME! */
        nl_add_symbol(nopkg->types, name->s->str, functype);
    }
}

static void collect_function_signatures(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_FUNCTION == global->tag) {
            collect_function_signature(&global->function, nopkg->definitions, analysis);
        }
        global = global->next;
    }
}

static void collect_global_declaration(struct nl_ast_decl *decl,
    struct nl_symtable *symbols, struct analysis *analysis)
{

}

static void collect_global_declarations(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_DECL == global->tag) {
            collect_global_declaration(&global->decl, nopkg->definitions, analysis);
        }
        global = global->next;
    }
}

static void analyze_global_initialization(struct nl_ast_decl *decl,
    struct nl_symtable *symbols, struct analysis *analysis)
{

}

static void analyze_global_initializations(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        if (NL_AST_DECL == global->tag) {
            analyze_global_initialization(&global->decl, nopkg->definitions, analysis);
        }
        global = global->next;
    }
}

static void analyze_function(struct nl_ast_function *func,
    struct nl_symtable *symbols, struct analysis *analysis)
{
    // assert(analysis);
    // assert(node);

    // struct nl_type *ft = analyze(function->type, analysis);
    // assert(ft);

    // /* check if function is an un-named function literal */
    // struct nl_ast *name = function->name;
    // if (name) {
    //     assert(name->s->str);
    //     nl_add_symbol(analysis->local_symbols, name->s->str, ft);
    // }

    // struct nl_symtable *functable = nl_symtable_create(analysis->local_symbols);
    // analysis->local_symbols = functable;

    // analyze(function->body, analysis);
    // analysis->local_symbols = functable->parent;
}

static void analyze_class_methods(struct nl_ast_class *classdef,
    struct nl_symtable *symbols, struct analysis *analysis)
{
    /* create a new type */
    /* struct nl_type *ct = nl_type_new_class(node->classdef.name->s->str); */

    // analyze(classdef->name, analysis);

    // if (classdef->tmpl) {
    //     analyze(classdef->tmpl, analysis);
    // }

    // analyze(classdef->members, analysis);

    // analyze(classdef->methods, analysis);
}

static void analyze_methods_and_functions(struct nl_ast *node, struct analysis *analysis)
{
    assert(NL_AST_UNIT == node->tag);

    struct pkgtable *nopkg = nl_check_symbol(analysis->packages, NOPKG_NAME);
    assert(nopkg != NULL);

    struct nl_ast *globals = node->unit.globals;
    struct nl_ast *global = globals->list.head;
    while (global) {
        switch (global->tag) {
            case NL_AST_FUNCTION:
                analyze_function(&global->function, nopkg->definitions, analysis);
                break;
            case NL_AST_CLASS:
                analyze_class_methods(&global->classdef, nopkg->definitions, analysis);
                break;
            default:break;
        }
        global = global->next;
    }
}

static struct nl_type* analyze_units(struct nl_ast *node, struct analysis *analysis)
{
    struct nl_ast *unit = NULL;

    /*  Make symbol & type tables */
    unit = node->list.head;
    while (unit) {
        make_package_tables(unit, analysis);
        unit = unit->next;
    }

    /* Collect class & interface names */
    unit = node->list.head;
    while (unit) {
        collect_types(unit, analysis);
        unit = unit->next;
    }

    /* Collect type aliases */
    unit = node->list.head;
    while (unit) {
        collect_aliases(unit, analysis);
        unit = unit->next;
    }

    /* Collect class & interface types (template, member, method types) */
    unit = node->list.head;
    while (unit) {
        collect_type_definitions(unit, analysis);
        unit = unit->next;
    }

    /* Collect function signatures */
    unit = node->list.head;
    while (unit) {
        collect_function_signatures(unit, analysis);
        unit = unit->next;
    }

    /* Collect global declarations */
    unit = node->list.head;
    while (unit) {
        collect_global_declarations(unit, analysis);
        unit = unit->next;
    }

    /* Analyze global initializations */
    unit = node->list.head;
    while (unit) {
        analyze_global_initializations(unit, analysis);
        unit = unit->next;
    }

    /* Analyze class methods & function bodies */
    unit = node->list.head;
    while (unit) {
        analyze_methods_and_functions(unit, analysis);
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
        NULL,
        analyze_classlit,

        analyze_return,
        analyze_break,
        analyze_continue,

        NULL,
        NULL,
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