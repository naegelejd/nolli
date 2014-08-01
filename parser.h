#ifndef NOLLI_PARSER_H
#define NOLLI_PARSER_H

#include "nolli.h"

struct nl_parser {
    struct nl_lexer *lexer;
    struct stringtable *strtab;
    int cur;
};

struct nl_ast *nl_parse_buffer(char*);

#endif /* NOLLI_PARSER_H */
