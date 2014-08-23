#ifndef NOLLI_DEBUG_H
#define NOLLI_DEBUG_H

#define NL_ANSI_RED        "\x1b[31m"
#define NL_ANSI_GREEN      "\x1b[32m"
#define NL_ANSI_YELLOW     "\x1b[33m"
#define NL_ANSI_BLUE       "\x1b[34m"
#define NL_ANSI_MAGENTA    "\x1b[35m"
#define NL_ANSI_CYAN       "\x1b[36m"
#define NL_ANSI_BOLD       "\x1b[1m"
#define NL_ANSI_RESET      "\x1b[0m"

#define NL_BLUE_DEBUG      NL_ANSI_BOLD NL_ANSI_BLUE "Debug" NL_ANSI_RESET ": "
#define NL_RED_ERROR       NL_ANSI_BOLD NL_ANSI_RED "Error" NL_ANSI_RESET ": "
#define NL_RED_FATAL       NL_ANSI_BOLD NL_ANSI_RED "Fatal" NL_ANSI_RESET ": "

#ifdef DEBUG

#define NL_DEBUGF(ctx, fmt, ...) \
        ctx->debug_handler(ctx->user_data, \
                NL_BLUE_DEBUG "[%s:%d]: " fmt "\n", \
                __func__, __LINE__, __VA_ARGS__)

#define NL_ERRORF(ctx, e, fmt, ...) \
        ctx->error_handler(ctx->user_data, e, \
                NL_RED_ERROR "[%s:%d]: " fmt "\n", \
                __func__, __LINE__, __VA_ARGS__)

#define NL_FATALF(ctx, e, fmt, ...) \
    do { \
        ctx->error_handler(ctx->user_data, e, \
                NL_RED_FATAL "[%s:%d:%s]: " fmt "\n", \
                __FILE__, __LINE__, __func__, __VA_ARGS__); \
        exit(e); \
    } while (0)

#else   /* DEBUG */

#define NL_DEBUGF(ctx, fmt, ...)

#define NL_ERRORF(ctx, e, fmt, ...) \
        ctx->error_handler(ctx->user_data, e, \
        NL_RED_ERROR fmt "\n", __VA_ARGS__)

#define NL_FATALF(ctx, e, fmt, ...) \
    do { \
        ctx->error_handler(ctx->user_data, e, \
        NL_RED_FATAL fmt "\n", __VA_ARGS__); \
        exit(e); \
    } while (0)

#endif  /* DEBUG */

#define NL_DEBUG(ctx, S)    NL_DEBUGF(ctx, "%s", S)
#define NL_ERROR(ctx, e, S) NL_ERRORF(ctx, e, "%s", S)
#define NL_FATAL(ctx, e, S) NL_FATALF(ctx, e, "%s", S)

#endif /* NOLLI_DEBUG_H */
