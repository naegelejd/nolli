/**
 * @file nolli.h
 */
#ifndef NOLLI_H
#define NOLLI_H

#include <stddef.h>

/**
 * @defgroup user-api User API
 * @{
 */
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

/** nolli error message handler function type */
typedef void (*nl_error_handler)(void* user_data, int err, const char* fmt, ...);
/** nolli debug message handler function type */
typedef void (*nl_debug_handler)(void* user_data, const char* fmt, ...);
/** nolli allocator function type */
typedef void* (*nl_allocator)(void* user_data, void* memory, size_t bytes);
/** nolli deallocator function type */
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

/**
 * Initialize a nolli context. Required before using a context.
 *
 * @param ctx nolli context
 */
int nl_init(struct nl_context* ctx);

/**
 * Parse a null-terminated string of nolli source code
 *
 * @param ctx nolli context
 * @param s string of nolli source code
 * @param src source code identifier (e.g. filename)
 */
int nl_parse_string(struct nl_context* ctx, const char* s, const char* src);

int nl_compile_file(struct nl_context* ctx, const char* filename);
int nl_compile_string(struct nl_context* ctx, const char* s, const char* src);

/**
 * Add an AST to a context
 *
 * @param ctx nolli context
 * @param ast root of AST
 */
void nl_add_ast(struct nl_context* ctx, struct nl_ast* ast);

/**
 * Generate a Graphviz (dot) compatible graph of the AST. Printed to `stderr`
 *
 * @param ctx nolli context
 * @returns error code
 */
int nl_graph_ast(struct nl_context* ctx);

/**
 * Perform semantic analysis on an AST
 *
 * @param ctx nolli context
 * @param packages address of pointer to AST list of package nodes
 * @returns error code
 */
int nl_analyze(struct nl_context* ctx, struct nl_ast** packages);

/**
 * JIT-compile and execute the code in the given AST.
 *
 * @param ctx nolli context
 * @param packages an AST list of package nodes
 * @param return_code return code of executed `main` function
 * @returns error code
 */
int nl_jit(struct nl_context* ctx, struct nl_ast* packages, int* return_code);

/**
 * Configure an error message handler for a context.
 *
 * @param ctx nolli context
 * @param handler error message handler function
 */
void nl_set_error_handler(struct nl_context* ctx, nl_error_handler handler);

/**
 * Configure a debug message handler for a context.
 *
 * @param ctx nolli context
 * @param handler debug message handler function
 */
void nl_set_debug_handler(struct nl_context* ctx, nl_debug_handler handler);

/**
 * Configure a custom allocator for a context.
 *
 * @param ctx nolli context
 * @param allocator custom allocator function
 */
void nl_set_allocator(struct nl_context* ctx, nl_allocator allocator);

/**
 * Configure a custom deallocator for a context.
 *
 * @param ctx nolli context
 * @param deallocator custom deallocator function
 */
void nl_set_deallocator(struct nl_context* ctx, nl_deallocator deallocator);

/**
 * Store user data with a context.
 *
 * @param ctx nolli context
 * @param user_data user-defined data
 */
void nl_set_user_data(struct nl_context* ctx, void* user_data);

/**
 * Retrieve user data from a context.
 *
 * @param ctx nolli context
 * @returns user_data
 */
void* nl_get_user_data(struct nl_context* ctx);

/**
 * Equivalent to C-standard `realloc` using context's allocator.
 *
 * @param ctx nolli context
 * @param memory existing memory to resize, or NULL for a new allocation
 * @param bytes size in bytes of requested memory
 * @returns pointer to allocated block of memory
 */
void* nl_realloc(struct nl_context* ctx, void* memory, size_t bytes);

/**
 * Equivalent to C-standard `malloc` using context's allocator.
 *
 * @param ctx nolli context
 * @param bytes size in bytes of requested memory
 * @returns pointer to allocated block of memory
 */
#define nl_alloc(ctx, bytes) nl_realloc((ctx), NULL, (bytes))

/**
 * Equivalent to C-standard `free` using context's deallocator.
 *
 * @param ctx nolli context
 * @param memory block of memory to deallocate
 */
void nl_free(struct nl_context* ctx, void* memory);

/** @} */ /* user-api */

extern const char* NL_GLOBAL_PACKAGE_NAME;

#endif /* NOLLI_H */
