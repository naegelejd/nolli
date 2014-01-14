#include "lexer.h"

static char *tok_type_names[] = {
    "EOF",
    "identifier",
    "bool", "char", "int", "real", "string", "file",

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

    "var", "const",
    "if", "else",
    "while", "for",
    "break", "continue",
    "in",
    "typedef",
    "func", "return",
    "struct", "iface",
    "module", "import", "from",
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

/* returns former length of string in buffer */
static int rotate_buffers(struct lexer *lex)
{
    assert(lex);
    assert(lex->curbuff);
    assert(lex->lastbuff);

    memset(lex->lastbuff, 0, lex->balloc);
    strncpy(lex->lastbuff, lex->curbuff, lex->balloc);

    memset(lex->curbuff, 0, lex->balloc);

    size_t len = lex->blen;
    lex->blen = 0;

    return len;
}

static int appendc(struct lexer *lex, int c)
{
    assert(lex);
    assert(lex->curbuff);

    /* expand string buffer if it's value
     * (including nul terminator) is too long */
    if (lex->blen >= lex->balloc - 1) {
        size_t old_alloc = lex->balloc;
        size_t new_alloc = old_alloc * 2;

        /* realloc the buffer */
        lex->curbuff = nrealloc(lex->curbuff, new_alloc);
        lex->lastbuff = nrealloc(lex->lastbuff, new_alloc);
        lex->balloc = new_alloc;

        /* memory for strings must always be zeroed */
        memset(lex->curbuff + old_alloc, 0, new_alloc - old_alloc);
        memset(lex->lastbuff + old_alloc, 0, new_alloc - old_alloc);
    }

    lex->curbuff[lex->blen++] = (char)c;
    return c;
}

/* to be called with decimal point as lex->cur or already in buffer */
static int lex_real(struct lexer *lex)
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

    return TOK_REAL;
}

static int lex_integer(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isdigit(lex->cur));

    if (lex->cur == '.' || strchr("eE", lex->cur)) {
        return lex_real(lex);
    }

    if (strchr("xX", lex->cur)) {
        appendc(lex, lex->cur);
        next(lex);
        while (strchr("abcdefABCDEF123456890", lex->cur)) {
            appendc(lex, lex->cur);
            next(lex);
        }
    }

    return TOK_INT;
}

static int lex_string(struct lexer *lex)
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

static int lookup_keyword(struct lexer *lex)
{
    /* TODO: use hash-table or similar O(1) lookup */
    const char *keywords[] = {
        "var", "const", "if", "else", "while", "for", "break",
        "continue", "in", "typedef", "func", "return",
        "struct", "iface", "module", "import", "from",
    };
    unsigned int kidx = 0;
    for (kidx = 0; kidx < sizeof(keywords) / sizeof(*keywords); kidx++) {
        if (strncmp(lex->curbuff, keywords[kidx], 16) == 0) {
            return TOK_VAR + kidx;
        }
    }
    return 0;
}

static int lex_ident(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isalnum(lex->cur) || lex->cur == '_');

    int keyword = lookup_keyword(lex);
    if (keyword) {
        return keyword;
    }

    /* otherwise, it's an identifier */
    return TOK_IDENT;
}

static int lex_symbol(struct lexer *lex)
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
                appendc(lex, lex->cur);
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
    rotate_buffers(lex);    /* clear the lexer's current string buffer */

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
            appendc(lex, lex->cur);
            next(lex);
            next(lex);  /* skip closing ' */
            return TOK_CHAR;
        }
        else if (lex->cur == '"') {
            return lex_string(lex);
        }
        else if (isalpha(lex->cur) || lex->cur == '_') {
            return lex_ident(lex);
        }
        /* stupid floating points with no leading zero (e.g. '.123') */
        /* else if (lex->cur == '.') { */
        /*     appendc(lex, lex->cur); */
        /*     next(lex); */
        /*     if (isdigit(lex->cur)) { */
        /*         return lex_real(lex); */
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

void lexer_init(struct lexer **lexaddr, FILE *file)
{
    struct lexer *lex = nalloc(sizeof(*lex));
    lex->input = file;

    size_t bufsize = 16;
    lex->curbuff = nalloc(bufsize);
    lex->lastbuff = nalloc(bufsize);
    lex->blen = 0;
    lex->balloc = bufsize;

    lex->line = 1;
    lex->col = 0;

    *lexaddr = lex;
}

int lexer_scan_all(struct lexer *lex)
{
    int good = 1;
    while (good) {
        good = gettok(lex);
        printf("%s", get_tok_name(good));
        switch (good) {
            case TOK_IDENT:
            case TOK_INT:
            case TOK_REAL:
                printf(": %s", lex->curbuff);
                break;
            case TOK_CHAR:
                printf(": '%c'", lex->curbuff[0]);
            case TOK_STRING:
                printf(": \"%s\"", lex->curbuff);
                break;
            default:;
        }
        printf("\n");
    }
    return good;
}
