#ifndef NOLLI_AST_H
#define NOLLI_AST_H

struct type_t;

typedef enum {
    AST_BOOL_LIT,
    AST_CHAR_LIT,
    AST_INT_NUM,
    AST_REAL_NUM,
    AST_STR_LIT,

    AST_IDENT,

    AST_MODULE,
    AST_IMPORT,

    AST_TYPEDEF,
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
    AST_STRUCT,
    AST_MEMBER,

    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,

    AST_STATEMENTS,
} ast_type_t;

typedef enum {
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_MOD,
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


astnode_t* make_bool_lit(token_t);
astnode_t* make_char_lit(token_t);
astnode_t* make_int_num(token_t);
astnode_t* make_real_num(token_t);
astnode_t* make_str_lit(token_t);

astnode_t* make_ident(token_t);

astnode_t* make_module(astnode_t*, astnode_t*);
astnode_t* make_import(astnode_t*, astnode_t*);

astnode_t* make_typedef(type_t*, astnode_t* id);

astnode_t* make_decl(type_t*, astnode_t*);
astnode_t* make_unexpr(expr_op_t, astnode_t*);
astnode_t* make_binexpr(astnode_t*, expr_op_t, astnode_t*);

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
astnode_t* make_member(astnode_t*, astnode_t*);

astnode_t* make_break(void);
astnode_t* make_continue(void);
astnode_t* make_return(astnode_t*);

astnode_t* make_statements(astnode_t*, astnode_t*);

#endif /* NOLLI_AST_H */
