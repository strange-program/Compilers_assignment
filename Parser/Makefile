CC = gcc
CFLAGS = -Wall

lexer.c : lexer.l parser.h
	flex -s -o lexer.c lexer.l

lexer.o : lexer.c lexer.h parser.h

parser.h parser.c : parser.y
	bison -dv -o parser.c parser.y

parser.o : parser.c lexer.h

danac : lexer.o parser.o
	$(CC) $(CFLAGS) -o danac lexer.o parser.o -lfl

clean: 
	$(RM) lexer.c parser.c parser.h parser.output *.o

distclean: clean
	$(RM) danac
