#include "lexer.h"


static char *tok_type_names[] = {
    "EOF",
    "bool", "char", "int", "real", "string",
    "type", "identifier",

    "'+'", "'+='",
    "'-'", "'-='",
    "'*'", "'*='",
    "'/'", "'/='",
    "'%'", "'%='",
    "'^'", "'^='",
    "'='", "'=='",
    "'!'", "'!='",
    "'<'", "'>='",
    "'>'", "'>='",

    "'('", "')'", "'['", "']'", "'{'", "'}'",
    "','", "'.'", "':'", "';'",

    "if", "else",
    "while", "for",
    "break", "continue",
    "func", "return",
    "struct", "iface",
    "module", "import", "from",
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

/* returns former length of string in buffer */
int clear_sbuffer(struct sbuffer *sbuff)
{
    assert(sbuff);

    memset(sbuff->buff, 0, sbuff->balloc);

    size_t len = sbuff->blen;
    sbuff->blen = 0;

    return len;
}

struct sbuffer *new_sbuffer(size_t nbytes)
{
    assert(nbytes);

    struct sbuffer *sbuff = zalloc(sizeof(*sbuff));
    sbuff->buff = zalloc(nbytes);
    sbuff->blen = 0;
    sbuff->balloc = nbytes;
    return sbuff;
}

int appendc(struct lexer *lex, int c)
{
    assert(lex);

    struct sbuffer *sbuff = lex->sbuff;
    assert(sbuff);
    assert(sbuff->buff);

    /* expand string buffer if it's value
     * (including nul terminator) is too long */
    if (sbuff->blen >= sbuff->balloc - 1) {
        size_t old_alloc = sbuff->balloc;
        size_t new_alloc = old_alloc * 2;

        /* realloc the buffer */
        sbuff->buff = zrealloc(sbuff->buff, new_alloc);
        sbuff->balloc = new_alloc;

        /* memory for strings must always be zeroed */
        memset(sbuff->buff + old_alloc, 0, new_alloc - old_alloc);
    }

    sbuff->buff[sbuff->blen++] = (char)c;
    return c;
}

/* to be called with decimal point as lex->cur or already in buffer */
int lex_float(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isdigit(lex->cur));

    /* allow 'scientific E notation' */
    if (strchr("eE", lex->cur)) {
        appendc(lex, lex->cur);
        next(lex);

        if (strchr("-+", lex->cur)) {
            appendc(lex, lex->cur);
            next(lex);
        }

        while (isdigit(lex->cur)) {
            appendc(lex, lex->cur);
            next(lex);
        }
    }

    char *endptr = NULL;
    double d = strtod(lex->sbuff->buff, &endptr);
    if (endptr != (lex->sbuff->buff + lex->sbuff->blen)) {
        lexerror(lex, "Invalid real number %s", lex->sbuff->buff);
    }
    lex->data.real = d;

    return TOK_REAL;
}

int lex_integer(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isdigit(lex->cur));

    if (lex->cur == '.' || strchr("eE", lex->cur)) {
        return lex_float(lex);
    }

    if (strchr("xX", lex->cur)) {
        appendc(lex, lex->cur);
        next(lex);
        while (strchr("abcdefABCDEF123456890", lex->cur)) {
            appendc(lex, lex->cur);
            next(lex);
        }
    }

    char *endptr = NULL;
    long l = strtol(lex->sbuff->buff, &endptr, 0);
    if (endptr != (lex->sbuff->buff + lex->sbuff->blen)) {
        lexerror(lex, "Invalid integer %s", lex->sbuff->buff);
    }
    lex->data.integer = l;

    return TOK_INT;
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

int lex_ident(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isalnum(lex->cur));

    /* TODO: use hash-table or similar O(1) lookup */
    const char *keywords[] = {
        "if",
        "else",
        "while",
        "for",
        "break",
        "continue",
        "func",
        "return",
        "struct",
        "iface",
        "module",
        "import",
        "from",
    };
    unsigned int kidx = 0;
    for (kidx = 0; kidx < sizeof(keywords) / sizeof(*keywords); kidx++) {
        if (strncmp(lex->sbuff->buff, keywords[kidx], 16) == 0) {
            return TOK_IF + kidx;
        }
    }

    /* TODO: use hash-table or similar O(1) lookup */
    const char *typenames[] = {
        "bool", "char", "int", "real", "str"
    };
    unsigned int tidx = 0;
    for (tidx = 0; tidx < sizeof(typenames) / sizeof(*typenames); tidx++) {
        if (strncmp(lex->sbuff->buff, typenames[tidx], 8) == 0) {
            lex->data.type = START_TYPE + tidx;
            return TOK_TYPE;
        }
    }

    return TOK_IDENT;
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
            static char symbols[] = "()[]{},.:;";
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
    clear_sbuffer(lex->sbuff);     /* clear the lexer's string buffer */

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
        else if (lex->cur == '\'') {
            /* lex single-quoted character */
            next(lex);  /* skip opening ' */
            lex->data.rune = lex->cur;
            next(lex);
            next(lex);  /* skip closing ' */
            return TOK_CHAR;
        }
        else if (lex->cur == '"') {
            return lex_string(lex);
        }
        else if (isalpha(lex->cur)) {
            return lex_ident(lex);
        }
        /* stupid floating points with no leading zero (e.g. '.123') */
        /* else if (lex->cur == '.') { */
        /*     appendc(lex, lex->cur); */
        /*     next(lex); */
        /*     if (isdigit(lex->cur)) { */
        /*         return lex_float(lex); */
        /*     } else { */
        /*         return TOK_DOT; */
        /*     } */
        /* } */
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

const char *get_tok_name(int tok)
{
    return tok_type_names[tok];
}

int lexer_init(struct lexer **lexaddr, FILE *file)
{
    struct lexer *lex = zalloc(sizeof(*lex));
    lex->input = file;

    size_t bufsize = 16;
    lex->sbuff = new_sbuffer(bufsize);

    lex->line = 1;
    lex->col = 0;

    *lexaddr = lex;

    return 1;
}

int lexer_scan_all(struct lexer *lex)
{
    int good = 1;
    while (good) {
        good = gettok(lex);
        printf("%s", get_tok_name(good));
        switch (good) {
            case TOK_IDENT:
                printf(": %s", lex->sbuff->buff);
                break;
            case TOK_STRING:
                printf(": \"%s\"", lex->sbuff->buff);
                break;
            case TOK_INT:
                printf(": %ld", lex->data.integer);
                break;
            case TOK_REAL:
                printf(": %f", lex->data.real);
                break;
            default:;
        }
        printf("\n");
    }
    return good;
}
