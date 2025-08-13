CC = g++
CFLAGS = -Wall
BFLAGS = -Wcounterexamples

lexer.cpp : lexer.l parser.hpp
	flex -s -o lexer.cpp lexer.l

lexer.o : lexer.cpp lexer.hpp parser.hpp ast.hpp symbol.hpp

parser.hpp parser.cpp : parser.y
	bison -dv $(BFLAGS) -o parser.cpp parser.y

parser.o : parser.cpp lexer.hpp ast.hpp symbol.hpp

danac : lexer.o parser.o
	$(CC) $(CFLAGS) -o danac lexer.o parser.o -lfl

clean: 
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o

distclean: clean
	$(RM) danac
