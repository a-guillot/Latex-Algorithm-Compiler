
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

struct symtable * SYMTAB;
struct code * CODE;
struct code * FINDATA;

extern int yyparse();
