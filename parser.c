#include "parser.h"

static struct ast *statements(struct parser *parser);
static struct ast *statement(struct parser *parser);
static struct ast *assignment(struct parser *parser);
static struct ast *expression(struct parser *parser);
static struct ast *unary_expr(struct parser *parser);
static struct ast *term(struct parser *parser);
static struct ast *operand(struct parser *parser);
static struct ast *list_literal(struct parser *parser);
static struct ast *map_literal(struct parser *parser);
static struct ast *func_literal(struct parser *parser);
static struct ast *parameters(struct parser *parser);
static struct ast *arguments(struct parser *parser);
static struct ast *body(struct parser *parser);
static struct ast *ifelse(struct parser *parser);
static struct ast *whileloop(struct parser *parser);
static struct ast *forloop(struct parser *parser);
static struct ast *var_declaration(struct parser *parser);
static struct ast *const_declaration(struct parser *parser);
static struct ast *declaration_type(struct parser *parser);
static struct ast *declaration_names(struct parser *parser);
static struct ast *functype(struct parser *parser);
static struct ast *funcdecl(struct parser *parser);
static struct ast *funcdef(struct parser *parser);
static struct ast *typedefinition(struct parser *parser);
static struct ast *import(struct parser *parser);
static struct ast *structtype(struct parser *parser);
static struct ast *interface(struct parser *parser);
static void parse_error(struct parser *parser, char *msg, ...);
static void parse_debug(struct parser *parser, char *msg, ...);
static int accept(struct parser *parser, int tok);
static int expect(struct parser *parser, int tok);

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

/* int x; int y = 2 + 2; x = 1; x(y) */
static struct ast *statement(struct parser *parser)
{
    if (accept(parser, TOK_EOF)) {
        return NULL;    /* sentinel */
    } else if (accept(parser, TOK_VAR)) {
        return var_declaration(parser);
    } else if (accept(parser, TOK_CONST)) {
        return const_declaration(parser);
    /* } else if (check(parser, TOK_IDENT)) { */
        /* return ident_statement(parser); */
    } else if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        return import(parser);
    } else if (check(parser, TOK_STRUCT)) {
        return structtype(parser);
    } else if (check(parser, TOK_IFACE)) {
        return interface(parser);
    } else if (check(parser, TOK_IF)) {
        return ifelse(parser);
    } else if (check(parser, TOK_WHILE)) {
        return whileloop(parser);
    } else if (check(parser, TOK_FOR)) {
        return forloop(parser);
    } else if (check(parser, TOK_FUNC)) {
        return funcdef(parser); /* FIXME */
    } else if (accept(parser, TOK_RET)) {
        /* TODO: empty return statements? */
        struct ast *expr = expression(parser);
        parse_debug(parser, "Parsed a return statement");
        return ast_make_return(expr);
    } else if (accept(parser, TOK_BREAK)) {
        return ast_make_break();
    } else if (accept(parser, TOK_CONT)) {
        return ast_make_continue();
    } else if (accept(parser, TOK_TYPEDEF)) {
        return typedefinition(parser);
    } else if (!check(parser, TOK_RCURLY)) {
        return assignment(parser);
    }

    /* should be a '}', ending a series of statements */
    return NULL;
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
        struct ast *typename = ast_make_ident(parser->buffer);
        type = ast_make_type(typename);
    }

    return type;
}

static struct ast *declaration_name(struct parser *parser)
{
    expect(parser, TOK_IDENT);
    struct ast *ident = ast_make_ident(parser->buffer);
    if (accept(parser, TOK_ASS)) {
        struct ast *expr = expression(parser);
        parse_debug(parser, "Parsed initialization");
        return ast_make_initialization(ident, expr);
    } else {
        parse_debug(parser, "Parsed declaration");
        return ident;
    }
}

static struct ast *declaration_names(struct parser *parser)
{
    struct ast *name = declaration_name(parser);

    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct ast *list = ast_make_list(LIST_DECLS);
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
            parser->cur == TOK_IPOW) {
        int ass = parser->cur; /* save assignment operator */
        next(parser);   /* eat assignment operator */
        struct ast *expr = expression(parser);
        struct ast *assignment = ast_make_assignment(lhs, ass, expr);
        parse_debug(parser, "Parsed assignment");
        return assignment;
    } else if (lhs->type == AST_CALL) {
        /* The only type of expression that can double as a statement is
        * a function call (disregarding the return value) */
        return lhs;
    } else {
        parse_error(parser, "Invalid token after expr, found %s",
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

/*
 * TODO: Use Shunting Yard algorithm to parse respecting operator precedence
 */
static struct ast *expression(struct parser *parser)
{
    struct ast *lhs = unary_expr(parser);

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
    struct ast *term = operand(parser);

    while (true) {
        if (accept(parser, TOK_LSQUARE)) {
            struct ast *idx = expression(parser);
            term = ast_make_contaccess(term, idx);
            expect(parser, TOK_RSQUARE);
        } else if (check(parser, TOK_LPAREN)) {
            struct ast *args = arguments(parser);
            term = ast_make_call(term, args);
            parse_debug(parser, "Parsed function call");
        } else {
            break;
        }
    }

    /* at this point we can parse 'selectors' (member dereferences) */
    /* for now, let's build a list of selectors iteratively */
    if (check(parser, TOK_DOT)) {
        struct ast *selector = term;
        term = ast_make_list(LIST_SELECTORS);
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
                parse_debug(parser, "Parsed function call");
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
    } else if (accept(parser, TOK_LPAREN)) {
        struct ast *expr = expression(parser);
        expect(parser, TOK_RPAREN);
        return expr;
    } else if (check(parser, TOK_LSQUARE)) {
        return list_literal(parser);
    } else if (check(parser, TOK_LCURLY)) {
        return map_literal(parser);
    } else if (check(parser, TOK_FUNC)) {
        return func_literal(parser);
    } else {
        parse_error(parser, "Invalid literal: %s", parser->buffer);
        next(parser);
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

/**
 * Accept either multiple statements between curly braces, or
 * a single statement
 */
static struct ast *body(struct parser *parser)
{
    struct ast *bod = NULL;
    if (accept(parser, TOK_LCURLY)) {
        bod = statements(parser);
        expect(parser, TOK_RCURLY);
    } else {
        bod = statement(parser);
    }
    return bod;
}

static struct ast *ifelse(struct parser *parser)
{
    accept(parser, TOK_IF);
    struct ast *cond = expression(parser);
    struct ast *if_body = body(parser);

    if (accept(parser, TOK_ELSE)) {
        struct ast *else_body = body(parser);
        parse_debug(parser, "Parsed `if+else` construct");
        return ast_make_ifelse(cond, if_body, else_body);
    } else {
        parse_debug(parser, "Parsed `if` construct");
        return ast_make_ifelse(cond, if_body, NULL);
    }
}

static struct ast *whileloop(struct parser *parser)
{
    accept(parser, TOK_WHILE);
    struct ast *cond = expression(parser);
    struct ast *bod = body(parser);
    parse_debug(parser, "Parsed `while` loop");
    return ast_make_while(cond, bod);
}

static struct ast *forloop(struct parser *parser)
{
    accept(parser, TOK_FOR);
    expect(parser, TOK_IDENT);
    struct ast *var = ast_make_ident(parser->buffer);
    expect(parser, TOK_IN);
    struct ast *range = expression(parser);
    struct ast *bod = body(parser);
    parse_debug(parser, "Parsed `for` loop");
    return ast_make_for(var, range, bod);
}

static struct ast *parameters(struct parser *parser)
{
    struct ast *params = ast_make_list(LIST_PARAMS);
    /* 1 decl, or many comma-separated decls */
    do {
        struct ast *tp = declaration_type(parser);
        struct ast *name = declaration_name(parser);
        struct ast *decl = ast_make_decl(DECL_VAR, tp, name);   /* FIXME - const? */
        params = ast_list_append(params, decl);
    } while (accept(parser, TOK_COMMA));

    return params;
}

static struct ast *func_literal(struct parser *parser)
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

    expect(parser, TOK_LCURLY);
    struct ast *body = statements(parser);
    expect(parser, TOK_RCURLY);

    parse_debug(parser, "Parsed function literal");

    return ast_make_funclit(ret_type, params, body);
}

static struct ast *funcdef(struct parser *parser)
{
    accept(parser, TOK_FUNC);

    /* we don't know if the function has a return type :( so try to parse
     * one, then check it */
    struct ast *ret_type = declaration_type(parser);

    struct ast *name = NULL;
    /* parse function name if the last token was the return type */
    if (accept(parser, TOK_IDENT)) {
        name = ast_make_ident(parser->buffer);
    } else if (ret_type->type == AST_IDENT) {
        name = ret_type;
        ret_type = NULL;
    } else {
        parse_error(parser, "Invalid function name");
    }

    expect(parser, TOK_LPAREN);

    struct ast *params = NULL;
    if (!check(parser, TOK_RPAREN)) {
        params = parameters(parser);
    }
    expect(parser, TOK_RPAREN);

    expect(parser, TOK_LCURLY);
    struct ast *body = statements(parser);
    expect(parser, TOK_RCURLY);

    parse_debug(parser, "Parsed function definition");

    return ast_make_funcdef(ret_type, name, params, body);
}

static struct ast *functype(struct parser *parser)
{
    expect(parser, TOK_FUNC);
    /* we don't know if this is a return type or the function name :( */
    /* expect(parser, TOK_TYPE); */
    /* function name if the last token was the return type */
    /* accept(parser, TOK_IDENT); */
    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = declaration_type(parser);
    }

    expect(parser, TOK_LPAREN);

    struct ast *param_types = NULL;
    if (!check(parser, TOK_RPAREN)) {
        param_types = ast_make_list(LIST_TYPES);
        do {
            struct ast *tp = declaration_type(parser);
            accept(parser, TOK_TYPE);   /* optional parameter name */
            param_types = ast_list_append(param_types, tp);
        } while (accept(parser, TOK_COMMA));
    }

    expect(parser, TOK_RPAREN);

    parse_debug(parser, "Parsed function type");
    return ast_make_func_type(ret_type, param_types);
}

static struct ast *funcdecl(struct parser *parser)
{
    expect(parser, TOK_FUNC);

    /* we don't know if the function has a return type :( so try to parse
     * one, then check it */
    struct ast *ret_type = declaration_type(parser);

    struct ast *name = NULL;
    /* parse function name if the last token was the return type */
    if (accept(parser, TOK_IDENT)) {
        name = ast_make_ident(parser->buffer);
    } else if (ret_type->type == AST_IDENT) {
        name = ret_type;
        ret_type = NULL;
    } else {
        parse_error(parser, "Invalid function name");
    }

    expect(parser, TOK_LPAREN);

    struct ast *param_types = NULL;
    if (!check(parser, TOK_RPAREN)) {
        param_types = ast_make_list(LIST_TYPES);
        do {
            struct ast *tp = declaration_type(parser);
            accept(parser, TOK_TYPE);   /* optional parameter name */
            param_types = ast_list_append(param_types, tp);
        } while (accept(parser, TOK_COMMA));
    }

    expect(parser, TOK_RPAREN);

    parse_debug(parser, "Parsed function declaration");

    struct ast *functype = ast_make_func_type(ret_type, param_types);
    return ast_make_decl(DECL_VAR, functype, name);
}

static struct ast *typedefinition(struct parser *parser)
{
    struct ast *type = NULL;
    if (check(parser, TOK_FUNC)) {
        funcdecl(parser); /* FIXME */
    } else {
        expect(parser, TOK_TYPE);
        type = ast_make_ident(parser->buffer);
    }
    expect(parser, TOK_IDENT);
    struct ast *alias = ast_make_ident(parser->buffer);
    parse_debug(parser, "Parsed `typedef`");

    return ast_make_typedef(type, alias);
}

static struct ast *import(struct parser *parser)
{
    struct ast *list = ast_make_list(LIST_IMPORTS);
    struct ast *from = NULL;

    if (accept(parser, TOK_FROM)) {
        expect(parser, TOK_IDENT);
        from = ast_make_ident(parser->buffer);
        expect(parser, TOK_IMPORT);
    } else {
        expect(parser, TOK_IMPORT);
    }

    do {
        expect(parser, TOK_IDENT);
        struct ast *module = ast_make_ident(parser->buffer);
        list = ast_list_append(list, module);
    } while (accept(parser, TOK_COMMA));

    parse_debug(parser, "Parsed `import`");

    struct ast *imp = ast_make_import(from, list);
    return imp;
}

static struct ast *structtype(struct parser *parser)
{
    accept(parser, TOK_STRUCT);
    expect(parser, TOK_IDENT);
    struct ast *name = ast_make_ident(parser->buffer);

    expect(parser, TOK_LCURLY);
    struct ast *members = ast_make_list(LIST_MEMBERS);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *member = NULL;
        if (check(parser, TOK_FUNC)) {
            member = funcdecl(parser);
        } else {
            member = var_declaration(parser);
        }
        members = ast_list_append(members, member);
    }
    expect(parser, TOK_RCURLY);
    parse_debug(parser, "Parsed `struct`");

    return ast_make_struct_type(name, members);
}

static struct ast *interface(struct parser *parser)
{
    accept(parser, TOK_IFACE);
    expect(parser, TOK_IDENT);
    struct ast *name = ast_make_ident(parser->buffer);

    expect(parser, TOK_LCURLY);
    struct ast *methods = ast_make_list(LIST_METHODS);
    while (!check(parser, TOK_RCURLY)) {
        struct ast *fd = funcdecl(parser);
        methods = ast_list_append(methods, fd);
    }
    expect(parser, TOK_RCURLY);
    parse_debug(parser, "Parsed `iface`");

    return ast_make_iface_type(name, methods);
}

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
