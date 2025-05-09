#include <cassert>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include "ast.hpp"
#include <koopa.h>
#include <vector>
#include <stack>
#include <unordered_map>

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成  出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
// 函数声明略
// ...
std::ostream * op;
int nxt = 0;
int nxt_block = 0;
std::unordered_map<size_t, std::string> var_reg_map;
std::unordered_map<std::string, int> symbol_name_map;
std::unordered_map<std::string, int> block_name_map;
std::stack<int> return_stack;
std::stack<loop_element> loop_stack;
pair<int, std::string> reg_map[15] = {
  {0, "t0"},
  {0, "t1"},
  {0, "t2"},
  {0, "t3"},
  {0, "t4"},
  {0, "t5"},
  {0, "t6"},
  {0, "a1"},
  {0, "a2"},
  {0, "a3"},
  {0, "a4"},
  {0, "a5"},
  {0, "a6"},
  {0, "a7"},
  {0, "a0"}
};


std::unordered_map<std::string, func_info> func_map;
func_info * cur_func = nullptr;
bool is_param = false;
class func_element{
public:
  std::string name;
  unsigned offset;
  std::unordered_map<size_t, unsigned> var_map;
  unsigned alloc_offset;
  func_element(std::string name, unsigned offset): name(name), offset(offset), var_map(), alloc_offset(0){}
};

std::list<block_element> block_list;

std::stack<func_element> func_stack;

std::string get_reg(){
  for (int i = 0; i < 15; i++){
    if (reg_map[i].first == 0){
      reg_map[i].first = 1;
      return reg_map[i].second;
    }
  }
  return "";
}

void reset_reg(std::string reg){
  for (int i = 0; i < 15; i++){
    if (reg_map[i].second == reg){
      reg_map[i].first = 0;
    }
  }
}

std::string get_koopa_array_type(std::vector<int> size_list, int index){
  if(index == size_list.size()) return "i32";
  return "[" + get_koopa_array_type(size_list, index+1) + ", " + std::to_string(size_list[index]) + "]" ;
}

std::string get_koopa_ptr_array_type(std::vector<int> size_list, int index){
  if(index == size_list.size()) return "*i32";
  return "[" + get_koopa_array_type(size_list, index+1) + ", " + std::to_string(size_list[index]) + "]" ;
}

std::vector<std::string> get_koopa_aggregate(ConstInitValAST & init, std::vector<int> size_list, int index, int total_size, std::ostream &of){
  std::cout << "total_size:" << total_size << std::endl;
  std::vector<std::string> res;
  for(auto &init_val : *init.const_init_val_list){
    if(dynamic_cast<ConstInitValAST*>(init_val.get())->exp){
      dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(init_val.get())->exp.get())->to_koopa_string(of);
      if(dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(init_val.get())->exp.get())->res_id == -1){
        res.push_back(dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(init_val.get())->exp.get())->val);
        std::cout << "val:" << dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(init_val.get())->exp.get())->val << std::endl;
      }
      else{
        res.push_back("%" + std::to_string(dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(init_val.get())->exp.get())->res_id));
        std::cout << "res_id:" << dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(init_val.get())->exp.get())->res_id << std::endl;
      }
    }
    else{
      int i = 0;
      int cur_size = 1;
      int end_idx = size_list.size() - 1;
      while(res.size() % cur_size == 0 && cur_size < total_size){
        cur_size *= size_list[end_idx - i];
        i++;
      }
      i--;
      cur_size /= size_list[end_idx - i];
      std::vector<std::string> tmp = get_koopa_aggregate(*dynamic_cast<ConstInitValAST*>(init_val.get()), size_list, end_idx - i + 1, cur_size, of);
      res.insert(res.end(), tmp.begin(), tmp.end());
    }
  }
  while(res.size() < total_size){
    res.push_back("0");
  }
  return res;
}

std::vector<std::string> get_koopa_aggregate(InitValAST & init, std::vector<int> size_list, int index, int total_size, std::ostream &of){
  std::vector<std::string> res;
  for(auto &init_val : *init.init_val_list){
    if(dynamic_cast<InitValAST*>(init_val.get())->exp){
      dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(init_val.get())->exp.get())->to_koopa_string(of);
      if(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(init_val.get())->exp.get())->res_id == -1){
        res.push_back(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(init_val.get())->exp.get())->val);
      }
      else{
        res.push_back("%" + std::to_string(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(init_val.get())->exp.get())->res_id));
      }
    }
    else{
      int i = 0;
      int cur_size = 1;
      int end_idx = size_list.size() - 1;
      while(res.size() % cur_size == 0 && cur_size < total_size){
        cur_size *= size_list[end_idx - i];
        i++;
      }
      i--;
      cur_size /= size_list[end_idx - i];
      std::vector<std::string> tmp = get_koopa_aggregate(*dynamic_cast<InitValAST*>(init_val.get()), size_list, end_idx - i + 1, cur_size, of);
      res.insert(res.end(), tmp.begin(), tmp.end());
    }
  }
  while(res.size() < total_size){
    res.push_back("0");
  }
  return res;
}

std::string output_koopa_aggregate(std::vector<int> size_list, int dim, int idx, std::vector<std::string> array){
  if(dim == size_list.size()){
    return array[idx];
  }
  std::string res = "{";
  int size = 1;
  for(int i = dim + 1; i < size_list.size(); i++){
    size *= size_list[i];
  }
  for(int i = 0; i < size_list[dim]; i++){
    res += output_koopa_aggregate(size_list, dim + 1, idx + i * size, array);
    if(i != size_list[dim] - 1) res += ", ";
  }
  res += "}";
  return res;
}

std::string gen_init_koopa_code(std::vector<int> size_list, int dim, int idx, std::vector<std::string> array, std::string base_ptr){
  if(dim == size_list.size()){
    return "  store " + array[idx] + ", " + base_ptr + "\n";
  }
  std::string res = "";
  for(int i=0;i<size_list[dim];++i){
    std::string nxt_ptr = "%" + std::to_string(nxt++);
    int size = 1;
    for(int i = dim + 1; i < size_list.size(); i++){
      size *= size_list[i];
    }
    res += "  " + nxt_ptr + " = getelemptr " + base_ptr + ", " + std::to_string(i) + "\n";
    res += gen_init_koopa_code(size_list, dim + 1, idx + i * size, array, nxt_ptr);
  }
  return res;
}

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary);
void Visit(const koopa_raw_global_alloc_t &global_alloc);
void Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
void Visit(const koopa_raw_call_t &call);
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr);
void Visit(const koopa_raw_get_ptr_t &get_ptr);

bool called;
int args_max_len;
void PreVisit(const koopa_raw_slice_t &slice);
void PreVisit(const koopa_raw_basic_block_t &bb);
void PreVisit(const koopa_raw_value_t &value);

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

int compute_size(koopa_raw_type_t type){
  if(type->tag == KOOPA_RTT_ARRAY){
    return type->data.array.len * compute_size(type->data.array.base);
  }
  else if(type->tag == KOOPA_RTT_POINTER){
    return 4;
  }
  else if(type->tag == KOOPA_RTT_INT32){
    return 4;
  }
  else{
    return 0;
  }
}

void PreVisit(const koopa_raw_slice_t &slice){
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        PreVisit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        PreVisit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  func_stack.push({func->name+sizeof(char), 0});
  if(func->bbs.len == 0) return;
  *op << "  .text" << std::endl;
  *op << "  .globl " << func->name+sizeof(char) << std::endl;
  *op << func->name+sizeof(char) << ":" << std::endl;
  called = false;
  args_max_len = 0;
  PreVisit(func->bbs);
  if(called) {
    func_stack.top().offset += 4;
    if(args_max_len > 8){
      func_stack.top().offset += ( args_max_len - 8 ) * 4;
      func_stack.top().alloc_offset = ( args_max_len - 8 ) * 4;
    }
  }
  func_stack.top().offset = (func_stack.top().offset + 15) / 16 * 16;
  if(func_stack.top().offset < 2048 && func_stack.top().offset > 0){
    *op << "  addi  sp, sp, -" << func_stack.top().offset << std::endl;
  }
  else if(func_stack.top().offset >= 2048){
    std::string reg = get_reg();
    *op << "  li    " << reg << ", " << func_stack.top().offset << std::endl;
    *op << "  sub   sp, sp, " << reg << std::endl;
    reset_reg(reg);
  }
  if(called){
    if(func_stack.top().offset - 4 < 2048){
      *op << "  sw    ra, " << func_stack.top().offset - 4 << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().offset - 4 << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  sw    ra, " << "0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  for (size_t i = 0; i < func->params.len; ++i) {
    auto ptr = func->params.buffer[i];
    koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
    if(i < 8) var_reg_map[(size_t)&(value->kind.data)] = "a" + std::to_string(i);
    else{
      func_stack.top().var_map[(size_t)&(value->kind.data)] = (i - 8) * 4 + func_stack.top().offset;
      std::cout << "func param " << i << " addr:";
    }
  }
  Visit(func->bbs);
  *op << std::endl;
  func_stack.pop();
}

void PreVisit(const koopa_raw_basic_block_t &bb){
  PreVisit(bb->insts);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  if(std::string(bb->name) != "%entry"){
    *op << bb->name+1 << ":" << std::endl;
  }
  Visit(bb->insts);
}

void PreVisit(const koopa_raw_value_t &value){
  // 根据指令类型判断后续需要如何访问
  const auto &ty = value->ty;
  switch (ty->tag) {
    case KOOPA_RTT_INT32:
      func_stack.top().offset += 4;
      if(value->kind.tag == KOOPA_RVT_CALL){
        called = true;
        args_max_len = max((int)value->kind.data.call.args.len, args_max_len);
      }
      break;
    case KOOPA_RTT_UNIT:
      if(value->kind.tag == KOOPA_RVT_CALL){
        called = true;
        args_max_len = max((int)value->kind.data.call.args.len, args_max_len);
      }
      break;    
    case KOOPA_RTT_POINTER:
      if(value->kind.tag == KOOPA_RVT_ALLOC){
        func_stack.top().offset += compute_size(value->ty->data.pointer.base);
        func_stack.top().offset += 4;
      }
      else{
        func_stack.top().offset += 4;
      }
      break;
    case KOOPA_RTT_FUNCTION:
      break;
    default:
      assert(false);
      break;
  }
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary);
      break;
    case KOOPA_RVT_ALLOC:
      func_stack.top().var_map[(size_t)&(kind.data)] = func_stack.top().alloc_offset;
      func_stack.top().alloc_offset += compute_size(value->ty->data.pointer.base);
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      Visit(kind.data.call);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      *op << "  .data" << std::endl;
      *op << "  .global " << value->name+1 << std::endl;
      *op << value->name+1 << ":" << std::endl;
      Visit(kind.data.global_alloc);
      *op << std::endl;
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

void Visit(const koopa_raw_return_t &ret) {
  std::cout << "Visit return" << std::endl;
  koopa_raw_value_t ret_value = ret.value; 
  if(called){
    if(func_stack.top().offset - 4 < 2048){
      *op << "  lw    ra, " << func_stack.top().offset - 4 << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().offset - 4 << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    ra, " << "0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  if(!ret_value){
    if(func_stack.top().offset < 2048 && func_stack.top().offset > 0) { 
      *op << "  addi  sp, sp, " << func_stack.top().offset << std::endl;
    }
    else if(func_stack.top().offset >= 2048){
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().offset << std::endl;
      *op << "  add   sp, sp, " << reg << std::endl;
      reset_reg(reg);
    }
  }
  else{
    if (ret_value->kind.tag == KOOPA_RVT_INTEGER){
      *op << "  li    a0, ";
      Visit(ret_value);
      *op << std::endl;
    } 
    else if(ret_value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << "a0" << ", " << ret_value->name+1 << std::endl;
      *op << "  lw    " << "a0" << ", " << "0(a0)" << std::endl;
    }
    else{
      if(func_stack.top().var_map[(size_t)&(ret_value->kind.data)] < 2048) {
        *op << "  lw    a0, " << func_stack.top().var_map[(size_t)&(ret_value->kind.data)] << "(sp)" << std::endl;
      }
      else{
        std::string reg = get_reg();
        *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(ret_value->kind.data)] << std::endl;
        *op << "  add   " << reg << ", sp, " << reg << std::endl;
        *op << "  lw    a0, 0(" << reg << ")" << std::endl;
        reset_reg(reg);
      }
    }      
    if(func_stack.top().offset < 2048 && func_stack.top().offset > 0) { 
      *op << "  addi  sp, sp, " << func_stack.top().offset << std::endl;
    }
    else if(func_stack.top().offset >= 2048){
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().offset << std::endl;
      *op << "  add   sp, sp, " << reg << std::endl;
      reset_reg(reg);
    }
  }
  *op << "  ret" << std::endl;
  return;
}

void Visit(const koopa_raw_integer_t &integer) {
  // 输出整数的值
  *op << integer.value;
}

void Visit(const koopa_raw_binary_t &binary) {

  // 访问操作符
  // 访问左右操作数
  if(binary.lhs->kind.tag == KOOPA_RVT_INTEGER){
    // if(binary.lhs->kind.data.integer.value == 0){
    //   var_reg_map[(size_t)&(binary.lhs->kind.data)] = "x0";
    // }
    // else{
      *op << "  li    ";
      *op << (var_reg_map[(size_t)&(binary.lhs->kind.data)] = get_reg()) << ", ";
      Visit(binary.lhs);
      *op << std::endl;
    // }
  }
  else if(binary.lhs->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(binary.lhs->kind.data)] = get_reg()) << ", " << binary.lhs->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", 0(" << var_reg_map[(size_t)&(binary.lhs->kind.data)] <<  ")" << std::endl;
  }
  else{
    if(func_stack.top().var_map[(size_t)&(binary.lhs->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(binary.lhs->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(binary.lhs->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(binary.lhs->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(binary.lhs->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  if(binary.rhs->kind.tag == KOOPA_RVT_INTEGER){
    // if(binary.rhs->kind.data.integer.value == 0){
    //   var_reg_map[(size_t)&(binary.rhs->kind.data)] = "x0";
    // }  
    // else{
      *op << "  li    ";
      *op << (var_reg_map[(size_t)&(binary.rhs->kind.data)] = get_reg()) << ", ";
      Visit(binary.rhs);
      *op << std::endl;
    // }
  }
  else if(binary.rhs->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(binary.rhs->kind.data)] = get_reg()) << ", " << binary.rhs->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << ", 0(" << var_reg_map[(size_t)&(binary.rhs->kind.data)] <<  ")" << std::endl;
  }
  else{
    if(func_stack.top().var_map[(size_t)&(binary.rhs->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(binary.rhs->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(binary.rhs->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(binary.rhs->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }

  switch (binary.op) {
    case KOOPA_RBO_EQ:
      // 访问EQ指令
      // ...  
      *op << "  xor   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      *op << "  seqz  " << var_reg_map[(size_t)&binary] << ", " << var_reg_map[(size_t)&binary] << std::endl;
      break;

    case KOOPA_RBO_ADD:
      // 访问加法指令
      // ...
      *op << "  add   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;
    case KOOPA_RBO_SUB:
      // 访问取反指令
      // ...
      *op << "  sub   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;
    case KOOPA_RBO_MUL:
      // 访问乘法指令
      // ...
      *op << "  mul   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;
    case KOOPA_RBO_DIV:
      // 访问除法指令
      *op << "  div   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;
    case KOOPA_RBO_MOD:
      // 访问取模指令
      *op << "  rem   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;

    case KOOPA_RBO_LT:
      *op << "  slt   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;
    case KOOPA_RBO_LE:
      *op << "  sgt   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      *op << "  seqz  " << var_reg_map[(size_t)&binary] << ", " << var_reg_map[(size_t)&binary] << std::endl;
      break;
    case KOOPA_RBO_GT:
      *op << "  sgt   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      break;
    case KOOPA_RBO_GE:
      *op << "  slt   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      *op << "  seqz  " << var_reg_map[(size_t)&binary] << ", " << var_reg_map[(size_t)&binary] << std::endl;
      break;
    case KOOPA_RBO_NOT_EQ:
      *op << "  xor   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;
      *op << "  snez  " << var_reg_map[(size_t)&binary] << ", " << var_reg_map[(size_t)&binary] << std::endl;
      break;
    case KOOPA_RBO_AND:
      *op << "  and   " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;

      break;
    case KOOPA_RBO_OR:
      *op << "  or    " << (var_reg_map[(size_t)&binary] = get_reg()) << ", " << var_reg_map[(size_t)&(binary.lhs->kind.data)] << ", " << var_reg_map[(size_t)&(binary.rhs->kind.data)] << std::endl;

      break;

    default:
      // 其他操作符暂时遇不到
      assert(false);
      break;
  }
  func_stack.top().var_map[(size_t)&binary] = func_stack.top().alloc_offset;
  func_stack.top().alloc_offset += 4;
  if(func_stack.top().var_map[(size_t)&binary] < 2048){
    *op << "  sw    " << var_reg_map[(size_t)&binary] << ", " << func_stack.top().var_map[(size_t)&binary] << "(sp)" << std::endl;
  }
  else{
    std::string reg = get_reg();
    *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&binary] << std::endl;
    *op << "  add   " << reg << ", sp, " << reg << std::endl;
    *op << "  sw    " << var_reg_map[(size_t)&binary] << ", 0(" << reg << ")" << std::endl;
    reset_reg(reg);
  }
  reset_reg(var_reg_map[(size_t)&(binary.lhs->kind.data)]);
  var_reg_map.erase((size_t)&(binary.lhs->kind.data));
  reset_reg(var_reg_map[(size_t)&(binary.rhs->kind.data)]);
  var_reg_map.erase((size_t)&(binary.rhs->kind.data));
  reset_reg(var_reg_map[(size_t)&binary]);
  var_reg_map.erase((size_t)&binary);
}

void output_aggregate(const koopa_raw_slice_t &elems){
  for(int i=0;i<elems.len;++i){
    auto ptr = elems.buffer[i];
    koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
    if(value->kind.tag == KOOPA_RVT_INTEGER){
      if(value->kind.data.integer.value == 0){
        *op << "  .zero 4" << std::endl;
      }
      else{
        *op << "  .word ";
        Visit(value->kind.data.integer);
        *op << std::endl;
      }
    }
    else if(value->kind.tag == KOOPA_RVT_AGGREGATE){
      output_aggregate(value->kind.data.aggregate.elems);
    }
  }
}

void Visit(const koopa_raw_global_alloc_t &global_alloc){
  if(global_alloc.init->kind.tag == KOOPA_RVT_INTEGER){
    if(global_alloc.init->kind.data.integer.value == 0) {
      *op << "  .zero 4" << std::endl;
    }
    else{
      *op<< "  .word ";
      Visit(global_alloc.init->kind.data.integer);
      *op << std::endl;
    }
  }
  else if(global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
    int size = compute_size(global_alloc.init->ty);
    *op << "  .zero "<< size << std::endl;
  }
  else if(global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE){
    output_aggregate(global_alloc.init->kind.data.aggregate.elems);
  }
}

void Visit(const koopa_raw_load_t &load){
  std::cout << load.src->kind.tag << std::endl;
  if(load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(load)] = get_reg()) << ", " << load.src->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(load)] << ", 0(" << var_reg_map[(size_t)&(load)] <<  ")" << std::endl;
      func_stack.top().var_map[(size_t)&load] = func_stack.top().alloc_offset;
      func_stack.top().alloc_offset += 4;
      if(func_stack.top().var_map[(size_t)&load] < 2048){
        *op << "  sw    " << var_reg_map[(size_t)&load] << ", " << func_stack.top().var_map[(size_t)&load] << "(" << "sp" << ")" << std::endl;
      }
      else{
        std::string reg = get_reg();
        *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&load] << std::endl;
        *op << "  add   " << reg << ", sp, " << reg << std::endl;
        *op << "  sw    " << var_reg_map[(size_t)&load] << ", 0(" << reg << ")" << std::endl;
        reset_reg(reg);
      }
    reset_reg(var_reg_map[(size_t)&load]);
    var_reg_map.erase((size_t)&load);
  }
  else if(load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || load.src->kind.tag == KOOPA_RVT_GET_PTR){
    if(func_stack.top().var_map[(size_t)&(load.src->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&load] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(load.src->kind.data)] << "(" << "sp" << ")" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(load.src->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&load] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
    std::string new_reg = get_reg();
    *op << "  lw    " << new_reg << ", 0(" << var_reg_map[(size_t)&load] << ")" << std::endl;
    reset_reg(var_reg_map[(size_t)&load]);
    var_reg_map[(size_t)&load] = new_reg;
    func_stack.top().var_map[(size_t)&load] = func_stack.top().alloc_offset;
    func_stack.top().alloc_offset += 4;
    if(func_stack.top().var_map[(size_t)&load] < 2048){
      *op << "  sw    " << var_reg_map[(size_t)&load] << ", " << func_stack.top().var_map[(size_t)&load] << "(" << "sp" << ")" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&load] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  sw    " << var_reg_map[(size_t)&load] << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
    reset_reg(var_reg_map[(size_t)&load]);
    var_reg_map.erase((size_t)&load);  }
  else{
    if(func_stack.top().var_map[(size_t)&(load.src->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&load] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(load.src->kind.data)] << "(" << "sp" << ")" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(load.src->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&load] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
    func_stack.top().var_map[(size_t)&load] = func_stack.top().alloc_offset;
    func_stack.top().alloc_offset += 4;
    if(func_stack.top().var_map[(size_t)&load] < 2048){
      *op << "  sw    " << var_reg_map[(size_t)&load] << ", " << func_stack.top().var_map[(size_t)&load] << "(" << "sp" << ")" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&load] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  sw    " << var_reg_map[(size_t)&load] << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
    reset_reg(var_reg_map[(size_t)&load]);
    var_reg_map.erase((size_t)&load);
  }
}

void Visit(const koopa_raw_store_t &store){
  if(store.value->kind.tag == KOOPA_RVT_INTEGER){
    // if(store.value->kind.data.integer.value == 0){
    //   var_reg_map[(size_t)&(store.value->kind.data)] = "x0";
    // }
    // else{
      *op << "  li    ";
      *op << (var_reg_map[(size_t)&(store.value->kind.data)] = get_reg()) << ", ";
      Visit(store.value);
      *op << std::endl;
    // }
  }
  else if(store.value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(store.value->kind.data)] = get_reg()) << ", " << store.value->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(store.value->kind.data)] << ", 0(" << var_reg_map[(size_t)&(store.value->kind.data)] <<  ")" << std::endl;
  }
  else{
    if(var_reg_map.find((size_t)&(store.value->kind.data)) == var_reg_map.end()){
      if(func_stack.top().var_map[(size_t)&(store.value->kind.data)] < 2048) {
        std::cout << "store value addr: ";
        *op << "  lw    " << (var_reg_map[(size_t)&(store.value->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(store.value->kind.data)] << "(" << "sp" << ")" << std::endl;
      }
      else{
        std::string reg = get_reg();
        *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(store.value->kind.data)]<< std::endl;
        *op << "  add   " << reg << ", sp, " << reg << std::endl;
        *op << "  lw    " << (var_reg_map[(size_t)&(store.value->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
        reset_reg(reg);
      }
    }
  }
  if(store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    *op << "  la    " << (var_reg_map[(size_t)&(store.dest->kind.data)] = get_reg()) << ", " << store.dest->name+1 << std::endl;
    *op << "  sw    " << var_reg_map[(size_t)&(store.value->kind.data)] << ", " << 0 << "(" << var_reg_map[(size_t)&(store.dest->kind.data)] << ")" << std::endl;
    reset_reg(var_reg_map[(size_t)&(store.dest->kind.data)]);
  }
  else if(store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || store.dest->kind.tag == KOOPA_RVT_GET_PTR){
    if(func_stack.top().var_map[(size_t)&(store.dest->kind.data)] < 2048){
      *op << "  lw    " << (var_reg_map[(size_t)&(store.dest->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(store.dest->kind.data)] << "(" << "sp" << ")" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(store.dest->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(store.dest->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
    *op << "  sw    " << var_reg_map[(size_t)&(store.value->kind.data)] << ", " << 0 << "(" << var_reg_map[(size_t)&(store.dest->kind.data)] << ")" << std::endl;
    reset_reg(var_reg_map[(size_t)&(store.dest->kind.data)]);
    var_reg_map.erase((size_t)&(store.dest->kind.data));
  }
  else{  
    if(func_stack.top().var_map[(size_t)&(store.dest->kind.data)] < 2048){
      *op << "  sw    " << var_reg_map[(size_t)&(store.value->kind.data)] << ", " << func_stack.top().var_map[(size_t)&(store.dest->kind.data)] << "(" << "sp" << ")" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(store.dest->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  sw    " << var_reg_map[(size_t)&(store.value->kind.data)] << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  reset_reg(var_reg_map[(size_t)&(store.value->kind.data)]);
  var_reg_map.erase((size_t)&(store.value->kind.data));
}

void Visit(const koopa_raw_branch_t &branch){
  if(branch.cond->kind.tag == KOOPA_RVT_INTEGER){
    // if(branch.cond->kind.data.integer.value == 0){
    //   var_reg_map[(size_t)&(branch.cond->kind.data)] = "x0";
    // }
    // else{
      *op << "  li    ";
      *op << (var_reg_map[(size_t)&(branch.cond->kind.data)] = get_reg()) << ", ";
      Visit(branch.cond);
      *op << std::endl;
    // }
  }
  else if(branch.cond->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(branch.cond->kind.data)] = get_reg()) << ", " << branch.cond->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(branch.cond->kind.data)] << ", 0(" << var_reg_map[(size_t)&(branch.cond->kind.data)] <<  ")" << std::endl;
  }
  else{
    if(func_stack.top().var_map[(size_t)&(branch.cond->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(branch.cond->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(branch.cond->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(branch.cond->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(branch.cond->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  *op << "  bnez  " << var_reg_map[(size_t)&(branch.cond->kind.data)] << ", bnez_block_true_" << std::to_string(nxt_block++) << std::endl;
  *op << "  j     bnez_block_false_" << std::to_string(nxt_block-1) << std::endl;
  *op << "bnez_block_true_" << std::to_string(nxt_block-1) << ":" << std::endl;
  *op << "  j     " << branch.true_bb->name+1 << std::endl;
  *op << "bnez_block_false_" << std::to_string(nxt_block-1) << ":" << std::endl;
  *op << "  j     " << branch.false_bb->name+1 << std::endl;
  reset_reg(var_reg_map[(size_t)&(branch.cond->kind.data)]);
  var_reg_map.erase((size_t)&(branch.cond->kind.data));
}

void Visit(const koopa_raw_jump_t &jump){
  *op << "  j     " << jump.target->name+1 << std::endl;
}

void Visit(const koopa_raw_call_t &call){
  koopa_raw_slice_t slice = call.args;
  // 在调用call之前，所有的局部变量都在内存里而寄存器内不存在局部变量，所以无需save寄存器？
  // TODO!
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    std::string arg_reg = "a" + std::to_string(i);
    koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
    switch (slice.kind) {
      case KOOPA_RSIK_VALUE:
        // 访问指令
        if(i < 8){
          if(value->kind.tag == KOOPA_RVT_INTEGER){
            *op << "  li    ";
            *op << arg_reg << ", ";
            Visit(value);
            *op << std::endl;
          }
          else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
            *op << "  la    " << arg_reg << ", " << value->name+1 << std::endl;
            *op << "  lw    " << arg_reg << ", 0(" << arg_reg <<  ")" << std::endl;
          }
          else{
            if(func_stack.top().var_map[(size_t)&(value->kind.data)] < 2048) {
              *op << "  lw    " << arg_reg << ", " << func_stack.top().var_map[(size_t)&(value->kind.data)] << "(sp)" << std::endl;
            }
            else{
              std::string reg = get_reg();
              *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(value->kind.data)] << std::endl;
              *op << "  add   " << reg << ", sp, " << reg << std::endl;
              *op << "  lw    " << arg_reg << ", 0(" << reg << ")" << std::endl;
              reset_reg(reg);
            }          
          }
        }
        else{
          if(value->kind.tag == KOOPA_RVT_INTEGER){
            *op << "  li    ";
            *op << (var_reg_map[(size_t)&(value->kind.data)] = get_reg()) << ", ";
            Visit(value);
            *op << std::endl;
            if((i - 8) * 4 < 2048){
              *op << "  sw    " << var_reg_map[(size_t)&(value->kind.data)]  << ", " << (i - 8) * 4 << "(" << "sp" << ")" << std::endl;
            }
            else{
              std::string reg = get_reg();
              *op << "  li    " << reg << ", " << (i - 8) * 4 << std::endl;
              *op << "  add   " << reg << ", sp, " << reg << std::endl;
              *op << "  sw    " << var_reg_map[(size_t)&(value->kind.data)] << ", 0(" << reg << ")" << std::endl;
              reset_reg(reg);
            }
            reset_reg(var_reg_map[(size_t)&(value->kind.data)]);
            var_reg_map.erase((size_t)&(value->kind.data));
          }
          else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
            *op << "  la    " << (var_reg_map[(size_t)&(value->kind.data)] = get_reg()) << ", " << value->name+1 << std::endl;
            *op << "  lw    " << var_reg_map[(size_t)&(value->kind.data)]  << ", 0(" << var_reg_map[(size_t)&(value->kind.data)] <<  ")" << std::endl;
            if((i - 8) * 4 < 2048){
              *op << "  sw    " << var_reg_map[(size_t)&(value->kind.data)]  << ", " << (i - 8) * 4 << "(" << "sp" << ")" << std::endl;
            }
            else{
              std::string reg = get_reg();
              *op << "  li    " << reg << ", " << (i - 8) * 4 << std::endl;
              *op << "  add   " << reg << ", sp, " << reg << std::endl;
              *op << "  sw    " << var_reg_map[(size_t)&(value->kind.data)] << ", 0(" << reg << ")" << std::endl;
              reset_reg(reg);
            }
            reset_reg(var_reg_map[(size_t)&(value->kind.data)]);
            var_reg_map.erase((size_t)&(value->kind.data));
          }
          else{
            std::string reg1 = get_reg();
            if(func_stack.top().var_map[(size_t)&(value->kind.data)] < 2048) {
              *op << "  lw    " << reg1 << ", " << func_stack.top().var_map[(size_t)&(value->kind.data)] << "(sp)" << std::endl;
            }
            else{
              std::string reg = get_reg();
              *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(value->kind.data)] << std::endl;
              *op << "  add   " << reg << ", sp, " << reg << std::endl;
              *op << "  lw    " << reg1 << ", 0(" << reg << ")" << std::endl;
              reset_reg(reg);
            }
            if((i - 8) * 4 < 2048){
              *op << "  sw    " << reg1  << ", " << (i - 8) * 4 << "(" << "sp" << ")" << std::endl;
            }
            else{
              std::string reg = get_reg();
              *op << "  li    " << reg << ", " << (i - 8) * 4 << std::endl;
              *op << "  add   " << reg << ", sp, " << reg << std::endl;
              *op << "  sw    " << reg1 << ", 0(" << reg << ")" << std::endl;
              reset_reg(reg);
            }
            reset_reg(reg1);
          }
        }
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
  *op << "  call " << call.callee->name+1 << std::endl;
  if(call.callee->ty->data.function.ret->tag == KOOPA_RTT_INT32) {
    func_stack.top().var_map[(size_t)&call] = func_stack.top().alloc_offset;
    func_stack.top().alloc_offset += 4;
    if(func_stack.top().var_map[(size_t)&call] < 2048){
      *op << "  sw    " << "a0" << ", " << func_stack.top().var_map[(size_t)&call] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&call] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  sw    " << "a0" << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
}


void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr){
  std::cout << "get_elem_ptr from: " << get_elem_ptr.src->kind.tag << std::endl;
  if(get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)] = get_reg()) << ", " << get_elem_ptr.src->name+1 << std::endl;
  }
  else if(get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_PTR){
    if(func_stack.top().var_map[(size_t)&(get_elem_ptr.src->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(get_elem_ptr.src->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(get_elem_ptr.src->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  else{
    if(func_stack.top().var_map[(size_t)&(get_elem_ptr.src->kind.data)] < 2048) {
      *op << "  addi  " << (var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)] = get_reg()) << ", sp, " << func_stack.top().var_map[(size_t)&(get_elem_ptr.src->kind.data)] << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(get_elem_ptr.src->kind.data)] << std::endl;
      *op << "  add   " << (var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)] = get_reg())<< ", sp, " << reg << std::endl;
      reset_reg(reg);
    }
  }
  if(get_elem_ptr.index->kind.tag == KOOPA_RVT_INTEGER){
    *op << "  li    ";
    *op << (var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] = get_reg()) << ", ";
    Visit(get_elem_ptr.index);
    *op << std::endl;
  }
  else if(get_elem_ptr.index->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] = get_reg()) << ", " << get_elem_ptr.index->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] << ", 0(" << var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] <<  ")" << std::endl;
  }
  else{
    if(func_stack.top().var_map[(size_t)&(get_elem_ptr.index->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(get_elem_ptr.index->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(get_elem_ptr.index->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  koopa_raw_type_t ty = get_elem_ptr.src->ty->data.pointer.base->data.array.base;
  int size = compute_size(ty);
  std::string size_reg = get_reg();
  *op << "  li    " << size_reg << ", " << size << std::endl;
  *op << "  mul   " << var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] << ", " << var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] << ", " << size_reg << std::endl;
  *op << "  add   " << (var_reg_map[(size_t)&get_elem_ptr] = get_reg()) << ", " << var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)] << ", " << var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)] << std::endl;
  reset_reg(size_reg);
  func_stack.top().var_map[(size_t)&get_elem_ptr] = func_stack.top().alloc_offset;
  func_stack.top().alloc_offset += 4;
  if(func_stack.top().var_map[(size_t)&get_elem_ptr] < 2048){
    *op << "  sw    " << var_reg_map[(size_t)&get_elem_ptr] << ", " << func_stack.top().var_map[(size_t)&get_elem_ptr] << "(sp)" << std::endl;
  }
  else{
    std::string reg = get_reg();
    *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&get_elem_ptr] << std::endl;
    *op << "  add   " << reg << ", sp, " << reg << std::endl;
    *op << "  sw    " << var_reg_map[(size_t)&get_elem_ptr] << ", 0(" << reg << ")" << std::endl;
    reset_reg(reg);
  }
  reset_reg(var_reg_map[(size_t)&(get_elem_ptr.index->kind.data)]);
  reset_reg(var_reg_map[(size_t)&(get_elem_ptr.src->kind.data)]);
  reset_reg(var_reg_map[(size_t)&get_elem_ptr]);
  var_reg_map.erase((size_t)&(get_elem_ptr.index->kind.data));
  var_reg_map.erase((size_t)&(get_elem_ptr.src->kind.data));
  var_reg_map.erase((size_t)&get_elem_ptr);
}

void Visit(const koopa_raw_get_ptr_t &get_ptr){
  std::cout << "get_ptr from: " << get_ptr.src->kind.tag << std::endl;
  if(get_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(get_ptr.src->kind.data)] = get_reg()) << ", " << get_ptr.src->name+1 << std::endl;
  }
  else if(get_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || get_ptr.src->kind.tag == KOOPA_RVT_GET_PTR || get_ptr.src->kind.tag == KOOPA_RVT_LOAD){
    if(func_stack.top().var_map[(size_t)&(get_ptr.src->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(get_ptr.src->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(get_ptr.src->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(get_ptr.src->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(get_ptr.src->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }

  else{
    if(func_stack.top().var_map[(size_t)&(get_ptr.src->kind.data)] < 2048) {
      *op << "  addi  " << (var_reg_map[(size_t)&(get_ptr.src->kind.data)] = get_reg()) << ", sp, " << func_stack.top().var_map[(size_t)&(get_ptr.src->kind.data)] << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(get_ptr.src->kind.data)] << std::endl;
      *op << "  add   " << (var_reg_map[(size_t)&(get_ptr.src->kind.data)] = get_reg()) << ", sp, " << reg << std::endl;
      reset_reg(reg);
    }
  }
  if(get_ptr.index->kind.tag == KOOPA_RVT_INTEGER){
    *op << "  li    ";
    *op << (var_reg_map[(size_t)&(get_ptr.index->kind.data)] = get_reg()) << ", ";
    Visit(get_ptr.index);
    *op << std::endl;
  }
  else if(get_ptr.index->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      *op << "  la    " << (var_reg_map[(size_t)&(get_ptr.index->kind.data)] = get_reg()) << ", " << get_ptr.index->name+1 << std::endl;
      *op << "  lw    " << var_reg_map[(size_t)&(get_ptr.index->kind.data)] << ", 0(" << var_reg_map[(size_t)&(get_ptr.index->kind.data)] <<  ")" << std::endl;
  }
  else{
    if(func_stack.top().var_map[(size_t)&(get_ptr.index->kind.data)] < 2048) {
      *op << "  lw    " << (var_reg_map[(size_t)&(get_ptr.index->kind.data)] = get_reg()) << ", " << func_stack.top().var_map[(size_t)&(get_ptr.index->kind.data)] << "(sp)" << std::endl;
    }
    else{
      std::string reg = get_reg();
      *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&(get_ptr.index->kind.data)] << std::endl;
      *op << "  add   " << reg << ", sp, " << reg << std::endl;
      *op << "  lw    " << (var_reg_map[(size_t)&(get_ptr.index->kind.data)] = get_reg()) << ", 0(" << reg << ")" << std::endl;
      reset_reg(reg);
    }
  }
  koopa_raw_type_t ty = get_ptr.src->ty->data.pointer.base;
  int size = compute_size(ty);
  std::string size_reg = get_reg();
  *op << "  li    " << size_reg << ", " << size << std::endl;
  *op << "  mul   " << var_reg_map[(size_t)&(get_ptr.index->kind.data)] << ", " << var_reg_map[(size_t)&(get_ptr.index->kind.data)] << ", " << size_reg << std::endl;
  *op << "  add   " << (var_reg_map[(size_t)&get_ptr] = get_reg()) << ", " << var_reg_map[(size_t)&(get_ptr.src->kind.data)] << ", " << var_reg_map[(size_t)&(get_ptr.index->kind.data)] << std::endl;
  reset_reg(size_reg);
  func_stack.top().var_map[(size_t)&get_ptr] = func_stack.top().alloc_offset;
  func_stack.top().alloc_offset += 4;
  if(func_stack.top().var_map[(size_t)&get_ptr] < 2048){
    *op << "  sw    " << var_reg_map[(size_t)&get_ptr] << ", " << func_stack.top().var_map[(size_t)&get_ptr] << "(sp)" << std::endl;
  }
  else{
    std::string reg = get_reg();
    *op << "  li    " << reg << ", " << func_stack.top().var_map[(size_t)&get_ptr] << std::endl;
    *op << "  add   " << reg << ", sp, " << reg << std::endl;
    *op << "  sw    " << var_reg_map[(size_t)&get_ptr] << ", 0(" << reg << ")" << std::endl;
    reset_reg(reg);
  }
  reset_reg(var_reg_map[(size_t)&(get_ptr.index->kind.data)]);
  reset_reg(var_reg_map[(size_t)&(get_ptr.src->kind.data)]);
  reset_reg(var_reg_map[(size_t)&get_ptr]);
  var_reg_map.erase((size_t)&(get_ptr.index->kind.data));
  var_reg_map.erase((size_t)&(get_ptr.src->kind.data));
  var_reg_map.erase((size_t)&get_ptr);

}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...

int main(int argc, const char *argv[]) {
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  std::cout.setf(std::ios::unitbuf);
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  // 输出解析得到的 AST, 其实就是个字符串
  // ast->Dump();cout << endl;
  if((std::string)mode == "-koopa"){
    std::ofstream outputFile(output);
    ast->Dump();
    ast->to_koopa_string(outputFile);
    outputFile.close();
  }
  else if((std::string)mode == "-riscv"||(std::string)mode == "-perf"){
    std::ostringstream astoss;
    ast->to_koopa_string(astoss);
    std::string aststr = astoss.str();
    // std::cout << aststr;
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t kooparet = koopa_parse_from_string(aststr.c_str(), &program);
    assert(kooparet == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    // 使用 for 循环遍历函数列表
    std::ofstream outputFile(output);
    op = &outputFile;
    Visit(raw);
    koopa_delete_raw_program_builder(builder);
    outputFile.close();
  }
  else if((std::string)mode == "-ast"){
    ast->Dump();
    std::cout << std::endl;

  }
  return 0;
}
