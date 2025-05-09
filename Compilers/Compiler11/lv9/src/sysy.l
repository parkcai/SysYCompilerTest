%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>
#include "sysy.tab.hpp"

using namespace std;

%}

WhiteSpace    [ \t\n\r]*
Comment         "//"[^\n]*
MultiLineComment    \/\*(?:[^\*]|\*+[^\/\*])*\*+\/

Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{Comment}       { /* 忽略, 不做任何操作 */ }
{MultiLineComment}  { /* 忽略, 不做任何操作 */ }

"int"           {return TInt;}
"return"        {return TReturn;}

"||"            {return TLogicOr;}
"&&"            {return TLogicAnd;}

"const"         {return TCst;}

"if"            {return TIf;}
"else"          {return TElse;}
"while"         {return TWhile;}

"break"         {return TBreak;}
"continue"      {return TContinue;}

"void"          {return TVoid;}

{Identifier}    {yylval.strDef = new string(yytext);return TIdent;}
{Decimal}       {yylval.intDef = strtol(yytext,nullptr,0);return TIntCst;}
{Octal}         {yylval.intDef = strtol(yytext,nullptr,0);return TIntCst;}
{Hexadecimal}   {yylval.intDef = strtol(yytext,nullptr,0);return TIntCst;}

.               { return yytext[0]; }

%%