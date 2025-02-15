
%{

#define T_eof       0
#define T_and       500
#define T_as        501
#define T_begin     502
#define T_break     503
#define T_byte      504
#define T_continue  505
#define T_decl      506
#define T_def       507
#define T_elif      508
#define T_else      509
#define T_end       510
#define T_exit      511
#define T_false     512
#define T_if        513
#define T_is        514
#define T_int       515
#define T_loop      516
#define T_not       517
#define T_or        518
#define T_ref       519
#define T_return    520
#define T_skip      521
#define T_true      522
#define T_var       523

#define T_not_equal 524
#define T_less_or_equal 525
#define T_more_or_equal 526

#define T_identifier 527
#define T_int_const  528
#define T_string     529

%}

D [0-9]
L [a-zA-Z]
W [ \n\t\r]
S [\x20a-zA-Z0-9\t!@#$%^&*()-=+<>?;:,./|\\\']

%%

"and"  { return T_and; }
"as"  { return T_as; }
"begin"  { return T_begin; }
"break"  { return T_break; }
"byte"  { return T_byte; }
"continue"  { return T_continue; }
"decl"  { return T_decl; }
"def"  { return T_def; }
"elif"  { return T_elif; }
"else"  { return T_else; }
"end"  { return T_end; }
"exit"  { return T_exit; }
"false"  { return T_false; }
"if"  { return T_if; }
"is"  { return T_is; }
"int"  { return T_int; }
"loop"  { return T_loop; }
"not"  { return T_not; }
"or"  { return T_or; }
"ref"  { return T_ref; }
"return"  { return T_return; }
"skip"  { return T_skip; }
"true"  { return T_true; }
"var"  { return T_var; }

[\+\-\*\/\%\!\&\|\=\<\>\(\)\[\]\,\:]   { return yytext[0]; }
"<>"                       { return T_not_equal; }
"<="			   { return T_less_or_equal; }
">="			   { return T_more_or_equal; }

{L}({L}|{D}|_)*            { return T_identifier; }
{D}+			   { return T_int_const; }


\"({S}|\\\")*\n?\" { return T_string; }

{W}+    		   { /*nothing*/ }

#.*			   { /*one-line-comment*/ }
\(\*(.|\n)*\*\)            { /*multi-line-commnet*/}
 
.                          { fprintf(stderr,"Invalid token\n");}

%%

int main () {
	int token;
	do {
		token = yylex();
		printf("token = %d, lexeme = %s\n",token,yytext);
	} while (token!=T_eof);
	return 0;
}
