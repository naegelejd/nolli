#include "nolli.h"

int main(int argc, char **argv)
{
    struct nolli_state nstate;
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

    struct parser *parser = NULL;calloc(1, sizeof(*parser));
    parser_init(&parser, lex);

    struct ast *root = NULL;

    bool scanonly = false;
    if (scanonly) {
        lexer_scan_all(lex);
    } else {
        root = parse(parser);
    }

    if (fclose(fin) != 0) {
        NOLLI_DIE("Failed to close file %s\n", filename);
    }

    if (parser->error) {
        return ERR_PARSE;
    }

    if (root == NULL) {
        NOLLI_DIE("%s\n", "Failed to construct AST");
    }

    walk(root);

    return EXIT_SUCCESS;
}
