#include "nolli.h"
#include "lexer.h"
#include "parser.h"


int main(void)
{
    nolli_state_t nstate;
    memset(&nstate, 0, sizeof(nstate));

    struct lexer *lex;
    lexer_init(&lex, stdin);

    struct parser *parser = calloc(1, sizeof(*parser));
    parser->lexer = lex;

    bool scanonly = false;
    if (scanonly) {
        lexer_scan_all(lex);
    } else {
        parse_module(parser);
    }

    if (parser->error) {
        return ERR_PARSE;
    }

    return EXIT_SUCCESS;
}
