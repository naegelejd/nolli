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
            fprintf(stderr, "Can't read from file %s\n", filename);
            return EXIT_FAILURE;
        }
    }

    bool scanonly = false;
    if (argc >= 3 && strncmp(argv[2], "-l", 3) == 0) {
        scanonly = true;
    }

    struct lexer *lex = NULL;
    lexer_init(&lex, fin);

    struct parser *parser = NULL;
    calloc(1, sizeof(*parser));
    parser_init(&parser, lex);

    struct ast *root = NULL;

    if (scanonly) {
        lexer_scan_all(lex);
    } else {
        root = parse(parser);
    }

    if (fclose(fin) != 0) {
        fprintf(stderr, "Failed to close file %s\n", filename);
        return EXIT_FAILURE;
    }

    if (parser->error) {
        fprintf(stderr, "Parse errors... cannot continue\n");
        return ERR_PARSE;
    }

    if (root == NULL) {
        fprintf(stderr, "Failed to construct AST");
        return EXIT_FAILURE;
    }

    type_check(root);

    return EXIT_SUCCESS;
}
