
// for .hpp
%code requires{
    #include <memory>
    #include <string>
    #include "ast.hpp"
    #include "ast_exp.hpp"
    #include "ast_var.hpp"

}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.hpp"
#include "ast_exp.hpp"
#include "ast_var.hpp"

// for lexer
int yylex();

void yyerror(past_t &ast, const char *s);

using namespace std;

bool is_reserved_func_name(const std::string &name) {
    return name == "main" || 
           name == "getint" ||
           name == "getch" ||
           name == "getarray" ||
           name == "putint" ||
           name == "putch" ||
           name == "putarray" ||
           name == "starttime" ||
           name == "stoptime";
}   

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
    std::string *str_val;
    int int_val;
    BaseAST *ast_val;
    char char_val;
    std::vector<std::unique_ptr<BaseAST> > *vec_val;
    Variable *var_val;
    std::vector<std::unique_ptr<Variable> > *var_list_val;
    LVal *lval;
}

// terminal symbols
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT 
%token <char_val> EQUAL GEQUAL LEQUAL NEQUAL AND OR NOT ADD SUB DIV MUL MOD GREATER LESS
%token <int_val> INT_CONST

// non-terminal symbols
%type <ast_val> FuncDef Block Stmt CloseStmt OpenStmt CompUnitListItem
%type <ast_val> Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp ConstExp 
%type <ast_val> Decl ConstDecl VarDecl ConstDef VarDef ConstInitVal InitVal BlockItem
%type <char_val> UnaryOp MulOp AddOp RelOp EqOp LAndOp LOrOp
%type <int_val> Number
%type <str_val> Type
%type <vec_val> BlockItemList ConstDefList VarDefList CompUnitList FuncRParams FuncRParamList ConstInitValList InitValList ArrayConstExpList ArrayExpList
%type <var_val> FuncFParam
%type <var_list_val> FuncFParams FuncFParamList 
%type <lval> LVal

%%

CompUnit
    : CompUnitList { 
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->comp_unit_list = unique_ptr<vector<past_t> >($1);
        ast = move(comp_unit); 
    }
    ;

CompUnitList
    : CompUnitList CompUnitListItem {
        auto vec = $1;
        vec->push_back(past_t($2));
        $$ = vec;
    }
    | CompUnitListItem {
        auto vec = new vector<unique_ptr<BaseAST> >();
        vec->push_back(past_t($1));
        $$ = vec;
    }
    ;

CompUnitListItem
    : Decl {
        $$ = $1;
    }
    | FuncDef {
        $$ = $1;
    }
    ;

FuncDef
    : Type IDENT '(' ')' Block { 
        auto ast = new FuncDefAST();
        ast->func_type = *ps_t($1);
        ast->ident = *ps_t($2);
        if (!is_reserved_func_name(ast->ident)) {
            ast->ident = "__" + ast->ident;
        }
        ast->block = past_t($5);
        ast->func_param_list = unique_ptr<vector<pvar_t> >(new vector<pvar_t>());
        $$ = ast;
    }
    | Type IDENT '(' FuncFParams ')' Block { 
        auto ast = new FuncDefAST();
        ast->func_type = *ps_t($1);
        ast->ident = *ps_t($2);
        if (!is_reserved_func_name(ast->ident)) {
            ast->ident = "__" + ast->ident;
        }
        ast->block = past_t($6);
        ast->func_param_list = unique_ptr<vector<pvar_t> >($4);
        $$ = ast;
    }
    ;

FuncFParams 
    : FuncFParamList {
        $$ = $1;
    }
    ;


FuncFParamList
    : FuncFParamList ',' FuncFParam {
        auto vec = $1;
        vec->push_back(pvar_t($3));
        $$ = vec;
    }
    | FuncFParam {
        auto vec = new vector<unique_ptr<Variable> >();
        vec->push_back(pvar_t($1));
        $$ = vec;
    }
    ;

FuncFParam
    : Type IDENT {
        auto var = new Variable();
        var->type = *ps_t($1);
        var->ident = "__" + *ps_t($2);
        $$ = var;
    }
    | Type IDENT '[' ']' {
        auto var = new Variable();
        var->type = "*" + *ps_t($1);
        var->ident = "__" + *ps_t($2);
        var->arr_exp_list = unique_ptr<vector<past_t> >(new vector<past_t>());
        $$ = var;
    }
    | Type IDENT '[' ']' ArrayConstExpList {
        auto var = new Variable();
        var->type = "*" + *ps_t($1);
        var->ident = "__" + *ps_t($2);
        var->arr_exp_list = unique_ptr<vector<past_t> >($5);
        $$ = var;
    }
    ;

Block
    : '{' BlockItemList '}' {
        auto ast = new BlockAST();
        ast->block_item_list = unique_ptr<vector<past_t> >($2);
        $$ = ast;
    }
    ;

BlockItemList
    : /* empty */ {
        auto vec = new vector<past_t>();
        $$ = vec;
    }
    | BlockItemList BlockItem {
        auto vec = $1;
        vec->push_back(past_t($2));
        $$ = vec;
    }
    ;

BlockItem 
    : Decl {
        auto ast = new BlockItemAST();
        ast->rule = 1;
        ast->decl = past_t($1);
        $$ = ast;
    }
    | Stmt {
        auto ast = new BlockItemAST();
        ast->rule = 2;
        ast->stmt = past_t($1);
        $$ = ast;
    }
    ;

Stmt
    : OpenStmt {
        $$ = $1;
    }
    | CloseStmt {
        $$ = $1;
    }
    ;


CloseStmt
    : LVal '=' Exp ';' {
        auto ast = new StmtAST();
        ast->rule = 1;
        ast->l_val = $1;
        ast->exp = past_t($3);
        $$ = ast;
    }
    | Exp ';' {
        auto ast = new StmtAST();
        ast->rule = 201;
        ast->exp = past_t($1);
    }
    | ';' {
        auto ast = new StmtAST();
        ast->rule = 202;
        $$ = ast;
    }
    | Block {
        auto ast = new StmtAST();
        ast->rule = 3;
        ast->block = past_t($1);
        $$ = ast;
    }
    | RETURN Exp ';' {
        auto ast = new StmtAST();
        ast->rule = 401;
        ast->exp = past_t($2);
        $$ = ast;
    }
    | RETURN ';' {
        auto ast = new StmtAST();
        ast->rule = 402;
        $$ = ast;
    }
    | IF '(' Exp ')' CloseStmt ELSE CloseStmt {
        auto ast = new StmtAST();
        ast->rule = 502;
        ast->exp = past_t($3);
        ast->then_stmt = past_t($5);
        ast->else_stmt = past_t($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' CloseStmt {
        auto ast = new StmtAST();
        ast->rule = 6;
        ast->exp = past_t($3);
        ast->then_stmt = past_t($5);
        $$ = ast;
    }
    | BREAK ';' {
        auto ast = new StmtAST();
        ast->rule = 7;
        $$ = ast;
    }
    | CONTINUE ';' {
        auto ast = new StmtAST();
        ast->rule = 8;
        $$ = ast;
    }
    ;


OpenStmt
    : IF '(' Exp ')' Stmt {
        auto ast = new StmtAST();
        ast->rule = 501;
        ast->exp = past_t($3);
        ast->then_stmt = past_t($5);
        $$ = ast;
    }
    | IF '(' Exp ')' CloseStmt ELSE OpenStmt {
        auto ast = new StmtAST();
        ast->rule = 502;
        ast->exp = past_t($3);
        ast->then_stmt = past_t($5);
        ast->else_stmt = past_t($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' OpenStmt {
        auto ast = new StmtAST();
        ast->rule = 6;
        ast->exp = past_t($3);
        ast->then_stmt = past_t($5);
        $$ = ast;
    }
    ;

Decl 
    : ConstDecl {
        auto ast = new DeclAST();
        ast->rule = 1;
        ast->const_decl = past_t($1);
        $$ = ast;
    }
    | VarDecl {
        auto ast = new DeclAST();
        ast->rule = 2;
        ast->var_decl = past_t($1);
        $$ = ast;
    }
    ;

ConstDecl 
    : CONST Type ConstDefList ';' {
        auto ast = new ConstDeclAST();
        ast->b_type = *ps_t($2);
        ast->const_def_list = unique_ptr<vector<past_t> >($3);
        $$ = ast;
    }
    ;

VarDecl
    : Type VarDefList ';' {
        auto ast = new VarDeclAST();
        ast->b_type = *ps_t($1);
        ast->var_def_list = unique_ptr<vector<past_t> >($2);
        $$ = ast;
    }
    ;

ConstDefList
    : ConstDef {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($1));
        $$ = vec;
    }
    | ConstDefList ',' ConstDef {
        auto vec = $1;
        vec->push_back(past_t($3));
        $$ = vec;
    }
    ;

VarDefList 
    : VarDef {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($1));
        $$ = vec;
    }
    | VarDefList ',' VarDef {
        auto vec = $1;
        vec->push_back(past_t($3));
        $$ = vec;
    }
    ;

ConstDef 
    : IDENT '=' ConstInitVal {
        auto ast = new ConstDefAST();
        ast->ident = "__" + *ps_t($1);
        ast->const_init_val = past_t($3);
        ast->rule = 1;
        $$ = ast;
    }
    | IDENT ArrayConstExpList '=' ConstInitVal {
        auto ast = new ConstDefAST();
        ast->ident = "__" + *ps_t($1);
        ast->arr_exp_list = unique_ptr<vector<past_t> >($2);
        ast->const_init_val = past_t($4);
        ast->rule = 2;
        $$ = ast;
    }
    ;

VarDef 
    : IDENT {
        auto ast = new VarDefAST();
        ast->rule = 101;
        ast->ident = "__" + *ps_t($1);
        $$ = ast;
    }
    | IDENT '=' InitVal {
        auto ast = new VarDefAST();
        ast->rule = 102;
        ast->ident = "__" + *ps_t($1);
        ast->init_val = past_t($3);
        $$ = ast;
    }
    | IDENT ArrayConstExpList {
        auto ast = new VarDefAST();
        ast->rule = 201;
        ast->ident = "__" + *ps_t($1);
        ast->arr_exp_list = unique_ptr<vector<past_t> >($2);
        $$ = ast;
    }
    | IDENT ArrayConstExpList '=' InitVal {
        auto ast = new VarDefAST();
        ast->rule = 202;
        ast->ident = "__" + *ps_t($1);
        ast->arr_exp_list = unique_ptr<vector<past_t> >($2);
        ast->init_val = past_t($4);
        $$ = ast;
    }
    ;

ArrayConstExpList
    : '[' ConstExp ']' ArrayConstExpList {
        auto vec = $4;
        vec->push_back(past_t($2));
        $$ = vec;
    }
    | '[' ConstExp ']' {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($2));
        $$ = vec;
    }
    ;

ConstInitVal
    : ConstExp {
        auto ast = new ConstInitValAST();
        ast->const_exp = past_t($1);
        ast->rule = 1;
        $$ = ast;
    }
    | '{' ConstInitValList '}' {
        auto ast = new ConstInitValAST();
        ast->const_init_val_list = unique_ptr<vector<past_t> >($2);
        ast->rule = 2;
        $$ = ast;
    }
    | '{' '}' {
        auto ast = new ConstInitValAST();
        ast->const_init_val_list = unique_ptr<vector<past_t> >(new vector<past_t>());
        ast->rule = 2;
        $$ = ast;
    }
    ;

ConstInitValList
    : ConstInitValList ',' ConstInitVal {
        auto vec = $1;
        vec->push_back(past_t($3));
        $$ = vec;
    }
    | ConstInitVal {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($1));
        $$ = vec;
    }
    ;

InitVal
    : Exp {
        auto ast = new InitValAST();
        ast->exp = past_t($1);
        ast->rule = 1;
        $$ = ast;
    }
    | '{' InitValList '}' {
        auto ast = new InitValAST();
        ast->init_val_list = unique_ptr<vector<past_t> >($2);
        ast->rule = 2;
        $$ = ast;
    }
    | '{' '}' {
        auto ast = new InitValAST();
        ast->init_val_list = unique_ptr<vector<past_t> >(new vector<past_t>());
        ast->rule = 2;
        $$ = ast;
    }
    ;

InitValList
    : InitValList ',' InitVal {
        auto vec = $1;
        vec->push_back(past_t($3));
        $$ = vec;
    }
    | InitVal {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($1));
        $$ = vec;
    }

/*
 *    <--  Exp  -->
 */

ConstExp
    : Exp {
        auto ast = new ConstExpAST();
        ast->exp = past_t($1);
        $$ = ast;
    }
    ;

Exp 
    : LOrExp {
        auto ast = new ExpAST();
        ast->l_or_exp = past_t($1);
        $$ = ast;
    }
    ;


RelExp 
    : AddExp {
        auto ast = new RelExpAST();
        ast->rule = 1;
        ast->add_exp = past_t($1);
        $$ = ast;
    }
    | RelExp RelOp AddExp {
        auto ast = new RelExpAST();
        ast->rule = 2;
        ast->rel_exp = past_t($1);
        ast->rel_op = $2;
        ast->add_exp = past_t($3);
        $$ = ast;
    }
    ;

EqExp 
    : RelExp {
        auto ast = new EqExpAST();
        ast->rule = 1;
        ast->rel_exp = past_t($1);
        $$ = ast;
    }
    | EqExp EqOp RelExp {
        auto ast = new EqExpAST();
        ast->rule = 2;
        ast->eq_exp = past_t($1);
        ast->eq_op = $2;
        ast->rel_exp = past_t($3);
        $$ = ast;
    }
    ;

LAndExp 
    : EqExp {
        auto ast = new LAndExpAST();
        ast->rule = 1;
        ast->eq_exp = past_t($1);
        $$ = ast;
    }
    | LAndExp LAndOp EqExp {
        auto ast = new LAndExpAST();
        ast->rule = 2;
        ast->l_and_exp = past_t($1);
        ast->l_and_op = $2;
        ast->eq_exp = past_t($3);
        $$ = ast;
    }
    ;

LOrExp
    : LAndExp {
        auto ast = new LOrExpAST();
        ast->rule = 1;
        ast->l_and_exp = past_t($1);
        $$ = ast;
    }
    | LOrExp LOrOp LAndExp {
        auto ast = new LOrExpAST();
        ast->rule = 2;
        ast->l_or_exp = past_t($1);
        ast->l_or_op = $2;
        ast->l_and_exp = past_t($3);
        $$ = ast;
    }
    ;


AddExp
    : MulExp {
        auto ast = new AddExpAST();
        ast->rule = 1;
        ast->mul_exp = past_t($1);
        $$ = ast;
    }
    | AddExp AddOp MulExp {
        auto ast = new AddExpAST();
        ast->rule = 2;
        ast->add_exp = past_t($1);
        ast->add_op = $2;
        ast->mul_exp = past_t($3);
        $$ = ast;
    }
    ;

MulExp 
    : UnaryExp {
        auto ast = new MulExpAST();
        ast->rule = 1;
        ast->unary_exp = past_t($1);
        $$ = ast;
    }
    | MulExp MulOp UnaryExp {
        auto ast = new MulExpAST();
        ast->rule = 2;
        ast->mul_exp = past_t($1);
        ast->mul_op = $2;
        ast->unary_exp = past_t($3);
        $$ = ast;
    }
    ;

PrimaryExp 
    : '(' Exp ')' {
        auto ast = new PrimaryExpAST();
        ast->rule = 1;
        ast->exp = past_t($2);
        $$ = ast;
    }
    | LVal {
        auto ast = new PrimaryExpAST();
        ast->rule = 2;
        ast->l_val = $1;
        $$ = ast;
    }
    | Number {
        auto ast = new PrimaryExpAST();
        ast->rule = 3;
        ast->number = $1;
        $$ = ast;
    }
    ;



UnaryExp 
    : PrimaryExp {
        auto ast = new UnaryExpAST();
        ast->rule = 1;
        ast->primary_exp = past_t($1);
        $$ = ast;
    }
    | UnaryOp UnaryExp {
        auto ast = new UnaryExpAST();
        ast->rule = 2;
        ast->unary_op = $1;
        ast->unary_exp = past_t($2);
        $$ = ast;
    }
    | IDENT '(' ')' {
        auto ast = new UnaryExpAST();
        ast->rule = 3;
        ast->func_name = *ps_t($1);
        if (!is_reserved_func_name(ast->func_name)) {
            ast->func_name = "__" + ast->func_name;
        }
        ast->func_args = unique_ptr<vector<past_t> >(new vector<past_t>());
        $$ = ast;
    }
    | IDENT '(' FuncRParams ')' {
        auto ast = new UnaryExpAST();
        ast->rule = 3;
        ast->func_name = *ps_t($1);
        if (!is_reserved_func_name(ast->func_name)) {
            ast->func_name = "__" + ast->func_name;
        }
        ast->func_args = unique_ptr<vector<past_t> >($3);
        $$ = ast;
    }
    ;


FuncRParams 
    : FuncRParamList {
        $$ = $1;
    }
    ;

FuncRParamList
    : Exp {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($1));
        $$ = vec;
    }
    | FuncRParamList ',' Exp {
        auto vec = $1;
        vec->push_back(past_t($3));
        $$ = vec;
    }
    ;


UnaryOp 
    : ADD { $$ = $1; } 
    | SUB { $$ = $1; }
    | NOT { $$ = $1; }
    ;

AddOp
    : ADD { $$ = $1; }
    | SUB { $$ = $1; }
    ;

MulOp 
    : MUL { $$ = $1; }
    | DIV { $$ = $1; }
    | MOD { $$ = $1; }
    ;

RelOp
    : LESS      { $$ = $1; }
    | GREATER   { $$ = $1; }
    | LEQUAL    { $$ = $1; }
    | GEQUAL    { $$ = $1; }
    ;

EqOp
    : EQUAL     { $$ = $1; }
    | NEQUAL    { $$ = $1; }
    ;

LAndOp : AND { $$ = $1; } ;

LOrOp : OR { $$ = $1; } ;

Type 
    : INT {
        $$ = new string("int");
    }
    | VOID {
        $$ = new string("void");
    }
    ;

LVal 
    : IDENT {
        auto l_val = new LVal();
        l_val->rule = 1;
        l_val->ident = "__" + *ps_t($1);
        $$ = l_val;
    }
    | IDENT ArrayExpList {
        auto l_val = new LVal();
        l_val->rule = 2;
        l_val->ident = "__" + *ps_t($1);
        l_val->arr_exp_list = unique_ptr<vector<past_t> >($2);
        $$ = l_val;
    }
    ;

ArrayExpList
    : ArrayExpList '[' Exp ']' {
        auto vec = $1;
        vec->push_back(past_t($3));
        $$ = vec;
    }
    | '[' Exp ']' {
        auto vec = new vector<past_t>();
        vec->push_back(past_t($2));
        $$ = vec;
    }
    ;

Number
    : INT_CONST {
        $$ = $1;
    }
    ;

%%


void yyerror(past_t &ast, const char *s) {
    cout << "error: " << s << endl;
}