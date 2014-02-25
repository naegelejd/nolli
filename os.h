#ifndef NOLLI_OS_H
#define NOLLI_OS_H

#include "nolli.h"

enum { NOLLI_OS_PATH_MAX_LEN = 16384 };

struct nolli_internal_os_struct
{
    int (*mkdir)(char *path);
    int (*rmdir)(char *path);

    struct {
        char *(*join)(char *fst, char *snd);
        char *(*abspath)(char *path);
        char *(*expanduser)(char *path);
    } path;
};

extern struct nolli_internal_os_struct os;

#endif /* NOLLI_OS_H */
