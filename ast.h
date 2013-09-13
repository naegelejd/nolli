#ifndef NOLLI_AST_H
#define NOLLI_AST_H

#include "nolli.h"

typedef enum {
    AST_CHAR,
    AST_INT,
    AST_REAL,
    AST_STR,
    AST_LIST,
    AST_MAP,
    AST_FILE,
    AST_FUNC_DEF,
    AST_CALL,
    AST_CLASS
} ast_type_t;

typedef struct astnode {
    ast_type_t type;
} astnode_t;

astnode_t* make_char(char c);
astnode_t* make_int(long l);
astnode_t* make_real(double d);

#endif /* NOLLI_AST_H */
