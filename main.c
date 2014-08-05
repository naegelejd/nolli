#include "nolli.h"
#include "debug.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        NOLLI_ERROR("Nothing to compile :(");
        return EXIT_FAILURE;
    }

    struct nl_context ctx;
    nl_init(&ctx);

    int i = 1;
    for (i = 1; i < argc; i++) {
        nl_compile_file(&ctx, argv[i]);
    }

    nl_graph_ast(&ctx);
    nl_analyze(&ctx);
    /* nl_execute(&ctx); */

    return EXIT_SUCCESS;
}
