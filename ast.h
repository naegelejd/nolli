#ifndef NOLLI_AST_H
#define NOLLI_AST_H

#include "nolli.h"

enum {
    AST_FIRST,

    AST_BOOL_LIT,
    AST_CHAR_LIT,
    AST_INT_NUM,
    AST_REAL_NUM,
    AST_STR_LIT,

    AST_IDENT,

    AST_QUALIFIED,
    AST_LIST_TYPE,
    AST_MAP_TYPE,
    AST_FUNC_TYPE,

    AST_DECL,
    AST_INIT,

    AST_UNEXPR,
    AST_BINEXPR,

    AST_KEYVAL,
    AST_LOOKUP,
    AST_SELECTOR,

    AST_BIND,
    AST_ASSIGN,
    AST_IFELSE,
    AST_WHILE,
    AST_FOR,
    AST_CALL,
    AST_FUNCTION,
    AST_DATALIT,

    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,

    AST_IMPL,
    AST_DATA,
    AST_INTERFACE,
    AST_ALIAS,
    AST_IMPORT,
    AST_PROGRAM,

    AST_LIST_SENTINEL,  /* never used */

    /* linked-lists */
    AST_LIST_LISTLIT,
    AST_LIST_MAPLIT,
    AST_LIST_GLOBALS,
    AST_LIST_IMPORTS,
    AST_LIST_MEMBERS,
    AST_LIST_STATEMENTS,
    AST_LIST_IDENTS,
    AST_LIST_METHODS,
    AST_LIST_METHOD_DECLS,
    AST_LIST_DECLS,
    AST_LIST_DATA_INITS,
    AST_LIST_PARAMS,
    AST_LIST_ARGS,

    AST_LAST

};

enum {
    DECL_VAR,
    DECL_CONST
};

struct ast_list_type {
    struct ast *name;
};

struct ast_qualified {
    struct ast *package, *name;
};

struct ast_map_type {
    struct ast *keytype, *valtype;
};

struct ast_func_type {
    struct ast *ret_type, *params;
};

struct ast_datalit {
    struct ast *name, *items;
};

struct ast_function {
    struct ast *name, *type, *body;
};

struct ast_init {
    struct ast *ident, *expr;
};

struct ast_unexpr {
    struct ast *expr;
    int op;
};

struct ast_binexpr {
    struct ast *lhs, *rhs;
    int op;
};

struct ast_call {
    struct ast *func, *args;
};

struct ast_bind {
    struct ast *ident,  *expr;
};

struct ast_assignment {
    struct ast *lhs, *expr;
    int op;
};

struct ast_keyval {
    struct ast *key, *val;
};

struct ast_return {
    struct ast *expr;
};

struct ast_list {
    struct ast *head, *tail;
    int type;
    unsigned int count;
};

struct ast_ifelse {
    struct ast *cond, *if_body, *else_body;
};

struct ast_while {
    struct ast *cond, *body;
};

struct ast_for {
    struct ast *var, *range, *body;
};

struct ast_lookup {
    struct ast *container, *index;
};

struct ast_selector {
    struct ast *parent, *child;
};

struct ast_interface {
    struct ast *name, *methods;
};

struct ast_impl {
    struct ast *name, *methods;
};

struct ast_data {
    struct ast *name, *members;
};

struct ast_alias {
    struct ast *type, *name;
};

struct ast_decl {
    struct ast *type;
    struct ast *rhs;     /* one ident or a list of idents */
    int tp;                 /* declaration type (var/const) */
};

struct ast_import {
    struct ast *from, *modules;
};

struct ast_program {
    struct ast *package, *globals;
};

struct ast {
    union {
        bool b;
        char c;
        long l;
        double d;
        struct string *s;

        struct ast_list_type list_type;
        struct ast_map_type map_type;
        struct ast_func_type func_type;
        struct ast_qualified qualified;
        struct ast_datalit datalit;
        struct ast_function function;
        struct ast_init init;
        struct ast_unexpr unexpr;
        struct ast_binexpr binexpr;
        struct ast_call call;
        struct ast_bind bind;
        struct ast_assignment assignment;
        struct ast_keyval keyval;
        struct ast_return ret;
        struct ast_list list;
        struct ast_ifelse ifelse;
        struct ast_while while_loop;
        struct ast_for for_loop;
        struct ast_lookup lookup;
        struct ast_selector selector;
        struct ast_interface interface;
        struct ast_impl impl;
        struct ast_data data;
        struct ast_alias alias;
        struct ast_decl decl;
        struct ast_import import;
        struct ast_program program;
    };
    struct ast* next;
    struct type* type;
    int tag;
    int lineno;
};

struct ast *ast_make_bool_lit(bool b, int);
struct ast *ast_make_char_lit(char c, int);
struct ast *ast_make_int_num(long l, int);
struct ast *ast_make_real_num(double d, int);
struct ast *ast_make_str_lit(struct string *s, int);

struct ast *ast_make_ident(struct string *s, int);

struct ast *ast_make_qualified(struct ast*, struct ast*, int);
struct ast *ast_make_list_type(struct ast*, int);
struct ast *ast_make_map_type(struct ast*, struct ast*, int);
struct ast *ast_make_func_type(struct ast*, struct ast*, int);

struct ast *ast_make_initialization(struct ast*, struct ast*, int);
struct ast *ast_make_unexpr(int op, struct ast*, int);
struct ast *ast_make_binexpr(struct ast*, int op, struct ast*, int);

struct ast *ast_make_list(int type, int);
struct ast *ast_list_append(struct ast*, struct ast*);

struct ast *ast_make_keyval(struct ast *key, struct ast *val, int);
struct ast *ast_make_lookup(struct ast*, struct ast*, int);
struct ast *ast_make_selector(struct ast*, struct ast*, int);

struct ast *ast_make_bind(struct ast*, struct ast*, int);
struct ast *ast_make_assignment(struct ast*, int op, struct ast*, int);

struct ast *ast_make_ifelse(struct ast*, struct ast*, struct ast*, int);
struct ast *ast_make_while(struct ast*, struct ast*, int);
struct ast *ast_make_for(struct ast*, struct ast*, struct ast*, int);
struct ast *ast_make_call(struct ast*, struct ast*, int);

struct ast *ast_make_break(int);
struct ast *ast_make_continue(int);
struct ast *ast_make_return(struct ast*, int);

struct ast *ast_make_datalit(struct ast *, struct ast *, int);


struct ast *ast_make_decl(int, struct ast*, struct ast*, int);
struct ast *ast_make_function(struct ast*, struct ast*, struct ast*, int);
struct ast *ast_make_data(struct ast *, struct ast*, int);
struct ast *ast_make_impl(struct ast*, struct ast*, int);
struct ast *ast_make_interface(struct ast *, struct ast*, int);
struct ast *ast_make_alias(struct ast*, struct ast *, int);
struct ast *ast_make_import(struct ast *, struct ast*, int);
struct ast *ast_make_program(struct ast*, struct ast*, int);

#endif /* NOLLI_AST_H */
