#include "nolli.h"
#include "alloc.h"
#include "ast.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


int nl_init(struct nl_context *ctx)
{
    return NL_NO_ERR;
}

char *nl_read_file(struct nl_context *ctx, const char *filename)
{
    FILE *fin = NULL;
    if (!(fin = fopen(filename, "r"))) {
        NOLLI_ERRORF("Can't read from file %s", filename);
        return NULL;
    }

    fseek(fin, 0, SEEK_END);
    long bytes = ftell(fin);
    rewind(fin);
    char *buff = nl_alloc(bytes + 1);
    if (fread(buff, 1, bytes, fin) < bytes) {
        NOLLI_ERRORF("Failed to read file %s", filename);
        return NULL;
    }

    buff[bytes] = '\0';

    if (fclose(fin) != 0) {
        NOLLI_ERRORF("Failed to close file %s", filename);
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
    assert(ctx);
    assert(ast);

    if (ctx->ast_list == NULL) {
        ctx->ast_list = nl_ast_make_list(NL_AST_LIST_UNITS, 0);
    }

    ctx->ast_list = nl_ast_list_append(ctx->ast_list, ast);
}

int nl_compile_file(struct nl_context *ctx, const char *filename)
{
    char *buff = nl_read_file(ctx, filename);
    int err = nl_parse_string(ctx, buff, filename);
    free(buff);     /* TODO: who owns? */

    if (err) {
        NOLLI_ERROR("Parse errors... cannot continue");
    }

    return err;
}

int nl_compile_string(struct nl_context *ctx, const char *s, const char *src)
{
    int err = nl_parse_string(ctx, s, src);

    if (err) {
        NOLLI_ERROR("Parse errors... cannot continue");
    }

    return err;
}
