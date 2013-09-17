#ifndef NOLLI_AST_H
#define NOLLI_AST_H

#include "nolli.h"

typedef enum {
    AST_BOOL_LIT,
    AST_CHAR_LIT,
    AST_INT_NUM,
    AST_REAL_NUM,
    AST_STR_LIT,

    AST_IDENT,
    AST_DECL,

    AST_UNEXPR,
    AST_BINEXPR,

    AST_LIST,
    AST_MAP,
    AST_MAPKV,
    AST_CONTACCESS,

    AST_ASSIGN,
    AST_CONTASSIGN,
    AST_IFELSE,
    AST_WHILE,
    AST_UNTIL,
    AST_FOR,
    AST_CALL,
    AST_FUNC_DEF,
    AST_CLASS,
    AST_STATEMENTS,
    AST_MODULE,
} ast_type_t;

typedef enum {
    DECL_BOOL,
    DECL_CHAR,
    DECL_INT,
    DECL_REAL,
    DECL_STR,
    DECL_LIST,
    DECL_MAP,
    DECL_FILE
} decl_type_t;

typedef struct type {
    decl_type_t id;
    unsigned int* kind;
    unsigned int n;
} type_t;


typedef enum {
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_POW,
    EXPR_LT,
    EXPR_GT,
    EXPR_LTEQ,
    EXPR_GTEQ,
    EXPR_EQ,
    EXPR_IS,
    EXPR_AND,
    EXPR_OR,

    EXPR_NEG,
    EXPR_NOT
} expr_op_t;

typedef enum {
    ASS_DEF,
    ASS_ADD,
    ASS_SUB,
    ASS_MUL,
    ASS_DIV,
    ASS_POW
} assign_op_t;

typedef struct astnode {
    ast_type_t type;
} astnode_t;


astnode_t* make_bool_lit(bool);
astnode_t* make_char_lit(char);
astnode_t* make_int_num(long);
astnode_t* make_real_num(double);
astnode_t* make_str_lit(const char*);

astnode_t* make_ident(const char*);

astnode_t* make_decl(decl_type_t, astnode_t*);
astnode_t* make_unexpr(expr_op_t, astnode_t*);
astnode_t* make_binexpr(expr_op_t, astnode_t*, astnode_t*);

astnode_t* make_list(astnode_t*, astnode_t*);
astnode_t* make_map(astnode_t*, astnode_t*);
astnode_t* make_mapkv(astnode_t*, astnode_t*);
astnode_t* make_contaccess(astnode_t*, astnode_t*);

astnode_t* make_assignment(astnode_t*, assign_op_t, astnode_t*);
astnode_t* make_contassign(astnode_t*, astnode_t*, assign_op_t, astnode_t*);

astnode_t* make_ifelse(astnode_t*, astnode_t*, astnode_t*);
astnode_t* make_while(astnode_t*, astnode_t*);
astnode_t* make_until(astnode_t*, astnode_t*);
astnode_t* make_for(astnode_t*, astnode_t*, astnode_t*);
astnode_t* make_call(astnode_t*, astnode_t*);

astnode_t* make_statements(astnode_t*, astnode_t*);
astnode_t* make_module(astnode_t*, astnode_t*);

#endif /* NOLLI_AST_H */
