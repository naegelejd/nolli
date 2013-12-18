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
    TOK_EOF = 0,
    TOK_BOOL, TOK_CHAR, TOK_INT, TOK_REAL, TOK_STRING, TOK_FILE,
    TOK_TYPE, TOK_IDENT,

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

enum {
    START_TYPE,

    TYPE_BOOL, TYPE_CHAR, TYPE_INT, TYPE_REAL, TYPE_STR,

    END_TYPE
};

struct sbuffer {
    char *buff;
    size_t blen;
    size_t balloc;
};

struct typetable {
    char **names;
    int *ids;

    unsigned long count;
    unsigned long size;
    unsigned int size_idx;
};

enum {TYPETABLE_SEARCH = 0, TYPETABLE_INSERT = 1};


struct lexer {
    FILE* input;

    union {
        char rune;      /* TODO: UTF-8 code points */
        int typeid;
        long integer;
        double real;
    } data;

    struct sbuffer *sbuff;

    struct typetable *typetable;

    int line;
    int col;
    int cur;
};


int gettok(struct lexer *lex);
int lexer_init(struct lexer **lexaddr, FILE *file);
int lexer_scan_all(struct lexer *lex);
const char *get_tok_name(int tok);

int check_type(struct lexer *lex, const char *name);
int add_type(struct lexer *lex, const char *name);
int readd_type(struct lexer *lex, const char *name, int id);


#endif /* LEXER_H */
