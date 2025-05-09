%code requires {
    #include <memory>
    #include <string>
    #include "AST.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "AST.hpp"

int yylex();
void yyerror(std::unique_ptr<BaseAST>& ast,const char* s);

using namespace std;

%}

%parse-param {std::unique_ptr<BaseAST>& ast}

%union {
    int int_val;
    std::string* str_val;
    BaseAST* ast_val;
    vector<std::unique_ptr<BaseAST>>* vec_val;
}

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT RELOP EQOP ANDOP OROP
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block BlockItem Stmt Exp UnaryExp PrimaryExp Number MulExp AddExp RelExp 
                EqExp LAndExp LOrExp Decl ConstDecl ConstDef ConstInitVal LVal ConstExp VarDecl VarDef Initval
                OpenStmt MatchedStmt CompUnitList FuncFParam FuncFParams FuncRParams

%type <str_val> UnaryOp MulOp AddOp

%type <vec_val> ConstDefList BlockItemList VarDefList ConstExpList ExpList ConstInitValList InitvalList

%%

CompUnit
    : CompUnitList
    {
        auto comp_unit=unique_ptr<BaseAST>($1);
        ast=move(comp_unit);
    }
    ;

CompUnitList
    : FuncDef
    {
        auto comp_unit=new CompUnitAST();
        auto func_def=unique_ptr<BaseAST>($1);
        comp_unit->func_defs.push_back(move(func_def));
        $$=comp_unit;
    }
    | Decl
    {
        auto comp_unit=new CompUnitAST();
        auto decl=unique_ptr<BaseAST>($1);
        comp_unit->decls.push_back(move(decl));
        $$=comp_unit;
    }
    | CompUnitList FuncDef
    {
        auto comp_unit=(CompUnitAST*)$1;
        auto func_def=unique_ptr<BaseAST>($2);
        comp_unit->func_defs.push_back(move(func_def));
        $$=comp_unit;
    }
    | CompUnitList Decl
    {
        auto comp_unit=(CompUnitAST*)$1;
        auto decl=unique_ptr<BaseAST>($2);
        comp_unit->decls.push_back(move(decl));
        $$=comp_unit;
    }
    ;

FuncDef
    : FuncType IDENT '(' ')' Block
    {
        auto ast=new FuncDefAST();
        ast->has_params=false;
        ast->functype=unique_ptr<BaseAST>($1);
        ast->ident=*unique_ptr<string>($2);
        ast->block=unique_ptr<BaseAST>($5);
        $$=ast;
    }
    | FuncType IDENT '(' FuncFParams ')' Block
    {
        auto ast=new FuncDefAST();
        ast->has_params=true;
        ast->functype=unique_ptr<BaseAST>($1);
        ast->ident=*unique_ptr<string>($2);
        ast->funcfparams=unique_ptr<BaseAST>($4);
        ast->block=unique_ptr<BaseAST>($6);
        $$=ast;
    }
    ;

FuncType
    : INT
    {
        auto ast=new FuncTypeAST();
        ast->type="int";
        $$=ast;
    }
    | VOID
    {
        auto ast=new FuncTypeAST();
        ast->type="void";
        $$=ast;
    }
    ;

FuncFParam
    : FuncType IDENT
    {
        auto ast=new FuncFParamAST();
        ast->kind=FuncFParamAST::Kind::Int;
        auto func_type=unique_ptr<BaseAST>($1);
        auto p=(FuncTypeAST*)func_type.get();
        ast->btype=p->type;
        ast->ident=*unique_ptr<string>($2);
        $$=ast;
    }
    | FuncType IDENT '[' ']'
    {
        auto ast=new FuncFParamAST();
        ast->kind=FuncFParamAST::Kind::Array;
        auto func_type=unique_ptr<BaseAST>($1);
        auto p=(FuncTypeAST*)func_type.get();
        ast->btype=p->type;
        ast->ident=*unique_ptr<string>($2);
        $$=ast;
    }
    | FuncType IDENT '[' ']' ConstExpList
    {
        auto ast=new FuncFParamAST();
        ast->kind=FuncFParamAST::Kind::Array;
        auto func_type=unique_ptr<BaseAST>($1);
        auto p=(FuncTypeAST*)func_type.get();
        ast->btype=p->type;
        ast->ident=*unique_ptr<string>($2);
        vector<unique_ptr<BaseAST>>* vec=$5;
        ast->constexps=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

FuncFParams
    : FuncFParam
    {
        auto ast=new FuncFParamsAST();
        auto func_fparam=unique_ptr<BaseAST>($1);
        ast->FuncFParams.push_back(move(func_fparam));
        $$=ast;
    }
    | FuncFParams ',' FuncFParam
    {
        auto ast=(FuncFParamsAST*)$1;
        auto func_fparam=unique_ptr<BaseAST>($3);
        ast->FuncFParams.push_back(move(func_fparam));
        $$=ast;
    }
    ;

FuncRParams
    : Exp
    {
        auto ast=new FuncRParamsAST();
        auto exp=unique_ptr<BaseAST>($1);
        ast->exps.push_back(move(exp));
        $$=ast;
    }
    | FuncRParams ',' Exp
    {
        auto ast=(FuncRParamsAST*)$1;
        auto exp=unique_ptr<BaseAST>($3);
        ast->exps.push_back(move(exp));
        $$=ast;
    }

Block
    : '{' BlockItemList '}'
    {
        auto ast=new BlockAST();
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->blockitem=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

BlockItemList
    : 
    {
        auto vec=new vector<unique_ptr<BaseAST>>();
        
        $$=vec;
    }
    | BlockItemList BlockItem
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($2));
        $$=vec;
    }
    ;

BlockItem
    : Decl
    {
        auto ast=new BlockItemAST();
        ast->kind=BlockItemAST::Kind::Decl;
        ast->decl=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | Stmt
    {
        auto ast=new BlockItemAST();
        ast->kind=BlockItemAST::Kind::Stmt;
        ast->stmt=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

Stmt
    : OpenStmt
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Open;
        ast->open_stmt=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | MatchedStmt
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Matched;
        ast->matched_stmt=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

OpenStmt
    : IF '(' Exp ')' Stmt
    {
        auto ast=new OpenStmtAST();
        ast->kind=OpenStmtAST::Kind::If;
        ast->exp=unique_ptr<BaseAST>($3);
        ast->if_stmt=unique_ptr<BaseAST>($5);
        $$=ast;
    }
    | IF '(' Exp ')' MatchedStmt ELSE OpenStmt
    {
        auto ast=new OpenStmtAST();
        ast->kind=OpenStmtAST::Kind::IfElse;
        ast->exp=unique_ptr<BaseAST>($3);
        ast->if_stmt=unique_ptr<BaseAST>($5);
        ast->else_stmt=unique_ptr<BaseAST>($7);
        $$=ast;
    }
    | WHILE '(' Exp ')' OpenStmt
    {
        auto ast=new OpenStmtAST();
        ast->kind=OpenStmtAST::Kind::While;
        ast->exp=unique_ptr<BaseAST>($3);
        ast->while_stmt=unique_ptr<BaseAST>($5);
        $$=ast;
    }
    ;

MatchedStmt
    : RETURN Exp ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Return;
        ast->ret="ret";
        ast->exp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    | LVal '=' Exp ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Assign;
        ast->lval=unique_ptr<BaseAST>($1);
        ast->exp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    | RETURN ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Return;
        ast->ret="ret";
        ast->exp=nullptr;
        $$=ast;
    }
    | Exp ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Exp;
        ast->exp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Exp;
        ast->exp=nullptr;
        $$=ast;
    }
    | Block
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Block;
        ast->block=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::IfElse;
        ast->exp=unique_ptr<BaseAST>($3);
        ast->if_stmt=unique_ptr<BaseAST>($5);
        ast->else_stmt=unique_ptr<BaseAST>($7);
        $$=ast;
    }
    | WHILE '(' Exp ')' MatchedStmt
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::While;
        ast->exp=unique_ptr<BaseAST>($3);
        ast->while_stmt=unique_ptr<BaseAST>($5);
        $$=ast;
    }
    | BREAK ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Break;
        $$=ast;
    }
    | CONTINUE ';'
    {
        auto ast=new MatchedStmtAST();
        ast->kind=MatchedStmtAST::Kind::Continue;
        $$=ast;
    }
    ;

Exp
    : LOrExp
    {
        auto ast=new ExpAST();
        ast->orexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

UnaryExp
    : PrimaryExp
    {
        auto ast=new UnaryExpAST();
        ast->kind=UnaryExpAST::Kind::Primary;
        ast->primaryexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | UnaryOp UnaryExp
    {
        auto ast=new UnaryExpAST();
        ast->kind=UnaryExpAST::Kind::Unary;
        ast->unaryop=*unique_ptr<string>($1);
        ast->unaryexp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    | IDENT '(' FuncRParams ')'
    {
        auto ast=new UnaryExpAST();
        ast->has_params=true;
        ast->kind=UnaryExpAST::Kind::FunCall;
        ast->fun_name=*unique_ptr<string>($1);
        ast->funcrparams=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    | IDENT '(' ')'
    {
        auto ast=new UnaryExpAST();
        ast->has_params=false;
        ast->kind=UnaryExpAST::Kind::FunCall;
        ast->fun_name=*unique_ptr<string>($1);
        $$=ast;
    }
    ;

PrimaryExp
    : '(' Exp ')'
    {
        auto ast=new PrimaryExpAST();
        ast->kind=PrimaryExpAST::Kind::Exp;
        ast->exp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    | Number
    {
        auto ast=new PrimaryExpAST();
        ast->kind=PrimaryExpAST::Kind::Number;
        ast->number=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | LVal
    {
        auto ast=new PrimaryExpAST();
        ast->kind=PrimaryExpAST::Kind::LVal;
        ast->lval=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

UnaryOp
    : '+'
    {   
        string* str=new string("+");
        $$=str;
    }
    | '-'
    {
        string* str=new string("-");
        $$=str;
    }
    | '!'
    {
        string* str=new string("!");
        $$=str;
    }
    ;

MulOp
    : '*'
    {
        string* str=new string("*");
        $$=str;
    }
    | '/'
    {
        string* str=new string("/");
        $$=str;
    }
    | '%'
    {
        string* str=new string("%");
        $$=str;
    }
    ;

AddOp
    : '+'
    {
        string* str=new string("+");
        $$=str;
    }
    | '-'
    {
        string* str=new string("-");
        $$=str;
    }
    ;

Number
    : INT_CONST
    {
        auto ast=new NumberAST();
        ast->int_const=$1;
        $$=ast;
    }
    ;

MulExp
    : UnaryExp
    {
        auto ast=new MulExpAST();
        ast->kind=MulExpAST::Kind::Unary;
        ast->unaryexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | MulExp MulOp UnaryExp
    {
        auto ast=new MulExpAST();
        ast->kind=MulExpAST::Kind::Mult;
        ast->mulexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->unaryexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
AddExp
    :MulExp
    {
        auto ast=new AddExpAST();
        ast->kind=AddExpAST::Kind::Mul;
        ast->mulexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | AddExp AddOp MulExp
    {
        auto ast=new AddExpAST();
        ast->kind=AddExpAST::Kind::Add;
        ast->addexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->mulexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }

RelExp
    :AddExp
    {
        auto ast=new RelExpAST();
        ast->kind=RelExpAST::Kind::Add;
        ast->addexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | RelExp RELOP AddExp
    {
        auto ast=new RelExpAST();
        ast->kind=RelExpAST::Kind::Rel;
        ast->relexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->addexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

EqExp
    : RelExp
    {
        auto ast=new EqExpAST();
        ast->kind=EqExpAST::Kind::Rel;
        ast->relexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | EqExp EQOP RelExp
    {
        auto ast=new EqExpAST();
        ast->kind=EqExpAST::Kind::Eq;
        ast->eqexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->relexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

LAndExp
    : EqExp
    {
        auto ast=new LAndExpAST();
        ast->kind=LAndExpAST::Kind::Eq;
        ast->eqexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | LAndExp ANDOP EqExp
    {
        auto ast=new LAndExpAST();
        ast->kind=LAndExpAST::Kind::And;
        ast->landexp=unique_ptr<BaseAST>($1);
        ast->eqexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

LOrExp
    : LAndExp
    {
        auto ast=new LOrExpAST();
        ast->kind=LOrExpAST::Kind::And;
        ast->landexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | LOrExp OROP LAndExp
    {
        auto ast=new LOrExpAST();
        ast->kind=LOrExpAST::Kind::Or;
        ast->lorexp=unique_ptr<BaseAST>($1);
        ast->landexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

Decl
    : ConstDecl
    {
        auto ast=new DeclAST();
        ast->kind=DeclAST::Kind::Const;
        ast->constdecl=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | VarDecl
    {
        auto ast=new DeclAST();
        ast->kind=DeclAST::Kind::Var;
        ast->vardecl=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

ConstDefList
    : ConstDef
    {
        auto vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($1));
        $$=vec;
    }
    | ConstDefList ',' ConstDef
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

ConstDecl
    : CONST FuncType ConstDefList ';'
    {
        auto ast=new ConstDeclAST();
        auto func_type=unique_ptr<BaseAST>($2);
        auto p=(FuncTypeAST*)func_type.get();
        ast->btype=p->type;
        vector<std::unique_ptr<BaseAST>>* vec=$3;
        ast->constdef=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

ConstDef
    : IDENT '=' ConstInitVal
    {
        auto ast=new ConstDefAST();
        ast->ident=*unique_ptr<string>($1);
        ast->kind=ConstDefAST::Kind::Int;
        ast->constinitval=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    | IDENT ConstExpList '=' ConstInitVal
    {
        auto ast=new ConstDefAST();
        ast->ident=*unique_ptr<string>($1);
        ast->kind=ConstDefAST::Kind::Array;
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->constexps=move(*vec);
        delete vec;
        ast->constinitval=unique_ptr<BaseAST>($4);
        $$=ast;
    }
    ;

ConstInitVal
    : ConstExp
    {
        auto ast=new ConstInitValAST();
        ast->kind=ConstInitValAST::Kind::Int;
        ast->constexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | '{' '}'
    {
        auto ast=new ConstInitValAST();
        ast->kind=ConstInitValAST::Kind::EmptyArray;
        $$=ast;
    }
    | '{' ConstInitValList '}'
    {
        auto ast=new ConstInitValAST();
        ast->kind=ConstInitValAST::Kind::Array;
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->constInitVals=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

ConstExp
    : Exp
    {
        auto ast=new ConstExpAST();
        ast->exp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

LVal
    : IDENT
    {
        auto ast=new LValAST();
        ast->kind=LValAST::Kind::Int;
        ast->ident=*unique_ptr<string>($1);
        $$=ast;   
    }
    | IDENT ExpList
    {
        auto ast=new LValAST();
        ast->kind=LValAST::Kind::Array;
        ast->ident=*unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->exps=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

VarDefList
    : VarDef
    {
        auto vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($1));
        $$=vec;
    }
    | VarDefList ',' VarDef
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

VarDef
    : IDENT
    {
        auto ast=new VarDefAST();
        ast->kind=VarDefAST::Kind::Int_Ident;
        ast->ident=*unique_ptr<string>($1);
        $$=ast;
    }
    | IDENT '=' Initval
    {
        auto ast=new VarDefAST();
        ast->kind=VarDefAST::Kind::Int_Init;
        ast->ident=*unique_ptr<string>($1);
        ast->initval=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    | IDENT ConstExpList
    {
        auto ast=new VarDefAST();
        ast->kind=VarDefAST::Kind::Array_Ident;
        ast->ident=*unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->constexps=move(*vec);
        delete vec;
        $$=ast;
    }
    | IDENT ConstExpList '=' Initval
    {
        auto ast=new VarDefAST();
        ast->kind=VarDefAST::Kind::Array_Init;
        ast->ident=*unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->constexps=move(*vec);
        delete vec;
        ast->initval=unique_ptr<BaseAST>($4);
        $$=ast;
    }
    ;

Initval
    : Exp
    {
        auto ast=new InitValAST();
        ast->kind=InitValAST::Kind::Exp;
        ast->exp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | '{' '}'
    {
        auto ast=new InitValAST();
        ast->kind=InitValAST::Kind::EmptyArray;
        $$=ast;
    }
    | '{' InitvalList '}'
    {
        auto ast=new InitValAST();
        ast->kind=InitValAST::Kind::Array;
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->initvals=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

VarDecl
    : FuncType VarDefList ';'
    {
        auto ast=new VarDeclAST();
        auto func_type=unique_ptr<BaseAST>($1);
        auto p=(FuncTypeAST*)func_type.get();
        ast->btype=p->type;
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->vardef=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

ConstExpList
    : '[' ConstExp ']'
    {
        vector<unique_ptr<BaseAST>>* vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($2));
        $$=vec;
    }
    | ConstExpList '[' ConstExp ']'
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

ExpList
    : '[' Exp ']'
    {
        vector<unique_ptr<BaseAST>>* vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($2));
        $$=vec;
    }
    | ExpList '[' Exp ']'
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

ConstInitValList
    : ConstInitVal
    {
        vector<unique_ptr<BaseAST>>* vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($1));
        $$=vec;
    }
    | ConstInitValList ',' ConstInitVal
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }

InitvalList
    : Initval
    {
        vector<unique_ptr<BaseAST>>* vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($1));
        $$=vec;
    }
    | InitvalList ',' Initval
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

%%

void yyerror(unique_ptr<BaseAST>& ast,const char* s){
    extern int yylineno;
    extern char* yytext;
    cerr<<"error: "<<s<<" at symbol '"<<yytext<<"' on line "<<yylineno<<endl;
};