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
};

static char *tok_type_names[] = {
    "EOF", "int", "float", "string", "ident",

    "add", "iadd",
    "sub", "isub",
    "mul", "imul",
    "div", "idiv",
    "mod", "imod",
    "pow", "ipow",
    "ass", "eq",
    "not", "neq",
    "lt", "lte",
    "gt", "gte",

    "lparen", "rparen", "lsquare", "rsquare", "lcurly", "rcurly",
};

/******** Placeholder Memory allocation *********/
void* zalloc(size_t bytes)
{
    assert(bytes);
    void* block = calloc(1, bytes);
    if (block == NULL) {
        fprintf(stderr, "Alloc failed\n");
        exit(EXIT_FAILURE);
    }

    return block;
}

void* zrealloc(void* block, size_t bytes)
{
    assert(block);
    assert(bytes);

    void* reblock = realloc(block, bytes);
    if (reblock == NULL) {
        fprintf(stderr, "Alloc failed\n");
        exit(EXIT_FAILURE);
    }

    return reblock;
}
/************************************************/

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

#define next(lex) \
    do { \
        lex->cur = getc(lex->input); \
        lex->col++; \
    } while (false)

void lexerror(struct lexer *lex, char *msg, ...)
{
    va_list ap;

    fprintf(stderr, "Error at line %d, column %d: ", lex->line, lex->col);

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    exit(1);
}

int clear(struct lexer *lex)
{
    memset(lex->buff, 0, lex->balloc);
    lex->blen = 0;
    return 0;
}

int appendc(struct lexer *lex, int c)
{
    assert(lex);
    assert(lex->buff);

    /* expand string buffer if it's value
     * (including nul terminator) is too long */
    if (lex->blen >= lex->balloc - 1) {
        size_t old_alloc = lex->balloc;
        size_t new_alloc = old_alloc * 2;

        /* realloc the buffer */
        lex->buff = zrealloc(lex->buff, new_alloc);
        lex->balloc = new_alloc;

        /* memory for strings must always be zeroed */
        memset(lex->buff + old_alloc, 0, new_alloc - old_alloc);
    }

    lex->buff[lex->blen++] = (char)c;
    return c;
}

int lex_integer(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isdigit(lex->cur) || lex->cur == '.' || strchr("eE", lex->cur));
    return TOK_INT;
}

int lex_float(struct lexer *lex)
{
    return TOK_FLOAT;
}

int lex_string(struct lexer *lex)
{
    /* eat the starting string delimiter */
    next(lex);
    do {
        switch (lex->cur) {
        case EOF:
            lexerror(lex, "Unexpected EOF");
            break;
        case '\n': case '\r':
            lexerror(lex, "Unterminated string literal");
            break;
        case '\\': {
            int c = lex->cur;
            next(lex);
            switch (c) {
            case '"': goto next_append;
            case '\\': goto next_append;
            case 'a': c = '\a'; goto next_append;
            case 'b': c = '\b'; goto next_append;
            case 'f': c = '\f'; goto next_append;
            case 'n': c = '\n'; goto next_append;
            case 'r': c = '\r'; goto next_append;
            case 't': c = '\t'; goto next_append;
            case 'v': c = '\v'; goto next_append;
            case EOF:
                lexerror(lex, "Unexpected EOF in string literal");
                break;
            case '\n': case '\r':
                lex->line++;
                lex->col = 0;
                appendc(lex, lex->cur);
                break;
            default:
                lexerror(lex, "Invalid escape character %c", lex->cur);
            }
        next_append:
            next(lex);
            appendc(lex, c);
            break;
        }

        /* normal character in string literal: */
        default:
            appendc(lex, lex->cur);
            next(lex);
        }
    } while (lex->cur != '"');

    next(lex);  /* eat closing double-quote */
    return TOK_STRING;
}

int lex_symbol(struct lexer *lex)
{
    int tok = 0;
    switch (lex->cur) {
        case '+': tok = TOK_ADD; break;
        case '-': tok = TOK_SUB; break;
        case '*': tok = TOK_MUL; break;
        case '/': tok = TOK_DIV; break;
        case '%': tok = TOK_MOD; break;
        case '^': tok = TOK_POW; break;
        case '=': tok = TOK_ASS; break;
        case '!': tok = TOK_NOT; break;
        case '<': tok = TOK_LT; break;
        case '>': tok = TOK_GT; break;
        default: {
            static char symbols[] = "()[]{}";
            char *at = NULL;
            if ((at = strchr(symbols, lex->cur))) {
                next(lex);
                return (at - symbols) + TOK_LPAREN;
            } else {
                lexerror(lex, "Invalid symbol %c", lex->cur);
            }
        }
    }

    next(lex);
    if (lex->cur == '=') {
        next(lex);
        /* in-place operations follow their base op equivalent */
        return tok + 1;
    } else {
        return tok;
    }
}

int gettok(struct lexer *lex)
{
    clear(lex);     /* clear the lexer's buffer */

    if (lex->cur == 0) {
        next(lex);
    }

    while (true) {
        if (lex->cur == '\n' || lex->cur == '\r') {
            lex->line++;
            lex->col = 0;
            next(lex);
            continue;
        }
        /* eat whitespace */
        else if (isspace(lex->cur)) {
            next(lex);
            continue;
        }
        else if (isdigit(lex->cur)) {
            return lex_integer(lex);
        }
        /* eat comments */
        else if (lex->cur == '#') {
            do {
                next(lex);  /* eat up comment line */
            } while (lex->cur != EOF && lex->cur != '\n' && lex->cur != '\r');
            continue;
        }
        else if (lex->cur == '"') {
            return lex_string(lex);
        }
        else if (isalpha(lex->cur)) {
            do {
                appendc(lex, lex->cur);
                next(lex);
            } while (isalnum(lex->cur));
            return TOK_IDENT;
        }
        else if (lex->cur == EOF) {
            return TOK_EOF;
        }
        /* all that's left is symbols */
        else {
            return lex_symbol(lex);
        }
    }

    return TOK_EOF;
}

/******** Driver ********/
int main(void)
{
    struct lexer *lex = zalloc(sizeof(*lex));
    lex->input = stdin;

    size_t bufsize = 16;
    lex->buff = zalloc(bufsize);
    lex->blen = 0;
    lex->balloc = bufsize;

    lex->line = 1;
    lex->col = 0;

    int good = 1;
    while (good) {
        good = gettok(lex);
        printf("%s", tok_type_names[good]);
        switch (good) {
            case TOK_IDENT:
                printf(": %s", lex->buff);
                break;
            case TOK_STRING:
                printf(": \"%s\"", lex->buff);
                break;
            default:;
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}
