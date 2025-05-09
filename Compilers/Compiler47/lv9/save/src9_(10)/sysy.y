%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.h"
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
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN VOID CONST LE GE EQ NE AND OR IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT 
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt 
%type <ast_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp 
%type <ast_val> Decl ConstDef ConstDecl ConstInitVal BlockItem ConstExp
%type <ast_val> VarDecl VarDef InitVal
%type <ast_val> ClosedStmt OpenStmt SingleStmt
%type <ast_val> CompUnitList FuncFParam 
%type <vec_val> FuncFParams FuncRParams
%type <int_val> Number
%type <str_val> UnaryOp Type LVal
%type <vec_val> ConstDefList BlockItemList VarDefList ConstInitValList InitValList ExpList ConstExpList
%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
    : CompUnitList {
        auto comp_unit = unique_ptr<BaseAST>($1);
        ast = move(comp_unit);
    }
    ;
CompUnitList
    : FuncDef {
        auto comp_unit = new CompUnitAST();
        auto func_def = unique_ptr<BaseAST>($1);
        comp_unit->func_def_list.push_back(move(func_def));
        $$ = comp_unit;
    }| CompUnitList FuncDef {
        auto comp_unit = (CompUnitAST*)($1);
        auto func_def = unique_ptr<BaseAST>($2);
        comp_unit->func_def_list.push_back(move(func_def));
        $$ = comp_unit;
    }
    | Decl {
        auto comp_unit = new CompUnitAST();
        auto decl = unique_ptr<BaseAST>($1);
        comp_unit->decl_list.push_back(move(decl));
        $$ = comp_unit;
    }| CompUnitList Decl {
        auto comp_unit = (CompUnitAST*)($1);
        auto decl = unique_ptr<BaseAST>($2);
        comp_unit->decl_list.push_back(move(decl));
        $$ = comp_unit;
    }
    ;
FuncDef
  : Type IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | Type IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    vector<unique_ptr<BaseAST>> *v_ptr = ($4);
    for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
        ast->params.push_back(move(*it));
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;
FuncFParams
    : FuncFParam {
        vector<unique_ptr<BaseAST>> *ast = new vector<unique_ptr<BaseAST>>;
        ast->push_back(unique_ptr<BaseAST>($1));
        $$ = ast;
    }
    | FuncFParams ',' FuncFParam {
        vector<unique_ptr<BaseAST>> *ast = ($1);
        ast->push_back(unique_ptr<BaseAST>($3));
        $$ = ast;
    }
    ;
FuncFParam
    : Type IDENT {
        auto ast = new FuncFParamAST();
        ast->type = "var";
        ast->b_type = *unique_ptr<string>($1);
        ast->ident = *unique_ptr<string>($2);
        $$ = ast;
    }
    ;
FuncRParams
    : Exp {
        vector<unique_ptr<BaseAST>> *ast = new vector<unique_ptr<BaseAST>>;
        ast->push_back(unique_ptr<BaseAST>($1));
        $$ = ast;
    }
    | FuncRParams ',' Exp {
        vector<unique_ptr<BaseAST>> *ast = ($1);
        ast->push_back(unique_ptr<BaseAST>($3));
        $$ = ast;
    }
    ;

Block
  : '{' BlockItemList '}' {
    auto block = new BlockAST();
    vector<unique_ptr<BaseAST>> *v_ptr = ($2);
    for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
        block->block_item_list.push_back(move(*it));
    $$ = block;
  }
  ;
BlockItemList
    : {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        $$ = v;
    }
    | BlockItemList BlockItem {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($2));
        $$ = v;
    }
    ;
ClosedStmt
    : SingleStmt {
        auto stmt = new StmtAST();
        stmt->type = "single";
        stmt->SingleStmt = unique_ptr<BaseAST>($1);
        $$ = stmt;
    }
    | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
        auto stmt = new StmtAST();
        stmt->type = "if else";
        stmt->Exp = unique_ptr<BaseAST>($3);
        stmt->ifStmt = unique_ptr<BaseAST>($5);
        stmt->elseStmt = unique_ptr<BaseAST>($7);
        $$ = stmt;
    }
    | WHILE '(' Exp ')' ClosedStmt {
        auto stmt = new StmtAST();
        stmt->type = "while";
        stmt->Exp = unique_ptr<BaseAST>($3);
        stmt->whileStmt = unique_ptr<BaseAST>($5);
        $$ = stmt;
    }
    ;
OpenStmt
    : IF '(' Exp ')' Stmt {
        auto stmt = new StmtAST();
        stmt->type = "if";
        stmt->Exp = unique_ptr<BaseAST>($3);
        stmt->ifStmt = unique_ptr<BaseAST>($5);
        $$ = stmt;
    }
    | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
        auto stmt = new StmtAST();
        stmt->type = "if else";
        stmt->Exp = unique_ptr<BaseAST>($3);
        stmt->ifStmt = unique_ptr<BaseAST>($5);
        stmt->elseStmt = unique_ptr<BaseAST>($7);
        $$ = stmt;
    }
    | WHILE '(' Exp ')' OpenStmt {
        auto stmt = new StmtAST();
        stmt->type = "while";
        stmt->Exp = unique_ptr<BaseAST>($3);
        stmt->whileStmt = unique_ptr<BaseAST>($5);
        $$ = stmt;
    }
    ;
Stmt
    : OpenStmt {
        auto stmt = ($1);
        $$ = stmt;
    }
    | ClosedStmt {
        auto stmt = ($1);
        $$ = stmt;
    }
    ;
SingleStmt
  : RETURN Exp ';' {
    auto ast = new SingleStmtAST();
    ast->type = "return";
    ast->Exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto stmt = new SingleStmtAST();
    stmt->type = "return empty";
    $$ = stmt;
  }
  | LVal '=' Exp ';'{
    auto ast = new SingleStmtAST();
    ast->LVal = *unique_ptr<string>($1);
    ast->Exp = unique_ptr<BaseAST>($3);
    ast->type = "assignment";
    $$ = ast;
  }
  | IDENT ExpList '=' Exp ';' 
  {
    auto ast = new SingleStmtAST();
    ast->type = "array assignment";
    ast->LVal = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST>> *v_ptr = ($2);
    for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
        ast->exp_list.push_back(move(*it));
    ast->Exp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | ';' {
    auto ast = new SingleStmtAST();
    ast->type = "empty";
    $$ = ast;
  }
  | Block {
    auto ast = new SingleStmtAST();
    ast->type = "block";
    ast->Block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new SingleStmtAST();
    ast->type = "exp";
    ast->Exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
    | BREAK ';' {
    auto stmt = new SingleStmtAST();
    stmt->type = "break";
    $$ = stmt;
    }
    | CONTINUE ';' {
    auto stmt = new SingleStmtAST();
    stmt->type = "continue";
    $$ = stmt;
    }
  ;
Type
    : INT {
        string *type = new string("int");
        $$ = type;
    }
    | VOID {
        string *type = new string("void");
        $$ = type;
    }
    ;
Number
  : INT_CONST {
    $$ = $1;
  }
  ;

Exp
  :LOrExp {
    auto ast = new ExpAST();
    ast->LOrExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
PrimaryExp
  :'(' Exp ')' {
        auto ast = new PrimaryExpAST();
        ast->type = "Exp";
        ast->Exp = unique_ptr<BaseAST>($2);
        $$ = ast;
  }
  | Number {
        auto ast = new PrimaryExpAST();
        ast->type = "Number";
        ast->number = ($1);
        $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = "LVal";
    ast->LVal = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ExpList {
    auto ast = new PrimaryExpAST();
    ast->type = "array assignment";
    ast->LVal = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST>> *v_ptr = ($2);
    for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
        ast->exp_list.push_back(move(*it));
    $$ = ast;
  }
  ;
  
UnaryExp
    : PrimaryExp {
        auto ast = new UnaryExpAST();
        ast->type = "Primary";
        ast->PrimaryExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | UnaryOp UnaryExp {
        auto ast = new UnaryExpAST();
        ast->type = "Unary";
        ast->UnaryOp = *unique_ptr<string>($1);
        ast->UnaryExp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | IDENT '(' FuncRParams ')' {
        auto ast = new UnaryExpAST();
        ast->type = "Func";
        ast->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *v_ptr = ($3);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            ast->params.push_back(move(*it));
        $$ = ast;
    }
    | IDENT '(' ')' {
        auto ast = new UnaryExpAST();
        ast->type = "Func";
        ast->ident = *unique_ptr<string>($1);
        $$ = ast;
    }
    ;
UnaryOp
    : '+' {
        string *op = new string("+");
        $$ = op;
    }
    | '-' {
        string *op = new string("-");
        $$ = op;
    }
    | '!' {
        string *op = new string("!");
        $$ = op;
    }
    ;
MulExp
  : UnaryExp {
      auto ast = new MulExpAST();
      ast->op = "";
      ast->left = unique_ptr<BaseAST>($1);
      $$ = ast;
  }
  | MulExp '*' UnaryExp {
      auto ast = new MulExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "*";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | MulExp '/' UnaryExp {
      auto ast = new MulExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "/";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | MulExp '%' UnaryExp {
      auto ast = new MulExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "%";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  ;

AddExp
  : MulExp {
      auto ast = new AddExpAST();
      ast->op = "";
      ast->left = unique_ptr<BaseAST>($1);
      $$ = ast;
  }
  | AddExp '+' MulExp {
      auto ast = new AddExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "+";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | AddExp '-' MulExp {
      auto ast = new AddExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "-";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  ;

RelExp
  : AddExp {
      $$ = $1;
  }
  | RelExp '<' AddExp {
      auto ast = new RelExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "<";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | RelExp '>' AddExp {
      auto ast = new RelExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = ">";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | RelExp LE AddExp {
      auto ast = new RelExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "<=";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | RelExp GE AddExp {
      auto ast = new RelExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = ">=";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  ;

EqExp
  : RelExp {
        auto ast = new EqExpAST();
        ast->op = "";
        ast->left = unique_ptr<BaseAST>($1);
        $$ = ast;
  }
  | EqExp EQ RelExp {
      auto ast = new EqExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "==";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | EqExp NE RelExp {
      auto ast = new EqExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "!=";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  ;

LAndExp
  : EqExp {
      auto ast = new LAndExpAST();
      ast->op = "";
      ast->left = unique_ptr<BaseAST>($1);
      $$ = ast;
  }
  | LAndExp AND EqExp {
      auto ast = new LAndExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "&&";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
      auto ast = new LOrExpAST();
      ast->op = "";
      ast->left = unique_ptr<BaseAST>($1);
      $$ = ast;
  }
  | LOrExp OR LAndExp {
      auto ast = new LOrExpAST();
      ast->left = unique_ptr<BaseAST>($1);
      ast->op = "||";
      ast->right = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  ;
Decl
  : ConstDecl {
      auto decl = new DeclAST();
      decl->ConstDecl = unique_ptr<BaseAST>($1);
      $$ = decl;
  }
  | VarDecl{
      auto decl = new DeclAST();
      decl->ConstDecl = unique_ptr<BaseAST>($1);
      $$ = decl;
  }
  ;
VarDecl
    : Type VarDefList ';' {
        auto var_decl = new VarDeclAST();
        var_decl->type = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *v_ptr = ($2);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            var_decl->VarDefList.push_back(move(*it));
        $$ = var_decl;
    }
    ;

VarDef
    : IDENT {
        auto var_def = new VarDefAST();
        var_def->ident = *unique_ptr<string>($1);
        var_def->init = false;
        $$ = var_def;
    }
    | IDENT '=' InitVal {
        auto var_def = new VarDefAST();
        var_def->ident = *unique_ptr<string>($1);
        var_def->init = true;
        var_def->InitVal = unique_ptr<BaseAST>($3);
        $$ = var_def;
    }
    | IDENT ConstExpList {
        auto var_def = new VarDefAST();
        var_def->ident = *unique_ptr<string>($1);
        var_def->init = false;
        vector<unique_ptr<BaseAST>> *v_ptr = ($2);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            var_def->exp_list.push_back(move(*it));
        $$ = var_def;
    }
    | IDENT ConstExpList '=' InitVal {
        auto var_def = new VarDefAST();
        var_def->ident = *unique_ptr<string>($1);
        var_def->init = true;
        vector<unique_ptr<BaseAST>> *v_ptr = ($2);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            var_def->exp_list.push_back(move(*it));
        var_def->InitVal = unique_ptr<BaseAST>($4);
        $$ = var_def;
    }
    ;

InitVal
    : Exp {
        auto init_val = new InitValAST();
        init_val->type = "exp";
        init_val->Exp = unique_ptr<BaseAST>($1);
        $$ = init_val;
    }
    | '{' '}' {
        auto init_val = new InitValAST();
        init_val->type = "list";
        $$ = init_val;
    }
    | '{' InitValList '}' {
        auto init_val = new InitValAST();
        init_val->type = "list";
        vector<unique_ptr<BaseAST>> *v_ptr = ($2);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            init_val->init_val_list.push_back(move(*it));
        $$ = init_val;
    }
    ;
VarDefList
    : VarDef {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        v->push_back(unique_ptr<BaseAST>($1));
        $$ = v;
    }
    | VarDefList ',' VarDef {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($3));
        $$ = v;
    }
    ;
ConstDecl
    : CONST Type ConstDefList ';' {
        auto const_decl = new ConstDeclAST();
        const_decl->type = *unique_ptr<string>($2);
        vector<unique_ptr<BaseAST>> *v_ptr = ($3);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            const_decl->const_def_list.push_back(move(*it));
        $$ = const_decl;
    }
    ;
ConstDefList
    : ConstDef {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        v->push_back(unique_ptr<BaseAST>($1));
        $$ = v;
    }
    | ConstDefList ',' ConstDef {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($3));
        $$ = v;
    }
    ;
ConstDef
    : IDENT '=' ConstInitVal {
        auto const_def = new ConstDefAST();
        const_def->type = "single";
        const_def->ident = *unique_ptr<string>($1);
        const_def->ConstInitVal = unique_ptr<BaseAST>($3);
        $$ = const_def;
    }
    | IDENT ConstExpList '=' ConstInitVal {
        auto const_def = new ConstDefAST();
        const_def->type = "list";
        const_def->ident = *unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>> *v_ptr = ($2);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            const_def->const_exp_list.push_back(move(*it));
        const_def->ConstInitVal = unique_ptr<BaseAST>($4);
        $$ = const_def;
    }
    ;

ConstExpList
    : '[' ConstExp ']' {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        v->push_back(unique_ptr<BaseAST>($2));
        $$ = v;
    }
    | ConstExpList '[' ConstExp ']' {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($3));
        $$ = v;
    }
    ;
ConstInitVal
    : ConstExp {
        auto const_init_val = new ConstInitValAST();
        const_init_val->type = "ConstExp";
        const_init_val->ConstExp = unique_ptr<BaseAST>($1);
        $$ = const_init_val;
    }
    | '{' '}' {
        auto const_init_val = new ConstInitValAST();
        const_init_val->type = "list";
        $$ = const_init_val;
    }
    | '{' ConstInitValList '}' {
        auto const_init_val = new ConstInitValAST();
        const_init_val->type = "list";
        vector<unique_ptr<BaseAST>> *v_ptr = ($2);
        for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
            const_init_val->init_val_list.push_back(move(*it));
        $$ = const_init_val;
    }
    ;
ConstInitValList
    : ConstInitVal {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        v->push_back(unique_ptr<BaseAST>($1));
        $$ = v;
    }
    | ConstInitValList ',' ConstInitVal {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($3));
        $$ = v;
    }
    ;
    
ConstExp
    : Exp {
        auto ast = new ConstExpAST();
        ast->Exp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;
BlockItem
    : Decl {
        auto block_item = new BlockItemAST();
        block_item->content = unique_ptr<BaseAST>($1);
        $$ = block_item;
    }
    | Stmt {
        auto block_item = new BlockItemAST();
        block_item->content = unique_ptr<BaseAST>($1);
        $$ = block_item;
    }
    ;
LVal
    : IDENT {
        string *lval = new string(*unique_ptr<string>($1));
        $$ = lval;
    }
    ;

ExpList
    : '[' Exp ']' {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        v->push_back(unique_ptr<BaseAST>($2));
        $$ = v;
    }
    | ExpList '[' Exp ']' {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($3));
        $$ = v;
    }
    ;
    
InitValList
    : InitVal {
        vector<unique_ptr<BaseAST>> *v = new vector<unique_ptr<BaseAST>>;
        v->push_back(unique_ptr<BaseAST>($1));
        $$ = v;
    }
    | InitValList ',' InitVal {
        vector<unique_ptr<BaseAST>> *v = ($1);
        v->push_back(unique_ptr<BaseAST>($3));
        $$ = v;
    }
    ;
%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
