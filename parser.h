#ifndef NOLLI_PARSER_H
#define NOLLI_PARSER_H

#include "lexer.h"

struct parser {
    struct lexer *lexer;
    int cur;
    bool error;
};

void parse_module(struct parser *parser);

#endif /* NOLLI_PARSER_H */
