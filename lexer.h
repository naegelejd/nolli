#ifndef NOLLI_LEXER_H
#define NOLLI_LEXER_H

#include "nolli.h"

enum {
    TOK_NEWLINE = -1,
    TOK_EOF = 0,
    TOK_IDENT = 1, TOK_TYPE = 1,
    TOK_BOOL, TOK_CHAR, TOK_INT, TOK_REAL, TOK_STRING,

    TOK_ADD, TOK_IADD,
    TOK_SUB, TOK_ISUB,
    TOK_MUL, TOK_IMUL,
    TOK_DIV, TOK_IDIV,
    TOK_MOD, TOK_IMOD,
    TOK_POW, TOK_IPOW,
    TOK_COLON, TOK_SHORTDECL,
    TOK_ASS, TOK_EQ,
    TOK_NOT, TOK_NEQ,
    TOK_LT, TOK_LTE,
    TOK_GT, TOK_GTE,
    TOK_OR, TOK_AND,

    TOK_LPAREN, TOK_RPAREN, TOK_LSQUARE, TOK_RSQUARE, TOK_LCURLY, TOK_RCURLY,
    TOK_COMMA, TOK_SEMI, TOK_DOT, TOK_AMP,

    TOK_PACKAGE, TOK_IMPORT, TOK_FROM,
    TOK_ALIAS,
    TOK_DATA, TOK_METHODS, TOK_INTERFACE,
    TOK_FUNC, TOK_RET,
    TOK_BREAK, TOK_CONT,
    TOK_VAR, TOK_CONST,
    TOK_IF, TOK_ELSE,
    TOK_WHILE, TOK_FOR, TOK_IN
};

struct lexer {
    char *input;
    char *sptr;

    char *curbuff;
    char *lastbuff;
    size_t blen;
    size_t balloc;

    int lasttok;

    int line;
    int col;
    int cur;
};


void lexer_init(struct lexer *, char *);

int gettok(struct lexer *lex);
const char *get_tok_name(int tok);

int lexer_scan_all(struct lexer *lex);

#endif /* NOLLI_LEXER_H */
