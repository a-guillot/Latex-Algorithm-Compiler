#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

/* TABLE DES SYMBOLES */

struct symtable
{
  unsigned int capacity;
  unsigned int temporary;
  unsigned int size;
  struct symbol *symbols;
};

/* QUADRUPLETS ET CODE */

struct quad
{
  enum quad_kind
  {
    COPY, //affectation
    PRINT_INT,
    PRINT_STR,
    PRINT_FLOAT,
    DATA, //créé les premières lignes .data
    TEXT, //créé les lignes .text et main:
    DECLARE_INT,
    DECLARE_FLOAT,
    DATA_STR,
    DATA_INT,
    DATA_FLOAT,
    PLUS, //opération +
    MOINS, // -
    U_MINUS,
    MULT, // *
    DIV, // /
    LOGICAL_OR, // ||
    LOGICAL_NOT,
    LOGICAL_AND,
    EQUALS,
    DIFFERS,
    GREATER,
    LESSER,
    GREATEREQ,
    LESSEREQ,
    IF_CONDITION,
    ELIF_CONDITION,
    JUMP,
    LABEL,
    LABEL2,
    INIT_WHILE,
    INIT_FOR,
    BEGIN_FOR,
    UPDATE_FOR,
    END_LOOP,
    ADDRCALC,
    EXIT
  } kind;
  struct symbol *sym1;
  struct symbol *sym2;
  struct symbol *sym3;
};

struct code
{
  unsigned int capacity;
  unsigned int nextquad;
  struct quad *quads;
};

struct symtable *symtable_new();

static void symtable_grow(struct symtable *t);

struct symbol *symtable_const(struct symtable *t, long int v);

struct symbol *symtable_fconst(struct symtable *t, float v); // pour les
                                                             // flotants

struct symbol *symtable_get(struct symtable *t, const char *s);

struct symbol *symtable_put(struct symtable *t, const char *s,int type);

void symtable_dump(struct symtable *t);

void symtable_free(struct symtable *t);

struct code *code_new();

static void code_grow(struct code *c);

void gencode(struct code *c, enum quad_kind k, struct symbol *s1,
             struct symbol *s2, struct symbol *s3);

struct symbol *newtemp(struct symtable *t,int type);

static void symbol_dump(struct symbol *s, FILE *out);

static void quad_dump(struct quad *q, FILE *out);

void code_dump(struct code *c, FILE *out);

void code_free(struct code *c);

void loadsym(struct symbol * s, FILE * out, int reg);

void savesym(struct symbol * s, FILE * out, int reg);
