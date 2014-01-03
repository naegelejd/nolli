#ifndef NOLLI_LEXER_H
#define NOLLI_LEXER_H

#include "nolli.h"


enum {
    TOK_EOF = 0,
    TOK_IDENT = 1, TOK_TYPE = 1,
    TOK_BOOL, TOK_CHAR, TOK_INT, TOK_REAL, TOK_STRING, TOK_FILE,

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
    TOK_COMMA, TOK_DOT, TOK_COLON, TOK_SEMI,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR,
    TOK_BREAK, TOK_CONT,
    TOK_IN,
    TOK_TYPEDEF,
    TOK_FUNC, TOK_RET,
    TOK_STRUCT, TOK_IFACE,
    TOK_MODULE, TOK_IMPORT, TOK_FROM
};

struct typetable {
    char **names;
    int *ids;

    unsigned long count;
    unsigned long size;
    unsigned int size_idx;
};

enum {TYPETABLE_SEARCH = 0, TYPETABLE_INSERT = 1};
enum {TYPENAME_MAXLEN = 32};

struct lexer {
    FILE* input;

    char *curbuff;
    char *lastbuff;
    size_t blen;
    size_t balloc;

    int line;
    int col;
    int cur;
};


int gettok(struct lexer *lex);
int lexer_init(struct lexer **lexaddr, FILE *file);
int lexer_scan_all(struct lexer *lex);
const char *get_tok_name(int tok);

#endif /* NOLLI_LEXER_H */
