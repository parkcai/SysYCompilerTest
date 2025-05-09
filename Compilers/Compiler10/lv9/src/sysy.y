%code requires {
  #include <memory>
  #include <string>
  #include <ast.h>
}

%{
#include <iostream>
#include <memory>
#include <string>
#include <ast.h>
#include <vector>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;
vector<BlockAST*> blockstack;
vector<int> ifidstack;
vector<int> whileidstack;
vector<int> func_call_params;
vector<vector<BaseAST*> > initval_stack;
vector<int> array_size_stack;
vector<vector<BaseAST*> > exp_stack;

void addir(unique_ptr<string> ir){
  blockstack.back()->blockir->append(ir->c_str());
}

string get_type_array(){
    string name = "i32";
    for(int i = (int)(array_size_stack.size()) - 1; i >= 0; i--){
        if(array_size_stack[i] == -1) name = string_format("*%s", name.c_str());
        else name = string_format("[%s, %d]", name.c_str(), array_size_stack[i]);
    }
    return name;
}

void addarrayir(const string& alias, InitvalAST* initval){
  string array_type = get_type_array();
  int total_size = calc_tot_size(array_size_stack);
  array_size_map[alias] = array_size_stack;
  vector<BaseAST*> initarray = calc_initval(initval, array_size_stack);
  if(envstack.depth() == 1){
    if(initarray.size() == 0){
      addir(make_unique<string>(string_format("global @%s = alloc %s, zeroinit\n", alias.c_str(), array_type.c_str())));
    }else{
      addir(make_unique<string>(string_format("global @%s = alloc %s, ", alias.c_str(), array_type.c_str())));
      vector<int> init_num_array(total_size, 0);
      for(auto val: initarray){
        auto initexp = dynamic_cast<InitexpAST*>(val);
        init_num_array[initexp->idx] = initexp->getval();
      }
      string irstr = "";
      for(int i=0; i<total_size; i++){
        int presize = 1;
        for(int j = 0;j < array_size_stack.size(); j++){
          if(i % (total_size / presize) == 0){
            irstr.append("{");
          }
          presize *= array_size_stack[j];
        }
        irstr.append(string_format("%d", init_num_array[i]));
        presize = 1;
        for(int j = 0;j < array_size_stack.size(); j++){
          if((i+1) % (total_size / presize) == 0 && i!=0){
            irstr.append("}");
          }
          presize *= array_size_stack[j];
        }
        if(i != total_size - 1) irstr.append(", ");
        else irstr.append("\n");
      }
      addir(make_unique<string>(irstr));
    }
  }else{
    if(initarray.size() == 0){
      addir(make_unique<string>(string_format("  @%s = alloc %s\n", alias.c_str(), array_type.c_str())));
      addir(make_unique<string>(string_format("  store zeroinit, @%s\n", alias.c_str())));
    }else{
      addir(make_unique<string>(string_format("  @%s = alloc %s\n", alias.c_str(), array_type.c_str())));
      addir(make_unique<string>(string_format("  store zeroinit, @%s\n", alias.c_str())));
      unique_ptr<string> irstr = make_unique<string>("");
      for(auto val: initarray){
        auto initexp = dynamic_cast<InitexpAST*>(val);
        int ptrid = ++tmp_id;
        int presize = 1, now_idx = initexp->idx;
        for(int i=0; i<array_size_stack.size(); i++){
          presize *= array_size_stack[i];
          int dim_size = total_size / presize;
          if(i == 0){
            irstr->append(string_format("  %s = getelemptr @%s, %d\n", GET_ID_STR(ptrid), alias.c_str(), now_idx / dim_size));
          }else{
            irstr->append(string_format("  %s = getelemptr %s, %d\n", GET_ID_STR(++tmp_id), GET_ID_STR(ptrid), now_idx / dim_size));
            ptrid = tmp_id;
          }
          now_idx %= dim_size;
        }
        int retid = 0;
        irstr->append(initexp->GetIR(retid)->c_str());
        irstr->append(string_format("  store %s, %s\n", GET_ID_STR(retid), GET_ID_STR(ptrid)));
      }
      addir(move(irstr));
    }
  }
}

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
  char char_val;
  BaseAST *ast_val;
  IfexpAST *if_ast;
  WhileexpAST *while_ast;
}


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <char_val> EqOp RelOp AndOp OrOp

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block
%type <int_val> Number
%type <char_val> UnaryOp
%type <ast_val> UnaryExp PrimaryExp Exp AddExp MulExp RelExp EqExp LAndExp LOrExp FuncFParams FuncFParam FuncRParams Initval
%type <ast_val> LVal
%type <if_ast> Ifexp
%type <while_ast> Whileexp
%%

CompUnit
  :{
    auto ast = new BlockAST();
    ast->blockir = make_unique<string>("");
    blockstack.push_back(ast);
    envstack.addblock();
  } CompUnitList{
    auto retast = make_unique<BlockAST>();
    unique_ptr<string> retstr = make_unique<string>("");
    retstr->append("decl @getint(): i32\ndecl @getch(): i32\ndecl @getarray(*i32): i32\ndecl @putint(i32)\ndecl @putch(i32)\ndecl @putarray(i32, *i32)\ndecl @starttime()\ndecl @stoptime()\n");
    retstr->append(blockstack.back()->blockir->c_str());
    retast->blockir = move(retstr);
    ast = move(retast);
    blockstack.pop_back();
    envstack.delblock();
  }

CompUnitList:FuncDef {
    addir($1->GetIR());
  }| CompUnitList FuncDef {
    addir($2->GetIR());
  }| Decl | CompUnitList Decl
  ;

FuncDef
  :FuncType IDENT '('{
    auto ast = new BlockAST();
    ast->blockir = make_unique<string>("");
    blockstack.push_back(ast);
    envstack.addblock();
  } FuncFParams ')'{
    unique_ptr<FuncAST> funcAst = make_unique<FuncAST>();
    funcAst->funcfparams = dynamic_cast<FuncFParamsAST*>($5);
    funcAst->functype = dynamic_cast<FuncTypeAST*>($1);
    global_func_table.addfunc(*($2), move(funcAst));
  } Block {
    unique_ptr<string> retstr = make_unique<string>(string_format("fun @%s(", ($2)->c_str()));
    for(auto &param : dynamic_cast<FuncFParamsAST*>($5)->fparams){
      auto paramast = dynamic_cast<FuncFParamAST*>(param);
      retstr->append(string_format("@%s: %s", paramast->ident.c_str(), paramast->type.c_str()));
      if(param != dynamic_cast<FuncFParamsAST*>($5)->fparams.back()){
        retstr->append(", ");
      }
    }
    if(dynamic_cast<FuncTypeAST*>($1)->type == "int"){
      retstr->append("): i32 {\n");
    }else{
      retstr->append(") {\n");
    }
    retstr->append("%entry:\n");
    retstr->append(blockstack.back()->blockir->c_str());
    int autoret_id = ++tmp_id;
    if(dynamic_cast<FuncTypeAST*>($1)->type == "int"){
      dynamic_cast<BlockAST*>(($8))->blockir->append(string_format("  %%%d = add 0, 0\n  ret %%%d\n", autoret_id, autoret_id));
    }else{ // void
      dynamic_cast<BlockAST*>(($8))->blockir->append("  ret\n");
    }
    retstr->append(($8)->GetIR()->c_str());
    retstr->append("}\n");
    blockstack.back()->blockir = move(retstr);
    $$ = blockstack.back();
    blockstack.pop_back();
    envstack.delblock();
  }
  ;

FuncFParams: FuncFParams ',' FuncFParam{
    auto ast = dynamic_cast<FuncFParamsAST*>($1);
    ast->fparams.push_back(($3));
    $$ = ast;
  }|FuncFParam{
    auto ast = new FuncFParamsAST();
    ast->fparams.push_back(($1));
    $$ = ast;
  }|{
    auto ast = new FuncFParamsAST();
    $$ = ast;
  }
  ;

FuncFParam: BType IDENT{
    auto ast = new FuncFParamAST();
    envstack.addvar(*($2), {VarType::VAR, 0});
    auto alias = envstack.getvar_withalias(*($2)).second;
    addir(make_unique<string>(string_format("  @%s = alloc i32\n  store @%s_1, @%s\n", alias.c_str(), alias.c_str(), alias.c_str())));
    alias.append("_1");
    ast->ident = alias;
    ast->type = "i32";
    $$ = ast;
  }| BType IDENT '[' ']'{
    auto ast = new FuncFParamAST();
    envstack.addvar(*($2), {VarType::ARR, 0});
    auto alias = envstack.getvar_withalias(*($2)).second;
    array_size_stack.push_back(-1);
    array_size_map[alias] = array_size_stack;
    addir(make_unique<string>(string_format("  @%s = alloc *i32\n", alias.c_str())));
    addir(make_unique<string>(string_format("  store @%s_1, @%s\n", alias.c_str(), alias.c_str())));
    alias.append("_1");
    ast->ident = alias;
    ast->type = "*i32";
    array_size_stack.clear();
    $$ = ast;
  }| BType IDENT '[' ']' ArraySize{
    auto ast = new FuncFParamAST();
    envstack.addvar(*($2), {VarType::ARR, 0});
    auto alias = envstack.getvar_withalias(*($2)).second;
    array_size_stack.insert(array_size_stack.begin(), -1);
    array_size_map[alias] = array_size_stack;
    string type_array_size = get_type_array();
    addir(make_unique<string>(string_format("  @%s = alloc %s\n", alias.c_str(), type_array_size.c_str())));
    addir(make_unique<string>(string_format("  store @%s_1, @%s\n", alias.c_str(), alias.c_str())));
    alias.append("_1");
    ast->ident = alias;
    ast->type = string_format("%s", type_array_size.c_str());
    array_size_stack.clear();
    $$ = ast;
  }

BType: INT

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = "int";
    $$ = ast;
  }| VOID {
    auto ast = new FuncTypeAST();
    ast->type = "void";
    $$ = ast;
  }
  ;

Block
  : '{'{
    auto ast = new BlockAST();
    ast->blockir = make_unique<string>("");
    // nowblock = ast;
    blockstack.push_back(ast);
    envstack.addblock();
  }  BlockItems '}' {
    $$ = blockstack.back();
    blockstack.pop_back();
    envstack.delblock();
  } | '{' '}' {
    auto ast = new BlockAST();
    ast->blockir = make_unique<string>("");
    $$ = ast;
  }
  ;

BlockItems: BlockItem BlockItems | BlockItem

BlockItem: Decl | Stmt

Ifexp: IF '(' Exp ')' {
    int retid = 0;
    ++global_if_id;
    addir(dynamic_cast<ExpAST*>($3)->GetIR(retid, string_format("%%then_%d", global_if_id), string_format("%%else_%d", global_if_id)));
    addir(make_unique<string>(string_format("  br %s, %%then_%d, %%else_%d\n", GET_ID_STR(retid), global_if_id, global_if_id)));
    addir(make_unique<string>(string_format("%%then_%d:\n", global_if_id)));
    $$ = new IfexpAST(global_if_id);
}
Whileexp:  WHILE '(' Exp ')'{
    int nowid = ++global_while_id;
    int retid = 0;
    addir(make_unique<string>(string_format("  jump %%while_entry_%d\n", nowid)));
    addir(make_unique<string>(string_format("%%while_entry_%d:\n", nowid)));
    addir(dynamic_cast<ExpAST*>($3)->GetIR(retid, string_format("%%while_body_%d", nowid), string_format("%%while_end_%d", nowid)));
    addir(make_unique<string>(string_format("  br %s, %%while_body_%d, %%while_end_%d\n", GET_ID_STR(retid), nowid, nowid)));
    whileidstack.push_back(nowid);
    $$ = new WhileexpAST(nowid);
  }


Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->type = STMT_TYPE_RET;
    addir(ast->GetIR());
    addir(make_unique<string>(string_format("%%reted_%d:\n", ++global_ret_id)));
  }| RETURN ';'{
    addir(make_unique<string>(string_format("  ret\n", ++global_ret_id)));
    addir(make_unique<string>(string_format("%%reted_%d:\n", ++global_ret_id)));
  }| LVal '=' Exp ';'{
    auto ast = new VarAssignAST();
    auto lval_info = envstack.getvar(dynamic_cast<LValAST*>($1)->ident);
    assert(lval_info.type == VarType::VAR || lval_info.type == VarType::ARR);
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    addir(ast->GetIR());
  }| Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->type = STMT_TYPE_EXP;
    addir(ast->GetIR());
  }| ';' | Block {
    addir($1->GetIR()); 
  }| Ifexp Stmt {
    int nowid = ($1)->if_id;
    addir(make_unique<string>(string_format("  jump %%else_%d\n", nowid)));
    addir(make_unique<string>(string_format("%%else_%d:\n", nowid)));
  }| Ifexp Stmt ELSE {
    int nowid = ($1)->if_id;
    addir(make_unique<string>(string_format("  jump %%end_%d\n", nowid)));
    addir(make_unique<string>(string_format("%%else_%d:\n", nowid)));
  }Stmt {
    int nowid = ($1)->if_id;
    addir(make_unique<string>(string_format("  jump %%end_%d\n", nowid)));
    addir(make_unique<string>(string_format("%%end_%d:\n", nowid)));
  }| Whileexp{
    addir(make_unique<string>(string_format("%%while_body_%d:\n", ($1)->while_id)));
  }Stmt{
    addir(make_unique<string>(string_format("  jump %%while_entry_%d\n", ($1)->while_id)));
    addir(make_unique<string>(string_format("%%while_end_%d:\n", ($1)->while_id)));
    whileidstack.pop_back();
  }| BREAK ';'{
    addir(make_unique<string>(string_format("  jump %%while_end_%d\n", whileidstack.back())));
    addir(make_unique<string>(string_format("%%break_%d:\n", ++global_break_id)));
  }| CONTINUE ';'{
    addir(make_unique<string>(string_format("  jump %%while_entry_%d\n", whileidstack.back())));
    addir(make_unique<string>(string_format("%%continue_%d:\n", ++global_continue_id)));
  }
Number
  : INT_CONST {
    $$ = ($1);
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lorExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->type = PRIMARYEXP_TYPE_EXP;
    $$ = ast;
  }| Number {
    auto ast = new PrimaryExpAST();
    ast->number = ($1);
    ast->type = PRIMARYEXP_TYPE_NUMBER;
    $$ = ast;
  }| LVal {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->type = PRIMARYEXP_TYPE_LVAL;
    $$ = ast;
  }

UnaryOp: '+'{
  $$ = '+';
}| '-'{
  $$ = '-';
}| '!'{
  $$ = '!';
};

UnaryExp: PrimaryExp {
  auto ast = new UnaryExpAST();
  ast->primaryExp = unique_ptr<BaseAST>($1);
  ast->type = UNARYEXP_TYPE_PRIMARY;
  $$ = ast;
}| UnaryOp UnaryExp {
  auto ast = new UnaryExpAST();
  ast->op = $1;
  ast->unaryExp = unique_ptr<BaseAST>($2);
  ast->type = UNARYEXP_TYPE_OP_UNARY;
  $$ = ast;
}| IDENT '(' FuncRParams ')'{
  auto ast = new UnaryExpAST();
  ast->ident = *unique_ptr<string>($1);
  ast->funcrparams = unique_ptr<BaseAST>($3);
  ast->type = UNARYEXP_TYPE_FUNC;
  $$ = ast;
};

FuncRParams:FuncRParams ',' Exp{
  auto ast = dynamic_cast<FuncRParamsAST*>($1);
  ast->rparams.push_back(unique_ptr<BaseAST>($3));
  $$ = ast;
}|Exp{
  auto ast = new FuncRParamsAST();
  ast->rparams.push_back(unique_ptr<BaseAST>($1));
  $$ = ast;
}|{
  auto ast = new FuncRParamsAST();
  $$ = ast;
}

MulExp: UnaryExp {
  auto ast = new MulExpAST();
  ast->unaryExp = unique_ptr<BaseAST>($1);
  ast->type = MULEXP_TYPE_UNARY;
  $$ = ast;
}| MulExp '*' UnaryExp {
  auto ast = new MulExpAST();
  ast->op = '*';
  ast->mulExp = unique_ptr<BaseAST>($1);
  ast->unaryExp = unique_ptr<BaseAST>($3);
  ast->type = MULEXP_TYPE_COMPLEX;
  $$ = ast;
}| MulExp '/' UnaryExp {
  auto ast = new MulExpAST();
  ast->op = '/';
  ast->mulExp = unique_ptr<BaseAST>($1);
  ast->unaryExp = unique_ptr<BaseAST>($3);
  ast->type = MULEXP_TYPE_COMPLEX;
  $$ = ast;
}| MulExp '%' UnaryExp {
  auto ast = new MulExpAST();
  ast->op = '%';
  ast->mulExp = unique_ptr<BaseAST>($1);
  ast->unaryExp = unique_ptr<BaseAST>($3);
  ast->type = MULEXP_TYPE_COMPLEX;
  $$ = ast;
};

AddExp: MulExp {
  auto ast = new AddExpAST();
  ast->mulExp = unique_ptr<BaseAST>($1);
  ast->type = ADDEXP_TYPE_MUL;
  $$ = ast;
}| AddExp '+' MulExp {
  auto ast = new AddExpAST();
  ast->op = '+';
  ast->addExp = unique_ptr<BaseAST>($1);
  ast->mulExp = unique_ptr<BaseAST>($3);
  ast->type = ADDEXP_TYPE_COMPLEX;
  $$ = ast;
}| AddExp '-' MulExp {
  auto ast = new AddExpAST();
  ast->op = '-';
  ast->addExp = unique_ptr<BaseAST>($1);
  ast->mulExp = unique_ptr<BaseAST>($3);
  ast->type = ADDEXP_TYPE_COMPLEX;
  $$ = ast;
};

RelExp: AddExp {
  auto ast = new RelExpAST();
  ast->addExp = unique_ptr<BaseAST>($1);
  ast->type = RELEXP_TYPE_ADD;
  $$ = ast;
}| RelExp '<' AddExp {
  auto ast = new RelExpAST();
  ast->op = '<';
  ast->relExp = unique_ptr<BaseAST>($1);
  ast->addExp = unique_ptr<BaseAST>($3);
  ast->type = RELEXP_TYPE_COMPLEX;
  $$ = ast;
}| RelExp '>' AddExp {
  auto ast = new RelExpAST();
  ast->op = '>';
  ast->relExp = unique_ptr<BaseAST>($1);
  ast->addExp = unique_ptr<BaseAST>($3);
  ast->type = RELEXP_TYPE_COMPLEX;
  $$ = ast;
}| RelExp RelOp AddExp {
  auto ast = new RelExpAST();
  ast->op = $2;
  ast->relExp = unique_ptr<BaseAST>($1);
  ast->addExp = unique_ptr<BaseAST>($3);
  ast->type = RELEXP_TYPE_COMPLEX;
  $$ = ast;
}

EqExp: RelExp {
  auto ast = new EqExpAST();
  ast->relExp = unique_ptr<BaseAST>($1);
  ast->type = EQEXP_TYPE_REL;
  $$ = ast;
}| EqExp EqOp RelExp {
  auto ast = new EqExpAST();
  ast->op = $2;
  ast->eqExp = unique_ptr<BaseAST>($1);
  ast->relExp = unique_ptr<BaseAST>($3);
  ast->type = EQEXP_TYPE_COMPLEX;
  $$ = ast;
}

LAndExp: EqExp {
  auto ast = new LAndExpAST();
  ast->eqExp = unique_ptr<BaseAST>($1);
  ast->type = LANDEXP_TYPE_EQ;
  $$ = ast;
}| LAndExp AndOp EqExp {
  auto ast = new LAndExpAST();
  ast->landExp = unique_ptr<BaseAST>($1);
  ast->eqExp = unique_ptr<BaseAST>($3);
  ast->type = LANDEXP_TYPE_COMPLEX;
  $$ = ast;
};

LOrExp: LAndExp {
  auto ast = new LOrExpAST();
  ast->landExp = unique_ptr<BaseAST>($1);
  ast->type = LOREXP_TYPE_LAND;
  $$ = ast;
}| LOrExp OrOp LAndExp {
  auto ast = new LOrExpAST();
  ast->lorExp = unique_ptr<BaseAST>($1);
  ast->landExp = unique_ptr<BaseAST>($3);
  ast->type = LOREXP_TYPE_COMPLEX;
  $$ = ast;
};

Decl: ConstDecl | VarDecl

ConstDecl: CONST FuncType ConstDecl_right ';'

ConstDecl_right: ConstDef | ConstDef ',' ConstDecl_right

VarDecl: FuncType VarDecl_right ';'

VarDecl_right: VarDef | VarDef ',' VarDecl_right

ConstDef: IDENT '=' Exp{
  auto exp = unique_ptr<BaseAST>($3);
  envstack.addvar(*($1), {VarType::CONST, exp->getval()});
}| IDENT ArraySize '=' Initval{
  envstack.addvar(*($1), {VarType::ARR, 0});
  addarrayir(envstack.getvar_withalias(*($1)).second.c_str(), dynamic_cast<InitvalAST*>($4));
  initval_stack.clear();
  array_size_stack.clear();
}| IDENT ArraySize{
  envstack.addvar(*($1), {VarType::ARR, 0});
  auto initvalast = new InitvalAST();
  initvalast->initval_list = make_unique<vector<BaseAST*>>(vector<BaseAST*>());
  initvalast->type = INITVAL_TYPE_LIST;
  addarrayir(envstack.getvar_withalias(*($1)).second.c_str(), initvalast);
  array_size_stack.clear();
}

ArraySize: ArraySize ArraySize_single | ArraySize_single

ArraySize_single: '[' Exp ']'{ // constexp, use getval()
  array_size_stack.push_back(dynamic_cast<ExpAST*>($2)->getval());
}

Initval: Exp{
  auto ast = new InitvalAST();
  ast->exp = unique_ptr<BaseAST>($1);
  ast->type = INITVAL_TYPE_EXP;
  $$ = ast;
}|'{'{
  initval_stack.push_back({});
} InitvalList '}'{
  auto ast = new InitvalAST();
  vector<BaseAST*> initval_list = initval_stack.back();
  initval_stack.pop_back();
  ast->initval_list = make_unique<vector<BaseAST*>>(initval_list);
  ast->type = INITVAL_TYPE_LIST;
  $$ = ast;
}|'{' '}'{
  auto ast = new InitvalAST();
  ast->initval_list = make_unique<vector<BaseAST*>>(vector<BaseAST*>());
  ast->type = INITVAL_TYPE_LIST;
  $$ = ast;
};

InitvalList: InitvalList ',' Initval{
  initval_stack.back().push_back(($3));
}| Initval{
  initval_stack.back().push_back(($1));
};

VarDef: IDENT '=' Exp{
  auto exp = unique_ptr<BaseAST>($3);
  auto ident = *unique_ptr<string>($1);
  envstack.addvar(ident, {VarType::VAR, 0});
  if(envstack.depth() == 1){
    addir(make_unique<string>(string_format("global @%s = alloc i32, %d\n", envstack.getvar_withalias(ident).second.c_str(), exp->getval())));
  }else{
    addir(make_unique<string>(string_format("  @%s = alloc i32\n", envstack.getvar_withalias(ident).second.c_str())));
    auto ast = new VarAssignAST();
    auto lvalast = make_unique<LValAST>();
    lvalast->ident = ident;
    ast->lval = move(lvalast);
    ast->exp = move(exp);
    addir(ast->GetIR());
  }
}| IDENT{
  auto ident = *unique_ptr<string>($1);
  envstack.addvar(ident, {VarType::VAR, 0});
  if(envstack.depth() == 1){
    addir(make_unique<string>(string_format("global @%s = alloc i32, zeroinit\n", envstack.getvar_withalias(ident).second.c_str())));
  }else{
    addir(make_unique<string>(string_format("  @%s = alloc i32\n", envstack.getvar_withalias(ident).second.c_str())));
  }
}| IDENT ArraySize '=' Initval{
  auto ident = *($1);
  envstack.addvar(ident, {VarType::ARR, 0});
  addarrayir(envstack.getvar_withalias(ident).second.c_str(), dynamic_cast<InitvalAST*>($4));
  initval_stack.clear();
  array_size_stack.clear();
}| IDENT ArraySize{
  auto ident = *($1);
  envstack.addvar(ident, {VarType::ARR, 0});
  auto initvalast = new InitvalAST();
  initvalast->initval_list = make_unique<vector<BaseAST*>>(vector<BaseAST*>());
  initvalast->type = INITVAL_TYPE_LIST;
  addarrayir(envstack.getvar_withalias(ident).second.c_str(), initvalast);
  array_size_stack.clear();
}

LVal: IDENT{
  auto ast = new LValAST();
  ast->ident = *unique_ptr<string>($1);
  $$ = ast;
}| IDENT{
  exp_stack.push_back({});
} LValInit_ExpList{
  auto ast = new LValAST();
  ast->ident = *unique_ptr<string>($1);
  auto exp_list = exp_stack.back();
  exp_stack.pop_back();
  ast->exps = vector<ExpAST*>();
  for (auto &exp : exp_list){
    ast->exps.push_back(dynamic_cast<ExpAST*>(exp));
  }
  $$ = ast;
};

LValInit_ExpList: LValInit_ExpList '[' Exp ']'{
  exp_stack.back().push_back(($3));
} | '[' Exp ']'{
  exp_stack.back().push_back(($2));
}

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
