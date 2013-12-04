# Grammar

## TYPE
    'bool' | 'char' | 'int' | 'real' | 'string' |
        '(' TYPE { ',' TYPE } ') |
        '[' TYPE ']' |
        '{' TYPE ',' TYPE '}'

## DECL
    TYPE IDENT

## LITERAL
    boolean, i.e. 'true' | 'false'
    single-quoted char, e.g. 'c'
    any integer, e.g. 123
    any real number, e.g. 3.14
    double-quoted string, e.g. "hello world"
    tuple, i.e. '(' EXPR {',' EXPR} ')'
    list, i.e. '[' EXPR {',' EXPR} ']'
    map, i.e. '{' EXPR ':' EXPR {',' EXPR ':' EXPR} ']'

## EXPR
    LITERAL
    IDENT
    '(' IDENT ')'
    BINARYOP
    UNARYOP
    CALL

## UNARYOP
    '-' EXPR
    '!' EXPR

## BINARYOP
    EXPR '+' EXPR
    EXPR '-' EXPR
    EXPR '*' EXPR
    EXPR '/' EXPR
    EXPR '%' EXPR
    EXPR '^' EXPR
    EXPR '==' EXPR
    EXPR '!=' EXPR
    EXPR '<' EXPR
    EXPR '<=' EXPR
    EXPR '>' EXPR
    EXPR '>=' EXPR

## ASSIGN
    ':=' | '+=' | '-=' | '*=' | '/=' | '%=' | '^='

## ASSIGNMENT
    IDENT ASSIGN EXPR | DECL ASSIGN EXPR

## LOOP
    'for' IDENT in EXPR BLOCK |
    'while' EXPR BLOCK |
    'until' EXPR BLOCK

## COND
    'if' EXPR BLOCK ['else' BLOCK]

## BLOCK
    STATEMENT | '{' STATEMENTS '}'

## FUNCDEF
  'func' IDENT '(' CSVS ')' BLOCK

## CSVS
  DECL {',' DECL}

## STATEMENT
    DECL
    ASSIGNMENT
    CALL
    FUNCDEF
    COND
    LOOP

## STATEMENTS
    {STATEMENT}


## MODULE
    'module' IDENT

## IMPORT
    'import' IDENT ['as' IDENT] {',' IDENT ['as' IDENT]}
    'from' IDENT 'import' IDENT ['as' IDENT] {',' IDENT ['as' IDENT]}

## GLOBALCONSTANTS
    'const' DECL ['=' LITERAL]

## GLOBALVARS
    DECL ['=' LITERAL]

## STRUCT
    'struct' IDENT '{' {TYPE ['=' LITERAL]} '}'

## INTERFACE
    'interface' IDENT '{' {FUNCDECL} '}'

# FILE
    MODULE [{TOP_LEVEL_CONSTRUCT}]

# TOPLEVELCONSTRUCT
    IMPORT | GLOBALCONSTANTS | GLOBALVARS | STRUCT | INTERFACE | FUNCDEF


# OLD

## example types

int
real
str
list<int>
map<str, int>
func int (real, real)
struct

## example keywords

if
else
while
until
for
typedef

## declarations

int i
real r
str s
list<int> li
map<str, int> m
func int (real, real) lcd
struct Vehicle

## definitions

int i = 1
real r = 3.14
str s = "hello"
list<int> li = [1, 2, 3, 4]
map<str, int> m = {"hello":0, "world":1"}
func int (real, real) lcd = { 

## functions

func int (str s) main { ... }     FUNC type (decl...) ident funcbody
typedef func int (str) main_t;    TYPEDEF FUNC type (type...) ident
main_t other = main;              ident ident = ident;
func int (str) another = main;    FUNC type (type...) ident ASSIGN ident
main = func int (str s) main { ... }  ident = FUNC type (decl...) ident funcbody

### definition

type: func int (str)

func int (str s) main = { print(s); };
func int (str s) main1; main1 = main;

func () (func int (str) set_callback = { 

func int (str) main = |s| { untile i = 0 { print(s); i-= 1; } };


func int (str args) main =
{

}

func register (func int (str) cb) =
{

}

typedef func int (str args) callback;
