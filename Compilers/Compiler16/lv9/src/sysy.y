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

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<BaseAST*> *ast_vec;
}

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token PLUS MINUS NOT MUL DIV MOD
%token LT GT LE GE EQ NEQ
%token AND OR
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> CompUnit FuncDef Type FuncFParam CompUnitItem
%type <ast_val> Block BlockItem Stmt MatchedStmt OpenStmt OtherStmt
%type <ast_val> Exp AddExp MulExp UnaryExp PrimaryExp EqExp LAndExp RelExp LOrExp ConstExp
%type <ast_val> Decl ConstDecl VarDecl ConstDef VarDef ConstInitVal InitVal LVal Number
%type <ast_vec> BlockItemList ConstDefList VarDefList FuncRParams FuncFParams CompUnitItemList ConstInitValList InitValList ArrayDims ArrayIndices FuncArrayDims
%type <str_val> UnaryOp AddOp MulOp RelOp EqOp

%%

CompUnit:
    CompUnitItemList {
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->comp_unit_items = move(*$1);
        ast = move(comp_unit);
    }
    ;

CompUnitItemList:
    CompUnitItem {
        $$ = new std::vector<BaseAST*>();
        $$->push_back($1);
    }
    | CompUnitItemList CompUnitItem {
        $$ = $1;
        $$->push_back($2);
    }
    ;

CompUnitItem
  : FuncDef {
      $$ = $1;
    }
  | Decl {
      $$ = $1;
    }
  ;

FuncDef
  : Type IDENT '(' ')' Block {
      auto func_def = new FuncDefAST();
      func_def->func_type = std::unique_ptr<BaseAST>($1);
      func_def->ident = *($2);
      func_def->block = std::unique_ptr<BaseAST>($5);
      $$ = func_def;
    }
  | Type IDENT '(' FuncFParams ')' Block {
      auto func_def = new FuncDefAST();
      func_def->func_type = std::unique_ptr<BaseAST>($1);
      func_def->ident = *($2);
      func_def->params = std::move(*$4);
      func_def->block = std::unique_ptr<BaseAST>($6);
      delete $4;
      $$ = func_def;
    }
  ;

Type
  : INT {
      $$ = new TypeAST("int");
    }
  | VOID {
      $$ = new TypeAST("void");
    }
  ;

FuncFParams
  : FuncFParam {
      $$ = new std::vector<BaseAST*>();
      $$->push_back($1);
    }
  | FuncFParams ',' FuncFParam {
      $$ = $1;
      $$->push_back($3);
    }
  ;

FuncArrayDims:
    FuncArrayDims '[' ConstExp ']' {
          $$ = $1;
          $$->push_back($3);
      }
    | {
          $$ = new std::vector<BaseAST*>();
      }
    ;


FuncFParam
  : Type IDENT '[' ']' FuncArrayDims {
      auto param = new FuncFParamAST();
      param->btype = dynamic_cast<TypeAST*>($1)->type;
      param->ident = *($2);
      param->is_array = true;
      param->array_dims.push_back(new NumberAST(-1));
      for(auto dim_ast : *($5)){
        param->array_dims.push_back(dim_ast);
      }
      $$ = param;
    }
  | Type IDENT {
      auto param = new FuncFParamAST();
      param->btype = dynamic_cast<TypeAST*>($1)->type;
      param->ident = *($2);
      $$ = param;
  }
  ;

Block
  : '{' '}' {
      auto block = new BlockAST();
      $$ = block;
    }
  | '{' BlockItemList '}' {
      auto block = new BlockAST();
      block->block_items = std::move(*$2);
      delete $2;
      $$ = block;
    }
  ;

BlockItemList
  : BlockItem {
      $$ = new std::vector<BaseAST*>();
      $$->push_back($1);
    }
  | BlockItemList BlockItem {
      $$ = $1;
      $$->push_back($2);
    }
  ;

BlockItem
  : Decl {
      $$ = $1;
    }
  | Stmt {
      $$ = $1;
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
  : CONST Type ConstDefList ';' {
      auto const_decl = new ConstDeclAST();
      const_decl->btype = dynamic_cast<TypeAST*>($2) -> type;
      // 将 BaseAST* 转换为 ConstDefAST*
      for(auto def : *$3) {
          ConstDefAST* const_def = dynamic_cast<ConstDefAST*>(def);
          if (const_def) {
              const_decl->const_defs.push_back(const_def);
          }
          else {
              // 处理类型错误
              cerr << "Error: ConstDefList contains non-ConstDefAST elements.\n";
              exit(1);
          }
      }
      delete $3;
      $$ = const_decl;
    }
  ;

VarDecl
  : Type VarDefList ';' {
      auto var_decl = new VarDeclAST();
      var_decl->btype = dynamic_cast<TypeAST*>($1) -> type;
      for(auto def : *$2) {
          VarDefAST* var_def = dynamic_cast<VarDefAST*>(def);
          if (var_def) {
              var_decl->var_defs.push_back(var_def);
          }
          else {
              // 处理类型错误
              cerr << "Error: VarDefList contains non-VarDefAST elements.\n";
              exit(1);
          }
      }
      delete $2;
      $$ = var_decl;
    }
  ;

ConstDefList
  : ConstDef {
      $$ = new std::vector<BaseAST*>();
      $$->push_back($1);
    }
  | ConstDefList ',' ConstDef {
      $$ = $1;
      $$->push_back($3);
    }
  ;

VarDefList
  : VarDef {
      $$ = new std::vector<BaseAST*>();
      $$->push_back($1);
    }
  | VarDefList ',' VarDef {
      $$ = $1;
      $$->push_back($3);
    }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
      auto const_def = new ConstDefAST();
      const_def->ident = *($1);
      const_def->const_init_val = unique_ptr<BaseAST>($3);
      $$ = const_def;
    }
  | IDENT ArrayDims '=' ConstInitVal {
        auto const_def = new ConstDefAST();
        const_def->ident = *($1);
        const_def->is_array = true;
        // 将各个维度的 ConstExp 转换为 AST
        for(auto dim : *$2){
            const_def->array_dimensions.emplace_back(std::unique_ptr<BaseAST>(dim));
        }
        const_def->const_init_val = std::unique_ptr<BaseAST>($4);
        $$ = const_def;
    }
  ;

VarDef
  : IDENT {
      auto var_def = new VarDefAST();
      var_def->ident = *($1);
      $$ = var_def;
    }
  | IDENT '=' InitVal {
      auto var_def = new VarDefAST();
      var_def->ident = *($1);
      var_def->init_val = unique_ptr<BaseAST>($3);
      $$ = var_def;
    }
  | IDENT ArrayDims {
        auto var_def = new VarDefAST();
        var_def->ident = *($1);
        var_def->is_array = true;
        // 将各个维度的 ConstExp 转换为 AST
        for(auto dim : *$2){
            var_def->array_dimensions.emplace_back(std::unique_ptr<BaseAST>(dim));
        }
        $$ = var_def;
    }
  | IDENT ArrayDims '=' InitVal {
        auto var_def = new VarDefAST();
        var_def->ident = *($1);
        var_def->is_array = true;
        // 将各个维度的 ConstExp 转换为 AST
        for(auto dim : *$2){
            var_def->array_dimensions.emplace_back(std::unique_ptr<BaseAST>(dim));
        }
        var_def->init_val = std::unique_ptr<BaseAST>($4);
        $$ = var_def;
    }
  ;

InitVal
  : Exp {
      $$ = $1;
    }
  | '{' InitValList '}' {
      auto aggregate = new AggregateAST();
      aggregate->elements = std::move(*$2);
      delete $2;
      $$ = aggregate;
    }
  | '{' '}' {
      auto aggregate = new AggregateAST();
      aggregate->elements = *(new std::vector<BaseAST*>());
      $$ = aggregate;
    }
  ;

ConstInitVal
  : ConstExp {
      $$ = $1;
    }
  | '{' ConstInitValList '}' {
      auto aggregate = new AggregateAST();
      aggregate->elements = std::move(*$2);
      delete $2;
      $$ = aggregate;
    }
  | '{' '}' {
      auto aggregate = new AggregateAST();
      aggregate->elements = *(new std::vector<BaseAST*>());
      $$ = aggregate;
    }
  ;

InitValList:
      InitVal {
          $$ = new std::vector<BaseAST*>();
          $$->push_back($1);
      }
    | InitValList ',' InitVal {
          $$ = $1;
          $$->push_back($3);
      }
    ;

ConstInitValList:
      ConstInitVal {
          $$ = new std::vector<BaseAST*>();
          $$->push_back($1);
      }
    | ConstInitValList ',' ConstInitVal {
          $$ = $1;
          $$->push_back($3);
      }
    ;

ArrayDims:
    ArrayDims '[' ConstExp ']' {
          $$ = $1;
          $$->push_back($3);
      }
    | '[' ConstExp ']' {
          $$ = new std::vector<BaseAST*>();
          $$->push_back($2);
      }
    ;

ArrayIndices:
      ArrayIndices '[' Exp ']' {
          $$ = $1;
          $$->push_back($3);
      }
    | '[' Exp ']' {
          $$ = new std::vector<BaseAST*>();
          $$->push_back($2);
      }
    ;
  
ConstExp
  : Exp {
      $$ = $1;
    }
  ;

Stmt
  : MatchedStmt
  | OpenStmt
  ;

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt
    {
      auto if_stmt = new IfStmtAST();
      if_stmt->cond = std::unique_ptr<BaseAST>($3);
      if_stmt->then_stmt = std::unique_ptr<BaseAST>($5);
      if_stmt->else_stmt = std::unique_ptr<BaseAST>($7);
      $$ = if_stmt;
    }
  | OtherStmt
    {
      $$ = $1;
    }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt
    {
      auto if_stmt = new IfStmtAST();
      if_stmt->cond = std::unique_ptr<BaseAST>($3);
      if_stmt->then_stmt = std::unique_ptr<BaseAST>($5);
      if_stmt->else_stmt = nullptr;
      $$ = if_stmt;
    }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt
    {
      auto if_stmt = new IfStmtAST();
      if_stmt->cond = std::unique_ptr<BaseAST>($3);
      if_stmt->then_stmt = std::unique_ptr<BaseAST>($5);
      if_stmt->else_stmt = std::unique_ptr<BaseAST>($7);
      $$ = if_stmt;
    }
  ;

OtherStmt
  : LVal '=' Exp ';'
    {
      auto assign_stmt = new AssignStmtAST();
      assign_stmt->lval = unique_ptr<BaseAST>($1);
      assign_stmt->exp = unique_ptr<BaseAST>($3);
      $$ = assign_stmt;
    }
  | RETURN Exp ';'
    {
      auto stmt = new ReturnStmtAST();
      stmt->exp = unique_ptr<BaseAST>($2);
      $$ = stmt;
    }
  | RETURN ';'
    {
      auto stmt = new ReturnStmtAST();
      stmt->exp = nullptr;
      $$ = stmt;
    }
  | Exp ';'
    {
      auto exp_stmt = new ExpStmtAST(unique_ptr<BaseAST>($1));
      $$ = exp_stmt;
    }
  | ';'
    {
      auto empty_stmt = new EmptyStmtAST();
      $$ = empty_stmt;
    }
  | Block
    {
      $$ = $1;
    }
  | WHILE '(' Exp ')' Stmt
  {
      auto while_stmt = new WhileStmtAST();
      while_stmt->cond = std::unique_ptr<BaseAST>($3);
      while_stmt->body = std::unique_ptr<BaseAST>($5);
      $$ = while_stmt;
  }
  | BREAK ';'
  {
      auto break_stmt = new BreakStmtAST();
      $$ = break_stmt;
  }
  | CONTINUE ';'
  {
      auto continue_stmt = new ContinueStmtAST();
      $$ = continue_stmt;
  }

Exp
  : LOrExp {
      $$ = $1;
    }
  ;

LOrExp
  : LAndExp {
      $$ = $1;
    }
  | LOrExp OR LAndExp {
      auto lor_exp = new LOrExpAST();
      lor_exp->lhs = std::unique_ptr<BaseAST>($1);
      lor_exp->rhs = std::unique_ptr<BaseAST>($3);
      $$ = lor_exp;

    }
  ;

LAndExp
  : EqExp {
      $$ = $1;
    }
  | LAndExp AND EqExp {
      auto land_exp = new LAndExpAST();
      land_exp->lhs = std::unique_ptr<BaseAST>($1);
      land_exp->rhs = std::unique_ptr<BaseAST>($3);
      $$ = land_exp;
    }
  ;

EqExp
  : RelExp {
      $$ = $1;
    }
  | EqExp EqOp RelExp {
      auto eq_exp = new BinaryExpAST(*($2), unique_ptr<BaseAST>($1), unique_ptr<BaseAST>($3));
      $$ = eq_exp;
    }
  ;

EqOp
  : EQ {
      $$ = new std::string("==");
    }
  | NEQ {
      $$ = new std::string("!=");
    }
  ;

RelExp
  : AddExp {
      $$ = $1;
    }
  | RelExp RelOp AddExp {
      auto rel_exp = new BinaryExpAST(*($2), unique_ptr<BaseAST>($1), unique_ptr<BaseAST>($3));
      $$ = rel_exp;
    }
  ;

RelOp
  : LT {
      $$ = new std::string("<");
    }
  | GT {
      $$ = new std::string(">");
    }
  | LE {
      $$ = new std::string("<=");
    }
  | GE {
      $$ = new std::string(">=");
    }
  ;

AddExp
  : MulExp {
      $$ = $1;
    }
  | AddExp AddOp MulExp {
      auto add_exp = new BinaryExpAST(*($2), unique_ptr<BaseAST>($1), unique_ptr<BaseAST>($3));
      $$ = add_exp;
    }
  ;

AddOp
  : PLUS {
      $$ = new std::string("+");
    }
  | MINUS {
      $$ = new std::string("-");
    }
  ;

MulExp
  : UnaryExp {
      $$ = $1;
    }
  | MulExp MulOp UnaryExp {
      auto mul_exp = new BinaryExpAST(*($2), unique_ptr<BaseAST>($1), unique_ptr<BaseAST>($3));
      $$ = mul_exp;
    }
  ;

MulOp
  : MUL {
      $$ = new std::string("*");
    }
  | DIV {
      $$ = new std::string("/");
    }
  | MOD {
      $$ = new std::string("%");
    }
  ;

UnaryExp
  : PrimaryExp {
      $$ = $1;
    }
  | UnaryOp UnaryExp {
      auto unary_exp = new UnaryExpAST(*($1), std::unique_ptr<BaseAST>($2));
      $$ = unary_exp;
    }
  | IDENT '(' ')' {
      auto func_call = new FuncCallAST(*($1));
      $$ = func_call;
    }
  | IDENT '(' FuncRParams ')' {
      auto func_call = new FuncCallAST(*($1));
      func_call->params = std::move(*$3);
      delete $3;
      $$ = func_call;
    }
  ;

FuncRParams
  : Exp {
      $$ = new std::vector<BaseAST*>();
      $$->push_back($1);
    }
  | FuncRParams ',' Exp {
      $$ = $1;
      $$->push_back($3);
    }
  ;


UnaryOp
  : PLUS {
      $$ = new std::string("+");
    }
  | MINUS {
      $$ = new std::string("-");
    }
  | NOT {
      $$ = new std::string("!");
    }
  ;

PrimaryExp
  : '(' Exp ')' {
      $$ = $2;
    }
  | LVal {
      $$ = $1;
    }
  | Number {
      $$ = $1;
    }
  ;

LVal
  : IDENT {
      auto lval = new LValAST();
      lval->ident = *($1);
      lval->is_array = false;
      $$ = lval;
    }
  | IDENT ArrayIndices {
        auto lval = new LValAST();
        lval->ident = *($1);
        lval->is_array = true;
        for(auto idx : *$2){
            lval->index_exps.emplace_back(std::unique_ptr<BaseAST>(idx));
        }
        $$ = lval;
    }
  ;

Number
  : INT_CONST {
      $$ = new NumberAST($1);
    }
  ;

%%

// 定义错误处理函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  std::cerr << "error: " << s << std::endl;
}

