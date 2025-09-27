%{
#include <string>
#include <stack>
#include <cstdlib>  // for strtol
#include <cstring>  // for strlen

#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"

#define T_eof 0

using namespace std;

int lineno = 1;
int lexical_errors = 0;

int indent = 0;
int leader_indent = 0; 
int auto_end_pending = 0;
int tablen = 8;

stack<int> indentstack;
stack<int> blocktype_stack;

char get_char (char* str) {
	if (str[1]=='\\') {
		if (str[2]=='n') return '\n';
		else if (str[2]=='t') return '\t';
		else if (str[2]=='r') return '\r';
		else if (str[2]=='0') return '\0';
		else if (str[2]=='\'') return '\'';
		else if (str[2]=='\"') return '\"';
		else {
			return static_cast<char>(strtol(str + 3, nullptr, 16));
		}
	}
	else return str[1];
}

%}

D [0-9]
L [a-zA-Z]
W [ \t\r]
S [\x20a-zA-Z0-9\t!@#$%^&*()-=+<>?;:,./|\\\']

%%

"and"       { leader_indent+= yyleng; return T_and; }
"as"        { leader_indent+= yyleng; return T_as; }

"begin"     { 

	leader_indent+= yyleng; 
	if(!indentstack.empty()) indentstack.pop();
	if(!blocktype_stack.empty()) blocktype_stack.pop();
	indentstack.push(0);
	blocktype_stack.push(0);
	return T_begin; 
}

"break"     { leader_indent+= yyleng; return T_break; }
"byte"      { leader_indent+= yyleng; return T_byte; }
"continue"  { leader_indent+= yyleng; return T_continue; }
"decl"      { leader_indent+= yyleng; return T_decl; }
"def"       { indentstack.push(leader_indent); blocktype_stack.push(1); leader_indent+= yyleng; return T_def; }
"elif"      { indentstack.push(leader_indent); blocktype_stack.push(1); leader_indent+= yyleng; return T_elif; }
"else"      { indentstack.push(leader_indent); blocktype_stack.push(1); leader_indent+= yyleng; return T_else; }

"end"       { 
	leader_indent+= yyleng; 
	if (!indentstack.empty()) indentstack.pop(); 
	if (!blocktype_stack.empty()) blocktype_stack.pop(); 
	return T_end; 
}

"exit"      { leader_indent+= yyleng; return T_exit; }
"false"     { leader_indent+= yyleng; return T_false; }
"if"        { indentstack.push(leader_indent); blocktype_stack.push(1); leader_indent+= yyleng; return T_if; }
"is"        { leader_indent+= yyleng; return T_is; }
"int"       { leader_indent+= yyleng; return T_int; }
"loop"      { indentstack.push(leader_indent); blocktype_stack.push(1); leader_indent+= yyleng; return T_loop; }
"not"       { leader_indent+= yyleng; return T_not; }
"or"        { leader_indent+= yyleng; return T_or; }
"ref"       { leader_indent+= yyleng; return T_ref; }
"return"    { leader_indent+= yyleng; return T_return; }
"skip"      { leader_indent+= yyleng; return T_skip; }
"true"      { leader_indent+= yyleng; return T_true; }
"var"       { leader_indent+= yyleng; return T_var; }
"<>"        { leader_indent+= yyleng; return T_not_equal; }
"<="        { leader_indent+= yyleng; return T_less_or_equal; }
">="	    { leader_indent+= yyleng; return T_more_or_equal; }
":="        { leader_indent+= yyleng; return T_assign; }

<<EOF>>     { 

	if (lexical_errors>0) exit(1);
	
	auto_end_pending = indentstack.size();
	indentstack = stack<int>();     // Clear the stacks
	blocktype_stack = stack<int>();

	if (auto_end_pending > 0) return T_auto_end;
	else return T_eof;
    
}

{L}({L}|{D}|_)*     { leader_indent+= yyleng; yylval.identifier = strdup(yytext); return T_identifier; }
{D}+			    { leader_indent+= yyleng; yylval.int_const = atoi(yytext); return T_int_const; }

\"(\\[ntr0\\'"]|\\x[0-9a-fA-F]{2}|[^\\'"\n])*\"  { leader_indent+= yyleng; yylval.string_const = strdup(yytext); return T_string; }
\'(\\[ntr0\\'"]|\\x[0-9a-fA-F]{2}|[^\\'"\n])\'   { leader_indent+= yyleng; yylval.char_const = get_char(yytext); return T_char_const; }

\n({W})*/(begin|end|\#|[(][*]|\n)   {
	lineno++; 
  	indent = 0;
  	
	for (int i=0; i<yyleng; i++) {
		if (yytext[i]=='\t') indent += (tablen - (indent%tablen));	
		else if (yytext[i]=='\r') indent = 0;
		else indent++;
  	}

  	leader_indent = indent;
}
 

\n({W})* {
  
	lineno++; 
  	indent = 0;
  	
	for (int i=0; i<yyleng; i++) {
		if (yytext[i]=='\t') indent += (tablen - (indent%tablen));	
		else if (yytext[i]=='\r') indent = 0;
		else indent++;
  	}

  	leader_indent = indent;

  	if (!blocktype_stack.empty() && !indentstack.empty()) {
  		if (blocktype_stack.top() == 1 && indentstack.top() >= indent) {
			for (int i=yyleng-1; i>=0; i--) {
				unput(yytext[i]);
			}

			lineno--;			

			blocktype_stack.pop();
			indentstack.pop();
			return T_auto_end;
			
  		}
	}
}
  

[\+\-\*\/\%\!\&\|\=\<\>\(\)\[\]\,\:]   { leader_indent+= yyleng; return yytext[0]; }

{W}+        { 
	
	/*nothing*/ 
	for (int i=0; i<yyleng; i++) {
		if (yytext[i]=='\t') leader_indent += (tablen - (leader_indent%tablen));	
		else if (yytext[i]=='\r') leader_indent = 0;
		else leader_indent++;
	}
}

#.*	        { leader_indent+= yyleng; /*one-line-comment*/ }

[(][*][^*]*([*]+[^)][^*]*)*[*]+[)] { 

	/*multi-line-comment*/
	for (int i=0; i<yyleng; i++) {
		if (yytext[i]=='\n') lineno++;
	}

}

.               {

	if (auto_end_pending > 0) {
        	auto_end_pending--;
        	return T_auto_end;
    	}
  	
	fprintf(stderr, "Lexical Error at line %d: invalid character '%c' (ASCII %d)\n", lineno,yytext[0], yytext[0]);
	lexical_errors++;
 }

%%

