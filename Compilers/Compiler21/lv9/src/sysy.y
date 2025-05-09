%code requires {
    #include <memory>
    #include <string>
    #include "AST.hpp"
}

%{
#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"

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
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string> ?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
    std::string *str_val;
    int int_val;
    char char_val;
    BaseAST *ast_val;
    std::vector<std::unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT RELOP EQOP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal ConstExp BlockItem LVal
%type <ast_val> CompUnitItem FuncFParam
%type <ast_val> VarDecl VarDef InitVal
%type <int_val> Number
%type <char_val> UnaryOp AddOp MulOp
%type <vec_val> BlockItemVec ConstDefVec VarDefVec FuncFParamsVec CompUnitVec ExpVec
%type <vec_val> ConstExpVec ConstExpVec2 ExpVec0
%type <vec_val> ConstArrayDimVec ConstArrayInitVal ConstInitValVec ArrayInitVal ArrayInitValVec DimVec

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
    : CompUnitVec {
        auto comp_unit = make_unique<CompUnitAST>();
        vector<unique_ptr<BaseAST>> *vec = ($1);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            comp_unit->compUnitItemVec.push_back(move(*iter));
        }
        ast = move(comp_unit);
    }
    ;

CompUnitVec
    : CompUnitItem {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($1));
        $$ = vec;
    }
    | CompUnitVec CompUnitItem {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($2));
        $$ = vec;
    }
    ;

CompUnitItem
    : FuncDef {
        auto ast = new CompUnitItemType_1_AST();
        ast->funcDef = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | Decl {
        auto ast = new CompUnitItemType_2_AST();
        ast->decl = unique_ptr<BaseAST>($1);
        $$ = ast;
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
    : INT IDENT '(' ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = "int";
        ast->ident = *unique_ptr<string>($2);
        ast->block = unique_ptr<BaseAST>($5);
        $$ = ast;
    }
    | INT IDENT '(' FuncFParamsVec ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = "int";
        ast->ident = *unique_ptr<string>($2);
        vector<unique_ptr<BaseAST>> *vec = ($4);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->funcFParamsVec.push_back(move(*iter));
        }
        ast->block = unique_ptr<BaseAST>($6);
        $$ = ast;
    }
    | VOID IDENT '(' ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = "void";
        ast->ident = *unique_ptr<string>($2);
        ast->block = unique_ptr<BaseAST>($5);
        $$ = ast;
    }
    | VOID IDENT '(' FuncFParamsVec ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = "void";
        ast->ident = *unique_ptr<string>($2);
        vector<unique_ptr<BaseAST>> *vec = ($4);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->funcFParamsVec.push_back(move(*iter));
        }
        ast->block = unique_ptr<BaseAST>($6);
        $$ = ast;
    }
    ;

// 同上, 不再解释


FuncFParamsVec
    : FuncFParam {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($1));
        $$ = vec;
    }
    | FuncFParamsVec ',' FuncFParam {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

FuncFParam
    : INT IDENT {
        auto ast = new FuncFParamAST();
        ast->bType = "int";
        ast->ident = *unique_ptr<string>($2);
        ast->type = 1;
        $$ = ast;
    }
    | INT IDENT '[' ']' ConstArrayDimVec {
        auto ast = new FuncFParamAST();
        ast->bType = "int";
        ast->ident = *unique_ptr<string>($2);
        vector<unique_ptr<BaseAST>> *vec = ($5);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->constExpVec.push_back(move(*iter));
        }
        ast->type = 2;
        $$ = ast;
    }
    | INT IDENT '[' ']' {
        auto ast = new FuncFParamAST();
        ast->bType = "int";
        ast->ident = *unique_ptr<string>($2);
        ast->type = 2;
        $$ = ast;
    }
    ;

Block
    : '{' BlockItemVec '}' {
        auto ast = new BlockAST();
        vector<unique_ptr<BaseAST>> *vec = ($2);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->blockItemVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    ;

BlockItemVec
    : {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | BlockItemVec BlockItem {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($2));
        $$ = vec;
    }
    ;

BlockItem
    : Decl {
        auto ast = new BlockItemType_1_AST();
        ast->decl = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | Stmt {
        auto ast = new BlockItemType_2_AST();
        ast->stmt = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;

Decl
    : ConstDecl {
        auto ast = new DeclType_1_AST();
        ast->constDecl = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | VarDecl {
        auto ast = new DeclType_2_AST();
        ast->varDecl = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;

ConstDecl
    : CONST INT ConstDef ConstDefVec ';' {
        auto ast = new ConstDeclAST();
        ast->bType =  "int";
        ast->constDefVec.push_back(unique_ptr<BaseAST>($3));
        vector<unique_ptr<BaseAST>> *vec = ($4);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->constDefVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    ;

ConstDef
    : IDENT '=' ConstInitVal {
        auto ast = new ConstDefAST();
        ast->ident = *unique_ptr<string>($1);
        ast->constInitVal = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    | IDENT ConstArrayDimVec '=' ConstInitVal {
        auto ast = new ConstDefAST();
        ast->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *vec = ($2);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->constExpVec.push_back(move(*iter));
        }
        ast->constInitVal = unique_ptr<BaseAST>($4);
        $$ = ast;
    }
    ;

ConstArrayDimVec
    : '[' ConstExp ']' {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($2));
        $$ = vec;
    }
    | ConstArrayDimVec '[' ConstExp ']' {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }

ConstDefVec
    : {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | ConstDefVec ',' ConstDef {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

ConstInitVal
    : ConstExp {
        auto ast = new ConstInitValType_1_AST();
        ast->constExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | ConstArrayInitVal {
        auto ast = new ConstInitValType_2_AST();
        vector<unique_ptr<BaseAST>> *vec = ($1);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->constInitValVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    ;

ConstArrayInitVal
    : '{' '}' {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | '{' ConstInitValVec '}' {
        vector<unique_ptr<BaseAST>> *vec = ($2);
        $$ = vec;
    }
    ;

ConstInitValVec
    : ConstInitVal {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($1));
        $$ = vec;
    }
    | ConstInitValVec ',' ConstInitVal {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

ConstExpVec
    : {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | ConstExp ConstExpVec2 {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($1));
        vector<unique_ptr<BaseAST>> *vec2 = ($2);
        for(auto iter = vec2.begin(); iter != vec2->end(); iter++){
            vec->push_back(move(*iter));
        }
        $$ = vec;
    }
    ;

ConstExpVec2
    : {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | ConstExpVec2 ',' ConstExp {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

ConstExp
    : Exp {
        auto ast = new ConstExpAST();
        ast->exp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;

LVal
    : IDENT {
        auto ast = new LValAST();
        ast->ident = *unique_ptr<string>($1);
        $$ = ast;
    }
    | IDENT DimVec {
        auto ast = new LValAST();
        ast->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *vec = ($2);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->expVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    ;

DimVec
    : '[' Exp ']' {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($2));
        $$ = vec;
    }
    | DimVec '[' Exp ']' {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

VarDecl
    : INT VarDef VarDefVec ';' {
        auto ast = new VarDeclAST();
        ast->bType = "int";
        ast->varDefVec.push_back(unique_ptr<BaseAST>($2));
        vector<unique_ptr<BaseAST>> *vec = ($3);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->varDefVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    ;

VarDef
    : IDENT {
        auto ast = new VarDefType_1_AST();
        ast->ident = *unique_ptr<string>($1);
        $$ = ast;
    }
    | IDENT '=' InitVal {
        auto ast = new VarDefType_2_AST();
        ast->ident = *unique_ptr<string>($1);
        ast->initVal = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    | IDENT ConstArrayDimVec {
        auto ast = new VarDefType_1_AST();
        ast->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *vec = ($2);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->constExpVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    | IDENT ConstArrayDimVec '=' InitVal {
        auto ast = new VarDefType_2_AST();
        ast->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *vec = ($2);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->constExpVec.push_back(move(*iter));
        }
        ast->initVal = unique_ptr<BaseAST>($4);
        $$ = ast;
    }
    ;

VarDefVec
    : {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | VarDefVec ',' VarDef {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

InitVal
    : Exp {
        auto ast = new InitValType_1_AST();
        ast->exp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | ArrayInitVal {
        auto ast = new InitValType_2_AST();
        vector<unique_ptr<BaseAST>> *vec = ($1);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->initValVec.push_back(move(*iter));
        }
        $$ = ast;
    }
    ;

ArrayInitVal
    : '{' '}' {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | '{' ArrayInitValVec '}' {
        vector<unique_ptr<BaseAST>> *vec = ($2);
        $$ = vec;
    }
    ;

ArrayInitValVec
    : InitVal {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($1));
        $$ = vec;
    }
    | ArrayInitValVec ',' InitVal {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;

ExpVec0
    : {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        $$ = vec;
    }
    | ExpVec {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        $$ = vec;
    }

Stmt
    : RETURN Exp ';' {
        auto ast = new StmtType_1_AST();
        ast->exp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | LVal '=' Exp ';' {
        auto ast = new StmtType_2_AST();
        ast->lVal = unique_ptr<BaseAST>($1);
        ast->exp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    | RETURN ';' {
        auto ast = new StmtType_1_AST();
        $$ = ast;
    }
    | Exp ';' {
        auto ast = new StmtType_3_AST();
        ast->exp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | ';' {
        auto ast = new StmtType_3_AST();
        $$ = ast;
    }
    | Block {
        auto ast = new StmtType_4_AST();
        ast->block = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | IF '(' Exp ')' Stmt {
        auto ast = new StmtType_5_AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->ifStmt = unique_ptr<BaseAST>($5);
        $$ = ast;
    }
    | IF '(' Exp ')' Stmt ELSE Stmt {
        auto ast = new StmtType_5_AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->ifStmt = unique_ptr<BaseAST>($5);
        ast->elseStmt = unique_ptr<BaseAST>($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' Stmt {
        auto ast = new StmtType_6_AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->stmt = unique_ptr<BaseAST>($5);
        $$ = ast;    
    }
    | BREAK ';' {
        auto ast = new StmtType_7_AST();
        $$ = ast;
    }
    | CONTINUE ';' {
        auto ast = new StmtType_8_AST();
        $$ = ast;
    }
    ;

Number
    : INT_CONST {
        $$ = $1;
    }
    ;

Exp
    : LOrExp {
        auto ast = new ExpAST();
        ast->lOrExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;

MulExp
    : UnaryExp {
        auto ast = new MulExpType_1_AST();
        ast->unaryExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | MulExp MulOp UnaryExp {
        auto ast = new MulExpType_2_AST();
        ast->mulExp = unique_ptr<BaseAST>($1);
        ast->mulOp = $2;
        ast->unaryExp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

AddExp
    : MulExp {
        auto ast = new AddExpType_1_AST();
        ast->mulExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | AddExp AddOp MulExp {
        auto ast = new AddExpType_2_AST();
        ast->addExp = unique_ptr<BaseAST>($1);
        ast->addOp = $2;
        ast->mulExp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

PrimaryExp
    : '(' Exp ')' {
        auto ast = new PrimaryExpType_1_AST();
        ast->exp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | Number {
        auto ast = new PrimaryExpType_2_AST();
        ast->number = $1;
        $$ = ast;
    }
    | LVal {
        auto ast = new PrimaryExpType_3_AST();
        ast->lVal = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;

UnaryExp
    : PrimaryExp {
        auto ast = new UnaryExpType_1_AST();
        ast->primaryExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | UnaryOp UnaryExp {
        auto ast = new UnaryExpType_2_AST();
        ast->unaryOp = $1;
        ast->unaryExp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | IDENT '(' ')' {
        auto ast = new UnaryExpType_3_AST();
        ast->ident = *unique_ptr<string>($1);
        $$ = ast;
    }
    | IDENT '(' ExpVec ')' {
        auto ast = new UnaryExpType_3_AST();
        ast->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *vec = ($3);
        for(auto iter = vec->begin(); iter != vec->end(); iter++){
            ast->expVec.push_back(move(*iter));
        }
        $$ = ast;
    } 
    ;

ExpVec
    : Exp {
        vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
        vec->push_back(unique_ptr<BaseAST>($1));
        $$ = vec;
    }
    | ExpVec ',' Exp {
        vector<unique_ptr<BaseAST>> *vec = ($1);
        vec->push_back(unique_ptr<BaseAST>($3));
        $$ = vec;
    }
    ;


RelExp
    : AddExp {
        auto ast = new RelExpType_1_AST();
        ast->addExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | RelExp RELOP AddExp {
        auto ast = new RelExpType_2_AST();
        ast->relExp = unique_ptr<BaseAST>($1);
        ast->relOp = *unique_ptr<string>($2);
        ast->addExp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

EqExp
    : RelExp {
        auto ast = new EqExpType_1_AST();
        ast->relExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | EqExp EQOP RelExp {
        auto ast = new EqExpType_2_AST();
        ast->eqExp = unique_ptr<BaseAST>($1);
        ast->eqOp = *unique_ptr<string>($2);
        ast->relExp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

LAndExp
    : EqExp {
        auto ast = new LAndExpType_1_AST();
        ast->eqExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | LAndExp '&' '&' EqExp {
        auto ast = new LAndExpType_2_AST();
        ast->lAndExp = unique_ptr<BaseAST>($1);
        ast->eqExp = unique_ptr<BaseAST>($4);
        $$ = ast;
    }
    ;

LOrExp
    : LAndExp {
        auto ast = new LOrExpType_1_AST();
        ast->lAndExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | LOrExp '|' '|' LAndExp {
        auto ast = new LOrExpType_2_AST();
        ast->lOrExp = unique_ptr<BaseAST>($1);
        ast->lAndExp = unique_ptr<BaseAST>($4);
        $$ = ast;
    }
    ;

UnaryOp
    : '+' {
        $$ = '+';
    }
    | '-' {
        $$ = '-';
    }
    | '!' {
        $$ = '!';
    }
    ;

MulOp
    : '*' {
        $$ = '*';
    }
    | '/' {
        $$ = '/';
    }
    | '%' {
        $$ = '%';
    }
    ;

AddOp
    : '+' {
        $$ = '+';
    }
    | '-' {
        $$ = '-';
    }
    ;
%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}