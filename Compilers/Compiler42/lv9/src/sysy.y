%code requires {
  #include <memory>
  #include <string>
  // CRay引入AST.h头文件
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include "AST.h"

// 声明 lexer 函数和错误处理函数
int yylex();
//CRay ==================替换成AST类
//void yyerror(unique_ptr<string> &ast, const char *s);
void yyerror(unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
// CRay根据参考文档,修改以下语句,替换成AST类
// %parse-param { unique_ptr<string> &ast }
%parse-param { unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  string *str_val;
  int int_val;
  BaseAST *ast_val;  
  vector<unique_ptr<BaseAST>> *ast_vec;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
// CRay增加token的定义
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE 
%token <str_val> IDENT EQOP RELOP AND OR
%token <int_val> INT_CONST

// 非终结符的类型定义
// CRay增加非终结符, 要都包含，不然会报错======：$$ of 'FuncType' has no declared type
%type <ast_val> CompUnit FuncDef Block BlockItem Stmt Decl Type If Def
%type <ast_val> Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp Number LVal
%type <ast_val> ConstDecl ConstDef
%type <ast_val> VarDecl VarDef InitVal FuncFParam FuncRParam

%type <ast_vec> BlockArray ConstDefArray VarDefArray DefArray InitValArray
%type <ast_vec> FuncFParamArray FuncRParamArray IndexArray 

%type <str_val> UNARYOP MULOP ADDOP

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : DefArray {
    auto defs = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    ast = unique_ptr<BaseAST>(new CompUnitAST(defs));
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
DefArray
  : Def DefArray {
    auto vec = (vector<unique_ptr<BaseAST>>*)($2);
    auto def = unique_ptr<BaseAST>($1);
    vec->push_back(move(def));
    $$ = vec;
  }
  | Def {
    //cout << "FuncDef" << endl;
    auto vec = new vector<unique_ptr<BaseAST>>();
    auto def = unique_ptr<BaseAST>($1);
    vec->push_back(move(def));
    $$ = vec;
  }
  ;

Def 
  : FuncDef {
    auto funcdef = unique_ptr<BaseAST>($1);
    $$ = new DefAST(funcdef, DefAST::DefType::FuncDef);
  } | ConstDecl {
    auto globaconstdef = unique_ptr<BaseAST>($1);
    $$ = new DefAST(globaconstdef, DefAST::DefType::ConstDef);
  } | VarDecl {
    auto globavardef = unique_ptr<BaseAST>($1);
    $$ = new DefAST(globavardef, DefAST::DefType::VarDef);
  };

FuncDef
  : Type IDENT '(' FuncFParamArray ')' Block {
    // CRay增加AST处理
    // auto type = unique_ptr<string>($1);
    // auto ident = unique_ptr<string>($2);
    // auto block = unique_ptr<string>($5);
    // $$ = new string(*type + " " + *ident + "() " + *block);
    //CRay================ error 多了new
    auto func_type = unique_ptr<BaseAST>($1);
    auto ident = unique_ptr<string>($2);
    auto func_fparam_array = unique_ptr<vector<unique_ptr<BaseAST>>>($4);
    auto block = unique_ptr<BaseAST>($6);
    $$ = new FuncDefAST(func_type, ident->c_str(), func_fparam_array, block);
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

FuncFParamArray
  : FuncFParam ',' FuncFParamArray {
    auto vec = (vector<unique_ptr<BaseAST>>*)($3);
    auto func_fparam = unique_ptr<BaseAST>($1);
    vec->push_back(move(func_fparam));
    $$ = vec;
  }
  | FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST>> ();
    auto func_fparam = unique_ptr<BaseAST>($1);
    vec->push_back(move(func_fparam));
    $$ = vec;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST>> ();
  }
  ;

FuncFParam
  : Type IDENT {
    auto type = unique_ptr<BaseAST>($1);
    auto ident = unique_ptr<string>($2);
    $$ = new FuncFParamAST(type, ident->c_str(), FuncFParamAST::FuncFParamType::Var);
  }
  | Type IDENT '[' ']' {
    auto type = unique_ptr<BaseAST>($1);
    auto ident = unique_ptr<string>($2);
    $$ = new FuncFParamAST(type, ident->c_str(), FuncFParamAST::FuncFParamType::Array);
  }
  | Type IDENT '[' ']' IndexArray {
    auto type = unique_ptr<BaseAST>($1);
    auto ident = unique_ptr<string>($2);
    auto index_exp = unique_ptr<vector<unique_ptr<BaseAST>>>($5);
    $$ = new FuncFParamAST(type, index_exp, ident->c_str(), FuncFParamAST::FuncFParamType::Array);
  }
  ;

Block
  : '{' BlockArray '}' {
    auto BlockArray = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = new BlockAST(BlockArray);
  }
  //CRay Lv5
  | '{' '}' {
    $$ = new BlockAST();
  }
  ;

BlockArray
  : BlockItem BlockArray{
    auto vec = (vector<unique_ptr<BaseAST>>*)($2);
    auto blockitem = unique_ptr<BaseAST>($1);
    vec->push_back(move(blockitem));
    $$ = vec;
  }
  | BlockItem {
    auto vec = new vector<unique_ptr<BaseAST>> ();
    auto blockitem = unique_ptr<BaseAST>($1);
    vec->push_back(move(blockitem));
    $$ = vec;
  }
  ;

BlockItem : Stmt | Decl;

Stmt
  : RETURN Exp ';' {
    // CRay增加AST处理
    // auto number = unique_ptr<string>($2);
    // $$ = new string("return " + *number + ";");
    // CRay增加Lv3 的 EXP处理

    auto exp = unique_ptr<BaseAST>($2);
    //CRay Lv5
    $$ = new StmtAST(exp, StmtAST::StmtType::Return);
  }
  | LVal '=' Exp ';' {
    //CRay 以下增加Lv4运算符相关内容
    auto lval = unique_ptr<BaseAST>($1);
    auto exp = unique_ptr<BaseAST>($3);
    //CRay Lv5
    $$ = new StmtAST(lval, exp, StmtAST::StmtType::Assign);
  }
  | Block {
    auto block = unique_ptr<BaseAST>($1);
    $$ = new StmtAST(block, StmtAST::StmtType::Block);
  }
  | Exp ';' {
    auto exp = unique_ptr<BaseAST>($1);
    $$ = new StmtAST(exp, StmtAST::StmtType::Exp);
  }
  | ';' {
    $$ = new StmtAST(StmtAST::StmtType::Empty);
  }
  | RETURN ';' {
    $$ = new StmtAST(StmtAST::StmtType::Return);  
  }
  | If {
    auto exp = unique_ptr<BaseAST>($1);
    $$ = new StmtAST(exp, StmtAST::StmtType::If);
  }
  | If ELSE Stmt {
    auto exp = unique_ptr<BaseAST>($1);
    auto stmt = unique_ptr<BaseAST>($3);
    $$ = new StmtAST(stmt, exp, StmtAST::StmtType::If);
  }

  //CRay Lv7
  | WHILE '(' Exp ')' Stmt {
    auto exp = unique_ptr<BaseAST>($3);
    auto stmt = unique_ptr<BaseAST>($5);
    $$ = new StmtAST(stmt, exp, StmtAST::StmtType::While);
  }
  | BREAK ';' {
    $$ = new StmtAST(StmtAST::StmtType::Break);
  }
  | CONTINUE ';' {
    $$ = new StmtAST(StmtAST::StmtType::Continue);
  }  
  ;

If
 : IF '(' Exp ')' Stmt {
    auto exp = unique_ptr<BaseAST>($3);
    auto stmt = unique_ptr<BaseAST>($5);
    $$ = new IfAST(exp, stmt);
  }
  ;

Decl : ConstDecl | VarDecl;

ConstDecl
  : CONST Type ConstDefArray ';' {
    auto type = unique_ptr<BaseAST>($2);
    auto const_defs = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = new ConstDeclAST(type, const_defs);
  }
  ;

ConstDefArray
  : ConstDef ',' ConstDefArray {
    auto vec = (vector<unique_ptr<BaseAST>>*)($3);
    auto const_def = unique_ptr<BaseAST>($1);
    vec->push_back(move(const_def));
    $$ = vec;
  }
  | ConstDef {
    auto vec = new vector<unique_ptr<BaseAST>> ();
    auto const_def = unique_ptr<BaseAST>($1);
    vec->push_back(move(const_def));
    $$ = vec;
  }

  ;

ConstDef 
  : IDENT '=' InitVal {
    auto ident = unique_ptr<string>($1);
    auto const_init_val = unique_ptr<BaseAST>($3);
    $$ = new ConstDefAST(ident->c_str(), const_init_val);
  }
  | IDENT IndexArray '=' InitVal {
    auto ident = unique_ptr<string>($1);
    auto index_exp = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    auto const_init_val = unique_ptr<BaseAST>($4);
    $$ = new ConstDefAST(ident->c_str(), index_exp, const_init_val);
  }
  ;

VarDecl
  : Type VarDefArray ';' {
    auto type = unique_ptr<BaseAST>($1);
    auto var_defs = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = new VarDeclAST(type, var_defs);
  }
  ;

VarDefArray
  : VarDef ',' VarDefArray {
    auto vec = (vector<unique_ptr<BaseAST>>*)($3);
    auto var_def = unique_ptr<BaseAST>($1);
    vec->push_back(move(var_def));
    $$ = vec;
  }
  | VarDef {
    auto vec = new vector<unique_ptr<BaseAST>> ();
    auto var_def = unique_ptr<BaseAST>($1);
    vec->push_back(move(var_def));
    $$ = vec;
  }
  ;

VarDef
  : IDENT '=' InitVal {
    auto ident = unique_ptr<string>($1);
    auto init_val = unique_ptr<BaseAST>($3);
    $$ = new VarDefAST(ident->c_str(), init_val, VarDefAST::VarDefType::Exp);
  }
  | IDENT {
    auto ident = unique_ptr<string>($1);
    $$ = new VarDefAST(ident->c_str(), VarDefAST::VarDefType::Exp);
  }
  | IDENT IndexArray {
    auto ident = unique_ptr<string>($1);
    auto index_exp = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = new VarDefAST(ident->c_str(), index_exp, VarDefAST::VarDefType::Array);
  }
  | IDENT IndexArray '=' InitVal {
    auto ident = unique_ptr<string>($1);
    auto index_exp = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    auto init_val = unique_ptr<BaseAST>($4);
    $$ = new VarDefAST(ident->c_str(), index_exp, init_val, VarDefAST::VarDefType::Array);
  }
  ;

InitVal 
  : Exp {
    auto exp = unique_ptr<BaseAST>($1);
    $$ = new InitValAST(exp);
  }
  | '{' InitValArray '}' {
    auto init_val_array = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = new InitValAST(init_val_array);
  }
  | '{' '}' {
    $$ = new InitValAST();
  }
  ;

InitValArray
  : InitVal ',' InitValArray {
    auto vec = (vector<unique_ptr<BaseAST>>*)($3);
    auto exp = unique_ptr<BaseAST>($1);
    vec->push_back(move(exp));
    $$ = vec;
  }
  | InitVal {
    auto vec = new vector<unique_ptr<BaseAST>> ();
    auto exp = unique_ptr<BaseAST>($1);
    vec->push_back(move(exp));
    $$ = vec;
  }
  ;

LVal
  : IDENT {
    auto ident = unique_ptr<string>($1);
    $$ = new LValAST(ident->c_str());
  }
  | IDENT IndexArray {
    auto ident = unique_ptr<string>($1);
    auto exp = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = new LValAST(ident->c_str(), exp);
  }
  ;

IndexArray
  : '[' Exp ']' {
    auto exp = unique_ptr<BaseAST>($2);
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(move(exp));
    $$ = vec;
  }
  | IndexArray '[' Exp ']' {
    auto vec = (vector<unique_ptr<BaseAST>>*)($1);
    auto exp = unique_ptr<BaseAST>($3);
    vec->push_back(move(exp));
    $$ = vec;
  }
  ;

Exp
  : LOrExp {
    auto lorexp = unique_ptr<BaseAST>($1);
    $$ = new ExpAST(lorexp);
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto exp = unique_ptr<BaseAST>($2);
    $$ = new PrimaryExpAST(exp);
  } 
  | Number {
    auto number = unique_ptr<BaseAST>($1);
    $$ = new PrimaryExpAST(number);
  }
  | LVal {
    auto lval = unique_ptr<BaseAST>($1);
    $$ = new PrimaryExpAST(lval);
  }
  ;

UnaryExp
  : PrimaryExp {
    auto primaryexp = unique_ptr<BaseAST>($1);
    $$ = new UnaryExpAST(primaryexp);
  }
  | UNARYOP UnaryExp {
    auto unaryop = unique_ptr<string>($1);
    auto unaryexp = unique_ptr<BaseAST>($2);
    $$ = new UnaryExpAST(unaryop->c_str(), unaryexp);
  }
  | IDENT '(' FuncRParamArray ')' {
    auto ident = unique_ptr<string>($1);
    auto func_rparam_array = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = new UnaryExpAST(ident->c_str(), func_rparam_array);
  }
  ;

FuncRParamArray
  : FuncRParam ',' FuncRParamArray {
    auto vec = (vector<unique_ptr<BaseAST>>*)($3);
    auto func_rparam = unique_ptr<BaseAST>($1);
    vec->push_back(move(func_rparam));
    $$ = vec;
  }
  | FuncRParam {
    auto vec = new vector<unique_ptr<BaseAST>> ();
    auto func_rparam = unique_ptr<BaseAST>($1);
    vec->push_back(move(func_rparam));
    $$ = vec;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST>> ();
  }
  ;

FuncRParam : Exp;

UNARYOP :
  '-' {
    $$ = new string("-");
  }| '!' {
    $$ = new string("!");
  }| '+' {
    $$ = new string("+");
  }
  ;

MulExp
  : UnaryExp {
    auto unaryexp = unique_ptr<BaseAST>($1);
    $$ = new MulExpAST(unaryexp);
  }
  | MulExp MULOP UnaryExp {
    auto mulexp = unique_ptr<BaseAST>($1);
    auto mulop = unique_ptr<string>($2);
    auto unaryexp = unique_ptr<BaseAST>($3);
    $$ = new MulExpAST(mulop->c_str(), mulexp, unaryexp);
  }
  ;

MULOP : 
  '*' {
    $$ = new string("*");
  }| '/' {
    $$ = new string("/");
  }| '%' {
    $$ = new string("%");
  }
  ;

AddExp
  : MulExp {
    auto mulexp = unique_ptr<BaseAST>($1);
    $$ = new AddExpAST(mulexp);
  }
  | AddExp ADDOP MulExp {
    auto addexp = unique_ptr<BaseAST>($1);
    auto addop = unique_ptr<string>($2);
    auto mulexp = unique_ptr<BaseAST>($3);
    $$ = new AddExpAST(addop->c_str(), addexp, mulexp);
  }
  ;

ADDOP :
  '+' {
    $$ = new string("+");
  }| '-' {
    $$ = new string("-");
  }
  ;

RelExp :
  AddExp {
    auto addexp = unique_ptr<BaseAST>($1);
    $$ = new RelExpAST(addexp);
  }
  | RelExp RELOP AddExp {
    auto relexp = unique_ptr<BaseAST>($1);
    auto relop = unique_ptr<string>($2);
    auto addexp = unique_ptr<BaseAST>($3);
    $$ = new RelExpAST(relop->c_str(), relexp, addexp);
  }
  ;

EqExp :
  RelExp {
    auto relexp = unique_ptr<BaseAST>($1);
    $$ = new EqExpAST(relexp);
  }
  | EqExp EQOP RelExp {
    auto eqexp = unique_ptr<BaseAST>($1);
    auto eqop = unique_ptr<string>($2);
    auto relexp = unique_ptr<BaseAST>($3);
    $$ = new EqExpAST(eqop->c_str(), eqexp, relexp);
  }
  ;

LAndExp :
  EqExp {
    auto eqexp = unique_ptr<BaseAST>($1);
    $$ = new LAndExpAST(eqexp);
  }
  | LAndExp AND EqExp {
    auto landexp = unique_ptr<BaseAST>($1);
    auto andop = unique_ptr<string>($2);
    auto eqexp = unique_ptr<BaseAST>($3);
    $$ = new LAndExpAST(andop->c_str(), landexp, eqexp);
  }
  ;

LOrExp :
  LAndExp {
    auto landexp = unique_ptr<BaseAST>($1);
    $$ = new LOrExpAST(landexp);
  }
  | LOrExp OR LAndExp {
    auto lorexp = unique_ptr<BaseAST>($1);
    auto orop = unique_ptr<string>($2);
    auto landexp = unique_ptr<BaseAST>($3);
    $$ = new LOrExpAST(orop->c_str(), lorexp, landexp);
  }
  ;

Number
  : INT_CONST {
    // CRay增加AST处理
    // $$ = new string(to_string($1));
    $$ = new NumberAST($1);
  }
  ;


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
// CRay增加AST处理
// void yyerror(unique_ptr<string> &ast, const char *s) {
//  cerr << "error: " << s << endl;
//}
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  extern int yylineno;
  extern char *yytext;
  int len = strlen(yytext);
  int i;
  char buf[512] = {0};
  for (i=0; i<len; ++i)
    sprintf(buf, "%s%d ", buf, yytext[i]);
  fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}
