#ifndef NOLLI_H
#define NOLLI_H

#include <stddef.h>

enum {
    NL_ERR_BEGIN = -1,
    NL_NO_ERR = 0,
    NL_ERR_MEM,
    NL_ERR_IO,
    NL_ERR_EOF,
    NL_ERR_LEX,
    NL_ERR_PARSE,
    NL_ERR_GRAPH,
    NL_ERR_ANALYZE,
    NL_ERR_FATAL,
    NL_ERR_END
};

typedef void (*nl_error_handler_t)(void *user_data, int err, const char *fmt, ...);
typedef void (*nl_debug_handler_t)(void *user_data, const char *fmt, ...);

typedef struct nl_context {
    struct nl_ast *ast_list;
    void *user_data;
    nl_error_handler_t error_handler;
    nl_debug_handler_t debug_handler;
} nl_context;

int nl_init(struct nl_context *context);

int nl_parse_string(struct nl_context *ctx, const char *s, const char *src);

int nl_compile_file(struct nl_context *ctx, const char *filename);
int nl_compile_string(struct nl_context *ctx, const char *s, const char *src);

void nl_add_ast(struct nl_context *ctx, struct nl_ast*);

int nl_graph_ast(struct nl_context *ctx);

int nl_analyze(struct nl_context *ctx);

void nl_execute(struct nl_context *ctx);

void nl_set_error_handler(struct nl_context *ctx, nl_error_handler_t handler);
void nl_set_debug_handler(struct nl_context *ctx, nl_debug_handler_t handler);

void nl_set_user_data(struct nl_context *ctx, void *user_data);
void *nl_get_user_data(struct nl_context *ctx);


void *nl_realloc(struct nl_context *ctx, void* block, size_t bytes);
#define nl_alloc(ctx, size) nl_realloc((ctx), NULL, (size))

#endif /* NOLLI_H */
