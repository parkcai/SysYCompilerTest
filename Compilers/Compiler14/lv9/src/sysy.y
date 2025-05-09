%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>

#include "ast.hpp"

// 本文件作用是语法分析器,将token流生成为AST数据结构
// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

const bool debug = true;

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { std::unique_ptr<BaseAST> &ast }


%union {
  std::string *str_val;
  int int_val;
  char char_val;
  BaseAST *ast_val;
  vector<std::unique_ptr<BaseAST> > *vec_val;
}


%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token LAND LOR
%token <str_val> IDENT RelOp EqOp Type
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnitItem
%type <ast_val> FuncDef FuncFParam
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal 
%type <ast_val> Block BlockItem
%type <ast_val> Stmt Matched_Stmt Open_Stmt
%type <ast_val> Exp LVal PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <vec_val> CompUnitItemList ConstDefList ConstIndexList ConstArrayInitVal ConstInitValList VarDefList InitValList ArrayInitVal BlockItemList IndexList
%type <vec_val> FuncFParams FuncFParamList FuncRParams FuncRParamList
%type <int_val> Number
%type <char_val> UnaryOp AddOp MulOp

%%


CompUnit
  : CompUnitItemList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->comp_unit_item_list = unique_ptr<vector<unique_ptr<BaseAST> > >($1);
    ast = move(comp_unit);
  }
  ;

CompUnitItemList
  : CompUnitItem {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CompUnitItemList CompUnitItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

CompUnitItem
  : Decl {
    auto ast = new CompUnitItemAST();
    ast->type = 1;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitItemAST();
    ast->type = 2;
    ast->func_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->type = 1;
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->type = 2;
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST Type ConstDefList ';' {  
    auto ast = new ConstDeclAST();
    ast->Type = *unique_ptr<string>($2);
    ast->const_def_list = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT ConstIndexList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstIndexList
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | ConstIndexList '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->type = 1;
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ConstArrayInitVal {
    auto ast = new ConstInitValAST();
    ast->type = 2;
    ast->const_init_val_list = unique_ptr<vector<unique_ptr<BaseAST> > >($1);
    $$ = ast;
  }
  ;

ConstArrayInitVal
  : '{' '}' {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | '{' ConstInitValList '}' {
    $$ = $2;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDecl
  : Type VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->Type = *unique_ptr<string>($1);
    ast->var_def_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

VarDefList
  : VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT ConstIndexList{
    auto ast = new VarDefAST();
    ast->type = 1;
    ast->ident = *unique_ptr<string>($1);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  | IDENT ConstIndexList '=' InitVal {
    auto ast = new VarDefAST();
    ast->type = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
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
  | ArrayInitVal {
    auto ast = new InitValAST();
    ast->type = 2;
    ast->init_val_list = unique_ptr<vector<unique_ptr<BaseAST> > >($1);
    $$ = ast;
  }
  ;

ArrayInitVal
  : '{' '}' {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | '{' InitValList '}' {
    $$ = $2;
  }
  ;

InitValList
  : InitVal {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitValList ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

  



FuncDef
  : Type IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->Type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : {   
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | FuncFParamList {
    $$ = $1;
  }
  ;

FuncFParamList
  : FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncFParamList ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

FuncFParam
  : Type IDENT {
    auto ast = new FuncFParamAST();
    ast->type = 1;
    ast->Type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | Type IDENT '[' ']' ConstIndexList {
    auto ast = new FuncFParamAST();
    ast->type = 2;
    ast->Type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST> > >($5);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_item_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

BlockItemList
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | BlockItemList BlockItem {
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

Stmt
  : Matched_Stmt {
    auto ast = new StmtAST();
    ast->type = 1;
    ast->matched_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Open_Stmt {
    auto ast = new StmtAST();
    ast->type = 2;
    ast->open_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }

Matched_Stmt
  : LVal '=' Exp ';' {
    auto ast = new MatchedStmtAST();
    ast->type = 1;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ';' {
    auto ast = new MatchedStmtAST();
    ast->type = 2;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new MatchedStmtAST();
    ast->type = 3;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block {
    auto ast = new MatchedStmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    ast->type = 4;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new MatchedStmtAST();
    ast->type = 5;
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new MatchedStmtAST();
    ast->type = 6;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IF '(' Exp ')' Matched_Stmt ELSE Matched_Stmt {
    auto ast = new MatchedStmtAST();
    ast->type = 7;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->matched_stmt1 = unique_ptr<BaseAST>($5);
    ast->matched_stmt2 = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Matched_Stmt {
    auto ast = new MatchedStmtAST();
    ast->type = 8;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->matched_stmt1 = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new MatchedStmtAST(); 
    ast->type = 9;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new MatchedStmtAST(); 
    ast->type = 10;
    $$ = ast;
  }
  ;

Open_Stmt
  : IF '(' Exp ')' Stmt {
    auto ast = new OpenStmtAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' Matched_Stmt ELSE Open_Stmt {
    auto ast = new OpenStmtAST();
    ast->type = 2;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->matched_stmt = unique_ptr<BaseAST>($5);
    ast->open_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Open_Stmt {
    auto ast = new OpenStmtAST();
    ast->type = 3;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->open_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT IndexList{
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->index_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

IndexList
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | IndexList '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = 2;
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type = 3;
    ast->number = $1;
    $$ = ast;
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
    ast->type = 1;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 2;
    ast->unary_op = $1;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->type = 3;
    ast->ident = *unique_ptr<string>($1);
    ast->func_r_param_list = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

FuncRParams
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | FuncRParamList {
    $$ = $1;
  }
  ;

FuncRParamList
  : Exp {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncRParamList ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
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

  MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->type = 1;
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->type = 2;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->mul_op = $2;
    $$ = ast;
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

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->type = 1;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->type = 2;
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    ast->add_op = $2;
    $$ = ast;
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

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type = 1;
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpAST();
    ast->type = 2;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->rel_op = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->type = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExpAST();
    ast->type = 2;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    ast->eq_op = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->type = 1;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    ast->type = 2;
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
  
LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 1;
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 2;
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
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
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
