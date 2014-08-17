#include "nolli.h"
#include "ast.h"
#include "debug.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

static void nl_default_error_handler(void *user_data, int err, const char *fmt, ...);
static void nl_default_debug_handler(void *user_data, const char *fmt, ...);


int nl_init(struct nl_context *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    nl_set_error_handler(ctx, nl_default_error_handler);
    nl_set_debug_handler(ctx, nl_default_debug_handler);
    return NL_NO_ERR;
}

static void nl_default_error_handler(void *user_data, int err, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    va_end(arglist);
}

void nl_set_error_handler(struct nl_context *ctx, nl_error_handler_t handler)
{
    ctx->error_handler = nl_default_error_handler;
}

static void nl_default_debug_handler(void *user_data, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    va_end(arglist);
}

void nl_set_debug_handler(struct nl_context *ctx, nl_debug_handler_t handler)
{
    ctx->debug_handler = nl_default_debug_handler;
}


char *nl_read_file(struct nl_context *ctx, const char *filename)
{
    FILE *fin = NULL;
    if (!(fin = fopen(filename, "r"))) {
        NL_ERRORF(ctx, NL_ERR_IO, "Can't read from file %s", filename);
        return NULL;
    }

    fseek(fin, 0, SEEK_END);
    long bytes = ftell(fin);
    rewind(fin);
    char *buff = nl_alloc(ctx, bytes + 1);
    if (fread(buff, 1, bytes, fin) < bytes) {
        NL_ERRORF(ctx, NL_ERR_IO, "Failed to read file %s", filename);
        free(buff);
        return NULL;
    }

    buff[bytes] = '\0';

    if (fclose(fin) != 0) {
        NL_ERRORF(ctx, NL_ERR_IO, "Failed to close file %s", filename);
        free(buff);
        return NULL;
    }

    return buff;
}

/**
 * Add an AST to a context. A context can contain many ASTs
 * prior to compilation.
 *
 * TODO: return error code
 */
void nl_add_ast(struct nl_context *ctx, struct nl_ast *ast)
{
    assert(ctx != NULL);
    assert(ast != NULL);

    if (ctx->ast_list == NULL) {
        ctx->ast_list = nl_ast_make_list(NL_AST_LIST_UNITS, 0);
    }

    ctx->ast_list = nl_ast_list_append(ctx->ast_list, ast);
}

/* TODO: use or delete this! */
int nl_transform_ast(struct nl_context *ctx)
{
    /* for each unit AST U:
     *  for each global AST G:
     *      if G is a package:
     *          make or get package AST P
     *          for each global g in package:
     *              add g to P
     *      if G is a global:
     *          make or get unnamed package AST P
     *          add G to P
     */

    return NL_NO_ERR;
}

void nl_set_user_data(struct nl_context *ctx, void *user_data)
{
    ctx->user_data = user_data;
}

void *nl_get_user_data(struct nl_context *ctx)
{
    return ctx->user_data;
}

int nl_compile_file(struct nl_context *ctx, const char *filename)
{
    char *buff = nl_read_file(ctx, filename);
    if (buff == NULL) {
        return NL_ERR_IO;
    }

    int err = nl_parse_string(ctx, buff, filename);
    free(buff);     /* TODO: who owns? */

    if (err) {
        NL_ERROR(ctx, err, "Parse errors... cannot continue");
    }

    return err;
}

int nl_compile_string(struct nl_context *ctx, const char *s, const char *src)
{
    int err = nl_parse_string(ctx, s, src);

    if (err) {
        NL_ERROR(ctx, err, "Parse errors... cannot continue");
    }

    return err;
}

void *nl_realloc(struct nl_context *ctx, void* block, size_t bytes)
{
    /* if (block == NULL) { */
    /*     return calloc(1, bytes); */
    /* } */

    void *newblock = realloc(block, bytes);
    if (newblock == NULL) {
        if (ctx != NULL) {
            NL_FATAL(ctx, NL_ERR_MEM, "realloc failed");
        } else {
            abort();    /* FIXME */
        }
    } else {
        memset(newblock, 0, bytes);
    }

    return newblock;
}
