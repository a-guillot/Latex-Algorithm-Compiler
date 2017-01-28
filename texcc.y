%{

#include "main.h"

extern void yyerror(const char * s);
extern int yylex();

int strnum = 0;
int floatnum = 0;

%}

%token BLANK INPUT OUTPUT LOCAL GLOBAL IN INT REAL BOOLEAN EMPTY_SET ENDL NEQ GEQ LEQ AFF NOT MBOX IF ELIF WHILE FOR TO AND OR TRUE FALSE PRINTINT PRINTREAL PRINTTEXT START END
%token <intval> NUMBER
%token <strval> VARNAME
%token <floval> DECIMAL
%token <stringval> STRING

%union {
    long int intval;
    name_t strval;
    string stringval;
    float floval;
    struct {
        struct symbol * ptr;
    } exprval;
    struct {
        struct typedec * td;
    } type_dec;
}

%left OR
%left AND
%left '=' NEQ
%left '<' '>' LEQ GEQ
%left '+' '-'
%left '*' '/'
%left NOT
%nonassoc UMINUS

%type <exprval> var
%type <exprval> expression
%type <exprval> exprval
%type <intval> type
%type <type_dec> ensemble

%%

algorithme : START {gencode(CODE,DATA,NULL,NULL,NULL);} declarations {gencode(CODE,TEXT,NULL,NULL,NULL);} instructions {gencode(CODE,EXIT,NULL,NULL,NULL);} END algorithme
      | ;
      ;

declarations : type_declarations declarations
      | BLANK
      ;

type_declarations : entree '{' '$' liste_declarations '$' '}'
      ;

entree : INPUT
    | OUTPUT
    | LOCAL
    | GLOBAL
  ;

liste_declarations : declaration
          | declaration ',' liste_declarations
          | EMPTY_SET
          ;

declaration : VARNAME IN ensemble
{
  struct symbol * id = symtable_get(SYMTAB,$1);
  struct symbol * taille = symtable_const(SYMTAB,$3.td->length);
  if ( id == NULL )
  {
  	if($3.td->type == INT)
      id = symtable_put(SYMTAB,$1,NAME_INT);
  	else if($3.td->type == REAL)
      id = symtable_put(SYMTAB,$1,NAME_FLOAT);
  }
  if($3.td->type == INT)
    gencode(CODE,DECLARE_INT,id,taille,NULL);
  else if($3.td->type == REAL)
    gencode(CODE,DECLARE_FLOAT,id,taille,NULL);
}
      ;

ensemble : type { struct typedec * td = malloc(sizeof(struct typedec)); td->type = $1; td->length = 1; $$.td = td;}
    | type '^' '{' expression '}' { struct typedec * td = malloc(sizeof(struct typedec)); td->type = $1; td->length = $4.ptr->u.value ; $$.td = td;}
    ;

type : INT { $$ = INT;}
	   | REAL {$$ = REAL;}
  ;

  // Fonction, initialisation, for while if,
instructions : affectation instructions
      | print instructions
      | condif instructions
      | loop instructions
      | ;
      ;

condif
: IF '{' '$' expression '$' '}' { gencode(CODE, IF_CONDITION, $4.ptr, NULL, NULL); } '{' instructions '}'
{
  gencode(CODE, LABEL, NULL, NULL, NULL);
}
| ELIF '{' '$' expression '$' '}' { gencode(CODE, ELIF_CONDITION, $4.ptr, NULL, NULL); } '{' instructions '}' { gencode(CODE, JUMP, NULL, NULL, NULL); gencode(CODE, LABEL, NULL, NULL, NULL); } '{' instructions '}'
{
  gencode(CODE, LABEL, NULL, NULL, NULL);
}
;

print: '$' MBOX '{' PRINTINT '(' '$' expression '$' ')' '}' '$' ENDL
{
  gencode(CODE,PRINT_INT,$7.ptr,NULL,NULL);
}

| '$' MBOX '{' PRINTREAL '(' '$' expression '$' ')' '}' '$' ENDL
{
  struct quad * q = (struct quad *) $7.ptr;
  switch (q->kind)
  {
    case FCONST: ; // constante
      struct symbol * id0 = symtable_fconst(SYMTAB,$7.ptr->u.fvalue);
      char nom[6];
      sprintf(nom,"flo%d",floatnum);
      struct symbol * id2 = symtable_put(SYMTAB,nom,NAME_FLOAT);
      gencode(FINDATA,DATA_FLOAT,id0,id2,NULL);
      floatnum++;

      gencode(CODE, PRINT_FLOAT, id2, NULL, NULL);
      break;
    case NAME_FLOAT: ; // nom de variable
      struct symbol * id = symtable_get(SYMTAB, $7.ptr->u.strvalue);
      gencode(CODE, PRINT_FLOAT, id, NULL, NULL);

      break;
  }
}
| '$' MBOX '{' PRINTTEXT '(' '$' STRING '$' ')' '}' '$' ENDL
{
	struct symbol * id = symtable_put(SYMTAB,$7,NAME_STR);
	char nom[6];
	sprintf(nom,"str%d",strnum);
	struct symbol * id2 = symtable_put(SYMTAB,nom,NAME_STR);
	gencode(FINDATA,DATA_STR,id,id2,NULL);
	gencode(CODE,PRINT_STR,id2,NULL,NULL);
	strnum++;
}
;

loop
: WHILE '{' { gencode(CODE, LABEL2, NULL, NULL, NULL); } '$' expression '$' {
    gencode(CODE, INIT_WHILE, $5.ptr, NULL, NULL);
  } '}' '{' instructions '}' {
    gencode(CODE, END_LOOP, NULL, NULL, NULL);
}

| FOR '{' '$' var AFF expression {
    struct symbol * id = $4.ptr;
    gencode(CODE,COPY,id,$6.ptr,NULL);
  } '$' TO '$' expression {
      gencode(CODE, INIT_FOR, $4.ptr, $11.ptr, NULL);
    } '$' '}' '{' { gencode(CODE, BEGIN_FOR, $4.ptr, $11.ptr, NULL); } instructions {
        gencode(CODE, UPDATE_FOR, NULL, NULL, NULL);
      } '}' {
        gencode(CODE, END_LOOP, NULL, NULL, NULL);
}
;

affectation :
'$' var AFF expression '$' ENDL //todo tableau (VARNAME -> var)
{
  struct symbol * id = $2.ptr;

  if(((id->kind == NAME_INT || id->kind == ADDR_INT) && ($4.ptr->kind == NAME_INT || $4.ptr->kind == CONSTANT || $4.ptr->kind == ADDR_INT)) ||
    ((id->kind == NAME_FLOAT || id->kind == ADDR_FLOAT) && ($4.ptr->kind == NAME_FLOAT || $4.ptr->kind == FCONST || $4.ptr->kind == ADDR_FLOAT)))
    gencode(CODE,COPY,id,$4.ptr,NULL);
  else
  {
    printf("Erreur sémantique affectation\n");
    exit(1);
  }
}
    ;

expression: expression AND expression // todo: not
{
  // dans les deux cas $$ sera égal a int vu que les bool sont des int
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LOGICAL_AND, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LOGICAL_AND, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}
| expression '<' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LESSER, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LESSER, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}
| expression LEQ expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LESSEREQ, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LESSEREQ, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}
| expression '>' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, GREATER, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, GREATER, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}
| expression GEQ expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, GREATEREQ, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, GREATEREQ, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}
| expression '=' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, EQUALS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, EQUALS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

| expression NEQ expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, DIFFERS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, DIFFERS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

| expression OR expression
{
  // dans les deux cas $$ sera égal a int vu que les bool sont des int
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LOGICAL_OR, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LOGICAL_OR, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

| expression '+' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, PLUS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_FLOAT);
    gencode(CODE, PLUS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

| expression '-' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, MOINS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_FLOAT);
    gencode(CODE, MOINS, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

| expression '*' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, MULT, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_FLOAT);
    gencode(CODE, MULT, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

| expression '/' expression
{
  if (($1.ptr->kind == CONSTANT || $1.ptr->kind == NAME_INT || $1.ptr->kind == ADDR_INT) ||
      ($3.ptr->kind == CONSTANT || $3.ptr->kind == NAME_INT || $3.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, DIV, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($1.ptr->kind == FCONST || $1.ptr->kind == NAME_FLOAT || $1.ptr->kind == ADDR_FLOAT) ||
          ($3.ptr->kind == FCONST || $3.ptr->kind == NAME_FLOAT || $3.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_FLOAT);
    gencode(CODE, DIV, $$.ptr, $1.ptr, $3.ptr);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}

|NOT expression
{
  // dans les deux cas $$ sera égal a int vu que les bool sont des int
  if (($2.ptr->kind == CONSTANT || $2.ptr->kind == NAME_INT || $2.ptr->kind == ADDR_INT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LOGICAL_NOT, $$.ptr, $2.ptr, NULL);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if (($2.ptr->kind == FCONST || $2.ptr->kind == NAME_FLOAT || $2.ptr->kind == ADDR_FLOAT))
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, LOGICAL_NOT, $$.ptr, $2.ptr, NULL);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
} // TODO -
| '-' expression %prec UMINUS
{
  if ($2.ptr->kind == CONSTANT || $2.ptr->kind == NAME_INT || $2.ptr->kind == ADDR_INT)
  {
    $$.ptr = newtemp(SYMTAB, NAME_INT);
    gencode(CODE, U_MINUS, $$.ptr, $2.ptr, NULL);
    gencode(FINDATA,DATA_INT,$$.ptr,NULL,NULL);
  }
  else if ($2.ptr->kind == FCONST || $2.ptr->kind == NAME_FLOAT || $2.ptr->kind == ADDR_FLOAT)
  {
    $$.ptr = newtemp(SYMTAB, NAME_FLOAT);
    gencode(CODE, U_MINUS, $$.ptr, $2.ptr, NULL);
    gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
  }
}
| '(' expression ')' { $$ = $2; }
| exprval
;

exprval: NUMBER
{
  $$.ptr = symtable_const(SYMTAB,$1);
}

| DECIMAL
{
	$$.ptr = symtable_fconst(SYMTAB,$1);
}
| var
| MBOX '{' VARNAME '(' '$' expression '$' ')' '}'
{
  fprintf(stderr, "Fonctions non implémentées.\n");
  exit(4);
}
      ;

var: VARNAME
{
  struct symbol * id = symtable_get(SYMTAB,$1);
  if ( id == NULL )
  {
      fprintf(stderr,"Name '%s' undeclared\n",$1);
      exit(3);
  }
  $$.ptr = id;
}
| VARNAME '_' '{' expression '}'
{
  struct symbol * id = symtable_get(SYMTAB,$1);
  if ( id == NULL )
  {
      fprintf(stderr,"Name '%s' undeclared\n",$1);
      exit(3);
  }
  if(id->kind == NAME_INT)
    $$.ptr = newtemp(SYMTAB, ADDR_INT);
  else if(id->kind == NAME_FLOAT)
    $$.ptr = newtemp(SYMTAB, ADDR_FLOAT);
  else
  {
    printf("Problème tableau\n");
    exit(4);
  }
  gencode(CODE, ADDRCALC, $$.ptr, $4.ptr, id);
  gencode(FINDATA,DATA_INT, $$.ptr,NULL,NULL);
}
  ;

/*liste_param : expression
          | expression ',' liste_param
          ;
*/
%%

void yyerror(const char * s)
{
    fprintf(stderr,"%s\n",s);
    exit(2);
}
