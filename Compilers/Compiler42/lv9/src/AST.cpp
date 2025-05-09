#include "AST.h"
#include <iostream>
#include <cassert>
#include <cstdio>

using namespace std;

// 1. CompUnitAST
//编译单元，是 AST 的根节点，使用def_vec 保存所有定义（即 DefAST，包括全局常数、变量和函数）
CompUnitAST::CompUnitAST(
    unique_ptr<vector<unique_ptr<BaseAST>>> &def_vec)  //unique_ptr是一种智能指针，用于管理动态分配的内存。
    : def_vec(move(def_vec)) {} //move用于转移对象的所有权，调用move后，原来的unique_ptr将变为nullptr。

//加载标准库函数： @getint 、 @putint、@getch、@putch、@getarray、@putarray、@starttime、@stoptime
//这些函数的类型和参数被封装为 koopa_raw_function_data_t 并存储到符号表和库函数列表中。
void CompUnitAST::load_lib_func(vector<const void *> &lib_func_vec) const {
  koopa_raw_function_data_t *func;
  koopa_raw_type_kind_t *ty;
  vector<const void *> params;

  // 创建并初始化一个函数类型数据结构，这是用于表示函数的结构体，包含函数的类型、名称、参数、基本块等信息。
  func = new koopa_raw_function_data_t();
  //创建一个新的 koopa_raw_type_kind_t 类型的对象。用于定义函数的类型信息，包括返回类型和参数类型。
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;   // 表示函数类型
  ty->data.function.params = slice(KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_INT32);
  func->ty = ty;
  func->name = "@getint";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  //将函数添加到符号表中
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));//func->name + 1,为了去掉名称中的 @ 符号
  //将函数对象添加到 lib_func_vec 中
  lib_func_vec.push_back(func); 

  //以下依次加载其它标准库函数
  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  ty->data.function.params = slice(KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_INT32);
  func->ty = ty;
  func->name = "@getch";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);

  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  params.clear();
  params.push_back(pointer_type_kind(KOOPA_RTT_INT32));
  ty->data.function.params = slice(params, KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_INT32);
  func->ty = ty;
  func->name = "@getarray";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);

  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  params.clear();
  params.push_back(type_kind(KOOPA_RTT_INT32));
  ty->data.function.params = slice(params, KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_UNIT);
  func->ty = ty;
  func->name = "@putint";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);

  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  params.clear();
  params.push_back(type_kind(KOOPA_RTT_INT32));
  ty->data.function.params = slice(params, KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_UNIT);
  func->ty = ty;
  func->name = "@putch";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);

  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  params.clear();
  params.push_back(type_kind(KOOPA_RTT_INT32));
  params.push_back(pointer_type_kind(KOOPA_RTT_INT32));
  ty->data.function.params = slice(params, KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_UNIT);
  func->ty = ty;
  func->name = "@putarray";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);

  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  ty->data.function.params = slice(KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_UNIT);
  func->ty = ty;
  func->name = "@starttime";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);

  func = new koopa_raw_function_data_t();
  ty = new koopa_raw_type_kind_t();
  ty->tag = KOOPA_RTT_FUNCTION;
  ty->data.function.params = slice(KOOPA_RSIK_TYPE);
  ty->data.function.ret = type_kind(KOOPA_RTT_UNIT);
  func->ty = ty;
  func->name = "@stoptime";
  func->params = slice(KOOPA_RSIK_VALUE);
  func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
  symbol_vector.addSymbol(func->name + 1, Value(ValueType::Func, func));
  lib_func_vec.push_back(func);
}

//将编译单元的 AST 节点转化为 Koopa 中间表示
void* CompUnitAST::to_koopa() const {
  //symbol_vector是我们定义的符号表，详见riscv.h中的定义
  symbol_vector.newScope();
  vector<const void *> funcs;
  vector<const void *> values;
  //调用上面的load_lib_func函数，将标准库函数加载到funcs vector中
  load_lib_func(funcs);
  for (auto def = (*def_vec).rbegin(); def != (*def_vec).rend(); def++) {
    //调用DefAST的-->to_koopa函数
    (*def)->to_koopa(funcs, values);
  }
  symbol_vector.delScope();
  //koopa_raw_program_t表示一个完整程序的数据结构（详见koopa.h）
  koopa_raw_program_t *ret = new koopa_raw_program_t();
  ret->values = slice(values, KOOPA_RSIK_VALUE);
  ret->funcs = slice(funcs, KOOPA_RSIK_FUNCTION);

  return ret;
}

// 2. DefAST
//“定义”（Definition）的AST节点
DefAST::DefAST(unique_ptr<BaseAST> &def, DefType type) : def(move(def)) {
  this->type = type;
}

//将定义转化为 Koopa 中间表示。type 包括：函数、常数、变量这3种
void *DefAST::to_koopa(vector<const void *> &funcs,
                       vector<const void *> &values) const {
  if (type == FuncDef) {
    funcs.push_back(def->to_koopa());
  } else if (type == ConstDef) {
    def->to_koopa(values);
  } else if (type == VarDef) {
    def->to_koopa(values);
  }
  return nullptr;
}

// 3. FuncDefAST
//表示函数定义，包括函数类型、名称、参数列表和函数体
FuncDefAST::FuncDefAST(
    unique_ptr<BaseAST> &func_type, const char *ident,
    unique_ptr<vector<unique_ptr<BaseAST>>> &param_vec,
    unique_ptr<BaseAST> &block)
    : func_type(move(func_type)), ident(ident), block(move(block)),
      param_vec(move(param_vec)) {}

//string FuncDefAST::to_string() const {
//  return "FuncDefAST { " + func_type->to_string() + ", " + ident + ", " +
//         block->to_string() + " }";
//}

//将函数转化为 Koopa 中间表示。步骤如下：
//(1)初始化函数返回类型和参数类型。
//(2)设置基本块（如 %entry）用于存储函数体。
//(3)使用符号表管理作用域。
void* FuncDefAST::to_koopa() const {
  koopa_raw_function_data_t* ret = new koopa_raw_function_data_t();

  koopa_raw_type_kind_t* ty = new koopa_raw_type_kind_t();
  symbol_vector.addSymbol(ident.c_str(), Value(ValueType::Func, ret));

  ty->tag = KOOPA_RTT_FUNCTION;
  vector<const void *> params;
  for (int i = (*param_vec).size() - 1; i >= 0; i--) {
    params.push_back((*param_vec)[i]->to_koopa());
  }
  if (params.size() == 0) {
    ty->data.function.params = slice(KOOPA_RSIK_TYPE);
  } else {
    ty->data.function.params = slice(params, KOOPA_RSIK_TYPE);
  }
  ty->data.function.ret = (koopa_raw_type_t)func_type->to_koopa();
  ret->ty = ty;

  char *name = new char[ident.length() + 1];
  ("@" + ident).copy(name, ident.length() + 1);
  name[ident.length() + 1] = '\0';
  ret->name = name;
  params.clear();
  for (int i = (*param_vec).size() - 1; i >= 0; i--) {
    params.push_back((*param_vec)[i]->to_koopa(i));
  }
  if (params.size() == 0) {
    ret->params = slice(KOOPA_RSIK_VALUE);
  } else {
    ret->params = slice(params, KOOPA_RSIK_VALUE);
  }

  vector<const void *> blocks;
  block_vector.init(&blocks);
  koopa_raw_basic_block_data_t *entry = new koopa_raw_basic_block_data_t();
  entry->name = "%entry";
  entry->params = slice(KOOPA_RSIK_VALUE);
  entry->used_by = slice(KOOPA_RSIK_VALUE);
  symbol_vector.newScope();
  block_vector.newBlock(entry);
  for (int i = 0; i < params.size(); i++) {
    koopa_raw_value_data *allo = new koopa_raw_value_data();
    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = ((koopa_raw_value_t)params[i])->ty;
    allo->ty = ty;
    allo->name = ((koopa_raw_value_t)params[i])->name;
    allo->used_by = slice(KOOPA_RSIK_VALUE);
    allo->kind.tag = KOOPA_RVT_ALLOC;
    if (ty->data.pointer.base->tag == KOOPA_RTT_INT32)
      symbol_vector.addSymbol(allo->name + 1, Value(ValueType::Var, allo));
    else
      symbol_vector.addSymbol(allo->name + 1, Value(ValueType::Pointer, allo));
    block_vector.addInst(allo);
    koopa_raw_value_data *store = new koopa_raw_value_data();
    store->ty = type_kind(KOOPA_RTT_UNIT);
    store->name = nullptr;
    store->used_by = slice(KOOPA_RSIK_VALUE);
    store->kind.tag = KOOPA_RVT_STORE;
    store->kind.data.store.dest = allo;
    store->kind.data.store.value = (koopa_raw_value_t)params[i];
    block_vector.addInst(store);
  }
  block->to_koopa();
  block_vector.addInst(
      ret_value(((koopa_raw_type_t)func_type->to_koopa())->tag));
  symbol_vector.delScope();
  block_vector.delBlock();
  block_vector.delUnreachableBlock();
    for (int i = 0; i < blocks.size(); i++) {
    koopa_raw_basic_block_data_t *block =
        (koopa_raw_basic_block_data_t *)blocks[i];
    char *name = new char[ident.length() + strlen(block->name) + 2];
    ("%" + ident + "_" + (block->name + 1))
        .copy(name, ident.length() + strlen(block->name) + 2);
    name[ident.length() + strlen(block->name) + 1] = '\0';
    block->name = name;
  }
  ret->bbs = slice(blocks, KOOPA_RSIK_BASIC_BLOCK);
  return ret;
}

// 4. FuncFParamAST
FuncFParamAST::FuncFParamAST(unique_ptr<BaseAST> &param_type,
                             const char *ident, FuncFParamType type)
    : param_type(move(param_type)), ident(ident), type(type) {}

FuncFParamAST::FuncFParamAST(
    unique_ptr<BaseAST> &param_type,
    unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
    const char *ident, FuncFParamType type)
    : param_type(move(param_type)), ident(ident), type(type),
      index_array(move(index_array)) {}

void *FuncFParamAST::to_koopa() const {
  if (type == Var)
    return type_kind(KOOPA_RTT_INT32);
  if (type == Array) {
    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
    if (index_array == nullptr) {
      ty = pointer_type_kind(KOOPA_RTT_INT32);
    } else {
      vector<size_t> size_vec;
      for (auto index = (*index_array).begin(); index != (*index_array).end();
           index++) {
        size_t tmp = (*index)->cal_value();
        size_vec.push_back(tmp);
      }
      ty->tag = KOOPA_RTT_POINTER;
      ty->data.pointer.base = array_type_kind(KOOPA_RTT_INT32, size_vec);
    }
    return ty;
  }
  assert(false);
}
void *FuncFParamAST::to_koopa(int index) const {
  koopa_raw_value_data *ret = new koopa_raw_value_data();
  //ret->ty = (koopa_raw_type_t)param_type->to_koopa();
  ret->ty = (koopa_raw_type_t)to_koopa();
  char *name = new char[ident.length() + 1];
  ("@" + ident).copy(name, ident.length() + 1);
  name[ident.length() + 1] = '\0';
  ret->name = name;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
  ret->kind.data.func_arg_ref.index = index;
  return ret;
}

// 5. BlockAST
BlockAST::BlockAST() {type = Empty;}

BlockAST::BlockAST(
    unique_ptr<vector<unique_ptr<BaseAST>>> &blockitem_vec)
    : blockitem_vec(move(blockitem_vec)) {
      type = Item;      
    }


//string BlockAST::to_string() const {
//  return "BlockAST { " + stmt->to_string() + " }";
//}


void *BlockAST::to_koopa() const {
  if (type == Empty)
    return nullptr;
  //判断类型  
  if (type == Item) {
    //symbol_vector.newScope();
    for (auto blockitem = (*blockitem_vec).rbegin();
         blockitem != (*blockitem_vec).rend(); blockitem++) {
      (*blockitem)->to_koopa();
    }
    //symbol_vector.delScope();
    return nullptr;
  }
  return nullptr;
}


// 6. StmtAST
//Stmt(statement)表示程序中的语句节点。
//AST.h中定义了enum StmtType{ Exp, Assign, Block, Return, Empty, If, While, Break, Continue};

//这个构造函数用于创建一个不包含任何表达式或子语句的语句节点,如，一个简单的return;
StmtAST::StmtAST(StmtType type) : type(type) {}

//创建一个包含表达式的语句节点,如，return x + 1;
StmtAST::StmtAST(unique_ptr<BaseAST> &exp, StmtType type)
    : type(type), exp(move(exp)) {}

//创建一个包含子语句和表达式的语句节点,如，if (x > 0) { return x; }
StmtAST::StmtAST(unique_ptr<BaseAST> &stmt, unique_ptr<BaseAST> &exp,
                 StmtType type)
    : type(type), exp(move(exp)), stmt(move(stmt)) {}

//CRAY  删除
//string StmtAST::to_string() const {
//  return "StmtAST { return, " + ret_num->to_string() + " }";
//}

//将语句节点转化为 Koopa 中间表示
void* StmtAST::to_koopa() const {
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_UNIT);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);  
  //根据type的不同类型进入不同分支进行处理
  if (type == Return) {
    ret->kind.tag = KOOPA_RVT_RETURN;
    if (exp != nullptr)
      ret->kind.data.ret.value =
          (koopa_raw_value_t)exp->to_koopa();
     block_vector.addInst(ret);
  } else if (type == Assign) {
    ret->kind.tag = KOOPA_RVT_STORE;
    ret->kind.data.store.dest = (koopa_raw_value_t)stmt->to_left_value();
    ret->kind.data.store.value =
        (koopa_raw_value_t)exp->to_koopa();
    block_vector.addInst(ret);
  } else if (type == Exp) {
    //如果类型为表达式
    symbol_vector.newScope();
    exp->to_koopa();
    symbol_vector.delScope();
  } else if (type == Block) {
    symbol_vector.newScope();
    exp->to_koopa();
    symbol_vector.delScope();
  } 
    // 判断是否为If语句
    else if (type == If) {
    ret = (koopa_raw_value_data *)exp->to_koopa();

//    bool true_check = block_vector.checkBlock();
    koopa_raw_basic_block_data_t *false_block =
        new koopa_raw_basic_block_data_t();
    false_block->name = "%false";
    false_block->params = slice(KOOPA_RSIK_VALUE);
    false_block->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.data.branch.false_bb = (koopa_raw_basic_block_t)false_block;
    ret->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);
    if (stmt != nullptr) {
      koopa_raw_basic_block_data_t *end_block =
          new koopa_raw_basic_block_data_t();
      end_block->name = "%end";
      end_block->params = slice(KOOPA_RSIK_VALUE);
      end_block->used_by = slice(KOOPA_RSIK_VALUE);
//      if (!true_check)
        block_vector.addInst(jump_value(end_block));
      block_vector.newBlock(false_block);
      symbol_vector.newScope();
      stmt->to_koopa();
      symbol_vector.delScope();
//      bool false_check = block_vector.checkBlock();
//      if (!false_check)
        block_vector.addInst(jump_value(end_block));
//      if (!true_check || !false_check)
        block_vector.newBlock(end_block);
    } else {
//      if (!true_check)
        block_vector.addInst(jump_value(false_block));
      block_vector.newBlock(false_block);
      } 
    }else if (type == While) {
    // LV7 while
    koopa_raw_basic_block_data_t *cond_block =
        new koopa_raw_basic_block_data_t();
    cond_block->name = "%while_entry";
    cond_block->params = slice(KOOPA_RSIK_VALUE);
    cond_block->used_by = slice(KOOPA_RSIK_VALUE);
    block_vector.addInst(jump_value(cond_block));
    block_vector.newBlock(cond_block);
    ret->kind.tag = KOOPA_RVT_BRANCH;
    ret->kind.data.branch.cond = (koopa_raw_value_t)exp->to_koopa();
    koopa_raw_basic_block_data_t *true_block =
        new koopa_raw_basic_block_data_t();
    true_block->name = "%while_body";
    true_block->params = slice(KOOPA_RSIK_VALUE);
    true_block->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.data.branch.true_bb = (koopa_raw_basic_block_t)true_block;
    ret->kind.data.branch.true_args = slice(KOOPA_RSIK_VALUE);
    koopa_raw_basic_block_data_t *end_block =
        new koopa_raw_basic_block_data_t();
    end_block->name = "%end";
    end_block->params = slice(KOOPA_RSIK_VALUE);
    end_block->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.data.branch.false_bb = (koopa_raw_basic_block_t)end_block;
    ret->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);
    loop_vector.addWhile(cond_block, end_block);
    block_vector.addInst(ret);
    block_vector.newBlock(true_block);
    stmt->to_koopa();
    block_vector.addInst(jump_value(cond_block));
    block_vector.newBlock(end_block);
    loop_vector.delWhile();
  } else if (type == Break) {
    //LV7 break
    if (loop_vector.getTail() == nullptr) {
      cout << "break not in loop" << endl;
      assert(false);
    }
    block_vector.addInst(jump_value(loop_vector.getTail()));
  } else if (type == Continue) {
    //LV7 continue
    if (loop_vector.getHead() == nullptr) {
      cout << "continue not in loop" << endl;
      assert(false);
    }
    block_vector.addInst(jump_value(loop_vector.getHead()));
    
  }
  return ret;
}

// 7.  IfAST
//初始化 if 语句的条件表达式和语句体
//exp： 表示 if 语句的条件表达式。
//stmt：表示 if 语句的语句体（即 if 条件为真时执行的语句）
IfAST::IfAST(unique_ptr<BaseAST> &exp, unique_ptr<BaseAST> &stmt)
    : exp(move(exp)), stmt(move(stmt)) {}


//将 if 语句的 AST 转换为 Koopa IR 的中间表示，生成分支指令和基本块。
//分支指令用于根据条件表达式的值跳转到不同的基本块。
//true 分支的基本块用于存储 if 条件为真时执行的语句。
void *IfAST::to_koopa() const {
  //创建分支指令
  koopa_raw_value_data *ret = new koopa_raw_value_data();
  ret->ty = type_kind(KOOPA_RTT_UNIT);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BRANCH;
  ret->kind.data.branch.cond = (koopa_raw_value_t)exp->to_koopa();
  //将分支指令添加到block_vector
  block_vector.addInst(ret);

  // 创建 true 分支的基本块
  koopa_raw_basic_block_data_t *true_block = new koopa_raw_basic_block_data_t();
  ret->kind.data.branch.true_bb = (koopa_raw_basic_block_t)true_block;
  ret->kind.data.branch.true_args = slice(KOOPA_RSIK_VALUE);
  true_block->name = "%true";
  true_block->params = slice(KOOPA_RSIK_VALUE);
  true_block->used_by = slice(KOOPA_RSIK_VALUE);
  block_vector.newBlock(true_block);
  stmt->to_koopa();
  return ret;
}


// 8. ConstDeclAST    //LV4
//常量声明
ConstDeclAST::ConstDeclAST(
    unique_ptr<BaseAST> &const_type,
    unique_ptr<vector<unique_ptr<BaseAST>>> &ConstDef_vec)
    : const_type(move(const_type)), ConstDef_vec(move(ConstDef_vec)) {
}

void *ConstDeclAST::to_koopa() const {
  koopa_raw_type_t type = (const koopa_raw_type_t)const_type->to_koopa();
  
  //备注 CRay 20241029
/*  for (auto const_def = (*ConstDef_vec).end() - 1;
       const_def >= (*ConstDef_vec).begin(); const_def--) {
    (*const_def)->to_koopa(type);
  }
*/
  if (type->tag == KOOPA_RTT_UNIT) {
    cout << "number type is void" << endl;
    assert(false);
  }
  for (auto const_def = (*ConstDef_vec).rbegin();
       const_def != (*ConstDef_vec).rend(); const_def++) {
    (*const_def)->to_koopa(type);
  }
  return nullptr;
}

void *ConstDeclAST::to_koopa(vector<const void *> &global_var) const {
  koopa_raw_type_t type = (const koopa_raw_type_t)const_type->to_koopa();

  if (type->tag == KOOPA_RTT_UNIT) {
    cout << "number type is void" << endl;
    assert(false);
  }
  for (auto const_def = (*ConstDef_vec).rbegin();
       const_def != (*ConstDef_vec).rend(); const_def++) {
    (*const_def)->to_koopa(global_var,type);
  }
  return nullptr;
}

// 9. TypeAST
TypeAST::TypeAST(const char *type) : type(type) {}

void *TypeAST::to_koopa() const {
  if (type == "int")
    return (void *)type_kind(KOOPA_RTT_INT32);

  if (type == "void")
    return (void *)type_kind(KOOPA_RTT_UNIT);
  return nullptr; // not implemented
}

// 10. ConstDefAST
ConstDefAST::ConstDefAST(const char *ident, unique_ptr<BaseAST> &exp)
    : ident(ident), exp(move(exp)) {
  type = Var;
}

ConstDefAST::ConstDefAST(
    const char *ident,
    unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
    unique_ptr<BaseAST> &exp)
    : ident(ident), exp(move(exp)), index_array(move(index_array)) {
  type = Array;
}
void *ConstDefAST::to_koopa(koopa_raw_type_t const_type) const {
  if (type == Var) {
    int val = exp->cal_value();
    Value value = Value(ValueType::Const, val);
    symbol_vector.addSymbol(ident.c_str(), value);
    return nullptr;
  }
  if (type == Array) {
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    vector<size_t> size_vec;
    size_t size = 1;
    for (auto index = (*index_array).begin(); index != (*index_array).end();
         index++) {
      size_t tmp = (*index)->cal_value();
      size_vec.push_back(tmp);
      size *= tmp;
    }
    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = array_type_kind(const_type->tag, size_vec);
    ret->ty = ty;
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    name[ident.length() + 1] = '\0';
    ret->name = name;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_ALLOC;
    block_vector.addInst(ret);
    Value value = Value(ValueType::Array, ret);
    symbol_vector.addSymbol(ident.c_str(), value);
    vector<const void *> init_vec;
    vector<int> number_vec;
    InitValAST *initval = dynamic_cast<InitValAST *>(exp.get());
    if (initval->type == InitValAST::Empty) {
      koopa_raw_value_data *store = new koopa_raw_value_data();
      store->ty = type_kind(KOOPA_RTT_UNIT);
      store->name = nullptr;
      store->used_by = slice(KOOPA_RSIK_VALUE);
      store->kind.tag = KOOPA_RVT_STORE;
      store->kind.data.store.dest = ret;
      store->kind.data.store.value =
          zero_init(array_type_kind(const_type->tag, size_vec));
    } else {
      initval->preprocess(init_vec, size_vec);
      if (init_vec.size() != size) {
        cout << "array size not match" << endl;
        assert(false);
      }
      vector<koopa_raw_value_data *> get_vec;
      for (int i = 0; i < size; i++) {
        int tmp = i;
        int tmp_size = size;
        for (int j = 0; j < size_vec.size(); j++) {
          tmp_size /= size_vec[j];
          int index = tmp / tmp_size;
          tmp = tmp % tmp_size;
          if (j < get_vec.size()) {
            if (index ==
                get_vec[j]
                    ->kind.data.get_elem_ptr.index->kind.data.integer.value) {
              continue;
            } else {
              while (j < get_vec.size()) {
                get_vec.pop_back();
              }
            }
          }
          koopa_raw_value_data *get = new koopa_raw_value_data();
          get->ty = type_kind(KOOPA_RTT_INT32);
          get->name = nullptr;
          get->used_by = slice(KOOPA_RSIK_VALUE);
          get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
          get->kind.data.get_elem_ptr.src =
              j == 0 ? ret : (koopa_raw_value_t)get_vec[j - 1];
          get->kind.data.get_elem_ptr.index =
              (koopa_raw_value_t)NumberAST(index).to_koopa();
          get_vec.push_back(get);
          block_vector.addInst(get);
        }
        koopa_raw_value_data *store = new koopa_raw_value_data();
        store->ty = type_kind(KOOPA_RTT_UNIT);
        store->name = nullptr;
        store->used_by = slice(KOOPA_RSIK_VALUE);
        store->kind.tag = KOOPA_RVT_STORE;
        store->kind.data.store.dest =
            (koopa_raw_value_t)get_vec[size_vec.size() - 1];
        store->kind.data.store.value = (koopa_raw_value_t)init_vec[i];
        block_vector.addInst(store);
      }
    }
    return nullptr;
  }
  return nullptr;
}

void *ConstDefAST::to_koopa(vector<const void *> &global_var,
                            koopa_raw_type_t const_type) const {
  if (type == Var) {
    int val = exp->cal_value();
    Value value = Value(ValueType::Const, val);
    symbol_vector.addSymbol(ident.c_str(), value);
    return nullptr;
  }
  if (type == Array) {
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    vector<size_t> size_vec;
    size_t size = 1;
    for (auto index = (*index_array).begin(); index != (*index_array).end();
         index++) {
      size_t tmp = (*index)->cal_value();
      size_vec.push_back(tmp);
      size *= tmp;
    }
    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = array_type_kind(const_type->tag, size_vec);
    ret->ty = ty;
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    name[ident.length() + 1] = '\0';
    ret->name = name;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
    global_var.push_back(ret);
    Value value = Value(ValueType::Array, ret);
    symbol_vector.addSymbol(ident.c_str(), value);
    vector<const void *> init_vec;
    InitValAST *initval = dynamic_cast<InitValAST *>(exp.get());
    vector<int> number_vec;
    if (initval->type == InitValAST::Empty) {
      ret->kind.data.global_alloc.init =
          zero_init(array_type_kind(const_type->tag, size_vec));
    } else {
      initval->preprocess(init_vec, size_vec);
      if (init_vec.size() != size) {
        cout << "array size not match" << endl;
        assert(false);
      }
      ret->kind.data.global_alloc.init =
          (koopa_raw_value_t)initval->to_koopa(init_vec, size_vec, 0);
    }
    return nullptr;
  }  
  return nullptr;
}

// 11. VarDeclAST
VarDeclAST::VarDeclAST(
    unique_ptr<BaseAST> &var_type,
    unique_ptr<vector<unique_ptr<BaseAST>>> &VarDef_vec)
    : var_type(move(var_type)), VarDef_vec(move(VarDef_vec)) {}

void *VarDeclAST::to_koopa() const {
  koopa_raw_type_t type = (const koopa_raw_type_t)var_type->to_koopa();
  
  if (type->tag == KOOPA_RTT_UNIT) {
    cout << "number type is void" << endl;
    assert(false);
  }
  for (auto var_def = (*VarDef_vec).rbegin(); var_def != (*VarDef_vec).rend();
       var_def++) {
    (*var_def)->to_koopa(type);
  }
  return nullptr;
}

void *VarDeclAST::to_koopa(vector<const void *> &global_var) const {
  koopa_raw_type_t type = (const koopa_raw_type_t)var_type->to_koopa();
  for (auto var_def = (*VarDef_vec).rbegin(); var_def != (*VarDef_vec).rend();
       var_def++) {
    (*var_def)->to_koopa(global_var, type);
  }
  return nullptr;
}

// 12. VarDefAST
VarDefAST::VarDefAST(const char *ident, unique_ptr<BaseAST> &exp,
                     VarDefType type)
    : ident(ident) {
  this->type = type;
  this->exp = move(exp);
}

VarDefAST::VarDefAST(const char *ident, VarDefType type) : ident(ident) {
  this->type = type;
}

VarDefAST::VarDefAST(
    const char *ident,
    unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
    VarDefType type)
    : ident(ident), index_array(move(index_array)) {
  this->type = type;
}

VarDefAST::VarDefAST(
    const char *ident,
    unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
    unique_ptr<BaseAST> &exp, VarDefType type)
    : ident(ident), exp(move(exp)), index_array(move(index_array)) {
  this->type = type;
}
void *VarDefAST::to_koopa(koopa_raw_type_t var_type) const {
  if (type == Exp) {
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    ret->ty = pointer_type_kind(KOOPA_RTT_INT32);
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    name[ident.length() + 1] = '\0';
    ret->name = name;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_ALLOC;
    block_vector.addInst(ret);
    Value value = Value(ValueType::Var, ret);
    symbol_vector.addSymbol(ident.c_str(), value);
    if (exp != nullptr) {
      koopa_raw_value_data *store = new koopa_raw_value_data();
      store->ty = type_kind(KOOPA_RTT_INT32);
      store->name = nullptr;
      store->used_by = slice(KOOPA_RSIK_VALUE);
      store->kind.tag = KOOPA_RVT_STORE;
      store->kind.data.store.dest = (koopa_raw_value_t)ret;
      store->kind.data.store.value = (koopa_raw_value_t)exp->to_koopa();
      block_vector.addInst(store);
    }
  } else if (type == Array) {
    vector<size_t> size_vec;
    size_t size = 1;
    for (auto index = (*index_array).begin(); index != (*index_array).end();
         index++) {
      size_t tmp = (*index)->cal_value();
      size_vec.push_back(tmp);
      size *= tmp;
    }
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = array_type_kind(var_type->tag, size_vec);
    ret->ty = ty;
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    name[ident.length() + 1] = '\0';
    ret->name = name;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_ALLOC;
    block_vector.addInst(ret);
    Value value = Value(ValueType::Array, ret);
    symbol_vector.addSymbol(ident.c_str(), value);
    if (exp != nullptr) {
      vector<const void *> init_vec;
      InitValAST *initval = dynamic_cast<InitValAST *>(exp.get());
      if (initval->type == InitValAST::Empty) {
        koopa_raw_value_data *store = new koopa_raw_value_data();
        store->ty = type_kind(KOOPA_RTT_UNIT);
        store->name = nullptr;
        store->used_by = slice(KOOPA_RSIK_VALUE);
        store->kind.tag = KOOPA_RVT_STORE;
        store->kind.data.store.dest = ret;
        store->kind.data.store.value =
            zero_init(array_type_kind(var_type->tag, size_vec));
        block_vector.addInst(store);
      } else {
        initval->preprocess(init_vec, size_vec);
        if (init_vec.size() != size) {
          cout << "array size not match" << endl;
          assert(false);
        }
        vector<koopa_raw_value_data *> get_vec;
        for (int i = 0; i < size; i++) {
          int tmp = i;
          int tmp_size = size;
          for (int j = 0; j < size_vec.size(); j++) {
            tmp_size /= size_vec[j];
            int index = tmp / tmp_size;
            tmp = tmp % tmp_size;
            if (j < get_vec.size()) {
              if (index ==
                  get_vec[j]
                      ->kind.data.get_elem_ptr.index->kind.data.integer.value) {
                continue;
              } else {
                while (j < get_vec.size()) {
                  get_vec.pop_back();
                }
              }
            }
            koopa_raw_value_data *get = new koopa_raw_value_data();
            get->ty = type_kind(KOOPA_RTT_INT32);
            get->name = nullptr;
            get->used_by = slice(KOOPA_RSIK_VALUE);
            get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            get->kind.data.get_elem_ptr.src =
                j == 0 ? ret : (koopa_raw_value_t)get_vec[j - 1];
            get->kind.data.get_elem_ptr.index =
                (koopa_raw_value_t)NumberAST(index).to_koopa();
            get_vec.push_back(get);
            block_vector.addInst(get);
          }
          koopa_raw_value_data *store = new koopa_raw_value_data();
          store->ty = type_kind(KOOPA_RTT_UNIT);
          store->name = nullptr;
          store->used_by = slice(KOOPA_RSIK_VALUE);
          store->kind.tag = KOOPA_RVT_STORE;
          store->kind.data.store.dest =
              (koopa_raw_value_t)get_vec[size_vec.size() - 1];
          store->kind.data.store.value = (koopa_raw_value_t)init_vec[i];
          block_vector.addInst(store);
        }
      }
    } else {
      koopa_raw_value_data *store = new koopa_raw_value_data();
      store->ty = type_kind(KOOPA_RTT_UNIT);
      store->name = nullptr;
      store->used_by = slice(KOOPA_RSIK_VALUE);
      store->kind.tag = KOOPA_RVT_STORE;
      store->kind.data.store.dest = ret;
      store->kind.data.store.value =
          zero_init(array_type_kind(var_type->tag, size_vec));
      block_vector.addInst(store);
    }
  }  
  return nullptr;
}

void *VarDefAST::to_koopa(vector<const void *> &global_var,
                          koopa_raw_type_t var_type) const {
  if (type == Exp) {
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    ret->ty = pointer_type_kind(var_type->tag);
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    name[ident.length() + 1] = '\0';
    ret->name = name;
    ret->used_by = slice(KOOPA_RSIK_VALUE);

    //kind.tag设为为全局变量属性KOOPA_RVT_GLOBAL_ALLOC
    ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
    global_var.push_back(ret);

    //赋值给Value
    Value value = Value(ValueType::Var, ret);

    //  加入到符号表
    symbol_vector.addSymbol(ident.c_str(), value);
    if (exp != nullptr) {
      ret->kind.data.global_alloc.init = (koopa_raw_value_t)exp->to_koopa();
    } else {
      ret->kind.data.global_alloc.init = zero_init(type_kind(var_type->tag));
    }
  } else if (type == Array) {
    vector<size_t> size_vec;
    size_t size = 1;
    for (auto index = (*index_array).begin(); index != (*index_array).end();
         index++) {
      size_t tmp = (*index)->cal_value();
      size_vec.push_back(tmp);
      size *= tmp;
    }
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = array_type_kind(var_type->tag, size_vec);
    ret->ty = ty;
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    name[ident.length() + 1] = '\0';
    ret->name = name;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
    global_var.push_back(ret);
    Value value = Value(ValueType::Array, ret);
    symbol_vector.addSymbol(ident.c_str(), value);
    if (exp != nullptr) {
      vector<const void *> init_vec;
      InitValAST *initval = dynamic_cast<InitValAST *>(exp.get());
      vector<int> number_vec;
      if (initval->type == InitValAST::Empty) {
        ret->kind.data.global_alloc.init =
            zero_init(array_type_kind(var_type->tag, size_vec));
      } else {
        initval->preprocess(init_vec, size_vec);
        if (init_vec.size() != size) {
          cout << "array size not match " << init_vec.size() << " " << size
                    << endl;
          assert(false);
        }
        ret->kind.data.global_alloc.init =
            (koopa_raw_value_t)initval->to_koopa(init_vec, size_vec, 0);
      }
    } else {
      ret->kind.data.global_alloc.init =
          zero_init(array_type_kind(var_type->tag, size_vec));
    }
  }
  return nullptr;
}

// 14. InitValAST
InitValAST::InitValAST() { type = Empty; }

InitValAST::InitValAST(unique_ptr<BaseAST> &exp) : exp(move(exp)) {
  type = Exp;
}

InitValAST::InitValAST(
    unique_ptr<vector<unique_ptr<BaseAST>>> &initlist_vec)
    : initlist_vec(move(initlist_vec)) {
  type = InitList;
}

void *InitValAST::to_koopa() const {
  if (type != Exp)
    assert(false);
  return exp->to_koopa();
}

void *InitValAST::to_koopa(vector<const void *> &init_vec,
                           vector<size_t> size_vec, int level) const {
  vector<const void *> *init_val = new vector<const void *>();
  if (level == size_vec.size() - 1) {
    for (int i = 0; i < size_vec[level]; i++) {
      init_val->push_back((*init_vec.begin()));
      init_vec.erase(init_vec.begin());
    }
  } else {
    for (int i = 0; i < size_vec[level]; i++) {
      init_val->push_back(to_koopa(init_vec, size_vec, level + 1));
    }
  }
  koopa_raw_value_data *ret = new koopa_raw_value_data();
  vector<size_t> sub_vec;
  for (int i = level; i < size_vec.size(); i++) {
    sub_vec.push_back(size_vec[i]);
  }
  ret->ty = array_type_kind(KOOPA_RTT_INT32, sub_vec);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_AGGREGATE;
  ret->kind.data.aggregate.elems = slice(*init_val, KOOPA_RSIK_VALUE);
  return (void *)ret;
}

void InitValAST::preprocess(vector<const void *> &init_vec,
                            vector<size_t> size_vec) {
  if (type != Empty) {
    size_t len_n = size_vec.back();
    for (auto init = (*initlist_vec).rbegin(); init != (*initlist_vec).rend();
         init++) {
      InitValAST *initval = dynamic_cast<InitValAST *>((*init).get());
      if (initval->type == Exp) {
        init_vec.push_back(NumberAST(initval->exp->cal_value()).to_koopa());
      } else if (initval->type == InitList || initval->type == Empty) {
        int curr_size = init_vec.size();
        vector<size_t> sub_vec;
        if (curr_size % len_n != 0) {
          cout << "init list error" << endl;
          assert(false);
        }
        int align_size = curr_size / len_n;
        int level = 1;
        for (int i = size_vec.size() - 2; i >= 0; i--) {
          if (align_size % size_vec[i] != 0) {
            break;
          }
          align_size /= size_vec[i];
          level++;
        }
        for (int i = max(int(size_vec.size() - level), 1);
             i < size_vec.size(); i++) {
          sub_vec.push_back(size_vec[i]);
        }
        initval->preprocess(init_vec, sub_vec);
      }
    }
  } else {
    int size = 1;
    for (int i = 0; i < size_vec.size(); i++) {
      size *= size_vec[i];
    }
    for (int i = 0; i < size; i++) {
      init_vec.push_back(NumberAST(0).to_koopa());
    }
  }
  int size = 1;
  for (int i = 0; i < size_vec.size(); i++) {
    size *= size_vec[i];
  }
  while (init_vec.size() % size != 0) {
    init_vec.push_back(NumberAST(0).to_koopa());
  }
}

int InitValAST::cal_value() const {
  if (type != Exp)
    assert(false);
  return exp->cal_value();
}

// 15. LValAST
LValAST::LValAST(const char *ident) : ident(ident) {}

LValAST::LValAST(
    const char *ident,
    unique_ptr<vector<unique_ptr<BaseAST>>> &index_array)
    : ident(ident), index_array(move(index_array)) {}

void *LValAST::to_left_value() const {
  Value value = symbol_vector.getSymbol(ident);
  if (value.type == ValueType::Var) {
    return (void *)value.data.var_value;
  }
  if (value.type == ValueType::Array) {
    vector<koopa_raw_value_data *> get_vec;

    for (int i = 0; i < index_array->size(); i++) {
      koopa_raw_value_data *get = new koopa_raw_value_data();
      get->name = nullptr;
      get->used_by = slice(KOOPA_RSIK_VALUE);
      get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
      get->kind.data.get_elem_ptr.index =
          (koopa_raw_value_t)(*index_array)[i]->to_koopa();
      koopa_raw_type_kind *ty = new koopa_raw_type_kind();
      ty->tag = KOOPA_RTT_POINTER;
      if (i == 0) {
        ty->data.pointer.base =
            value.data.array_value->ty->data.pointer.base->data.array.base;
        get->ty = ty;
        get->kind.data.get_elem_ptr.src =
            (koopa_raw_value_t)value.data.array_value;
      } else {
        ty->data.pointer.base =
            get_vec[i - 1]->ty->data.pointer.base->data.array.base;
        get->ty = ty;
        get->kind.data.get_elem_ptr.src = (koopa_raw_value_t)get_vec[i - 1];
      }
      get_vec.push_back(get);
      block_vector.addInst(get);
    }
    return (void *)get_vec.back();
  }
  if (value.type == ValueType::Const) {
    cout << "const is not left value" << endl;
    assert(false);
  }
  if (value.type == ValueType::Func) {
    cout << "func is not left value" << endl;
    assert(false);
  }
  if (value.type == ValueType::Pointer) {
    koopa_raw_value_data *load = new koopa_raw_value_data();
    load->ty = value.data.pointer_value->ty->data.pointer.base;
    load->name = nullptr;
    load->used_by = slice(KOOPA_RSIK_VALUE);
    load->kind.tag = KOOPA_RVT_LOAD;
    load->kind.data.load.src = (koopa_raw_value_t)value.data.pointer_value;
    block_vector.addInst(load);
    vector<koopa_raw_value_data *> get_vec;
    for (int i = 0; i < index_array->size(); i++) {
      koopa_raw_value_data *get = new koopa_raw_value_data();
      get->name = nullptr;
      get->used_by = slice(KOOPA_RSIK_VALUE);
      if (i == 0) {
        get->ty = load->ty;
        get->kind.tag = KOOPA_RVT_GET_PTR;
        get->kind.data.get_ptr.index =
            (koopa_raw_value_t)(*index_array)[i]->to_koopa();
        get->kind.data.get_ptr.src = load;
      } else {
        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base =
            get_vec[i - 1]->ty->data.pointer.base->data.array.base;
        get->ty = ty;
        get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
        get->kind.data.get_elem_ptr.index =
            (koopa_raw_value_t)(*index_array)[i]->to_koopa();
        get->kind.data.get_elem_ptr.src = (koopa_raw_value_t)get_vec[i - 1];
      }
      get_vec.push_back(get);
      block_vector.addInst(get);
    }
    return (void *)get_vec.back();
  }
  assert(false);
}

void *LValAST::to_koopa() const {
  Value value = symbol_vector.getSymbol(ident);
  koopa_raw_value_data *ret = new koopa_raw_value_data();
  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  // cout << "value: " << value.type << endl;
  if (value.type == ValueType::Var) {
    ret->kind.tag = KOOPA_RVT_LOAD;
    ret->kind.data.load.src = (koopa_raw_value_t)value.data.var_value;
    block_vector.addInst(ret);
  } else if (value.type == ValueType::Const) {
    ret->kind.tag = KOOPA_RVT_INTEGER;
    ret->kind.data.integer.value = value.data.const_value;
    } else if (value.type == ValueType::Array) {
    vector<koopa_raw_value_data *> get_vec;
    if (index_array != nullptr) {
      for (int i = 0; i < index_array->size(); i++) {
        koopa_raw_value_data *get = new koopa_raw_value_data();
        get->name = nullptr;
        get->used_by = slice(KOOPA_RSIK_VALUE);
        get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
        get->kind.data.get_elem_ptr.index =
            (koopa_raw_value_t)(*index_array)[i]->to_koopa();
        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
        ty->tag = KOOPA_RTT_POINTER;
        if (i == 0) {

          ty->data.pointer.base =
              value.data.array_value->ty->data.pointer.base->data.array.base;
          get->ty = ty;
          get->kind.data.get_elem_ptr.src =
              (koopa_raw_value_t)value.data.array_value;
        } else {
          ty->data.pointer.base =
              get_vec[i - 1]->ty->data.pointer.base->data.array.base;
          get->ty = ty;
          get->kind.data.get_elem_ptr.src = (koopa_raw_value_t)get_vec[i - 1];
        }
        get_vec.push_back(get);
        block_vector.addInst(get);
      }
    }
    if (index_array == nullptr) {
      koopa_raw_value_data *get = new koopa_raw_value_data();
      koopa_raw_type_kind *ty = new koopa_raw_type_kind();
      ty->tag = KOOPA_RTT_POINTER;
      ty->data.pointer.base =
          value.data.array_value->ty->data.pointer.base->data.array.base;
      get->ty = ty;
      get->name = nullptr;
      get->used_by = slice(KOOPA_RSIK_VALUE);
      get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
      get->kind.data.get_elem_ptr.src = value.data.array_value;
      get->kind.data.get_elem_ptr.index =
          (koopa_raw_value_t)NumberAST(0).to_koopa();
      block_vector.addInst(get);
      ret = get;
    } else if (get_vec.back()->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
      koopa_raw_value_data *get = new koopa_raw_value_data();
      koopa_raw_type_kind *ty = new koopa_raw_type_kind();
      ty->tag = KOOPA_RTT_POINTER;
      ty->data.pointer.base =
          get_vec.back()->ty->data.pointer.base->data.array.base;
      get->ty = ty;
      get->name = nullptr;
      get->used_by = slice(KOOPA_RSIK_VALUE);
      get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
      get->kind.data.get_elem_ptr.src = (koopa_raw_value_t)get_vec.back();
      get->kind.data.get_elem_ptr.index =
          (koopa_raw_value_t)NumberAST(0).to_koopa();
      block_vector.addInst(get);
      ret = get;
    } else {
      ret->kind.tag = KOOPA_RVT_LOAD;
      ret->kind.data.load.src = (koopa_raw_value_t)get_vec.back();
      block_vector.addInst(ret);
    }
  }
  if (value.type == ValueType::Func) {
    cout << "func is not a value" << endl;
    assert(false);
  }
  if (value.type == ValueType::Pointer) {
    koopa_raw_value_data *load = new koopa_raw_value_data();
    load->ty = value.data.pointer_value->ty->data.pointer.base;
    load->name = nullptr;
    load->used_by = slice(KOOPA_RSIK_VALUE);
    load->kind.tag = KOOPA_RVT_LOAD;
    load->kind.data.load.src = value.data.pointer_value;
    block_vector.addInst(load);
    vector<koopa_raw_value_data *> get_vec;
    if (index_array != nullptr) {
      for (int i = 0; i < index_array->size(); i++) {
        koopa_raw_value_data *get = new koopa_raw_value_data();
        get->name = nullptr;
        get->used_by = slice(KOOPA_RSIK_VALUE);
        if (i == 0) {
          get->kind.tag = KOOPA_RVT_GET_PTR;
          get->kind.data.get_ptr.index =
              (koopa_raw_value_t)(*index_array)[i]->to_koopa();
          get->kind.data.get_ptr.src = load;
          get->ty = load->ty;
        } else {
          get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
          get->kind.data.get_elem_ptr.index =
              (koopa_raw_value_t)(*index_array)[i]->to_koopa();
          get->kind.data.get_elem_ptr.src = (koopa_raw_value_t)get_vec[i - 1];
          koopa_raw_type_kind *ty = new koopa_raw_type_kind();
          ty->tag = KOOPA_RTT_POINTER;
          ty->data.pointer.base =
              get_vec[i - 1]->ty->data.pointer.base->data.array.base;
          get->ty = ty;
        }
        get_vec.push_back(get);
        block_vector.addInst(get);
      }
    }
    if (index_array == nullptr) {
      koopa_raw_value_data *get = new koopa_raw_value_data();
      get->ty = load->ty;
      get->name = nullptr;
      get->used_by = slice(KOOPA_RSIK_VALUE);
      get->kind.tag = KOOPA_RVT_GET_PTR;
      get->kind.data.get_ptr.src = load;
      get->kind.data.get_ptr.index = (koopa_raw_value_t)NumberAST(0).to_koopa();
      block_vector.addInst(get);
      ret = get;
    } else if (get_vec.back()->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
      koopa_raw_value_data *get = new koopa_raw_value_data();
      koopa_raw_type_kind *ty = new koopa_raw_type_kind();
      ty->tag = KOOPA_RTT_POINTER;
      ty->data.pointer.base =
          get_vec.back()->ty->data.pointer.base->data.array.base;
      get->ty = ty;
      get->name = nullptr;
      get->used_by = slice(KOOPA_RSIK_VALUE);
      get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
      get->kind.data.get_elem_ptr.src = (koopa_raw_value_t)get_vec.back();
      get->kind.data.get_elem_ptr.index =
          (koopa_raw_value_t)NumberAST(0).to_koopa();
      block_vector.addInst(get);
      ret = get;
    } else {
      ret->kind.tag = KOOPA_RVT_LOAD;
      ret->kind.data.load.src = (koopa_raw_value_t)get_vec.back();
      block_vector.addInst(ret);
    }
  }
  return ret;
}

int LValAST::cal_value() const {
  return symbol_vector.getSymbol(ident).data.const_value;
}

// 17. ExpAST
ExpAST::ExpAST(unique_ptr<BaseAST>& add_exp)
    : add_exp(move(add_exp)) {}

void* ExpAST::to_koopa() const {
  return add_exp->to_koopa();
}
int ExpAST::cal_value() const { return add_exp->cal_value(); }

// 18. PrimaryExpAST
PrimaryExpAST::PrimaryExpAST(unique_ptr<BaseAST>& exp)
    : exp(move(exp)) {}

void* PrimaryExpAST::to_koopa() const {
  return exp->to_koopa();
}

int PrimaryExpAST::cal_value() const { return exp->cal_value(); }


// 19. UnaryExpAST
//表示一元表达式节点（如 -x 或 !x）
UnaryExpAST::UnaryExpAST(unique_ptr<BaseAST>& exp) : exp(move(exp)) {
  type = Exp;
}

UnaryExpAST::UnaryExpAST(const char* op, unique_ptr<BaseAST>& exp)
    : op(op), exp(move(exp)) {
  type = Op;
}
UnaryExpAST::UnaryExpAST(
    const char *indet,
    unique_ptr<vector<unique_ptr<BaseAST>>> &args)
    : op(indet), args(move(args)) {
  type = Call;
}
void* UnaryExpAST::to_koopa() const {
  if (type == Exp || op == "+") 
    return exp->to_koopa();
  if (type == Op) {
    koopa_raw_value_data* ret = new koopa_raw_value_data();

    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_BINARY;

    auto& binary = ret->kind.data.binary;

    if (op == "-") {
      binary.op = KOOPA_RBO_SUB;
      /*ret->kind.data.integer.value =
          -(((const koopa_raw_value_data*)exp->to_koopa(parent, inst_buf))
                ->kind.data.integer.value);
      */
    }
    if (op == "!") {
      binary.op = KOOPA_RBO_EQ;
      /*
      ret->kind.data.integer.value =
          !(((const koopa_raw_value_data*)exp->to_koopa(parent, inst_buf))
                ->kind.data.integer.value);
      */
    }

    NumberAST zero(0);
    binary.lhs = (koopa_raw_value_t)zero.to_koopa();
    binary.rhs = (koopa_raw_value_t)exp->to_koopa();
    block_vector.addInst(ret);

    return ret;
  }else if (type == Call) {
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_function_t func = symbol_vector.getSymbol(op).data.func_value;
    ret->ty = func->ty->data.function.ret;
    ret->name = nullptr;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_CALL;
    ret->kind.data.call.callee = func;
    vector<const void *> arg_vec;
    //LV9
    for (int i = args->size() - 1; i >= 0; i--) {
      koopa_raw_value_t arg = (koopa_raw_value_t)(*args)[i]->to_koopa();
      arg_vec.push_back(arg);
    }
    if (arg_vec.size() == 0) {
      ret->kind.data.call.args = slice(KOOPA_RSIK_VALUE);
    } else {
      ret->kind.data.call.args = slice(arg_vec, KOOPA_RSIK_VALUE);
    }
    block_vector.addInst(ret);
    return ret;
  }
  return nullptr;
}


int UnaryExpAST::cal_value() const {
  if (type == Exp || op == "+")
    return exp->cal_value();
  if (op == "-")
    return -exp->cal_value();
  if (op == "!")
    return !exp->cal_value();
  return 0;
}

// 20. AddExpAST
AddExpAST::AddExpAST(unique_ptr<BaseAST>& mul_exp)
    : mul_exp(move(mul_exp)) {
  type = Exp;
}

AddExpAST::AddExpAST(const char* op, unique_ptr<BaseAST>& add_exp,
                     unique_ptr<BaseAST>& mul_exp)
    : op(op), add_exp(move(add_exp)), mul_exp(move(mul_exp)) {
  type = Op;
}

void* AddExpAST::to_koopa() const {
  if (type == Exp) return mul_exp->to_koopa();
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;
  auto& binary = ret->kind.data.binary;
  if (op == "+") {
    binary.op = KOOPA_RBO_ADD;
  }
  if (op == "-") {
    binary.op = KOOPA_RBO_SUB;
  }
  binary.lhs = (koopa_raw_value_t)add_exp->to_koopa();
  binary.rhs = (koopa_raw_value_t)mul_exp->to_koopa();
  block_vector.addInst(ret);
  return ret;
}

int AddExpAST::cal_value() const {
  if (type == Exp)
    return mul_exp->cal_value();
  if (op == "+")
    return add_exp->cal_value() + mul_exp->cal_value();
  if (op == "-")
    return add_exp->cal_value() - mul_exp->cal_value();
  return 0;
}

// 21. MulExpAST
MulExpAST::MulExpAST(unique_ptr<BaseAST>& unary_exp)
    : unary_exp(move(unary_exp)) {
  type = Exp;
}

MulExpAST::MulExpAST(const char* op, unique_ptr<BaseAST>& mul_exp,
                     unique_ptr<BaseAST>& unary_exp)
    : op(op), mul_exp(move(mul_exp)), unary_exp(move(unary_exp)) {
  type = Op;
}

void* MulExpAST::to_koopa() const {
  if (type == Exp) return unary_exp->to_koopa();
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;
  auto& binary = ret->kind.data.binary;
  if (op == "*") {
    binary.op = KOOPA_RBO_MUL;
  }
  if (op == "/") {
    binary.op = KOOPA_RBO_DIV;
  }
  if (op == "%") {
    binary.op = KOOPA_RBO_MOD;
  }
  binary.lhs = (koopa_raw_value_t)mul_exp->to_koopa();
  binary.rhs = (koopa_raw_value_t)unary_exp->to_koopa();
  block_vector.addInst(ret);
  return ret;
}

int MulExpAST::cal_value() const {
  if (type == Exp)
    return unary_exp->cal_value();
  if (op == "*")
    return mul_exp->cal_value() * unary_exp->cal_value();
  if (op == "/")
    return mul_exp->cal_value() / unary_exp->cal_value();
  if (op == "%")
    return mul_exp->cal_value() % unary_exp->cal_value();
  return 0;
}
// 22. RelExpAST
RelExpAST::RelExpAST(unique_ptr<BaseAST>& add_exp)
    : add_exp(move(add_exp)) {
  type = Exp;
}

RelExpAST::RelExpAST(const char* op, unique_ptr<BaseAST>& rel_exp,
                     unique_ptr<BaseAST>& add_exp)
    : op(op), rel_exp(move(rel_exp)), add_exp(move(add_exp)) {
  type = Op;
}

void* RelExpAST::to_koopa() const {
  if (type == Exp) return add_exp->to_koopa();
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;
  auto& binary = ret->kind.data.binary;
  if (op == "<") {
    binary.op = KOOPA_RBO_LT;
  }
  if (op == ">") {
    binary.op = KOOPA_RBO_GT;
  }
  if (op == "<=") {
    binary.op = KOOPA_RBO_LE;
  }
  if (op == ">=") {
    binary.op = KOOPA_RBO_GE;
  }
  binary.lhs = (koopa_raw_value_t)rel_exp->to_koopa();
  binary.rhs = (koopa_raw_value_t)add_exp->to_koopa();
  block_vector.addInst(ret);
  return ret;
}

int RelExpAST::cal_value() const {
  if (type == Exp)
    return add_exp->cal_value();
  if (op == "<")
    return rel_exp->cal_value() < add_exp->cal_value();
  if (op == ">")
    return rel_exp->cal_value() > add_exp->cal_value();
  if (op == "<=")
    return rel_exp->cal_value() <= add_exp->cal_value();
  if (op == ">=")
    return rel_exp->cal_value() >= add_exp->cal_value();
  return 0;
}

// 23. EqExpAST
EqExpAST::EqExpAST(unique_ptr<BaseAST>& rel_exp)
    : rel_exp(move(rel_exp)) {
  type = Exp;
}

EqExpAST::EqExpAST(const char* op, unique_ptr<BaseAST>& eq_exp,
                   unique_ptr<BaseAST>& rel_exp)
    : op(op), eq_exp(move(eq_exp)), rel_exp(move(rel_exp)) {
  type = Op;
}

void* EqExpAST::to_koopa() const {
  if (type == Exp) return rel_exp->to_koopa();
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;
  auto& binary = ret->kind.data.binary;
  if (op == "==") {
    binary.op = KOOPA_RBO_EQ;
  }
  if (op == "!=") {
    binary.op = KOOPA_RBO_NOT_EQ;
  }
  binary.lhs = (koopa_raw_value_t)eq_exp->to_koopa();
  binary.rhs = (koopa_raw_value_t)rel_exp->to_koopa();
  block_vector.addInst(ret);
  return ret;
}

int EqExpAST::cal_value() const {
  if (type == Exp)
    return rel_exp->cal_value();
  if (op == "==")
    return eq_exp->cal_value() == rel_exp->cal_value();
  if (op == "!=")
    return eq_exp->cal_value() != rel_exp->cal_value();
  return 0;
}

// 24. LAndExpAST
LAndExpAST::LAndExpAST(unique_ptr<BaseAST>& eq_exp)
    : eq_exp(move(eq_exp)) {
  type = Exp;
}

LAndExpAST::LAndExpAST(const char* op, unique_ptr<BaseAST>& and_exp,
                       unique_ptr<BaseAST>& eq_exp)
    : op(op), and_exp(move(and_exp)), eq_exp(move(eq_exp)) {
  type = Op;
}

void* LAndExpAST::make_bool(const unique_ptr<BaseAST>& exp) const {
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;
  auto& binary = ret->kind.data.binary;
  binary.op = KOOPA_RBO_NOT_EQ;
  /*
  binary.lhs = exp;
  koopa_raw_value_data* zero = new koopa_raw_value_data();
  zero->ty = type_kind(KOOPA_RTT_INT32);
  zero->name = nullptr;
  zero->used_by = used_by;
  zero->kind.tag = KOOPA_RVT_INTEGER;
  zero->kind.data.integer.value = 0;
  binary.rhs = (koopa_raw_value_t)zero;
  */

  binary.lhs = (koopa_raw_value_t)exp->to_koopa();
  NumberAST zero(0);
  binary.rhs = (koopa_raw_value_t)zero.to_koopa(); 
  block_vector.addInst(ret);
  return ret;
}

void* LAndExpAST::to_koopa() const {
  if (type == Exp) return eq_exp->to_koopa();
  koopa_raw_value_data *temp = new koopa_raw_value_data();
  temp->ty = pointer_type_kind(KOOPA_RTT_INT32);
  temp->name = "@temp";
  temp->used_by = slice(KOOPA_RSIK_VALUE);
  temp->kind.tag = KOOPA_RVT_ALLOC;
  block_vector.addInst(temp);
  koopa_raw_value_data *temp_store = new koopa_raw_value_data();
  temp_store->ty = type_kind(KOOPA_RTT_UNIT);
  temp_store->name = nullptr;
  temp_store->used_by = slice(KOOPA_RSIK_VALUE);
  temp_store->kind.tag = KOOPA_RVT_STORE;
  temp_store->kind.data.store.dest = temp;
  temp_store->kind.data.store.value =
      (koopa_raw_value_t)NumberAST(0).to_koopa();
  block_vector.addInst(temp_store);
  koopa_raw_value_data *branch = new koopa_raw_value_data();
  branch->ty = type_kind(KOOPA_RTT_UNIT);
  branch->name = nullptr;
  branch->used_by = slice(KOOPA_RSIK_VALUE);
  branch->kind.tag = KOOPA_RVT_BRANCH;
  branch->kind.data.branch.cond = (koopa_raw_value_t)make_bool(and_exp);
  koopa_raw_basic_block_data_t *true_block = new koopa_raw_basic_block_data_t();
  true_block->name = "%true";
  true_block->params = slice(KOOPA_RSIK_VALUE);
  true_block->used_by = slice(KOOPA_RSIK_VALUE);
  branch->kind.data.branch.true_bb = (koopa_raw_basic_block_t)true_block;
  branch->kind.data.branch.true_args = slice(KOOPA_RSIK_VALUE);
  koopa_raw_basic_block_data_t *false_block =
      new koopa_raw_basic_block_data_t();
  false_block->name = "%end";
  false_block->params = slice(KOOPA_RSIK_VALUE);
  false_block->used_by = slice(KOOPA_RSIK_VALUE);
  branch->kind.data.branch.false_bb = (koopa_raw_basic_block_t)false_block;
  branch->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);
  block_vector.addInst(branch);
  block_vector.newBlock(true_block);
  koopa_raw_value_data *true_store = new koopa_raw_value_data();
  true_store->ty = type_kind(KOOPA_RTT_UNIT);
  true_store->name = nullptr;
  true_store->used_by = slice(KOOPA_RSIK_VALUE);
  true_store->kind.tag = KOOPA_RVT_STORE;
  true_store->kind.data.store.dest = temp;
  true_store->kind.data.store.value = (koopa_raw_value_t)make_bool(eq_exp);
  block_vector.addInst(true_store);
  block_vector.addInst(jump_value(false_block));
  block_vector.newBlock(false_block);
  koopa_raw_value_data *ret = new koopa_raw_value_data();
  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_LOAD;
  ret->kind.data.load.src = temp;
  block_vector.addInst(ret);
  return ret;
}

int LAndExpAST::cal_value() const {
  if (type == Exp)
    return eq_exp->cal_value();
  return and_exp->cal_value() && eq_exp->cal_value();
}

// 25. LOrExpAST
LOrExpAST::LOrExpAST(unique_ptr<BaseAST>& and_exp)
    : and_exp(move(and_exp)) {
  type = Exp;
}

LOrExpAST::LOrExpAST(const char* op, unique_ptr<BaseAST>& or_exp,
                     unique_ptr<BaseAST>& and_exp)
    : op(op), or_exp(move(or_exp)), and_exp(move(and_exp)) {
  type = Op;
}

void* LOrExpAST::make_bool(const unique_ptr<BaseAST>& exp) const {
  koopa_raw_value_data* ret = new koopa_raw_value_data();

  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;
  auto& binary = ret->kind.data.binary;
  binary.op = KOOPA_RBO_NOT_EQ;
  /*
  binary.lhs = exp;
  koopa_raw_value_data* zero = new koopa_raw_value_data();
  zero->ty = type_kind(KOOPA_RTT_INT32);
  zero->name = nullptr;
  zero->used_by = used_by;
  zero->kind.tag = KOOPA_RVT_INTEGER;
  zero->kind.data.integer.value = 0;
  binary.rhs = (koopa_raw_value_t)zero;
  */

  binary.lhs = (koopa_raw_value_t)exp->to_koopa();
  NumberAST zero(0);
  binary.rhs = (koopa_raw_value_t)zero.to_koopa(); 
  block_vector.addInst(ret);
  return ret;
}

void* LOrExpAST::to_koopa() const {
  if (type == Exp) return and_exp->to_koopa();
  koopa_raw_value_data *temp = new koopa_raw_value_data();
  temp->ty = pointer_type_kind(KOOPA_RTT_INT32);
  temp->name = "@temp";
  temp->used_by = slice(KOOPA_RSIK_VALUE);
  temp->kind.tag = KOOPA_RVT_ALLOC;
  block_vector.addInst(temp);
  koopa_raw_value_data *temp_store = new koopa_raw_value_data();
  temp_store->ty = type_kind(KOOPA_RTT_UNIT);
  temp_store->name = nullptr;
  temp_store->used_by = slice(KOOPA_RSIK_VALUE);
  temp_store->kind.tag = KOOPA_RVT_STORE;
  temp_store->kind.data.store.dest = temp;
  temp_store->kind.data.store.value =
      (koopa_raw_value_t)NumberAST(1).to_koopa();
  block_vector.addInst(temp_store);
  koopa_raw_value_data *branch = new koopa_raw_value_data();
  branch->ty = type_kind(KOOPA_RTT_UNIT);
  branch->name = nullptr;
  branch->used_by = slice(KOOPA_RSIK_VALUE);
  branch->kind.tag = KOOPA_RVT_BRANCH;
  branch->kind.data.branch.cond = (koopa_raw_value_t)make_bool(or_exp);
  koopa_raw_basic_block_data_t *true_block = new koopa_raw_basic_block_data_t();
  true_block->name = "%end";
  true_block->params = slice(KOOPA_RSIK_VALUE);
  true_block->used_by = slice(KOOPA_RSIK_VALUE);
  branch->kind.data.branch.true_bb = (koopa_raw_basic_block_t)true_block;
  branch->kind.data.branch.true_args = slice(KOOPA_RSIK_VALUE);
  koopa_raw_basic_block_data_t *false_block =
      new koopa_raw_basic_block_data_t();
  false_block->name = "%false";
  false_block->params = slice(KOOPA_RSIK_VALUE);
  false_block->used_by = slice(KOOPA_RSIK_VALUE);
  branch->kind.data.branch.false_bb = (koopa_raw_basic_block_t)false_block;
  branch->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);
  block_vector.addInst(branch);
  block_vector.newBlock(false_block);
  koopa_raw_value_data *false_store = new koopa_raw_value_data();
  false_store->ty = type_kind(KOOPA_RTT_UNIT);
  false_store->name = nullptr;
  false_store->used_by = slice(KOOPA_RSIK_VALUE);
  false_store->kind.tag = KOOPA_RVT_STORE;
  false_store->kind.data.store.dest = temp;
  false_store->kind.data.store.value = (koopa_raw_value_t)make_bool(and_exp);
  block_vector.addInst(false_store);
  block_vector.addInst(jump_value(true_block));
  block_vector.newBlock(true_block);
  koopa_raw_value_data *ret = new koopa_raw_value_data();
  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_LOAD;
  ret->kind.data.load.src = temp;
  block_vector.addInst(ret);
  return ret;
}

int LOrExpAST::cal_value() const {
  if (type == Exp)
    return and_exp->cal_value();
  return or_exp->cal_value() || and_exp->cal_value();
}

// 26. NumberAST
NumberAST::NumberAST(int val) : val(val) {}

//string NumberAST::to_string() const {
//  return "NumberAST { int " + to_string(val) + " }";
//}

void* NumberAST::to_koopa() const {
  koopa_raw_value_data* ret = new koopa_raw_value_data();
  ret->ty = type_kind(KOOPA_RTT_INT32);
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_INTEGER;
  //ret->kind.data.integer.value = val % 256;
  ret->kind.data.integer.value = val;
  return ret;
}
int NumberAST::cal_value() const { 
  //return val % 256; 
  return val; 
}
