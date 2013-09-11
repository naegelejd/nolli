#include <stdlib.h>
#include <stdio.h>

extern FILE* yyin;
extern int yylex(void);

extern int yydebug;
extern int yyparse();

int main(void)
{
    yydebug = 1;
    yyin = stdin;

    /* int y; */
    /* while ((y = yylex()) > 0) { */
    /*     printf("%d\n", y); */
    /* } */

    yyparse();

    return EXIT_SUCCESS;
}
