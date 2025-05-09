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
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token IF INT VOID ELSE WHILE BREAK RETURN CONST CONTINUE
       ADDOP SUBOP NOTOP MULOP DIVOP MODOP
       LEOP GEOP NEOP EQOP LTOP GTOP LOROP LANDOP
%token <str_val> IDENT 
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnits CompUnit FuncDef Type Block BlockTemp 
                BlockItem Stmt LVal Exp PrimaryExp ConstExp 
                UnaryExp UnaryOp AddExp MulExp LOrExp LAndExp 
                EqExp RelExp Decl ConstDecl ConstDeclTemp 
                ConstDef ConstInitVal VarDecl VarDef ConstDefTemp
                InitVal VarDeclTemp StmtMatch StmtOpen
                FuncFParams FuncFParam FuncFParamTemp FuncRParams 
                ConstInitValTemp VarDefTemp InitValTemp LValTemp

%type <int_val> Number

%%

Root
  : CompUnits {
    ast = move(unique_ptr<BaseAST>($1));
    // cout<<"FINISHED!!!!\n";
  }

CompUnits 
  : CompUnits CompUnit {
    auto ast = new CompUnitAST();
    if (((CompUnitAST*)$2)->type_code == 0) {
      ast->parse_tag = ((CompUnitAST*)$1)->parse_tag;
      ast->glb_decl_vec = move(((CompUnitAST*)$1)->glb_decl_vec);
      ast->func_def_vec = move(((CompUnitAST*)$1)->func_def_vec);
      
      ast->func_def_vec.push_back(move(((CompUnitAST*)$2)->func_def_vec[0]));
      ast->parse_tag.push_back(((CompUnitAST*)$2)->parse_tag[0]);
    }
    else {
      ast->parse_tag = ((CompUnitAST*)$1)->parse_tag;
      ast->glb_decl_vec = move(((CompUnitAST*)$1)->glb_decl_vec);
      ast->func_def_vec = move(((CompUnitAST*)$1)->func_def_vec); 
      
      ast->glb_decl_vec.push_back(move(((CompUnitAST*)$2)->glb_decl_vec[0]));
      ast->parse_tag.push_back(((CompUnitAST*)$2)->parse_tag[0]);
    }
    $$ = ast;
  }
  | CompUnit {
    auto ast = new CompUnitAST();
    if (((CompUnitAST*)$1)->type_code == 0) {
      ast->parse_tag = ((CompUnitAST*)$1)->parse_tag; 
      ast->func_def_vec = move(((CompUnitAST*)$1)->func_def_vec);
    }
    else {
      ast->parse_tag = ((CompUnitAST*)$1)->parse_tag;
      ast->glb_decl_vec = move(((CompUnitAST*)$1)->glb_decl_vec);
    }
    $$ = ast;
  }
  ;

CompUnit
  : FuncDef {
    // cout<<"FuncDef\n";
    auto ast = new CompUnitAST();
    ast->type_code = 0;
    ast->func_def_vec.push_back(move(unique_ptr<BaseAST>($1)));
    ast->parse_tag.push_back(0);
    $$ = ast;
  }
  | Decl {
    // cout<<"Decl\n";
    auto ast = new CompUnitAST();
    ast->type_code = 1;
    ast->glb_decl_vec.push_back(move(unique_ptr<BaseAST>($1)));
    ast->parse_tag.push_back(1);
    $$ = ast;
  }
  ;

FuncDef
  : Type IDENT '(' ')' Block {
    // cout<<"FuncType IDENT Block\n";
    auto ast = new FuncDefAST();
    ast->type_code = 0;
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | Type IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->type_code = 1;
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($6);
    ast->func_f_param_vec = move(unique_ptr<FuncFParamsAST>(
                            dynamic_cast<FuncFParamsAST*>($4))->func_f_param_vec);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    // cout<<"FuncFParam\n";
    auto ast = new FuncFParamsAST();
    ast->func_f_param_vec.push_back(move(unique_ptr<BaseAST>($1)));
    $$ = ast;
  }
  | FuncFParams ',' FuncFParam {
    // cout<<"FuncFParams FuncFParam\n";
    auto ast = new FuncFParamsAST();
    ast->func_f_param_vec = move(unique_ptr<FuncFParamsAST>(
                            dynamic_cast<FuncFParamsAST*>($1))->func_f_param_vec);
    ast->func_f_param_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  ;

FuncFParam
  : Type IDENT {
    // cout<<"Type IDENT\n";
    auto ast = new FuncFParamAST();
    ast->type_code = 0;
    dynamic_cast<TypeAST*>($1)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | Type IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->type_code = 1;
    dynamic_cast<TypeAST*>($1)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | Type IDENT '[' ']' FuncFParamTemp {
    auto ast = new FuncFParamAST();
    ast->type_code = 1;
    dynamic_cast<TypeAST*>($1)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->const_exp_vec = move(unique_ptr<FuncFParamAST>(
                         dynamic_cast<FuncFParamAST*>($5))->const_exp_vec);
    $$ = ast;
  }
  ;

FuncFParamTemp
  : '[' ConstExp ']' {
    auto ast = new FuncFParamAST();
    ast->const_exp_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  | FuncFParamTemp '[' ConstExp ']' {
    auto ast = new FuncFParamAST();
    ast->const_exp_vec = move(unique_ptr<FuncFParamAST>(
                         dynamic_cast<FuncFParamAST*>($1))->const_exp_vec);
    ast->const_exp_vec.push_back(move(unique_ptr<BaseAST>($3)));
        $$ = ast;
  }
  ;

Type
  : INT {
    auto ast = new TypeAST();
    ast->type_name = "int";
    $$ = ast;
  }
  | VOID {
    auto ast = new TypeAST();
    ast->type_name = "void";
    $$ = ast;
  }
  ;

Block
  : '{' '}' {
    auto ast = new BlockAST();
    ast->block_item_vec.clear();
    $$ = ast;
  }
  | '{' BlockTemp '}' {
    // cout<<"BlockTemp\n";
    auto ast = new BlockAST();
    ast->block_item_vec = move(unique_ptr<BlockAST>(
      dynamic_cast<BlockAST*>($2))->block_item_vec);
    $$ = ast;
  }
  ;

BlockTemp
  : BlockTemp BlockItem {
    // cout<<"BlockTemp BlockItem\n";
    auto ast = new BlockAST();
    ast->block_item_vec = move(unique_ptr<BlockAST>(
                          dynamic_cast<BlockAST*>($1))->block_item_vec);
    ast->block_item_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  | BlockItem {
    // cout<<"BlockItem\n";
    auto ast = new BlockAST();
    ast->block_item_vec.push_back(move(unique_ptr<BaseAST>($1)));
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->type_code = 0;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    // cout<<"Stmt\n";
    auto ast = new BlockItemAST();
    ast->type_code = 1;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

StmtMatch
  : LVal '=' Exp ';' {
    // cout<<"LVal Exp\n";
    auto ast = new StmtAST();
    ast->type_code = 0;
    ast->exp_vec = move(dynamic_cast<LValAST*>($1)->exp_vec);
    ast->l_val = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    // cout<<"Exp\n";
    auto ast = new StmtAST();
    ast->type_code = 1;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    // cout<<";\n";
    auto ast = new StmtAST();
    ast->type_code = 1;
    $$ = ast;
  }
  | Block {
    // cout<<"Block\n";
    auto ast = new StmtAST();
    ast->type_code = 2;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    // cout<<"RETURN Exp ;\n";
    auto ast = new StmtAST();
    ast->type_code = 3;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    // cout<<"RETURN\n";
    auto ast = new StmtAST();
    ast->type_code = 3;
    $$ = ast;
  } 
  | IF '(' Exp ')' StmtMatch ELSE StmtMatch {
    // cout<<"IF Exp StmtMatch ELSE StmtMatch\n";
    auto ast = new StmtAST();
    ast->type_code = 4;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->true_stmt = unique_ptr<BaseAST>($5);
    ast->false_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    // cout<<"WHILE Exp Stmt\n";
    auto ast = new StmtAST();
    ast->type_code = 5;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->loop_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    // cout<<"BREAK\n";
    auto ast = new StmtAST();
    ast->type_code = 6;
    $$ = ast;
  }
  | CONTINUE ';' {
    // cout<<"CONTINUE\n";
    auto ast = new StmtAST();
    ast->type_code = 7;
    $$ = ast;
  }
  ;

StmtOpen
  : IF '(' Exp ')' Stmt {
    // cout<<"IF Exp Stmt\n";
    auto ast = new StmtAST();
    ast->type_code = 4;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->true_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' StmtMatch ELSE StmtOpen {
    // cout<<"IF Exp StmtMatch ELSE StmtOpen\n";
    auto ast = new StmtAST();
    ast->type_code = 4;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->true_stmt = unique_ptr<BaseAST>($5);
    ast->false_stmt = unique_ptr<BaseAST>($7);    
    $$ = ast;
  }
  ;

Stmt
  : StmtMatch {
    // cout<<"StmtMatch\n";
    auto ast = new StmtAST();
    ast->type_code = dynamic_cast<StmtAST*>($1)->type_code;
    ast->l_val = move(dynamic_cast<StmtAST*>($1)->l_val);
    ast->exp = move(dynamic_cast<StmtAST*>($1)->exp);
    ast->block = move(dynamic_cast<StmtAST*>($1)->block);
    ast->loop_stmt = move(dynamic_cast<StmtAST*>($1)->loop_stmt);
    ast->true_stmt = move(dynamic_cast<StmtAST*>($1)->true_stmt);
    ast->false_stmt = move(dynamic_cast<StmtAST*>($1)->false_stmt);
    ast->exp_vec = move(dynamic_cast<StmtAST*>($1)->exp_vec);
    $$ = ast;
  }
  | StmtOpen {
    // cout<<"StmtOpen\n";
    auto ast = new StmtAST();
    ast->type_code = dynamic_cast<StmtAST*>($1)->type_code;
    ast->exp = move(dynamic_cast<StmtAST*>($1)->exp);
    ast->true_stmt = move(dynamic_cast<StmtAST*>($1)->true_stmt);
    ast->false_stmt = move(dynamic_cast<StmtAST*>($1)->false_stmt);
    $$ = ast;
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
    ast->type_code = 0;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type_code = 1;
    ast->number = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type_code = 2;
    ast->exp_vec = move(dynamic_cast<LValAST*>($1)->exp_vec);
    ast->l_val = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->type_code = 0;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type_code = 1;
    ast->unary_op = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->type_code = 2;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->type_code = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->exp_vec = move(unique_ptr<FuncRParamsAST>(
                        dynamic_cast<FuncRParamsAST*>($3))->exp_vec);
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->exp_vec.push_back(move(unique_ptr<BaseAST>($1)));
    $$ = ast;
  }
  | FuncRParams ',' Exp {
    auto ast = new FuncRParamsAST();
    ast->exp_vec = move(unique_ptr<FuncRParamsAST>(
                        dynamic_cast<FuncRParamsAST*>($1))->exp_vec);
    ast->exp_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  ;

UnaryOp
  : ADDOP {
    auto ast = new UnaryOpAST();
    ast->type_code = 0;
    ast->op = "+";
    $$ = ast;
  }
  | SUBOP {
    auto ast = new UnaryOpAST();
    ast->type_code = 1;
    ast->op = "-";
    $$ = ast;
  }
  | NOTOP {
    auto ast = new UnaryOpAST();
    ast->type_code = 2;
    ast->op = "!";
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->type_code = 0;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp ADDOP MulExp {
    auto ast = new AddExpAST();
    ast->type_code = 1;
    ast->op = "+";
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp SUBOP MulExp {
    auto ast = new AddExpAST();
    ast->type_code = 1;
    ast->op = "-";
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->type_code = 0;
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MULOP UnaryExp {
    auto ast = new MulExpAST();
    ast->type_code = 1;
    ast->op = "*";
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp DIVOP UnaryExp {
    auto ast = new MulExpAST();
    ast->type_code = 1;
    ast->op = "/";
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp MODOP UnaryExp {
    auto ast = new MulExpAST();
    ast->type_code = 1;
    ast->op = "%";
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->type_code = 0;
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOROP LAndExp {
    auto ast = new LOrExpAST();
    ast->type_code = 1;
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->op = "||";
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->type_code = 0;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LANDOP EqExp {
    auto ast = new LAndExpAST();
    ast->type_code = 1;
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->op = "&&";
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->type_code = 0;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->type_code = 1;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = "==";
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NEOP RelExp {
    auto ast = new EqExpAST();
    ast->type_code = 1;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = "!=";
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 0;
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp LTOP AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = "<";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GTOP AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = ">";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp LEOP AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = "<=";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GEOP AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = ">=";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    // cout<<"ConstDecl\n";
    auto ast = new DeclAST();
    ast->type_code = 0;
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    // cout<<"VarDecl\n";
    auto ast = new DeclAST();
    ast->type_code = 1;
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST Type ConstDef ';' {
    // cout<<"CONST Type ConstDef\n";
    auto ast = new ConstDeclAST();
    dynamic_cast<TypeAST*>($2)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($2);
    ast->const_def_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  | CONST Type ConstDef ConstDeclTemp ';' {
    auto ast = new ConstDeclAST();
    
    dynamic_cast<TypeAST*>($2)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($2);
    ast->const_def_vec = move(unique_ptr<ConstDeclAST>(
                         dynamic_cast<ConstDeclAST*>($4))->const_def_vec);
    ast->const_def_vec.insert(ast->const_def_vec.begin(), 
                              move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  } 
  ;

ConstDeclTemp
  : ConstDeclTemp ',' ConstDef {
    auto ast = new ConstDeclAST();

    ast->const_def_vec = move(unique_ptr<ConstDeclAST>(
                         dynamic_cast<ConstDeclAST*>($1))->const_def_vec);
    ast->const_def_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  | ',' ConstDef {
    auto ast = new ConstDeclAST();
    ast->const_def_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  ;

ConstDef 
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->type_code = 0;
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ConstDefTemp '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->type_code = 1;
    ast->ident = *unique_ptr<string>($1);

    ast->const_exp_vec = move(unique_ptr<ConstDefAST>(
                         dynamic_cast<ConstDefAST*>($2))->const_exp_vec);

    dynamic_cast<ConstInitValAST*>($4)->ident = ast->ident;
    dynamic_cast<ConstInitValAST*>($4)->level = 0;
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstDefTemp
  : '[' ConstExp ']' {
    auto ast = new ConstDefAST();
    ast->const_exp_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  | ConstDefTemp '[' ConstExp ']' {
    auto ast = new ConstDefAST();

    ast->const_exp_vec = move(unique_ptr<ConstDefAST>(
                         dynamic_cast<ConstDefAST*>($1))->const_exp_vec);
    ast->const_exp_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->type_code = 0;
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast->type_code = 1;
    $$ = ast;
  }
  | '{' ConstInitValTemp '}' {
    auto ast = new ConstInitValAST();
    ast->type_code = 1;
    ast->const_init_vec = move(unique_ptr<ConstInitValAST>(
                          dynamic_cast<ConstInitValAST*>($2))->const_init_vec);
    $$ = ast;
  }
  ;

ConstInitValTemp
  : ConstInitVal {
    auto ast = new ConstInitValAST();
    ast->const_init_vec.push_back(move(unique_ptr<BaseAST>($1)));
    $$ = ast;
  }
  | ConstInitValTemp ',' ConstInitVal {
    auto ast = new ConstInitValAST();
    ast->const_init_vec = move(unique_ptr<ConstInitValAST>(
                          dynamic_cast<ConstInitValAST*>($1))->const_init_vec);
    ast->const_init_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->type_code = 0;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT LValTemp {
    auto ast = new LValAST();
    ast->type_code = 1;
    ast->ident = *unique_ptr<string>($1);
    ast->exp_vec = move(unique_ptr<LValAST>(
                   dynamic_cast<LValAST*>($2))->exp_vec);
    $$ = ast;
  }
  ;

LValTemp
  : '[' Exp ']' {
    auto ast = new LValAST();
    ast->exp_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  | LValTemp '[' Exp ']' {
    auto ast = new LValAST();
    ast->exp_vec = move(unique_ptr<LValAST>(
                   dynamic_cast<LValAST*>($1))->exp_vec);
    ast->exp_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
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
  : Type VarDef ';' {
    // cout<<"Type VarDef\n";
    auto ast = new VarDeclAST();
    dynamic_cast<TypeAST*>($1)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($1);
    ast->var_def_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  | Type VarDef VarDeclTemp ';' {
    auto ast = new VarDeclAST();

    dynamic_cast<TypeAST*>($1)->type_code = 1;
    ast->type = unique_ptr<BaseAST>($1);
    ast->var_def_vec = move(unique_ptr<VarDeclAST>(
                            dynamic_cast<VarDeclAST*>($3))->var_def_vec);
    ast->var_def_vec.insert(ast->var_def_vec.begin(), 
                            move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  ;

VarDeclTemp
  : VarDeclTemp ',' VarDef {
    auto ast = new VarDeclAST();

    ast->var_def_vec = move(unique_ptr<VarDeclAST>(
                            dynamic_cast<VarDeclAST*>($1))->var_def_vec);
    ast->var_def_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  | ',' VarDef {
    auto ast = new VarDeclAST();
    ast->var_def_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  ;

VarDef
  : IDENT {
    // cout<<"IDENT\n";
    auto ast = new VarDefAST();
    ast->type_code = 0;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->type_code = 1;
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT VarDefTemp {
    // cout<<"IDENT VarDefTemp\n";
    auto ast = new VarDefAST();
    ast->type_code = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->const_exp_vec = move(unique_ptr<VarDefAST>(
                         dynamic_cast<VarDefAST*>($2))->const_exp_vec);
    $$ = ast;
  }
  | IDENT VarDefTemp '=' InitVal {
    auto ast = new VarDefAST();
    ast->type_code = 3;
    ast->ident = *unique_ptr<string>($1);
    ast->const_exp_vec = move(unique_ptr<VarDefAST>(
                         dynamic_cast<VarDefAST*>($2))->const_exp_vec);

    dynamic_cast<InitValAST*>($4)->ident = ast->ident;
    dynamic_cast<InitValAST*>($4)->level = 0;
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

VarDefTemp
  : '[' ConstExp ']' {
    auto ast = new VarDefAST();
    ast->const_exp_vec.push_back(move(unique_ptr<BaseAST>($2)));
    $$ = ast;
  }
  | VarDefTemp '[' ConstExp ']' {
    auto ast = new VarDefAST();
    ast->const_exp_vec = move(unique_ptr<VarDefAST>(
                         dynamic_cast<VarDefAST*>($1))->const_exp_vec);
    ast->const_exp_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->type_code = 0;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->type_code = 1;
    $$ = ast;
  }
  | '{' InitValTemp '}' {
    auto ast = new InitValAST();
    ast->type_code = 1;
    ast->init_vec = move(unique_ptr<InitValAST>(
                    dynamic_cast<InitValAST*>($2))->init_vec);
    $$ = ast;
  }
  ;

InitValTemp
  : InitVal {
    auto ast = new InitValAST();
    ast->init_vec.push_back(move(unique_ptr<BaseAST>($1)));
    $$ = ast;
  }
  | InitValTemp ',' InitVal {
    auto ast = new InitValAST();
    ast->init_vec = move(unique_ptr<InitValAST>(
                    dynamic_cast<InitValAST*>($1))->init_vec);
    ast->init_vec.push_back(move(unique_ptr<BaseAST>($3)));
    $$ = ast;
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
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
