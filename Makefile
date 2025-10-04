CC          = g++
LLVM_CONFIG = llvm-config

# Flags
CFLAGS   = -Wall $(shell $(LLVM_CONFIG) --cxxflags) -std=c++17
CFLAGS2  = $(shell $(LLVM_CONFIG) --cxxflags) -std=c++17
BFLAGS   = -Wcounterexamples
LDFLAGS  = $(shell $(LLVM_CONFIG) --ldflags --libs core scalaropts transformutils analysis support) -lfl

# Default target
all: danac

lexer.cpp: lexer.l parser.hpp
	flex -s -o lexer.cpp lexer.l

lexer.o: lexer.cpp lexer.hpp parser.hpp ast.hpp symbol.hpp
	$(CC) $(CFLAGS2) -c lexer.cpp

parser.hpp parser.cpp: parser.y
	bison -dv $(BFLAGS) -o parser.cpp parser.y

parser.o: parser.cpp lexer.hpp ast.hpp symbol.hpp
	$(CC) $(CFLAGS2) -c parser.cpp

# Library build
lib.a: lib.cpp
	$(CC) -Wall -g -fPIC -c lib.cpp -o lib.o
	ar rcs lib.a lib.o

# Main binary depends on everything
danac: lexer.o parser.o lib.a
	$(CC) $(CFLAGS) -o danac $^ $(LDFLAGS)

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o

distclean: clean
	$(RM) danac lib.a
