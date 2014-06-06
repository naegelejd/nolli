#include "nolli.h"

int dofile(struct nolli_state *nstate, const char *filename)
{
    FILE *fin = NULL;
    if (!(fin = fopen(filename, "r"))) {
        NOLLI_ERRORF("Can't read from file %s", filename);
        return EXIT_FAILURE;
    }

    fseek(fin, 0, SEEK_END);
    long bytes = ftell(fin);
    rewind(fin);
    char *buff = nalloc(bytes + 1);
    fread(buff, bytes, 1, fin);
    buff[bytes] = '\0';
    int err = parse_buffer(nstate, buff);
    free(buff);

    if (fclose(fin) != 0) {
        NOLLI_ERRORF("Failed to close file %s", filename);
        return EXIT_FAILURE;
    }

    if (err) {
        NOLLI_ERROR("Parse errors... cannot continue");
        return err;
    }

    /* dump_ast_graph(nstate); */
    /* type_check(nstate); */

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    struct nolli_state nstate;
    nolli_init(&nstate);

    if (argc >= 2) {
        const char *filename = argv[1];
        return dofile(&nstate, filename);
    } else {
        NOLLI_ERROR("Missing filename argument");
    }

    return EXIT_SUCCESS;
}
