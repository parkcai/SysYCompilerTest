%code requires {
  #include <memory>
  #include <string>
  #include <ast.hpp>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <ast.hpp>

// 声明 lexer 函数和错误处理函数
int yydebug = 1;
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
  BaseAST *ast_val;
  vector<unique_ptr<BaseAST> >  *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> Def FuncDef FuncFParam Type Block BlockItem OpenStmt ClosedStmt NonIfStmt 
%type <ast_val> Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal LVal ConstExp VarDecl VarDef InitVal
%type <vec_val> ConstMultiDef BlockMultiItem VarMultiDef CompUnits FuncFParams FuncRParams ConstInitValList InitValList ArrayIndexes
%type <int_val> Number
%type <str_val> UnaryOp MulOp AddOp RelOp EqOp

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : CompUnits {
    auto comp_unit = make_unique<CompUnitAST>($1);
    ast = move(comp_unit);
  }
  ;

CompUnits
  : CompUnits Def {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | Def {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

Def
  : FuncDef {
    $$ = $1;
  }
  | Decl {
    $$ = $1;
  }

FuncDef
  : Type IDENT '(' ')' Block {
    auto ast = new FuncDefAST($1, $2->c_str(), nullptr, $5);
    $$ = ast;
  }
  | Type IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST($1, $2->c_str(), $4, $6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParams ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

FuncFParam
  : Type IDENT {
    auto ast = new FuncFParamAST1($1, $2->c_str());
    $$ = ast;
  }
  | Type IDENT '[' ']' {
    auto ast = new FuncFParamAST2($1, $2->c_str(), nullptr);
    $$ = ast;
  }
  | Type IDENT '[' ']' ArrayIndexes {
    auto ast = new FuncFParamAST2($1, $2->c_str(), $5);
    $$ = ast;
  }
  ;

Type
  : INT {
    auto ast = new TypeAST("int");
    $$ = ast;
  }
  | VOID {
    auto ast = new TypeAST("void");
    $$ = ast;
  }
  ;

Block
  : '{' BlockMultiItem '}' {
    auto ast = new BlockAST($2);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new BlockAST(nullptr);
    $$ = ast;
  }
  ;

BlockMultiItem
  : BlockMultiItem BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | BlockItem {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

BlockItem
  : OpenStmt {
    $$ = $1;
  }
  | ClosedStmt {
    $$ = $1;
  }
  ;

OpenStmt
  : IF '(' Exp ')' BlockItem {
    auto ast = new IfAST1($3, $5);
    $$ = ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto ast = new IfAST2($3, $5, $7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto ast = new WhileAST($3, $5);
    $$ = ast;
  }
  ;

ClosedStmt
  : IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto ast = new IfAST2($3, $5, $7);
    $$ = ast;
  }
  | NonIfStmt {
    $$ = $1;
  }
  | Decl {
    $$ = $1;
  }
  | WHILE '(' Exp ')' ClosedStmt {
    auto ast = new WhileAST($3, $5);
    $$ = ast;
  }
  ;

NonIfStmt
  : RETURN Exp ';' {
    auto ast = new StmtAST1($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST1(nullptr);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST2($1, $3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST3($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST3(nullptr);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST4($1);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST5();
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST6();
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    $$ = $1;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    $$ = $2;
  }
  | Number {
    auto ast = new PrimaryExpAST($1);
    $$ = ast;
  }
  | LVal {
    $$ = $1;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
    $$ = $1;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST1($1->c_str(), $2);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST2($1->c_str(), nullptr);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST2($1->c_str(), $3);
    $$ = ast;
  }
  ;

FuncRParams
  : FuncRParams ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | Exp {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

UnaryOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  | '!' {
    $$ = new string("!");
  }
  ;

MulExp
  : UnaryExp {
    $$ = $1;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST($1, $2->c_str(), $3);
    $$ = ast;
  }
  ;

MulOp
  : '*' {
    $$ = new string("*");
  }
  | '/' {
    $$ = new string("/");
  }
  | '%' {
    $$ = new string("%");
  }
  ;

AddExp 
  : MulExp {
    $$ = $1;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST($1, $2->c_str(), $3);
    $$ = ast;
  }
  ;

AddOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  ;

RelExp
  : AddExp {
    $$ = $1;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpAST($1, $2->c_str(), $3);
    $$ = ast;
  }
  ;

RelOp
  : '<' {
    $$ = new string("<");
  }
  | '>' {
    $$ = new string(">");
  }
  | '<' '=' {
    $$ = new string("<=");
  }
  | '>' '=' {
    $$ = new string(">=");
  }
  ;

EqExp
  : RelExp {
    $$ = $1;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExpAST($1, $2->c_str(), $3);
    $$ = ast;
  }
  ;

EqOp
  : '=' '=' {
    $$ = new string("==");
  }
  | '!' '=' {
    $$ = new string("!=");
  }
  ;

LAndExp 
  : EqExp {
    $$ = $1;
  }
  | LAndExp '&' '&' EqExp {
    auto ast = new LAndExpAST($1, $4);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    $$ = $1;
  }
  | LOrExp '|' '|' LAndExp {
    auto ast = new LOrExpAST($1, $4);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    $$ = $1;
  }
  | VarDecl {
    $$ = $1;
  }
  ;

ConstDecl
  : CONST Type ConstMultiDef ';' {
    auto ast = new ConstDeclAST($2, $3);
    $$ = ast;
  }
  ;

ConstMultiDef
  : ConstMultiDef ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | ConstDef {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST($1->c_str(), $3);
    $$ = ast;
  }
  | IDENT ArrayIndexes '=' ConstInitVal {
    auto ast = new ConstArrayDefAST($1->c_str(), $2, $4);
    $$ = ast;
  }
  ;

ConstInitVal
  : '{' '}' {
    auto ast = new ConstInitValAST(nullptr);
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST($2);
    $$ = ast;
  }
  | ConstExp {
    $$ = $1;
  }
  ;

ConstInitValList
  : ConstInitValList ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | ConstInitVal {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

VarDecl
  : Type VarMultiDef ';' {
    auto ast = new VarDeclAST($1, $2);
    $$ = ast;
  }
  ;

VarMultiDef
  : VarMultiDef ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

VarDef 
  : IDENT {
    auto ast = new VarDefAST1($1->c_str());
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST2($1->c_str(), $3);
    $$ = ast;
  }
  | IDENT ArrayIndexes {
    auto ast = new VarArrayDefAST1($1->c_str(), $2);
    $$ = ast;
  }
  | IDENT ArrayIndexes '=' InitVal {
    auto ast = new VarArrayDefAST2($1->c_str(), $2, $4);
    $$ = ast;
  }
  ;

InitVal
  : '{' '}' {
    auto ast = new InitValAST(nullptr);
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST($2);
    $$ = ast;
  }
  | Exp {
    $$ = $1;
  }
  ;

InitValList
  : InitValList ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | InitVal {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST1($1->c_str());
    $$ = ast;
  }
  | IDENT ArrayIndexes {
    auto ast = new LValAST2($1->c_str(), $2);
    $$ = ast;
  }
  ;

ConstExp 
  : Exp {
    $$ = $1;
  }
  ;

ArrayIndexes
  : ArrayIndexes '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  } 
  | '[' Exp ']' {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
