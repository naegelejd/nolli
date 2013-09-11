%{

#include <stdio.h>
#include <stdlib.h>

#define YYDEBUG 1

#define WRITE(...) \
    do { \
        fprintf(stdout, __VA_ARGS__); \
    } while (0)

void yyerror(const char *msg);

/* defined in generated lexer */
extern int yylex();

%}

%error-verbose

%start module

%token TOK_CHAR TOK_INT TOK_REAL TOK_STR
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
    |   TOK_MODULE TOK_IDENT
    |   TOK_MODULE TOK_IDENT statements
    ;

statements:
        statement
    |   statements statement
    ;

statement:
        TOK_TYPEDEF type TOK_IDENT
    |   decl ';'
    |   expr ';'
    |   assignment ';'
    |   container_assignment ';'
    |   function
    |   class
    |   ';'
    ;

class:
        TOK_CLASS TOK_IDENT '=' '{' class_members '}'
    |   TOK_CLASS TOK_IDENT '(' type ')' '=' '{' class_members '}'
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
    ;

list:
        '[' csvs ']'
    ;

csvs:
        /* nothing */
    |   expr
    |   csvs ',' expr
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
        expr container_index
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
    |   list
    |   call
    ;

decl:
    type TOK_IDENT
    ;

type:
        TOK_CHAR
    |   TOK_INT
    |   TOK_REAL
    |   TOK_LIST '<' type '>'
    |   TOK_STR
    |   TOK_MAP '<' type ',' type '>'
    |   TOK_FILE
    |   TOK_FUNC
    |   TOK_CLASS
    |   TOK_MODULE
    ;

%%

//extern void yyrestart();
extern FILE *yyin;

void yyerror(const char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(EXIT_FAILURE);
}
