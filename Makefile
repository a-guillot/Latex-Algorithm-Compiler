CC = gcc
CFLAGS = -g
EXEC = texcc

all: $(EXEC)

$(EXEC): main.c texcc.tab.o lex.yy.o lib.o stack.o
	gcc -o $@ $^

texcc.tab.o: texcc.tab.c lib.h
	gcc -c texcc.tab.c

texcc.tab.c: texcc.y
	bison -d $<

lex.yy.o: lex.yy.c
	gcc -c lex.yy.c

lex.yy.c: texcc.lex texcc.tab.c
	flex texcc.lex

lib.o: CFLAGS+=-Wall -Wextra

lib.o: lib.c lib.h
	gcc -c lib.c

stack.o: stack.c stack.h
	gcc -c stack.c

test:
	python tests/tests.py

clean:
	rm -f texcc *tab* lib.o stack.o lex.yy*
