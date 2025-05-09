%code requires {
  #include <memory>
  #include <string>
  #include <vector>
  #include <cassert>
  #include "all_ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

#include "all_ast.hpp"


// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%define parse.error verbose


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
  std::vector<std::unique_ptr<BaseAST> > *vector_val;
}


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN LE GE NE EQ LAND LOR CONST IF ELSE WHILE
%token <str_val> IDENT
%token <int_val> INT_CONST

%token BREAK CONTINUE VOID


// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt Number Expr PrimaryExpr UnaryExpr UnaryOp AddExpr MulExpr
%type <ast_val> RelExpr EqExpr LAndExpr LOrExpr
%type <ast_val> VarDecl VarDef InitVal OpenWhileStmt MatchWhileStmt

%type <str_val> BType

%type <ast_val> Decl ConstDecl ConstDef ConstInitVal BlockItem LVal ConstExpr GlobalUnit
%type <vector_val> ConstDefList BlockItemList VarDefList GlobalUnitList FuncCallOneParamList FuncDefOneParamList

%type <ast_val> NoIfStmt OpenIfStmt MatchIfStmt
%type <ast_val> FuncDefManyParams FuncDefOneParam 
%type <ast_val> FuncCall FuncCallOneParam 

// %type <vector_val> ConstExprList ExprList

%type <vector_val> ConstInitValList InitValList ConstExprArrayList ExprArrayList



%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : GlobalUnit GlobalUnitList {
    auto comp_unit = make_unique<CompUnitAST>();
    ($2) -> push_back(unique_ptr<BaseAST> ($1));
    comp_unit -> global_units = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast = move(comp_unit);
  }
  ;

GlobalUnit
  : FuncDef {
    auto ast = new GlobalUnitAST();
    ast->type = GlobalUnitAST::FUNCDEF;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new GlobalUnitAST();
    ast->type = GlobalUnitAST::DECL;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
  

GlobalUnitList
  : GlobalUnit GlobalUnitList {
    ($2)->push_back(unique_ptr<BaseAST>($1));
    $$ = $2;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> >();
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
  : FuncType IDENT '(' FuncDefOneParam FuncDefManyParams ')' Block {
    auto ast = new FuncDefAST();
    ast-> func_type = unique_ptr<BaseAST>($1);
    ast-> ident = unique_ptr<string>($2);
    auto tmp = dynamic_cast<FuncDefManyParamsAST*>($5);
    assert(tmp != nullptr);
    tmp-> func_def_params->push_back(unique_ptr<BaseAST> ($4));
    ast-> func_params = unique_ptr<BaseAST> ($5);
    ast -> block = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast-> func_type = unique_ptr<BaseAST>($1);
    ast-> ident = unique_ptr<string>($2);
    ast-> func_params = unique_ptr<BaseAST>(new FuncDefManyParamsAST());
    ast -> block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->val = unique_ptr<string>(new string("int"));
    $$ = ast;
  }
  | VOID {
    auto ast = new FuncTypeAST();
    ast->val = unique_ptr<string>(new string("void"));
    $$ = ast;    
  }
  ;

FuncDefManyParams
  : FuncDefOneParamList {
    auto ast = new FuncDefManyParamsAST();
    ast->func_def_params = unique_ptr<vector<unique_ptr<BaseAST> > >($1);
    $$ = ast;
  }
  ;

FuncDefOneParam
  : FuncType IDENT {
    auto ast = new FuncDefOneParamAST();
    ast->btype="int";
    ast->ident = *($2);
    $$ = ast;
  }
  | FuncType IDENT '[' ']' {
    auto ast = new FuncDefOneParamAST();
    ast->btype="int";
    ast->ident = *($2);
    ast->opt_array_exprs = unique_ptr<vector<unique_ptr<BaseAST> > >(new vector<unique_ptr<BaseAST> >());
    $$ = ast;    
  }
  | FuncType IDENT '[' ']' ConstExprArrayList {
    auto ast = new FuncDefOneParamAST();
    ast->btype="int";
    ast->ident = *($2);
    ast->opt_array_exprs = unique_ptr<vector<unique_ptr<BaseAST> > >($5);
    $$ = ast;    
  }
  ;

FuncDefOneParamList
  : ',' FuncDefOneParam FuncDefOneParamList {
    ($3) -> push_back(unique_ptr<BaseAST>($2));
    $$ = $3;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> > ();
  }
  ;

FuncCall
  : IDENT '(' ')' {
    auto ast = new FuncCallAST();
    ast->ident = *($1);
    ast->func_call_params = unique_ptr<vector<unique_ptr<BaseAST> > >(new vector<unique_ptr<BaseAST> >());
    $$ = ast;
  }
  | IDENT '(' FuncCallOneParam FuncCallOneParamList ')' {
    ($4)->push_back(unique_ptr<BaseAST>($3));
    auto ast = new FuncCallAST();
    ast->ident = *($1);
    ast->func_call_params = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    $$ = ast;
  }
  ;

FuncCallOneParam
  : Expr {
    auto ast = new FuncCallOneParamAST();
    ast->expr = unique_ptr<BaseAST> ($1);
    $$ = ast;
  }
  ;

FuncCallOneParamList
  : ',' FuncCallOneParam FuncCallOneParamList {
    ($3)->push_back(unique_ptr<BaseAST>($2));
    $$ = $3;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> > ();
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_items = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast-> type = BlockItemAST::DECL;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast-> type = BlockItemAST::STMT;
    ast->content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

NoIfStmt
  : RETURN Expr ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::RETURN;
    ast -> content = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::RETURN;
    $$ = ast;
  }
  | LVal '=' Expr ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::ASSIGN;
    ast -> opt_lval = unique_ptr<BaseAST>($1);
    ast -> content = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = StmtAST::BLOCK;
    ast -> content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Expr ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::EXPR;
    ast -> content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::EMPTY;
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::CONTINUE;
    $$ = ast;
  }
  ;

OpenWhileStmt
  : WHILE '(' Expr ')' OpenIfStmt {
    auto ast = new WhileStmtAST();
    ast->expr = unique_ptr<BaseAST>($3);
    ast->body = unique_ptr<BaseAST>($5);
    
    auto ans = new StmtAST();
    ans->type = StmtAST::WHILE;
    ans->content = unique_ptr<BaseAST>(ast);

    $$ = ans;
  }
  ;

MatchWhileStmt
  : WHILE '(' Expr ')' MatchIfStmt {
    auto ast = new WhileStmtAST();
    ast->expr = unique_ptr<BaseAST>($3);
    ast->body = unique_ptr<BaseAST>($5);
    
    auto ans = new StmtAST();
    ans->type = StmtAST::WHILE;
    ans->content = unique_ptr<BaseAST>(ast);

    $$ = ans;
  }
  ;



Stmt
  : MatchIfStmt {
    $$ = $1;
  }
  | OpenIfStmt {
    $$ = $1;
  }
  ;

MatchIfStmt
  : IF '(' Expr ')' MatchIfStmt ELSE MatchIfStmt {
    auto ast = new IfStmtAST();
    ast->type = IfStmtAST::IFELSE;
    ast->expr = unique_ptr<BaseAST> ($3);
    ast->then = unique_ptr<BaseAST> ($5);
    ast->opt_else = unique_ptr<BaseAST>($7);

    auto ans = new StmtAST();
    ans->type = StmtAST::IF;
    ans->content = unique_ptr<BaseAST>(ast);
    $$ = ans;
  }
  | MatchWhileStmt {
    $$ = $1;
  }
  | NoIfStmt {
    $$ = $1;
  }

OpenIfStmt
  : IF '(' Expr ')' MatchIfStmt {
    auto ast = new IfStmtAST();
    ast -> type = IfStmtAST::ONLYIF;
    ast -> expr = unique_ptr<BaseAST> ($3);
    ast -> then = unique_ptr<BaseAST> ($5);

    auto ans = new StmtAST();
    ans->type = StmtAST::IF;
    ans->content = unique_ptr<BaseAST>(ast);
    $$ = ans;
  }
  | IF '(' Expr ')' OpenIfStmt {
    auto ast = new IfStmtAST();
    ast -> type = IfStmtAST::ONLYIF;
    ast -> expr = unique_ptr<BaseAST> ($3);
    ast -> then = unique_ptr<BaseAST> ($5);

    auto ans = new StmtAST();
    ans->type = StmtAST::IF;
    ans->content = unique_ptr<BaseAST>(ast);
    $$ = ans;
  }
  | IF '(' Expr ')' MatchIfStmt ELSE OpenIfStmt {
    auto ast = new IfStmtAST();
    ast -> type = IfStmtAST::IFELSE;
    ast -> expr = unique_ptr<BaseAST> ($3);
    ast -> then = unique_ptr<BaseAST> ($5);
    ast -> opt_else = unique_ptr<BaseAST> ($7);

    auto ans = new StmtAST();
    ans->type = StmtAST::IF;
    ans->content = unique_ptr<BaseAST>(ast);
    $$ = ans;
  }
  | OpenWhileStmt {
    $$ = $1;
  }
  ;

Expr
  : LOrExpr {
    auto ast = new ExprAST();
    ast -> or_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExpr
  : PrimaryExpr {
    auto ast = new UnaryExprAST();
    ast->type = UnaryExprAST::PRIMARY;
    auto content = new UnaryExprAST::InnerPrimaryAST();
    content-> primary_expr = unique_ptr<BaseAST>($1);
    ast->content = unique_ptr<BaseAST>(content);
    $$ = ast;
  }
  | UnaryOp UnaryExpr {
    auto ast = new UnaryExprAST();
    ast->type = UnaryExprAST::UNARY;
    auto content = new UnaryExprAST::InnerUnaryAST();
    content -> unary_op = unique_ptr<BaseAST>($1);
    content -> unary_expr = unique_ptr<BaseAST>($2);
    ast->content = unique_ptr<BaseAST>(content);
    $$ = ast;
  }
  | FuncCall {
    auto ast = new UnaryExprAST();
    ast -> type = UnaryExprAST::FUNC;
    ast -> content = unique_ptr<BaseAST> ($1);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    auto ast = new UnaryOpAST();
    ast -> op = UnaryOpAST::ADD;
    $$ = ast;
  }
  | '-' {
    auto ast = new UnaryOpAST();
    ast -> op = UnaryOpAST::DEC;
    $$ = ast;
  }
  | '!' {
    auto ast = new UnaryOpAST();
    ast -> op = UnaryOpAST::TAN;
    $$ = ast;
  }
  ;

PrimaryExpr
  : '(' Expr ')' {
    auto ast = new PrimaryExprAST();
    ast -> type = PrimaryExprAST::EXPR;
    ast -> content = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExprAST();
    ast -> type = PrimaryExprAST::NUMBER;
    ast -> content = unique_ptr<BaseAST>($1);
    $$ = ast;    
  }
  | LVal {
    auto ast = new PrimaryExprAST();
    ast -> type = PrimaryExprAST::LVAL;
    ast -> content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast -> val = $1;
    $$ =ast;
  }
  ;

AddExpr
  : MulExpr {
    auto ast = new AddExprAST();
    ast -> type = AddExprAST::MUL;
    ast -> mul_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExpr '+' MulExpr {
    auto ast = new AddExprAST();
    ast -> type = AddExprAST::MULADD;
    ast -> opt_op = AddExprAST::ADD;
    ast -> opt_add_expr = unique_ptr<BaseAST>($1);
    ast -> mul_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | AddExpr '-' MulExpr {
    auto ast = new AddExprAST();
    ast -> type = AddExprAST::MULADD;
    ast -> opt_op = AddExprAST::DEC;
    ast -> opt_add_expr = unique_ptr<BaseAST>($1);
    ast -> mul_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  ;

MulExpr
  : UnaryExpr {
    auto ast = new MulExprAST();
    ast -> type = MulExprAST::UNARY;
    ast -> unary_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExpr '*' UnaryExpr {
    auto ast = new MulExprAST();
    ast -> type = MulExprAST::MULUNARY;
    ast -> opt_op = MulExprAST::MUL;
    ast -> opt_mul_expr = unique_ptr<BaseAST>($1);
    ast -> unary_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | MulExpr '/' UnaryExpr {
    auto ast = new MulExprAST();
    ast -> type = MulExprAST::MULUNARY;
    ast -> opt_op = MulExprAST::DIV;
    ast -> opt_mul_expr = unique_ptr<BaseAST>($1);
    ast -> unary_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | MulExpr '%' UnaryExpr {
    auto ast = new MulExprAST();
    ast -> type = MulExprAST::MULUNARY;
    ast -> opt_op = MulExprAST::MOD;
    ast -> opt_mul_expr = unique_ptr<BaseAST>($1);
    ast -> unary_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  ;

RelExpr
  : AddExpr {
    auto ast = new RelExprAST();
    ast -> type = RelExprAST::ADD;
    ast -> add_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExpr '<' AddExpr {
    auto ast = new RelExprAST();
    ast -> type = RelExprAST::RELADD;
    ast -> opt_op = RelExprAST::LT;
    ast -> opt_rel_expr = unique_ptr<BaseAST>($1);
    ast -> add_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | RelExpr '>' AddExpr {
    auto ast = new RelExprAST();
    ast -> type = RelExprAST::RELADD;
    ast -> opt_op = RelExprAST::GT;
    ast -> opt_rel_expr = unique_ptr<BaseAST>($1);
    ast -> add_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | RelExpr LE AddExpr {
    auto ast = new RelExprAST();
    ast -> type = RelExprAST::RELADD;
    ast -> opt_op = RelExprAST::LE;
    ast -> opt_rel_expr = unique_ptr<BaseAST>($1);
    ast -> add_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | RelExpr GE AddExpr {
    auto ast = new RelExprAST();
    ast -> type = RelExprAST::RELADD;
    ast -> opt_op = RelExprAST::GE;
    ast -> opt_rel_expr = unique_ptr<BaseAST>($1);
    ast -> add_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  ;

EqExpr
  : RelExpr {
    auto ast = new EqExprAST();
    ast -> type = EqExprAST::REL;
    ast -> rel_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExpr EQ RelExpr {
    auto ast = new EqExprAST();
    ast -> type = EqExprAST::EQREL;
    ast -> opt_op = EqExprAST::EQ;
    ast -> opt_eq_expr = unique_ptr<BaseAST>($1);
    ast -> rel_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  | EqExpr NE RelExpr {
    auto ast = new EqExprAST();
    ast -> type = EqExprAST::EQREL;
    ast -> opt_op = EqExprAST::NE;
    ast -> opt_eq_expr = unique_ptr<BaseAST>($1);
    ast -> rel_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  ;

LAndExpr
  : EqExpr {
    auto ast = new LAndExprAST();
    ast -> type = LAndExprAST::EQ;
    ast -> eq_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExpr LAND EqExpr {
    auto ast = new LAndExprAST();
    ast -> type = LAndExprAST::ANDEQ;
    ast -> opt_and_expr = unique_ptr<BaseAST>($1);
    ast -> eq_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  ;

LOrExpr
  : LAndExpr {
    auto ast = new LOrExprAST();
    ast -> type = LOrExprAST::AND;
    ast -> and_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExpr LOR LAndExpr {
    auto ast = new LOrExprAST();
    ast -> type = LOrExprAST::ORAND;
    ast -> opt_or_expr = unique_ptr<BaseAST>($1);
    ast -> and_expr = unique_ptr<BaseAST>($3);
    $$ = ast;    
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast -> type = DeclAST::CONST;
    ast -> content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast -> type = DeclAST::VAR;
    ast -> content = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST FuncType ConstDef ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast -> const_defs = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    ast -> const_defs -> push_back(unique_ptr<BaseAST>($3));
    ast -> btype = "int";
    $$ = ast;
  }
  ;

ConstDefList
  : ',' ConstDef ConstDefList{
    ($3) -> push_back(unique_ptr<BaseAST>($2));
    $$ = $3;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> >();
  }
  ;

VarDecl
// warning: this could be wrong
  : FuncType VarDef VarDefList ';' {
    auto ast = new VarDeclAST();
    ast -> btype = "int";
    ast -> var_defs = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    ast -> var_defs -> push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  ;

VarDefList
  : ',' VarDef VarDefList {
    ($3) -> push_back(unique_ptr<BaseAST>($2));
    $$ = $3;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> >();
  }
  ;

VarDef
  : IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *($1);
    ast->type = VarDefAST::INIT;
    ast->opt_initval = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT {
    auto ast = new VarDefAST();
    ast->ident = *($1);
    ast->type = VarDefAST::NOINI;
    $$ = ast;
  }
  | IDENT ConstExprArrayList '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *($1);
    ast->type = VarDefAST::INIT;
    ast->opt_array_exprs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast->opt_initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | IDENT ConstExprArrayList {
    auto ast = new VarDefAST();
    ast->ident = *($1);
    ast->type = VarDefAST::NOINI;
    ast->opt_array_exprs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

InitVal
  : Expr {
    auto ast = new InitValAST();
    ast->opt_expr = unique_ptr<BaseAST>($1);
    $$=ast;
  }
  | '{' InitVal InitValList '}' {
    ($3)->push_back(unique_ptr<BaseAST>($2));
    auto ast = new InitValAST();
    ast->opt_initvals = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$=ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->opt_initvals = unique_ptr<vector<unique_ptr<BaseAST> > >(new vector<unique_ptr<BaseAST> > ());
    $$=ast;
  }
  ;

InitValList
  : ',' InitVal InitValList {
    ($3)->push_back(unique_ptr<BaseAST> ($2));
    $$ = $3;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> >();
  }
  ;



BlockItemList
  : BlockItem BlockItemList {
    ($2) -> push_back(unique_ptr<BaseAST>($1));
    $$ = $2;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> >();
  }
  ;

BType
  : INT {
    $$ = new string("int");
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast -> ident = *($1);
    ast -> const_initval = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ConstExprArrayList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast -> ident = *($1);
    ast->opt_array_exprs = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast -> const_initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstExprArrayList
  : '[' ConstExpr ']' ConstExprArrayList {
    ($4)->push_back(unique_ptr<BaseAST>($2));
    $$ = $4;
  }
  | '[' ConstExpr ']' {
    auto tmp = new vector<unique_ptr<BaseAST> >();
    tmp ->push_back(unique_ptr<BaseAST>($2));
    $$ = tmp;
  }

ConstInitVal
  : ConstExpr {
    auto ast = new ConstInitValAST();
    ast -> opt_const_expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitVal ConstInitValList '}' {
    ($3)->push_back(unique_ptr<BaseAST>($2));
    auto ast = new ConstInitValAST();
    ast -> opt_const_initvals = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast -> opt_const_initvals = unique_ptr<vector<unique_ptr<BaseAST> > >(new vector<unique_ptr<BaseAST> > ());
    $$ = ast;
  }
  ;

ConstInitValList
  : ',' ConstInitVal ConstInitValList {
    ($3)->push_back(unique_ptr<BaseAST> ($2));
    $$ = $3;
  }
  | {
    $$ = new vector<unique_ptr<BaseAST> >();
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *($1);
    $$ = ast;
  }
  | IDENT ExprArrayList {
    auto ast = new LValAST();
    ast->ident = *($1);
    ast->opt_array_exprs = unique_ptr<vector<unique_ptr<BaseAST> > > ($2);
    $$ = ast;
  }
  ;

ExprArrayList
  : '[' Expr ']' ExprArrayList {
    ($4)->push_back(unique_ptr<BaseAST> ($2));
    $$ = $4;
  }
  | '[' Expr ']' {
    auto tmp = new vector<unique_ptr<BaseAST> >();
    tmp->push_back(unique_ptr<BaseAST> ($2));
    $$ = tmp;
  }



ConstExpr
  : Expr {
    auto ast = new ConstExprAST();
    ast->expr = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// ConstExprList
//   : ',' ConstExpr ConstExprList {
//     ($3)->push_back(unique_ptr<BaseAST>($2));
//     $$ =$3;
//   }
//   | {
//     $$ = new vector<unique_ptr<BaseAST> >();
//   }
//   ;

// ExprList
//   : ',' Expr ExprList {
//     ($3)->push_back(unique_ptr<BaseAST>($2));
//     $$ =$3;
//   }
//   | {
//     $$ = new vector<unique_ptr<BaseAST> >();
//   }
//   ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
