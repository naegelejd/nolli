#include "nolli.h"

void nolli_init(struct nolli_state *state, FILE *fin)
{
    memset(state, 0, sizeof(*state));

    state->lexer = nalloc(sizeof(*state->lexer));
    lexer_init(state->lexer, fin);

    state->parser = nalloc(sizeof(*state->parser));
    parser_init(state->parser, state->lexer);
}
