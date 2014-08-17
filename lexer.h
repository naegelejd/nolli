#ifndef NOLLI_LEXER_H
#define NOLLI_LEXER_H

#include <stddef.h>

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
    TOK_COLON, TOK_BIND,
    TOK_ASS, TOK_EQ,
    TOK_NOT, TOK_NEQ,
    TOK_LT, TOK_LTE,
    TOK_GT, TOK_GTE,
    TOK_OR, TOK_AND,

    TOK_LPAREN, TOK_RPAREN, TOK_LSQUARE, TOK_RSQUARE, TOK_LCURLY, TOK_RCURLY,
    TOK_COMMA, TOK_SEMI, TOK_DOT, TOK_PREF,

    TOK_PACKAGE, TOK_USING,
    TOK_NEW, TOK_ALIAS,
    TOK_CLASS, TOK_INTERFACE,
    TOK_FUNC, TOK_RET,
    TOK_BREAK, TOK_CONT,
    TOK_VAR, TOK_CONST,
    TOK_IF, TOK_ELSE,
    TOK_WHILE, TOK_FOR, TOK_IN
};

struct nl_context;

struct nl_lexer {
    struct nl_context *ctx;

    const char *input;
    const char *sptr;

    char *curbuff;
    char *lastbuff;
    size_t blen;
    size_t balloc;

    int lasttok;

    int line;
    int col;
    int cur;
};

void nl_lexer_init(struct nl_lexer *, struct nl_context *ctx, const char *);

int nl_gettok(struct nl_lexer *lex);
const char *nl_get_tok_name(int tok);

int nl_lexer_scan_all(struct nl_lexer *lex);

#endif /* NOLLI_LEXER_H */
