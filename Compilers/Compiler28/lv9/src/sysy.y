%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

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
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>> *ast_vec;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt Exp LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp PrimaryExp Number
%type <ast_val> BlockItem Decl ConstDecl VarDecl ConstDef VarDef ConstInitVal InitVal ConstExp LVal BType
%type <ast_val> If FuncFParam Def
%type <ast_vec> ConstDefArray BlockItemArray DefArray VarDefArray FuncFParams FuncRParams Array InitValArray
%type <str_val> UnaryOp

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值


CompUnit
  : DefArray {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->def_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($1);
    ast = move(comp_unit);
  }
  ;

DefArray
  : Def {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto def = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(def));
    $$ = vec;
  }
  | DefArray Def {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($1);
    auto def = std::unique_ptr<BaseAST>($2);
    vec->push_back(std::move(def));
    $$ = vec;
  }
  ;

Def
  : FuncDef {
    auto ast = new DefAST();
    ast->func_def = unique_ptr<BaseAST>($1);
    ast->def_type = *new string("FUNC");
    $$ = ast;
  }
  | Decl {
    auto ast = new DefAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->def_type = *new string("DECL");
    $$ = ast;
  }

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
  : BType IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto func_fparam = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(func_fparam));
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($1);
    auto func_fparam = std::unique_ptr<BaseAST>($3);
    vec->push_back(std::move(func_fparam));
    $$ = vec;
  }
  | {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }

FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->array = nullptr;
    ast->is_array = false;
    $$ = ast;
  }
  | BType IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->array = nullptr;
    ast->is_array = true;
    $$ = ast;
  }
  | BType IDENT '[' ']' Array {
    auto ast = new FuncFParamAST();
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($5);
    ast->is_array = true;
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemArray '}' {
    auto ast = new BlockAST();
    ast->block_item_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new BlockAST();
    ast->block_item_array = nullptr;
    $$ = ast;
  }
  ;

BlockItemArray
  : BlockItem {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto block_item = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(block_item));
    $$ = vec;
  }
  | BlockItemArray BlockItem {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($1);
    auto block_item = std::unique_ptr<BaseAST>($2);
    vec->push_back(std::move(block_item));
    $$ = vec;
  }
  ;

BlockItem
  : Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->block_item_type = *new string("STMT");
    $$ = ast;
  }
  | Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->block_item_type = *new string("DECL");
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    ast->decl_type = *new string("CONST");
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    ast->decl_type = *new string("VAR");
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefArray ';' {
    auto ast = new VarDeclAST();
    ast->type = std::unique_ptr<BaseAST>($1);
    ast->var_def_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefArray ';' {
    auto ast = new ConstDeclAST();
    ast->type = std::unique_ptr<BaseAST>($2);
    ast->const_def_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }

BType
  : INT {
    auto ast = new BTypeAST();
    ast->type = *new string("INT");
    $$ = ast;
  }
  | VOID {
    auto ast = new BTypeAST();
    ast->type = *new string("VOID");
    $$ = ast;
  }

ConstDefArray
  : ConstDef {
    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto const_def = std::unique_ptr<BaseAST>($1);
    vec->insert(vec->begin(), std::move(const_def));
    $$ = vec;
  }
  | ConstDef ',' ConstDefArray {
    auto vec =(std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto const_def = std::unique_ptr<BaseAST>($1);
    vec->insert(vec->begin(), std::move(const_def));
    $$ = vec;
  }
  ;

VarDefArray
  : VarDef {
    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto var_def = std::unique_ptr<BaseAST>($1);
    vec->insert(vec->begin(), std::move(var_def));
    $$ = vec;
  }
  | VarDef ',' VarDefArray {
    auto vec =(std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto var_def = std::unique_ptr<BaseAST>($1);
    vec->insert(vec->begin(), std::move(var_def));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array = nullptr;
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT Array '=' InitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

Array
  : '[' Exp ']' {
    auto *vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto exp = unique_ptr<BaseAST>($2);
    vec->push_back(std::move(exp));
    $$ = vec;
  }
  | Array '[' Exp ']' {
    auto vec = (std::vector<std::unique_ptr<BaseAST>> *)($1);
    auto exp = unique_ptr<BaseAST>($3);
    vec->push_back(std::move(exp));
    $$ = vec;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->var_init_val = nullptr;
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->var_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT Array '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    ast->var_init_val = std::unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | IDENT Array {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    ast->var_init_val = nullptr;
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }

InitValArray
  : InitVal {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto init_val = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(init_val));
    $$ = vec;
  }
  | InitValArray ',' InitVal {
    auto vec = (std::vector<std::unique_ptr<BaseAST>> *)($1);
    auto init_val = std::unique_ptr<BaseAST>($3);
    vec->push_back(std::move(init_val));
    $$ = vec;
  }
  ;

InitVal
  : '{' InitValArray '}' {
    auto ast = new InitValAST();
    ast->init_val_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    ast->type = InitValAST::ARRAY;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->type = InitValAST::ZERO;
    $$ = ast;
  }
  |  Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->type = InitValAST::EXP;
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = *new string("RETURN");
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->stmt_type = *new string("RETURN");
    ast->exp = nullptr;
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt_type = *new string("ASSIGN");
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->stmt_type = *new string("EXP");
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->stmt_type = *new string("EMPTY");
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    ast->stmt_type = *new string("BLOCK");
    $$ = ast;
  }
  | If {
    auto ast = new StmtAST();
    ast->if_stmt = unique_ptr<BaseAST>($1);
    ast->else_stmt = nullptr;
    ast->stmt_type = *new string("BRANCH");
    $$ = ast;
  }
  | If ELSE Stmt {
    auto ast = new StmtAST();
    ast->if_stmt = unique_ptr<BaseAST>($1);
    ast->else_stmt = unique_ptr<BaseAST>($3);
    ast->stmt_type = *new string("BRANCH");
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->while_stmt = unique_ptr<BaseAST>($5);
    ast->stmt_type = *new string("LOOP");
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->stmt_type = *new string("BREAK");
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->stmt_type = *new string("CONTINUE");
    $$ = ast;
  }
  ;

If
  : IF '(' Exp ')' Stmt {
    auto ast = new IfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
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

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->op = *new string(" ");
    $$ = ast;
  }
  | LOrExp '|''|' LAndExp {
    auto ast = new LOrExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($4);
    ast->op = *new string("||");
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = *new string(" ");
    $$ = ast;
  }
  | LAndExp '&''&' EqExp {
    auto ast = new LAndExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($4);
    ast->op = *new string("&&");
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *new string(" ");
    $$ = ast;
  }
  | EqExp '=''=' RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->rel_exp = unique_ptr<BaseAST>($4);
    ast->op = *new string("==");
    $$ = ast;
  }
  | EqExp '!''=' RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->rel_exp = unique_ptr<BaseAST>($4);
    ast->op = *new string("!=");
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->op = *new string(" ");
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string("<");
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string(">");
    $$ = ast;
  }
  | RelExp '<''=' AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($4);
    ast->op = *new string("<=");
    $$ = ast;
  }
  | RelExp '>''=' AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($4);
    ast->op = *new string(">=");
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = *new string(" ");
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string("+");
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string("-");
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    ast->op = *new string(" ");
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string("*");
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string("/");
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->op = *new string("%");
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->tag = "PEXP";
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->tag = "OPEXP";
    ast->unary_op = *($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->tag = "FCALL";
    ast->ident = *unique_ptr<string>($1);
    ast->func_rparams = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto exp = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(exp));
    $$ = vec;
  }
  | FuncRParams ',' Exp {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($1);
    auto exp = std::unique_ptr<BaseAST>($3);
    vec->push_back(std::move(exp));
    $$ = vec;
  }
  | {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  ;

UnaryOp
  : '+' {
    $$ = new std::string("+");
  }
  | '-' {
    $$ = new std::string("-");
  }
  | '!' {
    $$ = new std::string("!");
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->tag = "EXP";
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->tag = "NUM";
    ast->number = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->tag = "LVAL";
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->int_const = $1;
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array = nullptr;
    $$ = ast;
  }
  | IDENT Array {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
