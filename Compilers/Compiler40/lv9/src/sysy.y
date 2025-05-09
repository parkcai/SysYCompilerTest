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

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST* ast_val;
  std::list<std::unique_ptr<BaseAST> > * ast_list;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT UNARYOP MULOP ADDOP RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp BlockItem Decl ConstDecl ConstDef ConstInitVal ConstExp VarDecl VarDef InitVal FuncFParam FuncRParam GlobalDef LVal
%type <ast_list> BlockItems ConstDefList VarDefList FuncFParams GlobalDefs FuncRParams ConstInitValList InitValList ConstIdxList ExpList
%type <int_val> Number
// %type <str_val> LVal
%%

CompUnit :
  GlobalDefs {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->global_def_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($1);
    ast = move(comp_unit);
  }
  ;

GlobalDefs
  : GlobalDef {
    auto global_def_list = new std::list<std::unique_ptr<BaseAST> >();
    global_def_list->push_front(unique_ptr<BaseAST>($1));
    $$ = global_def_list;
  }
  | GlobalDef GlobalDefs {
    auto global_def_list = $2;
    global_def_list->push_front(unique_ptr<BaseAST>($1));
    $$ = global_def_list;
  }
  ;

GlobalDef
  : FuncDef {
    auto globaldef = new GlobalDefAST();
    globaldef->func_def = unique_ptr<BaseAST>($1);
    $$ = globaldef;
  }
  | ConstDecl {
    auto globaldef = new GlobalDefAST();
    globaldef->const_decl = unique_ptr<BaseAST>($1);
    $$ = globaldef;
  }
  | VarDecl {
    auto globaldef = new GlobalDefAST();
    globaldef->var_decl = unique_ptr<BaseAST>($1);
    $$ = globaldef;
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | FuncType IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->param_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($4);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam ',' FuncFParams {
    auto param_list = $3;
    param_list->push_front(unique_ptr<BaseAST>($1));
    $$ = param_list;
  }
  | FuncFParam {
    auto param_list = new std::list<unique_ptr<BaseAST> >();
    param_list->push_front(unique_ptr<BaseAST>($1));
    $$ = param_list;
  }
  ;

FuncFParam
  : FuncType IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | FuncType IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    auto sizeexp_list = new std::list<unique_ptr<BaseAST> >();
    ast->sizeexp_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >(sizeexp_list);
    $$ = ast;
  }
  | FuncType IDENT '[' ']' ConstIdxList {
    auto ast = new FuncFParamAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->sizeexp_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($5);
    $$ = ast;
  }
  ;
// ...


// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = "int";
    $$ = ast;
  }
  | VOID {
    auto ast = new FuncTypeAST();
    ast->type = "void";
    $$ = ast;
  }
  ;

Block
  : '{' BlockItems '}' {
    auto ast = new BlockAST();
    ast->block_item_list = std::unique_ptr<std::list<std::unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new BlockAST();
    $$ = ast;
  }
  ;

BlockItems
  : BlockItem BlockItems{
    auto block_item_list = $2;
    block_item_list->push_front(unique_ptr<BaseAST>($1));
    $$ = block_item_list;
  }
  | BlockItem {
    auto block_item_list = new std::list<unique_ptr<BaseAST> >();
    block_item_list->push_front(unique_ptr<BaseAST>($1));
    $$ = block_item_list;
  }

// Stmt
//   : RETURN Number ';' {
//     auto ast = new StmtAST();
//     ast->num = to_string($2);
//     $$ = ast;
//   }
//   ;
BlockItem : Stmt | Decl;
Decl : ConstDecl | VarDecl;
ConstDecl 
  : CONST FuncType ConstDefList ';' {
    std::cout << 11 << std::endl;
    auto ast = new ConstDeclAST();
    ast->btype = std::unique_ptr<BaseAST>($2);
    // ast->const_def_list = std::unique_ptr<BaseAST>($3);
    ast->const_def_list = std::unique_ptr<std::list<std::unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

// BType 
//   : INT {
//     std::cout << 12 << std::endl;
//     auto ast = new BTypeAST("int");
//     $$ = ast;
//   }
//   ;

ConstDefList
  : 
  ConstDef {
    std::cout << 9 << std::endl;
    auto const_def_list = new std::list<unique_ptr<BaseAST> >();
    const_def_list->push_front(unique_ptr<BaseAST>($1));
    std::cout << 99 << std::endl;
    $$ = const_def_list;
    // $$ = $1;
  }
  | ConstDefList ',' ConstDef {
    std::cout << 10 << std::endl;
    auto const_def_list = $1;
    const_def_list->push_back(unique_ptr<BaseAST>($3));
    $$ = const_def_list;
  }
  ;

ConstDef 
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ConstIdxList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->sizeexp_list = unique_ptr<list<unique_ptr<BaseAST> > >($2);
    ast->init = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstIdxList
  : '[' ConstExp ']' {
    auto const_idx_list = new std::list<unique_ptr<BaseAST> >();
    const_idx_list->push_front(unique_ptr<BaseAST>($2));
    $$ = const_idx_list;
  }
  | ConstIdxList '[' ConstExp ']' {
    auto const_idx_list = $1;
    const_idx_list->push_back(unique_ptr<BaseAST>($3));
    $$ = const_idx_list;
  }
  ;

ConstInitVal 
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    auto const_init_val_list = new std::list<unique_ptr<BaseAST> >();
    ast->const_init_val_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >(const_init_val_list);
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST();
    ast->const_init_val_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto const_init_val_list = new std::list<unique_ptr<BaseAST> >();
    const_init_val_list->push_front(unique_ptr<BaseAST>($1));
    $$ = const_init_val_list;
  }
  | ConstInitValList ',' ConstInitVal {
    auto const_init_val_list = $1;
    const_init_val_list->push_back(unique_ptr<BaseAST>($3));
    $$ = const_init_val_list;
  }
  ;

ConstExp
  : Exp {
    auto ast = $1;
    dynamic_cast<ExpAST*>(ast)->is_const = true;
    $$ = ast;
  }
  ;

VarDecl
  : FuncType VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->var_def_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($2);
    $$ = ast;
  };

VarDefList
  : VarDef {
    auto var_def_list = new std::list<unique_ptr<BaseAST> >();
    var_def_list->push_front(unique_ptr<BaseAST>($1));
    $$ = var_def_list;
  }
  | VarDefList ',' VarDef {
    auto var_def_list = $1;
    var_def_list->push_back(unique_ptr<BaseAST>($3));
    $$ = var_def_list;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ConstIdxList {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->sizeexp_list = unique_ptr<list<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  | IDENT ConstIdxList '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->sizeexp_list = unique_ptr<list<unique_ptr<BaseAST> > >($2);
    ast->init = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal 
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    auto init_val_list = new std::list<unique_ptr<BaseAST> >();
    ast->init_val_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >(init_val_list);
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST();
    ast->init_val_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

InitValList
  : InitVal {
    auto init_val_list = new std::list<unique_ptr<BaseAST> >();
    init_val_list->push_front(unique_ptr<BaseAST>($1));
    $$ = init_val_list;
  }
  | InitValList ',' InitVal {
    auto init_val_list = $1;
    init_val_list->push_back(unique_ptr<BaseAST>($3));
    $$ = init_val_list;
  }
  ;

ExpList
  : '[' Exp ']' {
    auto exp_list = new std::list<unique_ptr<BaseAST> >();
    exp_list->push_front(unique_ptr<BaseAST>($2));
    $$ = exp_list;
  }
  | ExpList '[' Exp ']' {
    auto exp_list = $1;
    exp_list->push_back(unique_ptr<BaseAST>($3));
    $$ = exp_list;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ExpList {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->exp_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;


Stmt
  : RETURN Exp ';' {
    std::cout << 8 << std::endl;
    auto ast = new StmtAST();
    ast->is_return = true;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->is_return = true;
    $$ = ast;
  }
  | LVal '=' Exp ';'{
    auto ast = new StmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    $$ = ast;
  }
  | IF Exp Stmt {
    auto ast = new BranchAST();
    ast->ifexp = unique_ptr<BaseAST>($2);
    ast->ifstmt = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IF Exp Stmt ELSE Stmt {
    auto ast = new BranchAST();
    ast->ifexp = unique_ptr<BaseAST>($2);
    ast->ifstmt = unique_ptr<BaseAST>($3);
    ast->elsestmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | WHILE Exp Stmt {
    auto ast = new LoopAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->stmt = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->is_break = true;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->is_continue = true;
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    std::cout << 7 << std::endl;
    $$ = $1;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

RelExp
  : RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

EqExp
  : EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LAndExp
  : LAndExp LANDOP EqExp {
    auto ast = new LAndExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->land_op = *unique_ptr<string>($2);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LOrExp LOROP LAndExp {
    auto ast = new LOrExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->lor_op = *unique_ptr<string>($2);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

MulExp
  : MulExp MULOP UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = *unique_ptr<string>($2);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

AddExp
  : AddExp ADDOP MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = *unique_ptr<string>($2);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    std::cout << 6 << std::endl;
    auto ast = new UnaryExpAST();
    std::cout << 666 << std::endl;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    std::cout << 66 << std::endl;
    $$ = ast;
    
  }
  ;

UnaryExp
  : UNARYOP UnaryExp {
    std::cout << 5 << std::endl;
    auto ast = new UnaryExpAST();
    ast->unary_op = * unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryExp
  : ADDOP UnaryExp {
    std::cout << 5 << std::endl;
    auto ast = new UnaryExpAST();
    ast->unary_op = * unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryExp
  : IDENT '(' ')'{
    auto func_call = new FuncCallAST();
    func_call->ident = *unique_ptr<string>($1);
    auto ast = new UnaryExpAST();
    ast->func_call = unique_ptr<BaseAST>(func_call);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')'{
    auto func_call = new FuncCallAST();
    func_call->ident = *unique_ptr<string>($1);
    func_call->param_list = unique_ptr<std::list<std::unique_ptr<BaseAST> > >($3);
    auto ast = new UnaryExpAST();
    ast->func_call = unique_ptr<BaseAST>(func_call);
    $$ = ast;
  }
  ;

FuncRParams
  : FuncRParam ',' FuncRParams {
    auto param_list = $3;
    param_list->push_front(unique_ptr<BaseAST>($1));
    $$ = param_list;
  }
  | FuncRParam {
    auto param_list = new std::list<unique_ptr<BaseAST> >();
    param_list->push_front(unique_ptr<BaseAST>($1));
    $$ = param_list;
  }
  ;

FuncRParam
  : Exp {
    $$ = $1;
  }
  ;

PrimaryExp
  : '(' Exp ')'{
    std::cout << 4 << std::endl;
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

PrimaryExp
  : LVal {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : Number {
    std::cout << 3 << std::endl;
    auto ast = new PrimaryExpAST();
    ast->val = to_string($1);
    $$ = ast;
  }
  ;

// UnaryOp
//   : '+' | '-' | '!' {
//     std::cout << 2 << std::endl;
//     $$ = $1;
//   }
//   ;

Number
  : INT_CONST {
    std::cout << 1 << std::endl;
    $$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
