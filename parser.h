#ifndef NOLLI_PARSER_H
#define NOLLI_PARSER_H

#include "nolli.h"

enum { MAX_IDENT_LENGTH = 1024 };

struct lexer;

struct parser {
    jmp_buf jmp;
    struct lexer *lexer;
    char *buffer;
    int cur;
    bool error;
};

void parser_init(struct parser **parser_addr, struct lexer *lexer);
struct ast *parse(struct parser *parser);

#endif /* NOLLI_PARSER_H */
