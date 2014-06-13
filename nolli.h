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

#include "debug.h"
#include "alloc.h"
#include "error.h"
#include "strtab.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "graph.h"
#include "type.h"
#include "symtable.h"
#include "analyze.h"

#if defined ( WIN32 )
#include "os.h"
#endif


#endif /* NOLLI_H */
