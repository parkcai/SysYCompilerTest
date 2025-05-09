%code requires {
  #include <memory>
  #include <string.h>
  #include <AST.h>
}

%{
#include <iostream>
#include <memory>
#include <string.h>
#include <AST.h>
#include <map>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);
using namespace std;
static int idx=0;
%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast  }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  char char_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT 
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block BlockList Stmt Exp UnaryExp PrimaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp Decl ConstDecl ConstDef ConstExp ConstDeclList VarDecl VarDeclList VarDef ConstInitVal InitVal MS UMS FuncParams FuncParam FuncRParams ConstInitValList InitValList ConstExpList LVal ExpList
%type <char_val> UnaryOp 
%type <int_val> Number 

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->vec.push_back(unique_ptr<BaseAST>($1));
    ast = move(comp_unit);
  }
  | CompUnit FuncDef {
    auto comp_unit = move(ast);
    comp_unit->vec.push_back(unique_ptr<BaseAST>($2));
    ast = move(comp_unit);
  }
  | Decl {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->vec2.push_back(unique_ptr<BaseAST>($1));
    ast = move(comp_unit);
  }
  | CompUnit Decl {
    auto comp_unit = move(ast);
    comp_unit->vec2.push_back(unique_ptr<BaseAST>($2));
    ast = move(comp_unit);
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
FuncDef
  : FuncType IDENT '(' FuncParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    string temp = *unique_ptr<string>($2);
    ast->ident = temp;
    ast->block = unique_ptr<BaseAST>($6);
    ast->params = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    auto params = new FuncParamsAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    string temp = *unique_ptr<string>($2);
    ast->ident = temp;
    ast->block = unique_ptr<BaseAST>($5);
    ast->params = unique_ptr<BaseAST>(params);
    $$ = ast;
  }
  ;

FuncParams
  : FuncParam {
    auto ast = new FuncParamsAST();
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncParams ',' FuncParam {
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

FuncParam
  : FuncType IDENT {
    auto ast = new FuncParamAST();
    ast->if_array = false;
    string temp = *unique_ptr<string>($2);
    ast->ident = temp;
    ast->idx = idx++;
    $$ = ast;
  }
  | FuncType IDENT '[' ']' ConstExpList{
    auto ast = new FuncParamAST();
    ast->if_array = true;
    string temp = *unique_ptr<string>($2);
    ast->ident = temp;
    ast->idx = idx++;
    ast->dim = unique_ptr<BaseAST>($5);
    $$ = ast;
  } 
  | FuncType IDENT '[' ']'{
    auto ast = new FuncParamAST();
    ast->if_array = true;
    string temp = *unique_ptr<string>($2);
    ast->ident = temp;
    ast->idx = idx++;
    ast->dim = unique_ptr<BaseAST>(new ConstExpListAST());
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->ident = "int";
    $$ = ast;
  }
  | VOID {
    auto ast = new FuncTypeAST();
    ast->ident = "void";
    $$ = ast;
  }
  ;

Block
  : '{' BlockList '}' {
    $$ = $2;
  }
  | '{' '}' {
    auto ast = new BlockAST();
    ast->vec.clear();
    $$ = ast;
  }
  ;

BlockList
  : Decl {
    auto ast = new BlockAST();
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockAST();
    if($1 != NULL) ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast; 
  }
  | BlockList Decl {
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | BlockList Stmt {
    auto ast = $1;
    if($2 != NULL) ast->vec.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;  
  }
  ;

Stmt
  : MS {
    $$ = $1;
  }
  | UMS {
    $$ = $1;
  }
  ;

MS 
  : IF '(' Exp ')' MS ELSE MS {
    auto ast = new StmtAST();
    ast->type = 4;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = 0;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = 2;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    BaseAST *ast = new StmtAST();
    ast->type = -1;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = 3;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->type = 9;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = 6;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->type = 7;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->type = 8;
    $$ = ast;
  }

UMS
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = 5;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MS ELSE UMS {
    auto ast = new StmtAST();
    ast->type = 4;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }

Exp
  : LOrExp {
    $$ = $1;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    $$ = $2;
  }
  | Number {
    auto ast = new ExpAST();
    ast->type = -1;
    ast->idx = -1;
    ast->val = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new ExpAST();
    ast->lval = unique_ptr<BaseAST>($1); 
    ast->type = 15;
    ast->idx = idx++;
    $$ = ast;
  }
  ;

LVal          
  : IDENT {
    auto ast = new LValAST();
    ast->type = 0;
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->idx = idx++;
    $$ = ast;
  }
  | IDENT ExpList {
    auto ast = new LValAST();
    ast->type = 1;
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->exp = unique_ptr<BaseAST>($2);
    ast->idx = idx + ($2)->vec.size() + 3;
    idx += ($2)->vec.size() + 4;
    $$ = ast;
  }
  ;

ExpList
  : '[' Exp ']'{
    auto ast = new ExpListAST();
    ast->vec.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | ExpList '[' Exp ']'{
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
      $$ = $1;
  }
  | UnaryOp UnaryExp {
      if($1 != '+'){
        auto ast = new ExpAST;
        ast->exp = unique_ptr<BaseAST>($2);
        ast->idx = idx++;
        if($1 == '-') {
          ast->type = 1;
        }
        if($1 == '!'){
          ast->type = 0;
        }
        $$ = ast;
      }
      else{
        $$ = $2;
      }
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new ExpAST;
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->type = 16;
    ast->idx = idx++;
    ast->params = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new ExpAST;
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->type = 17;
    ast->idx = idx++;
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncRParams ',' Exp {
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }

UnaryOp
  : '+' {$$ = '+';}
  | '-' {$$ = '-';}
  | '!' {$$ = '!';}
  ;

MulExp
  : UnaryExp{
    $$ = $1;
  }
  | MulExp '*' UnaryExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 2;
    $$ = ast;
  }
  | MulExp '/' UnaryExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 3;
    $$ = ast;
  }
  | MulExp '%' UnaryExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 4;
    $$ = ast;
  }
  ;



AddExp
  : MulExp{
    $$ = $1;
  }
  | AddExp '+' MulExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 5;
    $$ = ast;
  }
  | AddExp '-' MulExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 6;
    $$ = ast;
  }
  ;

RelExp
  : AddExp{
    $$ = $1;
  }
  | RelExp '<' AddExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 7;
    $$ = ast;
  }
  | RelExp '>' AddExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($3);
    ast->idx = idx++;
    ast->type = 8;
    $$ = ast;
  }
  | RelExp '<' '=' AddExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($4);
    ast->idx = idx++;
    ast->type = 9;
    $$ = ast;
  }
  | RelExp '>' '=' AddExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($4);
    ast->idx = idx++;
    ast->type = 10;
    $$ = ast;
  }
  ;

EqExp
  : RelExp{
    $$ = $1;
  }
  | EqExp '=' '=' RelExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($4);
    ast->idx = idx++;
    ast->type = 11;
    $$ = ast;
  }
  | EqExp '!' '=' RelExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($4);
    ast->idx = idx++;
    ast->type = 12;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp{
    $$ = $1;
  }
  | LAndExp '&' '&' EqExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($4);
    ast->idx = idx+2;
    idx+=3;
    ast->type = 13;
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp{
    $$ = $1;
  }
  | LOrExp '|' '|' LAndExp{
    auto ast = new ExpAST;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ano_exp = unique_ptr<BaseAST>($4);
    ast->idx = idx+2;
    idx+=3;
    ast->type = 14;
    ast->val = (ast->exp)->val || (ast->ano_exp)->val;
    $$ = ast;
  }
  ;

Decl
  : ConstDecl{
    $$->type = 1;
    $$ = $1;
  }
  | VarDecl{
    $$->type = 1;
    $$ = $1;
  }
  ;

ConstDecl
  : CONST FuncType ConstDeclList ';'{
    $$ = $3;
  }
  ;

ConstDeclList
  : ConstDeclList ',' ConstDef {
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | ConstDef{
    auto ast = new ConstDeclAST();
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  ;

VarDecl
  : FuncType VarDeclList ';'{
    $$ = $2;
  }
  ;

VarDeclList
  : VarDeclList ',' VarDef {
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | VarDef{
    auto ast = new VarDeclAST();
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  ;



ConstDef 
  : IDENT '=' ConstInitVal{
    auto ast = new ConstDefAST();
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->initval = unique_ptr<BaseAST>($3);
    ast->type = 0;
    $$ = ast;
  }
  | IDENT ConstExpList '=' ConstInitVal{
    auto ast = new ConstDefAST();
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->initval = unique_ptr<BaseAST>($4);
    ast->dim = unique_ptr<BaseAST>($2);
    ast->type = 1;
    $$ = ast;
  }
  ;

ConstExpList
  : '[' ConstExp ']'{
    auto ast = new ConstExpListAST();
    ast->vec.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | ConstExpList '[' ConstExp ']'{
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

ConstInitVal  
  : ConstExp{
    auto ast = new ConstInitValAST();
    ast->type = 0;
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | '{' ConstInitValList '}'{
    $$ = $2;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto ast = new ConstInitValAST();
    ast->type = 1;
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstInitValList ',' ConstInitVal{
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | {
    auto ast = new ConstInitValAST();
    ast->type = 1;
    $$ = ast;
  }

ConstExp
  : Exp{
    $$ = $1;
  }
  ;

VarDef
  : IDENT{
    auto ast = new VarDefAST();
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->type = 0;
    $$ = ast;
  }  
  | IDENT '=' InitVal{
    auto ast = new VarDefAST();
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->type = 1;
    ast->initval = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ConstExpList{
    auto ast = new VarDefAST();
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->type = 2;
    ast->dim = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT ConstExpList '=' InitVal{
    auto ast = new VarDefAST();
    string temp = *unique_ptr<string>($1);
    ast->ident = temp;
    ast->type = 3;
    ast->dim = unique_ptr<BaseAST>($2);
    ast->initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp{
    auto ast = new InitValAST();
    ast->type = 0;
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | '{' InitValList '}'{
    $$ = $2;
  }
  ;

InitValList
  : InitVal{
    auto ast = new InitValAST();
    ast->type = 1;
    ast->vec.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | InitValList ',' InitVal{
    auto ast = $1;
    ast->vec.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | {
    auto ast = new InitValAST();
    ast->type = 1;
    $$ = ast;
  }
  ;
%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
