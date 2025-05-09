%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>
// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;

%}

WhiteSpace    [ \t\n\r]*
LineComment   "//".*
CommentBegin  "/*"
CommentMid    ".|[\r\n]"
CommentEnd    "*/"
Identifier    [a-zA-Z_][a-zA-Z0-9_]*
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+
RelOp         "<="|">="|"<"|">"  
EqOp          "=="|"!="
LandOp        "&&"
LorOp         "||"

%x COMMENT

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{CommentBegin}  {BEGIN COMMENT; }
<COMMENT>{CommentMid}   { /* 忽略, 不做任何操作 */ }
<COMMENT>{CommentEnd}   {BEGIN INITIAL; }

"int"           { return INT; }
"void"          { return VOID; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }


{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{RelOp}         { yylval.str_val = new string(yytext); return RELOP; }
{EqOp}          { yylval.str_val = new string(yytext); return EQOP; }
{LandOp}        { yylval.str_val = new string(yytext); return LANDOP; }
{LorOp}         { yylval.str_val = new string(yytext); return LOROP; }

.               { return yytext[0]; }

%%

// 这里写一些用户自定义的代码
// 比如你希望在生成的 C/C++ 文件里定义一个函数, 做一些辅助工作
// 你同时希望在之前的规则描述里调用你定义的函数
// 那么, 你可以把 C/C++ 的函数定义写在这里, 声明写在文件开头
