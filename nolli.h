/**
 * @file nolli.h
 */
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
    NL_ERR_JIT,
    NL_ERR_FATAL,
    NL_ERR_END
};

typedef void (*nl_error_handler)(void* user_data, int err, const char* fmt, ...);
typedef void (*nl_debug_handler)(void* user_data, const char* fmt, ...);
typedef void* (*nl_allocator)(void* user_data, void* memory, size_t bytes);
typedef void (*nl_deallocator)(void* user_data, void* memory);

struct nl_context {
    struct nl_strtab* strtab;
    struct nl_ast* ast_list;
    void* user_data;
    nl_error_handler error_handler;
    nl_debug_handler debug_handler;
    nl_allocator allocator;
    nl_deallocator deallocator;
};

int nl_init(struct nl_context* ctx);

int nl_parse_string(struct nl_context* ctx, const char* s, const char* src);

int nl_compile_file(struct nl_context* ctx, const char* filename);
int nl_compile_string(struct nl_context* ctx, const char* s, const char* src);

void nl_add_ast(struct nl_context* ctx, struct nl_ast*);

int nl_graph_ast(struct nl_context* ctx);

int nl_analyze(struct nl_context* ctx, struct nl_ast** packages);

int nl_jit(struct nl_context* ctx, struct nl_ast* packages, int* return_code);

void nl_set_error_handler(struct nl_context* ctx, nl_error_handler handler);
void nl_set_debug_handler(struct nl_context* ctx, nl_debug_handler handler);

void nl_set_allocator(struct nl_context* ctx, nl_allocator allocator);
void nl_set_deallocator(struct nl_context* ctx, nl_deallocator deallocator);

void nl_set_user_data(struct nl_context* ctx, void* user_data);
void* nl_get_user_data(struct nl_context* ctx);

void* nl_realloc(struct nl_context* ctx, void* block, size_t bytes);
#define nl_alloc(ctx, size) nl_realloc((ctx), NULL, (size))
void nl_free(struct nl_context* ctx, void* block);

extern const char* NL_GLOBAL_PACKAGE_NAME;

#endif /* NOLLI_H */
