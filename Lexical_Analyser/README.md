This directory contains the lexical analyser source code of the compiler that uses flex. To get the lexical analyser executable do the following:
1. flex -o dana_lexer.c dana_lexer.l
2. gcc -o dana_lexer dana_lexer.c -lfl
Now, to check if your LA works properly you type: ./dana_lexer < {input} where {input} is the input file with your source code
