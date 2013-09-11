%option noyywrap
/* %option debug */

%x string

%{

#include "parser.tab.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void comment(void);
extern void yyerror(const char *msg);

#define MAX_STR_CONST 128
char string_buffer[MAX_STR_CONST];
char *str_buf_ptr;


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

\'.\'   { return TOK_CHAR_LIT; }

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

and     { return TOK_AND; }
or      { return TOK_OR; }
not     { return TOK_NOT; }

{INTEGER}   { return TOK_INT_NUM; }

{REAL}  { return TOK_REAL_NUM; }

{WORD}  { return TOK_IDENT; }

{OP}    { return yytext[0]; }
{SYM}   { return yytext[0]; }

"/*"    { comment(); }
#.*\n   { /* ignore one-line comments */ }

{WS}    { /* ignore white space */ }

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
