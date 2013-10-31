#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>


enum {
    tok_eof = 0,
    tok_int,
    tok_float,
    tok_string,
    tok_ident,
};

static char *tok_type_names[] = {
    "EOF",
    "int",
    "float",
    "string",
    "ident"
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

#define next(lex) \
    do { \
        lex->cur = getc(lex->input); \
        lex->col++; \
    } while (false)

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
    } while (isdigit(lex->cur) || lex->cur == '.');
    return tok_int;
}

int lex_float(struct lexer *lex)
{
    return tok_float;
}

int lex_string(struct lexer *lex)
{
    next(lex);
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (lex->cur != '"');
    next(lex);  /* eat closing double-quote */
    return tok_string;
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
            lex->col = -1;
            next(lex);
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
            return tok_ident;
        }
        else if (lex->cur == EOF) {
            return tok_eof;
        }
        else {
            int sym = lex->cur;
            next(lex);

            static char symbols[] = "()[]{}+-*/%^&|=<>";
            if (!strchr(symbols, sym)) {
                fprintf(stderr, "Error: invalid symbol '%c' at line %d, column %d\n",
                        sym, lex->line, lex->col);
                exit(1);
            } else {
                return sym;
            }
        }
    }

    return tok_eof;
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
    lex->col = -1;

    int good = 1;
    while (good) {
        good = gettok(lex);
        if (good < 5) {
            printf("%s\n", tok_type_names[good]);
        } else {
            printf("%c\n", good);
        }
    }

    return EXIT_SUCCESS;
}
