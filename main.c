#include "nolli.h"

int main(int argc, char **argv)
{
    FILE *fin = stdin;
    const char *filename = "stdin";
    if (argc >= 2) {
        filename = argv[1];
        if (!(fin = fopen(filename, "r"))) {
            fprintf(stderr, "Can't read from file %s\n", filename);
            return EXIT_FAILURE;
        }
    }

    struct nolli_state nstate;
    nolli_init(&nstate, fin);

    int p = parse(&nstate);

    if (fclose(fin) != 0) {
        fprintf(stderr, "Failed to close file %s\n", filename);
        return EXIT_FAILURE;
    }

    if (p == ERR_PARSE) {
        fprintf(stderr, "Parse errors... cannot continue\n");
        return p;
    } else if (p == ERR_AST) {
        fprintf(stderr, "Failed to construct AST... cannot continue\n");
        return p;
    }

    dump_ast_graph(&nstate);
    type_check(&nstate);

    return EXIT_SUCCESS;
}
