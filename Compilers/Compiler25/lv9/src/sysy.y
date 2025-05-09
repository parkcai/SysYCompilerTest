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
#include "my_var.h"

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
  BaseExprAST *expr_ast_val;
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
  std::vector<std::unique_ptr<BaseExprAST>> *expr_vec_val;
}

%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT 
%token <int_val> INT_CONST

%type <ast_val> FuncDef Block Stmt BlockItem
%type <expr_ast_val> Expr UnaryExpr PrimaryExpr AddExpr MulExpr LOrExpr LAndExpr EqExpr RelExpr ConstExpr ConstInitVal LVal LeftLVal 
%type <ast_val> Decl ConstDecl ConstDef CallParam
%type <ast_val> VarDecl VarDef FuncParam 
%type <expr_ast_val> VarInitVal 
%type <str_val> BType
%type <int_val> Number
%type <vec_val> BlockItemList ConstDefList VarDefList UnitList FuncParamList CallParamList 
%type <expr_vec_val> VarInitValList IndexList MoreArraySize
/* %type <expr_vec_val> ConstInitValList */

%%

CompUnit
  : UnitList {
    auto comp_unit = make_unique<CompUnitAST>();
    vector<unique_ptr<BaseAST>> *vec = ($1);
    for (auto it = vec->begin(); it!= vec->end(); it++)
      comp_unit->unit_list.push_back(move(*it));
    ast = move(comp_unit);
  }
  ;

UnitList
  : Decl{
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncDef{
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | UnitList Decl{
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | UnitList FuncDef{
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;
  
FuncDef
  : INT IDENT '(' ')' Block{
    auto func_def = new FuncDefAST();
    auto func_type = new FuncTypeAST();
    func_type->type = string("int");
    func_def->func_type = unique_ptr<BaseAST>(func_type);
    func_def->ident = *unique_ptr<string>($2);
    func_def->block = unique_ptr<BaseAST>($5);
    $$ = func_def;
  }
  | INT IDENT '(' FuncParamList ')' Block{
    auto func_def = new FuncDefAST();
    auto func_type = new FuncTypeAST();
    func_type->type = string("int");
    func_def->func_type = unique_ptr<BaseAST>(func_type);
    func_def->ident = *unique_ptr<string>($2);
    func_def->block = unique_ptr<BaseAST>($6);
    vector<unique_ptr<BaseAST>> *vec = ($4);
    for(auto it = vec->begin(); it!= vec->end(); it++)
      func_def->func_param_list.push_back(move(*it));
    $$ = func_def;
  }
  | VOID IDENT '(' ')' Block{
    auto func_def = new FuncDefAST();
    auto func_type = new FuncTypeAST();
    func_type->type = string("void");
    func_def->func_type = unique_ptr<BaseAST>(func_type);
    func_def->ident = *unique_ptr<string>($2);
    func_def->block = unique_ptr<BaseAST>($5);
    $$ = func_def;
  }
  | VOID IDENT '(' FuncParamList ')' Block{
    auto func_def = new FuncDefAST();
    auto func_type = new FuncTypeAST();
    func_type->type = string("void");
    func_def->func_type = unique_ptr<BaseAST>(func_type);
    func_def->ident = *unique_ptr<string>($2);
    func_def->block = unique_ptr<BaseAST>($6);
    vector<unique_ptr<BaseAST>> *vec = ($4);
    for(auto it = vec->begin(); it!= vec->end(); it++)
      func_def->func_param_list.push_back(move(*it));
    $$ = func_def;
  }
  ;

FuncParamList
  : FuncParam{
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncParamList ',' FuncParam{
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }

FuncParam
  : BType IDENT{
    auto func_param = new FuncParamAST();
    func_param->btype = "int";
    func_param->ident = *unique_ptr<string>($2);
    func_param->flag = FuncParamAST::INT;
    $$ = func_param;
  }
  | BType IDENT '[' ']' MoreArraySize{
    auto func_param = new FuncParamAST();
    func_param->btype = "int";
    func_param->ident = *unique_ptr<string>($2);
    func_param->flag = FuncParamAST::ARRAY;
    vector<unique_ptr<BaseExprAST>> *vec = ($5);
    for(auto it = vec->begin(); it != vec->end(); it++)
      func_param->more_array_size.push_back(move(*it));
    $$ = func_param;
  }
  ;

MoreArraySize
  : {
    vector<unique_ptr<BaseExprAST>> *vec = new vector<unique_ptr<BaseExprAST>>;
    $$ = vec;
  }
  | ConstExpr {
    vector<unique_ptr<BaseExprAST>> *vec = new vector<unique_ptr<BaseExprAST>>;
    vec->push_back(unique_ptr<BaseExprAST>($1));
    $$ = vec;
  }
  | MoreArraySize '[' ConstExpr ']' {
    vector<unique_ptr<BaseExprAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseExprAST>($3));
    $$ = vec;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    vector<unique_ptr<BaseAST> > *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->block_item_list.push_back(move(*it));
    $$ = ast;
  }
  ;

BlockItemList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | BlockItemList BlockItem {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    auto block_item = new BlockItemAST();
    block_item->flag = BlockItemAST::DECL;
    block_item->decl = unique_ptr<BaseAST>($1);
    $$ = block_item;
  }
  | Stmt {
    auto block_item = new BlockItemAST();
    block_item->flag = BlockItemAST::STMT;
    block_item->stmt = unique_ptr<BaseAST>($1);
    $$ = block_item;
  }
  ;
  
Stmt
  : RETURN Expr ';' {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::RETURN;
    stmt->expr = unique_ptr<BaseExprAST>($2);
    $$ = stmt;
  }
  | LeftLVal '=' Expr ';' {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::ASSIGN;
    stmt->lval = unique_ptr<BaseExprAST>($1);
    stmt->expr = unique_ptr<BaseExprAST>($3);
    $$ = stmt;
  }
  | Block {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::BLOCK;
    stmt->block = unique_ptr<BaseAST>($1);
    $$ = stmt;
  }
  | ';'{
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::EMPTY;
    $$ = stmt;
  }
  | Expr ';' {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::EXPR;
    stmt->expr = unique_ptr<BaseExprAST>($1);
    $$ = stmt;
  }
  | RETURN ';'{
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::RETURN_VOID;
    $$ = stmt;
  }
  | IF '(' Expr ')' Stmt {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::IF;
    stmt->cond = unique_ptr<BaseExprAST>($3);
    stmt->if_stmt = unique_ptr<BaseAST>($5);
  
    $$ = stmt;
  }
  | IF '(' Expr ')' Stmt ELSE Stmt {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::IF_ELSE;
    stmt->cond = unique_ptr<BaseExprAST>($3);
    stmt->if_stmt = unique_ptr<BaseAST>($5);
    stmt->else_stmt = unique_ptr<BaseAST>($7);
    $$ = stmt;
  }
  | WHILE '(' Expr ')' Stmt {
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::WHILE;
    stmt->cond = unique_ptr<BaseExprAST>($3);
    stmt->while_stmt = unique_ptr<BaseAST>($5);
    $$ = stmt;
  }
  | BREAK ';'{
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::BREAK;
    $$ = stmt;
  }
  | CONTINUE ';'{
    auto stmt = new StmtAST();
    stmt->flag = StmtAST::CONTINUE;
    $$ = stmt;
  }
  ;

LeftLVal
  : IDENT {
    auto lval = new LeftLValAST();
    lval->ident = *unique_ptr<string>($1);
    lval->flag = LeftLValAST::IDENT;
    $$ = lval;
  }
  | IDENT IndexList {
    auto lval = new LeftLValAST();
    lval->ident = *unique_ptr<string>($1);
    lval->flag = LeftLValAST::LVAL_IDX;
    vector<unique_ptr<BaseExprAST>> *vec = ($2);
    for(auto it = vec->begin(); it!= vec->end(); it++)
      lval->idx_lst.push_back(move(*it));
    $$ = lval;
  }
  ;


Expr 
  : LOrExpr {
    auto expr = new ExprAST();
    expr->lor_expr = unique_ptr<BaseExprAST>($1);
    expr->val = $1->val;
    $$ = expr;
  }
  ;

UnaryExpr
  : PrimaryExpr {
    auto unary_expr = new UnaryExprAST();
    unary_expr->flag=UnaryExprAST::PRIMARY_EXPR;
    unary_expr->primary_expr = unique_ptr<BaseExprAST>($1);
    unary_expr->val = $1->val;
    $$ = unary_expr;
  }
  | '+' UnaryExpr {
    auto unary_expr = new UnaryExprAST();
    unary_expr->flag=UnaryExprAST::OP_UNARY;
    unary_expr->op = "+";
    unary_expr->unary_expr = unique_ptr<BaseExprAST>($2);
    unary_expr->val = $2->val;  
    $$ = unary_expr;
  }
  | '-' UnaryExpr {
    auto unary_expr = new UnaryExprAST();
    unary_expr->flag=UnaryExprAST::OP_UNARY;
    unary_expr->op = "-";
    unary_expr->unary_expr = unique_ptr<BaseExprAST>($2);
    unary_expr->val = -$2->val;
    $$ = unary_expr;
  }
  | '!' UnaryExpr {
    auto unary_expr = new UnaryExprAST();
    unary_expr->flag=UnaryExprAST::OP_UNARY;
    unary_expr->op = "!";
    unary_expr->unary_expr = unique_ptr<BaseExprAST>($2);
    unary_expr->val =!$2->val;
    $$ = unary_expr;
  }
  | IDENT '(' CallParamList ')' {
    auto unary_expr = new UnaryExprAST();
    unary_expr->flag=UnaryExprAST::CALL;
    unary_expr->call_ident = *($1);
    vector<unique_ptr<BaseAST>> *vec = ($3);
    for(auto it = vec->begin(); it!= vec->end(); it++)
      unary_expr->call_expr_list.push_back(move(*it));
    $$ = unary_expr;
  }
  ;

CallParamList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | CallParam {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CallParamList ',' CallParam {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

CallParam
  : Expr {
    auto call_param = new CallParamAST();
    call_param->expr = unique_ptr<BaseExprAST>($1);
    $$ = call_param;
  }
  ;


PrimaryExpr
  : Number {
    auto primary_expr = new PrimaryExprAST();
    primary_expr->flag=PrimaryExprAST::NUMBER;
    primary_expr->number = to_string($1);
    primary_expr->val = $1;
    $$ = primary_expr;
  }
  | '(' Expr ')' {
    auto primary_expr = new PrimaryExprAST();
    primary_expr->flag=PrimaryExprAST::WITH_BRACKETS;
    primary_expr->expr = unique_ptr<BaseExprAST>($2);
    primary_expr->val = $2->val;
    $$ = primary_expr;
  }
  | LVal {
    auto primary_expr = new PrimaryExprAST();
    primary_expr->flag=PrimaryExprAST::LVAL;
    primary_expr->lval = unique_ptr<BaseExprAST>($1);
    primary_expr->val = $1->val;
    $$ = primary_expr;
  }
  ;

AddExpr
  : MulExpr {
    auto add_expr = new AddExprAST();
    add_expr->flag = AddExprAST::ONLY_MUL;
    add_expr->mul_expr = unique_ptr<BaseExprAST>($1);
    add_expr->val = $1->val;
    $$ = add_expr;
  }
  | AddExpr '+' MulExpr {
    auto add_expr = new AddExprAST();
    add_expr->flag = AddExprAST::ADD_MUL;
    add_expr->add_expr = unique_ptr<BaseExprAST>($1);
    add_expr->op = "+";
    add_expr->mul_expr = unique_ptr<BaseExprAST>($3);
    add_expr->val = $1->val + $3->val;
    $$ = add_expr;
  }
  | AddExpr '-' MulExpr {
    auto add_expr = new AddExprAST();
    add_expr->flag = AddExprAST::ADD_MUL;
    add_expr->add_expr = unique_ptr<BaseExprAST>($1);
    add_expr->op = "-";
    add_expr->mul_expr = unique_ptr<BaseExprAST>($3);
    add_expr->val = $1->val - $3->val;
    $$ = add_expr;
  }
  ;

MulExpr
  : UnaryExpr {
    auto mul_expr = new MulExprAST();
    mul_expr->flag = MulExprAST::ONLY_UNARY;
    mul_expr->unary_expr = unique_ptr<BaseExprAST>($1);
    mul_expr->val = $1->val;
    $$ = mul_expr;
  }
  | MulExpr '*' UnaryExpr {
    auto mul_expr = new MulExprAST();
    mul_expr->flag = MulExprAST::MUL_UNARY;
    mul_expr->mul_expr = unique_ptr<BaseExprAST>($1);
    mul_expr->op = "*";
    mul_expr->unary_expr = unique_ptr<BaseExprAST>($3);
    mul_expr->val = $1->val * $3->val;
    $$ = mul_expr;
  }
  | MulExpr '/' UnaryExpr {
    auto mul_expr = new MulExprAST();
    mul_expr->flag = MulExprAST::MUL_UNARY;
    mul_expr->mul_expr = unique_ptr<BaseExprAST>($1);
    mul_expr->op = "/";
    mul_expr->unary_expr = unique_ptr<BaseExprAST>($3);
    $$ = mul_expr;
  }
  | MulExpr '%' UnaryExpr {
    auto mul_expr = new MulExprAST();
    mul_expr->flag = MulExprAST::MUL_UNARY;
    mul_expr->mul_expr = unique_ptr<BaseExprAST>($1);
    mul_expr->op = "%";
    mul_expr->unary_expr = unique_ptr<BaseExprAST>($3);
    $$ = mul_expr;
  }
  ;

LOrExpr
  : LAndExpr {
    auto lor_expr = new LOrExprAST();
    lor_expr->flag = LOrExprAST::ONLY_LAND;
    lor_expr->land_expr = unique_ptr<BaseExprAST>($1);
    lor_expr->val = $1->val;
    $$ = lor_expr;
  }
  | LOrExpr '|' '|' LAndExpr {
    auto lor_expr = new LOrExprAST();
    lor_expr->flag = LOrExprAST::LOR_LAND;
    lor_expr->lor_expr = unique_ptr<BaseExprAST>($1);
    lor_expr->op = "||";
    lor_expr->land_expr = unique_ptr<BaseExprAST>($4);
    lor_expr->val = $1->val || $4->val;
    $$ = lor_expr;
  }
  ;

LAndExpr
  : EqExpr {
    auto land_expr = new LAndExprAST();
    land_expr->flag = LAndExprAST::ONLY_EQ;
    land_expr->eq_expr = unique_ptr<BaseExprAST>($1);
    land_expr->val = $1->val;
    $$ = land_expr;
  }
  | LAndExpr '&' '&' EqExpr {
    auto land_expr = new LAndExprAST();
    land_expr->flag = LAndExprAST::LAND_EQ;
    land_expr->land_expr = unique_ptr<BaseExprAST>($1);
    land_expr->op = "&&";
    land_expr->eq_expr = unique_ptr<BaseExprAST>($4);
    land_expr->val = $1->val && $4->val;
    $$ = land_expr;
  }
  ;

EqExpr
  : RelExpr {
    auto eq_expr = new EqExprAST();
    eq_expr->flag = EqExprAST::ONLY_REL;
    eq_expr->rel_expr = unique_ptr<BaseExprAST>($1);
    eq_expr->val = $1->val;
    $$ = eq_expr;
  }
  | EqExpr '=' '=' RelExpr {
    auto eq_expr = new EqExprAST();
    eq_expr->flag = EqExprAST::EQ_REL;
    eq_expr->eq_expr = unique_ptr<BaseExprAST>($1);
    eq_expr->op = "==";
    eq_expr->rel_expr = unique_ptr<BaseExprAST>($4);
    eq_expr->val = $1->val == $4->val;
    $$ = eq_expr;
  }
  | EqExpr '!' '=' RelExpr {
    auto eq_expr = new EqExprAST();
    eq_expr->flag = EqExprAST::EQ_REL;
    eq_expr->eq_expr = unique_ptr<BaseExprAST>($1);
    eq_expr->op = "!=";
    eq_expr->rel_expr = unique_ptr<BaseExprAST>($4);
    eq_expr->val = $1->val != $4->val;
    $$ = eq_expr;
  }
  ;

RelExpr
  : AddExpr {
    auto rel_expr = new RelExprAST();
    rel_expr->flag = RelExprAST::ONLY_ADD;
    rel_expr->add_expr = unique_ptr<BaseExprAST>($1);
    rel_expr->val = $1->val;
    $$ = rel_expr;
  }
  | RelExpr '<' AddExpr {
    auto rel_expr = new RelExprAST();
    rel_expr->flag = RelExprAST::REL_ADD;
    rel_expr->rel_expr = unique_ptr<BaseExprAST>($1);
    rel_expr->op = "<";
    rel_expr->add_expr = unique_ptr<BaseExprAST>($3);
    rel_expr->val = $1->val < $3->val;
    $$ = rel_expr;
  }
  | RelExpr '>' AddExpr {
    auto rel_expr = new RelExprAST();
    rel_expr->flag = RelExprAST::REL_ADD;
    rel_expr->rel_expr = unique_ptr<BaseExprAST>($1);
    rel_expr->op = ">";
    rel_expr->add_expr = unique_ptr<BaseExprAST>($3);
    rel_expr->val = $1->val > $3->val;
    $$ = rel_expr;
  }
  | RelExpr '<' '=' AddExpr {
    auto rel_expr = new RelExprAST();
    rel_expr->flag = RelExprAST::REL_ADD;
    rel_expr->rel_expr = unique_ptr<BaseExprAST>($1);
    rel_expr->op = "<=";
    rel_expr->add_expr = unique_ptr<BaseExprAST>($4);
    rel_expr->val = $1->val <= $4->val;
    $$ = rel_expr;
  }
  | RelExpr '>' '=' AddExpr {
    auto rel_expr = new RelExprAST();
    rel_expr->flag = RelExprAST::REL_ADD;
    rel_expr->rel_expr = unique_ptr<BaseExprAST>($1);
    rel_expr->op = ">=";
    rel_expr->add_expr = unique_ptr<BaseExprAST>($4);
    rel_expr->val = $1->val >= $4->val;
    $$ = rel_expr;
  }
  ;

LVal
  : IDENT {
    auto lval = new LValAST();
    lval->ident = *unique_ptr<string>($1);
    lval->flag = LValAST::IDENT;
    $$ = lval;
  }
  | IDENT IndexList {
    auto lval = new LValAST();
    lval->ident = *unique_ptr<string>($1);
    lval->flag = LValAST::LVAL_IDX;
    vector<unique_ptr<BaseExprAST>> *vec = ($2);
    for(auto it = vec->begin(); it != vec->end(); it++)
      lval->idx_lst.push_back(move(*it));
    $$ = lval;
  }
  ;

Decl
  : ConstDecl {
    auto decl = new DeclAST();
    decl->const_decl = unique_ptr<BaseAST>($1);
    decl->flag = DeclAST::CONST_DECL;
    $$ = decl;
  }
  | VarDecl {
    auto decl = new DeclAST();
    decl->var_decl = unique_ptr<BaseAST>($1);
    decl->flag = DeclAST::VAR_DECL;
    $$ = decl;
  }
  ;

ConstDecl
  : CONST INT ConstDefList ';'{
    auto const_decl = new ConstDeclAST();
    vector<unique_ptr<BaseAST>> *vec = ($3);
    for (auto it = vec->begin(); it != vec->end(); it++)
      const_decl->const_def_list.push_back(move(*it));
    $$ = const_decl;
  }
  ;

ConstDefList
  : ConstDef {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

BType
  : INT {
    $$ = new string("int");
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto const_def = new ConstDefAST();
    const_def->ident = *unique_ptr<string>($1);
    const_def->const_init_val = unique_ptr<BaseExprAST>($3);
    const_def->flag = ConstDefAST::VAR;
    $$ = const_def;
  }
  | IDENT IndexList '=' VarInitVal {
    auto const_def = new ConstDefAST();
    const_def->ident = *unique_ptr<string>($1);
    const_def->const_init_val = unique_ptr<BaseExprAST>($4);
    const_def->flag = ConstDefAST::ARRAY;
    vector<unique_ptr<BaseExprAST>> *vec = ($2);
    for(auto it = vec->begin(); it != vec->end(); it++)
      const_def->len.push_back(move(*it));
    $$ = const_def;
  }
  ;

ConstInitVal
  : ConstExpr {
    auto const_init_val = new ConstInitValAST();
    const_init_val->flag = ConstInitValAST::EXPR;
    const_init_val->const_expr = unique_ptr<BaseExprAST>($1);
    const_init_val->val = $1->val;
    $$ = const_init_val;
  }
  ;

/* ConstInitValList
  : ConstInitVal {
    vector<unique_ptr<BaseExprAST>> *vec = new vector<unique_ptr<BaseExprAST>>;
    vec->push_back(unique_ptr<BaseExprAST>($1));
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal {
    vector<unique_ptr<BaseExprAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseExprAST>($3));
    $$ = vec;
  }
  ; */

ConstExpr 
  : Expr {
    auto const_expr = new ConstExprAST();
    const_expr->expr = unique_ptr<BaseExprAST>($1);
    const_expr->val = $1->val;
    $$ = const_expr;
  }
  ;

VarDecl
  : INT VarDefList ';'{
    auto var_decl = new VarDeclAST();
    vector<unique_ptr<BaseAST>> *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      var_decl->var_def_list.push_back(move(*it));
    $$ = var_decl;
  }
  ;

VarDefList
  : VarDef {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->is_init = false;
    $$ = var_def;
  }
  | IDENT '=' VarInitVal {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->var_init_val = unique_ptr<BaseExprAST>($3);
    var_def->is_init = true;
    $$ = var_def;
  }
  | IDENT IndexList {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->flag = VarDefAST::ARRAY;
    var_def->var_init_val = unique_ptr<BaseExprAST>(new VarInitValAST());
    var_def->is_init = false;
    vector<unique_ptr<BaseExprAST>> *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      var_def->len.push_back(move(*it));
    $$ = var_def;
  }
  | IDENT IndexList '=' VarInitVal {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->var_init_val = unique_ptr<BaseExprAST>($4);
    var_def->is_init = true;
    var_def->flag = VarDefAST::ARRAY;
    vector<unique_ptr<BaseExprAST>> *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      var_def->len.push_back(move(*it));
    $$ = var_def;
  }
  ;

IndexList
  : '[' Expr ']' {
    vector<unique_ptr<BaseExprAST>> *vec = new vector<unique_ptr<BaseExprAST>>;
    vec->push_back(unique_ptr<BaseExprAST>($2));
    $$ = vec;
  }
  | IndexList '[' Expr ']' {
    vector<unique_ptr<BaseExprAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseExprAST>($3));
    $$ = vec;
  }
  ;

VarInitVal
  : Expr {
    auto var_init_val = new VarInitValAST();
    var_init_val->flag = VarInitValAST::EXPR;
    var_init_val->expr = unique_ptr<BaseExprAST>($1);
    var_init_val->val = $1->val;
    $$ = var_init_val;
  }
  | '{' '}' {
    auto var_init_val = new VarInitValAST();
    var_init_val->flag = VarInitValAST::ARRAY;
    $$ = var_init_val;
  }
  | '{' VarInitValList '}' {
    auto var_init_val = new VarInitValAST();
    var_init_val->flag = VarInitValAST::ARRAY;
    vector<unique_ptr<BaseExprAST>> *vec = ($2);
    for (auto it = vec->begin(); it!= vec->end(); it++)
      var_init_val->expr_list.push_back(move(*it));
    $$ = var_init_val;
  }
  ;

VarInitValList
  : VarInitVal {
    vector<unique_ptr<BaseExprAST>> *vec = new vector<unique_ptr<BaseExprAST>>;
    vec->push_back(unique_ptr<BaseExprAST>($1));
    $$ = vec;
  }
  | VarInitValList ',' VarInitVal {
    vector<unique_ptr<BaseExprAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseExprAST>($3));
    $$ = vec;
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
/* void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
} */
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  
    extern int yylineno;    // defined and maintained in lex
    extern char *yytext;    // defined and maintained in lex
    int len=strlen(yytext);
    int i;
    char buf[512]={0};
    for (i=0;i<len;++i)
    {
        sprintf(buf,"%s%d ",buf,yytext[i]);
    }
    fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);

}