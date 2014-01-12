#include "parser.h"

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


static struct ast *statements(struct parser *parser);
static struct ast *statement(struct parser *parser);
static struct ast *ident_statement(struct parser *parser);
static struct ast *expression(struct parser *parser);
static struct ast *term(struct parser *parser);
static struct ast *list_literal(struct parser *parser);
static struct ast *map_literal(struct parser *parser);
static struct ast *arguments(struct parser *parser);
static struct ast *member(struct parser *parser);
static struct ast *imports(struct parser *parser);
static struct ast *structtype(struct parser *parser);
static struct ast *interface(struct parser *parser);

void parser_init(struct parser **parser_addr, struct lexer *lex)
{
    struct parser *parser = nalloc(sizeof(*parser));
    parser->lexer = lex;
    parser->buffer = lex->lastbuff;

    *parser_addr = parser;
}

struct ast *parse(struct parser *parser)
{
    next(parser);

    struct ast *root = statements(parser);

    parse_debug(parser, "Parsed entire file");

    return root;
}

static struct ast *statements(struct parser *parser)
{
    struct ast *statements = ast_make_list(LIST_STATEMENTS);
    struct ast* stmt = statement(parser);
    while (stmt != NULL) {
        statements = ast_list_append(statements, stmt);
        /* add statement to list */
        stmt = statement(parser);
    }
    return statements;
}

static struct ast *ident_statement(struct parser *parser)
{
    struct ast *ident = ast_make_ident(parser->buffer);

    if (check(parser, TOK_LPAREN)) {
        struct ast *args = arguments(parser);
        struct ast *call = ast_make_call(ident, args);
        parse_debug(parser, "Parsed function call statement");
        return call;
    } else if (accept(parser, TOK_DOT)) {
        member(parser);
        return NULL;    /* FIXME */
    }

    if (accept(parser, TOK_LSQUARE)) {
        expression(parser);
        expect(parser, TOK_RSQUARE);
        /* parsing container assignment */
    }

    if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
            parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
            parser->cur == TOK_IDIV || parser->cur == TOK_IMOD ||
            parser->cur == TOK_IPOW) {
        int ass = parser->cur; /* save assignment operator */
        next(parser);   /* eat assignment operator */
        struct ast *expr = expression(parser);
        struct ast *assignment = ast_make_assignment(ident, ass, expr);
        parse_debug(parser, "Parsed assignment");
        return assignment;
    } else {
        parse_error(parser, "Invalid token after ident, found %s",
                get_tok_name(parser->cur));
        next(parser);
        return NULL;
    }
}

static struct ast *expression(struct parser *parser)
{
    struct ast *lhs = NULL;
    if (parser->cur == TOK_NOT || parser->cur == TOK_SUB) {
        int unop = parser->cur;
        next(parser);
        struct ast *t = term(parser);
        lhs = ast_make_unexpr(unop, t);
    } else {
        lhs = term(parser);
    }

    if (parser->cur == TOK_ADD || parser->cur == TOK_SUB ||
            parser->cur == TOK_MUL || parser->cur == TOK_DIV ||
            parser->cur == TOK_POW || parser->cur == TOK_MOD ||
            parser->cur == TOK_EQ || parser->cur == TOK_NEQ ||
            parser->cur == TOK_LT || parser->cur == TOK_LTE ||
            parser->cur == TOK_GT || parser->cur == TOK_GTE) {
        int op = parser->cur;
        next(parser);
        struct ast *rhs = expression(parser);
        return ast_make_binexpr(lhs, op, rhs);
    } else {
        return lhs;
    }
}

static struct ast *term(struct parser *parser)
{
    if (accept(parser, TOK_CHAR)) {
        char c = parser->buffer[0];
        return ast_make_char_lit(c);
    } else if (accept(parser, TOK_INT)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        long l = strtol(parser->buffer, &endptr, 0);
        if (endptr != (parser->buffer + len)) {
            parse_error(parser, "Invalid integer %s", parser->buffer);
        }
        return ast_make_int_num(l);
    } else if (accept(parser, TOK_REAL)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        double d = strtod(parser->buffer, &endptr);
        if (endptr != (parser->buffer + len)) {
            parse_error(parser, "Invalid real number %s", parser->buffer);
        }
        return ast_make_real_num(d);
    } else if (accept(parser, TOK_STRING)) {
        return ast_make_str_lit(parser->buffer);
    } else if (accept(parser, TOK_IDENT)) {
        struct ast *ident = ast_make_ident(parser->buffer);
        if (check(parser, TOK_LPAREN)) {
            struct ast *args = arguments(parser);
            parse_debug(parser, "Parsed function call expression");
            return ast_make_call(ident, args);
        } else if (accept(parser, TOK_DOT)) {
            member(parser);
            return NULL;    /* FIXME */
        } else if (accept(parser, TOK_LSQUARE)) {
            expression(parser);
            expect(parser, TOK_RSQUARE);
            /* parsed container lookup */
            return NULL;
        } else {
            return ast_make_ident(parser->buffer);
        }
    } else if (accept(parser, TOK_LPAREN)) {
        struct ast *expr = expression(parser);
        expect(parser, TOK_RPAREN);
        return expr;
    } else if (check(parser, TOK_LSQUARE)) {
        return list_literal(parser);
    } else if (check(parser, TOK_RSQUARE)) {
        return map_literal(parser);
    } else {
        parse_error(parser, "Invalid terminal: %s", parser->buffer);
        next(parser);
        return NULL;
    }
}

static struct ast *list_literal(struct parser *parser)
{
    expect(parser, TOK_LSQUARE);
    struct ast* expr_list = ast_make_list(LIST_LITERAL);
    if (check(parser, TOK_RSQUARE)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* expr = expression(parser);
            expr_list = ast_list_append(expr_list, expr);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RSQUARE);
    return expr_list;
}

static struct ast *map_literal(struct parser *parser)
{
    expect(parser, TOK_LCURLY);
    struct ast* keyval_list = ast_make_list(LIST_MAP_ITEMS);
    if (check(parser, TOK_RCURLY)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* key = expression(parser);
            expect(parser, TOK_COLON);
            struct ast* val = expression(parser);
            struct ast *kv = ast_make_keyval(key, val);
            keyval_list = ast_list_append(keyval_list, kv);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RCURLY);
    return keyval_list;
}

static struct ast *arguments(struct parser *parser)
{
    expect(parser, TOK_LPAREN);
    struct ast* arg_list = ast_make_list(LIST_ARGS);
    if (check(parser, TOK_RPAREN)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* expr = expression(parser);
            arg_list = ast_list_append(arg_list, expr);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RPAREN);
    return arg_list;
}

static struct ast *member(struct parser *parser)
{
    do {
        accept(parser, TOK_IDENT);
        if (check(parser, TOK_LPAREN)) {
            arguments(parser);
        }
    } while (accept(parser, TOK_DOT));
    parse_debug(parser, "Parsed member");

    return NULL; /* FIXME */
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

static void declaration_type(struct parser *parser)
{
    if (accept(parser, TOK_LSQUARE)) {
        expect(parser, TOK_TYPE);
        expect(parser, TOK_RSQUARE);
    } else if (accept(parser, TOK_LCURLY)) {
        expect(parser, TOK_TYPE);
        expect(parser, TOK_COMMA);
        expect(parser, TOK_TYPE);
        expect(parser, TOK_RCURLY);
    } else {
        expect(parser, TOK_TYPE);
    }
}

static void declaration_name(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    if (accept(parser, TOK_ASS)) {
        expression(parser);
        parse_debug(parser, "Parsed initialization");
    } else {
        parse_debug(parser, "Parsed declaration");
    }
}

static void declaration_names(struct parser *parser)
{
    do {
        declaration_name(parser);
    } while (accept(parser, TOK_COMMA));
}

static void declarations(struct parser *parser)
{
    declaration_type(parser);
    declaration_name(parser);
}

static void declaration(struct parser *parser)
{
    declaration_type(parser);
    declaration_name(parser);
}

static void parameters(struct parser *parser)
{
    /* 1 decl, or many comma-separated decls */
    do {
        declaration(parser);
    } while (accept(parser, TOK_COMMA));
}

static void funcdef(struct parser *parser)
{
    accept(parser, TOK_FUNC);

    /* we don't know if this is a return type or the function name :( */
    expect(parser, TOK_TYPE);
    /* parse function name if the last token was the return type */
    accept(parser, TOK_IDENT);

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

static void funcdecl(struct parser *parser)
{
    expect(parser, TOK_FUNC);
    /* we don't know if this is a return type or the function name :( */
    expect(parser, TOK_TYPE);
    /* function name if the last token was the return type */
    accept(parser, TOK_IDENT);

    expect(parser, TOK_LPAREN);

    if (!check(parser, TOK_RPAREN)) {
        do {
            declaration_type(parser);
            accept(parser, TOK_TYPE);   /* optional parameter name */
        } while (accept(parser, TOK_COMMA));
    }

    expect(parser, TOK_RPAREN);

    parse_debug(parser, "Parsed function declaration");
}

static void typedefinition(struct parser *parser)
{
    if (check(parser, TOK_FUNC)) {
        funcdecl(parser);
    } else {
        expect(parser, TOK_TYPE);
    }
    expect(parser, TOK_IDENT);
    parse_debug(parser, "Parsed type definition");
}

static struct ast *statement(struct parser *parser)
{
/*
int x;
int y = 2 + 2;
x = 1;
x(y)
*/
    if (accept(parser, TOK_IDENT)) {
        if (check(parser, TOK_IDENT)) {
            declaration_names(parser);
        } else {
            return ident_statement(parser);
        }
    } else if (check(parser, TOK_LSQUARE) || check(parser, TOK_LCURLY)) {
        declarations(parser);
    } else if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        imports(parser);
    } else if (accept(parser, TOK_STRUCT)) {
        structtype(parser);
    } else if (accept(parser, TOK_IFACE)) {
        interface(parser);
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
        return NULL;
    }
    return NULL;
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

static struct ast *imports(struct parser *parser)
{
    while (import(parser) != 0) ;

    return NULL; /* FIXME */
}

static struct ast *structtype(struct parser *parser)
{
    expect(parser, TOK_IDENT);

    expect(parser, TOK_LCURLY);
    while (!check(parser, TOK_RCURLY)) {
        if (check(parser, TOK_FUNC)) {
            funcdecl(parser);
        } else {
            declarations(parser);
        }
    }
    expect(parser, TOK_RCURLY);
    parse_debug(parser, "Parsed `struct`");

    return NULL; /* FIXME */
}

static struct ast *interface(struct parser *parser)
{
    expect(parser, TOK_IDENT);

    expect(parser, TOK_LCURLY);
    while (!check(parser, TOK_RCURLY)) {
        funcdecl(parser);
    }
    expect(parser, TOK_RCURLY);
    parse_debug(parser, "Parsed `iface`");

    return NULL; /* FIXME */
}
