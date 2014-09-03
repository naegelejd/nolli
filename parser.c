#include "lexer.h"
#include "ast.h"
#include "strtab.h"
#include "debug.h"
#include "nolli.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

struct nl_parser {
    struct nl_context *ctx;
    const char *source;
    struct nl_lexer *lexer;
    int cur;
};

#define PARSE_DEBUGF(P, fmt, ...) \
    NL_DEBUGF((P)->ctx, "(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)

#define PARSE_DEBUG(P, S) PARSE_DEBUGF(P, "%s", S)

#define PARSE_ERRORF(P, fmt, ...) \
    NL_ERRORF((P)->ctx, NL_ERR_PARSE, "(L %d, C %d): " fmt, \
            (P)->lexer->line, (P)->lexer->col, __VA_ARGS__)

#define PARSE_ERROR(P, S) PARSE_ERRORF(P, "%s", S)

static int init(struct nl_parser *parser, struct nl_context *ctx,
        const char *buffer, const char *src);

static struct nl_ast *unit(struct nl_parser *parser);
static struct nl_ast *package(struct nl_parser *parser);
static struct nl_ast *global(struct nl_parser *parser);
static struct nl_ast *ident(struct nl_parser *parser);
static struct nl_ast *using(struct nl_parser *parser);
static struct nl_ast *declaration(struct nl_parser *parser);
static struct nl_ast *declrhs(struct nl_parser *parser);
static struct nl_ast *classdef(struct nl_parser *parser);
static struct nl_ast *interface(struct nl_parser *parser);
static struct nl_ast *methdecl(struct nl_parser *parser);
static struct nl_ast *funcdef(struct nl_parser *parser);
static struct nl_ast *statement(struct nl_parser *parser);
static struct nl_ast *alias(struct nl_parser *parser);
static struct nl_ast *whileloop(struct nl_parser *parser);
static struct nl_ast *forloop(struct nl_parser *parser);
static struct nl_ast *ifelse(struct nl_parser *parser);
static struct nl_ast *return_statement(struct nl_parser *parser);
static struct nl_ast *ident_statement(struct nl_parser *parser);
static struct nl_ast *expression(struct nl_parser *parser);
static struct nl_ast *unary_expr(struct nl_parser *parser);
static struct nl_ast *term(struct nl_parser *parser);
static struct nl_ast *operand(struct nl_parser *parser);
static struct nl_ast *intlit(struct nl_parser *parser);
static struct nl_ast *reallit(struct nl_parser *parser);
static struct nl_ast *listlit(struct nl_parser *parser);
static struct nl_ast *maplit(struct nl_parser *parser);
static struct nl_ast *funclit(struct nl_parser *parser);
static struct nl_ast *classlit(struct nl_parser *parser);
static struct nl_ast *parameters(struct nl_parser *parser);
static struct nl_ast *arguments(struct nl_parser *parser);
static struct nl_ast *block(struct nl_parser *parser);
static struct nl_ast *type(struct nl_parser *parser);
static struct nl_ast *templ(struct nl_parser *parser);
static struct nl_ast *functype(struct nl_parser *parser);


#undef next
#define next(P)         ((P)->cur = nl_gettok((P)->lexer))
#define check(P, T)     ((P)->cur == (T))

#define accept(P, T) (check(P, T) ? next(P), true : false)

#define expect(P, T) (accept(P, T) ? true : ( \
            PARSE_ERRORF(P, "Unexpected token: %s, expecting %s", \
                nl_get_tok_name((P)->cur), nl_get_tok_name(T)), \
            next(P), false))

static int init(struct nl_parser *parser, struct nl_context *ctx,
        const char *buffer, const char *src)
{
    assert(parser);

    assert(ctx->strtab != NULL);

    memset(parser, 0, sizeof(*parser));

    parser->ctx = ctx;
    parser->source = src;
    parser->lexer = nl_alloc(ctx, sizeof(*parser->lexer));
    nl_lexer_init(parser->lexer, ctx, buffer);

    /* DEBUGGING: lexer_scan_all(parser->lexer); */

    /* Start parser */
    next(parser);

    return NL_NO_ERR;
}

int nl_parse_string(struct nl_context *ctx, const char *s, const char *src)
{
    struct nl_parser parser;
    int err = init(&parser, ctx, s, src);
    if (err) {
        return err;
    }

    struct nl_ast *root = unit(&parser);
    expect(&parser, TOK_EOF);

    /* DEBUG: dump all symbols/strings */
    /* nl_strtab_dump(parser->ctx->strtab, stdout); */

    if (root == NULL) {
        return NL_ERR_PARSE;
    } else {
        nl_add_ast(ctx, root);
        return NL_NO_ERR;
    }
}

static char *current_buffer(struct nl_parser *parser)
{
    return parser->lexer->lastbuff;
}

static int lineno(struct nl_parser *parser)
{
    return parser->lexer->line;
}

static struct nl_ast *unit(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *globals = nl_ast_make_list(NL_AST_LIST_GLOBALS, lineno(parser));
    struct nl_ast *packages = nl_ast_make_list(NL_AST_LIST_PACKAGES, lineno(parser));
    struct nl_ast *def = NULL;
    while (!check(parser, TOK_EOF)) {
        if (check(parser, TOK_PACKAGE)) {
            def = package(parser);
        } else {
            def = global(parser);
        }

        if (def == NULL) {
            err = true;
            break;
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        if (def->tag == NL_AST_PACKAGE) {
            packages = nl_ast_list_append(packages, def);
        } else {
            globals = nl_ast_list_append(globals, def);
        }
    }

    nl_string_t gname = nl_strtab_wrap(parser->ctx->strtab, NL_GLOBAL_PACKAGE_NAME);
    struct nl_ast *id = nl_ast_make_ident(gname, 0);
    struct nl_ast *gpkg = nl_ast_make_package(id, globals, 0);
    packages = nl_ast_list_append(packages, gpkg);

    struct nl_ast *prog = NULL;
    if (err) {
        prog = NULL;    /* TODO: destroy packages, globals */
    } else {
        prog = nl_ast_make_unit(packages, lineno(parser));
    }

    return prog;
}

static struct nl_ast *package(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_PACKAGE)) {
        err = true;
    }

    struct nl_ast *name = ident(parser);
    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }
    PARSE_DEBUG(parser, "Parsed package declaration");

    /* Parse global definitions inside of package */
    struct nl_ast *defs = nl_ast_make_list(NL_AST_LIST_GLOBALS, lineno(parser));
    /* parse statements until we see a '}' token or EOF */
    while (!check(parser, TOK_RCURLY)) {
        struct nl_ast *def = global(parser);
        if (def == NULL) {
            err = true;
            break;
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        defs = nl_ast_list_append(defs, def);
    }

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    struct nl_ast *pkg = NULL;
    if (err) {
        pkg = NULL;     /* TODO: destroy name, globals */
    } else {
        pkg = nl_ast_make_package(name, defs, lineno(parser));
    }
    return pkg;
}

static struct nl_ast *global(struct nl_parser *parser)
{
    struct nl_ast *def = NULL;
    if (check(parser, TOK_USING)) {
        def = using(parser);
    } else if (check(parser, TOK_VAR) || check(parser, TOK_CONST)) {
        def = declaration(parser);
    } else if (check(parser, TOK_CLASS)) {
        def = classdef(parser);
    } else if (check(parser, TOK_INTERFACE)) {
        def = interface(parser);
    } else if (check(parser, TOK_FUNC)) {
        def = funcdef(parser);
    } else if (check(parser, TOK_ALIAS)) {
        def = alias(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid global definition (found token %s)",
                nl_get_tok_name(parser->cur));
        def = NULL;
    }
    return def;
}

static struct nl_ast *using(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_USING)) {
        err = true;
    }

    struct nl_ast *list = nl_ast_make_list(NL_AST_LIST_USINGS, lineno(parser));
    do {
        struct nl_ast *pkg = ident(parser);
        list = nl_ast_list_append(list, pkg);
    } while (accept(parser, TOK_COMMA));

    PARSE_DEBUG(parser, "Parsed `using`");

    struct nl_ast *imp = NULL;
    if (err) {
        imp = NULL;     /* TODO: destroy from & list */
    } else {
        imp = nl_ast_make_using(list, lineno(parser));
    }
    return imp;
}

static struct nl_ast *alias(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_ALIAS)) {
        err = true;
    }

    struct nl_ast *tp = type(parser);
    if (tp == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid alias type");
    }

    struct nl_ast *name = ident(parser);

    PARSE_DEBUG(parser, "Parsed `alias`");

    struct nl_ast *ali = NULL;
    if (err) {
        ali = NULL; /* TODO: destroy type & name */
    } else {
        ali = nl_ast_make_alias(tp, name, lineno(parser));
    }
    return ali;
}

static struct nl_ast *classdef(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_CLASS)) {
        err = true;
    }

    struct nl_ast *name = ident(parser);
    if (name == NULL) {
        err = true;
    }

    struct nl_ast *tmpl = NULL;
    if (check(parser, TOK_LT)) {
        tmpl = templ(parser);
        if (tmpl == NULL) {
            err = true;
        }
    }

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct nl_ast *members = nl_ast_make_list(NL_AST_LIST_MEMBERS, lineno(parser));
    struct nl_ast *methods = nl_ast_make_list(NL_AST_LIST_METHODS, lineno(parser));

    while (!check(parser, TOK_RCURLY)) {

        struct nl_ast *tp = type(parser);
        if (tp == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid class member type");
            break;
        }

        struct nl_ast *name = ident(parser);
        if (name == NULL) {
            err = true;
        }

        if (check(parser, TOK_LCURLY)) {
            /* Parsing a method definition */
            struct nl_ast *blk = block(parser);
            if (blk == NULL) {
                err = true;
                break;
            }

            struct nl_ast *method = nl_ast_make_function(name, tp, blk, lineno(parser));
            PARSE_DEBUG(parser, "Parsed class method");
            methods = nl_ast_list_append(methods, method);
        } else {
            /* Parsing class member(s) */
            struct nl_ast *names = nl_ast_make_list(NL_AST_LIST_IDENTS, lineno(parser));
            names = nl_ast_list_append(names, name);
            while (accept(parser, TOK_COMMA)) {
                struct nl_ast *name = ident(parser);
                if (name == NULL) {
                    err = true;
                    break;
                }
                names = nl_ast_list_append(names, name);
            }
            struct nl_ast *member = nl_ast_make_decl(NL_DECL_VAR, tp, names, lineno(parser));
            PARSE_DEBUG(parser, "Parsed line of class members");
            members = nl_ast_list_append(members, member);
        }

        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
    }
    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed `class`");

    struct nl_ast *c = NULL;
    if (err) {
        c = NULL;   /* TODO: destroy name & members & methods */
    } else {
        c = nl_ast_make_class(name, tmpl, members, methods, lineno(parser));
    }

    return c;
}

static struct nl_ast *funcdef(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *ft = functype(parser);
    struct nl_ast *name = ident(parser);

    struct nl_ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid function definition");
    }

    PARSE_DEBUG(parser, "Parsed function definition");

    struct nl_ast *fn = NULL;
    if (err) {
        fn = NULL;   /* TODO: destroy ft, name, blk */
    } else {
        fn = nl_ast_make_function(name, ft, blk, lineno(parser));
    }
    return fn;
}

static struct nl_ast *interface(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_INTERFACE)) {
        err = true;
    }

    struct nl_ast *name = ident(parser);

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct nl_ast *decls = nl_ast_make_list(NL_AST_LIST_METHOD_DECLS, lineno(parser));
    while (!check(parser, TOK_RCURLY)) {
        struct nl_ast *decl = methdecl(parser);
        if (decl == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid method declaration");
            break;
        }
        if (!expect(parser, TOK_SEMI)) {
            err = true;
            break;
        }
        decls = nl_ast_list_append(decls, decl);
    }
    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed `interface`");

    struct nl_ast *iface = NULL;
    if (err) {
        iface = NULL; /* TODO: destroy name & methods */
    } else {
        iface = nl_ast_make_interface(name, decls, lineno(parser));
    }
    return iface;
}

static struct nl_ast *methdecl(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *ft = functype(parser);
    struct nl_ast *name = ident(parser);

    PARSE_DEBUG(parser, "Parsed method declaration");

    struct nl_ast *decl = NULL;
    if (err) {
        decl = NULL;   /* TODO: destroy ft, name */
    } else {
        decl = nl_ast_make_decl(NL_DECL_CONST, ft, name, lineno(parser));
    }
    return decl;
}

static struct nl_ast *statement(struct nl_parser *parser)
{
    struct nl_ast *stmt = NULL;
    if (check(parser, TOK_VAR) || check(parser, TOK_CONST)) {
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
        stmt = nl_ast_make_break(lineno(parser));
    } else if (accept(parser, TOK_CONT)) {
        stmt = nl_ast_make_continue(lineno(parser));
    } else if (!check(parser, TOK_RCURLY) && !check(parser, TOK_SEMI)) {
        stmt = ident_statement(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid statement (found token %s)", nl_get_tok_name(parser->cur));
        stmt = NULL;
    }
    return stmt;
}

static struct nl_ast *return_statement(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_RET)) {
        err = true;
    }

    struct nl_ast *expr = NULL;
    if (!check(parser, TOK_SEMI)) {
        expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid expression in return statement");
        }
    }

    PARSE_DEBUG(parser, "Parsed a return statement");

    struct nl_ast *ret = NULL;
    if (err) {
        ret = NULL;     /* TODO: destroy expr */
    } else {
        ret = nl_ast_make_return(expr, lineno(parser));
    }
    return ret;
}

static struct nl_ast *declaration(struct nl_parser *parser)
{
    bool err = false;

    /* Parse the 'kind' of declaration (variable or constant) */
    int kind = -1;
    if (accept(parser, TOK_VAR)) {
        kind = NL_DECL_VAR;
    } else if (accept(parser, TOK_CONST)) {
        kind = NL_DECL_CONST;
    } else {
        err = true;
        /* what should happen here? */
    }

    struct nl_ast *tp = type(parser);
    if (tp == NULL) {
        err = true;
        /* FIXME: error message */
    }

    /* parse the first declaration RHS */
    struct nl_ast *name = declrhs(parser);
    if (name == NULL) {
        err = true;
        /* FIXME: error message? */
    }

    struct nl_ast *rhs = NULL;
    /* only make a declaration list if more than one name is declared */
    if (accept(parser, TOK_COMMA)) {
        struct nl_ast *list = nl_ast_make_list(NL_AST_LIST_DECLS, lineno(parser));
        list = nl_ast_list_append(list, name);
        do {
            name = declrhs(parser);
            if (name == NULL) {
                err = true;
                /* FIXME: error message */
            }
            list = nl_ast_list_append(list, name);
        } while (accept(parser, TOK_COMMA));

        rhs = list;
    } else {
        rhs = name;
    }

    struct nl_ast *decl = NULL;
    if (err) {
        decl = NULL;    /* TODO: destroy type & names */
    } else {
        decl = nl_ast_make_decl(kind, tp, rhs, lineno(parser));
    }
    return decl;
}

static struct nl_ast *declrhs(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *id = ident(parser);

    struct nl_ast *rhs = NULL;
    if (accept(parser, TOK_ASS)) {
        struct nl_ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid initializer expression");
        }
        PARSE_DEBUG(parser, "Parsed initialization");
        rhs = nl_ast_make_initialization(id, expr, lineno(parser));
    } else {
        PARSE_DEBUG(parser, "Parsed declaration");
        rhs = id;
    }

    if (err) {
        rhs = NULL;    /* TODO: destroy rhs */
    }
    return rhs;
}

static struct nl_ast *type(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *type = NULL;

    if (check(parser, TOK_FUNC)) {
        type = functype(parser);
        if (type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function type");
        }
    } else {
        type = ident(parser);
        /* parse types defined in specific packages, e.g. std::file */
        if (accept(parser, TOK_PREF)) {
            struct nl_ast *type2 = ident(parser);
            struct nl_ast *qualified = nl_ast_make_qual_type(type, type2, lineno(parser));
            type = qualified;
            PARSE_DEBUG(parser, "Parsed qualified type");
        }

        if (check(parser, TOK_LT)) {
            struct nl_ast *tmpl = templ(parser);
            if (templ == NULL) {
                err = true;
            }
            type = nl_ast_make_tmpl_type(type, tmpl, lineno(parser));
        }
    }

    if (err) {
        type = NULL;    /* TODO: destroy type */
    }
    return type;
}

static struct nl_ast *templ(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LT)) {
        err = true;
    }

    struct nl_ast *tps = nl_ast_make_list(NL_AST_LIST_TYPES, lineno(parser));
    do {
        struct nl_ast *tp = type(parser);
        if (tp == NULL) {
            err = true;
            break;
        }
        tps = nl_ast_list_append(tps, tp);
    } while (accept(parser, TOK_COMMA));

    if (!expect(parser, TOK_GT)) {
        err = true;
    }

    if (err) {
        tps = NULL;    /* TODO: destroy tps */
    }
    return tps;
}

static struct nl_ast *ident_statement(struct nl_parser *parser)
{
    bool err = false;
    /* This is the easiest way to parse an assignment...
     * Parse the left-hand side as an expression, then worry about
     * what the expression evaluates to later (when traversing AST).  */
    /* struct nl_ast *lhs = expression(parser); */
    struct nl_ast *lhs = term(parser);
    if (lhs == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid left-hand-side");
    }

    struct nl_ast *stmt = NULL;
    if (parser->cur == TOK_ASS || parser->cur == TOK_IADD ||
            parser->cur == TOK_ISUB || parser->cur == TOK_IMUL ||
            parser->cur == TOK_IDIV || parser->cur == TOK_IMOD)
    {
        int ass = parser->cur; /* save assignment operator */
        next(parser);   /* eat assignment operator */
        struct nl_ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid right-hand-side in assignment");
        }
        struct nl_ast *assignment = nl_ast_make_assignment(lhs, ass, expr, lineno(parser));
        PARSE_DEBUG(parser, "Parsed assignment");
        stmt = assignment;
    } else if (parser->cur == TOK_BIND) {
        next(parser);
        struct nl_ast *expr = expression(parser);
        if (expr == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid expression in short-hand declaration");
        }
        struct nl_ast *bind = nl_ast_make_bind(lhs, expr, lineno(parser));
        PARSE_DEBUG(parser, "Parsed short_decl");
        stmt = bind;
    } else {
        /* The only type of expression that can double as a statement is
            a function call (disregarding the return value)...
            so "lhs" should resolve to a "call" or it's a semantic error */
        lhs->tag = NL_AST_CALL_STMT;    /* tweak AST tag */
        stmt = lhs;
    }

    if (err) {
        stmt = NULL;    /* TODO: destroy stmt */
    }
    return stmt;
}

static struct nl_ast *unary_expr(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *expr = NULL;
    if (parser->cur == TOK_NOT || parser->cur == TOK_SUB) {
        int unop = parser->cur;
        next(parser);
        struct nl_ast *inner = unary_expr(parser);
        if (inner == NULL) {
            err = true;
            /* FIXME: error message? NO */
        }
        expr = nl_ast_make_unexpr(unop, inner, lineno(parser));
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
    struct nl_ast **term_stk;
    int *op_stk;
    int term_top;
    int term_size;
    int op_top;
    int op_size;
};

static int shunter_init(struct shunter *shunter)
{
    shunter->op_top = 0;
    shunter->op_size = 8;
    shunter->op_stk = nl_alloc(NULL, shunter->op_size * sizeof(*shunter->op_stk));
    if (shunter->op_stk == NULL) {
        return NL_ERR_MEM;
    }
    shunter->term_top = 0;
    shunter->term_size = 8;
    shunter->term_stk = nl_alloc(NULL, shunter->term_size * sizeof(*shunter->term_stk));
    if (shunter->term_stk == NULL) {
        free(shunter->op_stk);
        return NL_ERR_MEM;
    }
    return NL_NO_ERR;
}

static void shunter_deinit(struct shunter *shunter)
{
    free(shunter->op_stk);
    free(shunter->term_stk);
}

static struct nl_ast *shunter_term_push(struct shunter *shunter, struct nl_ast *term)
{
    shunter->term_stk[shunter->term_top++] = term;
    if (shunter->term_top >= shunter->term_size) {
        shunter->term_size *= 2;
        shunter->term_stk = nl_realloc(NULL, shunter->term_stk,
                shunter->term_size * sizeof(*shunter->term_stk));
    }
    return term;
}

static struct nl_ast *shunter_term_pop(struct shunter *shunter)
{
    return shunter->term_stk[--shunter->term_top];
}

static int shunter_op_push(struct shunter *shunter, int op)
{
    shunter->op_stk[shunter->op_top++] = op;
    if (shunter->op_top >= shunter->op_size) {
        shunter->op_size *= 2;
        shunter->op_stk = nl_realloc(NULL, shunter->op_stk,
                shunter->op_size * sizeof(*shunter->op_stk));
        if (shunter->op_stk == NULL) {
            return NL_ERR_MEM;
        }
    }
    return NL_NO_ERR;
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

static struct nl_ast *expression(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *cur = unary_expr(parser);
    if (cur == NULL) {
        err = true;
        /* FIXME: error message? */
    }

    struct shunter shunter;
    /* if (!shunter_init(&shunter)) { */
    /*     err = true; */
    /*     /1* FIXME: clean-up and return *1/ */
    /* } */
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
                struct nl_ast *rhs = shunter_term_pop(&shunter);
                struct nl_ast *lhs = shunter_term_pop(&shunter);
                struct nl_ast *binexpr = nl_ast_make_binexpr(lhs, thisop, rhs, lineno(parser));
                shunter_term_push(&shunter, binexpr);
            } else {
                break;
            }
        }
        /* if (!shunter_op_push(&shunter, op)) { */
        /*     err = true; */
        /*     break; */
        /* } */
        shunter_op_push(&shunter, op);

        cur = unary_expr(parser);
        if (cur == NULL) {
            err = true;
            /* FIXME: error message? */
        }
    }

    while (!shunter_op_empty(&shunter)) {
        int op = shunter_op_pop(&shunter);
        struct nl_ast *lhs = shunter_term_pop(&shunter);
        cur = nl_ast_make_binexpr(lhs, op, cur, lineno(parser));
    }

    shunter_deinit(&shunter);

    if (err) {
        cur = NULL;     /* TODO: destroy cur */
    }
    return cur;
}

static struct nl_ast *term(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *term = operand(parser);
    if (term == NULL) {
        err = true;
        /* PARSE_ERROR(parser, "Invalid operand"); */
    }

    /* FIXME: dangerous loop? */
    while (true) {
        if (accept(parser, TOK_LSQUARE)) {
            struct nl_ast *idx = expression(parser);
            if (idx == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid expression index");
            }
            term = nl_ast_make_lookup(term, idx, lineno(parser));
            if (!expect(parser, TOK_RSQUARE)) {
                err = true;
            }
        } else if (check(parser, TOK_LPAREN)) {
            struct nl_ast *args = arguments(parser);
            if (args == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid func arguments");
            }
            term = nl_ast_make_call(term, args, lineno(parser));
            PARSE_DEBUG(parser, "Parsed function call");
        } else if (accept(parser, TOK_DOT)) {
            struct nl_ast *child = ident(parser);
            /* TODO: check if child is NULL? */
            term = nl_ast_make_selector(term, child, lineno(parser));
        } else {
            break;
        }
    }

    if (err) {
        term = NULL;    /* TODO: destroy term */
    }
    return term;
}

static struct nl_ast *ident(struct nl_parser *parser)
{
    struct nl_ast *id = NULL;

    if (!expect(parser, TOK_IDENT)) {
        PARSE_ERROR(parser, "Invalid identifier");
    } else {
        nl_string_t s = nl_strtab_wrap(parser->ctx->strtab,
                current_buffer(parser));
        assert(s);
        PARSE_DEBUGF(parser, "Parsed identifier: %s", s);
        id = nl_ast_make_ident(s, lineno(parser));
    }
    return id;
}

static struct nl_ast *intlit(struct nl_parser *parser)
{
    if (!expect(parser, TOK_INT)) {
        return NULL;
    }

    struct nl_ast *lit = NULL;

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
        lit = nl_ast_make_int_lit(l, lineno(parser));
    }
    return lit;
}

static struct nl_ast *reallit(struct nl_parser *parser)
{
    if (!expect(parser, TOK_REAL)) {
        return NULL;
    }

    struct nl_ast *lit = NULL;

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
        lit = nl_ast_make_real_lit(d, lineno(parser));
    }
    return lit;
}

static struct nl_ast *operand(struct nl_parser *parser)
{
    struct nl_ast *op = NULL;
    if (check(parser, TOK_IDENT)) {
        op = ident(parser);
        if (accept(parser, TOK_PREF)) {
            struct nl_ast *child = ident(parser);
            op = nl_ast_make_package_ref(op, child, lineno(parser));
        }
    } else if (accept(parser, TOK_BOOL)) {
        char *tmpbuff = current_buffer(parser);
        if (strcmp(tmpbuff, "true") == 0) {
            PARSE_DEBUGF(parser, "Parsed bool literal: %s", tmpbuff);
            op = nl_ast_make_bool_lit(true, lineno(parser));
        } else if (strcmp(tmpbuff, "false") == 0) {
            PARSE_DEBUGF(parser, "Parsed bool literal: %s", tmpbuff);
            op = nl_ast_make_bool_lit(false, lineno(parser));
        } else {
            PARSE_ERRORF(parser, "Invalid bool literal: %s", tmpbuff);
            assert(false);  /* stupid but should never happen */
        }
    } else if (accept(parser, TOK_CHAR)) {
        char *tmpbuff = current_buffer(parser);
        char c = tmpbuff[0];
        PARSE_DEBUGF(parser, "Parsed char literal: %s", tmpbuff);
        op = nl_ast_make_char_lit(c, lineno(parser));
    } else if (check(parser, TOK_INT)) {
        op = intlit(parser);
    } else if (check(parser, TOK_REAL)) {
        op = reallit(parser);
    } else if (accept(parser, TOK_STRING)) {
        nl_string_t s = nl_strtab_wrap(parser->ctx->strtab,
                current_buffer(parser));
        PARSE_DEBUGF(parser, "Parsed string literal: %s", s);
        op = nl_ast_make_str_lit(s, lineno(parser));
    } else if (accept(parser, TOK_LPAREN)) {
        struct nl_ast *expr = expression(parser);
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
    } else if (check(parser, TOK_NEW)) {
        op = classlit(parser);
    } else {
        PARSE_ERRORF(parser, "Invalid operand: %s", current_buffer(parser));
        op = NULL;
    }

    return op;
}

static struct nl_ast *listlit(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LSQUARE)) {
        err = true;
    }

    struct nl_ast *expr_list = nl_ast_make_list(NL_AST_LIST_LIT, lineno(parser));
    if (!check(parser, TOK_RSQUARE)) {
        do {
            struct nl_ast *expr = expression(parser);
            if (expr == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid expression in list literal");
            }
            expr_list = nl_ast_list_append(expr_list, expr);
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

static struct nl_ast *maplit(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct nl_ast *keyval_list = nl_ast_make_list(NL_AST_MAP_LIT, lineno(parser));
    if (!check(parser, TOK_RCURLY)) {
        do {
            struct nl_ast *key = expression(parser);
            if (key == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid key in map literal");
            }

            if (!expect(parser, TOK_COLON)) {
                err = true;
            }

            struct nl_ast *val = expression(parser);
            if (val == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid value in map literal");
            }

            struct nl_ast *kv = nl_ast_make_keyval(key, val, lineno(parser));
            keyval_list = nl_ast_list_append(keyval_list, kv);
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

static struct nl_ast *funclit(struct nl_parser *parser)
{
    bool err = false;

    struct nl_ast *ft = functype(parser);

    struct nl_ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid function literal");
    }

    PARSE_DEBUG(parser, "Parsed function literal");

    struct nl_ast *fnlit = NULL;
    if (err) {
        fnlit = NULL;   /* TODO: destroy ft, blk */
    } else {
        fnlit = nl_ast_make_function(NULL, ft, blk, lineno(parser));
    }
    return fnlit;
}

static struct nl_ast *classlit(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_NEW)) {
        err = true;
    }

    /* Parse the type of the class literal...
     * Could be in another package... could be templated...
     * TODO: verify elsewhere that a functype is NOT parsed here */
    struct nl_ast *clss = type(parser);

    struct nl_ast *tmpl = NULL;
    if (check(parser, TOK_LT)) {
        tmpl = templ(parser);
        if (tmpl == NULL) {
            err = true;
        }
    }

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct nl_ast *init_list = nl_ast_make_list(NL_AST_LIST_CLASS_INITS, lineno(parser));
    if (!check(parser, TOK_RCURLY)) {
        do {
            /*  class initializers can be either:
                    ident : expression
                    expression
                since they are ordered by their declaration */

            struct nl_ast *item = NULL;

            if (check(parser, TOK_IDENT)) {
                item = ident(parser);
                if (accept(parser, TOK_COLON)) {
                    /* forget about the class member name */
                    item = expression(parser);
                }
            } else {
                item = expression(parser);
            }

            if (item == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid item in class initializer");
            }

            init_list = nl_ast_list_append(init_list, item);
        } while (accept(parser, TOK_COMMA));
    }

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    PARSE_DEBUG(parser, "Parsed class initializer");

    struct nl_ast *clit = NULL;
    if (err) {
        clit = NULL;     /* TODO: destroy keyval_list */
    } else {
        clit = nl_ast_make_classlit(clss, tmpl, init_list, lineno(parser));
    }
    return clit;
}

static struct nl_ast *arguments(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct nl_ast *arg_list = nl_ast_make_list(NL_AST_LIST_ARGS, lineno(parser));
    if (!check(parser, TOK_RPAREN)) {
        do {
            struct nl_ast *expr = expression(parser);
            if (expr == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid argument expression");
            }
            arg_list = nl_ast_list_append(arg_list, expr);
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
static struct nl_ast *block(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LCURLY)) {
        err = true;
    }

    struct nl_ast *statements = nl_ast_make_list(NL_AST_LIST_STATEMENTS, lineno(parser));
    /* parse statements until we see a '}' token */
    while (!check(parser, TOK_RCURLY)) {
        struct nl_ast *stmt = statement(parser);
        if (stmt == NULL) {
            err = true;
            while (!check(parser, TOK_SEMI) && !check(parser, TOK_EOF)) {
                PARSE_DEBUG(parser, "synchronizing");
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
        statements = nl_ast_list_append(statements, stmt);
    }

    if (!expect(parser, TOK_RCURLY)) {
        err = true;
    }

    if (err) {
        /* TODO: destroy statements */
        statements = NULL;
    }
    return statements;
}

static struct nl_ast *ifelse(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_IF)) {
        err = true;
    }

    struct nl_ast *cond = expression(parser);
    if (cond == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid conditional expression in 'if' statement");
    }

    struct nl_ast *if_block = block(parser);
    if (if_block == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid block in 'if' statement");
    }

    struct nl_ast *else_block = NULL;
    if (accept(parser, TOK_ELSE)) {
        /* determine whether to parse another "if+else" statement or just
         * a block of statements */
        struct nl_ast *(*else_parser) (struct nl_parser *) = NULL;
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

    struct nl_ast *if_stmt = NULL;
    if (err) {
        if_stmt = NULL;     /* TODO: destroy cond & if_block & else_block */
    } else {
        if_stmt = nl_ast_make_ifelse(cond, if_block, else_block, lineno(parser));
    }

    return if_stmt;
}

static struct nl_ast *whileloop(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_WHILE)) {
        err = true;
    }

    struct nl_ast *cond = expression(parser);
    if (cond == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid conditional expression in 'while' statement");
    }

    struct nl_ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid statement block in while-statement");
    }

    PARSE_DEBUG(parser, "Parsed `while` loop");

    struct nl_ast *loop = NULL;
    if (err) {
        loop = NULL;    /* TODO: destroy cond & blk */
    } else {
        loop = nl_ast_make_while(cond, blk, lineno(parser));
    }
    return loop;
}

static struct nl_ast *forloop(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_FOR)) {
        err = true;
    }

    struct nl_ast *var = ident(parser);

    if (!expect(parser, TOK_IN)) {
        err = true;
    }
    struct nl_ast *range = expression(parser);
    if (range == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid range expression in 'for' statement");
    }

    struct nl_ast *blk = block(parser);
    if (blk == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid statement block in for-statement");
    }

    PARSE_DEBUG(parser, "Parsed `for` loop");

    struct nl_ast *loop = NULL;
    if (err) {
        loop = NULL;    /* TODO: destroy var & range & blk */
    } else {
        loop = nl_ast_make_for(var, range, blk, lineno(parser));
    }
    return loop;
}

/* A parameter list is either a single declaration or
 * many comma-separated declarations */
static struct nl_ast *parameters(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_LPAREN)) {
        err = true;
    }

    struct nl_ast *params = nl_ast_make_list(NL_AST_LIST_PARAMS, lineno(parser));
    if (!check(parser, TOK_RPAREN)) {
        do {

            /* parse optional parameter kind (const) */
            int kind = NL_DECL_VAR;
            if (accept(parser, TOK_CONST)) {
                kind = NL_DECL_CONST;
            }

            struct nl_ast *tp = type(parser);
            if (tp == NULL) {
                err = true;
                PARSE_ERROR(parser, "Invalid parameter type");
                break;
            }

            struct nl_ast *rhs = NULL;
            if (check(parser, TOK_IDENT)) {
                rhs = declrhs(parser);
                if (rhs == NULL) {
                    err = true;
                    PARSE_ERROR(parser, "Invalid parameter");
                    break;
                }
            }

            struct nl_ast *decl = nl_ast_make_decl(kind, tp, rhs, lineno(parser));
            params = nl_ast_list_append(params, decl);
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

static struct nl_ast *functype(struct nl_parser *parser)
{
    bool err = false;

    if (!expect(parser, TOK_FUNC)) {
        err = true;
    }

    struct nl_ast *ret_type = NULL;
    if (!check(parser, TOK_LPAREN)) {
        ret_type = type(parser);
        if (ret_type == NULL) {
            err = true;
            PARSE_ERROR(parser, "Invalid function return type");
        }
    }

    struct nl_ast *params = parameters(parser);
    if (params == NULL) {
        err = true;
        PARSE_ERROR(parser, "Invalid parameters in function signature");
    }

    PARSE_DEBUG(parser, "Parsed function type");

    struct nl_ast *ft = NULL;
    if (err) {
        ft = NULL;  /* TODO: destroy ret_type & params */
    } else {
        ft = nl_ast_make_func_type(ret_type, params, lineno(parser));
    }
    return ft;
}
