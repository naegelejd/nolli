#ifndef NOLLI_DEBUG_H
#define NOLLI_DEBUG_H

#include <stdio.h>

#define ANSI_RED        "\x1b[31m"
#define ANSI_GREEN      "\x1b[32m"
#define ANSI_YELLOW     "\x1b[33m"
#define ANSI_BLUE       "\x1b[34m"
#define ANSI_MAGENTA    "\x1b[35m"
#define ANSI_CYAN       "\x1b[36m"
#define ANSI_BOLD       "\x1b[1m"
#define ANSI_RESET      "\x1b[0m"

#define BLUE_DEBUG      ANSI_BOLD ANSI_BLUE "Debug" ANSI_RESET ": "
#define RED_ERROR       ANSI_BOLD ANSI_RED "Error" ANSI_RESET ": "
#define RED_FATAL       ANSI_BOLD ANSI_RED "Fatal" ANSI_RESET ": "

#ifdef DEBUG

#define NOLLI_DEBUGF(fmt, ...) \
        fprintf(stdout, BLUE_DEBUG "[%s:%d]: " fmt "\n", \
                __func__, __LINE__, __VA_ARGS__)

#define NOLLI_ERRORF(fmt, ...) \
        fprintf(stderr, RED_ERROR "[%s:%d]: " fmt "\n", \
                __func__, __LINE__, __VA_ARGS__)

#define NOLLI_FATALF(fmt, ...) \
    do { \
        fprintf(stderr, RED_FATAL "[%s:%d:%s]: " fmt "\n", \
                __FILE__, __LINE__, __func__, __VA_ARGS__); \
        exit(NL_ERR_FATAL); \
    } while (0)

#else   /* DEBUG */

#define NOLLI_DEBUGF(fmt, ...)

#define NOLLI_ERRORF(fmt, ...) \
        fprintf(stderr, RED_ERROR fmt "\n", __VA_ARGS__)

#define NOLLI_FATALF(fmt, ...) \
    do { \
        fprintf(stderr, RED_FATAL fmt "\n", __VA_ARGS__); \
        exit(NL_ERR_FATAL); \
    } while (0)

#endif  /* DEBUG */

#define NOLLI_DEBUG(S) NOLLI_DEBUGF("%s", S)
#define NOLLI_ERROR(S) NOLLI_ERRORF("%s", S)
#define NOLLI_FATAL(S) NOLLI_FATALF("%s", S)

#endif /* NOLLI_DEBUG_H */
