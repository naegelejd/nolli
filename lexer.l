%option noyywrap
/* %option debug */

%x string

%{

#include "nolli.h"
#include "parser.tab.h"

void comment(void);
extern void yyerror(const char *msg);

#define MAX_STR_CONST 128
char string_buffer[MAX_STR_CONST];
char *str_buf_ptr;

extern symtable_t* alias_table;
extern symtable_t* class_table;

%}

D           [0-9]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+

INTEGER     {D}+|0[xX]{H}+
REAL        {D}+{E}|{D}*\.{D}+({E})?|{D}+\.{D}*({E})?
WORD        [a-zA-Z_][a-zA-Z_0-9]*
WS          [ \t\f\r\n]+
OP          [\+\-\*\/\%\^]
SYM         [\=\<\>\(\)\{\}\[\]\.\,\;\:\'\"]
SEMIS       ([;]{WS}?)+

%%

\"          {
                str_buf_ptr = string_buffer;
                BEGIN(string);
            }
<string>{
\"        {
                BEGIN(INITIAL);
                *str_buf_ptr = '\0';
                /* yylval.string_v = strndup(string_buffer, MAX_STR_CONST); */
                return TOK_STR_LIT;
            }
\n        { yyerror("Unterminated string literal"); }
\\n       { *str_buf_ptr++ = '\n'; }
\\t       { *str_buf_ptr++ = '\t'; }
\\r       { *str_buf_ptr++ = '\r'; }
\\b       { *str_buf_ptr++ = '\b'; }
\\f       { *str_buf_ptr++ = '\f'; }
\\(.|\n)      { *str_buf_ptr++ = yytext[1]; }
[^\\\n\"]+    {
                    char *sptr = yytext;
                    while (*sptr) {
                        *str_buf_ptr++ = *sptr++;
                    }
                }
} /* <string> */

bool    { return TOK_BOOL; }
char    { return TOK_CHAR; }
int     { return TOK_INT; }
real    { return TOK_REAL; }
str     { return TOK_STR; }
list    { return TOK_LIST; }
map     { return TOK_MAP; }
file    { return TOK_FILE; }
func    { return TOK_FUNC; }
class   { return TOK_CLASS; }
module  { return TOK_MODULE; }
return  { return TOK_RETURN; }
typedef { return TOK_TYPEDEF; }

if      { return TOK_IF; }
else    { return TOK_ELSE; }
for     { return TOK_FOR; }
in      { return TOK_IN; }
while   { return TOK_WHILE; }
until   { return TOK_UNTIL; }

"=="    { return TOK_EQ; }
"<="    { return TOK_LTEQ; }
">="    { return TOK_GTEQ; }
is      { return TOK_IS; }
and     { return TOK_AND; }
or      { return TOK_OR; }
not     { return TOK_NOT; }

"+="    { return TOK_IADD; }
"-="    { return TOK_ISUB; }
"*="    { return TOK_IMUL; }
"/="    { return TOK_IDIV; }
"^="    { return TOK_IPOW; }

true    { yylval.b = true; return TOK_BOOL_LIT; }
false   { yylval.b = false; return TOK_BOOL_LIT; }

\'.\'   { yylval.c = yytext[0]; return TOK_CHAR_LIT; }

{INTEGER}   { yylval.i = strtol(yytext, NULL, 0); return TOK_INT_NUM; }

{REAL}  { yylval.r = strtod(yytext, NULL); return TOK_REAL_NUM;}

{WORD}  {
    yylval.s = strdup(yytext);
    if (symtable_contains(alias_table, yytext)) {
        return TOK_ALIAS;
    } else if (symtable_contains(class_table, yytext)) {
        return TOK_INST;
    } else {
        return TOK_IDENT;
    }
}

"/*"    { comment(); }
"//".*\n    { /* ignore one-line comments */ }

{WS}    { /* ignore white space */ }

{SEMIS} { /* ignore extra semicolons */ return ';'; }

{OP}    { return yytext[0]; }
{SYM}   { return yytext[0]; }

.       { yyerror("Invalid character\n"); }

<<EOF>> { yyterminate(); }


%%

void comment(void)
{
    char c, c1;

loop:
    while ((c = input()) != '*' && c != 0) {
        ;
    }

    if ((c1 = input()) != '/' && c != 0) {
        unput(c1);
        goto loop;
    }

}
