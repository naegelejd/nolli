#ifndef NOLLI_PARSER_H
#define NOLLI_PARSER_H

#include "lexer.h"

struct parser {
    struct lexer *lexer;
    char *buffer;
    int cur;
    bool error;
};

void parser_init(struct parser **parser_addr, struct lexer *lexer);
struct ast *parse(struct parser *parser);

#endif /* NOLLI_PARSER_H */
