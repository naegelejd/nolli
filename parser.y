%{

#include "nolli.h"
#include "symtable.h"

#define YYDEBUG 1

#define WRITE(...) \
    do { \
        fprintf(stdout, __VA_ARGS__); \
    } while (0)

void yyerror(const char *msg);

/* defined in generated lexer */
extern int yylex();

extern symtable_t* type_table;

%}

%error-verbose

%union {
    long p_long_t;
    double p_real_t;
    char* p_str_t;
}

%start module

%type <p_str_t> TOK_IDENT

%token TOK_TYPE TOK_CHAR TOK_INT TOK_REAL TOK_STR
%token TOK_LIST TOK_MAP TOK_FILE TOK_FUNC TOK_CLASS TOK_MODULE
%token TOK_RETURN TOK_TYPEDEF

%token TOK_CHAR_LIT TOK_INT_NUM TOK_REAL_NUM TOK_IDENT TOK_STR_LIT

%token ',' ':'

%left TOK_OR TOK_AND
%left TOK_EQ TOK_NEQ
%left '<' TOK_LTEQ '>' TOK_GTEQ
%left '+' '-'
%left '*' '/' '%'
%left '^'
%right '='
%right TOK_NOT
%right UMINUS ULGNOT
%left '(' ')'
%left '[' ']'
%left '{' '}'

%%

module:
        /* %empty */
    |   TOK_MODULE TOK_IDENT ';' statements
    |   statements
    ;

statements:
        statement
    |   statements statement
    ;

statement:
        TOK_TYPEDEF type TOK_IDENT { symtable_add(type_table, $3, NULL); }
    |   decl ';'
    |   function
    |   class
    |   call ';'
    |   container_assignment ';'
    |   assignment ';'
    |   ';'
    ;

class:
        TOK_CLASS TOK_IDENT '=' '{' class_members '}'
            { symtable_add(type_table, $2, NULL); }
    |   TOK_CLASS TOK_IDENT '(' type ')' '=' '{' class_members '}'
            { symtable_add(type_table, $2, NULL); }
    ;

class_members:
        class_member
    |   class_member class_members
    ;

class_member:
        function
    |   decl
    ;

function:
        TOK_FUNC TOK_IDENT '=' function_definition
    |   TOK_FUNC TOK_IDENT '(' params ')' '=' function_definition
    |   TOK_FUNC TOK_IDENT '(' params ')' ':' type '=' function_definition
    ;

function_definition:
        '{' function_statements '}'
    ;

function_statements:
        statement
    |   TOK_RETURN expr ';'
    ;

params:
        /* nothing */
    |   decl
    |   decl ',' params
    ;

call:
        TOK_IDENT '(' csvs ')'
    |   container_access '(' csvs ')'
    |   member '(' csvs ')'
    ;

member:
      TOK_IDENT '.' TOK_IDENT
    | member '.' TOK_IDENT
    ;

list:
        '[' csvs ']'
    ;

csvs:
        /* nothing */
    |   expr
    |   csvs ',' expr
    ;

map:
        '{' map_items '}'
    ;

map_items:
        /* nothing */
    | map_keyval
    | map_items ',' map_keyval;
    ;

map_keyval:
        expr ':' expr
    ;

assignment:
        decl '=' expr
    |   TOK_IDENT '=' expr
    ;

container_assignment:
        TOK_IDENT container_index '=' expr
    ;

container_index:
        '[' expr ']'
    ;

container_access:
        TOK_IDENT container_index
    |   call container_index
    ;

expr:
        TOK_CHAR_LIT
    |   TOK_INT_NUM
    |   TOK_REAL_NUM
    |   TOK_STR_LIT
    |   TOK_IDENT
    |   expr '+' expr
    |   expr '-' expr
    |   expr '*' expr
    |   expr '/' expr
    |   '(' expr ')'
    |   container_access
    |   list
    |   map
    |   call
    ;

decl:
    type TOK_IDENT
    ;

type:
        TOK_TYPE
    |   TOK_CHAR
    |   TOK_INT
    |   TOK_REAL
    |   TOK_LIST '<' type '>'
    |   TOK_STR
    |   TOK_MAP '<' type ',' type '>'
    |   TOK_FILE
    ;

%%

//extern void yyrestart();
extern FILE *yyin;

void yyerror(const char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(EXIT_FAILURE);
}
