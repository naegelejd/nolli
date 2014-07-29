#include "nolli.h"

int compile_file(const char *filename)
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
    struct ast *root = parse_buffer(buff);
    free(buff);

    if (fclose(fin) != 0) {
        NOLLI_ERRORF("Failed to close file %s", filename);
        return EXIT_FAILURE;
    }

    if (root == NULL) {
        NOLLI_ERROR("Parse errors... cannot continue");
        return EXIT_FAILURE;
    }

    graph_ast(root);
    analyze_ast(root);

    return NO_ERR;
}

int compile_files(char * const *paths, int count)
{
    int i = 0;
    for (i = 0; i < count; ++i) {
        int err = compile_file(paths[i]);
        if (err) {
            // do something
        }
    }

    return NO_ERR;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        NOLLI_ERROR("Nothing to compile :(");
    }

    char **paths = ++argv;
    return compile_files(paths, --argc);
}
