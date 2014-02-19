#include "parser.h"

#define PARSE_DEBUG(P, S) \
        NOLLI_DEBUGF("(L %d, C %d): " S, \
                (P)->lexer->line, (P)->lexer->line)

#define PARSE_ERRORF(P, fmt, ...) \
    do { \
        NOLLI_ERRORF("(L %d, C %d): " fmt, \
                (P)->lexer->line, (P)->lexer->line, __VA_ARGS__); \
        parser->error = true; \
        longjmp(parser->jmp, 42); \
    } while (0)


static struct ast *statements(struct parser *parser);
static struct ast *statement(struct parser *parser);
static struct ast *assignment(struct parser *parser);
static struct ast *expression(struct parser *parser);
static struct ast *unary_expr(struct parser *parser);
static struct ast *term(struct parser *parser);
static struct ast *operand(struct parser *parser);
static struct ast *list_literal(struct parser *parser);
static struct ast *map_literal(struct parser *parser);
static struct ast *funclit(struct parser *parser);
static struct ast *parameters(struct parser *parser);
static struct ast *arguments(struct parser *parser);
static struct ast *block(struct parser *parser);
static struct ast *ifelse(struct parser *parser);
static struct ast *whileloop(struct parser *parser);
static struct ast *forloop(struct parser *parser);
static struct ast *var_declaration(struct parser *parser);
static struct ast *const_declaration(struct parser *parser);
static struct ast *declaration_type(struct parser *parser);
static struct ast *declaration_names(struct parser *parser);
static struct ast *functype(struct parser *parser);
static struct ast *alias(struct parser *parser);
static struct ast *import(struct parser *parser);
static struct ast *structtype(struct parser *parser);
static struct ast *interface(struct parser *parser);
static bool accept(struct parser *parser, int tok);
static bool expect(struct parser *parser, int tok);

#undef next
#define next(p)         ((p)->cur = gettok((p)->lexer))
#define check(p, t)     ((p)->cur == (t))

static bool accept(struct parser *parser, int tok)
{
    if (check(parser, tok)) {
        next(parser);
        return true;
    }
    return false;
}

static bool expect(struct parser *parser, int tok)
{
    if (accept(parser, tok)) {
        return true;
    }

    NOLLI_ERRORF("(L %d, C %d): Unexpected token: %s , expecting %s",
            parser->lexer->line, parser->lexer->line,
            get_tok_name(parser->cur), get_tok_name(tok));
    parser->error = true;
    return false;
}


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

    PARSE_DEBUG(parser, "Finished parsing");

    return root;
}

static struct ast *statements(struct parser *parser)
{
    jmp_buf orig_jmp;
    memcpy(&orig_jmp, &parser->jmp, sizeof(orig_jmp));

    struct ast *statements = ast_make_list(LIST_STATEMENT);
    struct ast *stmt = NULL;

retry:
    if (setjmp(parser->jmp) == 0) {
        stmt = statement(parser);
        while (stmt != NULL) {
            expect(parser, TOK_SEMI);
            statements = ast_list_append(statements, stmt);
            /* add statement to list */
            stmt = statement(parser);
        }
    } else {
        while (parser->cur != TOK_SEMI && parser->cur != TOK_EOF) {
            /* printf("synchronizing\n"); */
            next(parser);
        }
        next(parser);
        goto retry;
    }

    memcpy(&parser->jmp, &orig_jmp, sizeof(parser->jmp));

    return statements;
}

static struct ast *statement(struct parser *parser)
{
    if (accept(parser, TOK_EOF)) {
        return NULL;    /* sentinel */
    } else if (accept(parser, TOK_STRUCT)) {
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
    /* } else if (check(parser, TOK_IDENT)) { */
        /* return ident_statement(parser); */
    } else if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        return import(parser);
    } else if (accept(parser, TOK_RET)) {
        struct ast *expr = NULL;
        if (!check(parser, TOK_SEMI)) {
            expr = expression(parser);
        }
        PARSE_DEBUG(parser, "Parsed a return statement");
        return ast_make_return(expr);
    } else if (accept(parser, TOK_BREAK)) {
        return ast_make_break();
    } else if (accept(parser, TOK_CONT)) {
        return ast_make_continue();
    } else if (accept(parser, TOK_ALIAS)) {
        return alias(parser);
    } else if (!check(parser, TOK_RCURLY) && !check(parser, TOK_SEMI)) {
        return assignment(parser);
    } else {
        return NULL;
    }
}

static struct ast *var_declaration(struct parser *parser)
{
    struct ast *type = declaration_type(parser);
    struct ast *names = declaration_names(parser);

    return ast_make_decl(DECL_VAR, type, names);
}

static struct ast *const_declaration(struct parser *parser)
{
    struct ast *type = declaration_type(parser);
    struct ast *names = declaration_names(parser);

    return ast_make_decl(DECL_CONST, type, names);
}

static struct ast *declaration_type(struct parser *parser)
{
    struct ast *type = NULL;

    if (accept(parser, TOK_LSQUARE)) {
        expect(parser, TOK_TYPE);
        struct ast *listtype = ast_make_ident(parser->buffer);
        expect(parser, TOK_RSQUARE);
        type = ast_make_list_type(listtype);
    } else if (accept(parser, TOK_LCURLY)) {
        expect(parser, TOK_TYPE);
        struct ast *keytype = ast_make_ident(parser->buffer);
        expect(parser, TOK_COMMA);
        expect(parser, TOK_TYPE);
        struct ast *valtype = ast_make_ident(parser->buffer);
        expect(parser, TOK_RCURLY);
        type = ast_make_map_type(keytype, valtype);
    } else if (check(parser, TOK_FUNC)) {
        type = functype(parser);
    } else {
        expect(parser, TOK_TYPE);
        type = ast_make_ident(parser->buffer);
        /* parse types defined in other modules, e.g. std.file */
        if (accept(parser, TOK_DOT)) {
            struct ast *extern_type = ast_make_list(LIST_SELECTOR);
            extern_type = ast_list_append(extern_type, type);
            expect(parser, TOK_TYPE);
            type = ast_make_ident(parser->buffer);
            extern_type = ast_list_append(extern_type, type);
            type = extern_type;
        }
    }

    return type;
}

static struct ast *declaration_name(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    struct ast *ident = ast_make_ident(parser->buffer);
    if (accept(parser, TOK_ASS)) {
        struct ast *expr = expression(parser);
        PARSE_DEBUG(parser, "Parsed initialization");
        return ast_make_initialization(ident, expr);
    } else {
        PARSE_DEBUG(parser, "Parsed declaration");
        return ident;
    }
}

static struct ast *declaration_names(struct parser *parser)
{
    struct ast *name = declaration_name(parser);

    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct ast *list = ast_make_list(LIST_DECL);
        list = ast_list_append(list, name);
        do {
            list = ast_list_append(list, declaration_name(parser));
        } while (accept(parser, TOK_COMMA));
        return list;
    }

    return name;
}

static struct ast *assignment(struct parser *parser)
{
    /* This is the easiest way to parse an assignment...
     * Parse the left-hand side as an expression, then worry about
     * what the expression evaluates to later (when traversing AST).  */
    /* struct ast *lhs = expression(parser); */
    struct ast *lhs = term(parser);

    if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
            parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
            parser->cur == TOK_IDIV || parser->cur == TOK_IMOD ||
            parser->cur == TOK_IPOW || parser->cur == TOK_IXOR) {
        int ass = parser->cur; /* save assignment operator */
        next(parser);   /* eat assignment operator */
        struct ast *expr = expression(parser);
        struct ast *assignment = ast_make_assignment(lhs, ass, expr);
        PARSE_DEBUG(parser, "Parsed assignment");
        return assignment;
    } else if (parser->cur == TOK_DECL) {
        next(parser);
        struct ast *expr = expression(parser);
        struct ast *short_decl = ast_make_short_decl(lhs, expr);
        PARSE_DEBUG(parser, "Parsed short_decl");
        return short_decl;
    } else if (lhs && lhs->type == AST_CALL) {
        /* The only type of expression that can double as a statement is
        * a function call (disregarding the return value) */
        return lhs;
    } else {
        PARSE_ERRORF(parser, "Invalid token after expr, found %s",
                get_tok_name(parser->cur));
        next(parser);
        return NULL;
    }
}

static struct ast *unary_expr(struct parser *parser)
{
    if (parser->cur == TOK_NOT || parser->cur == TOK_SUB) {
        int unop = parser->cur;
        next(parser);
        struct ast *inner = unary_expr(parser);
        return ast_make_unexpr(unop, inner);
    } else {
        return term(parser);
    }
}

static int precedence(int op)
{
    switch (op) {
        case TOK_ADD: case TOK_SUB:
            return 1;
            break;
        case TOK_MUL: case TOK_DIV:
            return 2;
            break;
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

static struct ast *expression(struct parser *parser)
{
    printf("parsing new expression\n");
    struct ast *expr = unary_expr(parser);

    struct shunter shunter;
    shunter_init(&shunter);

    while (parser->cur == TOK_ADD || parser->cur == TOK_SUB ||
            parser->cur == TOK_MUL || parser->cur == TOK_DIV ||
            parser->cur == TOK_POW || parser->cur == TOK_XOR ||
            parser->cur == TOK_MOD ||
            parser->cur == TOK_EQ || parser->cur == TOK_NEQ ||
            parser->cur == TOK_LT || parser->cur == TOK_LTE ||
            parser->cur == TOK_GT || parser->cur == TOK_GTE) {

        /* push operand on stack */
        shunter_term_push(&shunter, expr);
        printf("pushing term\n");

        int op = parser->cur;
        next(parser);

        while (!shunter_op_empty(&shunter) && (precedence(shunter_op_top(&shunter)) >= precedence(op))) {
            printf("popping op, making binexpr, pushing it\n");
            int thisop = shunter_op_pop(&shunter);
            /* pop RHS off first */
            struct ast *rhs = shunter_term_pop(&shunter);
            struct ast *lhs = shunter_term_pop(&shunter);
            struct ast *expr = ast_make_binexpr(lhs, thisop, rhs);
            shunter_term_push(&shunter, expr);
        }
        shunter_op_push(&shunter, op);
        printf("pushing op\n");

        expr = unary_expr(parser);
    }

    while (!shunter_op_empty(&shunter)) {
        int op = shunter_op_pop(&shunter);
        struct ast *lhs = shunter_term_pop(&shunter);
        expr = ast_make_binexpr(lhs, op, expr);
    }

    return expr;
}

static struct ast *term(struct parser *parser)
{
    struct ast *term = operand(parser);

    while (true) {
        if (accept(parser, TOK_LSQUARE)) {
            struct ast *idx = expression(parser);
            term = ast_make_contaccess(term, idx);
            expect(parser, TOK_RSQUARE);
        } else if (check(parser, TOK_LPAREN)) {
            struct ast *args = arguments(parser);
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
        expect(parser, TOK_IDENT);
        struct ast *subterm = ast_make_ident(parser->buffer);

        while (true) {
            if (accept(parser, TOK_LSQUARE)) {
                struct ast *idx = expression(parser);
                subterm = ast_make_contaccess(subterm, idx);
                expect(parser, TOK_RSQUARE);
            } else if (check(parser, TOK_LPAREN)) {
                struct ast *args = arguments(parser);
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

static struct ast *operand(struct parser *parser)
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
        }
        return ast_make_int_num(l);
    } else if (accept(parser, TOK_REAL)) {
        char *endptr = NULL;
        size_t len = strlen(parser->buffer);
        double d = strtod(parser->buffer, &endptr);
        if (endptr != (parser->buffer + len)) {
            PARSE_ERRORF(parser, "Invalid real number %s", parser->buffer);
        }
        return ast_make_real_num(d);
    } else if (accept(parser, TOK_STRING)) {
        return ast_make_str_lit(parser->buffer);
    } else if (accept(parser, TOK_LPAREN)) {
        struct ast *expr = expression(parser);
        expect(parser, TOK_RPAREN);
        return expr;
    } else if (check(parser, TOK_LSQUARE)) {
        return list_literal(parser);
    } else if (check(parser, TOK_LCURLY)) {
        return map_literal(parser);
    } else if (check(parser, TOK_FUNC)) {
        return funclit(parser);
    } else {
        next(parser);
        PARSE_ERRORF(parser, "Invalid operand: %s", parser->buffer);
        return NULL;
    }
}

static struct ast *list_literal(struct parser *parser)
{
    expect(parser, TOK_LSQUARE);
    struct ast* expr_list = ast_make_list(LIST_LITERAL);
    if (check(parser, TOK_RSQUARE)) {
        ; /* empty list literal */
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
    struct ast* keyval_list = ast_make_list(LIST_MAP_ITEM);
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
    struct ast* arg_list = ast_make_list(LIST_ARG);
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

/**
 * Accept either multiple statements between curly braces, or
 * a single statement
 */
static struct ast *block(struct parser *parser)
{
    struct ast *blk = NULL;
    expect(parser, TOK_LCURLY);
    blk = statements(parser);
    expect(parser, TOK_RCURLY);
    return blk;
}

static struct ast *ifelse(struct parser *parser)
{
    struct ast *cond = expression(parser);
    struct ast *if_block = block(parser);

    if (accept(parser, TOK_ELSE)) {
        struct ast *else_block = NULL;
        if (accept(parser, TOK_IF)) {
            else_block = ifelse(parser);
        } else {
            else_block = block(parser);
        }
        PARSE_DEBUG(parser, "Parsed `if+else` construct");
        return ast_make_ifelse(cond, if_block, else_block);
    } else {
        PARSE_DEBUG(parser, "Parsed `if` construct");
        return ast_make_ifelse(cond, if_block, NULL);
    }
}

static struct ast *whileloop(struct parser *parser)
{
    struct ast *cond = expression(parser);
    struct ast *blk = block(parser);
    PARSE_DEBUG(parser, "Parsed `while` loop");
    return ast_make_while(cond, blk);
}

static struct ast *forloop(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    struct ast *var = ast_make_ident(parser->buffer);
    expect(parser, TOK_IN);
    struct ast *range = expression(parser);
    struct ast *blk = block(parser);
    PARSE_DEBUG(parser, "Parsed `for` loop");
    return ast_make_for(var, range, blk);
}

static struct ast *parameters(struct parser *parser)
{
    struct ast *params = ast_make_list(LIST_PARAM);
    /* 1 decl, or many comma-separated decls */
    do {
        struct ast *tp = declaration_type(parser);
        struct ast *name = declaration_name(parser);
        struct ast *decl = ast_make_decl(DECL_VAR, tp, name);   /* FIXME - const? */
        params = ast_list_append(params, decl);
    } while (accept(parser, TOK_COMMA));

    return params;
}

static struct ast *funclit(struct parser *parser)
{
    accept(parser, TOK_FUNC);

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
    }

    expect(parser, TOK_LPAREN);

    struct ast *params = NULL;
    if (!check(parser, TOK_RPAREN)) {
        params = parameters(parser);
    }
    expect(parser, TOK_RPAREN);

    struct ast *blk = block(parser);

    PARSE_DEBUG(parser, "Parsed function literal");

    return ast_make_funclit(ret_type, params, blk);
}

static struct ast *functype(struct parser *parser)
{
    expect(parser, TOK_FUNC);

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
    }

    expect(parser, TOK_LPAREN);

    struct ast *param_types = NULL;

    if (!check(parser, TOK_RPAREN)) {
        param_types = ast_make_list(LIST_TYPE);
        do {
            struct ast *tp = declaration_type(parser);
            accept(parser, TOK_IDENT);   /* optional parameter name */
            param_types = ast_list_append(param_types, tp);
        } while (accept(parser, TOK_COMMA));
    }

    expect(parser, TOK_RPAREN);

    PARSE_DEBUG(parser, "Parsed function type");
    return ast_make_func_type(ret_type, param_types);
}

static struct ast *alias(struct parser *parser)
{
    struct ast *type = declaration_type(parser);
    expect(parser, TOK_IDENT);
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);
    PARSE_DEBUG(parser, "Parsed `alias`");

    return ast_make_alias(type, name);
}

static struct ast *import(struct parser *parser)
{
    struct ast *list = ast_make_list(LIST_IMPORT);
    char *from = NULL;

    if (accept(parser, TOK_FROM)) {
        expect(parser, TOK_IDENT);
        from = strndup(parser->buffer, MAX_IDENT_LENGTH);
        expect(parser, TOK_IMPORT);
    } else {
        expect(parser, TOK_IMPORT);
    }

    do {
        expect(parser, TOK_IDENT);
        struct ast *module = ast_make_ident(parser->buffer);
        list = ast_list_append(list, module);
    } while (accept(parser, TOK_COMMA));

    PARSE_DEBUG(parser, "Parsed `import`");

    struct ast *imp = ast_make_import(from, list);
    return imp;
}

static struct ast *structtype(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    expect(parser, TOK_LCURLY);
    struct ast *members = ast_make_list(LIST_MEMBER);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *member = var_declaration(parser);
        expect(parser, TOK_SEMI);
        members = ast_list_append(members, member);
    }
    expect(parser, TOK_RCURLY);
    PARSE_DEBUG(parser, "Parsed `struct`");

    return ast_make_struct_type(name, members);
}

static struct ast *interface(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    char *name = strndup(parser->buffer, MAX_IDENT_LENGTH);

    expect(parser, TOK_LCURLY);
    struct ast *methods = ast_make_list(LIST_METHOD);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *ft = functype(parser);
        expect(parser, TOK_IDENT);
        struct ast *name = ast_make_ident(parser->buffer);
        struct ast *meth = ast_make_decl(DECL_VAR, ft, name);
        expect(parser, TOK_SEMI);
        methods = ast_list_append(methods, meth);
    }
    expect(parser, TOK_RCURLY);
    PARSE_DEBUG(parser, "Parsed `iface`");

    return ast_make_iface_type(name, methods);
}
