#ifndef NOLLI_H
#define NOLLI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "debug.h"
#include "alloc.h"
#include "token.h"
#include "type.h"
#include "symtable.h"
#include "ast.h"

typedef struct nolli_state {

    astnode_t* ast_root;
    token_t cur_tok;

} nolli_state_t;

#endif /* NOLLI_H */
