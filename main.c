#include "nolli.h"
#include <stdlib.h>

struct nl_ast *compile_file(const char *filename)
{
    FILE *fin = NULL;
    if (!(fin = fopen(filename, "r"))) {
        NOLLI_ERRORF("Can't read from file %s", filename);
        return NULL;
    }

    fseek(fin, 0, SEEK_END);
    long bytes = ftell(fin);
    rewind(fin);
    char *buff = nl_alloc(bytes + 1);
    if (fread(buff, 1, bytes, fin) < bytes) {
        NOLLI_ERRORF("Failed to read file %s", filename);
        return NULL;
    }

    buff[bytes] = '\0';
    struct nl_ast *root = nl_parse_buffer(buff);
    free(buff);

    if (fclose(fin) != 0) {
        NOLLI_ERRORF("Failed to close file %s", filename);
        return NULL;
    }

    if (root == NULL) {
        NOLLI_ERROR("Parse errors... cannot continue");
        return NULL;
    }

    return root;
}

int compile_files(char * const *paths, int count)
{
    assert(count > 0);

    struct nl_ast *head = NULL;
    struct nl_ast **cur = &head;

    int i = 0;
    for (i = 0; i < count; ++i) {
        struct nl_ast *tmp = compile_file(paths[i]);
        if (tmp == NULL) {
            return EXIT_FAILURE;
        }

        *cur = tmp;
        cur = &(*cur)->next;
    }

    nl_graph_ast(head);
    nl_analyze(head);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        NOLLI_ERROR("Nothing to compile :(");
        return EXIT_FAILURE;
    }

    char **paths = ++argv;
    return compile_files(paths, --argc);
}
