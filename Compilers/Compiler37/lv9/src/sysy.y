%code requires {
  #include <memory>
  #include <string>
}
//todo head def
%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}
//todo up we def yyerror

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成BaseAST的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的BaseAST
//todo here we set param type BaseAST*
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
//todo here we add BaseAST
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  char char_val;
  std::vector<std::unique_ptr<BaseAST> > *vec_val;
  BlockItemAST *bitem_val;
  std::vector<std::unique_ptr<BlockItemAST> > *block_val;
  FuncParamTypeAST* funcp_val;
  FuncRParamAST* funcr_val;
}


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token VOID INT RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT RELOP EQOP LAND LOR
%token <int_val> INT_CONST

// 非终结符的类型定义
//todo here we change typedef in union
%type <ast_val> Exp PrimaryExp Number UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp BType ConstDef LVal VarDef InitVal FuncFParam CompUnitItem IntCompUnitItem IntFuncDef VoidFuncDef ArrayExp
%type <char_val> UnaryOp AddOp MulOp
%type <vec_val> ConstDefs VarDefs CompUnitItems InitVals ConstIndexs
%type <bitem_val> BlockItem Decl Stmt ConstDecl VarDecl Block 
%type <block_val> BlockItems 
%type <funcp_val> FuncParamType FuncFParams
%type <funcr_val> FuncRParams FuncRParamList

//if else 优先级声明
%precedence IFX
%precedence ELSE
%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : CompUnitItems {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->itemlist = unique_ptr<vector<unique_ptr<BaseAST> > >($1);
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
CompUnitItems
  : CompUnitItem {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CompUnitItems CompUnitItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

//CompUnit ::= [CompUnit] (Decl | FuncDef);
CompUnitItem
  : INT IntCompUnitItem {
    auto ast=new CompItemAST();
    ast->item = unique_ptr<BaseAST>($2);
    $$=ast;
  }
  | VOID VoidFuncDef {
    auto ast = new CompItemAST();
    ast->item = unique_ptr<BaseAST>($2);
    $$=ast;
  }
  | ConstDecl {
    dynamic_cast<DeclAST*>($1)->markAsGlobal();
    $$ = $1;
  }
  ;

IntCompUnitItem 
  : IntFuncDef {
    $$=$1;
  }
  | VarDefs ';' {
    auto ast = new DeclAST();
    ast -> isConst = false;
    ast -> btype = unique_ptr<BaseAST>(new BTypeAST());
    ast -> defs =  unique_ptr<vector<unique_ptr<BaseAST> > >($1);
    ast -> markAsGlobal();
    $$ = ast;
  }

VoidFuncDef
  : IDENT '(' FuncParamType ')' Block {
    auto ast = new FuncDefAST();
    auto type = new FuncTypeAST();
    type->type = "void";
    ast->func_type = unique_ptr<BaseAST>(type);
    ast->ident = *unique_ptr<string>($1);
    ast->block = unique_ptr<BlockItemAST>($5);
    ast->paramType = unique_ptr<FuncParamTypeAST>(dynamic_cast<FuncParamTypeAST*>($3));
    $$ = ast;
  }
  ;

IntFuncDef
  : IDENT '(' FuncParamType ')' Block {
    auto ast = new FuncDefAST();
    auto type = new FuncTypeAST();
    type->type = "int";
    ast->func_type = unique_ptr<BaseAST>(type);
    ast->ident = *unique_ptr<string>($1);
    ast->block = unique_ptr<BlockItemAST>($5);
    ast->paramType = unique_ptr<FuncParamTypeAST>(dynamic_cast<FuncParamTypeAST*>($3));
    $$ = ast;
  }
  ;

// 同上, 不再解释
//FuncType
//  : VOID {
//    auto ast = new FuncTypeAST();
//    ast->type = "void";
//    $$ = ast;
//  }
//  | INT {
//    auto ast = new FuncTypeAST();
//    ast->type = "int";
//    $$ = ast;
//  }
//  ;
FuncParamType
  : {
    auto vec = new FuncParamTypeAST();
    $$ = vec;
  }
  | FuncFParams {
    $$ = $1;
  }
  ;
FuncFParams
  : FuncFParam {
    auto vec = new FuncParamTypeAST();
    vec->paramList.push_back(unique_ptr<FuncParamAST>(dynamic_cast<FuncParamAST*>($1)));
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    auto vec = $1;
    vec->paramList.push_back(unique_ptr<FuncParamAST>(dynamic_cast<FuncParamAST*>($3)));
    $$ = vec;
  }
  ;
FuncFParam
  : BType IDENT {
    auto ast = new FuncParamAST();
    ast->type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | BType IDENT '[' ']' ConstIndexs {
    auto ast = new FuncParamPointerAST();
    ast->type = unique_ptr<BaseAST>($1);
    static_cast<BTypeAST*>(ast->type.get())->type="*i32";
    ast->ident = *unique_ptr<string>($2);
    ast -> followSize = unique_ptr<std::vector<std::unique_ptr<BaseAST> > >($5);
    $$ = ast;
  }
  ;

//Block         ::= "{" {BlockItem} "}";
Block
  : '{' BlockItems '}' {
    auto ast = new BlockAST();
    ast->item = std::unique_ptr< std::vector<std::unique_ptr<BlockItemAST> > >($2);
    $$ = ast;
  }
  ;
//Block         ::= "{" {BlockItem} "}";
BlockItems
  : {
    auto vec = new vector<unique_ptr<BlockItemAST> >();
    auto ast = new BlankStmtAST();
    //避免入口处就有label的情况,我们直接插入一个Empty和Func入口对接
    //哈哈哈啊哈哈
    vec->push_back(std::unique_ptr<BlockItemAST>(ast)); 
    $$ = vec;
  }
  | BlockItems BlockItem {
    auto vec = $1;
    auto ast = unique_ptr<BlockItemAST>($2);
    if(!vec->empty()){
      vec->back()->setNext(ast.get());
    }
    vec->push_back(move(ast));
    $$ = vec;
  }
  ;

//BlockItem     ::= Decl | Stmt;
BlockItem
  : Decl {
    $$=$1;
  }
  | Stmt {
    $$=$1;
  }
  ;

//Stmt        ::= "return" Exp ";";
//Stmt          ::= LVal "=" Exp ";"
//                | [Exp] ";"
//                | Block
//                | "return" [Exp] ";";
//                | "if" "(" Exp ")" Stmt ["else" Stmt]
//                | "while" "(" Exp ")" Stmt
//                | "break" ";"
//                | "continue" ";"
Stmt
  : RETURN Exp ';' {
    auto ast = new ReturnStmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new AssignStmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new ExpressionStmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    //what ?
    auto ast = new BlankStmtAST();
    $$ = ast;
  }
  | Block {
    $$ = $1;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new BranchWhileStructureAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->loopBody = unique_ptr<BlockItemAST>($5);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new EmptyReturnStmtAST();
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new AbortStmtAST();
    ast -> mode = CODE_BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new AbortStmtAST();
    ast -> mode = CODE_CONTINUE;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt %prec IFX {
    auto ast = new BranchIfStructureAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->branchIf = unique_ptr<BlockItemAST>($5);
    ast->branchElse = nullptr;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new BranchIfStructureAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->branchIf = unique_ptr<BlockItemAST>($5);
    ast->branchElse = unique_ptr<BlockItemAST>($7);
    $$ = ast;
  }
  ;
//Exp         ::= LOrExp;
Exp
  : LOrExp{
    $$ = $1;
  }
  ;

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
UnaryExp
  : PrimaryExp {
    $$ = $1;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->unaryOp=$1;
    ast->subExp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new FuncRefAST();
    ast->refName = *unique_ptr<string>($1);
    ast->invokeParam = unique_ptr<FuncRParamAST>($3);
    $$ = ast;
  }
  ;
FuncRParams
  : {
    auto ast = new FuncRParamAST();
    $$ = ast;
  }
  | FuncRParamList {
    $$ = $1;
  }
  ;
FuncRParamList
  : Exp {
    auto ast = new FuncRParamAST();
    ast->explist.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncRParamList ',' Exp {
    auto ast = $1;
    ast->explist.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

//UnaryOp     ::= "+" | "-" | "!";


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

//PrimaryExp  ::= "(" Exp ")" | Number;
//PrimaryExp    ::= "(" Exp ")" | LVal | Number;

PrimaryExp
  : '(' Exp ')' {
    $$=$2;
  }
  | LVal {
    $$=$1;
  }
  | Number {
    $$=$1;
  }
  ;

//Number      ::= INT_CONST;
Number
  : INT_CONST {
    auto ast=new NumberAST();
    ast->intValue = $1;
    $$ = ast;
  }
  ;



//MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;

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
MulExp
  : UnaryExp {
    $$=$1;
  }
  | MulExp MulOp UnaryExp {
    auto ast=new BiOpExpAST();
    ast->val1 = unique_ptr<BaseAST>($1);
    ast->val2 = unique_ptr<BaseAST>($3);
    ast->op = $2;
    $$ = ast;
  }

//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
AddOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  ;

AddExp
  : MulExp {
    $$=$1;
  }
  | AddExp AddOp MulExp {
    auto ast=new BiOpExpAST();
    ast->val1 = unique_ptr<BaseAST>($1);
    ast->val2 = unique_ptr<BaseAST>($3);
    ast->op = $2;
    $$ = ast;
  }
  ;

//RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
RelExp
  : AddExp {
    $$=$1;
  }
  | RelExp RELOP AddExp {
    auto ast=new BiOpExpAST();
    ast->val1 = unique_ptr<BaseAST>($1);
    ast->val2 = unique_ptr<BaseAST>($3);
    auto reop=$2;
    if(reop->front()=='<'){
      ast->op=(reop->size()==1)?'<':'l';
    }else{
      ast->op=(reop->size()==1)?'>':'g';
    }
    $$ = ast;
  }
  ;

//EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
EqExp
  : RelExp {
    $$=$1;
  }
  | EqExp EQOP RelExp {
    auto ast=new BiOpExpAST();
    ast->val1 = unique_ptr<BaseAST>($1);
    ast->val2 = unique_ptr<BaseAST>($3);
    ast->op = ($2->front()=='=')?'e':'n';
    $$ = ast;
  }
  ;

//LAndExp     ::= EqExp | LAndExp "&&" EqExp;
LAndExp
  : EqExp {
    $$=$1;
  }
  | LAndExp LAND EqExp {
    auto ast=new LOpExpAST();
    ast->val1 = unique_ptr<BaseAST>($1);
    ast->val2 = unique_ptr<BaseAST>($3);
    ast->op = 'a';
    $$ = ast;
  }
  ;
//LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
LOrExp
  : LAndExp {
    $$=$1;
  }
  | LOrExp LOR LAndExp {
    auto ast=new LOpExpAST();
    ast->val1 = unique_ptr<BaseAST>($1);
    ast->val2 = unique_ptr<BaseAST>($3);
    ast->op = 'o';
    $$ = ast;
  }
  ;


//Decl          ::= ConstDecl | VarDecl;
Decl
  : ConstDecl {
    $$=$1;
  }
  | VarDecl {
    $$=$1;
  }
  ;
//ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
//BType         ::= "int";
BType 
  : INT {
    auto ast = new BTypeAST();
    $$ = ast;
  }
  ;
//数组索引
ConstIndexs
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | ConstIndexs '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

ConstDefs
  : ConstDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec -> push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefs ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
ConstDecl
  : CONST BType ConstDefs ';' {
    auto ast = new DeclAST();
    ast -> isConst = true;
    ast -> btype = unique_ptr<BaseAST>($2);
    ast -> defs =  unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  } 
  ;

//ConstDef      ::= IDENT "=" ConstInitVal;
//const can have no init ,in global scope,
ConstDef
  : IDENT ConstIndexs '=' InitVal {
    auto ast = new DefAST();
    ast -> ident = *unique_ptr<string>($1);
    ast -> initVal = unique_ptr<BaseAST>($4);
    ast -> arraySizeRaw = unique_ptr<vector<unique_ptr< BaseAST> > >($2);
    ast -> declaredAsArray = ! (ast -> arraySizeRaw -> empty());
    ast -> declaredAsConst = true;
    $$ = ast;
  }
  | IDENT ConstIndexs {
    auto ast = new DefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->initVal = nullptr;
    ast -> arraySizeRaw = unique_ptr<vector<unique_ptr< BaseAST> > >($2);
    ast -> declaredAsArray = ! (ast -> arraySizeRaw -> empty());
    ast->declaredAsConst = true;
    $$ = ast;
  }
  ;





//LVal          ::= IDENT;
LVal 
  : IDENT ConstIndexs {
    //孩子们 这并不是Const的 但是我复用了
    //因为Const和nonconst的没有区别
    auto vec = $2;
    if(vec ->empty()){
      auto ast = new RefAST();
      ast -> refName = *unique_ptr<string>($1);
      $$ = ast;
    }else{
      auto ast = new ArrayIndexRefAST();
      ast -> refName = *unique_ptr<string>($1);
      ast -> indexPath = unique_ptr<vector<unique_ptr< BaseAST> > >($2);
      $$ = ast;
    }
    
  }
  ;

//VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDecl
  : BType VarDefs ';' {
    auto ast = new DeclAST();
    ast -> isConst = false;
    ast -> btype = unique_ptr<BaseAST>($1);
    ast -> defs =  unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;
VarDefs
  : VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefs ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

//VarDef        ::= IDENT | IDENT "=" InitVal;
VarDef
  : IDENT ConstIndexs {
    auto ast = new DefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->initVal = nullptr;
    ast -> arraySizeRaw = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast -> declaredAsArray = ! (ast -> arraySizeRaw -> empty());
    ast->declaredAsConst = false;
    $$ = ast;
  }
  | IDENT ConstIndexs '=' InitVal {
    auto ast = new DefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->initVal = unique_ptr<BaseAST>($4);
    ast -> arraySizeRaw = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    ast -> declaredAsArray = ! (ast -> arraySizeRaw -> empty());
    ast->declaredAsConst = false;
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    $$ = $1;
  }
  | ArrayExp {
    $$ = $1;
  }
  ;
ArrayExp
  : '{' '}' {
    auto vec = new vector<unique_ptr<BaseAST> > ();
    auto ast = new ArrayInitExpAST();
    ast -> rawInit = std::unique_ptr<vector<unique_ptr<BaseAST> > >(vec);
    $$ = ast ;
  }
  | '{' InitVals '}' {
    auto ast = new ArrayInitExpAST();
    ast -> rawInit = std::unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast ;
  }

InitVals
  : InitVal {
    auto vec = new vector<unique_ptr<BaseAST> > ();
    vec -> push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitVals ',' InitVal {
    auto vec = $1;
    vec -> push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
