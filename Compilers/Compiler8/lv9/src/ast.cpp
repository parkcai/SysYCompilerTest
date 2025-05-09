#include <iostream>
#include <cassert>
#include <unordered_map>
#include <algorithm>
#include "ast.hpp"

// 计数 koopa 语句的返回值 %xxx
static int koopacnt = 0;
// 计数 if 语句, 用于设置 entry
static int ifcnt = 0;
// 计数 while 语句, 用于设置 entry
static int whilecnt = 0;
// 当前 while 语句的标号
static int whilecur = 0;
// 保存 while 树上的 parent 关系
static std::unordered_map<int, int> whilepar;
// 当前 entry 是否已经 ret/br/jump, 若为 1 的话不应再生成任何语句
static int entry_end = 0;
// 当前是否在声明全局变量
static int declaring_global_var = 0;
// 当前是否在声明函数
static int declaring_func = 0;

// 声明库函数的函数
void declareLibraryFunctions() {
    std::cout << "decl @getint(): i32\n"
              "decl @getch(): i32\n"
              "decl @getarray(*i32): i32\n"
              "decl @putint(i32)\n"
              "decl @putch(i32)\n"
              "decl @putarray(i32, *i32)\n"
              "decl @starttime()\n"
              "decl @stoptime()\n" << std::endl;

    // 插入函数符号
    insert_symbol("getint", SYM_TYPE_FUNCINT, 0);
    insert_symbol("getch", SYM_TYPE_FUNCINT, 0);
    insert_symbol("getarray", SYM_TYPE_FUNCINT, 0);
    insert_symbol("putint", SYM_TYPE_FUNCVOID, 0);
    insert_symbol("putch", SYM_TYPE_FUNCVOID, 0);
    insert_symbol("putarray", SYM_TYPE_FUNCVOID, 0);
    insert_symbol("starttime", SYM_TYPE_FUNCVOID, 0);
    insert_symbol("stoptime", SYM_TYPE_FUNCVOID, 0);
}

// CompUnit
void CompUnitAST::KoopaIR() const {
    enter_code_block();
    
    // 调用声明库函数的函数
    declareLibraryFunctions();
    
    // 访问所有 CompUnitItem
    for (auto& comp_unit_item : *comp_unit_item_list) {
        comp_unit_item->KoopaIR();
    }
    
    exit_code_block();
}

// CompUnitItem
void CompUnitItemAST::KoopaIR() const {
  bool is_decl = (type == 1);  // 判断是否为声明
  bool is_func_def = (type == 2);  // 判断是否为函数定义

  if (is_decl || is_func_def) {
    if (is_decl) {
      declaring_global_var = 1;  // 如果是声明，设置标志
    }

    decl1_funcdef2->KoopaIR();  // 统一调用 IR 生成方法

    if (is_decl) {
      declaring_global_var = 0;  // 如果是声明，恢复标志
    }
  }
}

// Decl 
void DeclAST::KoopaIR() const {
  const_decl1_var_decl2_func_decl3->KoopaIR();
}

// ConstDecl
void ConstDeclAST::KoopaIR() const {
  for(auto& const_def: *const_def_list)
    const_def->KoopaIR();
}

// 打印一个 Aggregate
static void print_aggregate(const std::string& ident, std::vector<int>* agg, int pos,
  std::vector<int>* len, std::vector<int>* mul_len, int cur, char mode) {
  // std::cout <<"--" << pos << std::endl;
  if (mode == 'A') {
    while (cur != len->size()) { 
      std::cout << "{";
      int size = (*mul_len)[cur];
      size /= (*len)[cur];
      int i = 0;
      while (i < (*len)[cur]) { 
        print_aggregate(ident, agg, pos + i*size, len, mul_len, cur+1, mode);
        if (i != (*len)[cur]-1)
          std::cout << ", ";
        i++;
      }
      std::cout << "}";
      return; 
    }
    std::cout << (*agg)[pos];  // 当cur == len->size()时输出当前元素
  }
  else if (mode == 'S' || mode == 'K') {
    if(cur == len->size()) {
      if (mode == 'S')
        std::cout << "  store " << (*agg)[pos] << ", %" << koopacnt-1 << std::endl;
      else if (mode == 'K') {
        int tmp = (*agg)[pos];
        if (tmp == -1) 
          std::cout << "  store " << 0 << ", %" << koopacnt-1 << std::endl;
        else
          std::cout << "  store %" << tmp << ", %" << koopacnt-1 << std::endl;
      }
    }
    else {
      int size = (*mul_len)[cur];
      size /= (*len)[cur];
      int parent_ptr = koopacnt-1;
      int i = 0;
      while (i < (*len)[cur]) {  
        std::cout << "  %" << koopacnt << " = getelemptr ";
        if (cur == 0)
          std::cout << "@" << query_symbol(ident).first << ident;
        else
          std::cout << "%" << parent_ptr;
        std::cout << ", " << i << std::endl;
        koopacnt++;
        print_aggregate(ident, agg, pos + i*size, len, mul_len, cur+1, mode);
        i++;
      }
    }
  }
}

// ConstDef ::= IDENT ConstIndexList "=" ConstInitVal;
// ConstIndexList ::=  | ConstIndexList "[" ConstExp "]";
void ConstDefAST::KoopaIR() const {
  while (const_index_list->empty() == false) {
    // 常量数组情况
    insert_symbol(ident, SYM_TYPE_CONSTARRAY, const_index_list->size());

    // 判断是否是全局变量
    while (declaring_global_var) {
      std::cout << "global ";
      break;
    }

    // 打印数组分配
    std::cout << "@" << query_symbol(ident).first << ident << " = alloc";
    int i = 0;
    while (i < const_index_list->size()) {
      std::cout << "[";
      i++;
    }
    std::cout << "i32, ";

    // 计算维度大小并初始化
    auto mul_len = new std::vector<int>();
    auto len = new std::vector<int>();
    i = const_index_list->size() - 1;
    while (i >= 0) {
      const auto& const_exp = (*const_index_list)[i];
      int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
      len->push_back(tmp);
      if (mul_len->empty())
        mul_len->push_back(tmp);
      else
        mul_len->push_back(mul_len->back() * tmp);
      std::cout << tmp << "], ";
      i--;
    }
    std::cout.seekp(-2, std::cout.end); // 去掉最后的逗号

    // 反转mul_len和len
    std::reverse(mul_len->begin(), mul_len->end());
    std::reverse(len->begin(), len->end());

    // 聚合初始化
    std::vector<int> agg = dynamic_cast<ConstInitValAST*>
      (const_init_val.get())->Aggregate(mul_len->begin(), mul_len->end());

    // 判断是否为全局变量
    while (declaring_global_var) {
      std::cout << ", ";
      print_aggregate(ident, &agg, 0, len, mul_len, 0, 'A');
      std::cout << std::endl;
      break;
    }
    // 局部常量数组初始化
    while (!declaring_global_var) {
      std::cout << std::endl;
      print_aggregate(ident, &agg, 0, len, mul_len, 0, 'S');
      break;
    }

    delete mul_len;
    delete len;

    break;  // 如果 const_index_list 不为空，跳出外层的 while 循环
  }

  // 单常量的处理部分
  if (const_index_list->empty()) {
    insert_symbol(ident, SYM_TYPE_CONST,
      dynamic_cast<ConstInitValAST*>(const_init_val.get())->Calc());
  }
}

// ConstInitVal
void ConstInitValAST::KoopaIR() const {
  assert(0);  // 断言，表示该函数不应该被调用，通常用于非法路径的标记。
  return;
}

int ConstInitValAST::Calc() const {
  assert(type == 1);  // 断言当前类型为常量表达式（type == 1），以保证该方法适用正确的类型。
  return dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();  // 将 const_exp 转换为 ExpBaseAST，并计算其值。
}

std::vector<int> ConstInitValAST::CalculateArrayInitialization(std::vector<int>::iterator mul_len_begin, 
  std::vector<int>::iterator mul_len_end) const {
  std::vector<int> agg;  // 用来保存计算后的数组初始化值。
  auto it = mul_len_begin;
  ++it;

  while (it != mul_len_end) {
    if (agg.size() % (*it) == 0) {  // 如果当前数组可以被该维度整除
      auto child_agg = Aggregate(it, mul_len_end);  // 递归计算子数组初始化
      agg.insert(agg.end(), child_agg.begin(), child_agg.end());  // 添加到agg
      return agg;  // 初始化成功，直接返回
    }
    ++it;
  }

  assert(0);  // 如果没有找到合适的维度，断言失败，说明初始化逻辑有问题
  return agg;
}

std::vector<int> ConstInitValAST::Aggregate(std::vector<int>::iterator mul_len_begin,
  std::vector<int>::iterator mul_len_end) const {
  std::vector<int> agg;  // 用来保存常量数组的初始化值

  auto it = const_init_val_list->begin();
  while (it != const_init_val_list->end()) {
    auto child = dynamic_cast<ConstInitValAST*>(it->get());  // 强制转换为 ConstInitValAST 类型

    if (child->type == 1) {
      agg.push_back(child->Calc());  // 如果是常量，直接计算并添加
    }
    else if (child->type == 2) {
      // 如果是数组，调用 CalculateArrayInitialization 来处理
      std::vector<int> child_agg = child->CalculateArrayInitialization(mul_len_begin, mul_len_end);
      agg.insert(agg.end(), child_agg.begin(), child_agg.end());  // 将处理过的子数组结果合并到 agg 中
    }
    ++it;  // 移动到下一个元素
  }

  // 确保 agg 的大小符合数组要求，不足时填充 0
  while (agg.size() < *mul_len_begin) {
    agg.push_back(0);  // 填充 0 直到达到所需大小
  }
  return agg;  // 返回计算完成的初始化值
}

// VarDecl
void VarDeclAST::KoopaIR() const {
  auto it = var_def_list->begin();  // 使用迭代器遍历
  while (it != var_def_list->end()) {  // 使用 while 循环遍历变量定义列表
    (*it)->KoopaIR();  // 调用 KoopaIR
    ++it;  // 移动迭代器到下一个元素
  }
}

// VarDef
void VarDefAST::KoopaIR() const {
  if (const_index_list->empty()) {  // 判断是否为单变量
    // 处理单变量的情况
    insert_symbol(ident, SYM_TYPE_VAR, 0);  // 将变量插入符号表

    // 生成全局或局部变量的 IR 代码
    if (declaring_global_var) {  // 如果是全局变量
      generate_global_var();  // 生成全局变量的分配代码
    } else {  // 如果是局部变量
      generate_local_var();  // 生成局部变量的分配代码
    }
  } else {  // 如果是数组变量
    // 处理数组的情况
    insert_symbol(ident, SYM_TYPE_ARRAY, const_index_list->size());  // 将数组插入符号表

    if (declaring_global_var)  // 如果是全局数组
      std::cout << "global ";
    else  // 如果是局部数组
      std::cout << "  ";

    generate_array_allocation();  // 生成数组分配的 IR 代码
  }
}

// 生成全局变量的分配代码
void VarDefAST::generate_global_var() const {
  if (type == 1) {
    // global @var = alloc i32, zeroinit
    std::cout << "global @" << current_code_block() << ident;
    std::cout << " = alloc i32, zeroinit" << std::endl;
  } else if (type == 2) {
    // global @var = alloc i32, init_value
    std::cout << "global @" << current_code_block() << ident;
    std::cout << " = alloc i32, ";
    std::cout << dynamic_cast<InitValAST*>(init_val.get())->Calc() << std::endl;
  }
  std::cout << std::endl;  // 输出换行
}

// 生成局部变量的分配代码
void VarDefAST::generate_local_var() const {
  // 先分配内存
  std::cout << "  @" << current_code_block() << ident << " = alloc i32" << std::endl;

  if (type == 2) {  // 如果有初始化值
    init_val->KoopaIR();  // 调用初始化值的 IR 生成
    // 存储初始化值
    std::cout << "  store %" << koopacnt - 1 << ", @";
    std::cout << query_symbol(ident).first << ident << std::endl;
  }
}

// 生成数组分配的 IR 代码
void VarDefAST::generate_array_allocation() const {
  std::cout << "@" << query_symbol(ident).first << ident << " = alloc";
  int i = 0;
  while (i < const_index_list->size()) {
    std::cout << "[";  // 打印数组维度的括号
    ++i;  // 增加维度索引
  }
  std::cout << "i32, ";  // 数据类型为 i32（整型）

  auto mul_len = new std::vector<int>();  // 存储维度的长度积
  auto len = new std::vector<int>();  // 存储维度的长度

  // 遍历 const_index_list，计算每个维度的长度并打印
  calculate_array_dimensions(len, mul_len);  // 计算数组的维度

  std::cout.seekp(-2, std::cout.end);  // 去掉最后一个逗号和空格
  std::reverse(mul_len->begin(), mul_len->end());  // 反转 mul_len
  std::reverse(len->begin(), len->end());  // 反转 len

  // 初始化数组的 IR 代码
  initialize_array(len, mul_len);  // 生成初始化代码

  delete mul_len;  // 删除动态分配的内存
  delete len;  // 删除动态分配的内存
}

// 计算数组的维度和长度
void VarDefAST::calculate_array_dimensions(std::vector<int>* len, std::vector<int>* mul_len) const {
  int i = const_index_list->size() - 1;
  while (i >= 0) {
    const auto& const_exp = (*const_index_list)[i];  // 获取每一维的表达式
    int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();  // 计算该维度的长度
    len->push_back(tmp);  // 将长度加入 len
    if (mul_len->empty())
      mul_len->push_back(tmp);  // 第一个维度的长度积就是该维度的长度
    else
      mul_len->push_back(mul_len->back() * tmp);  // 计算当前维度的乘积长度

    std::cout << tmp << "], ";  // 输出当前维度的长度
    --i;  // 移动到下一个维度
  }
}

// 初始化数组的 IR 代码
void VarDefAST::initialize_array(std::vector<int>* len, std::vector<int>* mul_len) const {
  if (declaring_global_var) {  // 如果是全局数组
    if (type == 1) {
      std::cout << ", zeroinit" << std::endl;  // 全局数组零初始化
    } else if (type == 2) {  // 如果有初始化值
      std::vector<int> agg = dynamic_cast<InitValAST*>
        (init_val.get())->Aggregate(mul_len->begin(), mul_len->end());  // 计算初始化值
      std::cout << ", ";
      print_aggregate(ident, &agg, 0, len, mul_len, 0, 'A');  // 输出初始化值
      std::cout << std::endl;
    }
  } else {  // 如果是局部数组
    if (type == 1) {
      std::cout << std::endl;  // 输出换行
    } else if (type == 2) {  // 如果有初始化值
      std::cout << std::endl;  // 输出换行
      std::vector<int> agg = dynamic_cast<InitValAST*>
        (init_val.get())->Aggregate(mul_len->begin(), mul_len->end());  // 计算初始化值
      print_aggregate(ident, &agg, 0, len, mul_len, 0, 'K');  // 输出初始化值
    }
  }
}

// InitVal
void InitValAST::KoopaIR() const {
  assert(type == 1);  // 确保初始化值类型是表达式
  exp->KoopaIR();  // 调用表达式的 KoopaIR 生成函数
}

// 计算初始化值的整数值，返回对应的值
int InitValAST::Calc() const {
  assert(type == 1);  // 确保初始化值类型是表达式
  return dynamic_cast<ExpBaseAST*>(exp.get())->Calc();  // 计算并返回表达式的值
}

// 处理初始化列表，生成初始化数组的值或符号编号
std::vector<int> InitValAST::Aggregate(std::vector<int>::iterator mul_len_begin,
  std::vector<int>::iterator mul_len_end) const {
  
  bool is_global = declaring_global_var;  // 标记是否为全局变量
  std::vector<int> agg;  // 用于存储初始化值（常量或符号编号）

  auto init_val_iter = init_val_list->begin();  // 获取初始化值列表的迭代器
  while (init_val_iter != init_val_list->end()) {  // 遍历初始化值列表中的每个初始化项
    auto child = dynamic_cast<InitValAST*>(init_val_iter->get());  // 获取当前初始化项

    // 处理常量类型的初始化值
    if (child->type == 1) {
      if (is_global) {
        agg.push_back(child->Calc());  // 全局变量初始化值是常量，直接计算并加入
      } else {
        child->KoopaIR();  // 局部变量初始化值，生成 KoopaIR
        agg.push_back(koopacnt - 1);  // 将符号编号加入到 agg
      }
    }
    // 处理数组类型的初始化值
    else if (child->type == 2) {
      int flag = 0;  // 标记是否成功处理该数组初始化项
      auto it = mul_len_begin;
      ++it;  // 从第二维开始处理

      // 遍历剩余的维度
      auto mul_len_iter = mul_len_begin;
      ++mul_len_iter;
      while (mul_len_iter != mul_len_end) {
        if (agg.size() % (*mul_len_iter) == 0) {  // 如果当前初始化值列表中的元素个数能整除当前维度的大小
          auto child_agg = child->Aggregate(mul_len_iter, mul_len_end);  // 递归处理子数组
          agg.insert(agg.end(), child_agg.begin(), child_agg.end());  // 将子数组的初始化值加入
          flag = 1;  // 设置标记表示已处理
          break;
        }
        ++mul_len_iter;
      }

      if (!flag) {  // 如果没有成功处理，则触发断言，说明出现错误
        assert(0);
      }
    }
    ++init_val_iter;
  }

  // 填充不足部分，如果是全局变量用零填充，如果是局部变量用 -1 填充
  int fill_value = is_global ? 0 : -1;  
  // 补充不足的元素
  auto agg_size = agg.size();
  while (agg_size < (*mul_len_begin)) {
    agg.push_back(fill_value);
    ++agg_size;  // 更新当前大小
  }

  return agg;  // 返回最终的初始化值（常量数组或符号编号数组）
}

// FuncDecl
void FuncDeclAST::KoopaIR() const {
  // 如果符号表中不存在该函数，则插入符号
  if (!exist_symbol(ident)) {
    // 根据函数类型插入相应的符号
    if (func_type == "void") {
      insert_symbol(ident, SYM_TYPE_FUNCVOID, 0); // 插入 void 类型函数
    } else if (func_type == "int") {
      insert_symbol(ident, SYM_TYPE_FUNCINT, 0);  // 插入 int 类型函数
    }
  }

  // 设置函数声明标志为 true
  declaring_func = 1;

  // 打印函数声明的 Koopa IR 格式，函数名后加上左括号
  std::cout << "decl @" << ident << "(";

  auto param_iter = func_f_param_list->begin();
  bool first_param = true;  // 用于处理参数之间的逗号
  while (param_iter != func_f_param_list->end()) {
    if (!first_param) {
      std::cout << ", ";  // 在每个参数之间打印逗号
    }
    (*param_iter)->KoopaIR();  // 生成该参数的 Koopa IR
    first_param = false;  // 第一次后不再是第一个参数
    ++param_iter;  // 移动到下一个参数
  }

  // 打印右括号
  std::cout << ")";

  // 如果函数类型是 int，则添加返回类型声明
  if (func_type == "int") {
    std::cout << ": i32";
  }

  // 输出结束符，表示函数声明的结束
  std::cout << std::endl;

  // 清除函数声明标志
  declaring_func = 0;
}

// FuncDef
void FuncDefAST::KoopaIR() const {
  // 检查符号表中是否已经存在该函数名，如果没有，则插入相应的符号
  if (!exist_symbol(ident)) {
    // 若函数类型是 void，插入 void 类型的函数符号
    if (func_type == "void") {
      insert_symbol(ident, SYM_TYPE_FUNCVOID, 0);
    }
    // 若函数类型是 int，插入 int 类型的函数符号
    else if (func_type == "int") {
      insert_symbol(ident, SYM_TYPE_FUNCINT, 0);
    }
  }

  // 开始函数体的 Koopa IR 生成，进入新的代码块
  enter_code_block();

  // 打印函数声明的 Koopa IR 格式，形如 fun @func(@x: i32): i32 {}
  std::cout << "fun @" << ident << "(";

  auto it = func_f_param_list->begin();  // 获取参数列表的开始迭代器
  while (it != func_f_param_list->end()) {
    (*it)->KoopaIR();  // 生成每个参数的 Koopa IR
    ++it;  // 移动到下一个参数
    if (it != func_f_param_list->end()) {
      std::cout << ", ";  // 在每个参数之间插入逗号
    }
  }

  // 删除最后一个多余的逗号
  std::cout << ")";
  // 如果函数类型是 int，则输出返回类型 ": i32"
  if (func_type == "int") {
    std::cout << ": i32";
  }

  // 打印函数体的开始部分，使用大括号表示函数体
  std::cout << " {" << std::endl;

  // 打印入口标签
  std::cout << "%entry:" << std::endl;

  // 初始化 entry_end 标志为 0，表示函数体还未结束
  entry_end = 0;

  it = func_f_param_list->begin();  // 重新获取参数列表的开始迭代器
  while (it != func_f_param_list->end()) {
    // 对每个参数调用 Alloc 函数为其分配内存
    // 示例：@SYM_TABLE_233_x = alloc i32
    //        store @x, @SYM_TABLE_233_x
    dynamic_cast<FuncFParamAST*>(it->get())->Alloc();
    ++it;  // 移动到下一个参数
  }

  // 生成函数体内部的 Koopa IR
  block->KoopaIR();

  // 如果函数体未执行返回操作，补充一个默认的返回语句
  if (!entry_end) {
    // 如果函数类型是 int，返回 0 作为默认值
    if (func_type == "int")
      std::cout << "  ret 0" << std::endl;
    // 如果函数类型是 void，则直接返回
    else if (func_type == "void")
      std::cout << "  ret" << std::endl;
    else
      assert(0);  // 如果函数类型既不是 void 也不是 int，则出错
  }

  // 打印函数体的结束部分，使用右大括号表示函数结束
  std::cout << "}" << std::endl << std::endl;

  // 退出当前代码块，结束函数的 IR 生成
  exit_code_block();
}

// 计算并返回参数类型（支持数组类型的情况下包含维度）
std::string FuncFParamAST::ParamType() const {
  // 确保该参数是数组类型
  assert(type == 2);

  // 初始化返回字符串，表示指针类型
  std::string str = "*";

  int i = 0;
  while (i < const_index_list->size()) {
    str += "[";  // 添加左中括号表示数组维度
    ++i;  // 移动到下一个维度
  }

  // 在字符串中加入基本类型 "i32, "，表示数组元素的类型
  str += "i32, ";

  // 从数组的最后一个维度开始，逐个添加到类型字符串中
  i = const_index_list->size() - 1;
  while (i >= 0) {
    // 获取当前维度的常量表达式并计算其值
    const auto& const_exp = (*const_index_list)[i];
    int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
    str += std::to_string(tmp) + "], ";  // 添加当前维度的大小，并加上右中括号
    --i;  // 移动到上一个维度
  }

  // 删除末尾的逗号和空格
  str.pop_back();
  str.pop_back();

  // 返回最终计算的参数类型字符串
  return str;
}

// FuncFParam
void FuncFParamAST::KoopaIR() const {
  // 如果是基本类型的参数（type == 1）
  if(type == 1) {
    // 如果当前不是声明函数状态，输出参数名称
    if (!declaring_func)
      std::cout << "@" << ident << ": ";  // 输出形如 "@param_name: " 的格式

    // 输出基本类型 i32
    std::cout << "i32";  // 对于基本类型参数，输出 "i32"
  }
  // 如果是数组类型的参数（type == 2）
  else if(type == 2) {
    // 如果当前不是声明函数状态，输出参数名称
    if (!declaring_func)
      std::cout << "@" << ident << ": ";  // 输出形如 "@param_name: " 的格式

    // 输出数组参数的类型信息
    std::cout << ParamType();  // 输出数组的类型信息，包括维度和类型
  }
}

void FuncFParamAST::Alloc() const {
  // 基本类型参数的内存分配
  if(type == 1) {
    // 为基本类型参数分配内存并输出 alloc 指令
    std::cout << "  @" << current_code_block() << ident << " = alloc i32" << std::endl;
    
    // 在符号表中插入该变量
    insert_symbol(ident, SYM_TYPE_VAR, 0);  // 在符号表中插入该标识符，类型为变量
    
    // 输出 store 指令，将参数值存储到分配的内存地址
    std::cout << "  store @" << ident << ", @";  // 输出存储指令
    std::cout << query_symbol(ident).first << ident << std::endl;  // 输出符号表中的内存位置并存储
  }
  // 数组类型参数的内存分配
  else if(type == 2) {
    // 为数组参数在符号表中插入，类型为指针，数组维度为 const_index_list 大小 + 1
    insert_symbol(ident, SYM_TYPE_PTR, const_index_list->size() + 1);  // 插入数组参数到符号表
    
    // 为数组分配内存并输出 alloc 指令
    std::cout << "  @" << current_code_block() << ident << " = alloc " << ParamType() << std::endl;
    
    // 输出 store 指令，将数组的值存储到分配的内存地址
    std::cout << "  store @" << ident << ", @";  // 输出存储指令
    std::cout << query_symbol(ident).first << ident << std::endl;  // 输出符号表中的内存位置并存储
  }
}

// Block 
void BlockAST::KoopaIR() const {
  enter_code_block();
  for(auto& block_item: *block_item_list)
  {
    if(entry_end) break;
    block_item->KoopaIR();
  }
  exit_code_block();
}

// BlockItem
void BlockItemAST::KoopaIR() const {
  decl1_stmt2->KoopaIR();  // 调用声明或语句的 KoopaIR 函数生成对应的 IR 代码
}

// Stmt
void StmtAssignAST::KoopaIR() const {
  exp->KoopaIR();  // 计算右值表达式并生成对应的 IR 代码
  int exp_koopacnt = koopacnt - 1;  // 获取右侧表达式的 KoopaIR 编号
  auto lvalptr = dynamic_cast<LValAST*>(lval.get());  // 转换左值为 LValAST 类型指针
  auto symval = query_symbol(lvalptr->ident);  // 查询左值对应的符号表项

  // 根据符号类型生成相应的 Koopa IR 指令
  if (symval.second->type == SYM_TYPE_VAR) {  // 如果左值是一个变量
    // 生成存储指令，将右值存入变量
    std::cout << "  store %" << exp_koopacnt << ", @";
    std::cout << symval.first << lvalptr->ident << std::endl;  // 输出 store 指令
  }
  else if (symval.second->type == SYM_TYPE_ARRAY) {  // 如果左值是一个数组
    // 遍历数组的下标，依次计算每个元素的地址并执行存储
    size_t i = 0;
    while (i < lvalptr->index_list->size()) {  // 使用 while 循环遍历数组下标
      int lastptr_koopacnt = koopacnt - 1;  // 记录上一次计算的指针 KoopaIR 编号
      const auto& exp_index = (*(lvalptr->index_list))[i];  // 获取当前数组下标表达式
      dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();  // 计算数组下标的 IR 代码
      int exp_index_koopacnt = koopacnt - 1;  // 获取当前下标表达式的 KoopaIR 编号

      // 根据当前下标计算数组元素的地址（getelemptr）
      std::cout << "  %" << koopacnt << " = getelemptr ";
      if (i == 0)
        std::cout << "@" << symval.first << lvalptr->ident;  // 第一个下标，直接使用数组名
      else
        std::cout << "%" << lastptr_koopacnt;  // 后续下标，使用上次计算的指针
      std::cout << ", %" << exp_index_koopacnt << std::endl;  // 输出 getelemptr 指令
      koopacnt++;  // 增加 KoopaIR 编号
      i++;  // 增加索引，移动到下一个数组下标
    }

    // 将右值存入计算出的数组元素地址
    std::cout << "  store %" << exp_koopacnt << ", %" << koopacnt - 1 << std::endl;
  }
  else if (symval.second->type == SYM_TYPE_PTR) {  // 如果左值是一个指针
    // 先加载指针的值（load）
    std::cout << "  %" << koopacnt << " = load @" << symval.first << lvalptr->ident << std::endl;
    koopacnt++;  // 增加 KoopaIR 编号

    // 处理指针的下标，类似于数组的处理
    size_t i = 0;
    while (i < lvalptr->index_list->size()) {  // 使用 while 循环遍历指针的下标
      int lastptr_koopacnt = koopacnt - 1;  // 获取上一次计算的指针 KoopaIR 编号
      const auto& exp_index = (*(lvalptr->index_list))[i];  // 获取指针下标表达式
      dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();  // 计算指针下标的 IR 代码
      int exp_index_koopacnt = koopacnt - 1;  // 获取当前下标表达式的 KoopaIR 编号

      // 根据当前下标计算指针的新地址（getptr 或 getelemptr）
      if (i == 0)
        std::cout << "  %" << koopacnt << " = getptr %";  // 第一个下标使用 getptr
      else
        std::cout << "  %" << koopacnt << " = getelemptr %";  // 后续下标使用 getelemptr
      std::cout << lastptr_koopacnt << ", %" << exp_index_koopacnt << std::endl;  // 输出 getptr/getelemptr 指令
      koopacnt++;  // 增加 KoopaIR 编号
      i++;  // 增加索引，移动到下一个指针下标
    }

    // 将右值存储到计算出的指针地址中
    std::cout << "  store %" << exp_koopacnt << ", %" << koopacnt - 1 << std::endl;
  }
  else {
    assert(0);  // 如果符号类型不匹配，触发断言错误
  }
}

//        
void StmtExpAST::KoopaIR() const {
  if(type == 1) {  // 当类型为1时表示空语句，什么都不做
    // 没有任何操作
  }
  else if(type == 2) {  // 当类型为2时表示有表达式的语句
    exp->KoopaIR();  // 执行表达式的 Koopa IR 生成
  }
}

//        
void StmtBlockAST::KoopaIR() const {
  block->KoopaIR();  // 处理并转换整个代码块为 Koopa IR
}

//        | "if" "(" Exp ")" Stmt
//        | "if" "(" Exp ")" Stmt "else" Stmt
void StmtIfAST::KoopaIR() const {
  if(entry_end) return;  // 如果已标记为结束，则直接返回，不生成更多的 IR 代码

  int ifcur = ifcnt;  // 获取当前 if 语句的计数值，用作标记
  ifcnt++;  // 增加 if 语句计数器，确保每个 if 语句有唯一标识

  exp->KoopaIR();  // 将条件表达式转换为 Koopa IR

  while(type == 1) {
    std::cout << "  br %" << koopacnt - 1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_END_" << ifcur << std::endl;  // 如果条件为真，跳转到 then 部分，否则跳转到 end 部分

    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;  // 定义 then 部分的标签
    entry_end = 0;  // 重置 entry_end 标记
    stmt_if->KoopaIR();  // 生成 then 语句的 IR

    if(!entry_end) {  // 如果 then 部分没有提前结束
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;  // 跳转到 end 部分
    }

    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;  // 定义 end 部分的标签
    entry_end = 0;  // 重置 entry_end 标记，准备处理后续语句
    break;  // 确保 while 循环不会再继续执行
  }

  while(type == 2) {
    std::cout << "  br %" << koopacnt - 1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_ELSE_" << ifcur << std::endl;  // 如果条件为真，跳转到 then 部分，否则跳转到 else 部分

    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;  // 定义 then 部分的标签
    entry_end = 0;  // 重置 entry_end 标记
    stmt_if->KoopaIR();  // 生成 then 部分的 IR

    if(!entry_end) {  // 如果 then 部分没有提前结束
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;  // 跳转到 end 部分
    }

    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;  // 定义 else 部分的标签
    entry_end = 0;  // 重置 entry_end 标记
    stmt_else->KoopaIR();  // 生成 else 部分的 IR

    if(!entry_end) {  // 如果 else 部分没有提前结束
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;  // 跳转到 end 部分
    }

    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;  // 定义 end 部分的标签
    entry_end = 0;  // 重置 entry_end 标记，准备处理后续语句
    break;
  }
}

//        | "while" "(" Exp ")" Stmt
void StmtWhileAST::KoopaIR() const {
  if(entry_end) return;
  int whileold = whilecur;
  whilecur = whilecnt;
  whilecnt++;
  whilepar[whilecur] = whileold;

  std::cout << "  jump %STMTWHILE_ENTRY_" << whilecur << std::endl;
  std::cout << "%STMTWHILE_ENTRY_" << whilecur << ":" << std::endl;
  entry_end = 0;
  exp->KoopaIR();
  std::cout << "  br %" << koopacnt-1 << ", %STMTWHILE_BODY_" << whilecur;
  std::cout << ", %STMTWHILE_END_" << whilecur << std::endl;
  std::cout << "%STMTWHILE_BODY_" << whilecur << ":" << std::endl;
  entry_end = 0;
  stmt->KoopaIR();
  if(!entry_end){
    std::cout << "  jump %STMTWHILE_ENTRY_" << whilecur << std::endl;
  }
  std::cout << "%STMTWHILE_END_" << whilecur << ":" << std::endl;
  entry_end = 0;
  whilecur = whilepar[whilecur];
}

//        | "break" ";"
void StmtBreakAST::KoopaIR() const {
  // 输出跳转指令，跳出当前循环，跳转到 while 循环结束标签
  std::cout << "  jump %STMTWHILE_END_" << whilecur << std::endl;
  entry_end = 1;  // 设置 entry_end 为 1，标记循环结束
}

//        | "continue" ";"
void StmtContinueAST::KoopaIR() const {
  // 输出跳转指令，跳回当前循环的入口，继续下一轮循环
  std::cout << "  jump %STMTWHILE_ENTRY_" << whilecur << std::endl;
  entry_end = 1;  // 设置 entry_end 为 1，标记继续执行
}

//        | "return" ";";
//        | "return" Exp ";";
void StmtReturnAST::KoopaIR() const {
  if (type == 1) {  // 处理无返回值的 return
    std::cout << "  ret" << std::endl;  // 生成返回指令
    entry_end = 1;  // 设置 entry_end 为 1，表示函数执行完毕
  }
  else if (type == 2) {  // 处理有返回值的 return
    exp->KoopaIR();  // 生成返回值的 Koopa IR
    std::cout << "  ret %" << koopacnt - 1 << std::endl;  // 生成返回指令，将返回值输出
    entry_end = 1;  // 设置 entry_end 为 1，表示函数执行完毕
  }
}

// Exp ::= LOrExp;
void ExpAST::KoopaIR() const {
  lorexp->KoopaIR();  // 调用逻辑或表达式的 KoopaIR 方法生成 IR 代码
}

// 计算表达式的值，递归计算 lorexp
int ExpAST::Calc() const {
  return dynamic_cast<ExpBaseAST*>(lorexp.get())->Calc();  // 递归调用，并返回结果
}

// LVal ::= IDENT IndexList;
// IndexList ::=  | IndexList "[" Exp "]";
// 只有 LVal 出现在表达式中时会调用该 KoopaIR
// 如果 LVal 作为左值出现, 则在父节点 StmtAssign 读取其信息
void LValAST::KoopaIR() const {
  auto val = query_symbol(ident);
  assert(val.second->type != SYM_TYPE_UND);

  while (val.second->type == SYM_TYPE_CONST) { 
    assert(index_list->size() == 0); 
    // 常量值直接生成加法指令
    std::cout << "  %" << koopacnt << " = add 0, "; 
    std::cout << val.second->value << std::endl; 
    koopacnt++; 
    break; 
  }

  while (val.second->type == SYM_TYPE_VAR) { 
    assert(index_list->size() == 0); 
    // 变量值直接从内存中加载
    std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl; 
    koopacnt++; 
    break; 
  }

  while (val.second->type == SYM_TYPE_CONSTARRAY || val.second->type == SYM_TYPE_ARRAY) { 
    // 处理数组
    if (val.second->value == index_list->size()) { 
      // 访问数组元素
      int i = 0;
      while (i < index_list->size()) { 
        int lastptr_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt - 1;

        std::cout << "  %" << koopacnt << " = getelemptr ";
        std::cout << (i == 0 ? "@" : "%") << (i == 0 ? val.first + ident : std::to_string(lastptr_koopacnt));
        std::cout << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
        i++;
      }
      std::cout << "  %" << koopacnt << " = load %" << koopacnt - 1 << std::endl;
      koopacnt++;
    } else { 
      // 部分解引用，传递指针
      int i = 0;
      while (i < index_list->size()) { 
        int lastptr_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt - 1;

        std::cout << "  %" << koopacnt << " = getelemptr ";
        std::cout << (i == 0 ? "@" : "%") << (i == 0 ? val.first + ident : std::to_string(lastptr_koopacnt));
        std::cout << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
        i++;
      }
      int lastptr_koopacnt = koopacnt - 1;
      std::cout << "  %" << koopacnt << " = getelemptr ";
      std::cout << (index_list->empty() ? "@" : "%") << (index_list->empty() ? val.first + ident : std::to_string(lastptr_koopacnt));
      std::cout << ", 0" << std::endl;
      koopacnt++;
    }
    break; 
  }

  while (val.second->type == SYM_TYPE_PTR) { 
    // 处理指针
    if (val.second->value == index_list->size()) { 
      // 直接加载指针值
      std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl;
      koopacnt++;
      int i = 0;
      while (i < index_list->size()) { 
        int lastptr_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt - 1;

        std::cout << "  %" << koopacnt << " = " << (i == 0 ? "getptr" : "getelemptr") << " %";
        std::cout << lastptr_koopacnt << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
        i++;
      }
      std::cout << "  %" << koopacnt << " = load %" << koopacnt - 1 << std::endl;
      koopacnt++;
    } else { 
      // 部分解引用，做指针传递
      std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl;
      koopacnt++;
      int i = 0;
      while (i < index_list->size()) { 
        int lastptr_koopacnt = koopacnt - 1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt - 1;

        std::cout << "  %" << koopacnt << " = " << (i == 0 ? "getptr" : "getelemptr") << " %";
        std::cout << lastptr_koopacnt << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
        i++;
      }
      int lastptr_koopacnt = koopacnt - 1;
      std::cout << "  %" << koopacnt << " = " << (index_list->empty() ? "getptr" : "getelemptr") << " %";
      std::cout << lastptr_koopacnt << ", 0" << std::endl;
      koopacnt++;
    }
    break; 
  }
}

int LValAST::Calc() const {
  auto val = query_symbol(ident);
  assert(val.second->type == SYM_TYPE_CONST);
  return val.second->value;
}

// PrimaryExp ::= "(" Exp ")" | LVal | Number;
void PrimaryExpAST::KoopaIR() const {
  if(type==1) {
    exp1_lval2->KoopaIR();
  }
  else if(type==2) {
    exp1_lval2->KoopaIR();
  }
  else if(type==3) {
    // %0 = add 0, 233
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< number << std::endl;
    koopacnt++;
  }
}

int PrimaryExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(exp1_lval2.get())->Calc();
  }
  else if(type==2) {
    return dynamic_cast<ExpBaseAST*>(exp1_lval2.get())->Calc();
  }
  else if(type==3) {
    return number;
  }
  assert(0);
  return 0;
}

// UnaryExp ::= PrimaryExp | FuncExp | UnaryOp UnaryExp;
// UnaryOp ::= "+" | "-" | "!"
void UnaryExpAST::KoopaIR() const {
  int type_temp = type;
  
  while (type_temp == 1 || type_temp == 2) {  // 如果是 PrimaryExp 或 FuncExp 类型
    primaryexp1_funcexp2_unaryexp3->KoopaIR();  // 递归处理子表达式
    break;
  }

  while (type_temp == 3) {  // 如果是 UnaryOp UnaryExp 类型
    primaryexp1_funcexp2_unaryexp3->KoopaIR();  // 递归处理子表达式

    while (unaryop == '-') {  // 如果是一元减法运算符
      std::cout << "  %" << koopacnt << " = sub 0, %";  // 生成减法 IR 指令
      std::cout << koopacnt-1 << std::endl;  // 输出操作数 IR
      koopacnt++;  // 增加 IR 计数器
      break;
    }

    while (unaryop == '!') {  // 如果是一元逻辑非运算符
      std::cout << "  %" << koopacnt << " = eq 0, %";  // 生成相等比较 IR 指令
      std::cout << koopacnt-1 << std::endl;  // 输出操作数 IR
      koopacnt++;  // 增加 IR 计数器
      break;
    }
    
    break;
  }
}

// 计算表达式的值，根据不同的类型返回不同的结果
int UnaryExpAST::Calc() const {  // 一元表达式的计算
  if (type == 1) {  // 如果是类型 1，递归计算其子表达式
    return dynamic_cast<ExpBaseAST*>(primaryexp1_funcexp2_unaryexp3.get())->Calc();  // 调用子表达式的计算函数
  }
  else if (type == 3) {  // 如果是类型 3，表示一元操作
    int tmp = dynamic_cast<ExpBaseAST*>(primaryexp1_funcexp2_unaryexp3.get())->Calc();  // 计算子表达式的值
    switch (unaryop) { 
      case '+':
        return tmp;  // 返回原值
      case '-':
        return -tmp;  // 返回相反数
      case '!':
        return !tmp;  // 返回非运算结果
      default:
        assert(false);  // 如果 unaryop 不是已知的操作，触发断言错误
    }
  }
  // 如果类型为 2，则不能进行计算，直接断言
  assert(0);  // 如果是非法类型，直接抛出断言错误
  return 0; 
}

// FuncExp ::= IDENT "(" FuncRParams ")";
// FuncRParams ::=  | FuncRParamList;
// FuncRParamList ::= Exp | FuncRParamList "," Exp;
void FuncExpAST::KoopaIR() const {  // FuncExp 的 IR 生成函数
  auto func = query_symbol(ident);  // 查询符号表，获取函数的符号信息
  // 必须为全局符号
  assert(func.first == "SYM_TABLE_0_");  // 确保该符号是全局符号
  // 必须是函数
  assert(func.second->type == SYM_TYPE_FUNCVOID || func.second->type == SYM_TYPE_FUNCINT);  // 确保符号类型是函数（void 或 int）

  // 计算所有的参数并存储它们的 IR 编号
  std::vector<int> vec;
  auto iter = func_r_param_list->begin();  // 使用迭代器遍历参数列表
  while (iter != func_r_param_list->end()) {
    (*iter)->KoopaIR();  // 递归生成每个参数的 IR 代码
    vec.push_back(koopacnt - 1);  // 将当前计算出来的参数的 IR 编号加入到参数列表中
    ++iter;  // 迭代器自增
  }

  // 如果是 int 类型函数，准备生成返回值的 IR
  if (func.second->type == SYM_TYPE_FUNCINT)  
    std::cout << "  %" << koopacnt << " = ";  // 对于返回值类型为 int 的函数，生成 IR 代码
  else if (func.second->type == SYM_TYPE_FUNCVOID)  // 如果是 void 类型函数，不需要返回值
    std::cout << "  ";  // void 函数不需要赋值 IR 代码

  // 生成函数调用的 IR
  std::cout << "call @" << ident << "(";  // 输出函数调用的格式，@表示全局函数，ident为函数名
  auto vec_iter = vec.begin();  // 使用迭代器遍历 vec
  while (vec_iter != vec.end()) { 
    std::cout << "%" << *vec_iter << ", ";  // 输出参数的 IR 编号
    ++vec_iter;  // 迭代器自增
  }

  // 退格擦除最后一个逗号
  if (!vec.empty()) {  // 如果参数列表不为空
    std::cout.seekp(-2, std::cout.end);  // 去掉最后的逗号
  }
  std::cout << ")" << std::endl;  // 输出右括号，表示函数调用结束
  if (func.second->type == SYM_TYPE_FUNCINT)  // 如果是返回值为 int 的函数
    koopacnt++;  // 增加 IR 计数器，用于生成下一个 IR
}

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp ::= "*" | "/" | "%"
void MulExpAST::KoopaIR() const {  // MulExp 生成 Koopa IR 的函数
  if(type == 1) {  // 如果是类型 1，表示当前表达式是一个一元表达式
    unaryexp->KoopaIR();  // 递归生成一元表达式的 IR
  }
  else if(type == 2) {  // 如果是类型 2，表示当前表达式是两个乘法表达式用操作符连接
    // 递归处理左边的表达式
    mulexp->KoopaIR();  
    int left = koopacnt - 1;  // 获取左边表达式的 IR 编号

    // 递归处理右边的表达式
    unaryexp->KoopaIR();
    int right = koopacnt - 1;  // 获取右边表达式的 IR 编号

    // 根据操作符判断对应的运算
    switch(mulop) {
      case '*':  // 如果操作符是乘法
        std::cout << "  %" << koopacnt << " = mul %";  // 输出乘法的 IR 代码
        std::cout << left << ", %" << right << std::endl;
        koopacnt++;  // 更新 IR 编号
        break;

      case '/':  // 如果操作符是除法
        std::cout << "  %" << koopacnt << " = div %";  // 输出除法的 IR 代码
        std::cout << left << ", %" << right << std::endl;
        koopacnt++;  // 更新 IR 编号
        break;

      case '%':  // 如果操作符是取模
        std::cout << "  %" << koopacnt << " = mod %";  // 输出取模的 IR 代码
        std::cout << left << ", %" << right << std::endl;
        koopacnt++;  // 更新 IR 编号
        break;
    }
  }
}

int MulExpAST::Calc() const {  // MulExp 计算表达式值的函数
  if(type == 1) {  // 如果是类型 1，表示当前表达式是一个一元表达式
    return dynamic_cast<ExpBaseAST*>(unaryexp.get())->Calc();  // 递归调用一元表达式的计算函数
  }
  else if(type == 2) {  // 如果是类型 2，表示当前表达式是两个乘法表达式用操作符连接
    // 递归计算左侧和右侧的操作数
    int left = dynamic_cast<ExpBaseAST*>(mulexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(unaryexp.get())->Calc();

    // 根据操作符执行相应的计算
    switch(mulop) {
      case '*':  // 如果操作符是乘法
        return left * right;  // 返回乘法结果
      case '/':  // 如果操作符是除法
        return left / right;  // 返回除法结果
      case '%':  // 如果操作符是取模
        return left % right;  // 返回取模结果
    }
  }

  assert(0);  // 如果类型不符合预期，触发断言错误
  return 0;
}

// AddExp ::= MulExp | AddExp AddOp MulExp;
// AddOp ::= "+" | "-"
void AddExpAST::KoopaIR() const {  // AddExp 的 KoopaIR 生成函数
  switch(type) {  
    case 1:  // 如果类型是 1，表示是一个简单的乘法表达式
      mulexp->KoopaIR();  // 递归调用生成乘法表达式的 IR
      break;
    case 2:  // 如果类型是 2，表示是一个加法或减法表达式
      addexp->KoopaIR();  // 递归调用生成加法/减法左操作数的 IR
      int left = koopacnt - 1;  // 获取左操作数的 IR 编号

      mulexp->KoopaIR();  // 递归调用生成加法/减法右操作数的 IR
      int right = koopacnt - 1;  // 获取右操作数的 IR 编号

      // 根据操作符输出相应的 Koopa IR 代码
      if(addop == '+') {  // 如果操作符是加法
        std::cout << "  %" << koopacnt << " = add %";  // 输出加法的 Koopa IR
        std::cout << left << ", %" << right << std::endl;  // 输出加法操作
        koopacnt++;  // 更新 IR 编号
      } 
      else if(addop == '-') {  // 如果操作符是减法
        std::cout << "  %" << koopacnt << " = sub %";  // 输出减法的 Koopa IR
        std::cout << left << ", %" << right << std::endl;  // 输出减法操作
        koopacnt++;  // 更新 IR 编号
      }
      break;
  }
}

int AddExpAST::Calc() const {  // AddExp 的计算函数
  switch(type) {  
    case 1:  // 如果类型是 1，表示是一个简单的乘法表达式
      return dynamic_cast<ExpBaseAST*>(mulexp.get())->Calc();  // 递归调用计算乘法表达式的值
    case 2:  // 如果类型是 2，表示是一个加法或减法表达式
      int left = dynamic_cast<ExpBaseAST*>(addexp.get())->Calc();  // 计算左操作数的值
      int right = dynamic_cast<ExpBaseAST*>(mulexp.get())->Calc();  // 计算右操作数的值

      // 根据操作符返回相应的计算结果
      if(addop == '+') {  // 如果操作符是加法
        return left + right;  // 返回加法的计算结果
      } 
      else if(addop == '-') {  // 如果操作符是减法
        return left - right;  // 返回减法的计算结果
      }
      break;
  }

  assert(0);  // 如果类型不匹配，触发断言错误
  return 0;  
}

// RelExp ::= AddExp | RelExp RelOp AddExp;
// RelOp ::= "<" | ">" | "<=" | ">="
void RelExpAST::KoopaIR() const {  // RelExp 的 KoopaIR 生成函数
  if(type == 1) {  // 如果是类型 1，表示只是一个加法表达式
    addexp->KoopaIR();  // 调用加法表达式的 KoopaIR 生成函数
  }
  else if(type == 2) {  // 如果是类型 2，表示有关系运算符，包含左表达式、运算符和右表达式
    // 先生成左表达式的 IR
    relexp->KoopaIR();  
    int left = koopacnt - 1;  // 保存左操作数的 IR 编号

    // 再生成右表达式的 IR
    addexp->KoopaIR();  
    int right = koopacnt - 1;  // 保存右操作数的 IR 编号

    // 根据关系运算符输出对应的 Koopa IR 代码
    std::string relop_str = relop;  // 提前将关系操作符转换为字符串，避免重复访问
    if(relop_str == "<") {  // 如果操作符是小于
      std::cout << "  %" << koopacnt << " = lt %";  // 输出小于的 Koopa IR
      std::cout << left << ", %" << right << std::endl;  // 输出小于操作
      koopacnt++;  // 更新 IR 编号
    }
    else if(relop_str == ">") {  // 如果操作符是大于
      std::cout << "  %" << koopacnt << " = gt %";  // 输出大于的 Koopa IR
      std::cout << left << ", %" << right << std::endl;  // 输出大于操作
      koopacnt++;  // 更新 IR 编号
    }
    else if(relop_str == "<=") {  // 如果操作符是小于等于
      std::cout << "  %" << koopacnt << " = le %";  // 输出小于等于的 Koopa IR
      std::cout << left << ", %" << right << std::endl;  // 输出小于等于操作
      koopacnt++;  // 更新 IR 编号
    }
    else if(relop_str == ">=") {  // 如果操作符是大于等于
      std::cout << "  %" << koopacnt << " = ge %";  // 输出大于等于的 Koopa IR
      std::cout << left << ", %" << right << std::endl;  // 输出大于等于操作
      koopacnt++;  // 更新 IR 编号
    }
  }
}

int RelExpAST::Calc() const {  // RelExp 的计算函数
  if(type == 1) {  // 如果是类型 1，表示是加法表达式
    return dynamic_cast<ExpBaseAST*>(addexp.get())->Calc();  // 计算加法表达式的值
  }
  else if(type == 2) {  // 如果是类型 2，表示包含关系运算符
    // 计算左操作数和右操作数
    int left = dynamic_cast<ExpBaseAST*>(relexp.get())->Calc();  // 计算左操作数的值
    int right = dynamic_cast<ExpBaseAST*>(addexp.get())->Calc();  // 计算右操作数的值

    // 根据关系运算符返回相应的计算结果
    if(relop == "<") {  // 如果操作符是小于
      return left < right;  // 返回左小于右的布尔值
    }
    else if(relop == ">") {  // 如果操作符是大于
      return left > right;  // 返回左大于右的布尔值
    }
    else if(relop == "<=") {  // 如果操作符是小于等于
      return left <= right;  // 返回左小于等于右的布尔值
    }
    else if(relop == ">=") {  // 如果操作符是大于等于
      return left >= right;  // 返回左大于等于右的布尔值
    }
  }
  assert(0);  // 如果类型不匹配，触发断言错误
  return 0;  
}

// EqExp ::= RelExp | EqExp EqOp RelExp;
// EqOp ::= "==" | "!="
void EqExpAST::KoopaIR() const {
  while (type == 1) {  // 如果是 RelExp 类型
    relexp->KoopaIR();  // 递归调用生成 RelExp 部分的 KoopaIR 代码
    return; 
  }

  while (type == 2) {  // 如果是 EqExp EqOp RelExp 类型
    eqexp->KoopaIR();  // 递归调用生成 EqExp 部分的 KoopaIR 代码
    int left = koopacnt - 1;  // 保存左侧操作数
    relexp->KoopaIR();  // 递归调用生成 RelExp 部分的 KoopaIR 代码
    int right = koopacnt - 1;  // 保存右侧操作数

    while (eqop == "==") {  // 判断是否是相等操作
      // 生成等于比较的 KoopaIR 代码
      std::cout << "  %" << koopacnt << " = eq %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;  // 更新计数器
      return;
    }

    while (eqop == "!=") {  // 判断是否是不等操作
      // 生成不等比较的 KoopaIR 代码
      std::cout << "  %" << koopacnt << " = ne %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;  // 更新计数器
      return;
    }
  }
}

int EqExpAST::Calc() const {
  while (type == 1) {  // 如果是 RelExp 类型，直接计算 RelExp 的值
    return dynamic_cast<ExpBaseAST*>(relexp.get())->Calc();  // 返回 RelExp 的计算结果
  }

  while (type == 2) {  // 如果是 EqExp EqOp RelExp 类型
    int left = dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();  // 计算 EqExp 的值
    int right = dynamic_cast<ExpBaseAST*>(relexp.get())->Calc();  // 计算 RelExp 的值

    while (eqop == "==") {  // 判断是否是相等操作
      return left == right;  // 返回是否相等的结果
    }

    while (eqop == "!=") {  // 判断是否是不等操作
      return left != right;  // 返回是否不相等的结果
    }
  }
  assert(0);  // 如果没有匹配到任何类型，触发断言错误
  return 0;  // 返回默认值，防止编译器报错
}

// LAndExp ::= EqExp | LAndExp "&&" EqExp;  // 定义 LAndExp 表达式的文法，表示逻辑与操作（&&）
void LAndExpAST::KoopaIR() const {
  if (type == 1) {  // 如果当前是 EqExp 类型
    eqexp->KoopaIR();  // 递归调用，生成 EqExp 的 KoopaIR 代码
  }
  else if (type == 2) {  // 如果是 LAndExp "&&" EqExp 类型
    // A&&B <==> (A!=0)&(B!=0)  // 将逻辑与操作转换为短路求值方式
    landexp->KoopaIR();  // 递归调用，生成 LAndExp 部分的 KoopaIR 代码

    // %2 = ne %0, 0  // 检查左侧表达式是否不等于 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt - 1 << ", 0" << std::endl;  // 生成不等于零的指令
    koopacnt++;  // 增加计数器

    // 短路求值, 相当于一个if  // 如果左侧为 0，直接跳到 else 分支
    int ifcur = ifcnt;  // 保存当前 if 计数器的值
    ifcnt++;  // 增加 if 计数器
    // @STMTIF_LAND_RESULT_233 = alloc i32  // 分配内存，存储结果
    std::cout << "  @" << "STMTIF_LAND_RESULT_" << ifcur << " = alloc i32" << std::endl;

    // br %0, %then, %else  // 根据左侧是否为 0，进行跳转
    std::cout << "  br %" << koopacnt - 1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_ELSE_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry  // 生成 if 语句的 then 分支
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_end = 0;  // 标记当前条目没有结束
    // && 左侧 LAndExp 为 1, 答案为 EqExp 的值  // 如果左侧为 1，则继续计算右侧
    eqexp->KoopaIR();  // 递归调用，生成 EqExp 部分的 KoopaIR 代码
    // %2 = ne %0, 0  // 检查右侧表达式是否不等于 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt - 1 << ", 0" << std::endl;
    koopacnt++;  // 增加计数器
    std::cout << "  store %" << koopacnt - 1 << ", @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;  // 将计算结果存储到变量中

    if (!entry_end) {  // 如果当前条目没有结束
      // jump %STMTIF_END_233  // 跳转到语句结束部分
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry  // 生成 if 语句的 else 分支
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_end = 0;  // 标记当前条目没有结束
    // && 左侧 LAndExp 为 0, 答案为 0  // 如果左侧为 0，则右侧结果为 0
    std::cout << "  store 0, @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;

    if (!entry_end) {  // 如果当前条目没有结束
      // jump %STMTIF_END_233  // 跳转到语句结束部分
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry  // 生成语句结束的标签
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_end = 0;  // 标记当前条目没有结束
    std::cout << "  %" << koopacnt << " = load @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;  // 从变量中加载结果
    koopacnt++;  // 增加计数器
  }
}

int LAndExpAST::Calc() const {
  if (type == 1) {  // 如果是 EqExp 类型
    return dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();  // 计算 EqExp 的值
  }
  else if (type == 2) {  // 如果是 LAndExp "&&" EqExp 类型
    int left = dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();  // 计算左侧 LAndExp 的值
    if (!left) return 0;  // 如果左侧结果为 0，逻辑与操作结果为 0
    int right = dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();  // 计算右侧 EqExp 的值
    return (right != 0);  // 如果右侧不等于 0，结果为 1，否则为 0
  }
  assert(0);  // 如果没有匹配到任何类型，触发断言错误
  return 0;
}

// LOrExp ::= LAndExp | LOrExp "||" LAndExp;  // 定义 LOrExp 表达式的文法，表示逻辑或操作（||）
void LOrExpAST::KoopaIR() const {
  if(type == 1) {  // 如果当前是 LAndExp 类型
    landexp->KoopaIR();  // 递归调用，生成 LAndExp 的 KoopaIR 代码
  }
  else {  // 如果是 LOrExp "||" LAndExp 类型
    // A||B <==> (A!=0)|(B!=0)  // 将逻辑或操作转换为短路求值方式
    lorexp->KoopaIR();  // 递归调用，生成 LOrExp 部分的 KoopaIR 代码

    // %2 = ne %0, 0  // 检查左侧表达式是否不等于 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt - 1 << ", 0" << std::endl;  // 生成不等于零的指令
    koopacnt++;  // 增加计数器

    // 短路求值, 相当于一个if  // 如果左侧为非零，直接跳转到 then 分支
    int current_if = ifcnt;  // 保存当前 if 计数器的值
    ifcnt++;  // 增加 if 计数器
    // @STMTIF_LOR_RESULT_233 = alloc i32  // 分配内存，存储结果
    std::cout << "  @" << "STMTIF_LOR_RESULT_" << current_if << " = alloc i32" << std::endl;

    // br %0, %then, %else  // 根据左侧是否为零，进行跳转
    std::cout << "  br %" << koopacnt - 1 << ", %STMTIF_THEN_" << current_if;
    std::cout << ", %STMTIF_ELSE_" << current_if << std::endl;

    // %STMTIF_THEN_233: 创建新的entry  // 生成 if 语句的 then 分支
    std::cout << "%STMTIF_THEN_" << current_if << ":" << std::endl;
    entry_end = 0;  // 标记当前条目没有结束
    // || 左侧 LOrExp 为 1, 答案为 1, 即左侧 LOrExp 的值  // 如果左侧为 1，则结果为 1
    std::cout << "  store 1, @";
    std::cout << "STMTIF_LOR_RESULT_" << current_if << std::endl;

    if(!entry_end) {  // 如果当前条目没有结束
      // jump %STMTIF_END_233  // 跳转到语句结束部分
      std::cout << "  jump %STMTIF_END_" << current_if << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry  // 生成 if 语句的 else 分支
    std::cout << "%STMTIF_ELSE_" << current_if << ":" << std::endl;
    entry_end = 0;  // 标记当前条目没有结束
    // || 左侧 LOrExp 为 0, 答案为 LAndExp 的值
    landexp->KoopaIR();  // 递归调用，生成 LAndExp 部分的 KoopaIR 代码
    // %2 = ne %0, 0  // 检查右侧表达式是否不等于 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt - 1 << ", 0" << std::endl;
    koopacnt++;  // 增加计数器
    std::cout << "  store %" << koopacnt - 1 << ", @";
    std::cout << "STMTIF_LOR_RESULT_" << current_if << std::endl;  // 将计算结果存储到变量中

    if(!entry_end) {  // 如果当前条目没有结束
      // jump %STMTIF_END_233  // 跳转到语句结束部分
      std::cout << "  jump %STMTIF_END_" << current_if << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry  // 生成语句结束的标签
    std::cout << "%STMTIF_END_" << current_if << ":" << std::endl;
    entry_end = 0;  // 标记当前条目没有结束
    std::cout << "  %" << koopacnt << " = load @";
    std::cout << "STMTIF_LOR_RESULT_" << current_if << std::endl;  // 从变量中加载结果
    koopacnt++;  // 增加计数器
  }
}

int LOrExpAST::Calc() const {
  if(type == 1) {  // 如果是 LAndExp 类型
    return dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();  // 计算 LAndExp 的值
  }
  else {  // 如果是 LOrExp "||" LAndExp 类型
    int left = dynamic_cast<ExpBaseAST*>(lorexp.get())->Calc();  // 计算左侧 LOrExp 的值
    if(left) return 1;  // 如果左侧结果为非零，逻辑或操作结果为 1
    int right = dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();  // 计算右侧 LAndExp 的值
    return (right != 0);  // 如果右侧不等于 0，结果为 1，否则为 0
  }
  assert(0);  // 如果没有匹配到任何类型，触发断言错误
  return 0;  // 返回默认值，防止编译器报错
}

// ConstExp ::= Exp;
void ConstExpAST::KoopaIR() const {
  assert(0);
  return;
}

int ConstExpAST::Calc() const {
  return dynamic_cast<ExpBaseAST*>(exp.get())->Calc();
}