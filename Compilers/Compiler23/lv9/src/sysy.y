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

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }


%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  BaseExpAST *exp_ast_val;
  std::vector<std::unique_ptr<BaseAST> > *vec_ast_val;
  char char_val;
  std::vector<std::unique_ptr<BaseExpAST> > *vec_exp_ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnit FuncDef Type Block Stmt BlockItem CompUnitItem
%type <exp_ast_val> Exp PrimaryExp UnaryExp MulExp AddExp
%type <exp_ast_val> RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl ConstDef VarDecl VarDef
%type <exp_ast_val> LVal ConstExp ConstInitVal InitVal
%type <vec_ast_val> ConstDefs BlockItems VarDefs CompUnits FuncFParams
%type <int_val> Number
%type <char_val> UnaryOp MulOp AddOp
%type <ast_val> OpenStmt ClosedStmt FuncFParam
%type <vec_exp_ast_val> FuncRParams InitVals Index ExpIndex ConstInitVals
%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值

CompUnit
  :CompUnits {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->comp_units = unique_ptr<vector<unique_ptr<BaseAST> > >($1);
    ast = move(comp_unit);
  }
  ;


CompUnits
  :CompUnitItem {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  |CompUnits CompUnitItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

CompUnitItem
  :Decl {
    auto ast = new CompUnitItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  |FuncDef {
    auto ast = new CompUnitItemAST();
    ast->func_def = unique_ptr<BaseAST>($1);
    ast->case_=2;
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
  : Type IDENT '(' FuncFParams ')' Block{
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_params = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  :{
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  |FuncFParams ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  |FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

FuncFParam
  :Type IDENT {
    auto ast = new FuncFParamAST();
    ast->ident = *unique_ptr<string>($2);
    ast->case_=1;
    $$ = ast;
  }
  |Type IDENT '[' ']' Index{
    auto ast = new FuncFParamAST();
    ast->ident = *unique_ptr<string>($2);
    ast->index = unique_ptr<vector<unique_ptr<BaseExpAST> > >($5);
    ast->case_=2;
    $$ = ast;
  }
  ;

FuncRParams
  :{
    auto vec = new vector<unique_ptr<BaseExpAST> >();
    $$ = vec;
  }
  |Exp {
    auto vec = new vector<unique_ptr<BaseExpAST> >();
    vec->push_back(unique_ptr<BaseExpAST>($1));
    $$ = vec;
  }
  |FuncRParams ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseExpAST>($3));
    $$ = vec;
  }
  ;

// 同上, 不再解释
Type
  : INT {
    auto ast = new TypeAST();
    ast->type = "int";
    $$ = ast;
  }
  | VOID {
    auto ast = new TypeAST();
    ast->type = "void";
    $$ = ast;
  }
  ;

Block
  : '{' BlockItems '}' {
    auto ast = new BlockAST();
    ast->block_items = std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

BlockItems
  :{
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | BlockItems BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  :Decl {
    auto ast = new BlockItemAST();
    ast->decl_or_stmt = unique_ptr<BaseAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  |Stmt {
    auto ast = new BlockItemAST();
    ast->decl_or_stmt = unique_ptr<BaseAST>($1);
    ast->case_=2;
    $$ = ast;
  }

Stmt
  :ClosedStmt {
    auto ast = new StmtAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  |OpenStmt {
    auto ast = new StmtAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->case_=2;
    $$ = ast;
  }
  ;

OpenStmt
  :IF '(' Exp ')' Stmt {
    auto ast = new OpenStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->case_=1;
    $$ = ast;
  }
  |IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto ast = new OpenStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    ast->case_=2;
    $$ = ast;
  }
  |WHILE '(' Exp ')' OpenStmt {
    auto ast = new OpenStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->case_=3;
    $$ = ast;
  }
  ;

ClosedStmt
  :LVal '=' Exp ';' {
    auto ast = new ClosedStmtAST();
    ast->l_val = unique_ptr<BaseExpAST>($1);
    ast->exp = unique_ptr<BaseExpAST>($3);
    ast->case_=1;
    $$ = ast;
  }
  |RETURN Exp ';' {
    auto ast = new ClosedStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($2);
    ast->case_=2;
    $$ = ast;
  }
  |Block {
    auto ast = new ClosedStmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    ast->case_=3;
    $$ = ast;
  }
  |RETURN ';' {
    auto ast = new ClosedStmtAST();
    ast->case_=4;
    $$ = ast;
  }
  |Exp ';' {
    auto ast = new ClosedStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($1);
    ast->case_=5;
    $$ = ast;
  }
  |';'{
    auto ast = new ClosedStmtAST();
    ast->case_=6;
    $$ = ast;
  }
  |IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto ast = new ClosedStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    ast->case_=7;
    $$ = ast;
  }
  |WHILE '(' Exp ')' ClosedStmt {
    auto ast = new ClosedStmtAST();
    ast->exp = unique_ptr<BaseExpAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->case_=8;
    $$ = ast;
  }
  |BREAK ';' {
    auto ast = new ClosedStmtAST();
    ast->case_=9;
    $$ = ast;
  }
  |CONTINUE ';' {
    auto ast = new ClosedStmtAST();
    ast->case_=10;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->l_or_exp = unique_ptr<BaseExpAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  :'(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp_or_l_val = unique_ptr<BaseExpAST>($2);
    ast->case_=1;
    $$ = ast;
  }
  |LVal {
    auto ast = new PrimaryExpAST();
    ast->exp_or_l_val = unique_ptr<BaseExpAST>($1);
    ast->case_=2;
    $$ = ast;
  } 
  |Number {
    auto ast = new PrimaryExpAST();
    ast->number = $1;
    ast->case_=3;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp_or_unary_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->unary_op = $1;
    ast-> primary_exp_or_unary_exp = unique_ptr<BaseExpAST>($2);
    ast->case_=2;
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_r_params = unique_ptr<vector<unique_ptr<BaseExpAST> > >($3);
    ast->case_=3;
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

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseExpAST>($1);
    ast->mul_op = $2;
    ast->unary_exp = unique_ptr<BaseExpAST>($3);
    ast->case_=2;
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
    ast->mul_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseExpAST>($1);
    ast->add_op = $2;
    ast->mul_exp = unique_ptr<BaseExpAST>($3);
    ast->case_=2;
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
    ast->add_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseExpAST>($1);
    ast->rel_op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseExpAST>($3);
    ast->case_=2;
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseExpAST>($1);
    ast->eq_op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseExpAST>($3);
    ast->case_=2;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | LAndExp LANDOP EqExp {
    auto ast = new LAndExpAST();
    ast->l_and_exp = unique_ptr<BaseExpAST>($1);
    ast->eq_exp = unique_ptr<BaseExpAST>($3);
    ast->case_=2;
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->l_and_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | LOrExp LOROP LAndExp {
    auto ast = new LOrExpAST();
    ast->l_or_exp = unique_ptr<BaseExpAST>($1);
    ast->l_and_exp = unique_ptr<BaseExpAST>($3);
    ast->case_=2;
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl_or_var_decl = unique_ptr<BaseAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->const_decl_or_var_decl = unique_ptr<BaseAST>($1);
    ast->case_=2;
    $$ = ast;
  }
  ;

ConstDecl
  : CONST Type ConstDefs ';'{
    auto ast = new ConstDeclAST();
    ast->const_defs =  unique_ptr<vector<unique_ptr<BaseAST> > >($3);
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
  : IDENT Index '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->index = unique_ptr<vector<unique_ptr<BaseExpAST> > >($2);
    ast->const_init_val = unique_ptr<BaseExpAST>($4);
    $$ = ast;
  }
  ;

Index
  :{
    auto vec=new vector<unique_ptr<BaseExpAST> >();
    $$ = vec;
  }
  |Index '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseExpAST>($3));
    $$ = vec;
  }

ConstInitVal
  :ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  |'{' ConstInitVals '}' {
    auto ast = new ConstInitValAST();
    ast->const_init_vals = unique_ptr<vector<unique_ptr<BaseExpAST> > >($2);
    ast->case_=2;
    $$ = ast;
  }
  ;

ConstInitVals
  :{
    auto vec = new vector<unique_ptr<BaseExpAST> >();
    $$ = vec;
  }
  |ConstInitVal {
    auto vec = new vector<unique_ptr<BaseExpAST> >();
    vec->push_back(unique_ptr<BaseExpAST>($1));
    $$ = vec;
  }
  |ConstInitVals ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseExpAST>($3));
    $$ = vec;
  }
  ;

ConstExp
  :Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseExpAST>($1);
    $$ = ast;
  }
  ;

LVal
  :IDENT ExpIndex {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->exp_index = unique_ptr<vector<unique_ptr<BaseExpAST> > >($2);
    $$ = ast;
  }
  ;

ExpIndex
  :{
    auto vec=new vector<unique_ptr<BaseExpAST> >();
    $$ = vec;
  }
  |ExpIndex '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseExpAST>($3));
    $$ = vec;
  }
  ;

VarDecl
  :Type VarDefs ';'{
    auto ast = new VarDeclAST();
    ast->var_defs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

VarDefs
  :VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  |VarDefs ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  :IDENT Index{
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->index = unique_ptr<vector<unique_ptr<BaseExpAST> > >($2);
    ast->case_=1;
    $$ = ast;
  }
  |IDENT Index '=' InitVal{
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->index = unique_ptr<vector<unique_ptr<BaseExpAST> > >($2);
    ast->init_val = unique_ptr<BaseExpAST>($4);
    ast->case_=2;
    $$ = ast;
  }
  ;

InitVal
  :Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseExpAST>($1);
    ast->case_=1;
    $$ = ast;
  }
  |'{' InitVals'}' {
    auto ast = new InitValAST();
    ast->init_vals = unique_ptr<vector<unique_ptr<BaseExpAST> > >($2);
    ast->case_=2;
    $$ = ast;
  }
  ;

InitVals
  :{
    auto vec = new vector<unique_ptr<BaseExpAST> >();
    $$ = vec;
  }
  |InitVal {
    auto vec = new vector<unique_ptr<BaseExpAST> >();
    vec->push_back(unique_ptr<BaseExpAST>($1));
    $$ = vec;
  }
  |InitVals ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseExpAST>($3));
    $$ = vec;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
