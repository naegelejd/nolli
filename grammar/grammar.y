%define parse.lac full
%define parse.error verbose

%{

#define YYDEBUG 1

void yyerror(const char *msg);

/* defined in generated lexer */
extern int yylex();
%}

%token BREAK CONTINUE RETURN
%token PACKAGE IMPORT FROM
%token DATA METHODS INTERFACE
%token WHILE FOR IN
%token IF ELSE
%token BOOL CHAR INT REAL STRING IDENT
%token INIT
%token EQ NEQ LTEQ GTEQ
%token VAR CONST
%token FUNC ALIAS
%token CALL

%left OR AND
%left EQ NEQ
%left '<' LTEQ '>' GTEQ
%left '+' '-'
%left '*' '/' '%'
%left '^'
%left '~'
%left PREF
%right INIT
%right '=' IADD ISUB IMUL IDIV IMOD IPOW ICMP
%right UPLUS UMINUS UNOT UAMP UAT
%left '(' ')'
%left '[' ']'
%left '{' '}'

%%

unit: package ';' definitions;

package: PACKAGE ident ;
definitions: definition | definitions definition ;
definition:
        import ';' |
        declaration ';' |
        data ';' |
        methods ';' |
        interface ';' |
        funcdef ';' |
        alias ';' ;

return: RETURN | RETURN expr;

import: IMPORT idents | FROM ident IMPORT idents;
idents: ident | idents ',' ident;

data: DATA ident '{' members '}' ;
members: member ';' | members member ';' ;
member : type idents ;

methods: METHODS ident '{' funcdefs '}' ;
funcdefs: funcdef ';' | funcdefs funcdef ';' ;
funcdef: funcdecl block ;
funcdecl: functype ident ;

interface: INTERFACE ident '{' funcdecls '}' ;
funcdecls: funcdecl ';' | funcdecls funcdecl ';' ;


declaration: declkind decl ;
declkind: VAR | CONST ;
decl: type decllist ;
decllist: declrhs | decllist ',' declrhs ;
declrhs: ident | ident '=' expr;

shortinit: ident INIT expr ;

loop: WHILE expr block | FOR ident IN expr block ;

cond: IF expr block | IF expr block ELSE block | IF expr block ELSE cond ;

block: '{' statements '}' ;

statements: statement | statements statement ;
statement:
        alias ';' |
        declaration ';' |
        shortinit ';' |
        assignment ';' |
        loop ';' |
        cond ';' |
        proccall ';' |
        BREAK ';' |
        CONTINUE ';' |
        return ';' ;

type: typename | compound ;
typename: ident | qualified ;
qualified: ident '.' ident ;
compound: listtype | maptype | functype ;
listtype: '[' type ']' ;
maptype: '{' type ',' type '}' ;

functype: FUNC returntype '(' params ')' ;
returntype: /* no return type */ | type ;
param: paramkind type paramrhs ;
paramkind: /* var */ | CONST ;
paramrhs: /* absent */ | declrhs;
params: /* nothing */ | param | params ',' param;

ident: IDENT

proccall: term '(' exprs ')' ;

alias: ALIAS type ident ;

assignment: term '=' expr |
            term IADD expr |
            term ISUB expr |
            term IMUL expr |
            term IDIV expr |
            term IMOD expr |
            term IPOW expr |
            term ICMP expr ;

term:   operand |
        term '.' ident |
        term '[' expr ']' |
        term '(' exprs ')' ;

expr: unexpr | binexpr ;
exprs: /* nothing */ | expr | exprs ',' expr;

unexpr: '-' %prec UMINUS unexpr | '!' %prec UNOT unexpr | term ;

binexpr: expr '+' unexpr |
        expr '-' unexpr |
        expr '*' unexpr |
        expr '/' unexpr |
        expr '%' unexpr |
        expr '^' unexpr |
        expr '~' unexpr |
        expr EQ unexpr |
        expr NEQ unexpr |
        expr '<' unexpr |
        expr LTEQ unexpr |
        expr '>' unexpr |
        expr GTEQ unexpr ;

operand: literal | ident | '(' expr ')' ;
literal: BOOL | CHAR | INT | REAL | STRING |
       listlit |
       maplit |
       funclit |
       structlit;
listlit: '[' exprs ']' ;
maplit: '{' pairs '}' ;
pair: expr ':' expr ;
pairs: /* nothing */ | pair | pairs ',' pair;

funclit: functype block ;

structlit: '&' %prec UAMP ident '{' structinits '}' ;
structinits: structinit | structinits ',' structinit ;
structinit: expr | ident ':' expr;

%%

int main(void)
{
    yydebug = 1;
    yyparse();
}

extern int line;
#include <stdio.h>
void yyerror(const char *msg)
{
    printf("%s @ line #%d\n", msg, line);
}

