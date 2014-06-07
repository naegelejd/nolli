#ifndef NOLLI_H
#define NOLLI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <setjmp.h>

struct lexer;
struct parser;
struct root;

struct nolli_state {
    struct parser *parser;
    struct ast *root;
};

void nolli_init(struct nolli_state *state);

#include "debug.h"
#include "alloc.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "type.h"
#include "symtable.h"
#include "ast.h"
#include "ast_graph.h"
#include "walk.h"

#if defined ( WIN32 )
#include "os.h"
#endif


#endif /* NOLLI_H */
