#include "parser.h"

#define PARSE_DEBUG(P, S) \
        NOLLI_DEBUGF("(L %d, C %d): " S, \
                (P)->lexer->line, (P)->lexer->col)

#define PARSE_ERROR(P, S) \
        NOLLI_ERRORF("(L %d, C %d): " S, \
                (P)->lexer->line, (P)->lexer->col)

#define PARSE_ERRORF(P, fmt, ...) \
    do { \
        NOLLI_ERRORF("(L %d, C %d): " fmt, \
                (P)->lexer->line, (P)->lexer->col, __VA_ARGS__); \
    } while (0)


static int parse(struct nolli_state *state);
static int statements(struct parser *parser, struct ast **);
static int statement(struct parser *parser, struct ast **);
static int assignment_or_call(struct parser *parser, struct ast **);
static int expression(struct parser *parser, struct ast **);
static int unary_expr(struct parser *parser, struct ast **);
static int term(struct parser *parser, struct ast **);
static int operand(struct parser *parser, struct ast **);
static int list_literal(struct parser *parser, struct ast **);
static int map_literal(struct parser *parser, struct ast **);
static int funclit(struct parser *parser, struct ast **);
static int parameters(struct parser *parser, struct ast **);
static int arguments(struct parser *parser, struct ast **);
static int block(struct parser *parser, struct ast **);
static int return_statement(struct parser *parser, struct ast **);
static int ifelse(struct parser *parser, struct ast **);
static int whileloop(struct parser *parser, struct ast **);
static int forloop(struct parser *parser, struct ast **);
static int var_declaration(struct parser *parser, struct ast **);
static int const_declaration(struct parser *parser, struct ast **);
static int declaration_type(struct parser *parser, struct ast **);
static int declaration_names(struct parser *parser, struct ast **);
static int functype(struct parser *parser, struct ast **);
static int alias(struct parser *parser, struct ast **);
static int import(struct parser *parser, struct ast **);
static int structtype(struct parser *parser, struct ast **);
static int interface(struct parser *parser, struct ast **);

#undef next
#define next(P)         ((P)->cur = gettok((P)->lexer))
#define check(P, T)     ((P)->cur == (T))

#define accept(P, T) (check(P, T) ? next(P), true : false)

#define expect(P, T, S) \
    do { \
        if (!accept(P, T)) { \
            S = ERR_PARSE; \
            PARSE_ERRORF(P, "Unexpected token: %s, expecting %s", \
                    get_tok_name((P)->cur), get_tok_name(T)); \
        } \
    } while (0)


void parser_init(struct parser *parser)
{
    assert(parser);

    parser->lexer = nalloc(sizeof(*parser->lexer));
    lexer_init(parser->lexer);

    parser->buffer = parser->lexer->lastbuff;
}

int parse_file(struct nolli_state *state, FILE *fin)
{
    struct parser *parser = state->parser;
    assert(parser);

    int ret = lexer_set(parser->lexer, fin, LEX_FILE);
    if (ret != NO_ERR) {
        return ret;
    }
    return parse(state);
}

int parse_string(struct nolli_state *state, char *s)
{
    struct parser *parser = state->parser;
    assert(parser);

    int ret = lexer_set(parser->lexer, s, LEX_STRING);
    if (ret != NO_ERR) {
        return ret;
    }
    return parse(state);
}

static int parse(struct nolli_state *state)
{
    struct parser *parser = state->parser;
    assert(parser);

    /* read first char */
    next(parser);

    int err = statements(parser, &state->root);

    /* PARSE_DEBUG(parser, "Finished parsing"); */

    if (state->root == NULL) {
        return ERR_AST;
    }
    return err;
}


/**
 *  -----------------------
 * | stmt  |  err  | result
 * |-------|------ |--------
 * | good  | true  | part of statement is bad, continue anyway
 * | good  | false | statement parsed, continue
 * | null  | true  | statement all fucked up, probably stop parsing
 * | null  | false | end of statements
 *  -----------------------
 */
static int statements(struct parser *parser, struct ast **root)
{
    int err, status = NO_ERR;

    struct ast *statements = ast_make_list(LIST_STATEMENT);
    struct ast *stmt = NULL;

    while (true) {
        /* parse a single statement */
        err = statement(parser, &stmt);
        if (stmt == NULL) {
            /* no more statements in this 'block' - stop parsing */
            break;
        } else if (err) {
            status = err;
            /* synchronize on statement terminator (semicolon) */
            while (parser->cur != TOK_SEMI && parser->cur != TOK_EOF) {
                printf("synchronizing\n");
                next(parser);
            }
            if (parser->cur == TOK_EOF) {
                break;
            }
        }

        expect(parser, TOK_SEMI, status);
        /* add statement to list */
        statements = ast_list_append(statements, stmt);
        /* reset the node to which stmt points */
        stmt = NULL;
    }

    *root = statements;
    return status;
}

static int statement(struct parser *parser, struct ast **stmt)
{
    if (accept(parser, TOK_EOF)) {
        *stmt = NULL;   /* sentinel */
        return NO_ERR;
    } else if (check(parser, TOK_RCURLY)) {
        return NO_ERR;
    } else if (accept(parser, TOK_STRUCT)) {
        return structtype(parser, stmt);
    } else if (accept(parser, TOK_IFACE)) {
        return interface(parser, stmt);
    } else if (accept(parser, TOK_IF)) {
        return ifelse(parser, stmt);
    } else if (accept(parser, TOK_WHILE)) {
        return whileloop(parser, stmt);
    } else if (accept(parser, TOK_FOR)) {
        return forloop(parser, stmt);
    } else if (accept(parser, TOK_VAR)) {
        return var_declaration(parser, stmt);
    } else if (accept(parser, TOK_CONST)) {
        return const_declaration(parser, stmt);
    } else if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        return import(parser, stmt);
    } else if (accept(parser, TOK_RET)) {
        return return_statement(parser, stmt);
    } else if (accept(parser, TOK_BREAK)) {
        *stmt = ast_make_break();
        return NO_ERR;
    } else if (accept(parser, TOK_CONT)) {
        *stmt = ast_make_continue();
        return NO_ERR;
    } else if (accept(parser, TOK_ALIAS)) {
        return alias(parser, stmt);
    } else if (!check(parser, TOK_RCURLY) && !check(parser, TOK_SEMI)) {
        return assignment_or_call(parser, stmt);
    } else {
        PARSE_ERROR(parser, "Invalid statement");
        return ERR_PARSE;
    }
}

static int return_statement(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;
    struct ast *expr = NULL;
    if (!check(parser, TOK_SEMI)) {
        err = expression(parser, &expr);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid expression in return statement");
        }
    }
    PARSE_DEBUG(parser, "Parsed a return statement");
    *stmt = ast_make_return(expr);
    return status;
}

static int var_declaration(struct parser *parser, struct ast **decl)
{
    int err, status = NO_ERR;
    struct ast *type = NULL;
    struct ast *names = NULL;

    err = declaration_type(parser, &type); /* FIXME - check err? */
    if (err) {
        status = err;
    }
    err = declaration_names(parser, &names); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    *decl = ast_make_decl(DECL_VAR, type, names);

    return status;
}

static int const_declaration(struct parser *parser, struct ast **decl)
{
    int err, status = NO_ERR;
    struct ast *type = NULL;
    struct ast *names = NULL;

    err = declaration_type(parser, &type); /* FIXME - check err? */
    if (err) {
        status = err;
    }
    err = declaration_names(parser, &names); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    *decl = ast_make_decl(DECL_CONST, type, names);

    return status;
}

static int declaration_type(struct parser *parser, struct ast **ret)
{
    int err, status = NO_ERR;
    struct ast *type = NULL;

    if (accept(parser, TOK_LSQUARE)) {
        expect(parser, TOK_TYPE, status);
        struct ast *listtype = ast_make_ident(parser->buffer);
        expect(parser, TOK_RSQUARE, status);
        type = ast_make_list_type(listtype);
    } else if (accept(parser, TOK_LCURLY)) {
        expect(parser, TOK_TYPE, status);
        struct ast *keytype = ast_make_ident(parser->buffer);
        expect(parser, TOK_COMMA, status);
        expect(parser, TOK_TYPE, status);
        struct ast *valtype = ast_make_ident(parser->buffer);
        expect(parser, TOK_RCURLY, status);
        type = ast_make_map_type(keytype, valtype);
    } else if (check(parser, TOK_FUNC)) {
        err = functype(parser, &type);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid function type");
        }
    } else {
        expect(parser, TOK_TYPE, status);
        type = ast_make_ident(parser->buffer);
        /* parse types defined in other modules, e.g. std.file */
        if (accept(parser, TOK_DOT)) {
            struct ast *extern_type = ast_make_list(LIST_SELECTOR);
            extern_type = ast_list_append(extern_type, type);
            expect(parser, TOK_TYPE, status);
            type = ast_make_ident(parser->buffer);
            extern_type = ast_list_append(extern_type, type);
            type = extern_type;
        }
    }

    *ret = type;
    return status;
}

static int declaration_name(struct parser *parser, struct ast **name)
{
    int err, status = NO_ERR;

    expect(parser, TOK_IDENT, status);
    struct ast *ident = ast_make_ident(parser->buffer);

    if (accept(parser, TOK_ASS)) {
        struct ast *expr = NULL;
        err = expression(parser, &expr);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid initializer expression");
        }
        PARSE_DEBUG(parser, "Parsed initialization");
        *name = ast_make_initialization(ident, expr);
    } else {
        PARSE_DEBUG(parser, "Parsed declaration");
        *name = ident;
    }
    return status;
}

static int declaration_names(struct parser *parser, struct ast **names)
{
    int err, status = NO_ERR;
    struct ast *name = NULL;

    err = declaration_name(parser, &name); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct ast *list = ast_make_list(LIST_DECL);
        list = ast_list_append(list, name);
        do {
            err = declaration_name(parser, &name); /* FIXME - check err? */
            if (err) {
                status = err;
            }
            list = ast_list_append(list, name);
        } while (accept(parser, TOK_COMMA));

        *names = list;
    } else {
        *names = name;
    }

    return status;
}

static int assignment_or_call(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;
    /* This is the easiest way to parse an assignment...
     * Parse the left-hand side as an expression, then worry about
     * what the expression evaluates to later (when traversing AST).  */
    /* struct ast *lhs = expression(parser); */
    struct ast *lhs = NULL;
    err = term(parser, &lhs);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid left-hand-side");
    }

    if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
            parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
            parser->cur == TOK_IDIV || parser->cur == TOK_IMOD)
    {
        int ass = parser->cur; /* save assignment operator */
        next(parser);   /* eat assignment operator */
        struct ast *expr = NULL;
        err = expression(parser, &expr);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid right-hand-side in assignment");
        }
        struct ast *assignment = ast_make_assignment(lhs, ass, expr);
        PARSE_DEBUG(parser, "Parsed assignment");
        *stmt = assignment;
    } else if (parser->cur == TOK_DECL) {
        next(parser);
        struct ast *expr = NULL;
        err = expression(parser, &expr);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid expression in short-hand declaration");
        }
        struct ast *short_decl = ast_make_short_decl(lhs, expr);
        PARSE_DEBUG(parser, "Parsed short_decl");
        *stmt = short_decl;
    } else {
        assert(lhs);
        if (lhs->type == AST_CALL) {
            /* The only type of expression that can double as a statement is
            * a function call (disregarding the return value) */
            *stmt = lhs;
        } else {
            PARSE_ERROR(parser, "Invalid statement");
            return ERR_PARSE;
        }
    }
    return status;
}

static int unary_expr(struct parser *parser, struct ast **expr)
{
    int err = NO_ERR;

    if (parser->cur == TOK_NOT || parser->cur == TOK_SUB) {
        int unop = parser->cur;
        next(parser);
        struct ast *inner = NULL;
        err = unary_expr(parser, &inner); /* FIXME - check err? NO */
        *expr = ast_make_unexpr(unop, inner);
    } else {
        err = term(parser, expr); /* FIXME - check err? NO */
    }
    return err;
}

static int precedence(int op)
{
    switch (op) {
        case TOK_OR:
            return 1; break;
        case TOK_AND:
            return 2; break;
        case TOK_EQ: case TOK_NEQ: case TOK_LT:
        case TOK_LTE: case TOK_GT: case TOK_GTE:
            return 3; break;
        case TOK_ADD: case TOK_SUB:
            return 4; break;
        case TOK_MUL: case TOK_DIV: case TOK_MOD:
            return 5; break;
        case TOK_POW:
            return 6; break;
        default:
            return 0;
    }
}

struct shunter {
    struct ast **term_stk;
    int *op_stk;
    int term_top;
    int term_size;
    int op_top;
    int op_size;
};

static void shunter_init(struct shunter *shunter)
{
    shunter->op_top = 0;
    shunter->op_size = 8;
    shunter->op_stk = nalloc(shunter->op_size * sizeof(*shunter->op_stk));
    shunter->term_top = 0;
    shunter->term_size = 8;
    shunter->term_stk = nalloc(shunter->term_size * sizeof(*shunter->term_stk));

}

static struct ast* shunter_term_push(struct shunter *shunter, struct ast *term)
{
    shunter->term_stk[shunter->term_top++] = term;
    if (shunter->term_top >= shunter->term_size) {
        shunter->term_size *= 2;
        shunter->term_stk = nrealloc(shunter->term_stk,
                shunter->term_size * sizeof(*shunter->term_stk));
    }
    return term;
}

static struct ast *shunter_term_pop(struct shunter *shunter)
{
    return shunter->term_stk[--shunter->term_top];
}

static int shunter_op_push(struct shunter *shunter, int op)
{
    shunter->op_stk[shunter->op_top++] = op;
    if (shunter->op_top >= shunter->op_size) {
        shunter->op_size *= 2;
        shunter->op_stk = nrealloc(shunter->op_stk,
                shunter->op_size * sizeof(*shunter->op_stk));
    }
    return op;
}

static int shunter_op_pop(struct shunter *shunter)
{
    return shunter->op_stk[--shunter->op_top];
}

static int shunter_op_top(struct shunter *shunter)
{
    return shunter->op_stk[shunter->op_top - 1];
}

static int shunter_op_empty(struct shunter *shunter)
{
    return shunter->op_top == 0;
}

static int expression(struct parser *parser, struct ast **expr)
{
    int err, status = NO_ERR;
    struct ast *cur = NULL;
    err = unary_expr(parser, &cur); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    struct shunter shunter;
    shunter_init(&shunter);

    while (parser->cur == TOK_ADD || parser->cur == TOK_SUB ||
            parser->cur == TOK_MUL || parser->cur == TOK_DIV ||
            parser->cur == TOK_MOD || parser->cur == TOK_POW ||
            parser->cur == TOK_EQ || parser->cur == TOK_NEQ ||
            parser->cur == TOK_LT || parser->cur == TOK_LTE ||
            parser->cur == TOK_GT || parser->cur == TOK_GTE ||
            parser->cur == TOK_OR || parser->cur == TOK_AND) {

        /* push operand on stack */
        shunter_term_push(&shunter, cur);

        int op = parser->cur;
        next(parser);

        while (!shunter_op_empty(&shunter)) {
            int prec = precedence(op);
            int top = shunter_op_top(&shunter);
            int top_prec = precedence(top);
            if ((top != TOK_POW && top_prec >= prec) || top_prec > prec) {
                int thisop = shunter_op_pop(&shunter);
                /* pop RHS off first */
                struct ast *rhs = shunter_term_pop(&shunter);
                struct ast *lhs = shunter_term_pop(&shunter);
                struct ast *binexpr = ast_make_binexpr(lhs, thisop, rhs);
                shunter_term_push(&shunter, binexpr);
            } else {
                break;
            }
        }
        shunter_op_push(&shunter, op);

        err = unary_expr(parser, &cur); /* FIXME - check err? */
        if (err) {
            status = err;
        }
    }

    while (!shunter_op_empty(&shunter)) {
        int op = shunter_op_pop(&shunter);
        struct ast *lhs = shunter_term_pop(&shunter);
        cur = ast_make_binexpr(lhs, op, cur);
    }

    *expr = cur;
    return status;
}

static int term(struct parser *parser, struct ast **ret)
{
    int err, status = NO_ERR;

    struct ast *term = NULL;
    err = operand(parser, &term);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid operand");
    }
    /* FIXME:
     * if an error occurred above, I want this function
     * to return the error code at the end, but still
     * continue to parse the 'term' to check for more errors
     */

    while (true) {
        if (accept(parser, TOK_LSQUARE)) {
            struct ast *idx = NULL;
            err = expression(parser, &idx); /* FIXME - check err? */
            if (err) {
                status = err;
            }
            term = ast_make_contaccess(term, idx);
            expect(parser, TOK_RSQUARE, status);
        } else if (check(parser, TOK_LPAREN)) {
            struct ast *args = NULL;
            err = arguments(parser, &args); /* FIXME - check err? */
            if (err) {
                status = err;
            }
            term = ast_make_call(term, args);
            PARSE_DEBUG(parser, "Parsed function call");
        } else {
            break;
        }
    }

    /* at this point we can parse 'selectors' (member dereferences) */
    /* for now, let's build a list of selectors iteratively */
    if (check(parser, TOK_DOT)) {
        struct ast *selector = term;
        term = ast_make_list(LIST_SELECTOR);
        term = ast_list_append(term, selector);
    }

    while (accept(parser, TOK_DOT)) {
        /* selectors start only with an identifier,
         * e.g. parent.child[0]() */
        expect(parser, TOK_IDENT, status);
        struct ast *subterm = ast_make_ident(parser->buffer);

        while (true) {
            if (accept(parser, TOK_LSQUARE)) {
                struct ast *idx = NULL;
                err = expression(parser, &idx); /* FIXME - check err? */
                if (err) {
                    status = err;
                }
                subterm = ast_make_contaccess(subterm, idx);
                expect(parser, TOK_RSQUARE, status);
            } else if (check(parser, TOK_LPAREN)) {
                struct ast *args = NULL;
                err = arguments(parser, &args); /* FIXME - check err? */
                if (err) {
                    status = err;
                }
                subterm = ast_make_call(subterm, args);
                PARSE_DEBUG(parser, "Parsed function call");
            } else {
                break;
            }
        }
        term = ast_list_append(term, subterm);
    }

    *ret = term;
    return status;
}

static int operand(struct parser *parser, struct ast **ret)
{
    int status = NO_ERR;
    if (accept(parser, TOK_IDENT)) {
        *ret = ast_make_ident(parser->buffer);
    } else if (accept(parser, TOK_CHAR)) {
        char c = parser->buffer[0];
        *ret = ast_make_char_lit(c);
    } else if (accept(parser, TOK_INT)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        long l = strtol(parser->buffer, &endptr, 0);
        if (endptr != (parser->buffer + len)) {
            PARSE_ERRORF(parser, "Invalid integer %s", parser->buffer);
            status = ERR_PARSE;
        }
        *ret = ast_make_int_num(l);
    } else if (accept(parser, TOK_REAL)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        double d = strtod(parser->buffer, &endptr);
        if (endptr != (parser->buffer + len)) {
            PARSE_ERRORF(parser, "Invalid real number %s", parser->buffer);
            status = ERR_PARSE;
        }
        *ret = ast_make_real_num(d);
    } else if (accept(parser, TOK_STRING)) {
        *ret = ast_make_str_lit(parser->buffer);
    } else if (accept(parser, TOK_LPAREN)) {
        struct ast *expr = NULL;
        status = expression(parser, &expr);    /* FIXME - check status? */
        expect(parser, TOK_RPAREN, status);
        *ret = expr;
    } else if (check(parser, TOK_LSQUARE)) {
        status = list_literal(parser, ret);
    } else if (check(parser, TOK_LCURLY)) {
        status = map_literal(parser, ret);
    } else if (check(parser, TOK_FUNC)) {
        status = funclit(parser, ret);
    } else {
        status = ERR_PARSE;
    }
    return status;
}

static int list_literal(struct parser *parser, struct ast **lit)
{
    int err, status = NO_ERR;
    expect(parser, TOK_LSQUARE, status);
    struct ast* expr_list = ast_make_list(LIST_LITERAL);
    if (check(parser, TOK_RSQUARE)) {
        ; /* empty list literal */
    } else {
        do {
            struct ast* expr = NULL;
            err = expression(parser, &expr);
            if (err) {
                status = err;
                PARSE_ERROR(parser, "Invalid expression in list literal");
            }
            expr_list = ast_list_append(expr_list, expr);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RSQUARE, status);

    *lit = expr_list;
    return status;
}

static int map_literal(struct parser *parser, struct ast **lit)
{
    int err, status = NO_ERR;

    expect(parser, TOK_LCURLY, status);
    struct ast* keyval_list = ast_make_list(LIST_MAP_ITEM);
    if (check(parser, TOK_RCURLY)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* key = NULL;
            struct ast* val = NULL;
            err = expression(parser, &key);
            if (err) {
                status = err;
                PARSE_ERROR(parser, "Invalid key in map literal");
            }
            expect(parser, TOK_COLON, status);
            err = expression(parser, &val);
            if (err) {
                status = err;
                PARSE_ERROR(parser, "Invalid value in map literal");
            }
            struct ast *kv = ast_make_keyval(key, val);
            keyval_list = ast_list_append(keyval_list, kv);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RCURLY, status);

    *lit = keyval_list;
    return status;
}

static int arguments(struct parser *parser, struct ast **args)
{
    int err, status = NO_ERR;

    expect(parser, TOK_LPAREN, status);
    struct ast* arg_list = ast_make_list(LIST_ARG);
    if (check(parser, TOK_RPAREN)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* expr = NULL;
            err = expression(parser, &expr);/* FIXME - check err? */
            if (err) {
                status = err;
            }
            arg_list = ast_list_append(arg_list, expr);
        } while (accept(parser, TOK_COMMA));
    }
    expect(parser, TOK_RPAREN, status);

    *args = arg_list;
    return status;
}

/**
 * Accept either multiple statements between curly braces, or
 * a single statement
 */
static int block(struct parser *parser, struct ast **stmts)
{
    int status = NO_ERR;
    expect(parser, TOK_LCURLY, status);
    status = statements(parser, stmts);
    expect(parser, TOK_RCURLY, status);
    return status;
}

static int ifelse(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;
    struct ast *cond = NULL;
    struct ast *if_block = NULL;

    err = expression(parser, &cond);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid conditional expression in 'if' statement");
    }

    err = block(parser, &if_block); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    if (accept(parser, TOK_ELSE)) {
        struct ast *else_block = NULL;
        if (accept(parser, TOK_IF)) {
            err = ifelse(parser, &else_block); /* FIXME - check err? */
            if (err) {
                status = err;
            }
        } else {
            err = block(parser, &else_block); /* FIXME - check err? */
            if (err) {
                status = err;
            }
        }
        PARSE_DEBUG(parser, "Parsed `if+else` construct");
        *stmt = ast_make_ifelse(cond, if_block, else_block);
    } else {
        PARSE_DEBUG(parser, "Parsed `if` construct");
        *stmt = ast_make_ifelse(cond, if_block, NULL);
    }

    return status;
}

static int whileloop(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;

    struct ast *cond = NULL;
    struct ast *blk = NULL;
    err = expression(parser, &cond);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid conditional expression in 'while' statement");
    }

    err = block(parser, &blk); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    PARSE_DEBUG(parser, "Parsed `while` loop");

    *stmt = ast_make_while(cond, blk);
    return status;
}

static int forloop(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;

    expect(parser, TOK_IDENT, status);
    struct ast *var = ast_make_ident(parser->buffer);

    expect(parser, TOK_IN, status);
    struct ast *range = NULL;
    err = expression(parser, &range);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid range expression in 'for' statement");
    }

    struct ast *blk = NULL;
    err = block(parser, &blk); /* FIXME - check err? */
    if (err) {
        status = err;
    }

    PARSE_DEBUG(parser, "Parsed `for` loop");

    *stmt = ast_make_for(var, range, blk);
    return status;
}

static int parameters(struct parser *parser, struct ast **params)
{
    int err, status = NO_ERR;
    struct ast *param_list = ast_make_list(LIST_PARAM);
    /* 1 decl, or many comma-separated decls */
    do {
        struct ast *tp = NULL;
        struct ast *name = NULL;

        err = declaration_type(parser, &tp);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid parameter type");
        }

        err = declaration_name(parser, &name);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid parameter name");
        }

        struct ast *decl = ast_make_decl(DECL_VAR, tp, name);   /* FIXME - const? */
        param_list = ast_list_append(param_list, decl);
    } while (accept(parser, TOK_COMMA));

    *params = param_list;
    return status;
}

static int funclit(struct parser *parser, struct ast **lit)
{
    int err, status = NO_ERR;

    accept(parser, TOK_FUNC);

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        err = declaration_type(parser, &ret_type);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    expect(parser, TOK_LPAREN, status);

    struct ast *params = NULL;
    if (!check(parser, TOK_RPAREN)) {
        err = parameters(parser, &params); /* FIXME - check err? */
        if (err) {
            status = err;
        }
    }
    expect(parser, TOK_RPAREN, status);

    struct ast *blk = NULL;
    err = block(parser, &blk);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid function literal");
    }

    PARSE_DEBUG(parser, "Parsed function literal");

    *lit = ast_make_funclit(ret_type, params, blk);
    return status;
}

static int functype(struct parser *parser, struct ast **type)
{
    int err, status = NO_ERR;

    expect(parser, TOK_FUNC, status);

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        err = declaration_type(parser, &ret_type);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    expect(parser, TOK_LPAREN, status);

    struct ast *param_types = NULL;

    if (!check(parser, TOK_RPAREN)) {
        param_types = ast_make_list(LIST_TYPE);
        do {
            struct ast *tp = NULL;
            err = declaration_type(parser, &tp); /* FIXME - check err? */
            if (err) {
                status = err;
            }
            accept(parser, TOK_IDENT);   /* optional parameter name */
            param_types = ast_list_append(param_types, tp);
        } while (accept(parser, TOK_COMMA));
    }

    expect(parser, TOK_RPAREN, status);

    PARSE_DEBUG(parser, "Parsed function type");

    *type = ast_make_func_type(ret_type, param_types);
    return status;
}

static int alias(struct parser *parser, struct ast **alias)
{
    int err, status = NO_ERR;
    struct ast *type = NULL;
    err = declaration_type(parser, &type);
    if (err) {
        status = err;
        PARSE_ERROR(parser, "Invalid alias type");
    }
    expect(parser, TOK_IDENT, status);
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);
    PARSE_DEBUG(parser, "Parsed `alias`");

    *alias = ast_make_alias(type, name);
    return status;
}

static int import(struct parser *parser, struct ast **stmt)
{
    int status = NO_ERR;

    struct ast *list = ast_make_list(LIST_IMPORT);
    char *from = NULL;

    if (accept(parser, TOK_FROM)) {
        expect(parser, TOK_IDENT, status);
        from = strndup(parser->buffer, MAX_IDENT_LENGTH);
        expect(parser, TOK_IMPORT, status);
    } else {
        expect(parser, TOK_IMPORT, status);
    }

    do {
        expect(parser, TOK_IDENT, status);
        struct ast *module = ast_make_ident(parser->buffer);
        list = ast_list_append(list, module);
    } while (accept(parser, TOK_COMMA));

    PARSE_DEBUG(parser, "Parsed `import`");

    *stmt = ast_make_import(from, list);
    return status;
}

static int structtype(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;

    expect(parser, TOK_IDENT, status);
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    expect(parser, TOK_LCURLY, status);
    struct ast *members = ast_make_list(LIST_MEMBER);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *member = NULL;
        err = var_declaration(parser, &member);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid struct member");
        }
        expect(parser, TOK_SEMI, status);
        members = ast_list_append(members, member);
    }
    expect(parser, TOK_RCURLY, status);
    PARSE_DEBUG(parser, "Parsed `struct`");

    *stmt = ast_make_struct_type(name, members);
    return status;
}

static int interface(struct parser *parser, struct ast **stmt)
{
    int err, status = NO_ERR;

    expect(parser, TOK_IDENT, status);
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    expect(parser, TOK_LCURLY, status);
    struct ast *methods = ast_make_list(LIST_METHOD);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *ft = NULL;
        err = functype(parser, &ft);
        if (err) {
            status = err;
            PARSE_ERROR(parser, "Invalid interface method");
        }
        expect(parser, TOK_IDENT, status);
        struct ast *name = ast_make_ident(parser->buffer);
        struct ast *meth = ast_make_decl(DECL_VAR, ft, name);
        expect(parser, TOK_SEMI, status);
        methods = ast_list_append(methods, meth);
    }
    expect(parser, TOK_RCURLY, status);
    PARSE_DEBUG(parser, "Parsed `iface`");

    *stmt = ast_make_iface_type(name, methods);
    return status;
}
