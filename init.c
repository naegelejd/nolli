#include "nolli.h"

void nolli_init(struct nolli_state *state)
{
    state->parser = nalloc(sizeof(*state->parser));
    parser_init(state->parser);
}
