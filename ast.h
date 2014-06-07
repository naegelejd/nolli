#ifndef NOLLI_AST_H
#define NOLLI_AST_H

#include "nolli.h"

enum {
    AST_BAD_TYPE,

    AST_BOOL_LIT,
    AST_CHAR_LIT,
    AST_INT_NUM,
    AST_REAL_NUM,
    AST_STR_LIT,

    AST_IDENT,

    AST_LIST_TYPE,
    AST_MAP_TYPE,
    AST_FUNC_TYPE,

    AST_DECL,
    AST_INIT,

    AST_UNEXPR,
    AST_BINEXPR,

    AST_LIST,

    AST_KEYVAL,
    AST_LOOKUP,
    AST_SELECTOR,

    AST_SHORT_DECL,
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

    AST_METHODS,
    AST_DATA,
    AST_INTERFACE,
    AST_ALIAS,
    AST_IMPORT,
    AST_PROGRAM
};

enum {
    DECL_VAR,
    DECL_CONST
};

enum {
    LIST_DECL,
    LIST_ARG,
    LIST_PARAM,
    LIST_TYPE,
    LIST_LITERAL,
    LIST_IMPORT,
    LIST_MAP_ITEM,
    LIST_SELECTOR,
    LIST_MEMBER,
    LIST_METHOD,
    LIST_STRUCT_INIT,
    LIST_STATEMENT,
};

struct ast_list_type {
    struct ast *name;
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

struct ast_short_decl {
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

struct ast_methods {
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
    struct ast *package, *definitions;
};

struct ast {
    union {
        bool b;
        char c;
        long l;
        double d;
        char *s;

        struct ast_list_type list_type;
        struct ast_map_type map_type;
        struct ast_func_type func_type;
        struct ast_datalit datalit;
        struct ast_function function;
        struct ast_init init;
        struct ast_unexpr unexpr;
        struct ast_binexpr binexpr;
        struct ast_call call;
        struct ast_short_decl short_decl;
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
        struct ast_methods methods;
        struct ast_data data;
        struct ast_alias alias;
        struct ast_decl decl;
        struct ast_import import;
        struct ast_program program;
    };
    struct ast* next;
    int tag;
    int lineno;
};

struct ast *ast_make_bool_lit(bool b);
struct ast *ast_make_char_lit(char c);
struct ast *ast_make_int_num(long l);
struct ast *ast_make_real_num(double d);
struct ast *ast_make_str_lit(const char *s);

struct ast *ast_make_ident(const char *s);

struct ast *ast_make_list_type(struct ast*);
struct ast *ast_make_map_type(struct ast*, struct ast*);
struct ast *ast_make_func_type(struct ast*, struct ast*);

struct ast *ast_make_initialization(struct ast*, struct ast*);
struct ast *ast_make_unexpr(int op, struct ast*);
struct ast *ast_make_binexpr(struct ast*, int op, struct ast*);

struct ast *ast_make_list(int type);
struct ast *ast_list_append(struct ast*, struct ast*);

struct ast *ast_make_keyval(struct ast *key, struct ast *val);
struct ast *ast_make_lookup(struct ast*, struct ast*);
struct ast *ast_make_selector(struct ast*, struct ast*);

struct ast *ast_make_short_decl(struct ast*, struct ast*);
struct ast *ast_make_assignment(struct ast*, int op, struct ast*);

struct ast *ast_make_ifelse(struct ast*, struct ast*, struct ast*);
struct ast *ast_make_while(struct ast*, struct ast*);
struct ast *ast_make_for(struct ast*, struct ast*, struct ast*);
struct ast *ast_make_call(struct ast*, struct ast*);

struct ast *ast_make_break(void);
struct ast *ast_make_continue(void);
struct ast *ast_make_return(struct ast*);

struct ast *ast_make_datalit(struct ast *, struct ast *);

struct ast *ast_make_decl(int, struct ast*, struct ast*);
struct ast *ast_make_function(struct ast*, struct ast*, struct ast*);
struct ast *ast_make_data(struct ast *, struct ast*);
struct ast *ast_make_methods(struct ast*, struct ast*);
struct ast *ast_make_interface(struct ast *, struct ast*);
struct ast *ast_make_alias(struct ast*, struct ast *);
struct ast *ast_make_import(struct ast *, struct ast*);
struct ast *ast_make_program(struct ast*, struct ast*);

#endif /* NOLLI_AST_H */
