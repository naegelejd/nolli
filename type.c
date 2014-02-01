#include "type.h"

type_t bool_type = {
    TYPE_BOOL,
    "bool",
    0,
    0
};

type_t char_type = {
    TYPE_CHAR,
    "char",
    0,
    0
};

type_t int_type = {
    TYPE_INT,
    "int",
    0,
    0
};

type_t real_type = {
    TYPE_REAL,
    "real",
    0,
    0
};

type_t str_type = {
    TYPE_STR,
    "str",
    0,
    0
};

type_t file_type = {
    TYPE_FILE,
    "file",
    0,
    0
};

type_t* new_list_type(type_t* tp)
{
    type_t* list_type = nalloc(sizeof(*list_type));
    list_type->id = TYPE_LIST;
    list_type->name = strdup("list");
    list_type->n = 1;
    list_type->kinds = nalloc(list_type->n * sizeof(*list_type->kinds));
    *list_type->kinds = tp;

    return list_type;
}

type_t* new_map_type(type_t* ktp, type_t* vtp)
{
    type_t* map_type = nalloc(sizeof(*map_type));
    map_type->id = TYPE_MAP;
    map_type->name = strdup("map");
    map_type->n = 2;
    map_type->kinds = nalloc(map_type->n * sizeof(*map_type->kinds));
    map_type->kinds[0] = ktp;
    map_type->kinds[1] = vtp;

    return map_type;
}

type_t* new_user_type(char *name)
{
    type_t* user_type = nalloc(sizeof(*user_type));
    user_type->id = TYPE_USER;
    user_type->name = name;

    return user_type;
}
