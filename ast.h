#ifndef NOLLI_AST_H
#define NOLLI_AST_H

#include <stdbool.h>

enum {
    NL_AST_FIRST,

    /* expressions */
    NL_AST_BOOL_LIT,
    NL_AST_CHAR_LIT,
    NL_AST_INT_LIT,
    NL_AST_REAL_LIT,
    NL_AST_STR_LIT,
    NL_AST_LIST_LIT,
    NL_AST_MAP_LIT,
    NL_AST_CLASS_LIT,
    NL_AST_IDENT,
    NL_AST_UNEXPR,
    NL_AST_BINEXPR,
    NL_AST_CALL,
    NL_AST_KEYVAL,
    NL_AST_LOOKUP,
    NL_AST_SELECTOR,
    NL_AST_PACKAGE_REF,
    NL_AST_FUNCTION,

    /* non-identifier types */
    NL_AST_TMPL_TYPE,
    NL_AST_QUAL_TYPE,
    NL_AST_FUNC_TYPE,

    /* statements */
    NL_AST_DECL,
    NL_AST_INIT,
    NL_AST_BIND,
    NL_AST_ASSIGN,
    NL_AST_IFELSE,
    NL_AST_WHILE,
    NL_AST_FOR,
    NL_AST_CALL_STMT,
    NL_AST_RETURN,
    NL_AST_BREAK,
    NL_AST_CONTINUE,

    /* package-level-only statements */
    NL_AST_ALIAS,
    NL_AST_USING,

    /* collections of ASTs */
    NL_AST_CLASS,
    NL_AST_INTERFACE,
    NL_AST_PACKAGE,
    NL_AST_UNIT,

    NL_AST_LIST_SENTINEL,  /* never used */

    /* linked-lists */
    NL_AST_LIST_IDENTS,
    NL_AST_LIST_TYPES,
    NL_AST_LIST_PARAMS,
    NL_AST_LIST_ARGS,
    NL_AST_LIST_MEMBERS,
    NL_AST_LIST_METHODS,
    NL_AST_LIST_METHOD_DECLS,
    NL_AST_LIST_CLASS_INITS,
    NL_AST_LIST_DECLS,
    NL_AST_LIST_STATEMENTS,
    NL_AST_LIST_USINGS,
    NL_AST_LIST_GLOBALS,
    NL_AST_LIST_PACKAGES,
    NL_AST_LIST_UNITS,

    NL_AST_LAST
};

enum {
    NL_DECL_VAR,
    NL_DECL_CONST
};

struct nl_ast_tmpl_type {
    struct nl_ast *name, *tmpls;
};

struct nl_ast_qual_type {
    struct nl_ast *package, *name;
};

struct nl_ast_func_type {
    struct nl_ast *ret_type, *params;
};

struct nl_ast_classlit {
    struct nl_ast *type, *tmpl, *items;
};

struct nl_ast_function {
    struct nl_ast *name, *type, *body;
};

struct nl_ast_init {
    struct nl_ast *ident, *expr;
};

struct nl_ast_unexpr {
    struct nl_ast *expr;
    int op;
};

struct nl_ast_binexpr {
    struct nl_ast *lhs, *rhs;
    int op;
};

struct nl_ast_call {
    struct nl_ast *func, *args;
};

struct nl_ast_bind {
    struct nl_ast *ident,  *expr;
};

struct nl_ast_assignment {
    struct nl_ast *lhs, *expr;
    int op;
};

struct nl_ast_keyval {
    struct nl_ast *key, *val;
};

struct nl_ast_return {
    struct nl_ast *expr;
};

struct nl_ast_list {
    struct nl_ast *head, *tail;
    int type;
    unsigned int count;
};

struct nl_ast_ifelse {
    struct nl_ast *cond, *if_body, *else_body;
};

struct nl_ast_while {
    struct nl_ast *cond, *body;
};

struct nl_ast_for {
    struct nl_ast *var, *range, *body;
};

struct nl_ast_lookup {
    struct nl_ast *container, *index;
};

struct nl_ast_selector {
    struct nl_ast *parent, *child;
};

struct nl_ast_package_ref {
    struct nl_ast *package, *name;
};

struct nl_ast_interface {
    struct nl_ast *name, *methods;
};

struct nl_ast_class {
    struct nl_ast *name, *tmpl, *members, *methods;
};

struct nl_ast_alias {
    struct nl_ast *type, *name;
};

struct nl_ast_decl {
    struct nl_ast *type;
    struct nl_ast *rhs;     /* one ident or a list of idents */
    int tp;                 /* declaration type (var/const) */
};

struct nl_ast_using {
    struct nl_ast *names;
};

struct nl_ast_package {
    struct nl_ast *name, *globals;
};

struct nl_ast_unit {
    struct nl_ast *packages;
};

struct nl_ast {
    union {
        bool b;
        char c;
        long l;
        double d;
        struct nl_string *s;

        struct nl_ast_classlit class_lit;
        struct nl_ast_tmpl_type tmpl_type;
        struct nl_ast_qual_type qual_type;
        struct nl_ast_func_type func_type;
        struct nl_ast_function function;
        struct nl_ast_init init;
        struct nl_ast_unexpr unexpr;
        struct nl_ast_binexpr binexpr;
        struct nl_ast_call call;
        struct nl_ast_bind bind;
        struct nl_ast_assignment assignment;
        struct nl_ast_keyval keyval;
        struct nl_ast_return ret;
        struct nl_ast_list list;
        struct nl_ast_ifelse ifelse;
        struct nl_ast_while while_loop;
        struct nl_ast_for for_loop;
        struct nl_ast_lookup lookup;
        struct nl_ast_selector selector;
        struct nl_ast_package_ref package_ref;
        struct nl_ast_interface interface;
        struct nl_ast_class classdef;
        struct nl_ast_alias alias;
        struct nl_ast_decl decl;
        struct nl_ast_using usings;
        struct nl_ast_package package;
        struct nl_ast_unit unit;
    };
    struct nl_ast* next;
    struct nl_type* type;
    int tag;
    int lineno;
};

struct nl_ast *nl_ast_make_bool_lit(bool b, int);
struct nl_ast *nl_ast_make_char_lit(char c, int);
struct nl_ast *nl_ast_make_int_lit(long l, int);
struct nl_ast *nl_ast_make_real_lit(double d, int);
struct nl_ast *nl_ast_make_str_lit(struct nl_string *s, int);

struct nl_ast *nl_ast_make_ident(struct nl_string *s, int);

struct nl_ast *nl_ast_make_tmpl_type(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_qual_type(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_func_type(struct nl_ast*, struct nl_ast*, int);

struct nl_ast *nl_ast_make_initialization(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_unexpr(int op, struct nl_ast*, int);
struct nl_ast *nl_ast_make_binexpr(struct nl_ast*, int op, struct nl_ast*, int);
struct nl_ast *nl_ast_make_call(struct nl_ast*, struct nl_ast*, int);

struct nl_ast *nl_ast_make_list(int type, int);
struct nl_ast *nl_ast_list_append(struct nl_ast*, struct nl_ast*);

struct nl_ast *nl_ast_make_keyval(struct nl_ast *key, struct nl_ast *val, int);
struct nl_ast *nl_ast_make_lookup(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_selector(struct nl_ast*, struct nl_ast*, int);

struct nl_ast *nl_ast_make_package_ref(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_bind(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_assignment(struct nl_ast*, int op, struct nl_ast*, int);

struct nl_ast *nl_ast_make_ifelse(struct nl_ast*, struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_while(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_for(struct nl_ast*, struct nl_ast*, struct nl_ast*, int);

struct nl_ast *nl_ast_make_break(int);
struct nl_ast *nl_ast_make_continue(int);
struct nl_ast *nl_ast_make_return(struct nl_ast*, int);

struct nl_ast *nl_ast_make_classlit(struct nl_ast*, struct nl_ast*, struct nl_ast*, int);

struct nl_ast *nl_ast_make_decl(int, struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_function(struct nl_ast*, struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_class(struct nl_ast*, struct nl_ast*, struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_interface(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_alias(struct nl_ast*, struct nl_ast *, int);
struct nl_ast *nl_ast_make_using(struct nl_ast*, int);
struct nl_ast *nl_ast_make_package(struct nl_ast*, struct nl_ast*, int);
struct nl_ast *nl_ast_make_unit(struct nl_ast*, int);

char *nl_ast_name(const struct nl_ast* node);

#endif /* NOLLI_AST_H */
