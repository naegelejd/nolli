#ifndef NOLLI_H
#define NOLLI_H

#include <stddef.h>

enum {
    NL_ERR_BEGIN = -1,
    NL_NO_ERR = 0,
    NL_ERR_LEX,
    NL_ERR_PARSE,
    NL_ERR_IO,
    NL_ERR_EOF,
    NL_ERR_FATAL,
    NL_ERR_END
};

typedef struct nl_context {
    struct nl_ast *ast_head;
} nl_context;

int nl_init(struct nl_context *context);

int nl_parse_string(struct nl_context *ctx, const char *s, const char *src);

int nl_compile_file(struct nl_context *ctx, const char *filename);
int nl_compile_string(struct nl_context *ctx, const char *s, const char *src);

void nl_add_ast(struct nl_context *ctx, struct nl_ast*);

int nl_graph_ast(struct nl_context *ctx);

int nl_analyze(struct nl_context *ctx);

void nl_execute(struct nl_context *ctx);

#endif /* NOLLI_H */
