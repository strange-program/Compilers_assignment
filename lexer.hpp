#ifndef LEXER_HPP
#define LEXER_HPP
extern "C" int yylex();
void yyerror(const char *msg, int err_line);

inline void yyerror(const char *msg) {
    extern int lineno;
    yyerror(msg, lineno);
}

#endif // LEXER_HPP

