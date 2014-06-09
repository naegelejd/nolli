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

void parser_init(struct parser *parser);
int parse_buffer(struct nolli_state*, char*);

#endif /* NOLLI_PARSER_H */
