
#include "main.h"

int main()
{
  FILE *out = fopen("output.s", "w");
  SYMTAB = symtable_new();
  CODE = code_new();
  FINDATA = code_new();
  int r = yyparse();
  symtable_dump(SYMTAB);
  code_dump(CODE, out);
  code_dump(FINDATA, out);
  symtable_free(SYMTAB);
  code_free(CODE);
  return r;
}
