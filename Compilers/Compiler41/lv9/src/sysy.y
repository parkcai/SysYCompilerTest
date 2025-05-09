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

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>>* block_items;
  std::vector<std::unique_ptr<BaseAST>>* const_defs;
  std::vector<std::unique_ptr<BaseAST>>* var_defs;
  std::vector<std::unique_ptr<BaseAST>>* global_defs;
  std::vector<std::unique_ptr<BaseAST>>* fparams;
  std::vector<std::unique_ptr<BaseAST>>* rparams;
  std::vector<std::unique_ptr<BaseAST>>* array_const_init_val;
  std::vector<std::unique_ptr<BaseAST>>* array_init_val;
}

%token INT VOID RETURN CONST
%token LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP
%token IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> CompUnit GlobalDef FuncDef FuncFParams FuncFParam FuncRParams FuncFParamType
%type <ast_val> Block Stmt IfStmt ElseStmt Exp 
%type <ast_val> UnaryExp PrimaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal BlockItem LVal ConstExp
%type <ast_val> ArrayIdent
%type <str_val> Number UnaryOp

%type <block_items> BlockItems
%type <const_defs> ConstDefs
%type <var_defs> VarDefs
%type <global_defs> GlobalDefs
%type <fparams> FParams
%type <rparams> RParams
%type <array_const_init_val> ArrayConstInitVal
%type <array_init_val> ArrayInitVal

%%

CompUnit
  : GlobalDefs {
    auto comp_unit = make_unique<CompUnitAST>();
    for (int i = 0; i < $1->size(); i++) {
      comp_unit->global_defs.push_back(std::move((*$1)[i]));
    }
    ast = move(comp_unit);
  }
  ;

Decl
  : ConstDecl {
    auto decl = new DeclAST();
    decl->const_decl = unique_ptr<BaseAST>($1);
    decl->type = DeclAST::Type::CONST_DECL;
    $$ = decl;
  }
  | VarDecl {
    auto decl = new DeclAST();
    decl->var_decl = unique_ptr<BaseAST>($1);
    decl->type = DeclAST::Type::VAR_DECL;
    $$ = decl;
  }
  ;

ConstDecl
  : CONST INT ConstDef ConstDefs ';' {
    auto const_decl = new ConstDeclAST();
    const_decl->btype = "int";
    const_decl->const_def.push_back(unique_ptr<BaseAST>($3));
    for (int i = 0; i < $4->size(); i++) {
      const_decl->const_def.push_back(std::move((*$4)[i]));
    }
    $$ = const_decl;
  }
  ;

ConstDefs
  : /* empty */ {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | ConstDefs ',' ConstDef {
    auto const_def = unique_ptr<BaseAST>($3);
    $1->push_back(std::move(const_def));
    $$ = $1;
  }
  ;

ArrayIdent
  : IDENT '[' ConstExp ']' {
    auto array_ident = new ArrayIdentAST();
    array_ident->array_ident = nullptr;
    array_ident->ident = *unique_ptr<string>($1);
    array_ident->const_exp = unique_ptr<BaseAST>($3);
    $$ = array_ident;
  }
  | ArrayIdent '[' ConstExp ']' {
    auto array_ident = new ArrayIdentAST();
    array_ident->array_ident = unique_ptr<BaseAST>($1);
    array_ident->const_exp = unique_ptr<BaseAST>($3);
    $$ = array_ident;
  }
  ;

ArrayConstInitVal
  : ConstInitVal {
    auto array_const_init_val = new std::vector<std::unique_ptr<BaseAST>>();
    array_const_init_val->push_back(unique_ptr<BaseAST>($1));
    $$ = array_const_init_val;
  }
  | ArrayConstInitVal ',' ConstInitVal {
    auto const_init_val = unique_ptr<BaseAST>($3);
    $1->push_back(std::move(const_init_val));
    $$ = $1;
  }
  ;

ArrayInitVal
  : InitVal {
    auto array_init_val = new std::vector<std::unique_ptr<BaseAST>>();
    array_init_val->push_back(unique_ptr<BaseAST>($1));
    $$ = array_init_val;
  }
  | ArrayInitVal ',' InitVal {
    auto init_val = unique_ptr<BaseAST>($3);
    $1->push_back(std::move(init_val));
    $$ = $1;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto const_def = new ConstDefAST();
    const_def->ident = *unique_ptr<string>($1);
    const_def->const_init_val = unique_ptr<BaseAST>($3);
    const_def->type = ConstDefAST::Type::CONST_INT;
    $$ = const_def;
  }
  | ArrayIdent '=' ConstInitVal {
    auto const_def = new ConstDefAST();
    const_def->array_ident = unique_ptr<BaseAST>($1);
    const_def->const_init_val = unique_ptr<BaseAST>($3);
    const_def->type = ConstDefAST::Type::CONST_ARRAY;
    $$ = const_def;
  }
  ;

ConstInitVal
  : ConstExp {
    auto const_init_val = new ConstInitValAST();
    const_init_val->const_exp = unique_ptr<BaseAST>($1);
    const_init_val->type = ConstInitValAST::Type::CONST_EXP;
    $$ = const_init_val;
  }
  | '{' ArrayConstInitVal '}' {
    auto const_init_val = new ConstInitValAST();
    for (int i = 0; i < $2->size(); i++) {
      const_init_val->array_const_init_val.push_back(std::move((*$2)[i]));
    }
    const_init_val->type = ConstInitValAST::Type::CONST_ARRAY;
    $$ = const_init_val;
  }
  | '{' '}' {
    auto const_init_val = new ConstInitValAST();
    const_init_val->type = ConstInitValAST::Type::CONST_ARRAY;
    $$ = const_init_val;
  }
  ;

VarDecl
  : INT VarDef VarDefs ';' {
    auto var_decl = new VarDeclAST();
    var_decl->btype = "int";
    var_decl->var_def.push_back(unique_ptr<BaseAST>($2));
    for (int i = 0; i < $3->size(); i++) {
      var_decl->var_def.push_back(std::move((*$3)[i]));
    }
    $$ = var_decl;
  }
  ;

VarDefs 
  : /* empty */ {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | VarDefs ',' VarDef {
    auto var_def = unique_ptr<BaseAST>($3);
    $1->push_back(std::move(var_def));
    $$ = $1;
  }
  ;

VarDef
  : IDENT {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->type = VarDefAST::Type::UNINIT;
    $$ = var_def;
  }
  | IDENT '=' InitVal {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->init_val = unique_ptr<BaseAST>($3);
    var_def->type = VarDefAST::Type::INIT;
    $$ = var_def;
  }
  | ArrayIdent {
    auto var_def = new VarDefAST();
    var_def->array_ident = unique_ptr<BaseAST>($1);
    var_def->type = VarDefAST::Type::ARRAY_UNINIT;
    $$ = var_def;
  }
  | ArrayIdent '=' InitVal {
    auto var_def = new VarDefAST();
    var_def->array_ident = unique_ptr<BaseAST>($1);
    var_def->init_val = unique_ptr<BaseAST>($3);
    var_def->type = VarDefAST::Type::ARRAY_INIT;
    $$ = var_def;
  }
  ;

InitVal
  : Exp {
    auto init_val = new InitValAST();
    init_val->exp = unique_ptr<BaseAST>($1);
    init_val->type = InitValAST::Type::EXP;
    $$ = init_val;
  }
  | '{' ArrayInitVal '}' {
    auto init_val = new InitValAST();
    for (int i = 0; i < $2->size(); i++) {
      init_val->array_init_val.push_back(std::move((*$2)[i]));
    }
    init_val->type = InitValAST::Type::ARRAY;
    $$ = init_val;
  }
  | '{' '}' {
    auto init_val = new InitValAST();
    init_val->type = InitValAST::Type::ARRAY;
    $$ = init_val;
  }
  ;

GlobalDefs
  : GlobalDef {
    auto global_defs = new std::vector<std::unique_ptr<BaseAST>>();
    global_defs->push_back(unique_ptr<BaseAST>($1));
    $$ = global_defs;
  }
  | GlobalDefs GlobalDef {
    auto global_def = unique_ptr<BaseAST>($2);
    $1->push_back(std::move(global_def));
    $$ = $1;
  }
  ;

GlobalDef
  : FuncDef {
    auto global_def = new GlobalDefAST();
    global_def->func_def = unique_ptr<BaseAST>($1);
    global_def->type = GlobalDefAST::Type::FUNC_DEF;
    $$ = global_def;
  }
  | Decl {
    auto global_def = new GlobalDefAST();
    global_def->decl = unique_ptr<BaseAST>($1);
    global_def->type = GlobalDefAST::Type::DECL;
    $$ = global_def;
  }

FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = nullptr;
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = nullptr;
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam FParams {
    auto func_fparams = new FuncFParamsAST();
    func_fparams->fparams.push_back(unique_ptr<BaseAST>($1));
    for (int i = 0; i < $2->size(); i++) {
      func_fparams->fparams.push_back(std::move((*$2)[i]));
    }
    $$ = func_fparams;
  }

FParams
  : /* empty */ {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | FParams ',' FuncFParam {
    auto fparam = unique_ptr<BaseAST>($3);
    $1->push_back(std::move(fparam));
    $$ = $1;
  }
  ;

FuncFParamType
  : '[' ']' {
    auto func_fparam_type = new FuncFParamTypeAST();
    func_fparam_type->func_param_type = nullptr;
    func_fparam_type->const_exp = nullptr;
    $$ = func_fparam_type;
  }
  | FuncFParamType '[' ConstExp ']' {
    auto func_fparam_type = new FuncFParamTypeAST();
    func_fparam_type->func_param_type = unique_ptr<BaseAST>($1);
    func_fparam_type->const_exp = unique_ptr<BaseAST>($3);
    $$ = func_fparam_type;
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->func_param_type = nullptr;
    $$ = ast;
  }
  | INT IDENT FuncFParamType {
    auto ast = new FuncFParamAST();
    ast->btype = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->func_param_type = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItems '}' {
    auto block = new BlockAST();
    for (int i = 0; i < $2->size(); i++) {
      block->block_items.push_back(std::move((*$2)[i]));
    }
    $$ = block;
  }
  ;

BlockItems:
  /* empty */ {
      $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | BlockItems BlockItem {
      auto block_item = unique_ptr<BaseAST>($2);
      $1->push_back(std::move(block_item));
      $$ = $1;
  }
  ;

BlockItem
  : Decl {
    auto block_item = new BlockItemAST();
    block_item->decl = unique_ptr<BaseAST>($1);
    block_item->type = BlockItemAST::Type::DECL;
    $$ = block_item;
  }
  | Stmt {
    auto block_item = new BlockItemAST();
    block_item->stmt = unique_ptr<BaseAST>($1);
    block_item->type = BlockItemAST::Type::STMT;
    $$ = block_item;
  }
  ;

Stmt
  : LVal '=' Exp ';' {
    auto stmt = new StmtAST();
    stmt->lval = unique_ptr<BaseAST>($1);
    stmt->exp = unique_ptr<BaseAST>($3);
    stmt->type = StmtAST::Type::LVAL;
    $$ = stmt;
  }
  | Exp ';' {
    auto stmt = new StmtAST();
    stmt->exp = unique_ptr<BaseAST>($1);
    stmt->type = StmtAST::Type::EXP;
    $$ = stmt;
  }
  | ';' {
    auto stmt = new StmtAST();
    stmt->exp = nullptr;
    stmt->type = StmtAST::Type::EXP;
    $$ = stmt;
  }
  | Block {
    auto stmt = new StmtAST();
    stmt->block = unique_ptr<BaseAST>($1);
    stmt->exp = nullptr;
    stmt->type = StmtAST::Type::BLOCK;
    $$ = stmt;
  }
  | IF '(' Exp ')' IfStmt ElseStmt {
    auto stmt = new StmtAST();
    stmt->exp = unique_ptr<BaseAST>($3);
    stmt->if_stmt = unique_ptr<BaseAST>($5);
    stmt->else_stmt = unique_ptr<BaseAST>($6);
    stmt->type = StmtAST::Type::IF;
    $$ = stmt;
  }
  | WHILE '(' Exp ')' Stmt {
    auto stmt = new StmtAST();
    stmt->exp = unique_ptr<BaseAST>($3);
    stmt->while_stmt = unique_ptr<BaseAST>($5);
    stmt->type = StmtAST::Type::WHILE;
    $$ = stmt;
  }
  | BREAK ';' {
    auto stmt = new StmtAST();
    stmt->exp = nullptr;
    stmt->type = StmtAST::Type::BREAK;
    $$ = stmt;
  }
  | CONTINUE ';' {
    auto stmt = new StmtAST();
    stmt->exp = nullptr;
    stmt->type = StmtAST::Type::CONTINUE;
    $$ = stmt;
  }
  | RETURN Exp ';' {
    auto stmt = new StmtAST();
    stmt->exp = unique_ptr<BaseAST>($2);
    stmt->type = StmtAST::Type::RETURN;
    $$ = stmt;
  }
  | RETURN ';' {
    auto stmt = new StmtAST();
    stmt->exp = nullptr;
    stmt->type = StmtAST::Type::RETURN;
    $$ = stmt;
  }
  ;

IfStmt
  : Stmt {
    auto if_stmt = new IfStmtAST();
    if_stmt->stmt = unique_ptr<BaseAST>($1);
    $$ = if_stmt;
  }
  ;

ElseStmt
  : ELSE Stmt {
    auto else_stmt = new ElseStmtAST();
    else_stmt->stmt = unique_ptr<BaseAST>($2);
    $$ = else_stmt;
  }
  | /* empty */ {
    auto else_stmt = new ElseStmtAST();
    else_stmt->stmt = nullptr;
    $$ = else_stmt;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->type = PrimaryExpAST::Type::EXP;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->type = PrimaryExpAST::Type::LVAL;
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = *unique_ptr<string>($1);
    ast->type = PrimaryExpAST::Type::NUMBER;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    ast->type = UnaryExpAST::Type::PRIMARY_EXP;
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_rparams = nullptr;
    ast->type = UnaryExpAST::Type::FUNC_EXP;
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_rparams = unique_ptr<BaseAST>($3);
    ast->type = UnaryExpAST::Type::FUNC_EXP;
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = *unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    ast->type = UnaryExpAST::Type::UNARY_EXP;
    $$ = ast;
  }
  ;

FuncRParams
  : Exp RParams {
    auto func_rparams = new FuncRParamsAST();
    func_rparams->rparams.push_back(unique_ptr<BaseAST>($1));
    for (int i = 0; i < $2->size(); i++) {
      func_rparams->rparams.push_back(std::move((*$2)[i]));
    }
    $$ = func_rparams;
  }
  ;

RParams
  : /* empty */ {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | RParams ',' Exp {
    auto rparam = unique_ptr<BaseAST>($3);
    $1->push_back(std::move(rparam));
    $$ = $1;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    ast->type = MulExpAST::Type::UNARY_EXP;
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("*"));
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->type = MulExpAST::Type::MUL_EXP;
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("/"));
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->type = MulExpAST::Type::MUL_EXP;
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("%"));
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->type = MulExpAST::Type::MUL_EXP;
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->type = AddExpAST::Type::MUL_EXP;
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("+"));
    ast->mul_exp = unique_ptr<BaseAST>($3);
    ast->type = AddExpAST::Type::ADD_EXP;
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("-"));
    ast->mul_exp = unique_ptr<BaseAST>($3);
    ast->type = AddExpAST::Type::ADD_EXP;
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->type = RelExpAST::Type::ADD_EXP;
    $$ = ast;
  }
  | RelExp LE_OP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("<="));
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->type = RelExpAST::Type::REL_EXP;
    $$ = ast;
  }
  | RelExp GE_OP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string(">="));
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->type = RelExpAST::Type::REL_EXP;
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("<"));
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->type = RelExpAST::Type::REL_EXP;
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string(">"));
    ast->add_exp = unique_ptr<BaseAST>($3);
    ast->type = RelExpAST::Type::REL_EXP;
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->type = EqExpAST::Type::REL_EXP;
    $$ = ast;
  }
  | EqExp EQ_OP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("=="));
    ast->rel_exp = unique_ptr<BaseAST>($3);
    ast->type = EqExpAST::Type::EQ_EXP;
    $$ = ast;
  }
  | EqExp NE_OP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("!="));
    ast->rel_exp = unique_ptr<BaseAST>($3);
    ast->type = EqExpAST::Type::EQ_EXP;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->type = LAndExpAST::Type::EQ_EXP;
    $$ = ast;
  }
  | LAndExp AND_OP EqExp {
    auto ast = new LAndExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("&&"));
    ast->eq_exp = unique_ptr<BaseAST>($3);
    ast->type = LAndExpAST::Type::LAND_EXP;
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->type = LOrExpAST::Type::LAND_EXP;
    $$ = ast;
  }
  | LOrExp OR_OP LAndExp {
    auto ast = new LOrExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->op = *(new std::string("||"));
    ast->land_exp = unique_ptr<BaseAST>($3);
    ast->type = LOrExpAST::Type::LOR_EXP;
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto const_exp = new ConstExpAST();
    const_exp->exp = unique_ptr<BaseAST>($1);
    $$ = const_exp;
  }
  ;

LVal
  : IDENT {
    auto lval = new LValAST();
    lval->ident = *unique_ptr<string>($1);
    lval->type = LValAST::Type::IDENT;
    $$ = lval;
  }
  | LVal '[' Exp ']' {
    auto lval = new LValAST();
    lval->lval = unique_ptr<BaseAST>($1);
    lval->exp = unique_ptr<BaseAST>($3);
    lval->type = LValAST::Type::ARRAY;
    $$ = lval;
  }
  ;

Number
  : INT_CONST {
    $$ = new string(to_string($1));
  }
  ;

UnaryOp
  : '+' { $$ = new std::string("+"); }
  | '-' { $$ = new std::string("-"); }
  | '!' { $$ = new std::string("!"); }
  ;

%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}