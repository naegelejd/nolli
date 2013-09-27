#include "nolli.h"
#include "lexer.yy.h"
#include "grammar.h"

/* lemon-generated parser function prototypes */
void* parseAlloc(void*(*m)(size_t));
void parseFree(void*, void (*f)(void*));
void parse(void*, int, token_t, nolli_state_t*);
void parseTrace(FILE*, char*);

int main(void)
{
    nolli_state_t nstate;
    memset(&nstate, 0, sizeof(nstate));

    yyscan_t scanner;
    yylex_init_extra(&nstate, &scanner);
    yyset_in(stdin, scanner);

    parseTrace(stdout, "parser: ");
    void* parser = parseAlloc(malloc);

    int tok = 0;
    do {
        tok = yylex(scanner);
        parse(parser, tok, nstate.cur_tok, &nstate);
    } while (tok > 0);

    if (nstate.ast_root == NULL) {
        fprintf(stderr, "Nothing to compile\n");
        return EXIT_FAILURE;
    } else {
        assert(nstate.ast_root);
        assert(nstate.ast_root->type == AST_MODULE);
        printf("AST is ready to go!\n");
    }

    parseFree(parser, free);
    yylex_destroy(scanner);

    return EXIT_SUCCESS;
}
