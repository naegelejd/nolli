#ifndef NOLLI_TYPE_H
#define NOLLI_TYPE_H

#include "nolli.h"

enum {TYPENAME_MAXLEN = 32};

enum {
    TYPE_start,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_REAL,
    TYPE_STR,
    TYPE_FILE,
    TYPE_LIST,
    TYPE_MAP,
    TYPE_USER
};

struct type {
    int id;
    const char* name;
    struct type** kinds;
    unsigned int n;
};

extern struct type bool_type;
extern struct type char_type;
extern struct type int_type;
extern struct type real_type;
extern struct type str_type;
extern struct type file_type;

/* TODO: all types should be hashed when they are first parsed! */
struct type* new_list_type(struct type* tp);
struct type* new_map_type(struct type* ktp, struct type* vtp);
struct type* new_user_type(char *name);

#endif /* NOLLI_TYPE_H */
