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

extern const type_t bool_type;
extern const type_t char_type;
extern const type_t int_type;
extern const type_t real_type;
extern const type_t str_type;
extern const type_t file_type;

/* TODO: all types should be hashed when they are first parsed! */
type_t* new_list_type(const type_t* tp);
type_t* new_map_type(const type_t* ktp, const type_t* vtp);
type_t* new_user_type(const char *name);

#endif /* NOLLI_TYPE_H */
