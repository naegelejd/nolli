#include "nolli.h"
#include "linenoise/linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'p') {
        linenoiseAddCompletion(lc,"print");
    }
}

char *find_history(void)
{
    char *histfile = ".nollihist";
    unsigned long HOMEDIR_MAX_LEN = 128;

#ifdef WIN32
    char *homedrive = getenv("HOMEDRIVE");
    char *homepath = getenv("HOMEPATH");
    char *home = nalloc(strnlen(homedrive, HOMEDIR_MAX_LEN) +
            strnlen(HOMEPATH, HOMEDIR_MAX_LEN) + 1);
    home = strcat(home, homedrive);
    home = strcat(home, homepath);
#else
    char *home = getenv("HOME");
#endif

    char *histpath = nalloc(strnlen(home, HOMEDIR_MAX_LEN) + strlen(histfile) + 2);
    histpath = strcat(histpath, home);
    histpath = strcat(histpath, "/");
    histpath = strcat(histpath, histfile);

    return histpath;
}

int dofile(struct nolli_state *nstate, const char *filename)
{
    FILE *fin = NULL;
    if (!(fin = fopen(filename, "r"))) {
        NOLLI_ERRORF("Can't read from file %s", filename);
        return EXIT_FAILURE;
    }

    /* fseek(fin, 0, SEEK_END); */
    /* long bytes = ftell(fin); */
    /* rewind(fin); */
    /* char *buff = nalloc(bytes + 1); */
    /* fread(buff, bytes, 1, fin); */
    /* buff[bytes] = '\0'; */
    /* int p = parse_string(&nstate, buff); */

    int p = parse_file(nstate, fin);

    if (fclose(fin) != 0) {
        NOLLI_ERRORF("Failed to close file %s", filename);
        return EXIT_FAILURE;
    }

    if (p == ERR_PARSE) {
        NOLLI_ERROR("Parse errors... cannot continue");
        return p;
    } else if (p == ERR_AST) {
        NOLLI_ERROR("Failed to construct AST... cannot continue");
        return p;
    }

    dump_ast_graph(nstate);
    type_check(nstate);

    return EXIT_SUCCESS;
}

int interactive(struct nolli_state *nstate)
{
    /* otherwise, it's time to run a REPL! */
    linenoiseSetMultiLine(1);
    linenoiseSetCompletionCallback(completion);
    char *histpath = find_history();
    NOLLI_DEBUGF("History: %s", histpath);
    linenoiseHistoryLoad(histpath);

    char *line;
    while ((line = linenoise("nl: ")) != NULL) {
        int p = parse_string(nstate, line);
        linenoiseHistoryAdd(line);
        free(line);
    }
    /* move this inside the loop to save history more often */
    linenoiseHistorySave(histpath);

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
        return interactive(&nstate);
    }

    return EXIT_SUCCESS;
}
