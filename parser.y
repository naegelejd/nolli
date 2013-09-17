%{

#include "nolli.h"

#define YYDEBUG 1

#define WRITE(...) \
    do { \
        fprintf(stdout, __VA_ARGS__); \
    } while (0)

void yyerror(const char *msg);

/* defined in generated lexer */
extern int yylex();

extern symtable_t* alias_table;
extern symtable_t* class_table;

%}

%error-verbose

%union {
    int t;
    bool b;
    char c;
    long i;
    double r;
    char* s;
    struct astnode* n;
}

%start module

%type <b> TOK_BOOL_LIT
%type <c> TOK_CHAR_LIT
%type <i> TOK_INT_NUM
%type <r> TOK_REAL_NUM
%type <s> TOK_IDENT TOK_INST TOK_ALIAS TOK_STR_LIT

%type <t> assign

%type <n> ident decl expr assignment
%type <n> ifelse whileloop forloop
%type <n> container_index container_access container_assignment
%type <n> map map_items map_keyval list csvs
%type <n> body statement statements module

%token TOK_ALIAS TOK_INST TOK_BOOL TOK_CHAR TOK_INT TOK_REAL TOK_STR
%token TOK_LIST TOK_MAP TOK_FILE TOK_FUNC TOK_CLASS TOK_MODULE
%token TOK_IF TOK_ELSE TOK_FOR TOK_WHILE TOK_UNTIL
%token TOK_RETURN TOK_TYPEDEF
%token TOK_IN
%token TOK_BOOL_LIT TOK_CHAR_LIT TOK_INT_NUM TOK_REAL_NUM TOK_IDENT TOK_STR_LIT

%token ',' ':'

%left TOK_OR TOK_AND
%left TOK_IS TOK_EQ TOK_NEQ
%left '<' TOK_LTEQ '>' TOK_GTEQ
%left '+' '-'
%left '*' '/' '%'
%left '^'
%right '='
%right TOK_IADD TOK_ISUB TOK_IMUL TOK_IDIV TOK_IPOW
%right TOK_NOT
%right UMINUS UNOT
%left '(' ')'
%left '[' ']'
%left '{' '}'

%%

module:
        /* %empty */    { $$ = NULL; }
    |   TOK_MODULE ident ';' statements     { $$ = make_module($2, $4); }
    |   statements      { $$ = make_module(NULL, $1); }
    ;

statements:
        statement               { $$ = $1; }
    |   statements statement    { $$ = make_statements($1, $2); }
    ;

statement:
        TOK_TYPEDEF type TOK_IDENT ';' { symtable_add(alias_table, $3, NULL); $$ = NULL; }
    |   decl ';'        { $$ = $1; }
    |   function        { $$ = NULL; }
    |   class           { $$ = NULL; }
    |   ifelse          { $$ = $1; }
    |   forloop         { $$ = $1; }
    |   whileloop       { $$ = $1; }
    |   call ';'        { $$ = NULL; }
    |   container_assignment ';' { $$ = NULL; }
    |   assignment ';'  { $$ = $1; }
    ;

class:
        TOK_CLASS TOK_IDENT '=' '{' class_members '}'
            { symtable_add(class_table, $2, NULL); }
    |   TOK_CLASS TOK_IDENT '(' type ')' '=' '{' class_members '}'
            { symtable_add(class_table, $2, NULL); }
    ;

class_members:
        class_member
    |   class_members class_member
    ;

class_member:
        function
    |   decl
    ;

function:
        TOK_FUNC ident '=' function_definition
    |   TOK_FUNC ident '(' params ')' '=' function_definition
    |   TOK_FUNC ident '(' params ')' ':' type '=' function_definition
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
    |   params ',' decl
    ;

body:
        '{' statements '}'      { $$ = $2; }
    ;

forloop:
        TOK_FOR ident TOK_IN expr body    { $$ = make_for($2, $4, $5); }
    ;

whileloop:
        TOK_WHILE expr body     { $$ = make_while($2, $3); }
    |   TOK_UNTIL expr body     { $$ = make_until($2, $3); }
    ;

ifelse:
        TOK_IF expr body                { $$ = make_ifelse($2, $3, NULL); }
    |   TOK_IF expr body TOK_ELSE body  { $$ = make_ifelse($2, $3, $5); }
    |   TOK_IF expr body TOK_ELSE ifelse    { $$ = make_ifelse($2, $3, $5); }
    ;

call:
        ident '(' csvs ')'
    |   TOK_INST '(' csvs ')'   /* constructor */
    |   container_access '(' csvs ')'
    |   member '(' csvs ')'
    ;

member:
      ident '.' ident
    | member '.' ident
    ;

list:
        '[' csvs ']'    { $$ = $2; }
    ;

csvs:
        /* nothing */   { $$ = NULL; }
    |   expr            { $$ = make_list(NULL, $1); }
    |   csvs ',' expr   { $$ = make_list($1, $3); }
    ;

map:
        '{' map_items '}'   { $$ = $2; }
    ;

map_items:
        map_keyval                  { $$ = make_map(NULL, $1); }
    |   map_items ',' map_keyval    { $$ = make_map($1, $3); }
    ;

map_keyval:
        expr ':' expr       { $$ = make_mapkv($1, $3); }
    ;

assignment:
        decl assign expr    { $$ = make_assignment($1, $2, $3); }
    |   ident assign expr   { $$ = make_assignment($1, $2, $3); }
    ;

container_assignment:
        ident container_index assign expr { $$ = make_contassign($1, $2, $3, $4); }
    ;

assign:
        '='         { $$ = ASS_DEF; }
    |   TOK_IADD    { $$ = ASS_ADD; }
    |   TOK_ISUB    { $$ = ASS_SUB; }
    |   TOK_IMUL    { $$ = ASS_DIV; }
    |   TOK_IDIV    { $$ = ASS_DIV; }
    |   TOK_IPOW    { $$ = ASS_POW; }
    ;

container_index:
        '[' expr ']'    { $$ = $2; }
    ;

container_access:
        ident container_index   { $$ = make_contaccess($1, $2); }
    |   call container_index    { $$ = make_contaccess(NULL, $2); }
    ;

expr:
        TOK_BOOL_LIT    { $$ = make_bool_lit($1); }
    |   TOK_CHAR_LIT    { $$ = make_char_lit($1); }
    |   TOK_INT_NUM     { $$ = make_int_num($1); }
    |   TOK_REAL_NUM    { $$ = make_real_num($1); }
    |   TOK_STR_LIT     { $$ = make_str_lit($1); }
    |   ident           { $$ = $1; }
    |   expr '+' expr   { $$ = make_binexpr(EXPR_ADD, $1, $3); }
    |   expr '-' expr   { $$ = make_binexpr(EXPR_SUB, $1, $3); }
    |   expr '*' expr   { $$ = make_binexpr(EXPR_MUL, $1, $3); }
    |   expr '/' expr   { $$ = make_binexpr(EXPR_DIV, $1, $3); }
    |   expr '^' expr   { $$ = make_binexpr(EXPR_POW, $1, $3); }
    |   expr '<' expr   { $$ = make_binexpr(EXPR_LT, $1, $3); }
    |   expr '>' expr   { $$ = make_binexpr(EXPR_GT, $1, $3); }
    |   expr TOK_LTEQ expr   { $$ = make_binexpr(EXPR_LTEQ, $1, $3); }
    |   expr TOK_GTEQ expr   { $$ = make_binexpr(EXPR_GTEQ, $1, $3); }
    |   expr TOK_EQ expr   { $$ = make_binexpr(EXPR_EQ, $1, $3); }
    |   expr TOK_IS expr   { $$ = make_binexpr(EXPR_IS, $1, $3); }
    |   expr TOK_AND expr   { $$ = make_binexpr(EXPR_AND, $1, $3); }
    |   expr TOK_OR expr   { $$ = make_binexpr(EXPR_OR, $1, $3); }
    |   '-' expr %prec UMINUS   { $$ = make_unexpr(EXPR_NEG, $2); }
    |   TOK_NOT expr %prec UNOT   { $$ = make_unexpr(EXPR_NOT, $2); }
    |   '(' expr ')'        { $$ = $2; }
    |   container_access    { $$ = NULL; }
    |   list            { $$ = NULL; }
    |   map             { $$ = NULL; }
    |   call            { $$ = NULL; }
    ;

ident: TOK_IDENT    { $$ = make_ident($1); };

decl:
    type ident      { $$ = make_decl(0, $2); }
    ;

type:
        TOK_ALIAS
    |   TOK_INST
    |   TOK_BOOL
    |   TOK_CHAR
    |   TOK_INT
    |   TOK_REAL
    |   TOK_STR
    |   TOK_LIST '<' type '>'
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
