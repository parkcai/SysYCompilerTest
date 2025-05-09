%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>
#include <ast.h>
// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;
std::ofstream debug_output("debug_output.txt");

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  "/*"((\*)*[^\*/]|[^\*])*(\*)*"*/"

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* 双字节运算符 */
EqOp     ==|!=
RelOp    <=|>=
AndOp    &&
OrOp     \|\|

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{BlockComment}   { /* 忽略, 不做任何操作 */ }

"int"           { debug_output<<"INT"<<endl;return INT;}
"void"          { debug_output<<"VOID"<<endl;return VOID; }
"return"        { debug_output<<"RETURN"<<endl;return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; } 

{Identifier}    { yylval.str_val = new string(yytext); debug_output<<"IDENT"<<endl; return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); debug_output<<"INT_CONST"<<endl; return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); debug_output<<"INT_CONST"<<endl; return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); debug_output<<"INT_CONST"<<endl; return INT_CONST; }

{EqOp}          { if(yytext[0] == '=') yylval.char_val = EQ; else yylval.char_val = NE; return EqOp; }
{RelOp}         { if(yytext[0] == '<') yylval.char_val = LE; else if(yytext[0] == '>') yylval.char_val = GE; return RelOp; }
{AndOp}         { yylval.char_val = 0; return AndOp; }
{OrOp}          { yylval.char_val = 0; return OrOp; }
.               { debug_output<<yytext[0]<<endl; return yytext[0];}

%%
