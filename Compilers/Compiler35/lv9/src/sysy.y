%code requires {
#include <memory>
#include <string>
#include <vector>
#include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr ?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
    std::string *str_val;
    int int_val;
    BaseAST *ast_val;
    std::vector<BaseAST *> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN AND OR LEQ GEQ EQ NEQ CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt Exp PrimaryExp UnaryExp UnaryOp MulExp AddExp MulOp AddOp RelExp RelOp EqExp EqOp LAndExp LOrExp Decl ConstDecl ConstDef ConstInitVal ConstExp LVal BlockItem VarDecl VarDef InitVal MatchedStmt UnmatchedStmt FuncFParam GlobalItem ArrIdent
%type <int_val> Number BType
%type <vec_val> BlockItemList ConstDefList VarDefList GlobalItemList FuncFParamList FuncFParamListNE FuncRParamList FuncRParamListNE ConstInitList ConstInitListNE InitList InitListNE

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
    : GlobalItemList {
        auto compUnit = std::make_unique<CompUnit>();
        compUnit->items = convertToVector<GlobalItem>($1);
        delete $1;
        ast = std::move(compUnit);
    }
    ;

GlobalItemList
    : GlobalItem {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | GlobalItemList GlobalItem {
        auto vec = $1;
        vec->push_back($2);
        $$ = vec;
    }
    ;

GlobalItem
    : FuncDef {
        auto ast = new GlobalFuncDef();
        ast->funcDef = convertToUnique<FuncDef>($1);
        $$ = ast;
    }
    | Decl {
        auto ast = new GlobalDecl();
        ast->decl = convertToUnique<Decl>($1);
        $$ = ast;
    }
    ;

FuncDef
    : BType IDENT '(' FuncFParamList ')' Block {
        auto ast = new FuncDef();
        ast->funcType = $1;
        ast->ident = *$2;
        delete $2;
        ast->funcFParams = convertToVector<FuncFParam>($4);
        delete $4;
        ast->block = convertToUnique<Block>($6);
        $$ = ast;
    }
    ;

FuncFParamList
    : {
        $$ = new std::vector<BaseAST *>();
    }
    | FuncFParamListNE {
        $$ = $1;
    }
    ;

FuncFParamListNE
    : FuncFParam {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | FuncFParamListNE ',' FuncFParam {
        auto vec = $1;
        vec->push_back($3);
        $$ = $1;
    }
    ;

FuncFParam
    : BType ArrIdent {
        auto ast = new FuncFParam();
        ast->arrIdent = convertToUnique<ArrIdent>($2);
        $$ = ast;
    }

Block
    : '{' BlockItemList '}' {
        auto ast = new Block();
        ast->blockItems = convertToVector<BlockItem>($2);
        delete $2;
        $$ = ast;
    }
    ;

BlockItemList
    : {
        $$ = new std::vector<BaseAST *>();
    }
    | BlockItemList BlockItem {
        auto vec = $1;
        vec->push_back($2);
        $$ = vec;
    }
    ;

Stmt
    : MatchedStmt {
        $$ = $1;
    }
    | UnmatchedStmt {
        $$ = $1;
    }
    ;

UnmatchedStmt
    : IF '(' Exp ')' Stmt {
        auto ast = new StmtIf();
        ast->exp = convertToUnique<Exp>($3);
        ast->stmtIf = convertToUnique<Stmt>($5);
        ast->stmtElse = nullptr;
        $$ = ast;
    }
    | IF '(' Exp ')' MatchedStmt ELSE UnmatchedStmt {
        auto ast = new StmtIf();
        ast->exp = convertToUnique<Exp>($3);
        ast->stmtIf = convertToUnique<Stmt>($5);
        ast->stmtElse = convertToUnique<Stmt>($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' UnmatchedStmt {
        auto ast = new StmtWhile();
        ast->exp = convertToUnique<Exp>($3);
        ast->stmt = convertToUnique<Stmt>($5);
        $$ = ast;
    }
    ;

MatchedStmt
    : LVal '=' Exp ';' {
        auto ast = new StmtAssign();
        ast->lVal = convertToUnique<LVal>($1);
        ast->exp = convertToUnique<Exp>($3);
        $$ = ast;
    }
    | ';' {
        auto ast = new StmtExp();
        ast->exp = nullptr;
        $$ = ast;
    }
    | Exp ';' {
        auto ast = new StmtExp();
        ast->exp = convertToUnique<Exp>($1);
        $$ = ast;
    }
    | Block {
        auto ast = new StmtBlock();
        ast->block = convertToUnique<Block>($1);
        $$ = ast;
    }
    | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
        auto ast = new StmtIf();
        ast->exp = convertToUnique<Exp>($3);
        ast->stmtIf = convertToUnique<Stmt>($5);
        ast->stmtElse = convertToUnique<Stmt>($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' MatchedStmt {
        auto ast = new StmtWhile();
        ast->exp = convertToUnique<Exp>($3);
        ast->stmt = convertToUnique<Stmt>($5);
        $$ = ast;
    }
    | BREAK ';' {
        $$ = new StmtBreak();
    }
    | CONTINUE ';' {
        $$ = new StmtContinue();
    }
    | RETURN ';' {
        auto ast = new StmtReturn();
        ast->exp = nullptr;
        $$ = ast;
    }
    | RETURN Exp ';' {
        auto ast = new StmtReturn();
        ast->exp = convertToUnique<Exp>($2);
        $$ = ast;
    }
    ;

Exp
    : LOrExp {
        auto ast = new Exp();
        ast->lOrExp = convertToUnique<LOrExp>($1);
        $$ = ast;
    }

PrimaryExp
    : '(' Exp ')' {
        auto ast = new PrimaryExpParentheses();
        ast->exp = convertToUnique<Exp>($2);
        $$ = ast;
    }
    | Number {
        auto ast = new PrimaryExpNumber();
        ast->number = $1;
        $$ = ast;
    }
    | LVal {
        auto ast = new PrimaryExpLVal();
        ast->lVal = convertToUnique<LVal>($1);
        $$ = ast;
    }
    ;

UnaryExp
    : PrimaryExp {
        auto ast = new UnaryExpPrimary();
        ast->primaryExp = convertToUnique<PrimaryExp>($1);
        $$ = ast;
    }
    | UnaryOp UnaryExp {
        auto ast = new UnaryExpUnaryOp();
        ast->unaryOp = convertToUnique<UnaryOp>($1);
        ast->unaryExp = convertToUnique<UnaryExp>($2);
        $$ = ast;
    }
    | IDENT '(' FuncRParamList ')' {
        auto ast = new UnaryExpCall();
        ast->ident = *$1;
        delete $1;
        ast->args = convertToVector<Exp>($3);
        delete $3;
        $$ = ast;
    }
    ;

FuncRParamList
    : {
        $$ = new std::vector<BaseAST *>();
    }
    | FuncRParamListNE {
        $$ = $1;
    }
    ;

FuncRParamListNE
    : Exp {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | FuncRParamListNE ',' Exp {
        auto vec = $1;
        vec->push_back($3);
        $$ = vec;
    }
    ;

UnaryOp
    : '+' {
        $$ = new UnaryOpPos();
    }
    | '-' {
        $$ = new UnaryOpNeg();
    }
    | '!' {
        $$ = new UnaryOpNot();
    }
    ;

Number
    : INT_CONST {
        $$ = $1;
    }
    ;

MulExp
    : UnaryExp {
        auto ast = new MulExpUnaryExp();
        ast->unaryExp = convertToUnique<UnaryExp>($1);
        $$ = ast;
    }
    | MulExp MulOp UnaryExp {
        auto ast = new MulExpMulOp();
        ast->mulExp = convertToUnique<MulExp>($1);
        ast->mulOp = convertToUnique<MulOp>($2);
        ast->unaryExp = convertToUnique<UnaryExp>($3);
        $$ = ast;
    }
    ;

MulOp
    : '*' {
        $$ = new MulOpMul();
    }
    | '/' {
        $$ = new MulOpDiv();
    }
    | '%' {
        $$ = new MulOpMod();
    }
    ;

AddExp
    : MulExp {
        auto ast = new AddExpMulExp();
        ast->mulExp = convertToUnique<MulExp>($1);
        $$ = ast;
    }
    | AddExp AddOp MulExp {
        auto ast = new AddExpAddOp();
        ast->addExp = convertToUnique<AddExp>($1);
        ast->addOp = convertToUnique<AddOp>($2);
        ast->mulExp = convertToUnique<MulExp>($3);
        $$ = ast;
    }
    ;

AddOp
    : '+' {
        $$ = new AddOpAdd();
    }
    | '-' {
        $$ = new AddOpSub();
    }
    ;

RelExp
    : AddExp {
        auto ast = new RelExpAddExp();
        ast->addExp = convertToUnique<AddExp>($1);
        $$ = ast;
    }
    | RelExp RelOp AddExp {
        auto ast = new RelExpRelOp();
        ast->relExp = convertToUnique<RelExp>($1);
        ast->relOp = convertToUnique<RelOp>($2);
        ast->addExp = convertToUnique<AddExp>($3);
        $$ = ast;
    }
    ;

RelOp
    : '<' {
        $$ = new RelOpLt();
    }
    | '>' {
        $$ = new RelOpGt();
    }
    | LEQ {
        $$ = new RelOpLeq();
    }
    | GEQ {
        $$ = new RelOpGeq();
    }
    ;

EqExp
    : RelExp {
        auto ast = new EqExpRelExp();
        ast->relExp = convertToUnique<RelExp>($1);
        $$ = ast;
    }
    | EqExp EqOp RelExp {
        auto ast = new EqExpEqOp();
        ast->eqExp = convertToUnique<EqExp>($1);
        ast->eqOp = convertToUnique<EqOp>($2);
        ast->relExp = convertToUnique<RelExp>($3);
        $$ = ast;
    }
    ;

EqOp
    : EQ {
        $$ = new EqOpEq();
    }
    | NEQ {
        $$ = new EqOpNeq();
    }
    ;

LAndExp
    : EqExp {
        auto ast = new LAndExpEqExp();
        ast->eqExp = convertToUnique<EqExp>($1);
        $$ = ast;
    }
    | LAndExp AND EqExp {
        auto ast = new LAndExpAnd();
        ast->lAndExp = convertToUnique<LAndExp>($1);
        ast->eqExp = convertToUnique<EqExp>($3);
        $$ = ast;
    }
    ;

LOrExp
    : LAndExp {
        auto ast = new LOrExpLAndExp();
        ast->lAndExp = convertToUnique<LAndExp>($1);
        $$ = ast;
    }
    | LOrExp OR LAndExp {
        auto ast = new LOrExpOr();
        ast->lOrExp = convertToUnique<LOrExp>($1);
        ast->lAndExp = convertToUnique<LAndExp>($3);
        $$ = ast;
    }
    ;

Decl
    : ConstDecl {
        auto ast = new DeclConstDecl();
        ast->constDecl = convertToUnique<ConstDecl>($1);
        $$ = ast;
    }
    | VarDecl {
        auto ast = new DeclVarDecl();
        ast->varDecl = convertToUnique<VarDecl>($1);
        $$ = ast;
    }
    ;

ConstDecl
    : CONST BType ConstDefList ';' {
        auto ast = new ConstDecl();
        ast->constDefs = convertToVector<ConstDef>($3);
        $$ = ast;
    }
    ;

ConstDefList
    : ConstDef {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | ConstDefList ',' ConstDef {
        auto vec = $1;
        vec->push_back($3);
        $$ = vec;
    }
    ;
    
BType
    : INT {
        $$ = 1;
    }
    | VOID {
        $$ = 0;
    }
    ;

ConstDef
    : ArrIdent '=' ConstInitVal {
        auto ast = new ConstDef();
        ast->arrIdent = convertToUnique<ArrIdent>($1);
        ast->constInitVal = convertToUnique<ConstInitVal>($3);
        $$ = ast;
    }
    ;

ConstInitVal
    : ConstExp {
        auto ast = new ConstInitValExp();
        ast->constExp = convertToUnique<ConstExp>($1);
        $$ = ast;
    }
    | '{' ConstInitList '}' {
        auto ast = new ConstInitList();
        ast->list = convertToVector<ConstInitVal>($2);
        delete $2;
        $$ = ast;
    }
    ;

ConstInitList
    : {
        $$ = new std::vector<BaseAST *>();
    }
    | ConstInitListNE {
        $$ = $1;
    }
    ;

ConstInitListNE
    : ConstInitVal {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | ConstInitListNE ',' ConstInitVal {
        auto vec = $1;
        vec->push_back($3);
        $$ = vec;
    }
    ;

BlockItem
    : Decl {
        auto ast = new BlockItemDecl();
        ast->decl = convertToUnique<Decl>($1);
        $$ = ast;
    }
    | Stmt {
        auto ast = new BlockItemStmt();
        ast->stmt = convertToUnique<Stmt>($1);
        $$ = ast;
    }
    ;

LVal
    : ArrIdent {
        auto ast = new LVal();
        ast->arrIdent = convertToUnique<ArrIdent>($1);
        $$ = ast;
    }
    ;

ConstExp
    : Exp {
        auto ast = new ConstExp();
        ast->exp = convertToUnique<Exp>($1);
        $$ = ast;
    }
    ;

VarDecl
    : BType VarDefList ';' {
        auto ast = new VarDecl();
        ast->varDefs = convertToVector<VarDef>($2);
        $$ = ast;
    }
    ;

VarDefList
    : VarDef {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | VarDefList ',' VarDef {
        auto vec = $1;
        vec->push_back($3);
        $$ = vec;
    }
    ;

VarDef
    : ArrIdent {
        auto ast = new VarDef();
        ast->arrIdent = convertToUnique<ArrIdent>($1);
        ast->initVal = nullptr;
        $$ = ast;
    }
    | ArrIdent '=' InitVal {
        auto ast = new VarDef();
        ast->arrIdent = convertToUnique<ArrIdent>($1);
        ast->initVal = convertToUnique<InitVal>($3);
        $$ = ast;
    }
    ;

InitVal
    : Exp {
        auto ast = new InitValExp();
        ast->exp = convertToUnique<Exp>($1);
        $$ = ast;
    }
    | '{' InitList '}' {
        auto ast = new InitList();
        ast->list = convertToVector<InitVal>($2);
        delete $2;
        $$ = ast;
    }
    ;

InitList
    : {
        $$ = new std::vector<BaseAST *>();
    }
    | InitListNE {
        $$ = $1;
    }
    ;

InitListNE
    : InitVal {
        $$ = new std::vector<BaseAST *> {$1};
    }
    | InitListNE ',' InitVal {
        auto vec = $1;
        vec->push_back($3);
        $$ = vec;
    }
    ;

ArrIdent
    : IDENT {
        auto ast = new ArrIdent();
        ast->ident = *$1;
        delete $1;
        $$ = ast;
    }
    | ArrIdent '[' ']' {
        auto ast = $1;
        ((ArrIdent *) ast)->dimExps.push_back(nullptr);
        $$ = ast;
    }
    | ArrIdent '[' Exp ']' {
        auto ast = $1;
        ((ArrIdent *) ast)->dimExps.push_back(convertToUnique<Exp>($3));
        $$ = ast;
    }

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
    std::cerr << "error: " << s << std::endl;
}