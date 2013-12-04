#include "lexer.h"


struct parser {
    struct lexer *lexer;
    int cur;
};


void term(struct parser *parser);
void expression(struct parser *parser);
void statements(struct parser *parser);


void parse_error(struct parser *parser, char *msg, ...)
{
    va_list ap;

    fprintf(stderr, "Error (L %d): ", parser->lexer->line);

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    /* exit(1); */
}

#undef next
#define next(p)         ((p)->cur = gettok((p)->lexer))
#define check(p, t)     ((p)->cur == (t))

int accept(struct parser *parser, int tok)
{
    if (check(parser, tok)) {
        next(parser);
        return 1;
    }
    return 0;
}

int expect(struct parser *parser, int tok)
{
    if (accept(parser, tok)) {
        return 1;
    }

    parse_error(parser, "Unexpected token: %s , expecting %s",
            get_tok_name(parser->cur), get_tok_name(tok));
    return 0;
}

void expression(struct parser *parser)
{
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

void term(struct parser *parser)
{
    if (accept(parser, TOK_INT)) {
        ;
    } else if (accept(parser, TOK_FLOAT)) {
        ;
    } else if (accept(parser, TOK_STRING)) {
        ;
    } else if (accept(parser, TOK_IDENT)) {
        ;
    } else if (accept(parser, TOK_LPAREN)) {
        expression(parser);
        expect(parser, TOK_RPAREN);
    } else {
        parse_error(parser, "expression: invalid expression");
        next(parser);
    }
}

void condition(struct parser *parser) {
    expression(parser);
    if (parser->cur == TOK_EQ || parser->cur == TOK_NEQ ||
            parser->cur == TOK_LT || parser->cur == TOK_LTE ||
            parser->cur == TOK_GT || parser->cur == TOK_GTE) {
        next(parser);
        expression(parser);
    } else {
        parse_error(parser, "condition: invalid operator");
        next(parser);
    }
}

void arguments(struct parser *parser)
{
    expect(parser, TOK_LPAREN);
    if (accept(parser, TOK_RPAREN)) {
        ; /* function call with no args */
    } else {
        expression(parser);
        while (accept(parser, TOK_COMMA)) {
            expression(parser);
        }
        expect(parser, TOK_RPAREN);
    }
}

void ifelse(struct parser *parser)
{
    accept(parser, TOK_IF);
    condition(parser);
    expect(parser, TOK_LCURLY);
    statements(parser);
    expect(parser, TOK_RCURLY);
    if (accept(parser, TOK_ELSE)) {
        expect(parser, TOK_LCURLY);
        statements(parser);
        expect(parser, TOK_RCURLY);
    }
}

void whileloop(struct parser *parser)
{
    accept(parser, TOK_WHILE);
    condition(parser);
    expect(parser, TOK_LCURLY);
    statements(parser);
    expect(parser, TOK_RCURLY);
}

void forloop(struct parser *parser)
{
    accept(parser, TOK_FOR);
    condition(parser);
    expect(parser, TOK_LCURLY);
    statements(parser);
    expect(parser, TOK_RCURLY);
}

void parameters(struct parser *parser)
{

}

void funcdef(struct parser *parser)
{
    accept(parser, TOK_FUNC);
    expect(parser, TOK_IDENT);

    expect(parser, TOK_LPAREN);
    parameters(parser);
    expect(parser, TOK_RPAREN);

    expect(parser, TOK_LCURLY);
    statements(parser);
    expect(parser, TOK_RCURLY);
}

int statement(struct parser *parser)
{
/*
int x;
int y = 2 + 2;
x = 1;
x(y)
*/

    if (accept(parser, TOK_IDENT)) {
        if (accept(parser, TOK_IDENT)) {
            /* just parsed decl */
            if (accept(parser, TOK_ASS)) {
                expression(parser);
                /* just parsed declaration+initialization */
            }
        } else if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
                parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
                parser->cur == TOK_IDIV || parser->cur == TOK_IMOD ||
                parser->cur == TOK_IPOW) {
            next(parser);   /* eat assignment operator */
            expression(parser);
            /* just parsed assignment */
        } else if (check(parser, TOK_LPAREN)) {
            arguments(parser);
            /* just parsed a function call */
        } else {
            parse_error(parser, "statement: ident->??");
            next(parser);
            return 0;
        }
    } else if (check(parser, TOK_IF)) {
        ifelse(parser);
    } else if (check(parser, TOK_WHILE)) {
        whileloop(parser);
    } else if (check(parser, TOK_FUNC)) {
        funcdef(parser);
    } else {
        return 0;
        /* parse_error(parser, "statement: syntax parse_error"); */
        /* next(parser); */
    }
    return 1;
}

void statements(struct parser *parser)
{
    while (statement(parser) != 0) ;
}

void import(struct parser *parser)
{
    /* expect(parser, TOK_IMPORT); */
    expect(parser, TOK_IDENT);
    /* parsed import */
}

int struct_contents(struct parser *parser)
{

    return 0;
}

void structtype(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    expect(parser, TOK_LCURLY);
    while (struct_contents(parser)) {
        ;
    }
    expect(parser, TOK_RCURLY);
}

int interface_contents(struct parser *parser)
{

    return 0;
}

void interface(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    expect(parser, TOK_LCURLY);
    while (interface_contents(parser)) {
        ;
    }
    expect(parser, TOK_RCURLY);
}

int top_level_construct(struct parser *parser)
{
    if (accept(parser, TOK_IMPORT)) {
        import(parser);
    } else if (accept(parser, TOK_STRUCT)) {
        structtype(parser);
    } else if (accept(parser, TOK_IFACE)) {
        interface(parser);
    } else if (accept(parser, TOK_FUNC)) {
        funcdef(parser);
    } else {
        return 0;
    }
    return 1;
}

void module(struct parser *parser)
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
}


/******** Driver ********/
int main(void)
{
    struct lexer *lex;
    lexer_init(&lex);

    struct parser *parser = calloc(1, sizeof(*parser));
    parser->lexer = lex;

    module(parser);
/*
    int good = 1;
    while (good) {
        good = gettok(lex);
        printf("%s", get_tok_name(good));
        switch (good) {
            case TOK_IDENT:
                printf(": %s", lex->buff);
                break;
            case TOK_STRING:
                printf(": \"%s\"", lex->buff);
                break;
            case TOK_INT:
                printf(": %ld", lex->int_num);
                break;
            case TOK_FLOAT:
                printf(": %f", lex->float_num);
                break;
            default:;
        }
        printf("\n");
    }
*/
    return EXIT_SUCCESS;
}

/*
Grammar:

program = "module" ident ["import" ident {"," ident}] statements .

statements =
        statements statement .
    |   statement .

statement =
        funcdef
    |   assignment
    |   call
    |   ifelse
    |   loop .

ifelse = "if" expr "{" statements "}" ["else" "{" statements "}"] .

loop =
        "while" expr "{" statements "}"
    |   "for" expr ";" expr ";" expr ";" "{" statements "}" .

call = expr "(" [expr { "," expr }] ")" .

assignment = expr assop expr .

assop = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "^=" .

funcdef = ident "(" [ident { "," ident }] ")" "{" statements "}" .

expr =
        ident
    |   "(" expr ")"
    |   int
    |   float
    |   string
    |   unop expr
    |   expr binop expr .

unop = "-" | "!" .

binop = "+" | "-" | "*" | "/" | "%" | "^" | "==" | "!=" | "<" | "<=" | ">" | ">=" .

*/



