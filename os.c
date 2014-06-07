#include "os.h"
#include "sys/stat.h"
#include "unistd.h"


static int nl_mkdir(char *path);
static int nl_rmdir(char *path);

static char *nl_join(char *fst, char *snd);
static char *nl_abspath(char *path);
static char *nl_expanduser(char *path);


struct nolli_internal_os_struct os = {
    .mkdir = nl_mkdir,
    .rmdir = nl_rmdir,
    .path = {
        .join = nl_join,
        .abspath = nl_abspath,
        .expanduser = nl_expanduser
    }
};


static int nl_mkdir(char *path)
{
    return mkdir(path, 744);
}

static int nl_rmdir(char *path)
{
    return rmdir(path);
}

static char *nl_join(char *fst, char *snd)
{
    size_t len_fst = strnlen(fst, NOLLI_OS_PATH_MAX_LEN);
    size_t len_snd = strnlen(fst, NOLLI_OS_PATH_MAX_LEN);
    char *path = nalloc(len_fst + len_snd + 2);
    path = strcat(strcat(strcat(path, fst), "/"), snd);
    return path;
}

static char *nl_abspath(char *path)
{
    return path;
}

static char *nl_expanduser(char *orig)
{
    char *home = getenv("HOME");
    size_t len_home = strnlen(home, NOLLI_OS_PATH_MAX_LEN);
    size_t len_path = strnlen(orig, NOLLI_OS_PATH_MAX_LEN);

    char *ptr = orig;
    char c;
    while ((c = *ptr++)) {
        if (c == '~') {
            len_path += len_home;
        }
    }

    char *path = nalloc(len_path + 1);
    ptr = path;
    while ((c = *orig++)) {
        if (c == '~') {
            int i = 0;
            for (i = 0; i < len_home; i++) {
                *ptr++ = home[i];
            }
        } else {
            *ptr++ = c;
        }
    }

    return path;
}
