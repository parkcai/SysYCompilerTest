%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{
#include "ast.h"
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
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
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN NE EQ GE LE AND OR IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT CONST
%token <int_val> INT_CONST

// 非终结符的类型定义
//%type <str_val> FuncDef FuncType Block Stmt Number
%type <ast_val> GlobalCompUnit CompUnit FuncDef Block Stmt Exp PrimaryExp UnaryExp AddExp MulExp LOrExp RelExp EqExp LAndExp Decl ConstDecl ConstDef ConstDefs ConstInitVal VarDecl VarDef VarDefs InitVal BlockItem BlockItems LVal ConstExp MatchedStmt UnmatchedStmt FuncFParams FuncFParam FuncRParams ExpList ConstExpList InitValList ConstInitValList
%type <char_val> UnaryOp
%type <int_val> Number
%type <str_val> BType

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值

GlobalCompUnit
  : CompUnit {
    auto compunit = unique_ptr<CompUnitAST>((CompUnitAST*)$1);
    ast = move(compunit);
  }
  ;

CompUnit
  : CompUnit FuncDef {
    auto ast = new CompUnitAST();
    auto compunit = unique_ptr<CompUnitAST>((CompUnitAST*)$1);
    auto funcdef = unique_ptr<BaseAST>($2);
    for (int i=0; i<compunit->funcdefs.size();i++){
      ast->funcdefs.emplace_back(move(compunit->funcdefs[i]));
    }
    for (int i=0; i<compunit->decls.size();i++){
      ast->decls.emplace_back(move(compunit->decls[i]));
    }
    ast->funcdefs.emplace_back(move(funcdef));
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitAST();
    auto funcdef = unique_ptr<BaseAST>($1);
    ast->funcdefs.emplace_back(move(funcdef));
    $$ = ast;
  }
  | CompUnit Decl {
    auto ast = new CompUnitAST();
    auto compunit = unique_ptr<CompUnitAST>((CompUnitAST*)$1);
    auto decl = unique_ptr<DeclAST>((DeclAST*)$2);
    for (int i=0; i<compunit->funcdefs.size();i++){
      ast->funcdefs.emplace_back(move(compunit->funcdefs[i]));
    }
    for (int i=0; i<compunit->decls.size();i++){
      ast->decls.emplace_back(move(compunit->decls[i]));
    }
    ast->decls.emplace_back(move(decl));
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitAST();
    auto decl = unique_ptr<DeclAST>((DeclAST*)$1);
    ast->decls.emplace_back(move(decl));
    $$ =ast;
  }
  ;




Decl
  :ConstDecl {
    auto ast = new DeclAST();
    ast->constdecl = unique_ptr<ConstDeclAST>((ConstDeclAST*)$1);
    ast->catagory = 1;
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->vardecl = unique_ptr<VarDeclAST>((VarDeclAST*)$1);
    ast->catagory = 2;
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefs ';'{
    auto ast = new ConstDeclAST();
    ast->btype = *unique_ptr<string>($2);
    ast->constdefs = unique_ptr<ConstDefsAST>((ConstDefsAST*)$3);
    $$ = ast;
  }
  ;
BType
  : INT {
    $$ = new string("int");
  }
  | VOID {
    $$ = new string("void");
  }
  ;

ConstDef      
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->catagory = 1;
    ast->ident = "@"+*unique_ptr<string>($1);
    ast->constinitval = unique_ptr<ConstInitValAST>((ConstInitValAST*)$3);
    $$ = ast;
  }
  | IDENT ConstExpList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->catagory = 2;
    ast->ident = "@" +*unique_ptr<string>($1);
    auto list = unique_ptr<ConstDefAST>((ConstDefAST*)$2);
    for (int i=0; i<list->constexps.size();i++){
      ast->constexps.emplace_back(move(list->constexps[i]));
    }
    ast->constinitval = unique_ptr<ConstInitValAST>((ConstInitValAST*)$4);
    $$ = ast;
  }
  ;

ConstExpList 
  : ConstExpList '[' ConstExp ']' {
    auto ast = new ConstDefAST();
    ast->catagory =3;
    auto list =  unique_ptr<ConstDefAST>((ConstDefAST*)$1);
    auto constexp = unique_ptr<BaseAST>($3);
    for (int i=0; i<list->constexps.size();i++){
      ast->constexps.emplace_back(move(list->constexps[i]));
    }
    ast->constexps.emplace_back(move(constexp));
    $$ = ast;
  }
  | '[' ConstExp ']'{
    auto ast = new ConstDefAST();
    ast->catagory =3;
    auto constexp = unique_ptr<BaseAST>($2);
    ast->constexps.emplace_back(move(constexp));
    $$ = ast;
  }
  ;

ConstDefs 
  : ConstDef {
    auto ast = new ConstDefsAST();
    ast->catagory =1;
    ast->constdef = unique_ptr<ConstDefAST>((ConstDefAST*)$1);
    $$ = ast;
  }
  | ConstDef ',' ConstDefs {
    auto ast = new ConstDefsAST();
    ast->catagory =2;
    ast->constdef = unique_ptr<ConstDefAST>((ConstDefAST*)$1);
    ast->constdefs = unique_ptr<ConstDefsAST>((ConstDefsAST*)$3);
    $$ = ast;
  }
  ;

ConstInitVal  
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->catagory = 1;
    ast->constexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST();
    ast->catagory = 2;
    auto list =  unique_ptr<ConstInitValAST>((ConstInitValAST*)$2);
    for (int i=0; i<list->constinitvals.size();i++){
      ast->constinitvals.emplace_back(move(list->constinitvals[i]));
    }
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast->catagory = 3;
    $$ = ast;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto ast = new ConstInitValAST();
    ast->catagory = 2;
    ast->constinitvals.emplace_back(unique_ptr<ConstInitValAST>((ConstInitValAST*)$1));
    $$ = ast;
  }
  | ConstInitValList ',' ConstInitVal {
    auto ast = new ConstInitValAST();
    ast->catagory = 2;
    auto list =  unique_ptr<ConstInitValAST>((ConstInitValAST*)$1);
    for (int i=0; i<list->constinitvals.size();i++){
      ast->constinitvals.emplace_back(move(list->constinitvals[i]));
    }
    ast->constinitvals.emplace_back(unique_ptr<ConstInitValAST>((ConstInitValAST*)$3));
    $$ = ast;
  }
  ;

VarDecl       
  : BType VarDefs ';'{
    auto ast = new VarDeclAST();
    ast->btype = *unique_ptr<string>($1);
    ast->vardefs = unique_ptr<VarDefsAST>((VarDefsAST*)$2);
    $$ = ast;
  }
  ;

VarDef        
  : IDENT {
    auto ast = new VarDefAST();
    ast->catagory =1;
    ast->ident ="@" +*unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->catagory =2;
    ast->ident ="@"+ *unique_ptr<string>($1);
    ast->initval = unique_ptr<InitValAST>((InitValAST*)$3);
    $$ = ast;
  }
  | IDENT ConstExpList {
    auto ast = new VarDefAST();
    ast->catagory = 3;
    ast->ident = "@" + *unique_ptr<string>($1);
    auto list = unique_ptr<ConstDefAST>((ConstDefAST*)$2);
     for (int i=0; i<list->constexps.size();i++){
      ast->constexps.emplace_back(move(list->constexps[i]));
    }
    $$ = ast;
  }
  |  IDENT ConstExpList '=' InitVal {
    auto ast = new VarDefAST();
    ast->catagory =4;
    ast->ident ="@"+ *unique_ptr<string>($1);
    auto list = unique_ptr<ConstDefAST>((ConstDefAST*)$2);
     for (int i=0; i<list->constexps.size();i++){
      ast->constexps.emplace_back(move(list->constexps[i]));
    }
    ast->initval = unique_ptr<InitValAST>((InitValAST*)$4);
    $$ = ast;
  }
  ;

VarDefs
  : VarDef {
    auto ast = new VarDefsAST();
    ast->catagory = 1;
    ast->vardef = unique_ptr<VarDefAST>((VarDefAST*)$1);
    $$ = ast;
  }
  | VarDef ',' VarDefs {
    auto ast = new VarDefsAST();
    ast->catagory = 2;
    ast->vardef = unique_ptr<VarDefAST>((VarDefAST*)$1);
    ast->vardefs = unique_ptr<VarDefsAST>((VarDefsAST*)$3);
    $$ = ast;
  }
  ;

InitVal       
  : Exp {
    auto ast = new InitValAST();
    ast->catagory = 1;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST();
    ast->catagory = 2;
    auto list = unique_ptr<InitValAST>((InitValAST*)$2);
    for (int i=0;i<list->initvals.size();i++){
      ast->initvals.emplace_back(move(list->initvals[i]));
    }
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->catagory = 3;
    $$ = ast;
  };

InitValList
  : InitVal {
    auto ast = new InitValAST();
    auto init =unique_ptr<InitValAST>((InitValAST*)$1);
    ast->catagory = 2;
    ast->initvals.emplace_back(move(init));
    $$ = ast;
  } | InitValList ',' InitVal {
    auto ast = new InitValAST();
    ast->catagory=2;
    auto list = unique_ptr<InitValAST>((InitValAST*)$1);
    for (int i=0;i<list->initvals.size();i++){
      ast->initvals.emplace_back(move(list->initvals[i]));
    }
    auto init = unique_ptr<InitValAST>((InitValAST*)$3);
    ast->initvals.emplace_back(move(init));
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
  : BType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->catagory =1;
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = "@"+*unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BType IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->catagory = 2;
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = "@"+*unique_ptr<string>($2);
    ast->funcfparams = unique_ptr<FuncFParamsAST>((FuncFParamsAST*)$4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto ast = new FuncFParamsAST();
    ast->catagory =1;
    ast->funcfparam = unique_ptr<FuncFParamAST>((FuncFParamAST*)$1);
    $$ = ast;
  }
  | FuncFParam ',' FuncFParams {
    auto ast = new FuncFParamsAST();
    ast->catagory = 2;
    ast->funcfparam = unique_ptr<FuncFParamAST>((FuncFParamAST*)$1);
    ast->funcfparams = unique_ptr<FuncFParamsAST>((FuncFParamsAST*)$3);
    $$ = ast;
  }
  ;

FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->catagory =1;
    ast->btype = *unique_ptr<string>($1);
    ast->ident = "@"+*unique_ptr<string>($2);
    $$ = ast;
  }
  | BType IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->catagory = 2;
    ast->btype = *unique_ptr<string>($1);
    ast->ident = "@"+*unique_ptr<string>($2);
    $$ = ast;
  }
  | BType IDENT '[' ']' ConstExpList {
    auto ast = new FuncFParamAST();
    ast->catagory = 3;
    ast->btype = *unique_ptr<string>($1);
    ast->ident = "@"+*unique_ptr<string>($2);
    auto list =  unique_ptr<ConstDefAST>((ConstDefAST*)$5);
    for (int i=0; i<list->constexps.size();i++){
      ast->constexps.emplace_back(move(list->constexps[i]));
    }
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->catagory =1;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ =ast;
  }
  | Exp ',' FuncRParams {
    auto ast = new FuncRParamsAST();
    ast->catagory = 2;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->funcrparams = unique_ptr<FuncRParamsAST>((FuncRParamsAST*)$3);
    $$ =ast;
  }
  ;
// 同上, 不再解释


Block
  : '{' BlockItems '}' {
    auto ast = new BlockAST();
    ast->catagory =1;
    ast->blockitems = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '{' '}'{
    auto ast = new BlockAST();
    ast->catagory =2;
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->catagory =1;
    ast->decl =unique_ptr<DeclAST>((DeclAST*)$1);
    $$ = ast;
  }
  | Stmt{
    auto ast = new BlockItemAST();
    ast->catagory = 2;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

BlockItems 
  : BlockItem {
    auto ast = new BlockItemsAST();
    ast->catagory = 1;
    ast->blockitem=unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | BlockItem BlockItems {
    auto ast = new BlockItemsAST();
    ast->catagory = 2;
    ast->blockitem=unique_ptr<BaseAST>($1);
    ast->blockitems=unique_ptr<BaseAST>($2);
    $$ = ast;
  };

Stmt
  : MatchedStmt {
    $$ = $1;
  }
  | UnmatchedStmt {
    $$ = $1;
  }
  ;

MatchedStmt
  : LVal '=' Exp ';'{
    auto ast = new StmtAST();
    ast->catagory = 1;
    ast->lval=unique_ptr<LValAST>((LValAST*)$1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  |RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->catagory = 2;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  |Block{
    auto ast = new StmtAST();
    ast->block=unique_ptr<BaseAST>($1);
    ast->catagory =3;
    $$ = ast;
  }
  |RETURN ';'{
    auto ast = new StmtAST();
    ast->catagory =4;
    $$ = ast;
  }
  |Exp ';'{
    auto ast = new StmtAST();
    ast->catagory =5;
    ast->exp=unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';'{
    auto ast = new StmtAST();
    ast->catagory =6;
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new StmtAST();
    ast->catagory =7;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->ifstmt = unique_ptr<BaseAST>($5);
    ast->elsestmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->catagory = 9;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';'{
    auto ast = new StmtAST();
    ast->catagory = 10;
    $$ = ast;
  }
  | CONTINUE ';'{
    auto ast = new StmtAST();
    ast->catagory = 11;
    $$ = ast;

  }
  ;

UnmatchedStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->catagory =8;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->ifstmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE UnmatchedStmt {
    auto ast = new StmtAST();
    ast->catagory =7;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->ifstmt = unique_ptr<BaseAST>($5);
    ast->elsestmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->catagory = 1;
    ast->ident = "@"+*unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ExpList {
    auto ast = new LValAST();
    ast->catagory = 2;
    ast->ident = "@"+*unique_ptr<string>($1);
    auto list = unique_ptr<ExpAST>((ExpAST*)$2);
    for (int i=0; i<list->exps.size();i++){
      ast->exps.emplace_back(move(list->exps[i]));
    }
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

ExpList
  : '[' Exp ']' {
    auto ast = new ExpAST();
    ast->exps.emplace_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | ExpList '[' Exp ']' {
    auto ast = new ExpAST();
    auto list = unique_ptr<ExpAST>((ExpAST*)$1);
    for (int i=0; i<list->exps.size();i++){
      ast->exps.emplace_back(move(list->exps[i]));
    }
    ast->exps.emplace_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast= new PrimaryExpAST();
    ast->catagory = 1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$=ast;
  }
  |LVal {
    auto ast = new PrimaryExpAST();
    ast->catagory = 3;
    ast->lval = unique_ptr<LValAST>((LValAST*)$1);
    $$ = ast;
  }
  | Number {
    auto ast= new PrimaryExpAST();
    ast->catagory = 2;
    ast->number = int($1);
    $$=ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->catagory =1;
    ast->primaryexp = unique_ptr<BaseAST>($1);
    $$=ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->catagory =2;
    ast->unaryop = char($1);
    ast->unaryexp = unique_ptr<BaseAST>($2);
    $$=ast;
  }
  | IDENT '(' ')'{
    auto ast = new UnaryExpAST();
    ast->catagory = 3;
    ast->ident = "@"+*unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')'{
    auto ast = new UnaryExpAST();
    ast->catagory = 4;
    ast->ident ="@"+ *unique_ptr<string>($1);
    ast->funcrparams = unique_ptr<FuncRParamsAST>((FuncRParamsAST*)$3);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$='+';
  }
  | '-' {
    $$='-';
  }
  | '!' {
    $$='!';
  }
  ;

MulExp
  : UnaryExp{
    auto ast=new MulExpAST();
    ast->unaryexp=unique_ptr<BaseAST>($1);
    ast->catagory=1;
    $$=ast;
  }
  |MulExp '*' UnaryExp{
    auto ast=new MulExpAST();
    ast->mulexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op='*';
    ast->unaryexp=unique_ptr<BaseAST>($3);
    $$=ast;
  }
  |MulExp '/' UnaryExp{
    auto ast=new MulExpAST();
    ast->mulexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op='/';
    ast->unaryexp=unique_ptr<BaseAST>($3);
    $$=ast;
  }
  |MulExp '%' UnaryExp{
    auto ast=new MulExpAST();
    ast->mulexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op='%';
    ast->unaryexp=unique_ptr<BaseAST>($3);
    $$=ast;
  }
  ;

AddExp
  : MulExp {
    auto ast=new AddExpAST();
    ast->mulexp=unique_ptr<BaseAST>($1);
    ast->catagory=1;
    $$=ast;
  }
  |AddExp '+' MulExp{
    auto ast=new AddExpAST();
    ast->mulexp=unique_ptr<BaseAST>($3);
    ast->catagory=2;
    ast->op='+';
    ast->addexp=unique_ptr<BaseAST>($1);
    $$=ast;
  }
  |AddExp '-' MulExp{
    auto ast=new AddExpAST();
    ast->mulexp=unique_ptr<BaseAST>($3);
    ast->catagory=2;
    ast->op='-';
    ast->addexp=unique_ptr<BaseAST>($1);
    $$=ast;
  };

RelExp
  :AddExp{
    auto ast=new RelExpAST();
    ast->addexp=unique_ptr<BaseAST>($1);
    ast->catagory=1;
    $$=ast;
  }
  | RelExp '<' AddExp{
    auto ast=new RelExpAST();
    ast->addexp=unique_ptr<BaseAST>($3);
    ast->relexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op="<";
    $$=ast;
  }
  | RelExp '>' AddExp{
    auto ast=new RelExpAST();
    ast->addexp=unique_ptr<BaseAST>($3);
    ast->relexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op=">";
    $$=ast;
    
  }
  | RelExp LE AddExp{
    auto ast=new RelExpAST();
    ast->addexp=unique_ptr<BaseAST>($3);
    ast->relexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op="<=";
    $$=ast;
    
  }
  | RelExp GE AddExp{
    auto ast=new RelExpAST();
    ast->addexp=unique_ptr<BaseAST>($3);
    ast->relexp=unique_ptr<BaseAST>($1);
    ast->catagory=2;
    ast->op=">=";
    $$=ast;
  }
  ;

EqExp
  :RelExp{
    auto ast=new EqExpAST();
    ast->relexp=unique_ptr<BaseAST>($1);
    ast->catagory=1;
    $$=ast;
  }
  |EqExp EQ RelExp{
    auto ast=new EqExpAST();
    ast->relexp=unique_ptr<BaseAST>($3);
    ast->eqexp=unique_ptr<BaseAST>($1);
    ast->op="==";
    ast->catagory=2;
    $$=ast;
  }
  |EqExp NE RelExp{
    auto ast=new EqExpAST();
    ast->relexp=unique_ptr<BaseAST>($3);
    ast->eqexp=unique_ptr<BaseAST>($1);
    ast->op="!=";
    ast->catagory=2;
    $$=ast;
  }
  ;

LAndExp
  :EqExp{
    auto ast=new LAndExpAST();
    ast->catagory=1;
    ast->eqexp=unique_ptr<BaseAST>($1);
    $$=ast;
  }
  |LAndExp AND EqExp{
    auto ast=new LAndExpAST();
    ast->catagory=2;
    ast->eqexp=unique_ptr<BaseAST>($3);
    ast->landexp=unique_ptr<BaseAST>($1);
    ast->op="&&";
    $$=ast;
  }
  ;

LOrExp
  :LAndExp{
    auto ast=new LOrExpAST();
    ast->catagory=1;
    ast->landexp=unique_ptr<BaseAST>($1);
    $$=ast;
  }
  |LOrExp OR LAndExp{
    auto ast=new LOrExpAST();
    ast->catagory=2;
    ast->landexp=unique_ptr<BaseAST>($3);
    ast->lorexp=unique_ptr<BaseAST>($1);
    ast->op="||";
    $$=ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}