%option noyywrap
%option nounput
%option noinput

%{
#include <cstdlib>
#include <string>

#include "sysy.tab.hpp"

using namespace std;
%}

WhiteSpace      [ \t\n\r]*
LineComment     "//".*
MultiLineComment \/\*([^*]|(\*+[^*/]))*\*+\/
Identifier      [a-zA-Z_][a-zA-Z0-9_]*

UnaryOperator \!
AddOperator   \+|\-
MulOperator   \*|\/|\%
RelOperator   <|>|<=|>=
EqOperator    ==|\!=
LAndOperator  &&
LOrOperator   \|\|

Decimal         [1-9][0-9]*
Octal           0[0-7]*
Hexadecimal     0[xX][0-9a-fA-F]+

%%
{WhiteSpace}    {}
{LineComment}   {}
{MultiLineComment} {}
"int"           { return INT; }
"void"           { return VOID; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{UnaryOperator}      { yylval.str_val = new string(yytext); return UNARYOP; }
{AddOperator}      { yylval.str_val = new string(yytext); return ADDOP; }
{MulOperator}      { yylval.str_val = new string(yytext); return MULOP; }
{RelOperator}      { yylval.str_val = new string(yytext); return RELOP; }
{EqOperator}      { yylval.str_val = new string(yytext); return EQOP; }
{LOrOperator}      { yylval.str_val = new string(yytext); return LOROP; }
{LAndOperator}      { yylval.str_val = new string(yytext); return LANDOP; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 10); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 8); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 16); return INT_CONST; }

.               { return yytext[0]; }

%%
