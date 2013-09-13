#include "nolli.h"
#include "symtable.h"

extern FILE* yyin;
extern int yylex(void);

extern int yydebug;
extern int yyparse();

symtable_t* type_table = NULL;

int main(void)
{
    yydebug = 1;
    yyin = stdin;

    /* int y; */
    /* while ((y = yylex()) > 0) { */
    /*     printf("%d\n", y); */
    /* } */

    type_table = symtable_create();
    yyparse();
    symtable_destroy(type_table);

    return EXIT_SUCCESS;
}
