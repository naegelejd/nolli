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

    astnode_t* root = NULL;
    yyparse(&root);
    if (root == NULL) {
        fprintf(stderr, "Nothing to compile\n");
        return EXIT_FAILURE;
    } else {
        assert(root);
        assert(root->type == AST_MODULE);
    }

    symtable_destroy(alias_table);
    symtable_destroy(class_table);

    return EXIT_SUCCESS;
}
