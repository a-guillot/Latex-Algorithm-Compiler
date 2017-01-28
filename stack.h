/*
	Andréas Guillot
*/
#ifndef __STACK_H
#define __STACK_H
#include <stdlib.h>
#include <stdio.h>

typedef char name_t[8]; //nom de variables
typedef char string[1000]; //string

struct symbol
{
  enum
  {
    NAME_INT, //variable entière
    NAME_FLOAT, //variable float
    NAME_STR, //string
    CONSTANT, //entier
    FCONST, //float
    ADDR_INT,
    ADDR_FLOAT
  } kind;
  union {
    string strvalue; //valeur string
    name_t name; //valeur nom variable
    long int value; //valeur entière
    float fvalue; //valeur float
  } u;
};

struct typedec
{
  int type;
  int length;
} td;

struct elem
{
	int label_number;
	struct symbol * symbol;
	struct elem * suivant;
};

struct stack
{
	struct elem * top;
	int taille;
};

struct stack create_stack();
void push(struct stack * s, int label_number);
int pop(struct stack * s);
int peek(struct stack * s);

void pushs(struct stack * s, struct symbol * symbol);
struct symbol * pops(struct stack * s);
struct symbol * peeks(struct stack * s);

#endif
