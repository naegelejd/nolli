#ifndef NOLLI_TYPE_H
#define NOLLI_TYPE_H

typedef enum {
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
} typeid_t;

typedef struct type {
    typeid_t id;
    const char* name;
    struct type** kinds;
    unsigned int n;
} type_t;

extern type_t bool_type;
extern type_t char_type;
extern type_t int_type;
extern type_t real_type;
extern type_t str_type;
extern type_t file_type;

/* TODO: all types should be hashed when they are first parsed! */
type_t* new_list_type(type_t* tp);
type_t* new_map_type(type_t* ktp, type_t* vtp);
type_t* new_user_type(char *name);

#endif /* NOLLI_TYPE_H */
