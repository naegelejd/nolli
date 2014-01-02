#include "nolli.h"

int main(int argc, char **argv)
{
    nolli_state_t nstate;
    memset(&nstate, 0, sizeof(nstate));

    FILE *fin = stdin;
    const char *filename = "stdin";
    if (argc >= 2) {
        filename = argv[1];
        if (!(fin = fopen(filename, "r"))) {
            NOLLI_DIE("Can't read from file %s\n", filename);
        }
    }

    struct lexer *lex = NULL;
    lexer_init(&lex, fin);

    struct parser *parser = calloc(1, sizeof(*parser));
    parser->lexer = lex;

    bool scanonly = false;
    if (scanonly) {
        lexer_scan_all(lex);
    } else {
        parse_module(parser);
    }

    if (fclose(fin) != 0) {
        NOLLI_DIE("Failed to close file %s\n", filename);
    }

    if (parser->error) {
        return ERR_PARSE;
    }

    return EXIT_SUCCESS;
}
