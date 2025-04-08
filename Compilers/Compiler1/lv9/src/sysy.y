%code requires {
  #include <memory>
  #include <string>
  #include "abstract_syntax_tree.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "abstract_syntax_tree.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::shared_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}



// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::shared_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 shared_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  vector<shared_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN OR AND EQ NOTEQ NOGREATER NOLESSER CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnit FuncDef Block Stmt Exp UnaryExp PrimaryExp UnaryOp LOrExp LAndExp EqExp RelExp AddExp MulExp 
%type <ast_val> BlockItem Decl ConstDecl ConstDef ConstInitVal ConstExp LVal VarDecl VarDef InitVal StmtElse StmtNotElse FuncFParam
%type <vec_val> BlockItemList  FollowingConstDefList FollowingVarDefList CompUnitList FollowingFuncFParamList FollowingExpList
%type <vec_val> FollowingConstIndexList FollowingIndexList FollowingConstInitValList FollowingInitValList
/* %type <str_val> Number */
%type <int_val> Number

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值

SysYRoot
  : CompUnitList {
    auto sysy_root = make_unique<SysYRootAST>();
    sysy_root->comp_unit_list = std::move(*$1);
    delete $1;
    ast = move(sysy_root);
  }
  ;

CompUnitList
  : CompUnit CompUnitList {
    auto current_compunit = shared_ptr<BaseAST>($1);
    auto& former_list = *$2;
    former_list.insert(former_list.begin(), current_compunit);
    $$ = $2;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

CompUnit
  : Decl {
    auto ast = new CompUnitAST();
    ast->decl = shared_ptr<BaseAST>($1);
    ast->is_func_def_flag = 0;
    ast->is_decl_flag = 1;
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitAST();
    ast->func_def = shared_ptr<BaseAST>($1);
    ast->is_func_def_flag = 1;
    ast->is_decl_flag = 0;
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 shared_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 shared_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 shared_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->has_params = 0;
    ast->func_type = "int";
    ast->ident = *shared_ptr<string>($2);
    ast->block = shared_ptr<BaseAST>($5);
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->has_params = 0;
    ast->func_type = "void";
    ast->ident = *shared_ptr<string>($2);
    ast->block = shared_ptr<BaseAST>($5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParam FollowingFuncFParamList ')' Block {
    auto ast = new FuncDefAST();
    ast->has_params = 1;
    ast->func_type = "int";
    ast->ident = *shared_ptr<string>($2);
    ast->block = shared_ptr<BaseAST>($7);
    auto current_funcfparam = shared_ptr<BaseAST>($4);
    ast->funcfparams = move(*$5);
    delete $5;
    ast->funcfparams.insert(ast->funcfparams.begin(), current_funcfparam);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParam FollowingFuncFParamList ')' Block {
    auto ast = new FuncDefAST();
    ast->has_params = 1;
    ast->func_type = "void";
    ast->ident = *shared_ptr<string>($2);
    ast->block = shared_ptr<BaseAST>($7);
    auto current_funcfparam = shared_ptr<BaseAST>($4);
    ast->funcfparams = move(*$5);
    delete $5;
    ast->funcfparams.insert(ast->funcfparams.begin(), current_funcfparam);
    $$ = ast;
  }
  ;

FollowingFuncFParamList
  : ',' FuncFParam FollowingFuncFParamList{
    auto current_funcfparam = shared_ptr<BaseAST>($2);
    auto& former_list = *$3;
    former_list.insert(former_list.begin(), current_funcfparam);
    $$ = $3;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

FollowingConstIndexList
  : '[' ConstExp ']' FollowingConstIndexList{
    auto current_constexp = shared_ptr<BaseAST>($2);
    auto& former_list = *$4;
    former_list.insert(former_list.begin(), current_constexp);
    $$ = $4;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

FollowingIndexList
  : '[' Exp ']' FollowingIndexList{
    auto current_exp = shared_ptr<BaseAST>($2);
    auto& former_list = *$4;
    former_list.insert(former_list.begin(), current_exp);
    $$ = $4;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

FollowingConstInitValList
  : ',' ConstInitVal FollowingConstInitValList{
    auto current_constinitval = shared_ptr<BaseAST>($2);
    auto& former_list = *$3;
    former_list.insert(former_list.begin(), current_constinitval);
    $$ = $3;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

FollowingInitValList
  : ',' InitVal FollowingInitValList{
    auto current_initval = shared_ptr<BaseAST>($2);
    auto& former_list = *$3;
    former_list.insert(former_list.begin(), current_initval);
    $$ = $3;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = "int";
    ast->ident = *shared_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']'{
    auto ast = new FuncFParamAST();
    ast->btype = "int array";
    ast->ident = *shared_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']' '[' ConstExp ']' FollowingConstIndexList {
    auto ast = new FuncFParamAST();
    ast->btype = "int array";
    ast->ident = *shared_ptr<string>($2);
    auto current_constexp = shared_ptr<BaseAST>($6);
    auto& former_list = *$8;
    former_list.insert(former_list.begin(), current_constexp);
    ast->indexes = move(*$8);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->blockitems = std::move(*$2); 
    delete $2;
    $$ = ast;
  }
  ;

BlockItemList
  : BlockItem BlockItemList {
    auto current_blockitem = shared_ptr<BaseAST>($1);
    auto& former_list = *$2;  
    former_list.insert(former_list.begin(), current_blockitem); 
    $$ = $2; 
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;


BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->is_stmt_flag = 0;
    ast->decl = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->is_stmt_flag = 1;
    ast->stmt = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->is_constdecl_flag = 1;
    ast->constdecl = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->is_constdecl_flag = 0;
    ast->vardecl = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST INT ConstDef FollowingConstDefList ';' {
    auto ast = new ConstDeclAST();
    auto current_constdef = shared_ptr<BaseAST>($3); 
    ast->btype = "int"; 
    ast->constdefs = std::move(*$4); 
    delete $4;
    ast->constdefs.insert(ast->constdefs.begin(), current_constdef);
    $$ = ast; 
  }
  ;

FollowingConstDefList
  : ',' ConstDef FollowingConstDefList {
    auto current_constdef = shared_ptr<BaseAST>($2); 
    auto& former_list = *$3; 
    former_list.insert(former_list.begin(), current_constdef); 
    $$ = $3; 
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>(); 
  }
  ;

VarDecl
  : INT VarDef FollowingVarDefList ';' {
    auto ast = new VarDeclAST();
    auto current_vardef = shared_ptr<BaseAST>($2); 
    ast->btype = "int"; 
    ast->vardefs = move(*$3); 
    delete $3;
    ast->vardefs.insert(ast->vardefs.begin(), current_vardef);
    $$ = ast; 
  }
  ;

FollowingVarDefList
  : ',' VarDef FollowingVarDefList {
    auto current_vardef = shared_ptr<BaseAST>($2); 
    auto& former_list = *$3; 
    former_list.insert(former_list.begin(), current_vardef); 
    $$ = $3; 
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>(); 
  }
  ;


ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *shared_ptr<string>($1);
    ast->constinitval = shared_ptr<BaseAST>($3);
    ast->has_index_flag = 0;
    $$ = ast;
  }
  | IDENT '[' ConstExp ']' FollowingConstIndexList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *shared_ptr<string>($1);
    ast->constinitval = shared_ptr<BaseAST>($7);
    auto& former_list = *$5;
    former_list.insert(former_list.begin(), shared_ptr<BaseAST>($3));
    ast->indexes = move(*$5);
    ast->has_index_flag = 1;
    $$ = ast;
  }
  ;

VarDef
  : IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *shared_ptr<string>($1);
    ast->initval = shared_ptr<BaseAST>($3);
    ast->has_index_flag = 0;
    ast->initialized_flag = 1;
    $$ = ast;
  }
  | IDENT {
    auto ast = new VarDefAST();
    ast->ident = *shared_ptr<string>($1);
    ast->has_index_flag = 0;
    ast->initialized_flag = 0;
    $$ = ast;
  }
  | IDENT '[' ConstExp ']' FollowingConstIndexList '=' InitVal {
    auto ast = new VarDefAST();
    ast->has_index_flag = 1;
    ast->ident = *shared_ptr<string>($1);
    ast->initval = shared_ptr<BaseAST>($7);
    ast->initialized_flag = 1;
    auto& former_list = *$5;
    former_list.insert(former_list.begin(), shared_ptr<BaseAST>($3));
    ast->indexes = move(*$5);
    $$ = ast;
  }
  | IDENT '[' ConstExp ']' FollowingConstIndexList {
    auto ast = new VarDefAST();
    ast->ident = *shared_ptr<string>($1);
    ast->has_index_flag = 1;
    ast->initialized_flag = 0;
    auto& former_list = *$5;
    former_list.insert(former_list.begin(), shared_ptr<BaseAST>($3));
    ast->indexes = move(*$5);
    $$ = ast;
  }
  | IDENT '[' ConstExp ']' FollowingConstIndexList '=' '{' '}' {
    auto ast = new VarDefAST();
    ast->has_index_flag = 1;
    ast->ident = *shared_ptr<string>($1);
    auto initval = new InitValAST();
    initval->is_exp_flag = 0;
    vector<shared_ptr<BaseAST>> initvals;
    initval->initvals = initvals;
    ast->initval = shared_ptr<BaseAST>(initval);
    ast->initialized_flag = 1;
    auto& former_list = *$5;
    former_list.insert(former_list.begin(), shared_ptr<BaseAST>($3));
    ast->indexes = move(*$5);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->is_constexp_flag = 1;
    ast->constexp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitVal FollowingConstInitValList '}' {
    auto ast = new ConstInitValAST();
    ast->is_constexp_flag = 0;
    auto current_constinitval = shared_ptr<BaseAST>($2);
    auto& former_list = *$3;
    former_list.insert(former_list.begin(), current_constinitval);
    ast->constinitvals = move(*$3);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast->is_constexp_flag = 0;
    vector<shared_ptr<BaseAST>> constinitvals;
    ast->constinitvals = constinitvals;
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->is_exp_flag = 1;
    ast->exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' InitVal FollowingInitValList '}' {
    auto ast = new InitValAST();
    ast->is_exp_flag = 0;
    auto current_initval = shared_ptr<BaseAST>($2);
    auto& former_list = *$3;
    former_list.insert(former_list.begin(), current_initval);
    ast->initvals = move(*$3);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->is_exp_flag = 0;
    vector<shared_ptr<BaseAST>> initvals;
    ast->initvals = initvals;
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = shared_ptr<BaseAST>($2);
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 0;
    ast->is_block_flag = 0;
    ast->is_return_flag = 1;
    ast->return_empty_flag = 0;
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->lval = shared_ptr<BaseAST>($1);
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 0;
    ast->is_block_flag = 0;
    ast->is_return_flag = 0;
    ast->is_lval_flag = 1;
    ast->exp = shared_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 0;
    ast->is_block_flag = 1;
    ast->block = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 0;
    ast->is_block_flag = 0;
    ast->is_return_flag = 0;
    ast->is_lval_flag = 0;
    ast->is_empty_flag = 1;
    $$ = ast;

  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 0;
    ast->is_block_flag = 0;
    ast->is_return_flag = 1;
    ast->return_empty_flag = 1;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 0;
    ast->is_block_flag = 0;
    ast->is_return_flag = 0;
    ast->is_lval_flag = 0;
    ast->is_empty_flag = 0;
    ast->exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IF '(' Exp ')' StmtElse {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 1;
    ast->has_else_branch = 1;
    ast->if_condition = shared_ptr<BaseAST>($3);
    ast->stmtelse = shared_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' StmtNotElse {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 0;
    ast->is_if_flag = 1;
    ast->has_else_branch = 0;
    ast->if_condition = shared_ptr<BaseAST>($3);
    ast->stmtnotelse = shared_ptr<BaseAST>($5);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 0;
    ast->is_while_flag = 1;
    ast->while_condition = shared_ptr<BaseAST>($3);
    ast->while_stmt = shared_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->is_continue_flag = 0;
    ast->is_break_flag = 1;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->is_continue_flag = 1;
    $$ = ast;
  }
  ;

StmtElse
  : Stmt ELSE Stmt {
    auto ast = new StmtElseAST();
    ast->then_stmt = shared_ptr<BaseAST>($1);
    ast->else_stmt = shared_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

StmtNotElse
  : Stmt{
    auto ast = new StmtNotElseAST();
    ast->then_stmt = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = shared_ptr<BaseAST>($1);
    $$ = ast; 
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->is_single_land_flag = 1;
    ast->land_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->is_single_land_flag = 0;
    ast->lor_exp = shared_ptr<BaseAST>($1);
    ast->land_exp = shared_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->is_single_eq_flag = 1;
    ast->eq_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->is_single_eq_flag = 0;
    ast->land_exp = shared_ptr<BaseAST>($1);
    ast->eq_exp = shared_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->is_single_rel_flag = 1;
    ast->rel_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpAST();
    ast->is_single_rel_flag = 0;
    ast->eq_exp = shared_ptr<BaseAST>($1);
    ast->rel_exp = shared_ptr<BaseAST>($3);
    ast->eq_operator = "==";
    $$ = ast;
  }
  | EqExp NOTEQ RelExp {
    auto ast = new EqExpAST();
    ast->is_single_rel_flag = 0;
    ast->eq_exp = shared_ptr<BaseAST>($1);
    ast->rel_exp = shared_ptr<BaseAST>($3);
    ast->eq_operator = "!=";
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->is_single_add_flag = 1;
    ast->add_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->is_single_add_flag = 0;
    ast->rel_exp = shared_ptr<BaseAST>($1);
    ast->add_exp = shared_ptr<BaseAST>($3);
    ast->rel_operator = "<";
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->is_single_add_flag = 0;
    ast->rel_exp = shared_ptr<BaseAST>($1);
    ast->add_exp = shared_ptr<BaseAST>($3);
    ast->rel_operator = ">";
    $$ = ast;
  }
  | RelExp NOGREATER AddExp {
    auto ast = new RelExpAST();
    ast->is_single_add_flag = 0;
    ast->rel_exp = shared_ptr<BaseAST>($1);
    ast->add_exp = shared_ptr<BaseAST>($3);
    ast->rel_operator = "<=";
    $$ = ast;
  }
  | RelExp NOLESSER AddExp {
    auto ast = new RelExpAST();
    ast->is_single_add_flag = 0;
    ast->rel_exp = shared_ptr<BaseAST>($1);
    ast->add_exp = shared_ptr<BaseAST>($3);
    ast->rel_operator = ">=";
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->is_single_mul_flag = 1;
    ast->mul_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->is_single_mul_flag = 0;
    ast->add_exp = shared_ptr<BaseAST>($1);
    ast->mul_exp = shared_ptr<BaseAST>($3);
    ast->add_operator = "+";
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->is_single_mul_flag = 0;
    ast->add_exp = shared_ptr<BaseAST>($1);
    ast->mul_exp = shared_ptr<BaseAST>($3);
    ast->add_operator = "-";
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->is_single_unary_flag = 1;
    ast->unary_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->is_single_unary_flag = 0;
    ast->mul_exp = shared_ptr<BaseAST>($1);
    ast->unary_exp = shared_ptr<BaseAST>($3);
    ast->mul_operator = "*";
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->is_single_unary_flag = 0;
    ast->mul_exp = shared_ptr<BaseAST>($1);
    ast->unary_exp = shared_ptr<BaseAST>($3);
    ast->mul_operator = "/";
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->is_single_unary_flag = 0;
    ast->mul_exp = shared_ptr<BaseAST>($1);
    ast->unary_exp = shared_ptr<BaseAST>($3);
    ast->mul_operator = "%";
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->is_func_res_flag = 0;
    ast->is_primary_flag = 1;
    ast->primary_exp = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp{
    auto ast = new UnaryExpAST();
    ast->is_func_res_flag = 0;
    ast->is_primary_flag = 0;
    ast->unary_op = shared_ptr<BaseAST>($1);
    ast->unary_exp = shared_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '+' UnaryExp{
    auto ast = new UnaryExpAST();
    ast->is_func_res_flag = 0;
    ast->is_primary_flag = 1;
    ast->primary_exp = shared_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->is_func_res_flag = 1;
    ast->has_funcrparams = 0;
    ast->func_name = *shared_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '(' Exp FollowingExpList ')' {
    auto ast = new UnaryExpAST();
    ast->is_func_res_flag = 1;
    ast->has_funcrparams = 1;
    ast->func_name = *shared_ptr<string>($1);
    auto current_exp = shared_ptr<BaseAST>($3);
    auto& former_list = *$4;
    former_list.insert(former_list.begin(), current_exp);
    ast->funcrparams = move(*$4);
    $$ = ast;
  }
  ;

FollowingExpList
  : ',' Exp FollowingExpList{
    auto current_exp = shared_ptr<BaseAST>($2);
    auto& former_list = *$3;
    former_list.insert(former_list.begin(), current_exp);
    $$ = $3;
  }
  | /* empty */ {
    $$ = new vector<shared_ptr<BaseAST>>();
  }
  ;

PrimaryExp
  : Number {
    auto ast = new PrimaryExpAST();
    ast->is_single_number_flag = 1;
    ast->is_lval = 0;
    ast->number = $1;
    $$ = ast;
  }
  | '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->is_single_number_flag = 0;
    ast->is_lval = 0;
    ast->exp = shared_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->is_lval = 1;
    ast->lval = shared_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->has_index_flag = 0;
    ast->ident = *shared_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '[' Exp ']' FollowingIndexList {
    auto ast = new LValAST();
    ast->has_index_flag = 1;
    ast->ident = *shared_ptr<string>($1);
    auto& former_list = *$5;
    former_list.insert(former_list.begin(), shared_ptr<BaseAST>($3));
    ast->indexes = move(*$5);
    $$ = ast;
  }
  ;


UnaryOp
  : '-' {
    auto ast = new UnaryOpAST();
    ast->unary_op = '-';
    $$ = ast;
  }
  | '!' {
    auto ast = new UnaryOpAST();
    ast->unary_op = '!';
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(shared_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
