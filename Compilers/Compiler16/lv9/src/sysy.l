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

/* 定义开始条件 */
%x COMMENT

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]+
LineComment   "//".*

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%
{WhiteSpace}    { /* 忽略空白符 */ }
{LineComment}   { /* 忽略单行注释 */ }
"/*"            { BEGIN(COMMENT); }  /* 进入多行注释状态 */

<COMMENT>{
  "*/"          { BEGIN(INITIAL); }  /* 退出多行注释状态 */
  .             { /* 忽略其他字符 */ }
  \n            { /* 忽略换行符 */ }
  <<EOF>>       { /* 处理文件结束符，防止注释未闭合 */
                    fprintf(stderr, "Error: Unterminated comment\n");
                    exit(1);
                }
}

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

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

"=="            { return EQ; }
"!="            { return NEQ; }
"<="            { return LE; }
">="            { return GE; }
"&&"            { return AND; }
"||"            { return OR; }
"<"             { return LT; }
">"             { return GT; }
"+"             { return PLUS; }
"-"             { return MINUS; }
"!"             { return NOT; }
"*"             { return MUL; }
"/"             { return DIV; }
"%"             { return MOD; }

.               { return yytext[0]; }

%%
