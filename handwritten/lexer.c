#include "lexer.h"


static char *tok_type_names[] = {
    "EOF",
    "bool", "char", "int", "real", "string", "file",
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
    "in",
    "typedef",
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

int lookup_keyword(struct lexer *lex)
{
    /* TODO: use hash-table or similar O(1) lookup */
    const char *keywords[] = {
        "if", "else", "while", "for", "break",
        "continue", "in", "typedef", "func", "return",
        "struct", "iface", "module", "import", "from",
    };
    unsigned int kidx = 0;
    for (kidx = 0; kidx < sizeof(keywords) / sizeof(*keywords); kidx++) {
        if (strncmp(lex->sbuff->buff, keywords[kidx], 16) == 0) {
            return TOK_IF + kidx;
        }
    }
    return 0;
}


/* djb2 (Daniel J. Bernstein):
 *
 * hash(i) = hash(i - 1) * 33 + str[i]
 *
 * Magic Constant 5381:
 *  1. odd number
 *  2. prime number
 *  3. deficient number
 *  4. 001/010/100/000/101 b
 *
 */
static unsigned int string_hash0(const char* s)
{
    unsigned int h = 5381;
    int c;

    while ((c = *s++))
        h = ((h << 5) + h) + c;
    return h;
}

/* SDBM:
 *
 * hash(i) = hash(i - 1) * 65599 + str[i]
 */
static unsigned int string_hash1(const char* s)
{
    unsigned int h = 0;
    int c;
    while ((c = *s++))
        h = c + (h << 6) + (h << 16) - h;
    return h;
}


/* One-at-a-time (Bob Jenkins)
 *
 * @param s LuciStringObj to hash
 * @returns unsigned integer hash
 */
static unsigned int string_hash_2(char *s)
{
    unsigned int h = 0;
    int c;
    while ((c = *s++)) {
        h += c;
        h += h << 10;
        h ^= h >> 6;
    }
    h += h << 3;
    h ^= h >> 11;
    h += h << 15;
    return h;
}

//#define GET_INDEX(H0, H1, I, N)   ( ( (H0) + ((I) * (I)) * (H1) ) % (N) )
#define GET_INDEX(H0, I, N)   (((H0) + (I)) % (N))

/** total number of possible hash table sizes */
#define MAX_TABLE_SIZE_OPTIONS  28
/**
 * Array of prime number hash table sizes.
 *
 * Numbers courtesy of:
 * http://planetmath.org/GoodHashTablePrimes.html
 */
static unsigned int TYPETABLE_SIZES[] = {
    7, 17, 43, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741, 0
};

static int typetable_do(struct typetable *table,
        const char *key, int val, int what);

static struct typetable *typetable_resize(struct typetable *tt, unsigned int new_size_idx)
{
    assert(tt);

    if (new_size_idx <= 0 || new_size_idx >= MAX_TABLE_SIZE_OPTIONS) {
        return tt;
    }

    unsigned int old_size = tt->size;
    char **old_names = tt->names;
    int *old_ids = tt->ids;

    tt->size_idx = new_size_idx;
    tt->size = TYPETABLE_SIZES[new_size_idx];
    tt->count = 0;

    tt->names = zalloc(tt->size * sizeof(*tt->names));
    tt->ids = zalloc(tt->size * sizeof(*tt->ids));

    unsigned int i;
    for (i = 0; i < old_size; i++) {
        if (old_names[i] != NULL) {
            typetable_do(tt, old_names[i], old_ids[i], TYPETABLE_INSERT);
        }
    }

    free(old_names);
    free(old_ids);

    return tt;
}

static struct typetable *typetable_grow(struct typetable *tt)
{
    return typetable_resize(tt, tt->size_idx + 1);
}

static struct typetable *typetable_shrink(struct typetable *tt)
{
    return typetable_resize(tt, tt->size_idx - 1);
}

struct typetable *new_typetable(void)
{
    struct typetable *table = zalloc(sizeof(*table));
    table->count = 0;
    table->size_idx = 1;    /* start off with 17 slots in table */
    table->size = TYPETABLE_SIZES[table->size_idx];
    table->names = zalloc(table->size * sizeof(*table->names));
    table->ids = zalloc(table->size * sizeof(*table->ids));

    const char *typenames[] = {
        "bool", "char", "int", "real", "str", "file"
    };
    unsigned int tidx = 0;
    for (tidx = 0; tidx < sizeof(typenames) / sizeof(*typenames); tidx++) {
        typetable_do(table, typenames[tidx], -1, TYPETABLE_INSERT);
    }

    return table;
}

/*
    SEARCH
    NULL            key missing, return 0
    strcmp = 0      key found, return
    else            loop
    ...             not found, return 0

    INSERT
    NULL            insert it, return
    strcmp = 0      key exists, update value, return
    else            loop
    ...             should never get here if table dynamically expands

    GET
    NULL            key missing, return 0
    strcmp = 0      found key, return val
    else            loop
    ...             not found, return 0
*/
static int typetable_do(struct typetable *table, const char *key, int val, int what)
{
    if (table->count > (table->size * 0.60)) {
        typetable_grow(table);
    }

    unsigned int hash0 = string_hash0(key);

    unsigned int i = 0, idx = 0;
    for (i = 0; i < table->size; i++) {
        unsigned int idx = GET_INDEX(hash0, i, table->size);
        const char *curkey  = table->names[idx];

        if (curkey == NULL) {
            if (what == TYPETABLE_INSERT) {
                table->names[idx] = (char *)key;
                if (val < 0) {
                    table->ids[idx] = table->count;
                } else {
                    table->ids[idx] = val;
                }
                table->count++;
                return val;
            } else {
                return -1;
            }
        } else if (strcmp(curkey, key) == 0) {
            /* Once a type is defined, it cannot be changed */
            return table->ids[idx];
        }
    }

    return -1;
}

int check_type(struct lexer *lex, const char *name)
{
    return typetable_do(lex->typetable, name, -1, TYPETABLE_SEARCH);
}

int add_type(struct lexer *lex, const char *name)
{
    return typetable_do(lex->typetable, name, -1, TYPETABLE_INSERT);
}

int readd_type(struct lexer *lex, const char *name, int id)
{
    return typetable_do(lex->typetable, name, id, TYPETABLE_INSERT);
}

int lex_ident(struct lexer *lex)
{
    do {
        appendc(lex, lex->cur);
        next(lex);
    } while (isalnum(lex->cur) || lex->cur == '_');

    int keyword = lookup_keyword(lex);
    if (keyword) {
        return keyword;
    }

    int id = check_type(lex, lex->sbuff->buff);
    if (id > -1) {
        lex->data.typeid = id;
        return TOK_TYPE;
    }

    /* otherwise, it's a normal identifier */
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
        else if (isalpha(lex->cur) || lex->cur == '_') {
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

    lex->typetable = new_typetable();

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
