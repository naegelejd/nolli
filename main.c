#include "nolli.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "%s\n", "Nothing to compile :(");
        return EXIT_FAILURE;
    }

    struct nl_context ctx;
    nl_init(&ctx);

    int err = 0;

    int i = 1;
    for (i = 1; i < argc; i++) {
        int err = nl_compile_file(&ctx, argv[i]);
        if (err) {
            goto early_exit;
        }
    }

    err = nl_graph_ast(&ctx);
    if (err) {
        goto early_exit;
    }

    err = nl_analyze(&ctx);
    if (err) {
        goto early_exit;
    }

    /* nl_execute(&ctx); */

    return EXIT_SUCCESS;

early_exit:
    fprintf(stderr, "%s\n", "Stopping early.");
    return EXIT_FAILURE;
}
