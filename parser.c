#include "parser.h"

#define PARSE_DEBUG(P, S) \
    NOLLI_DEBUGF("(L %d, C %d): " S, \
            (P)->lexer->line, (P)->lexer->col)

#define PARSE_ERROR(P, S) \
    NOLLI_ERRORF("(L %d, C %d): " S, \
            (P)->lexer->line, (P)->lexer->col)

#define PARSE_ERRORF(P, fmt, ...) \
    NOLLI_ERRORF("(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)


/* static int parse(struct nolli_state *state); */
/* static struct ast* statements(struct parser *parser); */
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
static struct ast* interface(struct parser *parser);

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

    parser->buffer = parser->lexer->lastbuff;
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
        if (check(parser, TOK_EOF)) {
            return ERR_EOF;
        }
        return ERR_PARSE;
    }
    return NO_ERR;
}

int parse_line(struct nolli_state *state, char *line)
{
    struct parser *parser = state->parser;
    assert(parser);

    int ret = lexer_set(parser->lexer, line);
    if (ret != NO_ERR) {
        return ret;
    }
    next(parser);

    struct ast *stmt = statement(parser);
    if (stmt == NULL) {
        if (check(parser, TOK_EOF)) {
            return ERR_EOF;
        }
        printf("returning ERR_PARSE\n");
        return ERR_PARSE;
    }

    state->root = stmt;
    return NO_ERR;
}

/* static int parse(struct nolli_state *state) */
/* { */
/*     struct parser *parser = state->parser; */
/*     assert(parser); */

/*     /1* read first char *1/ */
/*     next(parser); */

/*     state->root = statements(parser); */

/*     return 1; /1* FIXME *1/ */
/* } */


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
/* static struct ast* statements(struct parser *parser) */
/* { */
/*     struct ast *statements = ast_make_list(LIST_STATEMENT); */
/*     struct ast *stmt = NULL; */

/*     while (true) { */
/*         /1* parse a single statement *1/ */
/*         stmt = statement(parser); */
/*         if (stmt == NULL) { */
/*             printf("ERROR!!!\n"); */
/*             /1* synchronize on statement terminator (semicolon) *1/ */
/*             while (parser->cur != TOK_SEMI && parser->cur != TOK_EOF) { */
/*                 printf("synchronizing\n"); */
/*                 next(parser); */
/*             } */
/*             if (parser->cur == TOK_EOF) { */
/*                 break; */
/*             } */
/*         } */

/*         if (stmt == NULL) { */
/*             /1* printf("NULL statement\n"); *1/ */
/*             break; */
/*         } */

/*         if (!expect(parser, TOK_SEMI)) { */
/*             return NULL; */
/*         } */

/*         /1* add statement to list *1/ */
/*         statements = ast_list_append(statements, stmt); */
/*         /1* reset the node to which stmt points *1/ */
/*         stmt = NULL; */
/*     } */

/*     return statements; */
/* } */

static struct ast* statement(struct parser *parser)
{
    /* if (check(parser, TOK_EOF)) { */
    /*     return NULL; */
    /* } else if (check(parser, TOK_RCURLY)) { */
    /*     return NULL; */
    /* } else */
    if (accept(parser, TOK_STRUCT)) {
        return structtype(parser);
    } else if (accept(parser, TOK_IFACE)) {
        return interface(parser);
    } else if (accept(parser, TOK_IF)) {
        return ifelse(parser);
    } else if (accept(parser, TOK_WHILE)) {
        return whileloop(parser);
    } else if (accept(parser, TOK_FOR)) {
        return forloop(parser);
    } else if (accept(parser, TOK_VAR)) {
        return var_declaration(parser);
    } else if (accept(parser, TOK_CONST)) {
        return const_declaration(parser);
    } else if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        return import(parser);
    } else if (accept(parser, TOK_RET)) {
        return return_statement(parser);
    } else if (accept(parser, TOK_BREAK)) {
        return ast_make_break();
    } else if (accept(parser, TOK_CONT)) {
        return ast_make_continue();
    } else if (accept(parser, TOK_ALIAS)) {
        return alias(parser);
    } else if (!check(parser, TOK_RCURLY) && !check(parser, TOK_SEMI)) {
        return assignment_or_call(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid statement (found token %s)", get_tok_name(parser->cur));
        return NULL;
    }
}

static struct ast* return_statement(struct parser *parser)
{
    struct ast *expr = NULL;
    if (!check(parser, TOK_SEMI)) {
        expr = expression(parser);
        if (expr == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid expression in return statement");
        }
    }

    PARSE_DEBUG(parser, "Parsed a return statement");

    return ast_make_return(expr);
}

static struct ast* var_declaration(struct parser *parser)
{
    struct ast *type = declaration_type(parser);
    if (type == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    struct ast *names = declaration_names(parser);
    if (names == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    return ast_make_decl(DECL_VAR, type, names);
}

static struct ast* const_declaration(struct parser *parser)
{
    struct ast *type = declaration_type(parser);
    if (type == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    struct ast *names = declaration_names(parser);
    if (names == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    return ast_make_decl(DECL_CONST, type, names);
}

static struct ast* declaration_type(struct parser *parser)
{
    struct ast *type = NULL;

    if (accept(parser, TOK_LSQUARE)) {
        if (!expect(parser, TOK_TYPE)) {
            return NULL;
        }
        struct ast *listtype = ast_make_ident(parser->buffer);
        if (!expect(parser, TOK_RSQUARE)) {
            return NULL;
        }
        type = ast_make_list_type(listtype);
    } else if (accept(parser, TOK_LCURLY)) {
        if (!expect(parser, TOK_TYPE)) {
            return NULL;
        }
        struct ast *keytype = ast_make_ident(parser->buffer);
        if (!expect(parser, TOK_COMMA)) {
            return NULL;
        }
        if (!expect(parser, TOK_TYPE)) {
            return NULL;
        }
        struct ast *valtype = ast_make_ident(parser->buffer);
        if (!expect(parser, TOK_RCURLY)) {
            return NULL;
        }
        type = ast_make_map_type(keytype, valtype);
    } else if (check(parser, TOK_FUNC)) {
        type = functype(parser);
        if (type == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid function type");
        }
    } else {
        if (!expect(parser, TOK_TYPE)) {
            return NULL;
        }
        type = ast_make_ident(parser->buffer);
        /* parse types defined in other modules, e.g. std.file */
        if (accept(parser, TOK_DOT)) {
            struct ast *extern_type = ast_make_list(LIST_SELECTOR);
            extern_type = ast_list_append(extern_type, type);
            if (!expect(parser, TOK_TYPE)) {
                return NULL;
            }
            type = ast_make_ident(parser->buffer);
            extern_type = ast_list_append(extern_type, type);
            type = extern_type;
        }
    }

    return type;
}

static struct ast* declaration_name(struct parser *parser)
{
    if (!expect(parser, TOK_IDENT)) {
        return NULL;
    }
    struct ast *ident = ast_make_ident(parser->buffer);

    struct ast* name = NULL;
    if (accept(parser, TOK_ASS)) {
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid initializer expression");
        }
        PARSE_DEBUG(parser, "Parsed initialization");
        name = ast_make_initialization(ident, expr);
    } else {
        PARSE_DEBUG(parser, "Parsed declaration");
        name = ident;
    }
    return name;
}

static struct ast* declaration_names(struct parser *parser)
{
    struct ast *name = declaration_name(parser);
    if (name == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    struct ast *names = NULL;
    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct ast *list = ast_make_list(LIST_DECL);
        list = ast_list_append(list, name);
        do {
            name = declaration_name(parser);
            if (name == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME: error message */
            }
            list = ast_list_append(list, name);
        } while (accept(parser, TOK_COMMA));

        names = list;
    } else {
        names = name;
    }

    return names;
}

static struct ast* assignment_or_call(struct parser *parser)
{
    /* This is the easiest way to parse an assignment...
     * Parse the left-hand side as an expression, then worry about
     * what the expression evaluates to later (when traversing AST).  */
    /* struct ast *lhs = expression(parser); */
    struct ast *lhs = term(parser);
    if (lhs == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
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
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid right-hand-side in assignment");
        }
        struct ast *assignment = ast_make_assignment(lhs, ass, expr);
        PARSE_DEBUG(parser, "Parsed assignment");
        stmt = assignment;
    } else if (parser->cur == TOK_DECL) {
        next(parser);
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid expression in short-hand declaration");
        }
        struct ast *short_decl = ast_make_short_decl(lhs, expr);
        PARSE_DEBUG(parser, "Parsed short_decl");
        stmt = short_decl;
    } else {
        if (lhs != NULL && lhs->type == AST_CALL) {
            /* The only type of expression that can double as a statement is
             * a function call (disregarding the return value) */
            stmt = lhs;
        } else {
            PARSE_ERROR(parser, "Invalid statement");
            return NULL;
        }
    }
    return stmt;
}

static struct ast* unary_expr(struct parser *parser)
{
    struct ast* expr = NULL;
    if (parser->cur == TOK_NOT || parser->cur == TOK_SUB) {
        int unop = parser->cur;
        next(parser);
        struct ast *inner = unary_expr(parser);
        if (inner == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            /* FIXME: error message? NO */
        }
        expr = ast_make_unexpr(unop, inner);
    } else {
        expr = term(parser);
        if (expr == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
        }
    }
    return expr;
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

static struct ast* expression(struct parser *parser)
{
    struct ast *cur = unary_expr(parser);
    if (cur == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
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
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            /* FIXME: error message */
        }
    }

    while (!shunter_op_empty(&shunter)) {
        int op = shunter_op_pop(&shunter);
        struct ast *lhs = shunter_term_pop(&shunter);
        cur = ast_make_binexpr(lhs, op, cur);
    }

    return cur;
}

static struct ast* term(struct parser *parser)
{
    struct ast *term = operand(parser);
    if (term == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        PARSE_ERROR(parser, "Invalid operand");
    }
    /* FIXME:
     * if an error occurred above, I want this function
     * to return the error code at the end, but still
     * continue to parse the 'term' to check for more errors
     */

    while (true) {
        if (accept(parser, TOK_LSQUARE)) {
            struct ast *idx = expression(parser);
            if (idx == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME: error message */
            }
            term = ast_make_contaccess(term, idx);
            if (!expect(parser, TOK_RSQUARE)) {
                return NULL;
            }
        } else if (check(parser, TOK_LPAREN)) {
            struct ast *args = arguments(parser);
            if (args == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME: error message */
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
        if (!expect(parser, TOK_IDENT)) {
            return NULL;
        }
        struct ast *subterm = ast_make_ident(parser->buffer);

        while (true) {
            if (accept(parser, TOK_LSQUARE)) {
                struct ast *idx = expression(parser);
                if (idx == NULL) {
                    if (check(parser, TOK_EOF)) {
                        return NULL;
                    }
                    /* FIXME: error message */
                }
                subterm = ast_make_contaccess(subterm, idx);
                if (!expect(parser, TOK_RSQUARE)) {
                    return NULL;
                }
            } else if (check(parser, TOK_LPAREN)) {
                struct ast *args = arguments(parser);
                if (args == NULL) {
                    if (check(parser, TOK_EOF)) {
                        return NULL;
                    }
                    /* FIXME: error message */
                }
                subterm = ast_make_call(subterm, args);
                PARSE_DEBUG(parser, "Parsed function call");
            } else {
                break;
            }
        }
        term = ast_list_append(term, subterm);
    }

    return term;
}

static struct ast* operand(struct parser *parser)
{
    if (accept(parser, TOK_IDENT)) {
        return ast_make_ident(parser->buffer);
    } else if (accept(parser, TOK_CHAR)) {
        char c = parser->buffer[0];
        return ast_make_char_lit(c);
    } else if (accept(parser, TOK_INT)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        long l = strtol(parser->buffer, &endptr, 0);
        if (endptr != (parser->buffer + len)) {
            PARSE_ERRORF(parser, "Invalid integer %s", parser->buffer);
            return NULL;
        }
        return ast_make_int_num(l);
    } else if (accept(parser, TOK_REAL)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        double d = strtod(parser->buffer, &endptr);
        if (endptr != (parser->buffer + len)) {
            PARSE_ERRORF(parser, "Invalid real number %s", parser->buffer);
            return NULL;
        }
        return ast_make_real_num(d);
    } else if (accept(parser, TOK_STRING)) {
        return ast_make_str_lit(parser->buffer);
    } else if (accept(parser, TOK_LPAREN)) {
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            /* FIXME: error message */
        }
        if (!expect(parser, TOK_RPAREN)) {
            return NULL;
        }
        return expr;
    } else if (check(parser, TOK_LSQUARE)) {
        return list_literal(parser);
    } else if (check(parser, TOK_LCURLY)) {
        return map_literal(parser);
    } else if (check(parser, TOK_FUNC)) {
        return funclit(parser);
    } else {
        return NULL;
    }
}

static struct ast* list_literal(struct parser *parser)
{
    if (!expect(parser, TOK_LSQUARE)) {
        return NULL;
    }
    struct ast* expr_list = ast_make_list(LIST_LITERAL);
    if (check(parser, TOK_RSQUARE)) {
        ; /* empty list literal */
    } else {
        do {
            struct ast* expr = expression(parser);
            if (expr == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                PARSE_ERROR(parser, "Invalid expression in list literal");
            }
            expr_list = ast_list_append(expr_list, expr);
        } while (accept(parser, TOK_COMMA));
    }
    if (!expect(parser, TOK_RSQUARE)) {
        return NULL;
    }

    return expr_list;
}

static struct ast* map_literal(struct parser *parser)
{
    if (!expect(parser, TOK_LCURLY)) {
        return NULL;
    }

    struct ast* keyval_list = ast_make_list(LIST_MAP_ITEM);
    if (check(parser, TOK_RCURLY)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* key = expression(parser);
            if (key == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                PARSE_ERROR(parser, "Invalid key in map literal");
            }
            if (!expect(parser, TOK_COLON)) {
                return NULL;
            }
            struct ast* val = expression(parser);
            if (val == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                PARSE_ERROR(parser, "Invalid value in map literal");
            }
            struct ast *kv = ast_make_keyval(key, val);
            keyval_list = ast_list_append(keyval_list, kv);
        } while (accept(parser, TOK_COMMA));
    }
    if (!expect(parser, TOK_RCURLY)) {
        return NULL;
    }

    return keyval_list;
}

static struct ast* arguments(struct parser *parser)
{
    if (!expect(parser, TOK_LPAREN)) {
        return NULL;
    }

    struct ast* arg_list = ast_make_list(LIST_ARG);
    if (check(parser, TOK_RPAREN)) {
        ; /* function call with no args */
    } else {
        do {
            struct ast* expr = expression(parser);
            if (expr == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME: error message */
            }
            arg_list = ast_list_append(arg_list, expr);
        } while (accept(parser, TOK_COMMA));
    }
    if (!expect(parser, TOK_RPAREN)) {
        return NULL;
    }

    return arg_list;
}

/**
 * Parse multiple statements between curly braces
 */
static struct ast* block(struct parser *parser)
{
    if (!expect(parser, TOK_LCURLY)) {
        return NULL;
    }

    struct ast *blk = statements(parser, TOK_RCURLY);
    expect(parser, TOK_RCURLY);

    return blk;
}

/* Parse a series of statements until the STOP token is encountered.
 * The caller is responsible for "eating" the STOP token. This
 * allows the caller to check that the STOP token is in fact the next
 * token, otherwise print an error.
 */
static struct ast* statements(struct parser *parser, int stop_tok)
{
    struct ast *statements = ast_make_list(LIST_STATEMENT);
    /* parse statements until we SEE the STOP token */
    do {
        struct ast *stmt = NULL;
        stmt = statement(parser);
        if (stmt == NULL) {
            while (!check(parser, TOK_SEMI) && !check(parser, TOK_EOF)) {
                printf("synchronizing\n");
                next(parser);
            }
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
        }
        if (!expect(parser, TOK_SEMI)) {
            return NULL;
        }
        statements = ast_list_append(statements, stmt);
    } while (!check(parser, stop_tok));

    /* The caller is now responsible for "eating" the STOP token */

    return statements;
}

static struct ast* ifelse(struct parser *parser)
{
    struct ast *cond = expression(parser);
    if (cond == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        PARSE_ERROR(parser, "Invalid conditional expression in 'if' statement");
    }

    struct ast *if_block = block(parser);
    if (if_block == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    struct ast* if_stmt = NULL;
    if (accept(parser, TOK_ELSE)) {
        struct ast *else_block = NULL;
        if (accept(parser, TOK_IF)) {
            else_block = ifelse(parser);
            if (else_block == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME - error message */
            }
        } else {
            else_block = block(parser);
            if (else_block == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME - error message */
            }
        }
        PARSE_DEBUG(parser, "Parsed `if+else` construct");
        if_stmt = ast_make_ifelse(cond, if_block, else_block);
    } else {
        PARSE_DEBUG(parser, "Parsed `if` construct");
        if_stmt = ast_make_ifelse(cond, if_block, NULL);
    }

    return if_stmt;
}

static struct ast* whileloop(struct parser *parser)
{
    struct ast *cond = expression(parser);
    if (cond == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        PARSE_ERROR(parser, "Invalid conditional expression in 'while' statement");
    }

    struct ast *blk = block(parser);
    if (blk == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME - error message */
    }

    PARSE_DEBUG(parser, "Parsed `while` loop");

    return ast_make_while(cond, blk);
}

static struct ast* forloop(struct parser *parser)
{
    if (!expect(parser, TOK_IDENT)) {
        return NULL;
    }
    struct ast *var = ast_make_ident(parser->buffer);

    if (!expect(parser, TOK_IN)) {
        return NULL;
    }
    struct ast *range = expression(parser);
    if (range == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        PARSE_ERROR(parser, "Invalid range expression in 'for' statement");
    }

    struct ast *blk = block(parser);
    if (blk == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        /* FIXME: error message */
    }

    PARSE_DEBUG(parser, "Parsed `for` loop");

    return ast_make_for(var, range, blk);
}

static struct ast* parameters(struct parser *parser)
{
    struct ast *param_list = ast_make_list(LIST_PARAM);
    /* 1 decl, or many comma-separated decls */
    do {

        struct ast *tp = declaration_type(parser);
        if (tp == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid parameter type");
        }

        struct ast *name = declaration_name(parser);
        if (name == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid parameter name");
        }

        struct ast *decl = ast_make_decl(DECL_VAR, tp, name);   /* FIXME - const? */
        param_list = ast_list_append(param_list, decl);
    } while (accept(parser, TOK_COMMA));

    return param_list;
}

static struct ast* funclit(struct parser *parser)
{
    accept(parser, TOK_FUNC);

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
        if (ret_type == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    if (!expect(parser, TOK_LPAREN)) {
        return NULL;
    }

    struct ast *params = NULL;
    if (!check(parser, TOK_RPAREN)) {
        params = parameters(parser);
        if (params == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            /* FIXME: error message */
        }
    }
    if (!expect(parser, TOK_RPAREN)) {
        return NULL;
    }

    struct ast *blk = block(parser);
    if (blk == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        PARSE_ERROR(parser, "Invalid function literal");
    }

    PARSE_DEBUG(parser, "Parsed function literal");

    return ast_make_funclit(ret_type, params, blk);
}

static struct ast* functype(struct parser *parser)
{
    if (!expect(parser, TOK_FUNC)) {
        return NULL;
    }

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
        if (ret_type == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    if (!expect(parser, TOK_LPAREN)) {
        return NULL;
    }

    struct ast *param_types = NULL;

    if (!check(parser, TOK_RPAREN)) {
        param_types = ast_make_list(LIST_TYPE);
        do {
            struct ast *tp = declaration_type(parser);
            if (tp == NULL) {
                if (check(parser, TOK_EOF)) {
                    return NULL;
                }
                /* FIXME: error message */
            }
            accept(parser, TOK_IDENT);   /* optional parameter name */
            param_types = ast_list_append(param_types, tp);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RPAREN)) {
        return NULL;
    }

    PARSE_DEBUG(parser, "Parsed function type");

    return ast_make_func_type(ret_type, param_types);
}

static struct ast* alias(struct parser *parser)
{
    struct ast *type = declaration_type(parser);
    if (type == NULL) {
        if (check(parser, TOK_EOF)) {
            return NULL;
        }
        PARSE_ERROR(parser, "Invalid alias type");
    }

    if (!expect(parser, TOK_IDENT)) {
        return NULL;
    }
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    PARSE_DEBUG(parser, "Parsed `alias`");

    return ast_make_alias(type, name);
}

static struct ast* import(struct parser *parser)
{
    struct ast *list = ast_make_list(LIST_IMPORT);
    char *from = NULL;

    if (accept(parser, TOK_FROM)) {
        if (!expect(parser, TOK_IDENT)) {
            return NULL;
        }
        from = strndup(parser->buffer, MAX_IDENT_LENGTH);
        if (!expect(parser, TOK_IMPORT)) {
            return NULL;
        }
    } else if (!expect(parser, TOK_IMPORT)) {
        return NULL;
    }

    do {
        if (!expect(parser, TOK_IDENT)) {
            return NULL;
        }
        struct ast *module = ast_make_ident(parser->buffer);
        list = ast_list_append(list, module);
    } while (accept(parser, TOK_COMMA));

    PARSE_DEBUG(parser, "Parsed `import`");

    return ast_make_import(from, list);
}

static struct ast* structtype(struct parser *parser)
{
    if (!expect(parser, TOK_IDENT)) {
        return NULL;
    }
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    if (!expect(parser, TOK_LCURLY)) {
        return NULL;
    }

    struct ast *members = ast_make_list(LIST_MEMBER);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *member = var_declaration(parser);
        if (member == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid struct member");
        }
        if (!expect(parser, TOK_SEMI)) {
            return NULL;
        }
        members = ast_list_append(members, member);
    }
    if (!expect(parser, TOK_RCURLY)) {
        return NULL;
    }

    PARSE_DEBUG(parser, "Parsed `struct`");

    return ast_make_struct_type(name, members);
}

static struct ast* interface(struct parser *parser)
{
    if (!expect(parser, TOK_IDENT)) {
        return NULL;
    }

    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    if (!expect(parser, TOK_LCURLY)) {
        return NULL;
    }

    struct ast *methods = ast_make_list(LIST_METHOD);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *ft = functype(parser);
        if (ft == NULL) {
            if (check(parser, TOK_EOF)) {
                return NULL;
            }
            PARSE_ERROR(parser, "Invalid interface method");
        }
        if (!expect(parser, TOK_IDENT)) {
            return NULL;
        }
        struct ast *name = ast_make_ident(parser->buffer);
        struct ast *meth = ast_make_decl(DECL_VAR, ft, name);
        if (!expect(parser, TOK_SEMI)) {
            return NULL;
        }
        methods = ast_list_append(methods, meth);
    }
    if (!expect(parser, TOK_RCURLY)) {
        return NULL;
    }

    PARSE_DEBUG(parser, "Parsed `iface`");

    return ast_make_iface_type(name, methods);
}
