#include "parser.h"

static void term(struct parser *parser);
static void expression(struct parser *parser);
static void arguments(struct parser *parser);
static int statement(struct parser *parser);
static void statements(struct parser *parser);


static void parse_error(struct parser *parser, char *msg, ...)
{
    parser->error = true;

    va_list ap;

    fprintf(stderr, "ERROR (L %d, C %d): ",
            parser->lexer->line, parser->lexer->col);

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    /* exit(1); */
}

static void parse_debug(struct parser *parser, char *msg, ...)
{
    va_list ap;

    fprintf(stderr, "DEBUG (L %d, C %d): ",
            parser->lexer->line, parser->lexer->col);

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fprintf(stderr, "\n");
}

#undef next
#define next(p)         ((p)->cur = gettok((p)->lexer))
#define check(p, t)     ((p)->cur == (t))

static int accept(struct parser *parser, int tok)
{
    if (check(parser, tok)) {
        next(parser);
        return 1;
    }
    return 0;
}

static int expect(struct parser *parser, int tok)
{
    if (accept(parser, tok)) {
        return 1;
    }

    parse_error(parser, "Unexpected token: %s , expecting %s",
            get_tok_name(parser->cur), get_tok_name(tok));
    return 0;
}

static void expression(struct parser *parser)
{
    if (accept(parser, TOK_NOT)) {
        ;
    } else if (accept(parser, TOK_SUB)) {
        ;
    }

    term(parser);

    while (parser->cur == TOK_ADD || parser->cur == TOK_SUB ||
            parser->cur == TOK_MUL || parser->cur == TOK_DIV ||
            parser->cur == TOK_POW || parser->cur == TOK_MOD ||
            parser->cur == TOK_EQ || parser->cur == TOK_NEQ ||
            parser->cur == TOK_LT || parser->cur == TOK_LTE ||
            parser->cur == TOK_GT || parser->cur == TOK_GTE) {
        next(parser);
        term(parser);
    }
}

static void list_literal(struct parser *parser)
{
    expect(parser, TOK_LSQUARE);
    if (check(parser, TOK_RSQUARE)) {
        ; /* empty list */
    } else {
        do {
            expression(parser);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RSQUARE);
}

static void map_literal(struct parser *parser)
{
    expect(parser, TOK_LCURLY);
    if (check(parser, TOK_RCURLY)) {
        ; /* empty list */
    } else {
        do {
            expression(parser);
            expect(parser, TOK_COLON);
            expression(parser);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RCURLY);
}

static void member(struct parser *parser)
{
    do {
        accept(parser, TOK_IDENT);
        if (check(parser, TOK_LPAREN)) {
            arguments(parser);
        }
    } while (accept(parser, TOK_DOT));
    parse_debug(parser, "Parsed member");
}

static void term(struct parser *parser)
{
    if (accept(parser, TOK_CHAR)) {
        char c = parser->lexer->lastbuff[0];
    } else if (accept(parser, TOK_INT)) {
        struct lexer *lex = parser->lexer;
        char *endptr = NULL;
        size_t blen = strnlen(lex->lastbuff, lex->balloc);
        long l = strtol(lex->lastbuff, &endptr, 0);
        if (endptr != (lex->lastbuff + blen)) {
            parse_error(parser, "Invalid integer %s", lex->lastbuff);
        }
    } else if (accept(parser, TOK_REAL)) {
        struct lexer *lex = parser->lexer;
        char *endptr = NULL;
        size_t blen = strnlen(lex->lastbuff, lex->balloc);
        double d = strtod(lex->lastbuff, &endptr);
        if (endptr != (lex->lastbuff + blen)) {
            parse_error(parser, "Invalid real number %s", lex->lastbuff);
        }
    } else if (accept(parser, TOK_STRING)) {
        ;
    } else if (accept(parser, TOK_IDENT)) {
        if (check(parser, TOK_LPAREN)) {
            arguments(parser);
            parse_debug(parser, "Parsed function call");
        } else if (accept(parser, TOK_DOT)) {
            member(parser);
        }
        /* else, parsed identifier */
    } else if (accept(parser, TOK_LPAREN)) {
        expression(parser);
        expect(parser, TOK_RPAREN);
    } else if (check(parser, TOK_LSQUARE)) {
        list_literal(parser);
    } else if (check(parser, TOK_LCURLY)) {
        map_literal(parser);
    } else {
        parse_error(parser, "expression: invalid expression");
        next(parser);
    }
}

static void arguments(struct parser *parser)
{
    expect(parser, TOK_LPAREN);
    if (check(parser, TOK_RPAREN)) {
        ; /* function call with no args */
    } else {
        do {
            expression(parser);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RPAREN);
}

/**
 * Accept either multiple statements between curly braces, or
 * a single statement
 */
static void body(struct parser *parser)
{
    if (accept(parser, TOK_LCURLY)) {
        statements(parser);
        expect(parser, TOK_RCURLY);
    } else {
        statement(parser);
    }
}

static void ifelse(struct parser *parser)
{
    accept(parser, TOK_IF);
    expression(parser);
    body(parser);

    if (accept(parser, TOK_ELSE)) {
        body(parser);
        parse_debug(parser, "Parsed `if+else` construct");
    } else {
        parse_debug(parser, "Parsed `if` construct");
    }
}

static void whileloop(struct parser *parser)
{
    accept(parser, TOK_WHILE);
    expression(parser);
    body(parser);
    parse_debug(parser, "Parsed `while` loop");
}

static void forloop(struct parser *parser)
{
    accept(parser, TOK_FOR);
    expect(parser, TOK_IDENT);
    expect(parser, TOK_IN);
    expression(parser);
    body(parser);
    parse_debug(parser, "Parsed `for` loop");
}

static void declaration(struct parser *parser)
{
    if (accept(parser, TOK_LSQUARE)) {
        expect(parser, TOK_TYPE);
        expect(parser, TOK_RSQUARE);
        /* parsed list type */
    } else if (accept(parser, TOK_LCURLY)) {
        expect(parser, TOK_TYPE);
        expect(parser, TOK_COMMA);
        expect(parser, TOK_TYPE);
        expect(parser, TOK_RCURLY);
        /* parsed map type */
    } else {
        expect(parser, TOK_TYPE);
    }

    do {
        expect(parser, TOK_IDENT);
        if (accept(parser, TOK_ASS)) {
            expression(parser);
            parse_debug(parser, "Parsed initialization");
        } else {
            parse_debug(parser, "Parsed declaration");
        }
    } while (accept(parser, TOK_COMMA));
}

static void parameters(struct parser *parser)
{
    /* 1 decl, or many comma-separated decls */
    do {
        declaration(parser);
    } while (check(parser, TOK_COMMA));
}

static void funcdef(struct parser *parser)
{
    accept(parser, TOK_FUNC);
    if (accept(parser, TOK_TYPE)) {
        /* function has a return value */
        ;
    }
    expect(parser, TOK_IDENT);

    expect(parser, TOK_LPAREN);

    if (!check(parser, TOK_RPAREN)) {
        parameters(parser);
    }
    expect(parser, TOK_RPAREN);

    expect(parser, TOK_LCURLY);
    statements(parser);
    expect(parser, TOK_RCURLY);

    parse_debug(parser, "Parsed function definition");
}

static int ident_statement(struct parser *parser)
{
    expect(parser, TOK_IDENT);

    if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
            parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
            parser->cur == TOK_IDIV || parser->cur == TOK_IMOD ||
            parser->cur == TOK_IPOW) {
        next(parser);   /* eat assignment operator */
        expression(parser);
        parse_debug(parser, "Parsed variable assignment");
    } else if (check(parser, TOK_LPAREN)) {
        arguments(parser);
        parse_debug(parser, "Parsed function call");
    } else {
        parse_error(parser, "Invalid token after ident, expecting %s",
                get_tok_name(parser->cur));
        next(parser);
        return 0;
    }
    return 1;
}

static void functype(struct parser *parser)
{
    accept(parser, TOK_FUNC);
    accept(parser, TOK_TYPE);
    accept(parser, TOK_IDENT);
    expect(parser, TOK_LPAREN);
    if (!check(parser, TOK_RPAREN)) {
        do {
            expect(parser, TOK_TYPE);
            accept(parser, TOK_IDENT);  /* optional param name */
        } while (accept(parser, TOK_COMMA));    /* optional param name */
    }
    expect(parser, TOK_RPAREN);
}

static void typedefinition(struct parser *parser)
{
    if (check(parser, TOK_FUNC)) {
        functype(parser);
    } else {
        expect(parser, TOK_TYPE);
    }
    expect(parser, TOK_IDENT);
    add_type(parser->lexer, parser->lexer->lastbuff);
    parse_debug(parser, "Parsed type definition");
}

int statement(struct parser *parser)
{
/*
int x;
int y = 2 + 2;
x = 1;
x(y)
*/
    if (check(parser, TOK_TYPE) ||
            check(parser, TOK_LSQUARE) ||
            check(parser, TOK_LCURLY)) {
        declaration(parser);
        /* just parsed decl or initialization */
    } else if (check(parser, TOK_IDENT)) {
        return ident_statement(parser);
    } else if (check(parser, TOK_IF)) {
        ifelse(parser);
    } else if (check(parser, TOK_WHILE)) {
        whileloop(parser);
    } else if (check(parser, TOK_FOR)) {
        forloop(parser);
    } else if (check(parser, TOK_FUNC)) {
        funcdef(parser);
    } else if (accept(parser, TOK_RET)) {
        /* TODO: empty return statements? */
        expression(parser);
        parse_debug(parser, "Parsed a return statement");
    } else if (accept(parser, TOK_BREAK)) {
        ;
    } else if (accept(parser, TOK_CONT)) {
        ;
    } else if (accept(parser, TOK_TYPEDEF)) {
        typedefinition(parser);
    } else {
        /* right-curly brace is the only token that would end a series of
         * statements. anything else would be an invalid first token */
        if (!check(parser, TOK_RCURLY)) {
            parse_error(parser, "Unexpected token %s", get_tok_name(parser->cur));
            next(parser);
        }
        return 0;
    }
    return 1;
}

static void statements(struct parser *parser)
{
    while (statement(parser) != 0) ;
}

static int import(struct parser *parser)
{
    if (accept(parser, TOK_IMPORT)) {
        ;
    } else if (accept(parser, TOK_FROM)) {
        expect(parser, TOK_IDENT);
        expect(parser, TOK_IMPORT);
    } else {
        return 0;
    }

    do {
        expect(parser, TOK_IDENT);
    } while (accept(parser, TOK_COMMA));

    parse_debug(parser, "Parsed `import`");

    return 1;
}

static void imports(struct parser *parser)
{
    while (import(parser) != 0) ;
}

static void structtype(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    add_type(parser->lexer, parser->lexer->lastbuff);

    expect(parser, TOK_LCURLY);
    while (!check(parser, TOK_RCURLY)) {
        if (check(parser, TOK_FUNC)) {
            functype(parser);
        } else {
            declaration(parser);
        }
    }
    expect(parser, TOK_RCURLY);
    parse_debug(parser, "Parsed `struct`");
}

static void funcdecl(struct parser *parser)
{
    accept(parser, TOK_FUNC);
    accept(parser, TOK_TYPE);   /* function has a return value */

    expect(parser, TOK_IDENT);  /* function name */

    expect(parser, TOK_LPAREN);

    if (!check(parser, TOK_RPAREN)) {
        do {
            accept(parser, TOK_TYPE);
        } while (accept(parser, TOK_COMMA));
    }

    expect(parser, TOK_RPAREN);

    parse_debug(parser, "Parsed function declaration");
}

static void interface(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    add_type(parser->lexer, parser->lexer->lastbuff);

    expect(parser, TOK_LCURLY);
    while (!check(parser, TOK_RCURLY)) {
        funcdecl(parser);
    }
    expect(parser, TOK_RCURLY);
    parse_debug(parser, "Parsed `iface`");
}

static int top_level_construct(struct parser *parser)
{
    if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        imports(parser);
    } else if (accept(parser, TOK_STRUCT)) {
        structtype(parser);
    } else if (accept(parser, TOK_IFACE)) {
        interface(parser);
    } else if (check(parser, TOK_FUNC)) {
        funcdef(parser);
    } else if (accept(parser, TOK_TYPEDEF)) {
        typedefinition(parser);
    } else {
        return 0;
    }
    return 1;
}

void parse_module(struct parser *parser)
{
    next(parser);   /* read first token */

    if (accept(parser, TOK_EOF)) {
        return;
    } else {
        if (!expect(parser, TOK_MODULE) || !expect(parser, TOK_IDENT)) {
            parse_error(parser, "Missing `module <identifier>`\n");
        }
        while (top_level_construct(parser) != 0) ;
        expect(parser, TOK_EOF);
    }
    parse_debug(parser, "Parsed entire file");
}
