#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>

/******** Memory allocation ********/
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

/******** String implementation ********/
enum { STRING_DEFAULT_LENGTH = 16 };

struct string {
    char* val;
    unsigned long len;
    size_t alloc;
};
typedef struct string string_t;

string_t* string_new(void)
{
    string_t* s = zalloc(sizeof(*s));
    s->alloc = STRING_DEFAULT_LENGTH;
    s->val = zalloc(s->alloc);
    s->len = 0;
    return s;
}

int string_expand(string_t* s)
{
    assert(s);

    size_t old_alloc = s->alloc;
    size_t new_alloc = old_alloc * 2;

    /* realloc the string buffer */
    s->val = zrealloc(s->val, new_alloc);
    s->alloc = new_alloc;

    /* memory for strings must always be zeroed */
    memset(s->val + old_alloc, 0, new_alloc - old_alloc);

    return 0;
}

int string_clear(string_t* s)
{
    memset(s->val, 0, s->alloc);
    s->len = 0;
    return 0;
}

int string_append_char(string_t* s, char c)
{
    assert(s);

    /* expand string buffer if it's value
     * (including nul terminator) is too long */
    if (s->len >= s->alloc - 1) {
        string_expand(s);
    }

    s->val[s->len++] = c;

    return 0;
}

void string_delete(string_t* s)
{
    free(s->val);
    free(s);
}

/******** Lexer ********/

enum {
    tok_eof = 0,
    tok_num,
    tok_ident,
    tok_def,
    tok_extern
};

static FILE* g_input;
static string_t* g_ident_str;
static string_t* g_num_str;

int gettok(void)
{
    static int lastc = ' ';

    while (isspace(lastc)) {
        lastc = getc(g_input);
    }

    if (isalpha(lastc)) {
        string_clear(g_ident_str);
        do {
            string_append_char(g_ident_str, lastc);
            lastc = getc(g_input);
        } while (isalnum(lastc));

        if (strncmp(g_ident_str->val, "def",
                    g_ident_str->alloc) == 0) {
            return tok_def;
        } else if (strncmp(g_ident_str->val, "extern",
                    g_ident_str->alloc) == 0) {
            return tok_extern;
        } else {
            return tok_ident;
        }
    }

    else if (isdigit(lastc)) {
        string_clear(g_num_str);
        do {
            string_append_char(g_num_str, lastc);
            lastc = getc(g_input);
        } while (isdigit(lastc) || lastc == '.');

        return tok_num;
    }

    else if (lastc == '#') {
        do {
            /* eat up comment line */
            lastc = getc(g_input);
        } while (lastc != EOF && lastc != '\n' && lastc != '\r');

        /* if there are more lines, continue scanning */
        if (lastc != EOF) {
            return gettok();
        }
    }

    else if (lastc == EOF) {
        return tok_eof;
    }

    else {
        /* return ascii value */
        int thisc = lastc;
        lastc = getc(g_input);
        return thisc;
    }

    return 0;
}

/******** Driver ********/
int main(void)
{
    g_input = stdin;
    g_ident_str = string_new();
    g_num_str = string_new();

    int good = 1;

    while (good) {
        good = gettok();
        printf("%d\n", good);
    }

    return EXIT_SUCCESS;
}
