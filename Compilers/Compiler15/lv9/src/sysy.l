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

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  "\/\*"[^*]*\*+([^*/][^*]*\*+)*\/

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{BlockComment}  { /* 忽略, 不做任何操作 */ }

"int"           { return INT; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"continue"      { return CONTINUE; }
"break"         { return BREAK; }
"void"          { return VOID; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

"<"             { yylval.str_val = new string(yytext); return LT; }        // 小于号
">"             { yylval.str_val = new string(yytext); return GT; }        // 大于号
"-"             { yylval.str_val = new string(yytext); return MINOR; }    // 负号
"+"             { yylval.str_val = new string(yytext); return PLUS; }     // 加号
"*"             { yylval.str_val = new string(yytext); return MUL; }      // 乘号
"/"             { yylval.str_val = new string(yytext); return DIV; }      // 除号
"%"             { yylval.str_val = new string(yytext); return MOD; }      // 取模
"!"             { yylval.str_val = new string(yytext); return NOT; }      // 逻辑非

"<="            { yylval.str_val = new string(yytext); return LE; }        // 小于等于
">="            { yylval.str_val = new string(yytext); return GE; }        // 大于等于
"=="            { yylval.str_val = new string(yytext); return EQ; }        // 等于
"!="            { yylval.str_val = new string(yytext); return NE; }        // 不等于
"&&"            { yylval.str_val = new string(yytext); return LAND; }      // 逻辑与
"||"            { yylval.str_val = new string(yytext); return LOR; }       // 逻辑或

.               { return yytext[0]; } /* 返回单个字符 */

%%
