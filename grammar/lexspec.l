%option noyywrap
/* %option debug */

%{

#include "grammar.tab.h"

extern void yyerror(const char *msg);

int line = 1;

int lasttok;

#define YIELD(x) lasttok = x; return x

static const unsigned int MAX_STR_CONST = 1024;
char string_buffer[MAX_STR_CONST];
char *str_buf_ptr;

%}

%x str

D           [0-9]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+

CHAR        '.'
INTEGER     {D}+|0[xX]{H}+
REAL        {D}+{E}|{D}*\.{D}+({E})?|{D}+\.{D}*({E})?
WORD        [a-zA-Z_$][a-zA-Z_0-9]*
WS          [ \t\f\r]+

%%

\"          { str_buf_ptr = string_buffer; BEGIN(str); }
<str>{
\"          {
                BEGIN(INITIAL);
                *str_buf_ptr = '\0';
                /* yylval.string_v = strndup(string_buffer, */
                /*         strlen(string_buffer)); */
                YIELD(STRING);
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
} /* <str> */


"package"       { YIELD(PACKAGE); }
"import"        { YIELD(IMPORT); }
"from"          { YIELD(FROM); }
"data"          { YIELD(DATA); }
"methods"       { YIELD(METHODS); }
"interface"     { YIELD(INTERFACE); }
"var"           { YIELD(VAR); }
"const"         { YIELD(CONST); }
"alias"         { YIELD(ALIAS); }
"while"         { YIELD(WHILE); }
"for"           { YIELD(FOR); }
"in"            { YIELD(IN); }
"if"            { YIELD(IF); }
"else"          { YIELD(ELSE); }
"func"          { YIELD(FUNC); }
"break"         { YIELD(BREAK); }
"continue"      { YIELD(CONTINUE); }
"return"        { YIELD(RETURN); }

"true"          { /* yylval.bool_v = true; */ YIELD(BOOL); }
"false"         { /* yylval.bool_v = false; */ YIELD(BOOL); }
{CHAR}          { YIELD(CHAR); }
{INTEGER}       { /* yylval.int_v = strtol(yytext, NULL, 0); */ YIELD(INT); }
{REAL}          { /* yylval.float_v = strtod(yytext, NULL); */ YIELD(REAL); }
{WORD}          { /* yylval.string_v = strdup(yytext); */ YIELD(IDENT); }

":="    { YIELD(INIT); }
"+="    { YIELD(IADD); }
"-="    { YIELD(ISUB); }
"*="    { YIELD(IMUL); }
"/="    { YIELD(IDIV); }
"%="    { YIELD(IMOD); }
"^="    { YIELD(IPOW); }
"~="    { YIELD(ICMP); }

"=="    { YIELD(EQ); }
"!="    { YIELD(NEQ); }
"<="    { YIELD(LTEQ); }
">="    { YIELD(GTEQ); }
"="     { YIELD('='); }
"<"     { YIELD('<'); }
">"     { YIELD('>'); }

"||"    { YIELD(OR); }
"&&"    { YIELD(AND); }
"!"     { YIELD('!'); }

"{"     { YIELD('{'); }
"}"     { YIELD('}'); }
"["     { YIELD('['); }
"]"     { YIELD(']'); }
"("     { YIELD('('); }
")"     { YIELD(')'); }

":"     { YIELD(':'); }
";"     { YIELD(';'); }
","     { YIELD(','); }
"."     { YIELD('.'); }
"&"     { YIELD('&'); }
"@"     { YIELD('@'); }

"-"     { YIELD('-'); }
"+"     { YIELD('+'); }
"*"     { YIELD('*'); }
"/"     { YIELD('/'); }
"%"     { YIELD('%'); }
"^"     { YIELD('^'); }
"~"     { YIELD('~'); }

\n              {
                    line++;
                    switch (lasttok) {
                        case IDENT: case BOOL: case CHAR: case INT:
                        case REAL: case STRING: case ')': case '}': case ']':
                        case RETURN: case BREAK: case CONTINUE:
                            YIELD(';');
                            break;
                        default:
                            break;

                    }
                }

#.*             { /* ignore one-line comments */ }

{WS}+           /* ignore whitespace */

.       { yyerror("Invalid character"); }

<<EOF>> { yyterminate(); }
