argshell: argshell.o lex.yy.c
	cc -o argshell argshell.o lex.yy.c -ll

lex.yy.c: shell.l
	flex -c shell.l

argshell.o: argshell.c
	cc -Wall -c argshell.c

lex.yy.o: lex.yy.c
	cc -Wall -c lex.yy.c 

clean: 
		-rm *.o
		-rm argshell
		-rm lex.yy.c