/*
        Andréas Guillot
*/
#include "stack.h"

struct stack create_stack()
{
  struct stack s;
  s.taille = 0;

  return s;
}

void push(struct stack *s, int label_number)
{
  struct elem *e;
  if ((e = malloc(sizeof(*e))) == NULL)
  {
    perror("malloc");
    exit(1);
  }
  e->label_number = label_number;

  if (s->taille == 0)
  {
    s->top = e;
    e->suivant = NULL;
  }
  else
  {
    e->suivant = s->top;
    s->top = e;
  }

  s->taille++;
}

int pop(struct stack *s)
{
  struct elem res;
  if (s->taille == 0)
  {
    return -1;
  }

  struct elem *top = s->top;
  res.label_number = top->label_number;

  if (s->taille == 1)
  {
    s->top = NULL;
  }
  else
  {
    s->top = s->top->suivant;
  }

  free(top);
  s->taille--;

  return res.label_number;
}

int peek(struct stack *s)
{
  if (s->taille == 0)
    return -1;
  return s->top->label_number;
}

void pushs(struct stack *s, struct symbol *symbol)
{
  struct elem *e;
  if ((e = malloc(sizeof(*e))) == NULL)
  {
    perror("malloc");
    exit(1);
  }
  e->symbol = symbol;

  if (s->taille == 0)
  {
    s->top = e;
    e->suivant = NULL;
  }
  else
  {
    e->suivant = s->top;
    s->top = e;
  }

  s->taille++;
}
struct symbol *pops(struct stack *s)
{
  struct elem res;

  struct elem *top = s->top;
  res.symbol = top->symbol;

  if (s->taille == 1)
  {
    s->top = NULL;
  }
  else
  {
    s->top = s->top->suivant;
  }

  free(top);
  s->taille--;

  return res.symbol;
}
struct symbol *peeks(struct stack *s) { return s->top->symbol; }
