#ifndef NOLLI_PARSER_H
#define NOLLI_PARSER_H

#include "nolli.h"

enum { MAX_IDENT_LENGTH = 1024 };

struct parser {
    struct lexer *lexer;
    struct stringtable *strtab;
    int cur;
    bool error;
};

struct ast *parse_buffer(char*);

#endif /* NOLLI_PARSER_H */
