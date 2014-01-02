#ifndef NOLLI_DEBUG_H
#define NOLLI_DEBUG_H

#define NOLLI_ERROR(fmt, ...) \
    do { \
        fprintf(stderr, "ERROR: %s(): " fmt, \
                __func__, __VA_ARGS__); \
    } while (0)

/**
 * Verbosely prints error information prior to exiting
 *
 * @param fmt C-style formatting
 * @param ... everything to print
 */
#define NOLLI_DIE(fmt, ...) \
    do { \
        fprintf(stderr, "FATAL: %s:%d:%s(): " fmt, \
                __FILE__, __LINE__, __func__, __VA_ARGS__); \
        exit(1); \
    } while (0)

#ifdef DEBUG

/**
 * Verbosely prints debug information
 *
 * @param fmt C-style formatting
 * @param ... everything to print
 */
#define NOLLI_DEBUG(fmt, ...) \
    do { \
        fprintf(stdout, "DEBUG: %s(): " fmt, \
                __func__, __VA_ARGS__); \
    } while (0)

#else   /* DEBUG */

/**
 * Does not print any debug information.
 *
 * @param fmt C-style formatting
 * @param ... everything to not print
 */
#define NOLLI_DEBUG(fmt, ...)

#endif  /* DEBUG */

#endif /* NOLLI_DEBUG_H */
