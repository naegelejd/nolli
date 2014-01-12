#ifndef NOLLI_AST_H
#define NOLLI_AST_H

struct type_t;

typedef enum {
    AST_BAD_TYPE,

    AST_BOOL_LIT,
    AST_CHAR_LIT,
    AST_INT_NUM,
    AST_REAL_NUM,
    AST_STR_LIT,

    AST_IDENT,

    AST_IMPORT,

    AST_TYPEDEF,
    AST_DECL,

    AST_UNEXPR,
    AST_BINEXPR,

    AST_LIST,

    AST_KEYVAL,
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

typedef enum {
    LIST_ARGS,
    LIST_LITERAL,
    LIST_MAP_ITEMS,
    LIST_STATEMENTS,
} list_type_t;

struct ast {
    int type;
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

struct ast_unexpr {
    struct ast HEAD;
    expr_op_t op;
    struct ast* expr;
};

struct ast_binexpr {
    struct ast HEAD;
    expr_op_t op;
    struct ast* lhs;
    struct ast* rhs;
};

struct ast_call {
    struct ast HEAD;
    struct ast* func;
    struct ast* args;
};

struct ast_assignment {
    struct ast HEAD;
    struct ast* ident;
    struct ast* expr;
};

struct ast_keyval {
    struct ast HEAD;
    struct ast* key;
    struct ast* val;
};

struct ast_list {
    struct ast HEAD;
    struct ast **items;
    list_type_t type;
    unsigned int alloc;
    unsigned int count;
};


struct ast* ast_make_bool_lit(bool b);
struct ast* ast_make_char_lit(char c);
struct ast* ast_make_int_num(long l);
struct ast* ast_make_real_num(double d);
struct ast* ast_make_str_lit(const char *s);

struct ast* ast_make_ident(const char *s);

struct ast* ast_make_import(struct ast*, struct ast*);

struct ast* ast_make_typedef(type_t*, struct ast* id);

struct ast* ast_make_decl(type_t*, struct ast*);
struct ast* ast_make_unexpr(expr_op_t, struct ast*);
struct ast* ast_make_binexpr(struct ast*, expr_op_t, struct ast*);

struct ast* ast_make_list(list_type_t);
struct ast* ast_list_append(struct ast*, struct ast*);
struct ast* ast_make_keyval(struct ast* key, struct ast* val);
struct ast* ast_make_contaccess(struct ast*, struct ast*);

struct ast* ast_make_assignment(struct ast*, assign_op_t, struct ast*);
struct ast* ast_make_contassign(struct ast*, struct ast*, assign_op_t, struct ast*);

struct ast* ast_make_ifelse(struct ast*, struct ast*, struct ast*);
struct ast* ast_make_while(struct ast*, struct ast*);
struct ast* ast_make_until(struct ast*, struct ast*);
struct ast* ast_make_for(struct ast*, struct ast*, struct ast*);
struct ast* ast_make_call(struct ast*, struct ast*);
struct ast* ast_make_member(struct ast*, struct ast*);

struct ast* ast_make_break(void);
struct ast* ast_make_continue(void);
struct ast* ast_make_return(struct ast*);

struct ast* ast_make_statement_list(struct ast*, struct ast*);

void walk(struct ast* root);

#endif /* NOLLI_AST_H */
