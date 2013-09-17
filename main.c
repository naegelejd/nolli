#include "nolli.h"
#include "symtable.h"

extern FILE* yyin;
extern int yylex(void);

extern int yydebug;
extern int yyparse();

symtable_t* alias_table = NULL;
symtable_t* class_table = NULL;

int main(void)
{
    yydebug = 1;
    yyin = stdin;

    /* int y; */
    /* while ((y = yylex()) > 0) { */
    /*     printf("%d\n", y); */
    /* } */

    alias_table = symtable_create();
    class_table = symtable_create();
    yyparse();
    symtable_destroy(alias_table);
    symtable_destroy(class_table);

    return EXIT_SUCCESS;
}
