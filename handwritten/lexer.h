#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>


enum {
    TOK_EOF = 0, TOK_INT, TOK_FLOAT, TOK_STRING, TOK_IDENT,

    TOK_ADD, TOK_IADD,
    TOK_SUB, TOK_ISUB,
    TOK_MUL, TOK_IMUL,
    TOK_DIV, TOK_IDIV,
    TOK_MOD, TOK_IMOD,
    TOK_POW, TOK_IPOW,
    TOK_ASS, TOK_EQ,
    TOK_NOT, TOK_NEQ,
    TOK_LT, TOK_LTE,
    TOK_GT, TOK_GTE,

    TOK_LPAREN, TOK_RPAREN, TOK_LSQUARE, TOK_RSQUARE, TOK_LCURLY, TOK_RCURLY,
    TOK_COMMA, TOK_DOT, TOK_SEMI,

    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_BREAK, TOK_CONT,
    TOK_FUNC, TOK_RET,
    TOK_STRUCT, TOK_IFACE,
    TOK_MODULE, TOK_IMPORT
};

struct lexer {
    FILE* input;

    char *buff;
    size_t blen;
    size_t balloc;

    long int_num;
    double float_num;
    int line;
    int col;
    int cur;
};


int gettok(struct lexer *lex);
int lexer_init(struct lexer **lexaddr);
const char *get_tok_name(int tok);

#endif /* LEXER_H */
