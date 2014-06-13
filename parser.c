#include "parser.h"

#define PARSE_DEBUGF(P, fmt, ...) \
    NOLLI_DEBUGF("(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)

#define PARSE_DEBUG(P, S) PARSE_DEBUGF(P, "%s", S)

#define PARSE_ERRORF(P, fmt, ...) \
    NOLLI_ERRORF("(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)

#define PARSE_ERROR(P, S) PARSE_ERRORF(P, "%s", S)

static struct ast* program(struct parser *parser);
static struct ast* globals(struct parser *parser);
static struct ast* global(struct parser *parser);
static struct ast* ident(struct parser *parser);
static struct ast* import(struct parser *parser);
static struct ast* declaration(struct parser *parser);
static struct ast* declrhs(struct parser *parser);
static struct ast* data(struct parser *parser);
static struct ast* data_member(struct parser *parser);
static struct ast* methods(struct parser *parser);
static struct ast* interface(struct parser *parser);
static struct ast* methdecl(struct parser *parser);
static struct ast* funcdef(struct parser *parser);
static struct ast* statement(struct parser *parser);
static struct ast* alias(struct parser *parser);
static struct ast* whileloop(struct parser *parser);
static struct ast* forloop(struct parser *parser);
static struct ast* ifelse(struct parser *parser);
static struct ast* return_statement(struct parser *parser);
static struct ast* ident_statement(struct parser *parser);
static struct ast* expression(struct parser *parser);
static struct ast* unary_expr(struct parser *parser);
static struct ast* term(struct parser *parser);
static struct ast* operand(struct parser *parser);
static struct ast* intlit(struct parser *parser);
static struct ast* reallit(struct parser *parser);
static struct ast* listlit(struct parser *parser);
static struct ast* maplit(struct parser *parser);
static struct ast* funclit(struct parser *parser);
static struct ast* datalit(struct parser *parser);
static struct ast* parameters(struct parser *parser);
static struct ast* arguments(struct parser *parser);
static struct ast* block(struct parser *parser);
static struct ast* type(struct parser *parser);
static struct ast* functype(struct parser *parser);


#undef next
#define next(P)         ((P)->cur = gettok((P)->lexer))
#define check(P, T)     ((P)->cur == (T))

#define accept(P, T) (check(P, T) ? next(P), true : false)

#define expect(P, T) (accept(P, T) ? true : ( \
            PARSE_ERRORF(P, "Unexpected token: %s, expecting %s", \
                get_tok_name((P)->cur), get_tok_name(T)), \
            next(P), false))

struct ast *parse_buffer(char *buffer)
{
    struct parser *parser = nalloc(sizeof(*parser));
    parser->strtab = nalloc(sizeof(*parser->strtab));
    parser->lexer = nalloc(sizeof(*parser->lexer));
    lexer_init(parser->lexer, buffer);

    /* DEBUGGING: lexer_scan_all(parser->lexer); */

    next(parser);

    struct ast *root = program(parser);
    expect(parser, TOK_EOF);

    /* DEBUG: dump all symbols/strings */
    /* stringtable_dump(parser->strtab, stdout); */

    return root;
}

static char *current_buffer(struct parser *parser)
{
    return parser->lexer->lastbuff;
}

static int lineno(struct parser *parser)
{
    return parser->lexer->line;
}

static struct ast* program(struct parser *parser)
{
    bool err = false;

    /* parse package declaration */
    if (!expect(parser, TOK_PACKAGE)) {
        err = true;
    }
    struct ast *pkg = ident(parser);
    if (!expect(parser, TOK_SEMI)) {
        err = true;
    }

    struct ast *defs = globals(parser);

    return ast_make_program(pkg, defs, lineno(parser));
}

static struct ast* globals(struct parser *parser)
{
    bool err = false;

    struct ast *defs = ast_make_list(AST_LIST_GLOBALS, lineno(parser));
    /* parse statements until we see a '}' token */
    while (!check(parser, TOK_EOF)) {
        struct ast *def = global(parser);
        if (def == NULL) {
            err = true;
            break;
        }

        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        defs = ast_list_append(defs, def);
    }

    if (err) {
        /* TODO: destroy defs */
        defs = NULL;
    }
    return defs;
}

static struct ast* global(struct parser *parser)
{
    struct ast* def = NULL;
    if (check(parser, TOK_IMPORT) || check(parser, TOK_FROM)) {
        def = import(parser);
    } else if (check(parser, TOK_VAR) || check(parser, TOK_CONST)) {
        def = declaration(parser);
    } else if (check(parser, TOK_DATA)) {
        def = data(parser);
    } else if (check(parser, TOK_METHODS)) {
        def = methods(parser);
    } else if (check(parser, TOK_INTERFACE)) {
        def = interface(parser);
    } else if (check(parser, TOK_FUNC)) {
        def = funcdef(parser);
    } else if (check(parser, TOK_ALIAS)) {
        def = alias(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid global definition (found token %s)",
                get_tok_name(parser->cur));
        def = NULL;
    }
    return def;
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

    struct ast *list = ast_make_list(AST_LIST_IMPORTS, lineno(parser));
    do {
        struct ast *module = ident(parser);
        list = ast_list_append(list, module);
    } while (accept(parser, TOK_COMMA));

    PARSE_DEBUG(parser, "Parsed `import`");

    struct ast *imp = NULL;
    if (err) {
        imp = NULL;     /* TODO: destroy from & list */
    } else {
        imp = ast_make_import(from, list, lineno(parser));
    }
    return imp;
}

static struct ast* alias(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_ALIAS)) {
        err = true;
    }

    struct ast *tp = type(parser);
    if (tp == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid alias type");
    }

    struct ast *name = ident(parser);

    PARSE_DEBUG(parser, "Parsed `alias`");

    struct ast *ali = NULL;
    if (err) {
        ali = NULL; /* TODO: destroy type & name */
    } else {
        ali = ast_make_alias(tp, name, lineno(parser));
    }
    return ali;
}

static struct ast* data(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_DATA)) {
        err = true;
    }

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast *members = ast_make_list(AST_LIST_MEMBERS, lineno(parser));
    while (!check(parser, TOK_RCURLY)) {
        struct ast *member = data_member(parser);
        if (member == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid data member");
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

    PARSE_DEBUG(parser, "Parsed `data`");

    struct ast *d = NULL;
    if (err) {
        d = NULL;   /* TODO: destroy name & members */
    } else {
        d = ast_make_data(name, members, lineno(parser));
    }

    return d;
}

static struct ast* data_member(struct parser *parser)
{
    bool err = false;

    struct ast* tp = type(parser);
    struct ast *names = ast_make_list(AST_LIST_IDENTS, lineno(parser));
    do {
        struct ast *name = ident(parser);
        if (name == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid data member name");
            break;
        }
        names = ast_list_append(names, name);
    } while (accept(parser, TOK_COMMA));

    struct ast *member = NULL;
    if (err) {
        member = NULL;  /* TODO: destroy tp, names */
    } else {
        member = ast_make_decl(DECL_VAR, tp, names, lineno(parser));
    }
    return member;
}

static struct ast* methods(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_METHODS)) {
        err = true;
    }

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast *defs = ast_make_list(AST_LIST_METHODS, lineno(parser));
    while (!check(parser, TOK_RCURLY)) {
        struct ast *def = funcdef(parser);
        if (def == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid method definition");
            break;
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        defs = ast_list_append(defs, def);
    }

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed `methods`");

    struct ast *meths = NULL;
    if (err) {
        meths = NULL;   /* TODO: clean up name and funcdefs */
    } else {
        meths = ast_make_impl(name, defs, lineno(parser));
    }
    return meths;
}

static struct ast* funcdef(struct parser *parser)
{
    bool err = false;

    struct ast *ft = functype(parser);
    struct ast *name = ident(parser);

    struct ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid function definition");
    }

    PARSE_DEBUG(parser, "Parsed function definition");

    struct ast *fn = NULL;
    if (err) {
        fn = NULL;   /* TODO: destroy ft, name, blk */
    } else {
        fn = ast_make_function(name, ft, blk, lineno(parser));
    }
    return fn;
}

static struct ast* interface(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_INTERFACE)) {
        err = true;
    }

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast *decls = ast_make_list(AST_LIST_METHOD_DECLS, lineno(parser));
    while (!check(parser, TOK_RCURLY)) {
        struct ast *decl = methdecl(parser);
        if (decl == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid method declaration");
            break;
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        decls = ast_list_append(decls, decl);
    }
    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed `interface`");

    struct ast *iface = NULL;
    if (err) {
        iface = NULL; /* TODO: destroy name & methods */
    } else {
        iface = ast_make_interface(name, decls, lineno(parser));
    }
    return iface;
}

static struct ast* methdecl(struct parser *parser)
{
    bool err = false;

    struct ast *ft = functype(parser);
    struct ast *name = ident(parser);

    PARSE_DEBUG(parser, "Parsed method declaration");

    struct ast *decl = NULL;
    if (err) {
        decl = NULL;   /* TODO: destroy ft, name */
    } else {
        decl = ast_make_decl(DECL_CONST, ft, name, lineno(parser));
    }
    return decl;
}

static struct ast* statement(struct parser *parser)
{
    struct ast *stmt = NULL;
    if (check(parser, TOK_ALIAS)) {
        stmt = alias(parser);
    } else if (check(parser, TOK_VAR) || check(parser, TOK_CONST)) {
        stmt = declaration(parser);
    } else if (check(parser, TOK_WHILE)) {
        stmt = whileloop(parser);
    } else if (check(parser, TOK_FOR)) {
        stmt = forloop(parser);
    } else if (check(parser, TOK_IF)) {
        stmt = ifelse(parser);
    } else if (check(parser, TOK_RET)) {
        stmt = return_statement(parser);
    } else if (accept(parser, TOK_BREAK)) {
        stmt = ast_make_break(lineno(parser));
    } else if (accept(parser, TOK_CONT)) {
        stmt = ast_make_continue(lineno(parser));
    } else if (!check(parser, TOK_RCURLY) && !check(parser, TOK_SEMI)) {
        stmt = ident_statement(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid statement (found token %s)", get_tok_name(parser->cur));
        stmt = NULL;
    }
    return stmt;
}

static struct ast* return_statement(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_RET)) {
        err = true;
    }

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
        ret = ast_make_return(expr, lineno(parser));
    }
    return ret;
}

static struct ast* declaration(struct parser *parser)
{
    bool err = false;

    /* Parse the 'kind' of declaration (variable or constant) */
    int kind = -1;
    if (accept(parser, TOK_VAR)) {
        kind = DECL_VAR;
    } else if (accept(parser, TOK_CONST)) {
        kind = DECL_CONST;
    } else {
        err = true;
        /* what should happen here? */
    }

    struct ast *tp = type(parser);
    if (tp == NULL) {
        err = true;
        /* FIXME: error message */
    }

    /* parse the first declaration RHS */
    struct ast *name = declrhs(parser);
    if (name == NULL) {
        err = true;
        /* FIXME: error message? */
    }

    struct ast *rhs = NULL;
    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct ast *list = ast_make_list(AST_LIST_DECLS, lineno(parser));
        list = ast_list_append(list, name);
        do {
            name = declrhs(parser);
            if (name == NULL) {
                err = true;
                /* FIXME: error message */
            }
            list = ast_list_append(list, name);
        } while (accept(parser, TOK_COMMA));

        rhs = list;
    } else {
        rhs = name;
    }

    struct ast *decl = NULL;
    if (err) {
        decl = NULL;    /* TODO: destroy type & names */
    } else {
        decl = ast_make_decl(kind, tp, rhs, lineno(parser));
    }
    return decl;
}

static struct ast* declrhs(struct parser *parser)
{
    bool err = false;

    struct ast *id = ident(parser);

    struct ast* rhs = NULL;
    if (accept(parser, TOK_ASS)) {
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid initializer expression");
        }
        PARSE_DEBUG(parser, "Parsed initialization");
        rhs = ast_make_initialization(id, expr, lineno(parser));
    } else {
        PARSE_DEBUG(parser, "Parsed declaration");
        rhs = id;
    }

    if (err) {
        rhs = NULL;    /* TODO: destroy rhs */
    }
    return rhs;
}

static struct ast* type(struct parser *parser)
{
    bool err = false;

    struct ast *type = NULL;

    if (accept(parser, TOK_LSQUARE)) {
        struct ast *listtype = ident(parser);
        if (!expect(parser, TOK_RSQUARE)) {
            err = true;
        }
        type = ast_make_list_type(listtype, lineno(parser));
    } else if (accept(parser, TOK_LCURLY)) {
        struct ast *keytype = ident(parser);
        if (!expect(parser, TOK_COMMA)) {
            err = true;
        }
        struct ast *valtype = ident(parser);
        if (!expect(parser, TOK_RCURLY)) {
            err = true;
        }
        type = ast_make_map_type(keytype, valtype, lineno(parser));
    } else if (check(parser, TOK_FUNC)) {
        type = functype(parser);
        if (type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function type");
        }
    } else {
        type = ident(parser);
        /* parse types defined in other modules, e.g. std.file */
        if (accept(parser, TOK_DOT)) {
            struct ast *type2 = ident(parser);
            struct ast *qualified = ast_make_qualified(type, type2, lineno(parser));
            type = qualified;
            PARSE_DEBUG(parser, "Parsed qualified type");
        }
    }

    if (err) {
        type = NULL;    /* TODO: destroy type */
    }
    return type;
}

static struct ast* ident_statement(struct parser *parser)
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
        struct ast *assignment = ast_make_assignment(lhs, ass, expr, lineno(parser));
        PARSE_DEBUG(parser, "Parsed assignment");
        stmt = assignment;
    } else if (parser->cur == TOK_BIND) {
        next(parser);
        struct ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid expression in short-hand declaration");
        }
        struct ast *bind = ast_make_bind(lhs, expr, lineno(parser));
        PARSE_DEBUG(parser, "Parsed short_decl");
        stmt = bind;
    } else {
        /* The only type of expression that can double as a statement is
            a function call (disregarding the return value)...
            so "lhs" should resolve to a "call" or it's a semantic error */
        stmt = lhs;
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
        expr = ast_make_unexpr(unop, inner, lineno(parser));
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
                struct ast *binexpr = ast_make_binexpr(lhs, thisop, rhs, lineno(parser));
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
        cur = ast_make_binexpr(lhs, op, cur, lineno(parser));
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
            term = ast_make_lookup(term, idx, lineno(parser));
            if (!expect(parser, TOK_RSQUARE)) {
                err = true;
            }
        } else if (check(parser, TOK_LPAREN)) {
            struct ast *args = arguments(parser);
            if (args == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid func arguments");
            }
            term = ast_make_call(term, args, lineno(parser));
            PARSE_DEBUG(parser, "Parsed function call");
        } else if (accept(parser, TOK_DOT)) {
            struct ast *child = ident(parser);
            /* TODO: check if child is NULL? */
            term = ast_make_selector(term, child, lineno(parser));
        } else {
            break;
        }
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
        struct string *s = stringtable_wrap(parser->strtab,
                current_buffer(parser));
        PARSE_DEBUGF(parser, "Parsed identifier: %s", s->str);
        id = ast_make_ident(s, lineno(parser));
    }
    return id;
}

static struct ast *intlit(struct parser *parser)
{
    if (!expect(parser, TOK_INT)) {
        return NULL;
    }

    struct ast *lit = NULL;

    /* don't call parser functions while holding this pointer */
    char *tmpbuff = current_buffer(parser);

    char *endptr = NULL;
    size_t len = strlen(tmpbuff);
    long l = strtol(tmpbuff, &endptr, 0);
    PARSE_DEBUGF(parser, "Parsed int literal: %s", tmpbuff);
    if (endptr != (tmpbuff + len)) {
        PARSE_ERRORF(parser, "Invalid integer %s", tmpbuff);
        lit = NULL;
    } else {
        lit = ast_make_int_num(l, lineno(parser));
    }
    return lit;
}

static struct ast *reallit(struct parser *parser)
{
    if (!expect(parser, TOK_REAL)) {
        return NULL;
    }

    struct ast *lit = NULL;

    /* don't call parser functions while holding this pointer */
    char *tmpbuff = current_buffer(parser);

    char *endptr = NULL;
    size_t len = strlen(tmpbuff);
    double d = strtod(tmpbuff, &endptr);
    PARSE_DEBUGF(parser, "Parsed real literal: %s", tmpbuff);
    if (endptr != (tmpbuff + len)) {
        PARSE_ERRORF(parser, "Invalid real number %s", tmpbuff);
        lit = NULL;
    } else {
        lit = ast_make_real_num(d, lineno(parser));
    }
    return lit;
}

static struct ast* operand(struct parser *parser)
{
    struct ast *op = NULL;
    if (check(parser, TOK_IDENT)) {
        op = ident(parser);
    } else if (accept(parser, TOK_BOOL)) {
        char *tmpbuff = current_buffer(parser);
        if (strcmp(tmpbuff, "true") == 0) {
            PARSE_DEBUGF(parser, "Parsed bool literal: %s", tmpbuff);
            op = ast_make_bool_lit(true, lineno(parser));
        } else if (strcmp(tmpbuff, "false") == 0) {
            PARSE_DEBUGF(parser, "Parsed bool literal: %s", tmpbuff);
            op = ast_make_bool_lit(false, lineno(parser));
        } else {
            PARSE_ERRORF(parser, "Invalid bool literal: %s", tmpbuff);
            assert(false);  /* stupid but should never happen */
        }
    } else if (accept(parser, TOK_CHAR)) {
        char *tmpbuff = current_buffer(parser);
        char c = tmpbuff[0];
        PARSE_DEBUGF(parser, "Parsed char literal: %s", tmpbuff);
        op = ast_make_char_lit(c, lineno(parser));
    } else if (check(parser, TOK_INT)) {
        op = intlit(parser);
    } else if (check(parser, TOK_REAL)) {
        op = reallit(parser);
    } else if (accept(parser, TOK_STRING)) {
        struct string *s = stringtable_wrap(parser->strtab,
                current_buffer(parser));
        PARSE_DEBUGF(parser, "Parsed string literal: %s", s->str);
        op = ast_make_str_lit(s, lineno(parser));
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
        op = listlit(parser);
    } else if (check(parser, TOK_LCURLY)) {
        op = maplit(parser);
    } else if (check(parser, TOK_FUNC)) {
        op = funclit(parser);
    } else if (check(parser, TOK_AMP)) {
        op = datalit(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid operand: %s", current_buffer(parser));
        op = NULL;
    }

    return op;
}

static struct ast* listlit(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LSQUARE)) {
        err = true;
    }

    struct ast* expr_list = ast_make_list(AST_LIST_LISTLIT, lineno(parser));
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

static struct ast* maplit(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast* keyval_list = ast_make_list(AST_LIST_MAPLIT, lineno(parser));
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

            struct ast *kv = ast_make_keyval(key, val, lineno(parser));
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

static struct ast* funclit(struct parser *parser)
{
    bool err = false;

    struct ast *ft = functype(parser);

    struct ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid function literal");
    }

    PARSE_DEBUG(parser, "Parsed function literal");

    struct ast *fnlit = NULL;
    if (err) {
        fnlit = NULL;   /* TODO: destroy ft, blk */
    } else {
        fnlit = ast_make_function(NULL, ft, blk, lineno(parser));
    }
    return fnlit;
}

static struct ast* datalit(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_AMP)) {
        err = true;
    }

    struct ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct ast* init_list = ast_make_list(AST_LIST_DATA_INITS, lineno(parser));
    if (!check(parser, TOK_RCURLY)) {
        do {
            /*  data initializers can be either:
                    ident : expression
                    expression
                since they are ordered by their declaration */

            struct ast* item = NULL;

            if (check(parser, TOK_IDENT)) {
                item = ident(parser);
                if (accept(parser, TOK_COLON)) {
                    /* forget about the data member name */
                    item = expression(parser);
                }
            } else {
                item = expression(parser);
            }

            if (item == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid item in data initializer");
            }

            init_list = ast_list_append(init_list, item);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed data initializer");

    struct ast *dlit = NULL;
    if (err) {
        dlit = NULL;     /* TODO: destroy keyval_list */
    } else {
        dlit = ast_make_datalit(name, init_list, lineno(parser));
    }
    return dlit;
}

static struct ast* arguments(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct ast* arg_list = ast_make_list(AST_LIST_ARGS, lineno(parser));
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

    struct ast *statements = ast_make_list(AST_LIST_STATEMENTS, lineno(parser));
    /* parse statements until we see a '}' token */
    do {
        struct ast *stmt = statement(parser);
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
    } while (!check(parser, TOK_RCURLY));

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    if (err) {
        /* TODO: destroy statements */
        statements = NULL;
    }
    return statements;
}

static struct ast* ifelse(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_IF)) {
        err = true;
    }

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
        if (check(parser, TOK_IF)) {
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
        if_stmt = ast_make_ifelse(cond, if_block, else_block, lineno(parser));
    }

    return if_stmt;
}

static struct ast* whileloop(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_WHILE)) {
        err = true;
    }

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
        loop = ast_make_while(cond, blk, lineno(parser));
    }
    return loop;
}

static struct ast* forloop(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_FOR)) {
        err = true;
    }

    struct ast *var = ident(parser);

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
        loop = ast_make_for(var, range, blk, lineno(parser));
    }
    return loop;
}

/* A parameter list is either a single declaration or
 * many comma-separated declarations */
static struct ast* parameters(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct ast *params = ast_make_list(AST_LIST_PARAMS, lineno(parser));
    if (!check(parser, TOK_RPAREN)) {
        do {

            /* parse optional parameter kind (const) */
            int kind = DECL_VAR;
            if (accept(parser, TOK_CONST)) {
                kind = DECL_CONST;
            }

            struct ast *tp = type(parser);
            if (tp == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid parameter type");
                break;
            }

            struct ast *rhs = NULL;
            if (check(parser, TOK_IDENT)) {
                rhs = declrhs(parser);
                if (rhs == NULL) {
                    err = true;
                    PARSE_ERROR(parser, "Invalid parameter");
                    break;
                }
            }

            struct ast *decl = ast_make_decl(kind, tp, rhs, lineno(parser));
            params = ast_list_append(params, decl);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RPAREN)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed parameters");

    if (err) {
        params = NULL;  /* TODO: destroy params */
    }
    return params;
}

static struct ast* functype(struct parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_FUNC)) {
        err = true;
    }

    struct ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = type(parser);
        if (ret_type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    struct ast *params = parameters(parser);
    if (params == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid parameters in function signature");
    }

    PARSE_DEBUG(parser, "Parsed function type");

    struct ast *ft = NULL;
    if (err) {
        ft = NULL;  /* TODO: destroy ret_type & params */
    } else {
        ft = ast_make_func_type(ret_type, params, lineno(parser));
    }
    return ft;
}
