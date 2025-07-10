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

%left "or"
%left "and"
%nonassoc "not"
%nonassoc '=' "<>" '>' '<' "<=" ">="
%left '+' '-' '|'
%left '*' '/' '%' '&'
%nonassoc UPLUS UMINUS '!'

%expect 1

%%

program : func_def ;

func_def : "def" header local_def_list block ;

header : T_identifier
| T_identifier "is" data_type
| T_identifier ':' fpar_def fpar_def_list
| T_identifier "is" data_type ':' fpar_def fpar_def_list
; 

fpar_def_list : /* epsilon */ | ',' fpar_def  ;

id_list : T_identifier | T_identifier id_list ;

fpar_def : id_list "as" fpar_type ;

data_type : "int" | "byte" ;

int_const_list : /* epsilon */ | '[' T_int_const ']' int_const_list ;

type : data_type int_const_list ;

fpar_type : type | "ref" data_type | data_type '[' ']' int_const_list ;

local_def_list : /* epsilon */ | local_def local_def_list ;

local_def : func_def | func_decl | var_def ;

func_decl : "decl" header ;

var_def : "var" id_list "is" type ;

stmt : "skip"
| lvalue ":=" expr
| proc_call
| "exit"
| "return" ':' expr
| "if" cond ':' block elif_list
| "if" cond ':' block elif_list "else" ':' block
| "loop" ':' block
| "loop" T_identifier ':' block
| "break"
| "break" ':' T_identifier
| "continue"
| "continue" ':' T_identifier ;

elif_list : /* epsilon */ | "elif" cond ':' block ;

stmt_list : stmt stmt_list | stmt ;

block : "begin" stmt_list "end" ;

proc_call : T_identifier | T_identifier ':' expr expr_list ;

func_call : T_identifier '(' ')' | T_identifier '(' expr expr_list ')' ;

expr_list : /* epsilon */ | ',' expr expr_list ;

lvalue : T_identifier | T_string | lvalue '[' expr ']' ;

expr : T_int_const
| T_char_const
| lvalue
| '(' expr ')'
| func_call
| '+' expr %prec UPLUS
| '-' expr %prec UMINUS
| expr '+' expr
| expr '-' expr
| expr '*' expr
| expr '/' expr
| expr '%' expr
| "true"
| "false"
| '!' expr
| expr '&' expr
| expr '|' expr ;

cond : expr
| '(' cond ')'
| "not" cond
| cond "and" cond
| cond "or" cond
| expr '=' expr
| expr "<>" expr
| expr '<' expr
| expr '>' expr
| expr "<=" expr
| expr ">=" expr ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", lineno, s);
}


int main() {
  int res = yyparse();
  if (res == 0) printf("Successful parsing\n");
  return res;
}
