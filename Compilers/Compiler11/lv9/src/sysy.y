%code requires {
  #include <memory>
  #include <string>
  #include "AST.hpp"
}

%{
#include "AST.hpp"
#include <iostream>
#include <memory>
#include <string>
// 声明 lexer 函数和错误处理函数
using namespace std;
extern int yylex();
extern void yyerror(Program **program, const char *s);
%}


%union{
    int intDef;
    string *strDef;
    BaseAST* baseastDef;
    DeclareDef *defDef;
    Var *varDef;
    BlockItems *itemsDef;
    Decls *declDef;
    Program *proDef;
}

%parse-param {Program **program}

//T开头的代表终结符
%token TInt TReturn TLogicAnd TLogicOr TCst TIf TElse TWhile TBreak TContinue TVoid
%token <strDef> TIdent
%token <intDef> TIntCst

//非终结符们，参考了直接用宏定义来intDef减少代码量的技术
%type <baseastDef> Expr AtomExpr AndExpr AddSubExpr MulDivExpr EqualExpr CompareExpr UnaryExpr FuncDef Block Stmt Number BlockItem MS UMS IfExpr FuncRealParams CompUnit InitVals InitVal
%type <intDef> VarType UnaryOp AddSubOp MulDivOp CompareOp EqualOp
%type <defDef> ConstDef VarDef FuncParam FuncArrayParam
%type <declDef> VarDecl Decl ConstDecl FuncParams
%type <itemsDef> BlockItems 
%type <proDef> CompUnits
%type <varDef> Var

%%
Program : CompUnits { *program = $1; }

CompUnits : CompUnits CompUnit { ($1->units).push_back($2); $$ = $1; }
          | CompUnit { Program *pro = new Program(); pro->units.push_back($1); $$ = pro; }

CompUnit : Decl { $$ = $1; }
         | FuncDef { $$ = $1; }

FuncDef : VarType TIdent '(' ')' Block { FuncDef* ast = new FuncDef(); ast->id = *($2); ast->func_type = $1; delete $2; ast->block = $5; $$ = ast; }
        | VarType TIdent '(' FuncParams ')' Block { FuncDef* ast = new FuncDef(); ast->id = *($2); ast->func_type = $1; delete $2; ast->block = $6; ast->params = $4->defs; $$ = ast; }

FuncParams : FuncParams ',' FuncParam { ($1->defs).push_back($3); $$ = $1;}
           | FuncParam { Decls *decl = new Decls(); (decl->defs).push_back($1); $$ = decl; }

FuncParam : VarType TIdent { DeclareDef *def = new DeclareDef(*($2)); def->declType = ParamDecl; delete $2; $$ = def; }
          | FuncArrayParam { $$ = $1; }

FuncArrayParam : FuncArrayParam '[' Expr ']' { ($1->offset).push_back($3); $$ = $1; }
               | VarType TIdent '[' ']' { DeclareDef *def = new DeclareDef(*($2)); def->declType = ParamDecl; def->type = TypePointer; delete $2; $$ = def; }

FuncRealParams : FuncRealParams ',' Expr { (((FuncCall*)$1)->params).push_back($3); $$ = $1; }
               | Expr { FuncCall *call = new FuncCall(); (call->params).push_back($1); $$ = call; }

VarType : TInt { $$ = TypeInt; }
        | TVoid { $$ = TypeVoid; }

Block : '{' BlockItems '}' { Block* ast = new Block(); ast->stmts = $2->vec; delete $2; $$ = ast; }

BlockItems : BlockItems BlockItem {
                if ($2) {
                    if ($2->is_decls()) {
                        auto decl = static_cast<Decls*>($2);
                        ($1->vec).insert(($1->vec).end(), decl->defs.begin(), decl->defs.end());
                    } else {
                        ($1->vec).push_back($2);
                    }
                }
                $$ = $1;
             }
           |  { BlockItems *blockHead = new BlockItems(); $$ = blockHead; }

BlockItem : Decl { $$ = static_cast<BaseAST*>$1; }
          | Stmt { $$ = $1; }

Decl : ConstDecl ';' { $$ = $1; }
     | VarDecl ';' { $$ = $1; }

ConstDecl : ConstDecl ',' ConstDef { $3->type = $1->type; $3->declType = ConstDecl; ($1->defs).insert(($1->defs).end(),$3); $$ = $1; }
          | TCst VarType ConstDef { Decls *decl = new Decls($2); $3->type = $2; $3->declType = ConstDecl; decl->defs.insert((decl->defs).end(),$3); $$ = decl; }

VarDecl : VarDecl ',' VarDef { $3->type = $1->type; $3->declType = VarDecl; ($1->defs).insert(($1->defs).end(),$3); $$ = $1; }
        | VarType VarDef { Decls *decl = new Decls($1);  $2->declType = VarDecl; $2->type = $1; decl->defs.insert((decl->defs).end(),$2); $$ = decl; }

VarDef : Var { DeclareDef *def = new DeclareDef($1->id); def->offset = $1->offset; delete $1; $$ = def; }
       | Var '=' InitVal { DeclareDef *def = new DeclareDef($1->id,$3); def->offset = $1->offset; delete $1; $$ = def; }

ConstDef : Var '=' InitVal { DeclareDef *def = new DeclareDef($1->id,$3); def->offset = $1->offset; delete $1; $$ = def; }

Stmt : MS { $$ = $1; }
     | UMS { $$ = $1; }

IfExpr : TIf '(' Expr ')' { $$ = $3; }

UMS : IfExpr Stmt { JumpStmt *ast = new JumpStmt($1,$2); $$ = ast; }
    | IfExpr MS TElse UMS { JumpStmt *ast = new JumpStmt($1,$2,$4); $$ = ast; }

MS : IfExpr MS TElse MS { JumpStmt *ast = new JumpStmt($1,$2,$4); $$ = ast; }
   | TReturn ';' { Stmt* ast = new Stmt(NULL,Return); $$ = ast; }
   | TReturn Expr ';' { Stmt* ast = new Stmt($2,Return); $$ = ast; }
   | Var '=' Expr ';' { Stmt *ast = new Stmt($3,Assign,(Var*)$1); $$ = ast; }
   | Block { $$ = $1; }
   | Expr ';' { Stmt *ast = new Stmt($1,Other); $$ = ast; }
   | ';' { $$ = NULL; }
   | TWhile '(' Expr ')' Stmt { WhileStmt *stmt = new WhileStmt($3,$5); $$ = stmt; }
   | TBreak ';' { Stmt *stmt = new Stmt(NULL,Break,NULL); $$ = stmt; }
   | TContinue ';' { Stmt *stmt = new Stmt(NULL,Continue,NULL); $$ = stmt; }

InitVals : InitVals ',' InitVal { (static_cast<InitVal*>$1->inits).push_back($3); $$ = $1; }
         | InitVal { InitVal *init = new InitVal(); (init->inits).push_back($1); $$ = init; }

InitVal : Expr { $$ = $1; }
        | '{' '}' { $$ = NULL; }
        | '{' InitVals '}' { $$ = $2; }

Var : TIdent { Var *var = new Var(*($1)); delete $1; $$ = var; }
    | Var '[' Expr ']' { $1->offset.push_back($3); $$ = $1; }

//这样设计以确保最低优先级
Expr : AndExpr { $$ = $1; }
     | Expr TLogicOr AndExpr { $$ = new Expr($3->is_boolean() ? $3 : new Expr($3, NULL, NotEqZero), $1->is_boolean() ? $1 : new Expr($1, NULL, NotEqZero), Or ); }

AndExpr : EqualExpr { $$ = $1; }
        | AndExpr TLogicAnd EqualExpr { $$ = new Expr($3->is_boolean() ? $3 : new Expr($3, NULL, NotEqZero), $1->is_boolean() ? $1 : new Expr($1, NULL, NotEqZero), And ); }

EqualExpr : CompareExpr { $$ = $1; }
          | EqualExpr EqualOp CompareExpr { $$ = new Expr($3, $1, $2); }

CompareExpr : AddSubExpr { $$ = $1; }
            | CompareExpr CompareOp AddSubExpr { $$ = new Expr($3, $1, $2); }

AddSubExpr : MulDivExpr { $$ = $1; }
           | AddSubExpr AddSubOp MulDivExpr { $$ = new Expr($3, $1, $2); }

MulDivExpr : UnaryExpr { $$ = $1; }
           | MulDivExpr MulDivOp UnaryExpr { $$ = new Expr($3, $1, $2); }

UnaryExpr : AtomExpr { $$ = $1; }
          | UnaryOp UnaryExpr { $$ = $1 == NoOp ? $2 : new Expr($2, NULL, $1); }

AtomExpr : '(' Expr ')' { $$ = $2; }
         | Number { $$ = $1; }
         | Var { $$ = $1; }
         | TIdent '(' FuncRealParams ')' { ((FuncCall*)$3)->name = *($1); delete $1; $$ = $3; }
         | TIdent '(' ')' { FuncCall *call = new FuncCall(); call->name = *($1); delete $1; $$ = call; }

UnaryOp : '+' { $$ = NoOp; }
        | '-' { $$ = Invert; }
        | '!' { $$ = EqZero; }

AddSubOp : '+' { $$ = Add; }
         | '-' { $$ = Sub; }

MulDivOp : '*' { $$ = Mul; }
         | '/' { $$ = Div; }
         | '%' { $$ = Mod; }

CompareOp : '<' { $$ = Less; }
          | '>' { $$ = Greater; }
          | '<' '=' { $$ = LEq; }
          | '>' '=' { $$ = GEq; }

EqualOp : '=' '=' { $$ = Equal; }
        | '!' '=' { $$ = NotEqual; }

Number : TIntCst { Number* ast = new Number(); ast->num = $1; $$ = ast; }

%%
extern void yyerror(Program **program,const char* s)
{
    cout<<"error: "<<s<<endl;
}
