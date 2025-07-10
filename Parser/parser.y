%{
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
extern int lineno;
%}

%token T_and      "and"       
%token T_as       "as"    
%token T_begin    "begin"
%token T_break    "break"
%token T_byte     "byte"
%token T_continue "continue"
%token T_decl     "decl"
%token T_def      "def"
%token T_elif     "elif"
%token T_else     "else"
%token T_end      "end"
%token T_exit     "exit"
%token T_false    "false"
%token T_if       "if"
%token T_is       "is"
%token T_int      "int"
%token T_loop     "loop"
%token T_not      "not"
%token T_or       "or"
%token T_ref      "ref"
%token T_return   "return"
%token T_skip     "skip"
%token T_true     "true"
%token T_var      "var"

%token T_not_equal     "<>"
%token T_less_or_equal "<="
%token T_more_or_equal ">="
%token T_assign        ":="

%token T_identifier
%token T_int_const
%token T_string
%token T_char_const


%left '+' '-'
%left '*' '/' '%'

%expect 1

%%

program : stmt_list
;

stmt_list : /* epsilon */
| stmt_list stmt
;

stmt : "begin" stmt_list "end"
| "loop" expr ':' stmt
| "if" expr ':' stmt
| "if" expr ':' stmt "else" stmt
| "var" T_identifier "is" type
| T_identifier ":=" expr
;

type : "int" | "byte";

expr : T_identifier | T_int_const
| '(' expr ')'
| expr '+' expr
| expr '-' expr
| expr '*' expr
| expr '/' expr
| expr '%' expr
;


%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", lineno, s);
}


int main() {
  int res = yyparse();
  if (res == 0) printf("Successful parsing\n");
  return res;
}
