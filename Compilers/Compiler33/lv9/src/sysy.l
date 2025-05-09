%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>


// file from bison 
#include "sysy.tab.hpp"

using namespace std;

%}
/* blanks */
WhiteSpace      [ \t\n\r]*
LineComment     "//".*
BlockComment    \/\*[^*]*\*+([^*/][^*]*\*+)*\/ 

/* identifiers */
Identifier      [a-zA-Z_][a-zA-Z0-9_]*

/* literals */
Decimal         [1-9][0-9]*
Octal           0[0-7]*
Hexadecimal     0[xX][0-9a-fA-F]*

/* operators */
Add             "+"
Sub             "-"
Div             "/"
Mul             "*"
Mod             "%"
Greater         ">"
Less            "<"
Not             "!"
Equal           "=="
GEqual          ">="
LEqual          "<="
NEqual          "!="
And             "&&"
Or              "||"


%%

{WhiteSpace}    {/* nothing */ }
{LineComment}   {/* nothing */ }
{BlockComment}  {/* nothing */}

"int"           { return INT; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }
"void"          { return VOID; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, 0, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, 0, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, 0, 0); return INT_CONST; }

{Equal}         { yylval.char_val = 'E'; return EQUAL; }
{GEqual}        { yylval.char_val = 'G'; return GEQUAL; }
{LEqual}        { yylval.char_val = 'L'; return LEQUAL; }
{NEqual}        { yylval.char_val = 'N'; return NEQUAL; }
{And}           { yylval.char_val = '&'; return AND; }
{Or}            { yylval.char_val = '|'; return OR; } 

{Add}           { yylval.char_val = '+'; return ADD; }
{Sub}           { yylval.char_val = '-'; return SUB; }
{Div}           { yylval.char_val = '/'; return DIV; }
{Mul}           { yylval.char_val = '*'; return MUL; }
{Mod}           { yylval.char_val = '%'; return MOD; }
{Greater}       { yylval.char_val = '>'; return GREATER; }
{Less}          { yylval.char_val = '<'; return LESS; }
{Not}           { yylval.char_val = '!'; return NOT; }

.               { return yytext[0]; }

%%