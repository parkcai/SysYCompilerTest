// 本文件作用语义分析器,将AST数据结构转换成文本形式的Koopa IR
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "ast.hpp"

using namespace std;

static int koopacnt = 0;  // 用于记录将要使用的寄存器编号
static int ifcnt = 0; // 用于记录if语句的数量
static int whilecnt = 0; // 用于记录while语句的数量
static int whilecur = 0; // 用于记录当前while语句的编号
static unordered_map<int, int> while_pair; // 用于记录while语句的父子关系(即多重循环下退回到哪一层)
static bool entryend = false; // 用于记录是否已经读到了一条return/jump/branch语句,这标志着一个块的结束

static bool dec_global_var = false; // 用于记录是否正在声明全局变量

// 处理全局常量或变量数组的初始化,即为aggregate产生的vector在合适的位置加上括号,并输出
static void global_handle_aggregate(const vector<int>& mul_len, const vector<int>& ret) {
  for (int pos = 0; pos < ret.size(); pos++) {
    for (int tmp = 0; tmp < mul_len.size(); tmp++) {
      if (pos % mul_len[tmp] == 0) {
        cout << "{";
      }
    }
    cout << ret[pos];
    for (int tmp = 0; tmp < mul_len.size(); tmp++) {
      if ((pos + 1) % mul_len[tmp] == 0) {
        cout << "}";
      }
    }
    if (pos != ret.size() - 1) {
      cout << ", ";
    }
  }
}

// 处理局部常量或变量数组的初始化,需要用到store和getelemptr指令
static void local_handle_aggregate(string ident, int symtid, const vector<int>& ret, int pos, const vector<int>& len, const vector<int>& mul_len, int cur) {
  if (cur == len.size()) {
    cout << "  store " << ret[pos] << ", %" << koopacnt - 1 << endl;
  }
  else {
    int size = mul_len[cur];
    size /= len[cur];
    int parent_ptr = koopacnt - 1;
    for (int i = 0; i < len[cur]; i++) {
      cout << "  %" << koopacnt << " = getelemptr ";
      if (cur == 0){
        cout << "@" << ident << "_" << symtid;
      }
      else {
        cout << "%" << parent_ptr;
      }
      cout << ", " << i << endl;
      koopacnt++;
      local_handle_aggregate(ident, symtid, ret, pos + i * size, len, mul_len, cur + 1);
    }
  }
}
 // 专门用于处理局部变量数组的初始化,此时ret vector中存放的是虚拟寄存器编号
static void local_var_handle_aggregate(string ident, int symtid, const vector<int>& ret, int pos, const vector<int>& len, const vector<int>& mul_len, int cur) {
  if (cur == len.size()) {  // 只需要对store指令稍作修改即可
    if (ret[pos] == -1) {
      cout << "  store 0, %" << koopacnt - 1 << endl;
    }
    else {
      cout << "  store %" << ret[pos] << ", %" << koopacnt - 1 << endl;
    }
  }
  else {
    int size = mul_len[cur];
    size /= len[cur];
    int parent_ptr = koopacnt - 1;
    for (int i = 0; i < len[cur]; i++) {
      cout << "  %" << koopacnt << " = getelemptr ";
      if (cur == 0){
        cout << "@" << ident << "_" << symtid;
      }
      else {
        cout << "%" << parent_ptr;
      }
      cout << ", " << i << endl;
      koopacnt++;
      local_var_handle_aggregate(ident, symtid, ret, pos + i * size, len, mul_len, cur + 1);
    }
  }
}

// CompUnit ::= CompUnitItemList
// CompUnitItemList ::= CompUnitItem | CompUnitItemList CompUnitItem  
void CompUnitAST::KoopaIR() const {

  enter_code_block(); // 进入全局作用域

  // 声明库函数并插入全局符号表
  cout << "decl @getint(): i32" << endl;
  cout << "decl @getch(): i32" << endl;
  cout << "decl @getarray(*i32): i32" << endl;
  cout << "decl @putint(i32)" << endl;
  cout << "decl @putch(i32)" << endl;
  cout << "decl @putarray(i32, *i32)" << endl;
  cout << "decl @starttime()" << endl;
  cout << "decl @stoptime()" << endl;
  cout << endl;
  insert_symbol("getint", SYM_TYPE_FUNCINT, 0);
  insert_symbol("getch", SYM_TYPE_FUNCINT, 0);
  insert_symbol("getarray", SYM_TYPE_FUNCINT, 0);
  insert_symbol("putint", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("putch", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("putarray", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("starttime", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("stoptime", SYM_TYPE_FUNCVOID, 0);

  // 访问所有 CompUnitItem
  for(auto& comp_unit_item: *comp_unit_item_list) {
    comp_unit_item->KoopaIR();
  }

  exit_code_block();
}

// CompUnitItem ::= Decl | FuncDef;
void CompUnitItemAST::KoopaIR() const {
  if (type == 1) {
    dec_global_var = true;
    decl->KoopaIR();
    dec_global_var = false;
  }
  else if (type == 2) {
    func_def->KoopaIR();
  }
}


// Decl ::= ConstDecl | VarDecl
void DeclAST::KoopaIR() const {
  if (type == 1) {
    const_decl->KoopaIR();
  }
  else if (type == 2) {
    var_decl->KoopaIR();
  }
}

// ConstDecl ::= CONST Type ConstDefList ';'
// ConstDefList ::= ConstDef | ConstDefList ',' ConstDef
void ConstDeclAST::KoopaIR() const {
  for(auto &const_def : *const_def_list) {
    const_def->KoopaIR();
  }
}

// ConstDef ::= IDENT ConstIndexList'=' CosntInitVal
// ConstIndexList ::= ε | ConstIndexList '[' ConstExp ']'
void ConstDefAST::KoopaIR() const {
  if (const_index_list->empty()) {  //常量
    int tmp = dynamic_cast<ConstInitValAST*>(const_init_val.get())->getValue();
    insert_symbol(ident, SYM_TYPE_CONST, tmp);  //将常量的名字与值存入字典
  }
  else {  // 常量数组
    int symtid = insert_symbol(ident, SYM_TYPE_CONSTARRAY, const_index_list->size());
    
    if (dec_global_var) { // 如果是全局常量数组
      cout << "global ";
    }
    else {
      cout << "  ";
    }

    cout << "@" << ident << "_" << symtid << " = alloc";
    // 输出数组维度
    for (int i = 0; i < const_index_list->size(); i++) {
      cout << "[";
    }
    cout << "i32, ";

    
    auto len = new vector<int>();
    auto mul_len = new vector<int>();
    for (int i = const_index_list->size() - 1; i >= 0; i--) {
      const auto& const_exp = (*const_index_list)[i];
      int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->getValue(); //从后向前求出维度具体值
      cout << tmp << "], ";
      len->push_back(tmp);
      if (mul_len->empty()) {
        mul_len->push_back(tmp);
      }
      else {
        mul_len->push_back(tmp * mul_len->back());
      }
    }
    reverse(len->begin(), len->end());  // 记录数组维度并将维度反转,用于后续的初始化
    reverse(mul_len->begin(), mul_len->end());  // 记录数组维度乘积并将维度乘积反转,用于后续的初始化
    // 例如,int[2][3][4]----->len = {2,3,4}----->mul_len={4*3*2,4*3,4}

    auto ret = dynamic_cast<ConstInitValAST*>(const_init_val.get())->make_aggregate(*mul_len, 0);

    if (dec_global_var) { 
      // 如果是全局常量数组,KoopaIR形如
      // global @x_0 = alloc [[[i32, 4], 3], 2], {
      // {{1, 2, 3, 4}, {5, 0, 0, 0}, {6, 0, 0, 0}},
      // {{7, 8, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}}
      global_handle_aggregate(*mul_len, ret);
      cout << endl; 
    }
    else {
      // 如果是局部常量数组,KoopaIR形如
      // @x_0 = alloc [[[i32, 4], 3], 2]
      // %1 = getelemptr @x_0, 0
      // %2 = getelemptr %1, 0
      // %3 = getelemptr %2, 0
      // store 1, %3 //  完成对array[0][0][0]的赋值
      // %4 = getelemptr %2, 1
      // store 2, %4 //  完成对array[0][0][1]的赋值,依此类推

      // 回退擦除最后一个逗号
      cout.seekp(-2, cout.end);
      cout << endl;
      local_handle_aggregate(ident, symtid, ret, 0, *len, *mul_len, 0);  
    }

    delete len;
    delete mul_len;
  }
}

// ConstInitVal     ::= ConstExp | ConstArrayInitVal;
// ConstArrayInitVal::= '{' '}' | '{' ConstInitValList '}';
// ConstInitValList ::= ConstInitVal | ConstInitValList ',' ConstInitVal;
void ConstInitValAST::KoopaIR() const {
  return;
}

int ConstInitValAST::getValue() const {
  assert(type == 1);  // 确保是常量表达式,而不是常量数组的初始化
  return dynamic_cast<ConstExpAST*>(const_exp.get())->getValue();
}

// 计算并补全初始化列表, 返回的 vector<int> 中即为补足 0 的各项的值
// 例如,对于int[2][3][4],初始化列表为{1,2,3,4,{5},{6},{7,8}},mul_len={4*3*2,4*3,4},pos=0
vector<int> ConstInitValAST::make_aggregate(const vector<int>& mul_len, int pos) const {
  assert(type == 2);  // 确保是常量数组的初始化,而不是常量表达式
  vector<int> ret;
  int size = mul_len.size();
  for(auto& const_init_val: *const_init_val_list) {
    // 依次处理初始化列表内的元素, 元素的形式无非就两种可能: 整数, 或者另一个初始化列表
    auto tmp = dynamic_cast<ConstInitValAST*>(const_init_val.get());

    if (tmp->type == 1) { // ConstExp
      ret.push_back(tmp->getValue());
    }
    else if (tmp->type == 2) {  // 另一个初始化列表,例如{5}
      int tmp_pos = pos + 1;
      int flag = 0;
      for (; tmp_pos < size; tmp_pos++) {
        if (ret.size() % mul_len[tmp_pos] == 0) { // 此时找到pos=2,维度=4满足条件(此时已填入1,2,3,4)
          auto child_ret = tmp->make_aggregate(mul_len, tmp_pos); // 利用初始化列表{5}去构造int[4]
          for(int i = 0; i < child_ret.size(); i++) {
            ret.push_back(child_ret[i]);
          } // 将结果与之前的ret vector合并
          flag++;
          break;
        }
      }
      if (flag == 0){
        assert(0);  // 如果没有找到合适的维度,说明初始化列表有误,是语义错误
      }
    }
  }
  ret.insert(ret.end(), mul_len[pos] - ret.size(), 0);  // 将剩下的部分补0
  
  return ret;
}

// VarDecl ::= Type VarDefList ';'
// VarDefList ::= VarDef | VarDefList ',' VarDef
void VarDeclAST::KoopaIR() const {
  for (auto &var_def : *var_def_list) {
    var_def->KoopaIR();
  }
}

// VarDef ::= IDENT ConstIndexList | IDENT ConstIndexList '=' InitVal
void VarDefAST::KoopaIR() const {
  if (const_index_list->empty()) {  // 单变量

    int symtid = insert_symbol(ident, SYM_TYPE_VAR, 0);  // 将变量的名字存入字典
    if (dec_global_var) { // 如果是全局变量
      if (type == 1){
        cout << "global @" << ident << "_" << symtid << " = alloc i32, zeroinit" << endl;
      }
      else if (type == 2) {
        int tmp = dynamic_cast<InitValAST*>(init_val.get())->getValue();
        cout << "global @" << ident << "_" << symtid << " = alloc i32, " << tmp << endl; 
      }
    }

    else {  // 如果是局部变量
      if (type == 1) {
        cout << "  @" << ident << "_" << symtid << " = alloc i32" << endl;
      }
      else if (type == 2) {
        init_val->KoopaIR();
        cout << "  @" << ident << "_" << symtid << " = alloc i32" << endl;
        cout << "  store %" << koopacnt - 1 << ", @" << ident << "_" << symtid << endl;
      }
    }
  }

  else {  // 变量数组,与常量数组几乎相同
    int symtid = insert_symbol(ident, SYM_TYPE_ARRAY, const_index_list->size());
    if (dec_global_var) { // 如果是全局数组
      cout << "global ";
    }
    else {
      cout << "  ";
    }

    cout << "@" << ident << "_" << symtid << " = alloc";
    // 输出数组维度
    for (int i = 0; i < const_index_list->size(); i++) {
      cout << "[";
    }
    cout << "i32, ";

    auto len = new vector<int>();
    auto mul_len = new vector<int>();
    for (int i = const_index_list->size() - 1; i >= 0; i--) {
      const auto& const_exp = (*const_index_list)[i];
      int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->getValue(); //从后向前求出维度具体值
      cout << tmp << "], ";
      len->push_back(tmp);
      if (mul_len->empty()) {
        mul_len->push_back(tmp);
      }
      else {
        mul_len->push_back(tmp * mul_len->back());
      }
    }
    reverse(len->begin(), len->end());  
    reverse(mul_len->begin(), mul_len->end());  
    
    if (dec_global_var) { 
      if (type == 1) {  // 这里是全局变量数组与全局常量数组的区别,如果变量数组不显式初始化,则输出zeroinit
        cout <<" zeroinit" << endl;
      }
      else{
        auto ret = dynamic_cast<InitValAST*>(init_val.get())->make_aggregate(*mul_len, 0);
        global_handle_aggregate(*mul_len, ret);
        cout << endl; 
      }
    }
    else {
      if (type == 1) {
        // 回退擦除最后一个逗号
        cout.seekp(-2, cout.end);
        cout << endl;
        vector<int> ret((*mul_len)[0], -1);
        local_var_handle_aggregate(ident, symtid, ret, 0, *len, *mul_len, 0);
      }
      else {
        // 回退擦除最后一个逗号
        cout.seekp(-2, cout.end);
        cout << endl;
        auto ret = dynamic_cast<InitValAST*>(init_val.get())->make_aggregate(*mul_len, 0);
        local_var_handle_aggregate(ident, symtid, ret, 0, *len, *mul_len, 0);
      }
    }

    delete len;
    delete mul_len;
  }
}


// InitVal          ::= Exp | ArrayInitVal;
// ArrayInitVal     ::= '{' '}' | '{' InitValList '}';
// InitValList      ::= InitVal | InitValList ',' InitVal;

void InitValAST::KoopaIR() const {
  assert(type == 1);  // 确保是表达式
  exp->KoopaIR();
}

int InitValAST::getValue() const {
  assert(type == 1);  // 确保是表达式
  return dynamic_cast<ExpAST*>(exp.get())->getValue();
}


vector<int> InitValAST::make_aggregate(const vector<int>& mul_len, int pos) const {
  assert(type == 2);  // 确保是数组初始化
  
  if (dec_global_var) { // 语义规定,全局数组变量的初始化列表中只能出现常量表达式
    // 因此这里的逻辑和常量数组完全相同
    vector<int> ret;
    int size = mul_len.size();
    for(auto& init_val: *init_val_list) {
      // 依次处理初始化列表内的元素, 元素的形式无非就两种可能: 整数, 或者另一个初始化列表
      auto tmp = dynamic_cast<InitValAST*>(init_val.get());

      if (tmp->type == 1) { // ConstExp
        ret.push_back(tmp->getValue());
      }
      else if (tmp->type == 2) {  
        int tmp_pos = pos + 1;
        int flag = 0;
        for (; tmp_pos < size; tmp_pos++) {
          if (ret.size() % mul_len[tmp_pos] == 0) { 
            auto child_ret = tmp->make_aggregate(mul_len, tmp_pos); 
            for(int i = 0; i < child_ret.size(); i++) {
              ret.push_back(child_ret[i]);
            } // 将结果与之前的ret vector合并
            flag++;
            break;
          }
        }
        if (flag == 0){
          assert(0);  // 如果没有找到合适的维度,说明初始化列表有误,是语义错误
        }
      }
    }
    
    ret.insert(ret.end(), mul_len[pos] - ret.size(), 0);  // 将剩下的部分补0
    return ret;
  }

  else {  // 如果是局部变量数组,则初始化列表中可能含有变量表达式
    // 因此我们这里稍作修改,先计算出变量表达式的值存放在虚拟寄存器%tmp中,再将tmp编号依次存入ret vector中
    // 如果是要补0的部分,则编号记为-1
    // 最后ret vector返回形如{%1, %2, %3...,-1, -1, -1}
    vector<int> ret;
    int size = mul_len.size();
    for(auto& init_val: *init_val_list) {
      // 依次处理初始化列表内的元素, 元素的形式无非就两种可能: 整数, 或者另一个初始化列表
      auto tmp = dynamic_cast<InitValAST*>(init_val.get());

      if (tmp->type == 1) { // Exp
        tmp->KoopaIR();
        ret.push_back(koopacnt - 1);
      }
      else if (tmp->type == 2) {  
        int tmp_pos = pos + 1;
        int flag = 0;
        for (; tmp_pos < size; tmp_pos++) {
          if (ret.size() % mul_len[tmp_pos] == 0) { 
            auto child_ret = tmp->make_aggregate(mul_len, tmp_pos); 
            for(int i = 0; i < child_ret.size(); i++) {
              ret.push_back(child_ret[i]);
            } // 将结果与之前的ret vector合并
            flag++;
            break;
          }
        }
        if (flag == 0){
          assert(0);  // 如果没有找到合适的维度,说明初始化列表有误,是语义错误
        }
      }
    }
    ret.insert(ret.end(), mul_len[pos] - ret.size(), -1);  // 将剩下的部分补0
    return ret;
  }

}



// FuncDef ::= Type Ident '(' FuncFParams ')' Block
// FuncFParams ::= ε | FuncFParamList 
// FuncFParamList ::= FuncFParam | FuncFParamList ',' FuncFParam
void FuncDefAST::KoopaIR() const {

  // 插入函数符号Ident(之前先检查是否已经存在)
  if (!exist_symbol(ident)) {
    if (Type == "void") {
      insert_symbol(ident, SYM_TYPE_FUNCVOID, 0);
    }
    else if (Type == "int") {
      insert_symbol(ident, SYM_TYPE_FUNCINT, 0);
    }
  }

  enter_code_block(); // 进入函数作用域

  cout << endl;
  cout << "fun @" << ident << "(";

  for(auto& func_f_param: *func_f_param_list) { // 输出所有函数参数
    func_f_param->KoopaIR();
    cout << ", ";
  }
  // 退格擦除最后一个逗号
  if(!func_f_param_list->empty()){
    cout.seekp(-2, cout.end);
  }
  cout << ")";

  if (Type == "int") {
    cout << ": i32";
  }

  cout << "{" << endl;
  cout << "%entry:" << endl;
  entryend = false; 

  for(auto& func_f_param: *func_f_param_list) {

    auto param = dynamic_cast<FuncFParamAST*>(func_f_param.get());
    if (param->type == 1) { // 函数参数为变量
      // 将函数参数存入符号表
      string ident = param->ident;
      insert_symbol(ident, SYM_TYPE_VAR, 0);

      // 分配内存空间
      // @x_id = alloc i32
      cout << "  @" << ident << "_" << get_current_code_block() << " = alloc i32" << endl;
    
      // store @x, @x_id
      cout << "  store @" << ident << ", @" << ident << "_" << get_current_code_block() << endl;
    }
    else if (param->type == 2) { // 函数参数为数组
      // 将函数参数存入符号表
      string ident = param->ident;
      insert_symbol(ident, SYM_TYPE_PTR, param->const_index_list->size() + 1);

      // 分配内存空间
      // @x_id = alloc *i32
      cout << "  @" << ident << "_" << get_current_code_block() << " = alloc ";
      
      string str = "*";
      for (int i = 0; i < param->const_index_list->size(); i++) {
        str += "[";
      }
      str += "i32, ";
      for (int i = param->const_index_list->size() - 1; i >= 0; i--) {
        const auto& const_exp = (*(param->const_index_list))[i];
        int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->getValue();
        str += to_string(tmp) + "], ";
      }
      str.pop_back();
      str.pop_back(); // 删除最后一个",] "
      cout << str <<endl;
    
      // store @x, @x_id
      cout << "  store @" << ident << ", @" << ident << "_" << get_current_code_block() << endl;
    }

    
  }


  block->KoopaIR();

  // 如果函数内部没有return语句,则在函数末尾加上ret 0
  if (!entryend) {
    if (Type == "int")
      cout << "  ret 0" << endl;
    else if (Type == "void")
      cout << "  ret" << endl;
  }

  cout << "}" << endl;

  exit_code_block(); // 退出函数作用域

}

// FuncFParam ::= Type IDENT | Type IDENT '[' ']' ConstIndexList
void FuncFParamAST::KoopaIR() const {
  if (type == 1) {
    cout << "@" << ident << ": ";
    if (Type == "int"){
      cout << "i32";
    }
  }
  else if (type == 2) { // 给出数组函数参数的类型,如*[i32, 3](具体维度取决于ConstIndexList)
    cout << "@" << ident << ": ";
    string str = "*";
    for (int i = 0; i < const_index_list->size(); i++) {
      str += "[";
    }
    str += "i32, ";
    for (int i = const_index_list->size() - 1; i >= 0; i--) {
      const auto& const_exp = (*const_index_list)[i];
      int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->getValue();
      str += to_string(tmp) + "], ";
    }
    str.pop_back();
    str.pop_back(); // 删除最后一个",] "
    cout << str;
  }
}

// Block ::= '{' BlockItemList '}'
void BlockAST::KoopaIR() const {

  enter_code_block();
  for(auto &block_item : *block_item_list) {
    if (entryend) {
      break;
    }
    block_item->KoopaIR();
  }
  exit_code_block();
}

// BlockItem ::= Decl | Stmt
void BlockItemAST::KoopaIR() const {
  if (type == 1) {
    decl->KoopaIR();
  }
  else if (type == 2) {
    stmt->KoopaIR();
  }
}

// Stmt ::= Matched_Stmt | Open_Stmt
void StmtAST::KoopaIR() const {
  if (type == 1) {
    matched_stmt->KoopaIR();
  }
  else if (type == 2) {
    open_stmt->KoopaIR();
  }
}

// Matched_Stmt ::= LVal '=' Exp ';' | ';' | Exp ';' | Block | "return" ';' | "return" Exp ';'
//                | IF '(' Exp ')' Matched_Stmt ELSE Matched_Stmt
void MatchedStmtAST::KoopaIR() const {

  if (entryend){
    return;
  }

  if (type == 1) {
    exp->KoopaIR();
    int exp_koopacnt = koopacnt - 1;
    auto lvalptr = dynamic_cast<LValAST*>(lval.get());
    auto string_name = lvalptr->ident;
    auto iter = query_symbol(string_name);
    int symtid = iter.first;
    auto val = iter.second;
    
    if (val->type == SYM_TYPE_VAR) {
      // LVal为一个变量
      cout << "  store %" << koopacnt - 1 << ", @" << string_name << "_" << symtid << endl;
    }

    else if (val->type == SYM_TYPE_ARRAY) {
      // LVal 为一个数组项
      for (int i = 0; i < val->value; i++) {
        int last_koopacnt = koopacnt - 1;
        const auto& exp_index = (*(lvalptr->index_list))[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int index_koopacnt = koopacnt - 1;

        cout << "  %" << koopacnt << " = getelemptr ";
        if (i == 0) {
          cout << "@" << string_name << "_" << symtid;
        }
        else {
          cout << "%" << last_koopacnt;
        }
        cout << ", %" << index_koopacnt << endl;
        koopacnt++;
      } //这一部分与LVal中对数组元素的访问相同,只是最后一步store操作修改数组元素
      cout << "  store %" << exp_koopacnt << ", %" << koopacnt - 1 << endl;
    }

    else if (val->type == SYM_TYPE_PTR) {
      // LVal 为一个指针
      cout << "  %" << koopacnt << " = load @" << string_name << "_" << symtid << endl;
      koopacnt++;
      for (int i = 0; i < val->value; i++) {
        int last_koopacnt = koopacnt - 1;
        const auto& exp_index = (*(lvalptr->index_list))[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int index_koopacnt = koopacnt - 1;
      if (i == 0) {
        cout << "  %" << koopacnt << " = getptr %";
      }
      else {
        cout << "  %" << koopacnt << " = getelemptr %";
      }
      cout << last_koopacnt << ", %" << index_koopacnt << endl;
      koopacnt++;
    } // 这一部分与LVal中使用数组指针对数组元素的访问相同,只是最后一步store操作修改数组元素
    cout << "  store %" << exp_koopacnt << ", %" << koopacnt - 1 << endl;
  }
  
    
  }
  else if (type == 2) {
    // do nothing
  }
  else if (type == 3) {
    exp->KoopaIR();
  }
  else if (type == 4) {
    block->KoopaIR();
  }
  else if (type == 5) {
    cout << "  ret" << endl;
    entryend = true;
  }
  else if (type == 6) {
    exp->KoopaIR();
    cout << "  ret %" << koopacnt - 1 << endl;
    entryend = true;
  }
  else if (type == 7) {
    exp->KoopaIR();
    int iftmp = ifcnt;  // 用于命名语句块
    ifcnt++;

    cout << "  br %" << koopacnt - 1 << ", %THEN_" << iftmp << ", %ELSE_" << iftmp << endl;

    // %THEN_id: 创建新的then_entry
    cout << "%THEN_" << iftmp << ":" << endl;
    entryend = false;
    matched_stmt1->KoopaIR();
    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %ELSE_id: 创建新的else_entry
    cout << "%ELSE_" << iftmp << ":" << endl;
    entryend = false;
    matched_stmt2->KoopaIR();
    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %END_id: 创建新的end_entry,是if语句和else语句的交会点,并开始下一个语句块
    cout << "%END_" << iftmp << ":" << endl;
    entryend = false;
  }
  else if (type == 8) {
    
    int while_father = whilecur;
    whilecur = whilecnt;
    whilecnt++;
    while_pair[whilecur] = while_father;

    //   jump %while_entry
    // %while_entry:
    //   %cond = Exp
    //   br %cond, %while_body, %while_end
    // %while_body:
    //   Matched_Stmt
    //   jump %while_entry
    // %while_end:

    // jump %while_entry
    cout << "  jump %WHILE_ENTRY_" << whilecur << endl;

    // %WHILE_ENTRY_id: 创建新的while_entry
    cout << "%WHILE_ENTRY_" << whilecur << ":" << endl;
    entryend = false;
    exp->KoopaIR();
    cout << "  br %" << koopacnt - 1 << ", %WHILE_BODY_" << whilecur << ", %WHILE_END_" << whilecur << endl;
    
    // %WHILE_BODY_id: 创建新的while_body
    cout << "%WHILE_BODY_" << whilecur << ":" << endl;
    entryend = false;
    matched_stmt1->KoopaIR();
    if(!entryend){
      cout << "  jump %WHILE_ENTRY_" << whilecur << endl;
    }

    // %WHILE_END_id: 创建新的while_end
    cout << "%WHILE_END_" << whilecur << ":" << endl;
    entryend = false; 
    whilecur = while_pair[whilecur];  // 退回到父while语句
  }
  else if (type == 9) { // break
    // jump %while_end
    cout << "  jump %WHILE_END_" << whilecur << endl;
    entryend = true;  // 当前块结束
  }
  else if (type == 10) { // continue
    // jump %while_entry
    cout << "  jump %WHILE_ENTRY_" << whilecur << endl;
    entryend = true;  // 当前块结束
  }
}

// Open_Stmt ::= IF '(' Exp ')' Stmt | IF '(' Exp ')' Matched_Stmt ELSE Open_Stmt
void OpenStmtAST::KoopaIR() const {
  if (type == 1) {
    exp->KoopaIR();
    int iftmp = ifcnt;  // 用于命名语句块
    ifcnt++;

    cout << "  br %" << koopacnt - 1 << ", %THEN_" << iftmp << ", %END_" << iftmp << endl;

    // %THEN_id: 创建新的then_entry
    cout << "%THEN_" << iftmp << ":" << endl;
    entryend = false;
    stmt->KoopaIR();
    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %END_id: 创建新的end_entry
    cout << "%END_" << iftmp << ":" << endl;
    entryend = false;
  }
  else if (type == 2) {
    exp->KoopaIR();
    int iftmp = ifcnt;  // 用于命名语句块
    ifcnt++;

    cout << "  br %" << koopacnt - 1 << ", %THEN_" << iftmp << ", %ELSE_" << iftmp << endl;

    // %THEN_id: 创建新的then_entry
    cout << "%THEN_" << iftmp << ":" << endl;
    entryend = false;
    matched_stmt->KoopaIR();
    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %ELSE_id: 创建新的else_entry
    cout << "%ELSE_" << iftmp << ":" << endl;
    entryend = false;
    open_stmt->KoopaIR();
    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %END_id: 创建新的end_entry
    cout << "%END_" << iftmp << ":" << endl;
    entryend = false;
  }
  else if (type == 3) {
    exp->KoopaIR();
    int while_father = whilecur;
    whilecur = whilecnt;
    whilecnt++;
    while_pair[whilecur] = while_father;

    //   jump %while_entry
    // %while_entry:
    //   %cond = Exp
    //   br %cond, %while_body, %while_end
    // %while_body:
    //   Open_Stmt
    //   jump %while_entry
    // %while_end:

    // jump %while_entry
    cout << "  jump %WHILE_ENTRY_" << whilecur << endl;

    // %WHILE_ENTRY_id: 创建新的while_entry
    cout << "%WHILE_ENTRY_" << whilecur << ":" << endl;
    entryend = false;
    exp->KoopaIR();
    cout << "  br %" << koopacnt - 1 << ", %WHILE_BODY_" << whilecur << ", %WHILE_END_" << whilecur << endl;
    
    // %WHILE_BODY_id: 创建新的while_body
    cout << "%WHILE_BODY_" << whilecur << ":" << endl;
    entryend = false;
    open_stmt->KoopaIR();
    if(!entryend){
      cout << "  jump %WHILE_ENTRY_" << whilecur << endl;
    }

    // %WHILE_END_id: 创建新的while_end
    cout << "%WHILE_END_" << whilecur << ":" << endl;
    entryend = false;
    whilecur = while_pair[whilecur];  // 退回到父while语句
  }
}


// Exp ::= LOrExp
void ExpAST::KoopaIR() const {
  lor_exp->KoopaIR();
}

int ExpAST::getValue() const {
  return dynamic_cast<ExpBaseAST*>(lor_exp.get())->getValue();
}

//LVal ::= Ident
void LValAST::KoopaIR() const {
  auto iter = query_symbol(ident);
  int symtid = iter.first;
  auto val = iter.second;

  assert(val->type != SYM_TYPE_UND);

  if (val->type == SYM_TYPE_CONST) {
    cout << "  %" << koopacnt << " = add 0, " << val->value << endl;
    koopacnt++;
  }

  else if(val->type == SYM_TYPE_VAR) {
    // 从内存读取 LVal
    // %0 = load @x
    cout << "  %" << koopacnt << " = load @" << ident << "_" << symtid << endl;
    koopacnt++;
  }

  else if (val->type == SYM_TYPE_ARRAY || val->type == SYM_TYPE_CONSTARRAY) {
    if (val->value == index_list->size()) {  // 访问的是数组元素,需要返回一个int类型的数
      // 例如,对于数组a[2][2]
      // @a = alloc [[i32, 2], 2] // @a的类型为*[[i32, 2], 2]
      // 对a[1][1]的访问应该返回一个int,需要经过2次getelemptr,与index_list=2的大小相同
      // %0 = getelemptr @a, 1  // %0的类型为*[i32, 2]
      // %1 = getelemptr %0, 1  // %1的类型为*i32
      // %2 = load %1  // %2的类型为i32

      // 依次getelemptr即可
      for (int i = 0; i < index_list->size(); i++) {
        int last_koopacnt = koopacnt - 1; // 记录原先指针
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        // 此时索引值(偏移量)也可能是变量表达式,需要先计算出结果,存放在虚拟寄存器中
        int index_koopacnt = koopacnt - 1;  // 纪录偏移量

        cout << "  %" << koopacnt << " = getelemptr ";  // 新指针
        if (i == 0) {
          cout << "@" << ident << "_" << symtid;
        }
        else {
          cout << "%" << last_koopacnt;
        }
        cout << ", %" << index_koopacnt << endl;
        koopacnt++;
      }
      cout << "  %" << koopacnt << " = load %" << koopacnt - 1 << endl;
      // 将指针所指位置加载到虚拟寄存器,完成对数组元素的访问
      koopacnt++;
    }

    else {  // 给出的index_list长度小于数组维数,称为数组的部分解引用(只会用于数组参数),此时需要返回一个指针
      // 例如,对于数组a[2][2]
      // @a = alloc [[i32, 2], 2] // @a的类型为*[[i32, 2], 2]
      // 对a[1]的访问应该返回一个*int,需要经过2次getelemptr,比index_list=1的大小多1次
      // %0 = getelemptr @a, 1  // %0的类型为*[i32, 2]
      // %1 = getelemptr %0, 0  // %1的类型为*i32
      for (int i = 0; i < index_list->size(); i++) {
        int last_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int index_koopacnt = koopacnt - 1;

        cout << "  %" << koopacnt << " = getelemptr ";
        if (i == 0) {
          cout << "@" << ident << "_" << symtid;
        }
        else {
          cout << "%" << last_koopacnt;
        }
        cout << ", %" << index_koopacnt << endl;
        koopacnt++;
      }
      
      cout << "  %" << koopacnt << " = getelemptr ";
      if (index_list->size() == 0) {
        cout << "@" << ident << "_" << symtid;
      }
      else {
        cout << "%" << koopacnt - 1;
      }
      cout << ", 0" << endl;
      koopacnt++;
    }
  }

   else if(val->type == SYM_TYPE_PTR) {
    if(val->value == index_list->size()) {
      // 访问的是数组元素
      // 例如,对于数组a[][2]
      // @a = alloc *[i32, 2] // @a的类型为**[i32, 2]
      // 对a[1][1]的访问应该返回一个int,需要经过1次getptr和1次getelemptr,与index_list=2的大小相同
      // %0 = load @a  // %0的类型为*[i32, 2]
      // %1 = getptr %0, 1  // %1的类型为*[i32, 2]
      // %2 = getelemptr %1, 1  // %2的类型为*i32
      // %3 = load %2  // %3的类型为i32
      // 事实上,第一次用getptr,之后都用getelemptr即可
      cout << "  %" << koopacnt << " = load @" << ident << "_" << symtid << endl;
      koopacnt++;
      for (int i = 0; i < index_list->size(); i++) {
        int last_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int index_koopacnt = koopacnt - 1;
        if (i == 0) {
          cout << "  %" << koopacnt << " = getptr %";
        }
        else {
          cout << "  %" << koopacnt << " = getelemptr %";
        }
        cout << last_koopacnt << ", %" << index_koopacnt << endl;
        koopacnt++;
      }

      cout << "  %" << koopacnt << " = load %" << koopacnt - 1 << endl;
      koopacnt++;
    }
    else {
      // 部分解引用
      // 例如,对于数组a[][2]
      // @a = alloc *[i32, 2] // @a的类型为**[i32, 2]
      // 对a[1]的访问应该返回一个*int,需要经过1次getptr和1次getelemptr,比index_list=1的大小多1次
      // %0 = load @a  // %0的类型为*[i32, 2]
      // %1 = getptr %0, 1  // %1的类型为*[i32, 2]
      // %2 = getelemptr %1, 0  // %2的类型为*i32
      // 事实上,第一次用getptr,之后都用getelemptr即可
      cout << "  %" << koopacnt << " = load @" << ident << "_" << symtid << endl;
      koopacnt++;
      for (int i = 0; i < index_list->size(); i++) {
        int last_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int index_koopacnt = koopacnt - 1;
        if(i == 0) {
          cout << "  %" << koopacnt << " = getptr %";
        }
        else {
          cout << "  %" << koopacnt << " = getelemptr %";
        }
        cout << last_koopacnt << ", %" << index_koopacnt << endl;
        koopacnt++;
      }
      if(index_list->size() == 0) {
        cout << "  %" << koopacnt << " = getptr %";
      }
      else {
        cout << "  %" << koopacnt << " = getelemptr %";
      }
      cout << koopacnt - 1 << ", 0" << endl;
      koopacnt++;
    }
  }

  else {
    assert(0);
  }

}

int LValAST::getValue() const {
  auto val = query_symbol(ident).second;
  assert(val->type == SYM_TYPE_CONST);
  return val->value;
}


// PrimaryExp  ::= "(" Exp ")" | LVal | Number;
void PrimaryExpAST::KoopaIR() const {
  if (type == 1) {
    exp->KoopaIR();
  }
  else if (type == 2) {
    lval->KoopaIR();
  }
  else if (type == 3) {
    // %0 = add 0, number
    cout << "  %" << koopacnt << " = add 0, " << number << endl;
    koopacnt++;
  }
}

int PrimaryExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(exp.get())->getValue();
  } 
  else if (type == 2) {
    return dynamic_cast<ExpBaseAST*>(lval.get())->getValue();
  }
  else if (type == 3) {
    return number;
  }
  assert(0);
}


// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp | IDENT '(' FuncRParams ')'
// UnaryOp ::= "+" | "-" | "!"
void UnaryExpAST::KoopaIR() const {
  if(type == 1) {
    primary_exp->KoopaIR();
  }
  else if(type == 2) {
    unary_exp->KoopaIR();
    if(unary_op == '-') {
      // %1 = sub 0, %0
      cout << "  %" << koopacnt << " = sub 0, %" << koopacnt - 1 <<endl;
      koopacnt++;
    }
    else if(unary_op == '!') {
      // %1 = eq 0, %0
      cout << "  %" << koopacnt << " = eq 0, %" << koopacnt - 1 <<endl;
      koopacnt++;
    }
  }
  else if(type == 3) {  // 函数调用

    auto func = query_symbol(ident);

    // 函数名必须为之前定义过的全局符号
    assert(func.first == 0);
    // 确保函数类型正确
    assert(func.second->type == SYM_TYPE_FUNCVOID || func.second->type == SYM_TYPE_FUNCINT);

    // 计算所有的参数,并将对应的虚拟寄存器编号记录
    auto vec = new vector<int>();
    for(auto& exp: *func_r_param_list) {
      exp->KoopaIR();
      vec->push_back(koopacnt-1);
    }   

    // 如果是 int 函数, 把返回值保存下来放入新的虚拟寄存器
    if (func.second->type == SYM_TYPE_FUNCINT) {
      cout << "  %" << koopacnt << " = ";
      koopacnt++;
    }
    
    else if(func.second->type == SYM_TYPE_FUNCVOID) {
      cout << "  ";
    }

    // call @f(%0, %1, ...)
    cout << "call @" << ident << "(";
    for(int i: *vec) {
      cout << "%" << i << ", ";
    }

    // 退格擦除最后一个逗号
    if(!vec->empty()) {
      cout.seekp(-2, cout.end);
    }
    cout << ")" << endl;
    delete vec;
  }
}

int UnaryExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(primary_exp.get())->getValue();
  } 
  else {
    int value = dynamic_cast<ExpBaseAST*>(unary_exp.get())->getValue();
    if (unary_op == '-') {
      return -value;
    } 
    else if (unary_op == '!') {
      return !value;
    }
    assert(0);
  }
}

// AddExp ::= MulExp | AddExp AddOp MulExp 
void AddExpAST::KoopaIR() const {
  if(type == 1) {
    mul_exp->KoopaIR();
  }
  else if(type == 2) {
    add_exp->KoopaIR();
    int left = koopacnt - 1;
    mul_exp->KoopaIR();
    int right = koopacnt - 1;
    if(add_op == '+') {
      // %2 = add %0, %1
      cout << "  %" << koopacnt << " = add %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(add_op == '-') {
      // %2 = sub %0, %1
      cout << "  %" << koopacnt << " = sub %" << left << ", %" << right << endl;
      koopacnt++;
    }
  }
}

int AddExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(mul_exp.get())->getValue();
  } 
  else {
    int left = dynamic_cast<ExpBaseAST*>(add_exp.get())->getValue();
    int right = dynamic_cast<ExpBaseAST*>(mul_exp.get())->getValue();
    if (add_op == '+') {
      return left + right;
    } 
    else if (add_op == '-') {
      return left - right;
    }
    assert(0);
  }
}

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp 
void MulExpAST::KoopaIR() const {
  if(type == 1) {
    unary_exp->KoopaIR();
  }
  else if(type == 2) {
    mul_exp->KoopaIR();
    int left = koopacnt - 1;
    unary_exp->KoopaIR();
    int right = koopacnt - 1;
    if(mul_op == '*') {
      // %2 = mul %0, %1
      cout << "  %" << koopacnt << " = mul %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(mul_op == '/') {
      // %2 = div %0, %1
      cout << "  %" << koopacnt << " = div %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(mul_op == '%') {
      // %2 = mod %0, %1
      cout << "  %" << koopacnt << " = mod %" << left << ", %" << right << endl;
      koopacnt++;
    }
  }
}

int MulExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(unary_exp.get())->getValue();
  } 
  else {
    int left = dynamic_cast<ExpBaseAST*>(mul_exp.get())->getValue();
    int right = dynamic_cast<ExpBaseAST*>(unary_exp.get())->getValue();
    if (mul_op == '*') {
      return left * right;
    } 
    else if (mul_op == '/') {
      return left / right;
    } 
    else if (mul_op == '%') {
      return left % right;
    }
    assert(0);
  }
}

// RelExp ::= AddExp | RelExp RelOp AddExp
// RelOp ::= "<" | "<=" | ">" | ">="
void RelExpAST::KoopaIR() const {
  if(type == 1) {
    add_exp->KoopaIR();
  }
  else if(type == 2) {
    rel_exp->KoopaIR();
    int left = koopacnt - 1;
    add_exp->KoopaIR();
    int right = koopacnt - 1;
    if(rel_op == "<") {
      // %2 = lt %0, %1
      cout << "  %" << koopacnt << " = lt %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(rel_op == "<=") {
      // %2 = le %0, %1
      cout << "  %" << koopacnt << " = le %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(rel_op == ">") {
      // %2 = gt %0, %1
      cout << "  %" << koopacnt << " = gt %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(rel_op == ">=") {
      // %2 = ge %0, %1
      cout << "  %" << koopacnt << " = ge %" << left << ", %" << right << endl;
      koopacnt++;
    }
  }
}

int RelExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(add_exp.get())->getValue();
  } 
  else {
    int left = dynamic_cast<ExpBaseAST*>(rel_exp.get())->getValue();
    int right = dynamic_cast<ExpBaseAST*>(add_exp.get())->getValue();
    if (rel_op == "<") {
      return left < right;
    } 
    else if (rel_op == "<=") {
      return left <= right;
    } 
    else if (rel_op == ">") {
      return left > right;
    } 
    else {
      return left >= right;
    }
  }
}

// EqExp ::= RelExp | EqExp EqOp RelExp
// EqOp ::= "==" | "!="
void EqExpAST::KoopaIR() const {
  if(type == 1) {
    rel_exp->KoopaIR();
  }
  else if(type == 2) {
    eq_exp->KoopaIR();
    int left = koopacnt - 1;
    rel_exp->KoopaIR();
    int right = koopacnt - 1;
    if(eq_op == "==") {
      // %2 = eq %0, %1
      cout << "  %" << koopacnt << " = eq %" << left << ", %" << right << endl;
      koopacnt++;
    }
    else if(eq_op == "!=") {
      // %2 = ne %0, %1
      cout << "  %" << koopacnt << " = ne %" << left << ", %" << right << endl;
      koopacnt++;
    }
  }
}

int EqExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(rel_exp.get())->getValue();
  } 
  else {
    int left = dynamic_cast<ExpBaseAST*>(eq_exp.get())->getValue();
    int right = dynamic_cast<ExpBaseAST*>(rel_exp.get())->getValue();
    if (eq_op == "==") {
      return left == right;
    } 
    else {
      return left != right;
    }
  }
}

// LAndExp ::= EqExp | LAndExp "&&" EqExp
void LAndExpAST::KoopaIR() const {
  if(type == 1) {
    eq_exp->KoopaIR();
  }
  else if(type == 2) {
    // A&&B <==> (A!=0)&(B!=0)
    land_exp->KoopaIR();
    cout << "  %" << koopacnt << " = ne %" << koopacnt - 1 << ", 0" << endl;
    koopacnt++;

    // 短路求值
    int iftmp = ifcnt;
    ifcnt++;

    // @LAND_RESULT_id = alloc i32
    cout << "  @LAND_RESULT_" << iftmp << " = alloc i32" << endl;

    // br %0, %then, %else, 先看LAndExp是否为真
    cout << "  br %" << koopacnt - 1 << ", %THEN_" << iftmp << ", %ELSE_" << iftmp << endl;

    // %THEN_id: 创建新的then_entry
    cout << "%THEN_" << iftmp << ":" << endl;
    entryend = false;
    // && 左侧LAndExp为真, 答案为 EqExp != 0 的值
    eq_exp->KoopaIR();
    // %2 = ne %0, 0
    cout << "  %" << koopacnt << " = ne %" << koopacnt - 1 << ", 0" << endl;
    koopacnt++;
    cout << "  store %" << koopacnt - 1 << ", @LAND_RESULT_" << iftmp << endl;

    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %ELSE_id: 创建新的else_entry
    cout << "%ELSE_" << iftmp << ":" << endl;
    entryend = false;
    // && 左侧LAndExp已经为假, 答案为0
    cout << "  store 0, @LAND_RESULT_" << iftmp << endl;

    if(!entryend) {
      // jump %END_233
      cout << "  jump %END_" << iftmp << endl;
    }

    // %END_id: 创建新的end_entry
    cout << "%END_" << iftmp << ":" << endl;
    entryend = false;
    cout << "  %" << koopacnt << " = load @LAND_RESULT_" << iftmp << endl;
    koopacnt++;
  }
  
}

int LAndExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(eq_exp.get())->getValue();
  } 
  else {
    int left = dynamic_cast<ExpBaseAST*>(land_exp.get())->getValue();
    if (!left) {
      return 0;
    }
    int right = dynamic_cast<ExpBaseAST*>(eq_exp.get())->getValue();
    return (right != 0);
  }
}

// LOrExp ::= LAndExp | LOrExp "||" LAndExp
void LOrExpAST::KoopaIR() const {
  if(type == 1) {
    land_exp->KoopaIR();
  }
  else if(type == 2) {
    // A||B <==> (A!=0)|(B!=0)
    lor_exp->KoopaIR();
    cout << "  %" << koopacnt << " = ne %" << koopacnt - 1 << ", 0" << endl;
    koopacnt++;

    // 短路求值
    int iftmp = ifcnt;
    ifcnt++;

    // @LOR_RESULT_id = alloc i32
    cout << "  @LOR_RESULT_" << iftmp << " = alloc i32" << endl;

    // br %0, %then, %else, 先看LORExp是否为真
    cout << "  br %" << koopacnt - 1 << ", %THEN_" << iftmp << ", %ELSE_" << iftmp << endl;

    // %THEN_id: 创建新的then_entry
    cout << "%THEN_" << iftmp << ":" << endl;
    entryend = false;
    // || 左侧LOrExp已经为真, 答案为1
    cout << "  store 1, @LOR_RESULT_" << iftmp << endl;

    if(!entryend) {
      // jump %END_id
      cout << "  jump %END_" << iftmp << endl;
    }

    // %ELSE_id: 创建新的else_entry
    cout << "%ELSE_" << iftmp << ":" << endl;
    entryend = false;
    // || 左侧LOrExp为假, 答案为 LAndExp != 0 的值
    land_exp->KoopaIR();
    // %2 = ne %0, 0
    cout << "  %" << koopacnt << " = ne %" << koopacnt - 1 << ", 0" << endl;
    koopacnt++;
    cout << "  store %" << koopacnt - 1 << ", @LOR_RESULT_" << iftmp << endl;

    if(!entryend) {
      // jump %END_233
      cout << "  jump %END_" << iftmp << endl;
    }

    // %END_id: 创建新的end_entry
    cout << "%END_" << iftmp << ":" << endl;
    entryend = false;
    cout << "  %" << koopacnt << " = load @LOR_RESULT_" << iftmp << endl;
    koopacnt++;
  }
}

int LOrExpAST::getValue() const {
  if (type == 1) {
    return dynamic_cast<ExpBaseAST*>(land_exp.get())->getValue();
  } 
  else {
    int left = dynamic_cast<ExpBaseAST*>(lor_exp.get())->getValue();
    if (left) {
      return 1;
    }
    int right = dynamic_cast<ExpBaseAST*>(land_exp.get())->getValue();
    return (right != 0);
  }
}

// ConstExp ::= Exp
void ConstExpAST::KoopaIR() const {
  return;
}

int ConstExpAST::getValue() const{
  return dynamic_cast<ExpBaseAST*>(exp.get())->getValue();
}