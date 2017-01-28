
%{
#include "lib.h"
#include "texcc.tab.h"
#include <string.h>
%}

%option nounput
%option noyywrap
%s HEADER GET_EVT BODY DECLARATIONS ALGORITHM

VARNAME [a-zA-Z]+[0-9]*
STRING \"(\\.|[^"])*\"
NUMBER [0-9]+
DECIMAL [0-9]+\.[0-9]+
COMMENT "\\tcc{".*"}"

%%

  /* Règles générales (dont les commentaires, keywords), et celles qui comment les états */
"\\documentclass".* { BEGIN(HEADER);}

<HEADER>{
  "\\begin{document}".*   { BEGIN(BODY); }
  .             {}
  \n              {}
}

<BODY>{
  "\\begin{texsci}{"{VARNAME}"}" { BEGIN(DECLARATIONS); return START; }
  .   {}
  \n {}
}

<DECLARATIONS>{
  [ \t]     {}
  "\\BlankLine"   { return BLANK; }
  "\\Input"       { return INPUT; }
  "\\Output"      { return OUTPUT; }
  "\\Local"       { return LOCAL; }
  "\\Global"      { return GLOBAL; }
  "\\in"          { return IN; }
  "\\Integer"     { return INT; }
  "\\Real"        { return REAL; }
  "\\Boolean"     { return BOOLEAN; }
  "\\emptyset"    { return EMPTY_SET; }
  "\\;"           { return ENDL; }
  "\\leftarrow"   { return AFF; }
  "\\neq"         { return NEQ; }
  "\\times"       { return '*'; }
  "\\mbox"        { return MBOX ; }
  "\\div"         { return '/';}
  "+"             { return '+'; }
  "-"             { return '-'; }
  "("             { return '('; }
  ")"             { return ')'; }
  "^"             { return '^'; }
  "="             { return '='; }
  ","             { return ','; }
  "/"             { return '/'; }
  "$"             { return '$'; }
  "{"             { return '{'; }
  "}"             { return '}'; }
  "_"             { return '_'; }
  "\\eIf"         { return ELIF; }
  "\\If"          { return IF;}
  "\\While"       { return WHILE; }
  "\\For"         { return FOR; }
  "\\KwTo"        { return TO; }
  "\\vee"         { return OR; }
  "\\neg"             { return NOT; }
  "\\wedge"       { return AND; }
  "<"             { return '<'; }
  ">"             { return '>'; }
  "\\geq"         { return GEQ; }
  "\\leq"         { return LEQ; }

  "\\end{texsci}".* { BEGIN(BODY); return END; }

  "printText"     { return PRINTTEXT; }
  "printInt"      { return PRINTINT; }
  "printReal"     { return PRINTREAL; }

  {VARNAME}       { if ( yyleng > 7 )
                      fprintf(stderr,"Identifier '%s' too long, truncated\n",yytext);
                    strncpy(yylval.strval,yytext,7);
                    yylval.strval[7] = '\0';
                    return VARNAME; }
  {STRING}        { if ( yyleng > 999 )
                      fprintf(stderr,"String too long, truncated\n");
                    strncpy(yylval.strval,yytext,999);
                    yylval.strval[998] = '"';
                    yylval.strval[999] = '\0';
                    return STRING; }
  "\\true"        { yylval.intval = 1; return NUMBER; }
  "\\false"       { yylval.intval = 0; return NUMBER; }
  {NUMBER}        { yylval.intval = atoi(yytext); return NUMBER; }
  {DECIMAL}       { yylval.floval = atof(yytext); return DECIMAL; }

  {COMMENT}       {}

  \n {}
}
. { printf("erreur_lexicale(%s) \n", yytext); exit(1); }

%%
