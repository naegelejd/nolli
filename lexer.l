%option noyywrap
%option reentrant
%option extra-type="nolli_state_t*"
/* %option debug */

%x string

%{

#include "nolli.h"
#include "grammar.h"

void yyerror(const char *msg);

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
SEMIS       ([;]{WS}?)+
COMMENT     \/\*.*\*\/|\/\/.*\n

%%

\"          {
                str_buf_ptr = string_buffer;
                BEGIN(string);
            }
<string>{
\"        {
                BEGIN(INITIAL);
                *str_buf_ptr = '\0';
                yyextra->cur_tok.s = strndup(string_buffer, MAX_STR_CONST);
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
struct  { return TOK_STRUCT; }
module  { return TOK_MODULE; }
import  { return TOK_IMPORT; }
typedef { return TOK_TYPEDEF; }
from    { return TOK_FROM; }
return  { return TOK_RETURN; }
break   { return TOK_BREAK; }
continue    { return TOK_CONTINUE; }

if      { return TOK_IF; }
else    { return TOK_ELSE; }
for     { return TOK_FOR; }
in      { return TOK_IN; }
while   { return TOK_WHILE; }
until   { return TOK_UNTIL; }

is      { return TOK_IS; }
and     { return TOK_AND; }
or      { return TOK_OR; }
not     { return TOK_NOT; }

true    { yyextra->cur_tok.b = true; return TOK_BOOL_LIT; }
false   { yyextra->cur_tok.b = false; return TOK_BOOL_LIT; }

\'.\'   { yyextra->cur_tok.c = yytext[0]; return TOK_CHAR_LIT; }

{INTEGER}   { yyextra->cur_tok.i = strtol(yytext, NULL, 0); return TOK_INT_NUM; }

{REAL}  { yyextra->cur_tok.r = strtod(yytext, NULL); return TOK_REAL_NUM;}

{WORD}  { yyextra->cur_tok.s = strdup(yytext); return TOK_IDENT; }

{COMMENT}   { /* ignore block comments */ }

{WS}    { /* ignore white space */ }

"=="    { return TOK_EQ; }
"<="    { return TOK_LTEQ; }
">="    { return TOK_GTEQ; }
"+="    { return TOK_IADD; }
"-="    { return TOK_ISUB; }
"*="    { return TOK_IMUL; }
"/="    { return TOK_IDIV; }
"^="    { return TOK_IPOW; }

"+"     { return TOK_PLUS; }
"-"     { return TOK_MINUS; }
"*"     { return TOK_ASTERISK; }
"/"     { return TOK_FSLASH; }
"%"     { return TOK_PERCENT; }
"^"     { return TOK_CARAT; }

"="     { return TOK_ASS; }
"<"     { return TOK_LT; }
">"     { return TOK_GT; }
"("     { return TOK_LPAREN; }
")"     { return TOK_RPAREN; }
"{"     { return TOK_LCURLY; }
"}"     { return TOK_RCURLY; }
"["     { return TOK_LBRACK; }
"]"     { return TOK_RBRACK; }
"."     { return TOK_DOT; }
","     { return TOK_COMMA; }
":"     { return TOK_COLON; }
    /* {SEMIS} { /1* ignore extra semicolons *1/ return TOK_SEMI; } */
";" /* ignore semicolons */

.       { yyerror("Invalid character\n"); }

<<EOF>> { yyterminate(); }


%%

void yyerror(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}
