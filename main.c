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

#ifdef WIN32
    char *homedrive = getenv("HOMEDRIVE");
    char *homepath = getenv("HOMEPATH");
    char *home = os.path.join(homedrive, homepath);
#else
    char *home = getenv("HOME");
#endif

    char *histpath = os.path.join(home, histfile);

    return histpath;
}

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

    const char *normprompt = "nl> ";
    const char *contprompt = "nl----> ";
    const char *prompt = normprompt;

    size_t bufcap = 1024, buflen = 0;
    char *buffer = nalloc(bufcap);
    char *line = NULL;
    /* read lines until EOF */
    while ((line = linenoise(prompt)) != NULL) {
        size_t len = strlen(line) + 1; /* add 1 for newline */

        /* if the line is empty, read another one */
        if (len < 2) {
            continue;
        }

        /* expand the buffer as necessary */
        while (bufcap < buflen + len) {
            bufcap *= 2;
            buffer = nrealloc(buffer, bufcap);
        }
        strcat(buffer, line);
        strcat(buffer, "\n");
        buflen += len;
        buffer[buflen] = 0;

        int err = parse_line(nstate, buffer);

        if (err == ERR_EOF) {
            prompt = contprompt;
        } else {
            memset(buffer, 0, buflen); /* buffer[0] = 0; */
            buflen = 0;
            linenoiseHistoryAdd(line);
            prompt = normprompt;
        }
    }

    /* move this inside the loop to save history more often */
    linenoiseHistorySave(histpath);

    free(buffer);

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
