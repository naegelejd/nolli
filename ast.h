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

    AST_IMPORT,

    AST_ALIAS,

    AST_LIST_TYPE,
    AST_MAP_TYPE,
    AST_FUNC_TYPE,
    AST_STRUCT_TYPE,
    AST_IFACE_TYPE,

    AST_DECL,
    AST_INIT,

    AST_UNEXPR,
    AST_BINEXPR,

    AST_LIST,

    AST_KEYVAL,
    AST_CONTACCESS,

    AST_SHORT_DECL,
    AST_ASSIGN,
    AST_IFELSE,
    AST_WHILE,
    AST_FOR,
    AST_CALL,
    AST_FUNCLIT,
    AST_STRUCTLIT,

    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
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

struct ast {
    int type;
    int lineno;
};

struct ast_bool {
    struct ast HEAD;
    bool b;
};

struct ast_char {
    struct ast HEAD;
    char c;
};

struct ast_int {
    struct ast HEAD;
    long l;
};

struct ast_real {
    struct ast HEAD;
    double d;
};

struct ast_str {
    struct ast HEAD;
    char *s;
};

struct ast_ident {
    struct ast HEAD;
    char *s;
};

struct ast_list_type {
    struct ast HEAD;
    struct ast *name;
};

struct ast_map_type {
    struct ast HEAD;
    struct ast *keyname;
    struct ast *valname;
};

struct ast_func_type {
    struct ast HEAD;
    struct ast *ret_type;
    struct ast *param_types;
};

struct ast_struct_type {
    struct ast HEAD;
    struct ast *name;
    struct ast *members;
};

struct ast_structlit {
    struct ast HEAD;
    struct ast *name;
    struct ast *items;
};

struct ast_iface_type {
    struct ast HEAD;
    struct ast *name;
    struct ast *methods;
};

struct ast_decl {
    struct ast HEAD;
    struct ast *type;
    struct ast *name_s;     /* one ident or a list of idents */
    int tp;                 /* declaration type (var/const) */
};

struct ast_init {
    struct ast HEAD;
    struct ast *ident;
    struct ast *expr;
};

struct ast_unexpr {
    struct ast HEAD;
    int op;
    struct ast* expr;
};

struct ast_binexpr {
    struct ast HEAD;
    int op;
    struct ast* lhs;
    struct ast* rhs;
};

struct ast_call {
    struct ast HEAD;
    struct ast* func;
    struct ast* args;
};

struct ast_short_decl {
    struct ast HEAD;
    struct ast* ident;
    struct ast* expr;
};

struct ast_assignment {
    struct ast HEAD;
    struct ast* ident;
    struct ast* expr;
    int op;
};

struct ast_keyval {
    struct ast HEAD;
    struct ast* key;
    struct ast* val;
};

struct ast_import {
    struct ast HEAD;
    struct ast *from;
    struct ast *modules;
};

struct ast_alias {
    struct ast HEAD;
    struct ast *type;
    struct ast *name;
};

struct ast_return {
    struct ast HEAD;
    struct ast *expr;
};

struct ast_break {
    struct ast HEAD;
};

struct ast_cont {
    struct ast HEAD;
};

struct ast_list {
    struct ast HEAD;
    struct ast **items;
    int type;
    unsigned int alloc;
    unsigned int count;
};

struct ast_ifelse {
    struct ast HEAD;
    struct ast *cond;
    struct ast *if_body;
    struct ast *else_body;
};

struct ast_while {
    struct ast HEAD;
    struct ast *cond;
    struct ast *body;
};

struct ast_for {
    struct ast HEAD;
    struct ast *var;
    struct ast *range;
    struct ast *body;
};

struct ast_contaccess {
    struct ast HEAD;
    struct ast *ident;
    struct ast *index;
};

struct ast_funcdef {
    struct ast HEAD;
    struct ast *ret_type;
    struct ast *name;
    struct ast *params;
    struct ast *body;
};

struct ast_funclit {
    struct ast HEAD;
    struct ast *ret_type;
    struct ast *params;
    struct ast *body;
};

struct ast *ast_make_bool_lit(bool b);
struct ast *ast_make_char_lit(char c);
struct ast *ast_make_int_num(long l);
struct ast *ast_make_real_num(double d);
struct ast *ast_make_str_lit(const char *s);

struct ast *ast_make_ident(const char *s);

struct ast *ast_make_import(struct ast *, struct ast*);

struct ast *ast_make_alias(struct ast*, struct ast *);

struct ast *ast_make_list_type(struct ast*);
struct ast *ast_make_map_type(struct ast*, struct ast*);
struct ast *ast_make_func_type(struct ast*, struct ast*);
struct ast *ast_make_struct_type(struct ast *, struct ast*);
struct ast *ast_make_iface_type(struct ast *, struct ast*);

struct ast *ast_make_decl(int, struct ast*, struct ast*);
struct ast *ast_make_initialization(struct ast*, struct ast*);
struct ast *ast_make_unexpr(int op, struct ast*);
struct ast *ast_make_binexpr(struct ast*, int op, struct ast*);

struct ast *ast_make_list(int type);
struct ast *ast_list_append(struct ast*, struct ast*);

struct ast *ast_make_keyval(struct ast *key, struct ast *val);
struct ast *ast_make_contaccess(struct ast*, struct ast*);

struct ast *ast_make_short_decl(struct ast*, struct ast*);
struct ast *ast_make_assignment(struct ast*, int op, struct ast*);

struct ast *ast_make_ifelse(struct ast*, struct ast*, struct ast*);
struct ast *ast_make_while(struct ast*, struct ast*);
struct ast *ast_make_for(struct ast*, struct ast*, struct ast*);
struct ast *ast_make_call(struct ast*, struct ast*);

struct ast *ast_make_break(void);
struct ast *ast_make_continue(void);
struct ast *ast_make_return(struct ast*);

struct ast *ast_make_funclit(struct ast*, struct ast*, struct ast*);
struct ast* ast_make_structlit(struct ast *, struct ast *);

#endif /* NOLLI_AST_H */
