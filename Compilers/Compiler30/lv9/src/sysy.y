%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.h"
#include "SymbolTable.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<BaseAST *> *vast_val;
}

// lexer 返回的所有 token 种类的声明 
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val 
%token INT VOID RETURN OR AND CONST IF ELSE WHILE CONTINUE BREAK
%token <str_val> IDENT EQOP RELOP UNARYOP MULOP
%token <int_val> INT_CONST

// 非终结符的类型定义 
%type <vast_val> CompUnitList BlockItemList ConstDefList VarDefList FuncFParamList FuncRParams 
%type <vast_val> ExpList ConstExpList ConstInitValList InitValList
%type <ast_val> FuncDef FuncFParam Block BlockItem Stmt Decl CompUnitProd
%type <ast_val> AssignStmt ExpStmt ReturnStmt MatchedStmt OpenStmt WhileStmt WhileJumpStmt
%type <ast_val> ConstDecl ConstDef ConstInitVal ConstExp
%type <ast_val> VarDecl VarDef InitVal
%type <ast_val> Exp AddExp MulExp UnaryExp PrimaryExp LVal
%type <ast_val> LOrExp LAndExp EqExp RelExp
%type <str_val> BType FuncType
%type <int_val> Number

%%

CompUnit 
  : CompUnitList {
    auto comp_unit = make_unique<CompUnitAST>();
    for(int i = 0; i < $1->size(); i++) {
      comp_unit->decl_func_list.push_back(unique_ptr<BaseAST>((*$1)[i]));
    }
    delete $1;
    ast = move(comp_unit);
  }
  ;

CompUnitList
  : CompUnitProd {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | CompUnitList CompUnitProd {
    auto vast = move($1);
    vast->push_back($2);
    $$ = move(vast);
  }
  ;

CompUnitProd
  : Decl {
    $$ = $1;
  }
  | FuncDef {
    $$ = $1;
  }
  ;

FuncDef 
  : BType IDENT '(' FuncFParamList ')' Block { 
    auto ast = new FuncDefAST();
    ast->func_type = *$1; 
    delete $1;
    ast->ident = *$2; 
    delete $2;
    for(int i = 0; i < $4->size(); i++) {
      ast->func_params.push_back(unique_ptr<BaseAST>((*$4)[i]));
    }
    delete $4;
    ast->block = unique_ptr<BaseAST>($6); 
    $$ = ast;
  } 
  ; 

FuncType 
  : INT {
    $$ = new string("int");
  } 
  | VOID {
    $$ = new string("void");
  }
  ;

FuncFParamList
  : /* empty */ {
    auto vast = new vector<BaseAST *>();
    $$ = vast;
  }
  | FuncFParam {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | FuncFParamList ',' FuncFParam {
    auto vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
  }
  ;

FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->type = *$1;
    delete $1;
    ast->ident = *$2;
    delete $2;
    $$ = ast;
  }
  | BType IDENT '[' ']' ExpList {
    auto ast = new FuncFParamAST();
    ast->type = "array";
    delete $1;
    ast->ident = *$2;
    delete $2;
    for(int i = 0; i < $5->size(); i++) {
      ast->const_exps.push_back(unique_ptr<BaseAST>((*$5)[i]));
    }
    delete $5;
    $$ = ast;
  }

Block 
  : '{' '}' {
    auto ast = new BlockAST();
    $$ = ast;
  }
  | '{' BlockItemList '}' {
    auto ast = new BlockAST();
    for(int i = 0; i < $2->size(); i++) {
      ast->block_items.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    $$ = ast;
  }
  ;

BlockItemList 
  : BlockItem {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | BlockItemList BlockItem {
    auto vast = move($1);
    vast->push_back($2);
    $$ = move(vast);
  }
  ;

BlockItem 
  : Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';' {
    auto ast = new ConstDeclAST();
    delete $2;
    for(int i = 0; i < $3->size(); i++) {
      ast->const_def.push_back(unique_ptr<BaseAST>((*$3)[i]));
    }
    delete $3;
    $$ = ast;
  }
  ;

ConstDefList 
  : ConstDef {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | ConstDefList ',' ConstDef {
    auto vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
  }
  ;

ConstDef
  : IDENT ExpList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *$1;
    delete $1;
    for(int i = 0; i < $2->size(); i++) {
      ast->const_exps.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST();
    for(int i = 0; i < $2->size(); i++) {
      ast->const_init_vals.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    $$ = ast;
  }
  ;

ConstInitValList
  : /* empty */ {
    auto vast = new vector<BaseAST *>();
    $$ = move(vast);
  }
  | ConstInitVal {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | ConstInitValList ',' ConstInitVal {
    auto vast = new vector<BaseAST *>();
    vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->type = Type::TYPE_VAR;
    delete $1;
    for(int i = 0; i < $2->size(); i++) {
      ast->var_def.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
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

VarDefList 
  : VarDef {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | VarDefList ',' VarDef {
    auto vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
  }
  ;

VarDef
  : IDENT ExpList {
    auto ast = new VarDefAST();
    ast->ident = *$1;
    delete $1;
    for(int i = 0; i < $2->size(); i++) {
      ast->const_exps.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    $$ = ast;
  }
  | IDENT ExpList '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *$1;
    delete $1;
    for(int i = 0; i < $2->size(); i++) {
      ast->const_exps.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST();
    for(int i = 0; i < $2->size(); i++) {
      ast->init_vals.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    $$ = ast;
  }
  ;

InitValList
  : /* empty */ {
    auto vast = new vector<BaseAST *>();
    $$ = move(vast);
  }
  | InitVal {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | InitValList ',' InitVal {
    auto vast = new vector<BaseAST *>();
    vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
  }
  ;

Stmt
  : MatchedStmt {
    auto ast = new StmtAST();
    ast->matched_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | OpenStmt {
    auto ast = new StmtAST();
    ast->open_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

MatchedStmt 
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new MatchedStmtAST();
    ast->cond = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | AssignStmt {
    auto ast = new MatchedStmtAST();
    ast->assign_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ExpStmt {
    auto ast = new MatchedStmtAST();
    ast->exp_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block {
    auto ast = new MatchedStmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ReturnStmt {
    auto ast = new MatchedStmtAST();
    ast->return_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WhileStmt {
    auto ast = new MatchedStmtAST();
    ast->while_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WhileJumpStmt {
    auto ast = new MatchedStmtAST();
    ast->while_jump_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ; 

OpenStmt 
  : IF '(' Exp ')' Stmt {
    auto ast = new OpenStmtAST();
    ast->cond = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new OpenStmtAST();
    ast->cond = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

AssignStmt
  : LVal '=' Exp ';' {
    auto ast = new AssignStmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ExpStmt
  : Exp ';' {
    auto ast = new ExpStmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new ExpStmtAST();
    $$ = ast;
  }
  ;

ReturnStmt
  : RETURN Exp ';' {
    auto ast = new ReturnStmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new ReturnStmtAST();
    $$ = ast;
  }
  ; 

WhileStmt
  : WHILE '(' Exp ')' Stmt {
    auto ast = new WhileStmtAST();
    ast->cond = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }

WhileJumpStmt
  : BREAK ';' {
    auto ast = new WhileJumpStmtAST();
    ast->type = "break";
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new WhileJumpStmtAST();
    ast->type = "continue";
    $$ = ast;
  }

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
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = *$2;
    delete $2;
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *$2;
    delete $2;
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp UNARYOP MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = *$2;
    delete $2;
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MULOP UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = *$2;
    delete $2;
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UNARYOP UnaryExp {
    auto ast = new UnaryExpAST();
    ast->unary_op = *$1;
    delete $1;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *$1;
    delete $1;
    for(int i = 0; i < $3->size(); i++) {
      ast->func_params.push_back(unique_ptr<BaseAST>((*$3)[i]));
    }
    delete $3;
    $$ = ast;
  }
  ;

FuncRParams
  : /* */ {
    auto vast = new vector<BaseAST *>();
    $$ = move(vast);
  }
  | Exp {
    auto vast = new vector<BaseAST *>();
    vast->push_back($1);
    $$ = move(vast);
  }
  | FuncRParams ',' Exp {
    auto vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = $1;
    $$ = ast;
  }
  ;

LVal
  : IDENT ExpList {
    auto ast = new LValAST();
    ast->ident = *$1;
    delete $1;
    for(int i = 0; i < $2->size(); i++) {
      ast->exps.push_back(unique_ptr<BaseAST>((*$2)[i]));
    }
    delete $2;
    $$ = ast;
  }
  ;

ExpList
  : /* */ {
    auto vast = new vector<BaseAST *>();
    $$ = move(vast);
  }
  | '[' Exp ']' {
    auto vast = new vector<BaseAST *>();
    vast->push_back($2);
    $$ = move(vast);
  }
  | ExpList '[' Exp ']' {
    auto vast = move($1);
    vast->push_back($3);
    $$ = move(vast);
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
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}