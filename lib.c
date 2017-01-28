
#include "lib.h"

int label_number = 0;
struct stack stack;
struct stack sstack;

struct symtable *symtable_new()
{
  stack = create_stack();
  sstack = create_stack();
  struct symtable *t = malloc(sizeof(struct symtable));
  t->capacity = 1024;
  t->symbols = malloc(t->capacity * sizeof(struct symbol));
  t->temporary = 0;
  t->size = 0;
  return t;
}

static void symtable_grow(struct symtable *t)
{
  t->capacity += 1024;
  t->symbols = realloc(t->symbols, t->capacity * sizeof(struct symbol));
  if (t->symbols == NULL)
  {
    fprintf(stderr,
            "Error attempting to grow symbol table (actual size is %d)\n",
            t->size);
    exit(4);
  }
}

struct symbol *symtable_const(struct symtable *t, long int v)
{
  unsigned int i;
  for (i = 0; i < t->size &&
              (t->symbols[i].u.value != v || t->symbols[i].kind != CONSTANT);
       i++)
    ;
  if (i == t->size)
  {
    if (t->size == t->capacity)
      symtable_grow(t);
    struct symbol *s = &(t->symbols[t->size]);
    s->kind = CONSTANT;
    s->u.value = v;
    ++(t->size);
    return s;
  }
  else
  {
    return &(t->symbols[i]);
  }
}

struct symbol *symtable_fconst(struct symtable *t, float v)
{
  unsigned int i;
  for (i = 0; i < t->size &&
              (t->symbols[i].u.value != v || t->symbols[i].kind != FCONST);
       i++)
    ;
  if (i == t->size)
  {
    if (t->size == t->capacity)
      symtable_grow(t);
    struct symbol *s = &(t->symbols[t->size]);
    s->kind = FCONST;
    s->u.fvalue = v;
    ++(t->size);
    return s;
  }
  else
  {
    return &(t->symbols[i]);
  }
}

struct symbol *symtable_get(struct symtable *t, const char *id)
{
  unsigned int i;
  for (i = 0; i < t->size && strcmp(t->symbols[i].u.name, id) != 0; i++)
    ;
  if (i < t->size)
    return &(t->symbols[i]);
  return NULL;
}

struct symbol *symtable_put(struct symtable *t, const char *id, int type)
{
  if (t->size == t->capacity)
    symtable_grow(t);
  struct symbol *s = &(t->symbols[t->size]);
  s->kind = type;
  strcpy(s->u.name, id);
  ++(t->size);
  return s;
}

void symtable_dump(struct symtable *t)
{
  unsigned int i;
  for (i = 0; i < t->size; i++)
  {
    if (t->symbols[i].kind == CONSTANT)
      printf("       %p = %ld\n", &(t->symbols[i]), t->symbols[i].u.value);
    if (t->symbols[i].kind == NAME_INT || t->symbols[i].kind == NAME_FLOAT ||
        t->symbols[i].kind == NAME_STR || t->symbols[i].kind == ADDR_INT ||
        t->symbols[i].kind == ADDR_FLOAT)
      printf("       %p = %s\n", &(t->symbols[i]), t->symbols[i].u.name);
    if (t->symbols[i].kind == FCONST)
      printf("       %p = %f\n", &(t->symbols[i]), t->symbols[i].u.fvalue);
  }
  printf("       --------\n");
}

void symtable_free(struct symtable *t)
{
  free(t->symbols);
  free(t);
}

struct code *code_new()
{
  struct code *r = malloc(sizeof(struct code));
  r->capacity = 1024;
  r->quads = malloc(r->capacity * sizeof(struct quad));
  r->nextquad = 0;
  return r;
}

static void code_grow(struct code *c)
{
  c->capacity += 1024;
  c->quads = realloc(c->quads, c->capacity * sizeof(struct quad));
  if (c->quads == NULL)
  {
    fprintf(stderr, "Error attempting to grow quad list (actual size is %d)\n",
            c->nextquad);
    exit(4);
  }
}

void gencode(struct code *c, enum quad_kind k, struct symbol *s1,
             struct symbol *s2, struct symbol *s3)
{
  if (c->nextquad == c->capacity)
    code_grow(c);
  struct quad *q = &(c->quads[c->nextquad]);
  q->kind = k;
  q->sym1 = s1;
  q->sym2 = s2;
  q->sym3 = s3;
  ++(c->nextquad);
}

struct symbol *newtemp(struct symtable *t, int type)
{
  struct symbol *s;
  name_t name;
  sprintf(name, "t%d", t->temporary);
  s = symtable_put(t, name, type);
  ++(t->temporary);
  return s;
}

static void symbol_dump(struct symbol *s, FILE *out)
{
  switch (s->kind)
  {
  case NAME_INT:
    if (strlen(s->u.name) < 2) // Pas de variable à un seul caractère en MIPS
                               // ...
      fprintf(out, "%s_", s->u.name);
    else
      fprintf(out, "%s", s->u.name);
    break;
  case NAME_FLOAT:
    if (strlen(s->u.name) < 2)
      fprintf(out, "%s_", s->u.name);
    else
      fprintf(out, "%s", s->u.name);
    break;
  case NAME_STR:
    if (strlen(s->u.name) < 2)
      fprintf(out, "%s_", s->u.name);
    else
      fprintf(out, "%s", s->u.name);
    break;
  case CONSTANT:
    fprintf(out, "%ld", s->u.value);
    break;
  case FCONST:
    fprintf(out, "%f", s->u.fvalue);
    break;
  case ADDR_INT:
    if (strlen(s->u.name) < 2)
      fprintf(out, "%s_", s->u.name);
    else
      fprintf(out, "%s", s->u.name);
    break;
  case ADDR_FLOAT:
    if (strlen(s->u.name) < 2)
      fprintf(out, "%s_", s->u.name);
    else
      fprintf(out, "%s", s->u.name);
  default:
    break;
  }
}

static void quad_dump(struct quad *q, FILE *out)
{
  switch (q->kind)
  {

  case PRINT_INT:
    fprintf(out, "li $v0, 1\n");
    switch (q->sym1->kind)
    {
    case CONSTANT:
      fprintf(out, "li $a0, ");
      symbol_dump(q->sym1, out);
      break;
    case NAME_INT:
      fprintf(out, "lw $a0, ");
      symbol_dump(q->sym1, out);
      break;
    case ADDR_INT:
      fprintf(out, "lw $t7, ");
      symbol_dump(q->sym1, out);
      fprintf(out, "\nlw $a0, 0($t7)");
      break;
    default:
      printf("Erreur sémantique print_INT\n");
      exit(3);
      break;
    }
    fprintf(out, "\nsyscall\n");
    // print un retour à la ligne.
    fprintf(out, "addi $a0, $0, 0xA\n");
    fprintf(out, "addi $v0, $0, 0xB\n");
    fprintf(out, "syscall\n");
    break;

  case PRINT_FLOAT:
    fprintf(out, "li $v0, 2\n");
    switch (q->sym1->kind)
    {
    case FCONST: // constante
      fprintf(out, "lwc1 $f12, ");
      symbol_dump(q->sym1, out);
      break;
    case NAME_FLOAT: // nom de variable
      fprintf(out, "lwc1 $f2, ");
      symbol_dump(q->sym1, out);
      fprintf(out, "\nmov.s $f12,$f2");
      break;
    case ADDR_FLOAT:
      fprintf(out, "lw $t7, ");
      symbol_dump(q->sym1, out);
      fprintf(out, "\nlwc1 $f12, 0($t7)");
      break;
    }
    fprintf(out, "\nsyscall\n");
    // print un retour à la ligne.
    fprintf(out, "addi $a0, $0, 0xA\n");
    fprintf(out, "addi $v0, $0, 0xB\n");
    fprintf(out, "syscall\n");
    break;

  case PRINT_STR:
    fprintf(out, "li $v0,4\nla $a0, ");
    symbol_dump(q->sym1, out);
    fprintf(out, "\nsyscall\n");

    // print un retour à la ligne.
    fprintf(out, "addi $a0, $0, 0xA\n");
    fprintf(out, "addi $v0, $0, 0xB\n");
    fprintf(out, "syscall\n");
    break;

  case COPY:
    loadsym(q->sym2, out, 0);
    savesym(q->sym1, out, 0);
    break;

  case DATA:
    fprintf(out, ".data\nprintln: .asciiz \"\\n\"\n");
    break;

  case TEXT:
    fprintf(out, ".text\nmain:\n");
    break;

  case DECLARE_INT:
  {
    symbol_dump(q->sym1, out);
    fprintf(out, ": .word 0");
    int i;
    for (i = 1; i < q->sym2->u.value; i++)
      fprintf(out, ",0");
    fprintf(out, "\n");
    break;
  }

  case DECLARE_FLOAT:
  {
    symbol_dump(q->sym1, out);
    fprintf(out, ": .float 0.0");
    int i;
    for (i = 1; i < q->sym2->u.value; i++)
      fprintf(out, ",0.0");
    fprintf(out, "\n");
    break;
  }

  case DATA_INT:
    symbol_dump(q->sym1, out);
    fprintf(out, ": .word 0\n");
    break;

  case DATA_FLOAT:
    symbol_dump(q->sym2, out);
    fprintf(out, ": .float ");
    symbol_dump(q->sym1, out);
    fprintf(out, "\n");
    break;

  case DATA_STR:
    symbol_dump(q->sym2, out);
    fprintf(out, ": .asciiz ");
    symbol_dump(q->sym1, out);
    fprintf(out, "\n");
    break;

  case PLUS:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
      fprintf(out, "\nadd $t0, $t1, $t2\n");
    else if ((q->sym1->kind == FCONST || q->sym1->kind == NAME_FLOAT ||
              q->sym1->kind == ADDR_FLOAT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
      fprintf(out, "\nadd.s $f0, $f1, $f2\n");
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case MOINS:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
      fprintf(out, "\nsub $t0, $t1, $t2\n");
    else if ((q->sym1->kind == FCONST || q->sym1->kind == NAME_FLOAT ||
              q->sym1->kind == ADDR_FLOAT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
      fprintf(out, "\nsub.s $f0, $f1, $f2\n");
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case U_MINUS:
    loadsym(q->sym2, out, 1);
    fprintf(out, "\nli $t2, 0\n");
    fprintf(out, "li.s $f2, 0.0");
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT))
      fprintf(out, "\nsub $t0, $t2, $t1\n");
    else if ((q->sym1->kind == FCONST || q->sym1->kind == NAME_FLOAT ||
              q->sym1->kind == ADDR_FLOAT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT))
      fprintf(out, "\nsub.s $f0, $f2, $f1\n");
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case MULT:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nmult $t1, $t2\n");
      fprintf(out, "mflo $t0\n");
    }
    else if ((q->sym1->kind == FCONST || q->sym1->kind == NAME_FLOAT ||
              q->sym1->kind == ADDR_FLOAT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      fprintf(out, "\nmul.s $f0, $f1, $f2\n");
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case DIV:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\ndiv $t1, $t2\n");
      fprintf(out, "mflo $t0\n");
    }
    else if ((q->sym1->kind == FCONST || q->sym1->kind == NAME_FLOAT ||
              q->sym1->kind == ADDR_FLOAT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      fprintf(out, "\ndiv.s $f0, $f1, $f2\n");
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case LOGICAL_OR:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nbnez $t1, L%d\n", label_number);
      fprintf(out, "bnez $t2, L%d\n", label_number++);
      // si les deux sont égaux à 0 alors c'est faux.
      fprintf(out, "li $t0 0\n");
      fprintf(out, "j L%d\n\n", label_number);
      // Sinon c'est vrai
      fprintf(out, "L%d:\n", (label_number - 1));
      fprintf(out, "li $t0 1\n\n");
      // Fin
      fprintf(out, "L%d:\n", label_number++);
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;
      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nli.s $f3, 0.0\n");

      fprintf(out, "c.eq.s $f1, $f3\n");
      fprintf(out, "bc1f L%d\n", true);

      fprintf(out, "c.eq.s $f2, $f3\n");
      fprintf(out, "bc1f L%d\n", true);

      fprintf(out, "li $t0, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t0, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case LOGICAL_AND:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nbeqz $t1, L%d\n", label_number);
      fprintf(out, "beqz $t2, L%d\n", label_number++);

      // si les deux sont égaux à 0 alors c'est faux.
      fprintf(out, "li $t0 1\n");
      fprintf(out, "j L%d\n\n", label_number);

      // Sinon c'est vrai
      fprintf(out, "L%d:\n", (label_number - 1));
      fprintf(out, "li $t0 0\n\n");

      // Fin
      fprintf(out, "L%d:\n", label_number++);
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nli.s $f3, 0.0\n");

      fprintf(out, "c.eq.s $f1, $f3\n");
      fprintf(out, "bc1t L%d\n", true);

      fprintf(out, "c.eq.s $f2, $f3\n");
      fprintf(out, "bc1t L%d\n", true);

      fprintf(out, "li $t0, 1\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t0, 0\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case LOGICAL_NOT:
    loadsym(q->sym2, out, 1);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT))
    {
      fprintf(out, "\nbnez $t1, L%d\n", label_number++);

      // si  égaux à 0 alors il devient vrai
      fprintf(out, "li $t0 1\n");
      fprintf(out, "j L%d\n\n", label_number);

      // Sinon il devient faux
      fprintf(out, "L%d:\n", (label_number - 1));
      fprintf(out, "li $t0 0\n\n");

      // Fin
      fprintf(out, "L%d:\n", label_number++);
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nli.s $f3, 0.0\n");

      fprintf(out, "c.eq.s $f1, $f3\n");
      fprintf(out, "bc1t L%d\n", true);

      fprintf(out, "li $t0, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t0, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case EQUALS:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nbeq $t1, $t2, L%d\n", label_number++);

      fprintf(out, "li $t0 0\n");
      fprintf(out, "j L%d\n\n", label_number);

      fprintf(out, "L%d:\n", (label_number - 1));
      fprintf(out, "li $t0 1\n\n");

      // Fin
      fprintf(out, "L%d:\n", label_number++);
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nc.eq.s $f1, $f2\n");
      fprintf(out, "bc1t L%d\n", true);

      fprintf(out, "li $t0, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t0, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case GREATER:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nslt $t3, $t2, $t1\n");
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nc.le.s $f1, $f2\n");
      fprintf(out, "bc1f L%d\n", true);

      fprintf(out, "li $t3, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t3, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 3);
    break;

  case GREATEREQ:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nsle $t3, $t2, $t1\n");
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nc.lt.s $f1, $f2\n");
      fprintf(out, "bc1f L%d\n", true);

      fprintf(out, "li $t3, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t3, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 3);
    break;

  case LESSER:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nslt $t3, $t1, $t2\n");
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nc.lt.s $f1, $f2\n");
      fprintf(out, "bc1t L%d\n", true);

      fprintf(out, "li $t3, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t3, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 3);
    break;

  case LESSEREQ:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nsle $t3, $t1, $t2\n");
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nc.le.s $f1, $f2\n");
      fprintf(out, "bc1t L%d\n", true);

      fprintf(out, "li $t3, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t3, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 3);
    break;

  case DIFFERS:
    loadsym(q->sym2, out, 1);
    loadsym(q->sym3, out, 2);
    if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT ||
         q->sym1->kind == ADDR_INT) &&
        (q->sym2->kind == CONSTANT || q->sym2->kind == NAME_INT ||
         q->sym2->kind == ADDR_INT) &&
        (q->sym3->kind == CONSTANT || q->sym3->kind == NAME_INT ||
         q->sym3->kind == ADDR_INT))
    {
      fprintf(out, "\nbne $t1, $t2, L%d\n", label_number++);

      fprintf(out, "li $t0 0\n");
      fprintf(out, "j L%d\n\n", label_number);

      fprintf(out, "L%d:\n", (label_number - 1));
      fprintf(out, "li $t0 1\n\n");

      // Fin
      fprintf(out, "L%d:\n", label_number++);
    }
    else if ((q->sym1->kind == CONSTANT || q->sym1->kind == NAME_INT) &&
             (q->sym2->kind == FCONST || q->sym2->kind == NAME_FLOAT ||
              q->sym2->kind == ADDR_FLOAT) &&
             (q->sym3->kind == FCONST || q->sym3->kind == NAME_FLOAT ||
              q->sym3->kind == ADDR_FLOAT))
    {
      int true = label_number, end = label_number + 1;

      // les bonnes valeurs sont déjà dans $f1 et $f2
      fprintf(out, "\nc.eq.s $f1, $f2\n");
      fprintf(out, "bc1f L%d\n", true);

      fprintf(out, "li $t0, 0\n");
      fprintf(out, "j L%d\n", end);

      fprintf(out, "L%d:\n", true);
      fprintf(out, "li $t0, 1\n");

      // Fin
      fprintf(out, "L%d:\n", end);
      label_number += 2;
    }
    else
    {
      printf("Erreur sémantique : incompatibilité des types.\n");
      exit(3);
    }
    savesym(q->sym1, out, 0);
    break;

  case IF_CONDITION:
    loadsym(q->sym1, out, 0);
    fprintf(out, "\nbeqz $t0, L%d\n", label_number);
    push(&stack, label_number++);
    break;

  case ELIF_CONDITION:
    loadsym(q->sym1, out, 0);
    int vrai = label_number, faux = label_number + 1;

    fprintf(out, "\nbeqz $t0, L%d\n", faux);

    push(&stack, vrai);
    push(&stack, faux);
    push(&stack, vrai);

    label_number += 2;
    break;

  case INIT_WHILE:
  { // obligé de mettre {} parce que le c n'accepte pas de commencer par une
    // déclaration
    int true = peek(&stack);
    loadsym(q->sym1, out, 0);
    fprintf(out, "\nbeqz $t0, L%d\n", true);
    break;
  }

  case INIT_FOR:
  {
    loadsym(q->sym1, out, 8);
    loadsym(q->sym2, out, 9);
    break;
  }

  case BEGIN_FOR:
  {
    fprintf(out, "\nL%d:", label_number);
    push(&stack, label_number++);
    push(&stack, label_number++);
    int true = peek(&stack);

    fprintf(out, "\nbgt $t8, $t9, L%d\n", true);

    savesym(q->sym1, out, 8);
    savesym(q->sym2, out, 9);

    pushs(&sstack, q->sym1);
    pushs(&sstack, q->sym2);
    break;
  }

  case UPDATE_FOR:
  {
    struct symbol *sup = pops(&sstack);
    struct symbol *inf = pops(&sstack);

    loadsym(inf, out, 8);
    loadsym(sup, out, 9);

    fprintf(out, "\naddi $t8, $t8, 1\n");
    break;
  }

  case END_LOOP:
  {
    int true = pop(&stack);
    int init = pop(&stack);
    fprintf(out, "j L%d\n", init);
    fprintf(out, "L%d:\n", true);
    break;
  }

  case LABEL:
    fprintf(out, "L%d:\n", pop(&stack));
    break;

  case LABEL2:
    fprintf(out, "L%d:\n", label_number);
    push(&stack, label_number++);
    push(&stack, label_number++);
    break;

  case JUMP:
    fprintf(out, "j L%d\n", pop(&stack));
    break;

  case ADDRCALC:
    fprintf(out, "la $t3, ");
    symbol_dump(q->sym3, out);
    fprintf(out, "\n");
    loadsym(q->sym2, out, 2);
    fprintf(out, "add $t2, $t2, $t2\n");
    fprintf(out, "add $t2, $t2, $t2\n");
    fprintf(out, "add $t1, $t2, $t3\n");
    fprintf(out, "sw $t1, ");
    symbol_dump(q->sym1, out);
    fprintf(out, "\n");
    break;

  case EXIT:
    fprintf(
        out,
        "li $v0, 4\nla $a0, println\nsyscall\nli $v0, 10\nsyscall\n.data\n");
    break;

  default:
    printf("BUG\n");
    break;
  }
}

void code_dump(struct code *c, FILE *out)
{
  unsigned int i;
  for (i = 0; i < c->nextquad; i++)
  {
    quad_dump(&(c->quads[i]), out);
  }
}

void code_free(struct code *c)
{
  free(c->quads);
  free(c);
}

void loadsym(struct symbol *s, FILE *out, int reg)
{
  switch (s->kind)
  {
  case CONSTANT:
    fprintf(out, "li $t%d, ", reg);
    symbol_dump(s, out);
    break;
  case NAME_INT:
    fprintf(out, "lw $t%d, ", reg);
    symbol_dump(s, out);
    break;
  case FCONST: // constante
    fprintf(out, "li.s $f%d, ", reg);
    symbol_dump(s, out);
    break;
  case NAME_FLOAT: // nom de variable
    fprintf(out, "lwc1 $f12, ");
    symbol_dump(s, out);
    break;
  case ADDR_FLOAT:
    fprintf(out, "lw $t7, ");
    symbol_dump(s, out);
    fprintf(out, "\nlwc1 $f%d, 0($t7)", reg);
    break;
  case ADDR_INT:
    fprintf(out, "lw $t7, ");
    symbol_dump(s, out);
    fprintf(out, "\nlw $t%d, 0($t7)", reg);
    break;
  }
  if (s->kind == NAME_FLOAT)
    fprintf(out, "\nmov.s $f%d, $f12", reg);
  fprintf(out, "\n");
}

void savesym(struct symbol *s, FILE *out, int reg)
{
  switch (s->kind)
  {
  case NAME_INT:
    fprintf(out, "sw $t%d, ", reg);
    symbol_dump(s, out);
    fprintf(out, "\n");
    break;
  case NAME_FLOAT:
    fprintf(out, "swc1 $f%d, ", reg);
    symbol_dump(s, out);
    fprintf(out, "\n");
    break;
  case ADDR_FLOAT:
    fprintf(out, "lw $t7, ");
    symbol_dump(s, out);
    fprintf(out, "\nswc1 $f%d, 0($t7)\n", reg);
    break;
  case ADDR_INT:
    fprintf(out, "lw $t7, ");
    symbol_dump(s, out);
    fprintf(out, "\nsw $t%d, 0($t7)\n", reg);
    break;
  }
}
