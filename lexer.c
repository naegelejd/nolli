#include "lexer.h"

#define LEX_ERRORF(L, fmt, ...) \
    do { \
        NOLLI_ERRORF("(L %d, C %d): " fmt, (L)->line, (L)->col, __VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while (0)

#define LEX_ERROR(L, S) LEX_ERRORF(L, "%s", S)

#undef next
#define next(lex) \
    do { \
        lex->cur = *lex->sptr++; \
        lex->col++; \
    } while (false)

static char *tok_type_names[] = {
    "EOF",
    "identifier",
    "bool", "char", "int", "real", "string",

    "+", "+=",
    "-", "-=",
    "*", "*=",
    "/", "/=",
    "%", "%=",
    "^", "^=",
    ":", ":=",
    "=", "==",
    "!", "!=",
    "<", ">=",
    ">", ">=",
    "||", "&&",

    "(", ")", "[", "]", "{", "}",
    ",", ";", ".",

    "var", "const",
    "if", "else",
    "while", "for",
    "break", "continue",
    "in",
    "alias",
    "func", "return",
    "struct", "iface",
    "module", "import", "from",
};

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
    } else {
        /* eat numbers and extra (BAD) decimal points and let `strtod`
         * handle errors in the parser */
        do {
            appendc(lex, lex->cur);
            next(lex);
        } while (isdigit(lex->cur) || lex->cur == '.');
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

    /* eat all numbers/letters and let `strtol` handle errors in parser */
    while (isalnum(lex->cur)) {
        appendc(lex, lex->cur);
        next(lex);
    }
    /* if (strchr("xX", lex->cur)) { */
    /*     appendc(lex, lex->cur); */
    /*     next(lex); */
    /*     while (strchr("abcdefABCDEF123456890", lex->cur)) { */
    /*         appendc(lex, lex->cur); */
    /*         next(lex); */
    /*     } */
    /* } */

    return TOK_INT;
}

static void lex_escape(struct lexer *lex)
{
    switch (lex->cur) {
        case '"':
            appendc(lex, '"');
            break;
        case '\\':
            appendc(lex, '\\');
            break;
        case 'a':
            appendc(lex, '\a');
            break;
        case 'b':
            appendc(lex, '\b');
            break;
        case 'f':
            appendc(lex, '\f');
            break;
        case 'n':
            appendc(lex, '\n');
            break;
        case 'r':
            appendc(lex, '\r');
            break;
        case 't':
            appendc(lex, '\t');
            break;
        case 'v':
            appendc(lex, '\v');
            break;
        case '\n': case '\r':   /* FIXME - Windows */
            lex->line++;
            lex->col = 0;
            appendc(lex, lex->cur);
            break;
        case EOF: case 0:
            LEX_ERROR(lex, "Unexpected EOF in string literal");
            break;
        default:
            LEX_ERRORF(lex, "Invalid escape sequence \\%c", lex->cur);
    }
    next(lex);
}

static int lex_string(struct lexer *lex)
{
    /* eat the starting string delimiter */
    next(lex);
    do {
        switch (lex->cur) {
        case EOF: case 0:
            LEX_ERROR(lex, "Unexpected EOF");
            break;
        case '\n': case '\r':
            LEX_ERROR(lex, "Unterminated string literal");
            break;
        case '\\':
            next(lex);
            lex_escape(lex);
            break;
        default:
            /* normal character in string literal */
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
        "var", "const", "if", "else", "while", "for",
        "break", "continue", "in", "alias", "func", "return",
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
        case ':': tok = TOK_COLON; break;
        case '=': tok = TOK_ASS; break;
        case '!': tok = TOK_NOT; break;
        case '<': tok = TOK_LT; break;
        case '>': tok = TOK_GT; break;
        case '|':
            appendc(lex, lex->cur);
            next(lex);
            if (lex->cur != '|') {
                LEX_ERRORF(lex, "Invalid symbol |%c", lex->cur);
            }
            appendc(lex, lex->cur);
            next(lex);
            return TOK_OR;
            break;
        case '&':
            appendc(lex, lex->cur);
            next(lex);
            if (lex->cur != '&') {
                LEX_ERRORF(lex, "Invalid symbol &%c", lex->cur);
            }
            appendc(lex, lex->cur);
            next(lex);
            return TOK_AND;
            break;
        default: {
            static char symbols[] = "()[]{},;.";
            char *at = NULL;
            if ((at = strchr(symbols, lex->cur))) {
                appendc(lex, lex->cur);
                next(lex);
                return (at - symbols) + TOK_LPAREN;
            } else {
                LEX_ERRORF(lex, "Invalid symbol %c (0x%0x)", lex->cur, lex->cur);
            }
        }
    }

    appendc(lex, lex->cur);
    next(lex);
    if (lex->cur == '=') {
        appendc(lex, lex->cur);
        next(lex);
        /* in-place operations follow their base op equivalent */
        return tok + 1;
    }
    return tok;
}

int gettok(struct lexer *lex)
{
    rotate_buffers(lex);    /* clear the lexer's current string buffer */

    while (true) {
        int tok = TOK_EOF;

        if (lex->cur == '\0') {
            tok = TOK_EOF;
            lex->line++;
            lex->col = 0;
            switch (lex->lasttok) {
                case TOK_IDENT: case TOK_BOOL: case TOK_CHAR: case TOK_INT:
                case TOK_REAL: case TOK_STRING: case TOK_RPAREN: case TOK_RCURLY:
                case TOK_RSQUARE: case TOK_RET: case TOK_BREAK: case TOK_CONT:
                    tok = TOK_SEMI;
                    break;
                default:
                    tok = TOK_EOF;
            }
        }
        /* FIXME: Windows line-endings? */
        else if (lex->cur == '\n') {
            lex->line++;
            lex->col = 0;
            next(lex);

            switch (lex->lasttok) {
                case TOK_IDENT: case TOK_BOOL: case TOK_CHAR: case TOK_INT:
                case TOK_REAL: case TOK_STRING: case TOK_RPAREN: case TOK_RCURLY:
                case TOK_RSQUARE: case TOK_RET: case TOK_BREAK: case TOK_CONT:
                    tok = TOK_SEMI;
                    break;
                default:
                    continue;
            }
        }
        /* eat whitespace */
        else if (isspace(lex->cur)) {
            next(lex);
            continue;
        }
        else if (isdigit(lex->cur)) {
            tok = lex_integer(lex);
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
            tok = TOK_CHAR;
        }
        else if (lex->cur == '"') {
            tok = lex_string(lex);
        }
        else if (isalpha(lex->cur) || lex->cur == '_') {
            tok = lex_ident(lex);
        }
        /* stupid floating points with no leading zero (e.g. '.123') */
        else if (lex->cur == '.') {
            appendc(lex, lex->cur);
            next(lex);
            if (isdigit(lex->cur)) {
                tok = lex_real(lex);
            } else {
                tok = TOK_DOT;
            }
        }
        else if (lex->cur == EOF) {
            tok = TOK_EOF;
        }
        /* all that's left is symbols */
        else {
            tok = lex_symbol(lex);
        }

        /* return the scanned token */
        lex->lasttok = tok;
        /* NOLLI_DEBUGF("tok: %s", get_tok_name(tok)); */
        return tok;
    }

    return TOK_EOF;
}

const char *get_tok_name(int tok)
{
    assert(tok >= TOK_EOF);
    return tok_type_names[tok];
}

void lexer_init(struct lexer *lexer)
{
    assert(lexer);

    size_t bufsize = 16;
    lexer->curbuff = nalloc(bufsize);
    lexer->lastbuff = nalloc(bufsize);
    lexer->blen = 0;
    lexer->balloc = bufsize;

    lexer->line = 1;
    lexer->col = 0;
}

int lexer_set(struct lexer *lexer, char *buffer)
{
    assert(lexer);
    assert(buffer);

    lexer->input = buffer;
    lexer->sptr = lexer->input;

    /* reset these: */
    lexer->line = 1;
    lexer->col = 0;

    /* sync lexer on first char in input */
    next(lexer);

    return NO_ERR;
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
