%code requires {
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "../head/ast.hpp"
using namespace std;
}

%{
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "../head/ast.hpp"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);
static std::vector<InstSet> env_stk;
static std::vector<BaseAST*> func_list;
static std::vector<BaseAST*> fparams;
static std::vector<std::vector<BaseAST*>> rparams;
static std::vector<BaseAST*> arr_size;
static std::vector<std::vector<BaseAST*>> idx_stk;
//arr_list->idx_stk


void add_inst(InstType instType, BaseAST *ast)
{
    env_stk[env_stk.size()-1].push_back(make_pair(instType, std::unique_ptr<BaseAST>(ast)));
}

using namespace std;
%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT UNARYOP MULOP ADDOP RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST
%type <str_val> FuncType BType
%type <ast_val> FuncDef Block Number LVal Exp IF_exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp InitVal

%%

CompUnit
    : {
    cout<<"CompUnit"<<endl;
    env_stk.push_back(InstSet());
    } _CompUnit {
        cout<<env_stk[env_stk.size()-1].size()<<endl;
        ast = std::unique_ptr<BaseAST>(new CompUnitAST(func_list, env_stk[env_stk.size()-1]));
        env_stk.pop_back();
    }
;


_CompUnit
    : FuncDef {
        cout<<"FuncDef"<<endl;
        func_list.push_back($1);
    }
    | _CompUnit FuncDef {
        cout<<" _CompUnit FuncDef "<<endl;
        func_list.push_back($2);
    } 
    | Decl {
        cout<<"Decl_to_"<<endl;
    }
    | _CompUnit Decl 
;
FuncDef
    : FuncType IDENT '('  {
            fparams.clear();
        } FuncFParams ')' Block {
             cout<<"FuncFparam"<<endl;
            auto ast = new FuncDefAST(fparams);
            ast->func_type = *unique_ptr<string> ($1);
            ast->ident = *unique_ptr<string> ($2);  
            ast->block = std::unique_ptr<BlockAST>(dynamic_cast<BlockAST*>(($7))); 
            $$ = ast;
    } 
    | FuncType IDENT '(' ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = *unique_ptr<string> ($1);
        ast->ident = *unique_ptr<string> ($2);  
        ast->block = std::unique_ptr<BlockAST>(dynamic_cast<BlockAST*>(($5))); 
        fparams.clear();
        $$ = ast;
    }
;

FuncFParams : FuncFParam | FuncFParams ',' FuncFParam;

FuncFParam
    : INT IDENT {
        fparams.push_back(new FuncFParamAST(FuncFParamAST::Int, $2->c_str(), fparams.size()));
    }
    | INT IDENT '[' ']' {
        fparams.push_back(new FuncFParamAST(FuncFParamAST::Array, $2->c_str(), fparams.size(), arr_size));
    }
    | INT IDENT '[' ']' ArraySizeList {
        fparams.push_back(new FuncFParamAST(FuncFParamAST::Array, $2->c_str(), fparams.size(), arr_size));
        arr_size.clear();
    };

FuncType:
    INT {
        cout<<"INT_F"<<endl;
        $$ = new string("int");
    }
    | VOID {
        $$ = new string("void");
    };
;

Block:
    '{' {
        cout<<endl;
        cout<<"begin block"<<endl;
        env_stk.push_back(InstSet());
    }
    BlockItems '}' {
        cout<<"end block"<<endl;
        cout<<endl;
        auto ast = new BlockAST(env_stk[env_stk.size()-1]);
        $$ = ast;
        env_stk.pop_back();
    }
    | '{' '}' {
        $$ = new BlockAST();
    };
;

BlockItems : BlockItem | BlockItem BlockItems ;

BlockItem : Decl | Stmt ;


Decl : ConstDecl | VarDecl;


ConstDecl : CONST BType ConstDefList ';';
ConstDefList : ConstDef | ConstDefList ',' ConstDef

BType :
    INT {
        cout<<"INT"<<endl;
        $$ = new string("int");
    }
;

ConstDef
    : IDENT '=' Exp {
        auto exp = std::unique_ptr<BaseAST>($3);
        add_inst(InstType::ConstDecl, new ConstDefAST($1->c_str(), exp));
    }
    | IDENT ArraySizeList '=' InitVal {
        auto initval = std::unique_ptr<BaseAST>($4);
        initval->is_const = true;
        add_inst(InstType::ArrayDecl, new ArrayDefAST($1->c_str(), arr_size, initval));
        arr_size.clear();
    }
    | IDENT ArraySizeList {
        add_inst(InstType::ArrayDecl, new ArrayDefAST($1->c_str(), arr_size));
        arr_size.clear();
    }
;


ArraySizeList : ArraySize | ArraySizeList ArraySize;

ArraySize 
    : '[' Exp ']' {
        arr_size.push_back($2);
    };


VarDecl : FuncType VarDefList ';';
VarDefList : VarDef | VarDefList ',' VarDef ;

VarDef
    : IDENT {
        cout<<"vardef"<<endl;
        add_inst(InstType::Decl, new VarDefAST($1->c_str()));
    }
    | IDENT '=' Exp {
        auto exp = std::unique_ptr<BaseAST>($3);
        add_inst(InstType::Decl, new VarDefAST($1->c_str(), exp));
    }
    | IDENT ArraySizeList '=' InitVal {
        auto initval = std::unique_ptr<BaseAST>($4);
        add_inst(InstType::ArrayDecl, new ArrayDefAST($1->c_str(), arr_size, initval));
        arr_size.clear();
    }
    | IDENT ArraySizeList {
        add_inst(InstType::ArrayDecl, new ArrayDefAST($1->c_str(), arr_size));
        arr_size.clear();
    }
;
InitVal : Exp {
        auto exp = std::unique_ptr<BaseAST>($1);
        $$ = new InitValAST(exp);
    }
    | '{' {
            idx_stk.push_back(std::vector<BaseAST*>());
        } ArrInitList '}' {
        $$ = new InitValAST(idx_stk[idx_stk.size()-1]);
        idx_stk.pop_back();
    }
    | '{' '}' {
        idx_stk.push_back(std::vector<BaseAST*>());
        $$ = new InitValAST(idx_stk[idx_stk.size()-1]);
        idx_stk.pop_back();
    };

ArrInitList : InitVal {
        idx_stk[idx_stk.size()-1].push_back($1);
    } 
    | ArrInitList ',' InitVal {
        idx_stk[idx_stk.size()-1].push_back($3);
    };



Stmt :  
    RETURN Exp ';' {
        cout<<"return exp success!"<<endl;
        auto number = unique_ptr<BaseAST> ($2);
        auto ast_1 = new ReturnAST(number);
        add_inst(InstType::Stmt, ast_1);
    }
    |RETURN ';' {
        cout<<"return success!"<<endl;
        add_inst(InstType::Stmt, new ReturnAST());
    }
    |LVal '=' Exp ';' {
        auto lval = std::unique_ptr<BaseAST>($1);
        auto exp = std::unique_ptr<BaseAST>($3);
        add_inst(InstType::Stmt, new AssignmentAST(lval, exp));
    }
    | ';' 
    | Exp ';' {
        add_inst(InstType::Stmt, $1);
    } 
    | Block {
        add_inst(InstType::Stmt, $1);
    }
    | IF_exp Stmt {
        auto exp = std::unique_ptr<BaseAST>($1);
        InstSet true_instset;
        for(auto &inst : env_stk[env_stk.size()-1])
            true_instset.push_back(std::make_pair(inst.first, std::move(inst.second)));
        env_stk.pop_back();
        add_inst(InstType::Branch, new BranchAST(exp, true_instset));
    }
    |IF_exp Stmt ELSE {
            env_stk.push_back(InstSet());
        } Stmt {
            cout<<"if_else_open"<<endl;
            auto exp = std::unique_ptr<BaseAST>($1);
            InstSet true_instset, false_instset;
            for(auto &inst : env_stk[env_stk.size()-2])
                true_instset.push_back(std::make_pair(inst.first, std::move(inst.second)));
            for(auto &inst : env_stk[env_stk.size()-1])
                false_instset.push_back(std::make_pair(inst.first, std::move(inst.second)));
            env_stk.pop_back();
            env_stk.pop_back();
            add_inst(InstType::Branch, new BranchAST(exp, true_instset, false_instset));
    }
    |WHILE '(' Exp ')' {
            env_stk.push_back(InstSet());
        } Stmt {
            auto exp = std::unique_ptr<BaseAST>($3);
            InstSet while_body;
            for(auto &inst : env_stk[env_stk.size()-1])
                while_body.push_back(std::make_pair(inst.first, std::move(inst.second)));
            env_stk.pop_back();
            add_inst(InstType::While, new WhileAST(exp, while_body));
    }
    | BREAK ';' {
        add_inst(InstType::Break, new BreakAST());
    } 
    | CONTINUE ';' {
        add_inst(InstType::Continue, new ContinueAST());
    }
;


IF_exp: IF '(' Exp ')'{
    cout<<"exp_success!"<<endl;
    env_stk.push_back(InstSet());
    $$ = $3;
}
;

Number:
    INT_CONST {
        cout<<"number"<<endl;
        auto ast = new NumberAST($1);
        ast->number = to_string($1);
        $$ = ast;
    }
;

Exp 
    : LOrExp {
        auto Lor_exp = std::unique_ptr<BaseAST>($1);
        $$ = new ExpAST(Lor_exp);
    }
;

LOrExp
    : LAndExp {
        auto land_exp = std::unique_ptr<BaseAST>($1);
        $$ = new LOrExpAST(land_exp);
    }
    | LOrExp LOROP LAndExp {
        auto left_exp = std::unique_ptr<BaseAST>($1);
        auto op = std::unique_ptr<std::string>($2);
        auto right_exp = std::unique_ptr<BaseAST>($3);
        $$ = new LOrExpAST(left_exp, op->c_str(), right_exp);
    }
;

LAndExp
    : EqExp {
        auto eq_exp = std::unique_ptr<BaseAST>($1);
        $$ = new LAndExpAST(eq_exp);
    }
    | LAndExp LANDOP EqExp {
        auto left_exp = std::unique_ptr<BaseAST>($1);
        auto op = std::unique_ptr<std::string>($2);
        auto right_exp = std::unique_ptr<BaseAST>($3);
        $$ = new LAndExpAST(left_exp, op->c_str(), right_exp);
    }
;

EqExp
    : RelExp {
        auto rel_exp = std::unique_ptr<BaseAST>($1);
        $$ = new EqExpAST(rel_exp);
    }
    | EqExp EQOP RelExp {
        auto left_exp = std::unique_ptr<BaseAST>($1);
        auto op = std::unique_ptr<std::string>($2);
        auto right_exp = std::unique_ptr<BaseAST>($3);
        $$ = new EqExpAST(left_exp, op->c_str(), right_exp);
    }
;

RelExp
    : AddExp {
        auto add_exp = std::unique_ptr<BaseAST>($1);
        $$ = new RelExpAST(add_exp);
    }
    | RelExp RELOP AddExp {
        auto left_exp = std::unique_ptr<BaseAST>($1);
        auto op = std::unique_ptr<std::string>($2);
        auto right_exp = std::unique_ptr<BaseAST>($3);
        $$ = new RelExpAST(left_exp, op->c_str(), right_exp);
    }
;

AddExp
    : MulExp {
        auto mul_exp = std::unique_ptr<BaseAST>($1);
        $$ = new MulExpAST(mul_exp);
    }
    | AddExp ADDOP MulExp {
        auto left_exp = std::unique_ptr<BaseAST>($1);
        auto op = std::unique_ptr<std::string>($2);
        auto right_exp = std::unique_ptr<BaseAST>($3);
        $$ = new AddExpAST(left_exp, op->c_str(), right_exp);
    }
;

MulExp
    : UnaryExp {
        auto unary_exp = std::unique_ptr<BaseAST>($1);
        $$ = new MulExpAST(unary_exp);
    }
    | MulExp MULOP UnaryExp {
        auto left_exp = std::unique_ptr<BaseAST>($1);
        auto op = std::unique_ptr<std::string>($2);
        auto right_exp = std::unique_ptr<BaseAST>($3);
        $$ = new MulExpAST(left_exp, op->c_str(), right_exp);
    }
;

UnaryExp
    : PrimaryExp {
        auto primary_exp = std::unique_ptr<BaseAST>($1);
        $$ = new UnaryExpAST(primary_exp);
    }
    | UNARYOP UnaryExp {
        auto op = std::unique_ptr<std::string>($1);
        auto unary_exp = std::unique_ptr<BaseAST>($2);
        $$ = new UnaryExpAST(op->c_str(), unary_exp);
    }
    | ADDOP UnaryExp {
        auto op = std::unique_ptr<std::string>($1);
        auto unary_exp = std::unique_ptr<BaseAST>($2);
        $$ = new UnaryExpAST(op->c_str(), unary_exp);
    }
    | IDENT '(' {
            rparams.push_back(std::vector<BaseAST*>());
        } FuncRParams ')' {
            $$ = new UnaryExpAST($1->c_str(), rparams[rparams.size()-1]);
            rparams.pop_back();
    }
    | IDENT '(' ')' {
        rparams.push_back(std::vector<BaseAST*>());
        $$ = new UnaryExpAST($1->c_str(), rparams[rparams.size()-1]);
        rparams.pop_back();
    }
;

FuncRParams : FuncRParam | FuncRParams ',' FuncRParam;

FuncRParam 
    : Exp {
        rparams[rparams.size()-1].push_back($1);
    }
;


PrimaryExp  
    : '(' Exp ')' {
        auto exp = std::unique_ptr<BaseAST>($2);
        $$ = new PrimaryExpAST(exp);
    }
    | Number {
        auto number = std::unique_ptr<BaseAST>($1);
        $$ = new PrimaryExpAST(number);
    }
    | LVal {
        auto lval = std::unique_ptr<BaseAST>($1);
        $$ = new PrimaryExpAST(lval);
    }
;

LVal
    : IDENT {
        $$ = new LValAST($1->c_str());
    }
    |IDENT {
            idx_stk.push_back(std::vector<BaseAST*>());
        } IndexList {
            $$ = new LValAST($1->c_str(), idx_stk[idx_stk.size()-1]);
            idx_stk.pop_back();
    }
;

IndexList : Index | IndexList Index ;

Index : '[' Exp ']' {
        idx_stk[idx_stk.size()-1].push_back($2);
    }
;

%%
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}