%{
#include "lexer.hpp"
#include "parser.hpp"

#define T_eof 0

int lineno = 1;
%}

D [0-9]
L [a-zA-Z]
W [ \t\r]
S [\x20a-zA-Z0-9\t!@#$%^&*()-=+<>?;:,./|\\\']

%%

"and"       { return T_and; }
"as"        { return T_as; }
"begin"     { return T_begin; }
"break"     { return T_break; }
"byte"      { return T_byte; }
"continue"  { return T_continue; }
"decl"      { return T_decl; }
"def"       { return T_def; }
"elif"      { return T_elif; }
"else"      { return T_else; }
"end"       { return T_end; }
"exit"      { return T_exit; }
"false"     { return T_false; }
"if"        { return T_if; }
"is"        { return T_is; }
"int"       { return T_int; }
"loop"      { return T_loop; }
"not"       { return T_not; }
"or"        { return T_or; }
"ref"       { return T_ref; }
"return"    { return T_return; }
"skip"      { return T_skip; }
"true"      { return T_true; }
"var"       { return T_var; }
"<>"        { return T_not_equal; }
"<="        { return T_less_or_equal; }
">="	    { return T_more_or_equal; }
":="        { return T_assign; }

<<EOF>>     { return T_eof; }

{L}({L}|{D}|_)*             { return T_identifier; }
{D}+			    { return T_int_const; }

\"(\\[ntr0\\'"]|\\x[0-9a-fA-F]{2}|[^\\'"\n])*\"  { return T_string; }
\'(\\[ntr0\\'"]|\\x[0-9a-fA-F]{2}|[^\\'"\n])\'   { return T_char_const; }

\n   { ++lineno; }  

[\+\-\*\/\%\!\&\|\=\<\>\(\)\[\]\,\:]   { return yytext[0]; }

{W}+            { /*nothing*/ }
#.*	        { /*one-line-comment*/ }
\(\*(.|\n)*\*\) { /*multi-line-commnet*/}

.               {  fprintf(stderr, "Lexical Error at line %d: invalid character '%c' (ASCII %d)\n", lineno,yytext[0], yytext[0]);}

%%
