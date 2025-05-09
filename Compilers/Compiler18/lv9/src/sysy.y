%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include "ast.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  char char_val;
  BaseAST *ast_val;
  vector<unique_ptr<BaseAST> > *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> BTYPE IDENT
%token <int_val> INT_CONST
%token <str_val> RELOP EQOP ANDOP OROP

// 非终结符的类型定义
%type <ast_val> CompUnitItem FuncFParam FuncRParam
%type <ast_val> FuncDef Block Stmt Number Exp PrimaryExp UnaryExp MulExp AddExp
%type <ast_val> LOrExp LAndExp EqExp RelExp
%type <ast_val> BlockItem Decl VarDecl ConstDecl VarDef ConstDef InitVal ConstInitVal ConstExp LVal
%type <vec_val> ConstDefs VarDefs BlockItems CompUnitItems FuncFParams FuncRParams
%type <vec_val> Exps ConstExps ArrayVarIndexs ArrayConstIndexs
%type <char_val> UnaryOp MulOp AddOp

// 优先级设计, 越后面优先级越高， 有else大于没else
%precedence IFWITHNOELSE
%precedence ELSE

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : CompUnitItems {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->comp_unit_items = unique_ptr<vector<unique_ptr<BaseAST> > >($1);;
    ast = move(comp_unit);
  }

CompUnitItems
  : CompUnitItem {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CompUnitItems CompUnitItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

CompUnitItem
  : FuncDef {
    auto ast = new CompUnitItemAST();
    ast->type = 1;
    ast->funcdef = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitItemAST();
    ast->type = 2;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : BTYPE IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

// int x | int x[] | int x[][10]
// 第一维省略
FuncFParam
  : BTYPE IDENT {
    auto ast = new FuncFParamAST();
    ast->type = 1;
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | BTYPE IDENT '[' ']' ArrayConstIndexs{
    auto ast = new FuncFParamAST();
    ast->type = 2;
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->array_indexs = unique_ptr<vector<unique_ptr<BaseAST> > >($5);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItems '}' {
    auto ast = new BlockAST();
    ast->blockitems = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

BlockItems
  : {

    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | BlockItems BlockItem{

    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->type = 1;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->type = 2;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->type = 1;
    ast->constdecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->type = 2;
    ast->vardecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDecl
  : BTYPE VarDefs ';' {
    auto ast = new VarDeclAST();
    ast->btype = *unique_ptr<string>($1);
    ast->vardefs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }

VarDefs
  : VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefs ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

// VarDef        ::= IDENT | IDENT "=" InitVal;
VarDef
  : IDENT ArrayConstIndexs{
    auto ast = new VarDefAST();
    ast->type = 1;
    ast->ident = *unique_ptr<string>($1);
    ast->array_indexs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;

  }
  | IDENT ArrayConstIndexs '=' InitVal {
    auto ast = new VarDefAST();
    ast->type = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->array_indexs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } 
  | '{' Exps '}' {
    auto ast = new InitValAST();
    ast->type = 2;
    ast->exps = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

Exps
  : InitVal {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | Exps ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  ;

// ConstDefs -> ConstDef {"," ConstDef}
ConstDecl
  : CONST BTYPE ConstDefs ';' {
    auto ast = new ConstDeclAST();
    ast->btype = *unique_ptr<string>($2);
    ast->constdefs = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

ConstDefs
  : ConstDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefs ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT ArrayConstIndexs '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array_indexs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ArrayConstIndexs
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | ArrayConstIndexs '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->type = 1;
    ast->constexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstExps '}' {
    auto ast = new ConstInitValAST();
    ast->type = 2;
    ast->constexps = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

// ConstExps -> ConstExp {"," ConstExp}
ConstExps 
  : ConstInitVal {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstExps ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = 2;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = 3;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->type = 4;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = 5;
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = 6;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt %prec IFWITHNOELSE {
    auto ast = new StmtAST();
    ast->type = 7;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new StmtAST();
    ast->type = 8;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = 9;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->while_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->type = 10;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->type = 11;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->number = $1;
    $$ = ast;
  }
  ;

Exp 
  : LOrExp {
    auto ast = new ExpAST();
    ast->lorexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp 
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 1;
    ast->landexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OROP LAndExp{
    auto ast = new LOrExpAST();
    ast->type = 2;
    ast->lorexp = unique_ptr<BaseAST>($1);
    ast->landexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->type = 1;
    ast->eqexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp ANDOP EqExp{
    auto ast = new LAndExpAST();
    ast->type = 2;
    ast->landexp = unique_ptr<BaseAST>($1);
    ast->eqexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->type = 1;
    ast->relexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->type = 2;
    ast->eqexp = unique_ptr<BaseAST>($1);
    ast->eqop = *unique_ptr<string>($2);
    ast->relexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type = 1;
    ast->addexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->type = 2;
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->relop = *unique_ptr<string>($2);
    ast->addexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->type = 1;
    ast->mulexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp{
    auto ast = new AddExpAST();
    ast->type = 2;
    ast->addexp = unique_ptr<BaseAST>($1);
    ast->addop = $2;
    ast->mulexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->type = 1;
    ast->unaryexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp{
    auto ast = new MulExpAST();
    ast->type = 2;
    ast->mulexp = unique_ptr<BaseAST>($1);
    ast->mulop = $2;
    ast->unaryexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 1;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 2;
    ast->unaryop = $1;
    ast->content = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->type = 3;
    ast->ident = *unique_ptr<string>($1);
    ast->func_rparams = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

FuncRParams
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | FuncRParams ',' FuncRParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | FuncRParam {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

FuncRParam
  : Exp {
    auto ast = new FuncRParamAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;


PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->type = 1;
    ast->content = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type = 2;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = 3;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ArrayVarIndexs
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | ArrayVarIndexs '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

LVal
  : IDENT ArrayVarIndexs{
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array_indexs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  | '!' {
    $$ = '!';
  }
  ;

MulOp
  : '*' {
    $$ = '*';
  }
  | '/' {
    $$ = '/';
  }
  | '%' {
    $$ = '%';
  }
  ;

AddOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  extern int yylineno;
  extern char *yytext;
  int len = strlen(yytext);
  char buf[512] = {0};
  for (int i=0; i<len; ++i)
    sprintf(buf, "%s%d ", buf, yytext[i]);
  fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}