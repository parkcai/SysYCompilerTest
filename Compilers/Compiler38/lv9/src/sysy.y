%code requires {
  #include <memory>
  #include <string>
  #include "ast_class.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast_class.h"

int yylex();
void yyerror(CompUnitAST* ast, const char *s);

using namespace std;

%}


%parse-param { CompUnitAST* &ast }


%union {
  std::string *str_val;
  int int_val;
  CompUnitAST *CompUnitAST_ast_val;
  FuncDefAST *FuncDefAST_ast_val;
  BlockAST *BlockAST_ast_val;
  BlockItemAST *BlockItemAST_ast_val;
  StmtAST *StmtAST_ast_val;
  ExpAST *ExpAST_ast_val;
  PrimaryExpAST *PrimaryExpAST_ast_val;
  DeclAST *DeclAST_ast_val;
  DefAST *DefAST_ast_val;
  ValueAST *ValueAST_ast_val;
  InitvalAST *InitvalAST_ast_val;
  FuncFParamsAST *FuncFParamsAST_ast_val;
  FuncFParamAST *FuncFParamAST_ast_val;
  FuncRParamsAST *FuncRParamsAST_ast_val;
  BaseAST *BaseAST_ast_val;
}

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST

%type <CompUnitAST_ast_val>      CompUnit
%type <FuncDefAST_ast_val>       FuncDef
%type <BlockAST_ast_val>         Block BlockItems
%type <BlockItemAST_ast_val>     BlockItem
%type <StmtAST_ast_val>          SStmt MatchStmt UnmatchStmt Stmt
%type <ExpAST_ast_val>           Exp LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp ConstExp
%type <PrimaryExpAST_ast_val>    PrimaryExp
%type <DeclAST_ast_val>          Decl ConstDecl VarDecl ConstDecls VarDecls 
%type <DefAST_ast_val>           ConstDef VarDef
%type <ValueAST_ast_val>         ConstIdent VarIdent LVal ParaIdent
%type <InitvalAST_ast_val>       ConstInitval VarInitval ConstInitvals VarInitvals
%type <FuncFParamsAST_ast_val>   FuncFParams
%type <FuncFParamAST_ast_val>    FuncFParam
%type <FuncRParamsAST_ast_val>   FuncRParams
%%

CompUnit
  : Decl { 
    ast = new CompUnitAST();
    ast->decls.push_back($1);
    $$ = ast;
  }
  | FuncDef {
     ast = new CompUnitAST();
     ast->funcs.push_back($1);
     $$ = ast; 
  }
  | CompUnit Decl { 
    auto ast = $1;
    ast->decls.push_back($2); 
    $$ = ast; 
  }
  | CompUnit FuncDef {
    auto ast = $1;
    ast->funcs.push_back($2); 
    $$ = ast; 
  }
  ;

FuncDef
  : INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "i32";
    ast->ident = *$2;
    ast->params = $4;
    ast->block = $6;
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "";
    ast->ident = *$2;
    ast->params = $4;
    ast->block = $6;
    $$ = ast;
  }
  ;

Block
  : BlockItems '}' { $$ = $1;}
  ;

BlockItems 
  : '{' {$$ = new BlockAST();}
  | BlockItems BlockItem {
    auto ast = $1;
    ast->items.push_back($2);
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->type = BlockItem_type::Decl;
    ast->decl = $1;
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->type = BlockItem_type::Stmt;
    ast->stmt = $1;
    $$ = ast;
  }
  ;

Stmt 
  : MatchStmt {$$ = $1;}
  | UnmatchStmt {$$ = $1;}
  ;

UnmatchStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = Stmt_type::If_Then;
    ast->exp = $3;
    ast->stmt1 = $5;
    $$ = ast;
  }
  | IF '(' Exp ')' MatchStmt ELSE UnmatchStmt {
    auto ast = new StmtAST();
    ast->type = Stmt_type::If_Then_Else;
    ast->exp = $3;
    ast->stmt1 = $5;
    ast->stmt2 = $7;
    $$ = ast;
  }

MatchStmt
  : IF '(' Exp ')' MatchStmt ELSE MatchStmt {
    auto ast = new StmtAST();
    ast->type = Stmt_type::If_Then_Else;
    ast->exp = $3;
    ast->stmt1 = $5;
    ast->stmt2 = $7;
    $$ = ast;
  }
  | SStmt {$$ = $1;}
  ;

SStmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = Stmt_type::Return_Exp;
    ast->exp = $2;
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = Stmt_type::Assign;
    ast->lval = $1;
    ast->exp = $3;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = Stmt_type::Block;
    ast->block = $1;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->type = Stmt_type::Exp;
    ast->exp = $1;
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = Stmt_type::Empty;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = Stmt_type::Return_Void;
    $$ = ast;
  }
  | BREAK ';' { 
    auto ast = new StmtAST();
    ast->type = Stmt_type::Break;
    $$ = ast;
  }
  | CONTINUE ';' { 
    auto ast = new StmtAST();
    ast->type = Stmt_type::Continue;
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = Stmt_type::While;
    ast->exp = $3;
    ast->stmt1 = $5;
    $$ = ast;
  }
  ;

Decl
  : ConstDecl { $$ = $1; }
  | VarDecl { $$ = $1; }
  ;
  
ConstDecl
  : ConstDecls ';' { $$ = $1; };

VarDecl
  : VarDecls ';' { $$ = $1; };

ConstDecls
  : CONST INT ConstDef {
    auto ast = new DeclAST();
    ast->type = Decl_type::Const;
    ast->btype = "int";
    ast->defs.push_back($3);
    $$ = ast;
  }
  | ConstDecls ',' ConstDef { 
    auto ast = $1;
    ast->defs.push_back($3);
    $$ = ast;
  }
  ;
  
VarDecls
  : INT VarDef {
    auto ast = new DeclAST();
    ast->type = Decl_type::Var;
    ast->btype = "int";
    ast->defs.push_back($2);
    $$ = ast;
  }
  | VarDecls ',' VarDef { 
    auto ast = $1;
    ast->defs.push_back($3);
    $$ = ast;
  }
  ;

ConstDef
  : ConstIdent '=' ConstInitval {
    auto ast = new DefAST();
    ast->type = Def_type::Const;
    ast->value = $1;
    ast->initval = $3;
    $$ = ast;
  }
  ;

VarDef
  : VarIdent {
    auto ast = new DefAST();
    ast->type = Def_type::Var;
    ast->value = $1;
    $$ = ast;
  }
  | VarIdent '=' VarInitval {
    auto ast = new DefAST();
    ast->type = Def_type::Var;
    ast->value = $1;
    ast->initval = $3;
    $$ = ast;
  }
  ;

FuncFParams
  : {$$ = new FuncFParamsAST();}
  | FuncFParam {
    auto ast = new FuncFParamsAST();
    ast->params.push_back($1);
    $$ = ast;
  }
  | FuncFParams ',' FuncFParam {
    auto ast = $1;
    ast->params.push_back($3); 
    $$ = ast;
  }
  ;

FuncFParam
  : INT ParaIdent {
    auto ast = new FuncFParamAST();
    ast->value = $2;
    $$ = ast;
  }
  ;

FuncRParams
  : {$$ = new FuncRParamsAST();}
  | Exp {
    auto ast = new FuncRParamsAST();
    ast->exps.push_back($1);
    $$ = ast;
  }
  | FuncRParams ',' Exp {
    auto ast = $1;
    ast->exps.push_back($3);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new ValueAST();
    ast->ident = *$1;
    ast->type = Value_type::LVal;
    $$ = ast;
  }
  | LVal '[' Exp ']' {
    auto ast = $1;
    ast->exps.push_back($3);
    $$ = ast;
  }
  ;

ConstIdent
  : IDENT {
    auto ast = new ValueAST();
    ast->ident = *$1;
    ast->type = Value_type::ConstIdent;
    $$ = ast; 
  }
  | ConstIdent '[' ConstExp ']' {
    auto ast = $1;
    ast->exps.push_back($3);
    $$ = ast;
  }
  ;

VarIdent
  : IDENT {
    auto ast = new ValueAST();
    ast->ident = *$1;
    ast->type = Value_type::VarIdent;
    $$ = ast; 
  }
  | VarIdent '[' ConstExp ']' {
    auto ast = $1;
    ast->exps.push_back($3);
    $$ = ast;
  }
  ;

ParaIdent
  : IDENT {
    auto ast = new ValueAST();
    ast->ident = *$1;
    ast->type = Value_type::VarIdent;
    $$ = ast; 
  }
  | IDENT '[' ']' {
    auto ast = new ValueAST();
    ast->ident = *$1;
    ast->type = Value_type::VarIdent;
    ast->empty_array_init = true;
    $$ = ast;
  }
  | ParaIdent '[' ConstExp ']' {
    auto ast = $1;
    ast->exps.push_back($3);
    $$ = ast;
  }
  ;

ConstInitval
  : ConstExp {
    auto ast = new InitvalAST();
    ast->type = Initval_type::Const;
    ast->exp = $1;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitvalAST();
    ast->type = Initval_type::Const;
    $$ = ast;
  }
  | ConstInitvals '}' { $$ = $1; }
  ;

VarInitval
  : Exp {
    auto ast = new InitvalAST();
    ast->type = Initval_type::Var;
    ast->exp = $1;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitvalAST();
    ast->type = Initval_type::Var;
    $$ = ast;
  }
  | VarInitvals '}' { $$ = $1; }
  ;

ConstInitvals
  : '{' ConstInitval {
    auto ast = new InitvalAST();
    ast->type = Initval_type::Const;
    ast->inits.push_back($2);
    $$ = ast;
  }
  | ConstInitvals ',' ConstInitval {
    auto ast = $1;
    ast->inits.push_back($3);
    $$ = ast;
  }
  ;

VarInitvals
  : '{' VarInitval {
    auto ast = new InitvalAST();
    ast->type = Initval_type::Var;
    ast->inits.push_back($2);
    $$ = ast;
  }
  | VarInitvals ',' VarInitval {
    auto ast = $1;
    ast->inits.push_back($3);
    $$ = ast;
  }
  ;

ConstExp
  : Exp { $$ = $1;};

Exp 
  : LOrExp { $$ = $1;};

LOrExp
  : LAndExp { $$ = $1;}
  | LOrExp LOROP LAndExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::LOR;
    ast->right = $3;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp { $$ = $1;}
  | LAndExp LANDOP EqExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::LAND;
    ast->right = $3;
    $$ = ast;
  }
  ;

EqExp
  : RelExp { $$ = $1;}
  | EqExp EQOP RelExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = (*$2) == "==" ? Exp_op::EQ : Exp_op::NE;
    ast->right = $3;
    $$ = ast;
  }
  ;

RelExp
  : AddExp { $$ = $1;}
  | RelExp RELOP AddExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = (*$2) == ">" ? Exp_op::GT : (*$2) == "<" ? Exp_op::LT : (*$2) == ">=" ? Exp_op::GE : Exp_op::LE;
    ast->right = $3;
    $$ = ast;
  }
  ;

AddExp
  : MulExp { $$ = $1;}
  | AddExp '+' MulExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::ADD;
    ast->right = $3;
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::SUB;
    ast->right = $3;
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp { $$ = $1;}
  | MulExp '*' UnaryExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::MUL;
    ast->right = $3;
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::DIV;
    ast->right = $3;
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new ExpAST();
    ast->left = $1;
    ast->op = Exp_op::MOD;
    ast->right = $3;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
     auto ast = new ExpAST();
     ast->primary = $1;
     ast->op = Exp_op::PRI;
     $$ = ast;
  }
  | '+' UnaryExp {
    auto ast = new ExpAST();
    ast->right = $2;
    ast->op = Exp_op::POS;
    $$ = ast;
  }
  | '-' UnaryExp {
    auto ast = new ExpAST();
    ast->right = $2;
    ast->op = Exp_op::NEG;
    $$ = ast;
  }
  | '!' UnaryExp {
    auto ast = new ExpAST();
    ast->right = $2;
    ast->op = Exp_op::NOT;
    $$ = ast;
  }
  | '(' Exp ')' { $$ = $2;}
  ;

PrimaryExp
  : LVal {
    auto ast = new PrimaryExpAST();
    ast->type = PrimaryExp_type::LVal;
    ast->lval = $1;
    $$ = ast;
  }
  | INT_CONST {
    auto ast = new PrimaryExpAST();
    ast->type = PrimaryExp_type::INT_CONST;
    ast->const_val = $1;
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new PrimaryExpAST();
    ast->type = PrimaryExp_type::FuncCall;
    ast->func_name = *$1;
    ast->params = $3;
    $$ = ast;
  }
  ;

%%

// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(CompUnitAST* ast, const char *s) {
  std::cerr << "Call yyerror "<< s << std::endl;
}