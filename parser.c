#include "parser.h"

#define PARSE_DEBUGF(P, fmt, ...) \
    NOLLI_DEBUGF("(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)

#define PARSE_DEBUG(P, S) PARSE_DEBUGF(P, "%s", S)

#define PARSE_ERRORF(P, fmt, ...) \
    NOLLI_ERRORF("(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)

#define PARSE_ERROR(P, S) PARSE_ERRORF(P, "%s", S)

static struct ast* statements(struct parser *parser, int stop_tok);
static struct ast* statement(struct parser *parser);
static struct ast* assignment_or_call(struct parser *parser);
static struct ast* expression(struct parser *parser);
static struct ast* unary_expr(struct parser *parser);
static struct ast* term(struct parser *parser);
static struct ast* operand(struct parser *parser);
static struct ast* list_literal(struct parser *parser);
static struct ast* map_literal(struct parser *parser);
static struct ast* funclit(struct parser *parser);
static struct ast* parameters(struct parser *parser);
static struct ast* arguments(struct parser *parser);
static struct ast* block(struct parser *parser);
static struct ast* return_statement(struct parser *parser);
static struct ast* ifelse(struct parser *parser);
static struct ast* whileloop(struct parser *parser);
static struct ast* forloop(struct parser *parser);
static struct ast* var_declaration(struct parser *parser);
static struct ast* const_declaration(struct parser *parser);
static struct ast* declaration_type(struct parser *parser);
static struct ast* declaration_names(struct parser *parser);
static struct ast* functype(struct parser *parser);
static struct ast* alias(struct parser *parser);
static struct ast* import(struct parser *parser);
static struct ast* structtype(struct parser *parser);
static struct ast* structlit(struct parser *parser);
static struct ast* interface(struct parser *parser);

static struct ast *ident(struct parser *parser);
static struct ast *int_literal(struct parser *parser);
static struct ast *real_literal(struct parser *parser);

#undef next
#define next(P)         ((P)->cur = gettok((P)->lexer))
#define check(P, T)     ((P)->cur == (T))

#define accept(P, T) (check(P, T) ? next(P), true : false)

#define expect(P, T) (accept(P, T) ? true : ( \
            PARSE_ERRORF(P, "Unexpected token: %s, expecting %s", \
                get_tok_name((P)->cur), get_tok_name(T)), \
            next(P), false))


void parser_init(struct parser *parser)
{
    assert(parser);

    parser->lexer = nalloc(sizeof(*parser->lexer));
    lexer_init(parser->lexer);

    parser->bufptr = &parser->lexer->lastbuff;
}

int parse_buffer(struct nolli_state *state, char *buffer)
{
    struct parser *parser = state->parser;
    assert(parser);

    int ret = lexer_set(parser->lexer, buffer);
    if (ret != NO_ERR) {
        return ret;
    }
    next(parser);

    state->root = statements(parser, TOK_EOF);
    expect(parser, TOK_EOF);

    if (state->root == NULL) {
        return ERR_PARSE;
    }
    return NO_ERR;
}

static struct ast* statement(struct parser *parser)
{
    struct ast *stmt = NULL;
    if (accept(parser, TOK_STRUCT)) {
        stmt = structtype(parser);
    } else if (accept(parser, TOK_IFACE)) {
        stmt = interface(parser);
    } else if (accept(parser, TOK_IF)) {
        stmt = ifelse(parser);
    } else if (accept(parser, TOK_WHILE)) {
        stmt = whileloop(parser);
    } else if (accept(parser, TOK_FOR)) {
        stmt = forloop(parser);
    } else if (accept(parser, TOK_VAR)) {
        stmt = var_declaration(parser);
    } else if (accept(parser, TOK_CONST)) {
        stmt = const_declaration(parser);
    } else if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        stmt = import(parser);
    } else if (accept(parser, TOK_RET)) {
        stmt = return_statement(parser);
    } else if (accept(parser, TOK_BREAK)) {
        stmt = ast_make_break();
    } else if (accept(parser, TOK_CONT)) {
        stmt = ast_make_continue();
    } else if (accept(parser, TOK_ALIAS)) {
        stmt = alias(parser);
    } else if (!check(parser, TOK_RCURLY) && !check(parser, TOK_SEMI)) {
        stmt = assignment_or_call(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid statement (found token %s)", get_tok_name(parser->cur));
        stmt = NULL;
    }
    return stmt;
}

static struct ast* return_statement(struct parser *parser)
{
    bool err = false;

    struct ast *expr = NULL;
    if (!check(parser, TOK_SEMI)) {
        expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid expression in return statement");
        }
    }

    PARSE_DEBUG(parser, "Parsed a return statement");

    struct ast *ret = NULL;
    if (err) {
        ret = NULL;     /* TODO: destroy expr */
    } else {
        ret = ast_make_return(expr);
    }
    return ret;
}

static struct ast* var_declaration(struct parser *parser)
{
    bool err = false;

    struct ast *type = declaration_type(parser);
    if (type == NULL) {
        err = true;
        /* FIXME: error message */
    }

    struct ast *names = declaration_names(parser);
    if (names == NULL) {
        err = true;
        /* FIXME: error message */
    }

    struct ast *decl = NULL;
    if (err) {
        decl = NULL;    /* TODO: destroy type & names */
    } else {
        decl = ast_make_decl(DECL_VAR, type, names);
    }
    return decl;
}

static struct ast* const_declaration(struct parser *parser)
{
    bool err = false;

    struct ast *type = declaration_type(parser);
    if (type == NULL) {
        err = true;
        /* FIXME: error message? */
    }

    struct ast *names = declaration_names(parser);
    if (names == NULL) {
        err = true;
        /* FIXME: error message? */
    }

    struct ast *decl = NULL;
    if (err) {
        decl = NULL;    /* TODO: destroy type & names */
    } else {
        decl = ast_make_decl(DECL_CONST, type, names);
    }
    return decl;
}

static struct ast* declaration_type(struct parser *parser)
{
    bool err = false;

    struct ast *type = NULL;

    if (accept(parser, TOK_LSQUARE)) {
        if (!expect(parser, TOK_TYPE)) {
            err = true;
        }
        struct ast *listtype = ast_make_ident(*parser->bufptr);
        if (!expect(parser, TOK_RSQUARE)) {
            err = true;
        }
        type = ast_make_list_type(listtype);
    } else if (accept(parser, TOK_LCURLY)) {
        if (!expect(parser, TOK_TYPE)) {
            err = true;
        }
        struct ast *keytype = ast_make_ident(*parser->bufptr);
        if (!expect(parser, TOK_COMMA)) {
            err = true;
        }
        if (!expect(parser, TOK_TYPE)) {
            err = true;
        }
        struct ast *valtype = ast_make_ident(*parser->bufptr);
        if (!expect(parser, TOK_RCURLY)) {
            err = true;
        }
        type = ast_make_map_type(keytype, valtype);
    } else if (check(parser, TOK_FUNC)) {
        type = functype(parser);
        if (type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function type");
        }
    } else {
        if (!expect(parser, TOK_TYPE)) {
            err = true;
        }
        type = ast_make_ident(*parser->bufptr);
        /* parse types defined in other modules, e.g. std.file */
        if (accept(parser, TOK_DOT)) {
            struct ast *extern_type = ast_make_list(LIST_SELECTOR);
            extern_type = ast_list_append(extern_type, type);
            if (!expect(parser, TOK_TYPE)) {
                err = true;
            }
            type = ast_make_ident(*parser->bufptr);
            extern_type = ast_list_append(extern_type, type);
            type = extern_type;
        }
    }

    if (err) {
        type = NULL;    /* TODO: destroy type */
    }
    return type;
}

static struct ast* declaration_name(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_IDENT)) {
        err = true;
    }
    struct ast *ident = ast_make_ident(*parser->bufptr);

    struct ast* name = NULL;
    if (accept(parser, TOK_ASS)) {
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid initializer expression");
        }
        PARSE_DEBUG(parser, "Parsed initialization");
        name = ast_make_initialization(ident, expr);
    } else {
        PARSE_DEBUG(parser, "Parsed declaration");
        name = ident;
    }

    if (err) {
        name = NULL;    /* TODO: destroy name */
    }
    return name;
}

static struct ast* declaration_names(struct parser *parser)
{
    bool err = false;

    struct ast *name = declaration_name(parser);
    if (name == NULL) {
        err = true;
        /* FIXME: error message? */
    }

    struct ast *names = NULL;
    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct ast *list = ast_make_list(LIST_DECL);
        list = ast_list_append(list, name);
        do {
            name = declaration_name(parser);
            if (name == NULL) {
                err = true;
                /* FIXME: error message */
            }
            list = ast_list_append(list, name);
        } while (accept(parser, TOK_COMMA));

        names = list;
    } else {
        names = name;
    }

    if (err) {
        names = NULL;   /* TODO: destroy names */
    }
    return names;
}

static struct ast* assignment_or_call(struct parser *parser)
{
    bool err = false;
    /* This is the easiest way to parse an assignment...
     * Parse the left-hand side as an expression, then worry about
     * what the expression evaluates to later (when traversing AST).  */
    /* struct ast *lhs = expression(parser); */
    struct ast *lhs = term(parser);
    if (lhs == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid left-hand-side");
    }

    struct ast* stmt = NULL;
    if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
            parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
            parser->cur == TOK_IDIV || parser->cur == TOK_IMOD)
    {
        int ass = parser->cur; /* save assignment operator */
        next(parser);   /* eat assignment operator */
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid right-hand-side in assignment");
        }
        struct ast *assignment = ast_make_assignment(lhs, ass, expr);
        PARSE_DEBUG(parser, "Parsed assignment");
        stmt = assignment;
    } else if (parser->cur == TOK_DECL) {
        next(parser);
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid expression in short-hand declaration");
        }
        struct ast *short_decl = ast_make_short_decl(lhs, expr);
        PARSE_DEBUG(parser, "Parsed short_decl");
        stmt = short_decl;
    } else {
        stmt = lhs;
        /* if (lhs != NULL && lhs->type == AST_CALL) { */
        /*     /1* The only type of expression that can double as a statement is */
        /*      * a function call (disregarding the return value) *1/ */
        /*     stmt = lhs; */
        /* } else { */
        /*     err = true; */
        /*     PARSE_ERROR(parser, "Invalid statement"); */
        /* } */
    }

    if (err) {
        stmt = NULL;    /* TODO: destroy stmt */
    }
    return stmt;
}

static struct ast* unary_expr(struct parser *parser)
{
    bool err = false;

    struct ast* expr = NULL;
    if (parser->cur == TOK_NOT || parser->cur == TOK_SUB) {
        int unop = parser->cur;
        next(parser);
        struct ast *inner = unary_expr(parser);
        if (inner == NULL) {
            err = true;
            /* FIXME: error message? NO */
        }
        expr = ast_make_unexpr(unop, inner);
    } else {
        expr = term(parser);
        if (expr == NULL) {
            err = true;
            /* FIXME: error message? NO */
        }
    }

    if (err) {
        expr = NULL;    /* TODO: destroy expr */
    }
    return expr;
}

static int precedence(int op)
{
    int prec = 0;
    switch (op) {
        case TOK_OR:
            prec = 1; break;
        case TOK_AND:
            prec = 2; break;
        case TOK_EQ: case TOK_NEQ: case TOK_LT:
        case TOK_LTE: case TOK_GT: case TOK_GTE:
            prec = 3; break;
        case TOK_ADD: case TOK_SUB:
            prec = 4; break;
        case TOK_MUL: case TOK_DIV: case TOK_MOD:
            prec = 5; break;
        case TOK_POW:
            prec = 6; break;
        default:
            prec = 0;
    }
    return prec;
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

static struct ast* expression(struct parser *parser)
{
    bool err = false;

    struct ast *cur = unary_expr(parser);
    if (cur == NULL) {
        err = true;
        /* FIXME: error message? */
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
            /* remember '^' or TOK_POW has right-associativity */
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

        cur = unary_expr(parser);
        if (cur == NULL) {
            err = true;
            /* FIXME: error message? */
        }
    }

    while (!shunter_op_empty(&shunter)) {
        int op = shunter_op_pop(&shunter);
        struct ast *lhs = shunter_term_pop(&shunter);
        cur = ast_make_binexpr(lhs, op, cur);
    }

    if (err) {
        cur = NULL;     /* TODO: destroy cur */
    }
    return cur;
}

static struct ast* term(struct parser *parser)
{
    bool err = false;

    struct ast *term = operand(parser);
    if (term == NULL) {
        err = true;
        /* PARSE_ERROR(parser, "Invalid operand"); */
    }

    /* FIXME: dangerous loop? */
    while (true) {
        if (accept(parser, TOK_LSQUARE)) {
            struct ast *idx = expression(parser);
            if (idx == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid expression index");
            }
            term = ast_make_contaccess(term, idx);
            if (!expect(parser, TOK_RSQUARE)) {
                err = true;
            }
        } else if (check(parser, TOK_LPAREN)) {
            struct ast *args = arguments(parser);
            if (args == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid func arguments");
            }
            term = ast_make_call(term, args);
            PARSE_DEBUG(parser, "Parsed function call");
        } else {
            break;
        }
    }

    /* at this point we can parse 'selectors' (member dereferences) */
    /* for now, let's build a list of selectors iteratively... */
    if (check(parser, TOK_DOT)) {
        struct ast *selector = term;
        term = ast_make_list(LIST_SELECTOR);
        term = ast_list_append(term, selector);
    }

    /* FIXME:
     * since I'm building a list of 'selectors' iteratively, I'm fully
     * aware that this next loop is a duplicate of the loop above. */
    while (accept(parser, TOK_DOT)) {
        /* selectors start only with an identifier, e.g. parent.child[0]() */
        if (!expect(parser, TOK_IDENT)) {
            err = true;
        }
        struct ast *subterm = ast_make_ident(*parser->bufptr);

        /* FIXME: dangerous loop? */
        while (true) {
            if (accept(parser, TOK_LSQUARE)) {
                struct ast *idx = expression(parser);
                if (idx == NULL) {
                    err = true;
                    PARSE_ERROR(parser, "Invalid expression index");
                }
                subterm = ast_make_contaccess(subterm, idx);
                if (!expect(parser, TOK_RSQUARE)) {
                    err = true;
                }
            } else if (check(parser, TOK_LPAREN)) {
                struct ast *args = arguments(parser);
                if (args == NULL) {
                    err = true;
                    PARSE_ERROR(parser, "Invalid func arguments");
                }
                subterm = ast_make_call(subterm, args);
                PARSE_DEBUG(parser, "Parsed function call");
            } else {
                break;
            }
        }
        term = ast_list_append(term, subterm);
    }

    if (err) {
        term = NULL;    /* TODO: destroy term */
    }
    return term;
}

static struct ast *ident(struct parser *parser)
{
    struct ast *id = NULL;

    if (!expect(parser, TOK_IDENT)) {
        PARSE_ERROR(parser, "Invalid identifier");
    } else {
        char *ident = strndup(*parser->bufptr, MAX_IDENT_LENGTH);
        if (ident == NULL) {
            PARSE_ERROR(parser, "strndup failure");
            id = NULL;
        } else {
            PARSE_DEBUGF(parser, "Parsed identifier: %s", ident);
            id = ast_make_ident(ident);
        }
    }
    return id;
}

static struct ast *int_literal(struct parser *parser)
{
    if (!expect(parser, TOK_INT)) {
        return NULL;
    }

    struct ast *lit = NULL;

    char *endptr = NULL;
    size_t len = strlen(*parser->bufptr);
    long l = strtol(*parser->bufptr, &endptr, 0);
    PARSE_DEBUGF(parser, "Parsed int literal: %s", *parser->bufptr);
    if (endptr != (*parser->bufptr + len)) {
        PARSE_ERRORF(parser, "Invalid integer %s", *parser->bufptr);
        lit = NULL;
    } else {
        lit = ast_make_int_num(l);
    }
    return lit;
}

static struct ast *real_literal(struct parser *parser)
{
    if (!expect(parser, TOK_REAL)) {
        return NULL;
    }

    struct ast *lit = NULL;

    char *endptr = NULL;
    size_t len = strlen(*parser->bufptr);
    double d = strtod(*parser->bufptr, &endptr);
    PARSE_DEBUGF(parser, "Parsed real literal: %s", *parser->bufptr);
    if (endptr != (*parser->bufptr + len)) {
        PARSE_ERRORF(parser, "Invalid real number %s", *parser->bufptr);
        lit = NULL;
    } else {
        lit = ast_make_real_num(d);
    }
    return lit;
}

static struct ast* operand(struct parser *parser)
{
    struct ast *op = NULL;
    if (check(parser, TOK_IDENT)) {
        op = ident(parser);
    } else if (accept(parser, TOK_CHAR)) {
        PARSE_DEBUGF(parser, "Parsed char literal: %s", *parser->bufptr);
        char c = *parser->bufptr[0];
        op = ast_make_char_lit(c);
    } else if (check(parser, TOK_INT)) {
        op = int_literal(parser);
    } else if (check(parser, TOK_REAL)) {
        op = real_literal(parser);
    } else if (accept(parser, TOK_STRING)) {
        PARSE_DEBUGF(parser, "Parsed string literal: %s", *parser->bufptr);
        op = ast_make_str_lit(*parser->bufptr);
    } else if (accept(parser, TOK_LPAREN)) {
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            /* FIXME: error message? */
        }
        if (!expect(parser, TOK_RPAREN)) {
            expr = NULL;
        }
        op = expr;
    } else if (check(parser, TOK_LSQUARE)) {
        op = list_literal(parser);
    } else if (check(parser, TOK_LCURLY)) {
        op = map_literal(parser);
    } else if (check(parser, TOK_FUNC)) {
        op = funclit(parser);
    } else if (check(parser, TOK_AMP)) {
        op = structlit(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid operand: %s", *parser->bufptr);
        op = NULL;
    }

    return op;
}

static struct ast* list_literal(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LSQUARE)) {
        err = true;
    }

    struct ast* expr_list = ast_make_list(LIST_LITERAL);
    if (!check(parser, TOK_RSQUARE)) {
        do {
            struct ast* expr = expression(parser);
            if (expr == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid expression in list literal");
            }
            expr_list = ast_list_append(expr_list, expr);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RSQUARE)) {
        err = true;
    }

    if (err) {
        expr_list = NULL;   /* TODO: destroy expr_list */
    }
    return expr_list;
}

static struct ast* map_literal(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast* keyval_list = ast_make_list(LIST_MAP_ITEM);
    if (!check(parser, TOK_RCURLY)) {
        do {
            struct ast* key = expression(parser);
            if (key == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid key in map literal");
            }

            if (!expect(parser, TOK_COLON)) {
                err = true;
            }

            struct ast* val = expression(parser);
            if (val == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid value in map literal");
            }

            struct ast *kv = ast_make_keyval(key, val);
            keyval_list = ast_list_append(keyval_list, kv);
        } while (accept(parser, TOK_COMMA));
    }
    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    if (err) {
        keyval_list = NULL;     /* TODO: destroy keyval_list */
    }
    return keyval_list;
}

static struct ast* arguments(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct ast* arg_list = ast_make_list(LIST_ARG);
    if (!check(parser, TOK_RPAREN)) {
        do {
            struct ast* expr = expression(parser);
            if (expr == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid argument expression");
            }
            arg_list = ast_list_append(arg_list, expr);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RPAREN)) {
        err = true;
    }

    if (err) {
        arg_list = NULL;    /* TODO: destroy arg_list */
    }
    return arg_list;
}

/**
 * Parse multiple statements between curly braces
 */
static struct ast* block(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    /* don't bother checking whether 'blk' is NULL since all callers
     * of this function will check it anyway */
    struct ast *blk = statements(parser, TOK_RCURLY);

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    if (err) {
        blk = NULL;     /* TODO: destroy blk */
    }
    return blk;
}

/* Parse a series of statements until the STOP token is encountered.
 * The caller is responsible for "eating" the STOP token. This
 * allows the caller to check that the STOP token is in fact the next
 * token, otherwise print an error.
 */
static struct ast* statements(struct parser *parser, int stop_tok)
{
    bool err = false;
    struct ast *statements = ast_make_list(LIST_STATEMENT);
    /* parse statements until we SEE the STOP token */
    do {
        struct ast *stmt = NULL;
        stmt = statement(parser);
        if (stmt == NULL) {
            err = true;
            while (!check(parser, TOK_SEMI) && !check(parser, TOK_EOF)) {
                printf("synchronizing\n");
                next(parser);
            }
            if (check(parser, TOK_EOF)) {
                break;
            }
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        statements = ast_list_append(statements, stmt);
    } while (!check(parser, stop_tok));

    /* The caller is now responsible for "eating" the STOP token */

    if (err) {
        /* TODO: destroy statements */
        statements = NULL;
    }
    return statements;
}

static struct ast* ifelse(struct parser *parser)
{
    bool err = false;

    struct ast *cond = expression(parser);
    if (cond == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid conditional expression in 'if' statement");
    }

    struct ast *if_block = block(parser);
    if (if_block == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid block in 'if' statement");
    }

    struct ast *else_block = NULL;
    if (accept(parser, TOK_ELSE)) {
        /* determine whether to parse another "if+else" statement or just
         * a block of statements */
        struct ast* (*else_parser) (struct parser *) = NULL;
        if (accept(parser, TOK_IF)) {
            else_parser = ifelse;
        } else {
            else_parser = block;
        }

        else_block = else_parser(parser);
        if (else_block == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid 'else' block in 'if' statement");
        }
        PARSE_DEBUG(parser, "Parsed `if+else` construct");
    } else {
        else_block = NULL;
        PARSE_DEBUG(parser, "Parsed `if` construct");
    }

    struct ast *if_stmt = NULL;
    if (err) {
        if_stmt = NULL;     /* TODO: destroy cond & if_block & else_block */
    } else {
        if_stmt = ast_make_ifelse(cond, if_block, else_block);
    }

    return if_stmt;
}

static struct ast* whileloop(struct parser *parser)
{
    bool err = false;

    struct ast *cond = expression(parser);
    if (cond == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid conditional expression in 'while' statement");
    }

    struct ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid statement block in while-statement");
    }

    PARSE_DEBUG(parser, "Parsed `while` loop");

    struct ast *loop = NULL;
    if (err) {
        loop = NULL;    /* TODO: destroy cond & blk */
    } else {
        loop = ast_make_while(cond, blk);
    }
    return loop;
}

static struct ast* forloop(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_IDENT)) {
        err = true;
    }
    struct ast *var = ast_make_ident(*parser->bufptr);

    if (!expect(parser, TOK_IN)) {
        err = true;
    }
    struct ast *range = expression(parser);
    if (range == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid range expression in 'for' statement");
    }

    struct ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid statement block in for-statement");
    }

    PARSE_DEBUG(parser, "Parsed `for` loop");

    struct ast *loop = NULL;
    if (err) {
        loop = NULL;    /* TODO: destroy var & range & blk */
    } else {
        loop = ast_make_for(var, range, blk);
    }
    return loop;
}

/* A parameter list is either a single declaration or
 * many comma-separated declarations */
static struct ast* parameters(struct parser *parser)
{
    bool err = false;

    struct ast *param_list = ast_make_list(LIST_PARAM);
    do {

        struct ast *tp = declaration_type(parser);
        if (tp == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid parameter type");
            break;
        }

        struct ast *name = declaration_name(parser);
        if (name == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid parameter name");
            break;
        }

        struct ast *decl = ast_make_decl(DECL_VAR, tp, name);   /* FIXME - const? */
        param_list = ast_list_append(param_list, decl);
    } while (accept(parser, TOK_COMMA));

    if (err) {
        param_list = NULL;  /* TODO: destroy param_list */
    }
    return param_list;
}

static struct ast* funclit(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_FUNC)) {
        err = true;     /* FIXME: just immediately return NULL? */
    }

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
        if (ret_type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct ast *params = NULL;
    if (!check(parser, TOK_RPAREN)) {
        params = parameters(parser);
        if (params == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid parameters in func literal");
        }
    }
    if (!expect(parser, TOK_RPAREN)) {
        err = true;
    }

    struct ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid function literal");
    }

    PARSE_DEBUG(parser, "Parsed function literal");

    struct ast *fnlit = NULL;
    if (err) {
        fnlit = NULL;   /* TODO: destroy ret_type & params & blk */
    } else {
        fnlit = ast_make_funclit(ret_type, params, blk);
    }
    return fnlit;
}

static struct ast* functype(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_FUNC)) {
        err = true;
    }

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
        if (ret_type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct ast *param_types = NULL;

    if (!check(parser, TOK_RPAREN)) {
        param_types = ast_make_list(LIST_TYPE);
        do {
            struct ast *tp = declaration_type(parser);
            if (tp == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid parameter in func type");
                break;
            }
            accept(parser, TOK_IDENT);   /* optional parameter name */
            param_types = ast_list_append(param_types, tp);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RPAREN)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed function type");

    struct ast *ft = NULL;
    if (err) {
        ft = NULL;  /* TODO: destroy ret_type & param_types */
    } else {
        ft = ast_make_func_type(ret_type, param_types);
    }
    return ft;
}

static struct ast* alias(struct parser *parser)
{
    bool err = false;

    struct ast *type = declaration_type(parser);
    if (type == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid alias type");
    }

    struct ast *name = ident(parser);

    PARSE_DEBUG(parser, "Parsed `alias`");

    struct ast *ali = NULL;
    if (err) {
        ali = NULL;     /* TODO: destroy type & name */
    } else {
        ali = ast_make_alias(type, name);
    }
    return ali;
}

static struct ast* import(struct parser *parser)
{
    bool err = false;
    struct ast *from = NULL;

    if (accept(parser, TOK_FROM)) {
        from = ident(parser);
        if (!expect(parser, TOK_IMPORT)) {
            err = true;
        }
    } else if (!expect(parser, TOK_IMPORT)) {
        err = true;
    }

    struct ast *list = ast_make_list(LIST_IMPORT);
    do {
        struct ast *module = ident(parser);
        list = ast_list_append(list, module);
    } while (accept(parser, TOK_COMMA));

    PARSE_DEBUG(parser, "Parsed `import`");

    struct ast *imp = NULL;
    if (err) {
        imp = NULL;     /* TODO: destroy from & list */
    } else {
        imp = ast_make_import(from, list);
    }
    return imp;
}

static struct ast* structlit(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_AMP)) {
        err = true;
    }

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast* init_list = ast_make_list(LIST_STRUCT_INIT);
    if (!check(parser, TOK_RCURLY)) {
        do {
            /*  struct initializers can be either:
                    ident : expression
                    expression
                since they are ordered by their declaration */

            struct ast* item = NULL;

            if (check(parser, TOK_IDENT)) {
                item = ident(parser);
                if (accept(parser, TOK_COLON)) {
                    /* forget about the struct member name */
                    item = expression(parser);
                }
            } else {
                item = expression(parser);
            }

            if (item == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid item in struct initializer");
            }

            init_list = ast_list_append(init_list, item);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed struct initializer");

    struct ast *strctlit = NULL;
    if (err) {
        init_list = NULL;     /* TODO: destroy keyval_list */
    } else {
        strctlit = ast_make_structlit(name, init_list);
    }
    return strctlit;
}

static struct ast* structtype(struct parser *parser)
{
    bool err = false;

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast *members = ast_make_list(LIST_MEMBER);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *member = var_declaration(parser);
        if (member == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid struct member");
            break;
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        members = ast_list_append(members, member);
    }
    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed `struct`");

    struct ast *strct = NULL;
    if (err) {
        strct = NULL;   /* TODO: destroy name & members */
    } else {
        strct = ast_make_struct_type(name, members);
    }

    return strct;
}

static struct ast* interface(struct parser *parser)
{
    bool err = false;

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast *methods = ast_make_list(LIST_METHOD);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *ft = functype(parser);
        if (ft == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid interface method");
            break;
        }
        if (!expect(parser, TOK_IDENT)) {
            err = true;
            break;
        }
        struct ast *name = ast_make_ident(*parser->bufptr);
        struct ast *meth = ast_make_decl(DECL_VAR, ft, name);
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        methods = ast_list_append(methods, meth);
    }
    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed `iface`");

    struct ast *iface = NULL;
    if (err) {
        iface = NULL; /* TODO: destroy name & methods */
    } else {
        iface = ast_make_iface_type(name, methods);
    }
    return iface;
}
