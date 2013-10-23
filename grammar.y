%include { #include "nolli.h" }
%token_prefix TOK_
%name parse
%start_symbol module
%extra_argument { void* nstate }
%default_type { astnode_t* }
%token_type { token_t }
%type type { type_t* }
%type assign { assign_op_t }

%left OR AND.
%left IS EQ NEQ.
%left LT LTEQ GT GTEQ.
%left PLUS MINUS.
%left ASTERISK FSLASH PERCENT.
%right CARAT.
%right ASS.
%right IADD ISUB IMUL IDIV IPOW.
%right NOT.
%left LPAREN RPAREN.
%left LBRACK RBRACK.
%left LCURLY RCURLY.

module(M) ::= MODULE ident(I) statements(S). { M = ((nolli_state_t*)nstate)->ast_root = make_module(I, S); }

statements(SS) ::= statement(S).            { SS = S; }
statements(SS) ::= statements(L) statement(S).  { SS = make_statements(L, S); }

statement(S) ::= import(I).      { S = I; }
statement(S) ::= typedef(T).     { S = T; }
/* statement(S) ::= decl(D).        { S = D; } */
statement(S) ::= assignment(A).  { S = A; }
statement(S) ::= container_assignment(A). { S = A; }
statement(S) ::= ifelse(I).          { S = I; }
statement(S) ::= forloop(F).         { S = F; }
statement(S) ::= whileloop(W).       { S = W; }
statement(S) ::= call(C).        { S = C; }
statement(S) ::= RETURN expr(E).  { S = make_return(E); }
statement(S) ::= BREAK.      { S = make_break(); }
statement(S) ::= CONTINUE.   { S = make_continue(); }

import(I) ::= IMPORT idents(M).         { I = make_import(NULL, M); }
import(I) ::= FROM ident(M) IMPORT idents(C). { I = make_import(M, C); }

typedef(D) ::= TYPEDEF type(T) ident(I). { D = make_typedef(T, I); }

body(B) ::= LCURLY statements(S) RCURLY.      { B = S; }

forloop(F) ::= FOR ident(I) IN expr(E) body(B).  { F = make_for(I, E, B); }

whileloop(W) ::= WHILE expr(E) body(B).     { W = make_while(E, B); }
whileloop(W) ::= UNTIL expr(E) body(B).     { W = make_until(E, B); }

ifelse(IE) ::= IF expr(E) body(B).   { IE = make_ifelse(E, B, NULL); }
ifelse(IE) ::= IF expr(E) body(BA) ELSE body(BB).  { IE = make_ifelse(E, BA, BB); }
ifelse(IE) ::= IF expr(E) body(B) ELSE ifelse(N).    { IE = make_ifelse(E, B, N); }

call(C) ::= ident(F) LPAREN csvs(A) RPAREN.     { C = make_call(F, A); }
call(C) ::= container_access(F) LPAREN csvs(A) RPAREN.  { C = make_call(F, A); }
call(C) ::= member(F) LPAREN csvs(A) RPAREN.    { C = make_call(F, A); }
/* call(C) ::= expr LPAREN csvs RPAREN.    { C = NULL; } */

member(M) ::= ident(P) DOT ident(I).        { M = make_member(P, I); }
member(M) ::= member(P) DOT ident(I).       { M = make_member(P, I); }

list(L) ::= LBRACK csvs(C) RBRACK.    { L = C; }

csvs(C) ::= .                   { C = NULL; }
csvs(C) ::= expr(E).            { C = make_list(NULL, E); }
csvs(C) ::= csvs(L) COMMA expr(E).    { C = make_list(L, E); }

map(M) ::= LCURLY map_items(I) RCURLY.   { M = I; }

map_items(I) ::= map_keyval(KV).                  { I = make_map(NULL, KV); }
map_items(I) ::= map_items(M) COMMA map_keyval(KV).   { I = make_map(M, KV); }

map_keyval(KV) ::= expr(A) COLON expr(B).  { KV = make_mapkv(A, B); }

assignment(M) ::= ident(I) assign(A) expr(E).   { M = make_assignment(I, A, E); }
assignment(M) ::= decl(D) assign(A) expr(E).    { M = make_assignment(D, A, E); }

container_assignment(CA) ::= ident(I) container_index(X) assign(A) expr(E). {
        CA = make_contassign(I, X, A, E); }

assign(A) ::= ASS.     { A = ASS_DEF; }
assign(A) ::= IADD.    { A = ASS_ADD; }
assign(A) ::= ISUB.    { A = ASS_SUB; }
assign(A) ::= IMUL.    { A = ASS_DIV; }
assign(A) ::= IDIV.    { A = ASS_DIV; }
assign(A) ::= IPOW.    { A = ASS_POW; }

container_index(X) ::= LBRACK expr(E) RBRACK.    { X = E; }

container_access(A) ::= ident(I) container_index(X). { A = make_contaccess(I, X); }
container_access(A) ::= call(C) container_index(X). { A = make_contaccess(C, X); }

expr(E) ::= BOOL_LIT(B).    { E = make_bool_lit(B); }
expr(E) ::= CHAR_LIT(C).    { E = make_char_lit(C); }
expr(E) ::= INT_NUM(I).     { E = make_int_num(I); }
expr(E) ::= REAL_NUM(R).    { E = make_real_num(R); }
expr(E) ::= STR_LIT(S).     { E = make_str_lit(S); }
expr(E) ::= ident(I).           { E = I; }
expr(E) ::= expr(A) PLUS expr(B).   { E = make_binexpr(A, EXPR_ADD, B); }
expr(E) ::= expr(A) MINUS expr(B).  { E = make_binexpr(A, EXPR_SUB, B); }
expr(E) ::= expr(A) ASTERISK expr(B).  { E = make_binexpr(A, EXPR_MUL, B); }
expr(E) ::= expr(A) FSLASH expr(B). { E = make_binexpr(A, EXPR_DIV, B); }
expr(E) ::= expr(A) PERCENT expr(B). { E = make_binexpr(A, EXPR_MOD, B); }
expr(E) ::= expr(A) CARAT expr(B).  { E = make_binexpr(A, EXPR_POW, B); }
expr(E) ::= expr(A) LT expr(B).     { E = make_binexpr(A, EXPR_LT, B); }
expr(E) ::= expr(A) GT expr(B).     { E = make_binexpr(A, EXPR_GT, B); }
expr(E) ::= expr(A) LTEQ expr(B).   { E = make_binexpr(A, EXPR_LTEQ, B); }
expr(E) ::= expr(A) GTEQ expr(B).   { E = make_binexpr(A, EXPR_GTEQ, B); }
expr(E) ::= expr(A) EQ expr(B).     { E = make_binexpr(A, EXPR_EQ, B); }
expr(E) ::= expr(A) IS expr(B).     { E = make_binexpr(A, EXPR_IS, B); }
expr(E) ::= expr(A) AND expr(B).    { E = make_binexpr(A, EXPR_AND, B); }
expr(E) ::= expr(A) OR expr(B).     { E = make_binexpr(A, EXPR_OR, B); }
expr(E) ::= MINUS expr(A). [NOT]    { E = make_unexpr(EXPR_NEG, A); }
expr(E) ::= NOT expr(A).            { E = make_unexpr(EXPR_NOT, A); }
expr(E) ::= LPAREN expr(A) RPAREN.  { E = A; }
expr(E) ::= container_access(C).    { E = C; }
expr(E) ::= list(L).                { E = L; }
expr(E) ::= map(M).                 { E = M; }
expr(E) ::= call(C).                { E = C; }
expr(E) ::= member(M).              { E = M; }
expr(E) ::= body(F).            { E = F; }

ident(I) ::= IDENT(S).    { I = make_ident(S); }

idents(IS) ::= ident.    { IS = NULL; }
idents(IS) ::= idents COMMA ident.    { IS = NULL; }

decl(D) ::= type(T) ident(I).    { D = make_decl(T, I); }

type(T) ::= BOOL.    { T = &bool_type; }
type(T) ::= CHAR.    { T = &char_type; }
type(T) ::= INT.     { T = &int_type; }
type(T) ::= REAL.    { T = &real_type; }
type(T) ::= STR.     { T = &str_type; }
type(T) ::= FILE.    { T = &file_type; }
type(T) ::= LIST LT type(A) GT.             { T = new_list_type(A); }
type(T) ::= MAP LT type(A) COMMA type(B) GT.        { T = new_map_type(A, B); }
type(T) ::= IDENT(S).   { T = new_user_type(S.s); }
type(T) ::= functype(F).   { T = F; }
type(T) ::= STRUCT.  { T = NULL; }

functype(T) ::= FUNC type LPAREN params RPAREN.    { T = NULL; }
functype(T) ::= FUNC LPAREN params RPAREN.         { T = NULL; }
/* functype(T) ::= FUNC type.    { T = NULL; } */
/* functype(T) ::= FUNC.         { T = NULL; } */

params(P) ::= .                         { P = NULL; }
params(P) ::= decl.                     { P = NULL; }
params(P) ::= params COMMA decl.        { P = NULL; }


%syntax_error {
    NOLLI_ERROR("%s:\n", "Syntax Error");
    size_t n = sizeof(yyTokenName) / sizeof(yyTokenName[0]);
    unsigned int i = 0;
    for (i = 0; i < n; ++i) {
        int a = yy_find_shift_action(yypParser, (YYCODETYPE)i);
        if (a < YYNSTATE + YYNRULE) {
            NOLLI_ERROR("\tpossible token: %s\n", yyTokenName[i]);
        }
    }
}

%parse_accept {
    NOLLI_DEBUG("%s\n", "Parse succeeded!");
}

%parse_failure {
    NOLLI_ERROR("%s\n", "Giving up. Parser is hopelessly lost...\n");
}

%stack_overflow {
    NOLLI_ERROR("%s\n", "Giving up. Parser stack overflow...\n");
}

/* vim: set ft=lemon syn=lemon: */
