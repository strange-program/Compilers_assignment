%{
#include <cstdio>
#include <cstdlib>
#include <string>

#include "lexer.hpp"
#include "ast.hpp"

using namespace std;

extern int lineno;
Var_Symbol_Table vst;
Func_Symbol_Table fst;
vector<pair<string,FuncSTEntry>> declared_functions;
vector<pair<string,Data_Type*>> return_type_vec;
vector<string> loop_stack;

%}
%require "3.2"

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

%token<identifier>   T_identifier
%token<int_const>    T_int_const
%token<string_const> T_string
%token<char_const>   T_char_const

%token T_auto_end

%left "or"
%left "and"
%nonassoc "not"
%nonassoc '=' "<>" '>' '<' "<=" ">="
%left '+' '-' '|'
%left '*' '/' '%' '&'
%nonassoc UPLUS UMINUS '!'

%union{
	Declaration* declaration;
	Statement* statement;
	Expression* expression;
	Block* blk;	
	Func_Def* function_definition;
	Func_Header* header;

	L_value* lval;
	Proc_call* procedure_call;	
	Func_call* function_call;
	Parameter* parameter;
	Param_type* param_type;
	Variable_decl* variable_declaration;

	vector<Int_const*>* int_const_list;
	vector<Identifier*>* identifier_list;
	vector<Parameter*>* parameter_list;
	Decl_list* decl_list;
	vector<Expression*> *expression_list;
	Elif_list* elif_list;

	Data_Type* data_type;
	char* identifier; 
	char* string_const;
	int int_const;
	char char_const;
}

//%destructor {
//    delete $$;
//} <expression_list>

%type<data_type> data_type

%type<statement> stmt
%type<expression> expr
%type<expression> cond 
%type<blk> block
%type<blk> stmt_list
%type<function_definition> func_def
%type<header> header
%type<header> func_decl
%type<declaration> local_def

%type<lval> lvalue
%type<procedure_call> proc_call
%type<function_call> func_call
%type<parameter> fpar_def
%type<param_type> fpar_type
%type<variable_declaration> var_def

%type<int_const_list> int_const_list
%type<identifier_list> id_list
%type<parameter_list> fpar_def_list
%type<decl_list> local_def_list
%type<expression_list> expr_list
%type<elif_list> elif_list

%%

program : func_def       { cout << *$1 << endl; $1->sem_analysis(); } ;

func_def : "def" header local_def_list block { $$ = new Func_Def($2,$3,$4); $$->set_line(lineno); } ;

header : T_identifier                                     { $$ = new Func_Header(new Identifier($1),definition,nullptr,nullptr); }
| T_identifier "is" data_type                             { $$ = new Func_Header(new Identifier($1),definition,$3,nullptr); }
| T_identifier ':' fpar_def fpar_def_list                 { $4->insert($4->begin(),$3); $$ = new Func_Header(new Identifier($1),definition,nullptr,$4); }
| T_identifier "is" data_type ':' fpar_def fpar_def_list  { $6->insert($6->begin(),$5); $$ = new Func_Header(new Identifier($1),definition,$3,$6); } ; 

fpar_def_list : /* epsilon */   { $$ = new vector<Parameter*>(); }
| ',' fpar_def  fpar_def_list   { $3->insert($3->begin(),$2); $$ = $3; } ;

id_list : T_identifier  { $$ = new vector<Identifier*>(); $$->push_back(new Identifier($1)); } 
| id_list T_identifier  { $1->push_back(new Identifier($2)); $$ = $1; } ;

fpar_def : id_list "as" fpar_type   { $$ = new Parameter($1,$3); } ;

data_type : "int"   { $$ = new Data_Type; *$$ = type_integer; } 
| "byte"            { $$ = new Data_Type; *$$ = type_byte; };

int_const_list : /* epsilon */        { $$ = new vector<Int_const*>(); }
| '[' T_int_const ']' int_const_list  { $4->insert($4->begin(),new Int_const($2)); $$ = $4; } ;

//type : data_type int_const_list ;

fpar_type : data_type int_const_list  { $$ = new Param_type($2,value,$1); $$->set_line(lineno); }
| "ref" data_type                     { $$ = new Param_type(nullptr,reference,$2); $$->set_line(lineno); }
| data_type '[' ']' int_const_list    { $4->insert($4->begin(),nullptr); $$ = new Param_type($4,value,$1); $$->set_line(lineno); } ;

local_def_list : /* epsilon */  { $$ = new Decl_list(); } 
| local_def_list local_def       { $1->append($2); $$ = $1; } ;

local_def : func_def  { $$ = $1; }
| func_decl           { $$ = $1; } 
| var_def             { $$ = $1; } ;

func_decl : "decl" header   { $2->set_to_decl(); $$ = $2; } ;

var_def : "var" id_list "is" data_type int_const_list { $$ = new Variable_decl($2,$5,$4); $$->set_line(lineno); } ;

stmt : "skip"                                    { /*nothing */ }
| lvalue ":=" expr                               { $$ = new Assignment($1,$3); $$->set_line(lineno); }
| proc_call                                      { $$ = $1; }
| "exit"                                         { $$ = new Exit(); $$->set_line(lineno); }
| "return" ':' expr                              { $$ = new Return($3); $$->set_line(lineno); }
| "if" cond ':' block elif_list                  { $$ = new If($2,$4,$5,nullptr); $$->set_line(lineno); }
| "if" cond ':' block elif_list "else" ':' block { $$ = new If($2,$4,$5,$8); $$->set_line(lineno); }
| "loop" ':' block                               { $$ = new Loop(nullptr,$3); $$->set_line(lineno); }
| "loop" T_identifier ':' block                  { $$ = new Loop(new Identifier($2),$4); $$->set_line(lineno); }
| "break"                                        { $$ = new Break(nullptr); $$->set_line(lineno); }
| "break" ':' T_identifier                       { $$ = new Break(new Identifier($3)); $$->set_line(lineno); }
| "continue"                                     { $$ = new Continue(nullptr); $$->set_line(lineno); }
| "continue" ':' T_identifier                    { $$ = new Continue(new Identifier($3)); $$->set_line(lineno); } ;


elif_list : /* epsilon */            { $$ = new Elif_list(); }
| elif_list "elif" cond ':' block     { $1->append($3,$5); $$ = $1; } ;


stmt_list : stmt_list stmt   { $1->append($2); $$ = $1; }
| stmt                        { $$ = new Block(); $$->append($1); } ;


block : "begin" stmt_list "end" { $$ = $2; }
| stmt_list T_auto_end          { $$ = $1; };


proc_call : T_identifier            { $$ = new Proc_call(new Identifier($1),nullptr); $$->set_line(lineno); }
| T_identifier ':' expr expr_list   { $4->insert($4->begin(),$3); $$ = new Proc_call(new Identifier($1),$4); $$->set_line(lineno); } ;


func_call : T_identifier '(' ')'       { $$ = new Func_call(new Identifier($1),nullptr); $$->set_line(lineno); }
| T_identifier '(' expr expr_list ')'  { $4->insert($4->begin(),$3); $$ = new Func_call(new Identifier($1),$4); $$->set_line(lineno); } ;


expr_list : /* epsilon */   { $$ = new vector<Expression*>(); }
| ',' expr expr_list        { $3->insert($3->begin(),$2); $$ = $3; } ;


lvalue : T_identifier       { $$ = new L_value(new Identifier($1),nullptr); $$->set_line(lineno); }
| T_string                  { $$ = new L_value(nullptr,new String_const($1)); $$->set_line(lineno); }
| lvalue '[' expr ']'       { $1->append($3); $$ = $1;} ;


expr : T_int_const          { $$ = new Int_const($1); }
| T_char_const              { $$ = new Char_const($1); }
| lvalue                    { $$ = $1; }
| '(' expr ')'              { $$ = $2; }
| func_call                 { $$ = $1; }
| '+' expr %prec UPLUS      { $$ = new Operation($2,"++",nullptr); $$->set_line(lineno); }
| '-' expr %prec UMINUS     { $$ = new Operation($2,"--",nullptr); $$->set_line(lineno); }
| expr '+' expr             { $$ = new Operation($1,"+",$3); $$->set_line(lineno); }  
| expr '-' expr             { $$ = new Operation($1,"-",$3); $$->set_line(lineno); }
| expr '*' expr             { $$ = new Operation($1,"*",$3); $$->set_line(lineno); }
| expr '/' expr             { $$ = new Operation($1,"/",$3); $$->set_line(lineno); }
| expr '%' expr             { $$ = new Operation($1,"%",$3); $$->set_line(lineno); }
| "true"                    { $$ = new Bool_const(1); }
| "false"                   { $$ = new Bool_const(0); }
| '!' expr                  { $$ = new Operation($2,"!",nullptr); $$->set_line(lineno); }
| expr '&' expr             { $$ = new Operation($1,"&",$3); $$->set_line(lineno); }
| expr '|' expr             { $$ = new Operation($1,"|",$3); $$->set_line(lineno); } ;


cond : expr                 { $$ = $1; }
| '(' cond ')'              { $$ = $2; }
| "not" cond                { $$ = new Operation($2,"not",nullptr); $$->set_line(lineno); }
| cond "and" cond           { $$ = new Operation($1,"and",$3); $$->set_line(lineno); }
| cond "or" cond            { $$ = new Operation($1,"or",$3); $$->set_line(lineno); }
| expr '=' expr             { $$ = new Operation($1,"=",$3); $$->set_line(lineno); }
| expr "<>" expr            { $$ = new Operation($1,"<>",$3); $$->set_line(lineno); }
| expr '<' expr             { $$ = new Operation($1,"<",$3); $$->set_line(lineno); }
| expr '>' expr             { $$ = new Operation($1,">",$3); $$->set_line(lineno); }
| expr "<=" expr            { $$ = new Operation($1,"<=",$3); $$->set_line(lineno); }
| expr ">=" expr            { $$ = new Operation($1,">=",$3); $$->set_line(lineno); } ;

%%

void yyerror(const char *msg, int err_line) {
    fprintf(stderr, "Error at line %d: %s\n", err_line, msg);
	exit(1);
}


int main() {
  int res = yyparse();
  if (res == 0) printf("Successful parsing\n");
  return res;
}
