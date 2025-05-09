#include <iostream>
#include <assert.h>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "koopa.h"
#include "visitraw.hpp"
#include <unordered_map>

using namespace std;
static unordered_map<koopa_raw_value_t, int> stack_location;
static unordered_map<koopa_raw_value_t, vector<int> > array_dimension;
static int stack_length = 0;
static int stack_used = 0;
static int need_ra = 0;
static int jump_station = 0;

void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
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
  if(func->bbs.len == 0)
    return;
  // 执行一些其他的必要操作
  cout<<"  .text\n";
  cout<<"  .globl "<<(func->name)+1<<endl;
  cout<<(func->name)+1<<":\n";

  stack_length = 0;
  stack_used = 0;
  need_ra = 0;
  int var_cnt = 0;
  int max_argc = 0;
  // 遍历基本块
  for (size_t i = 0; i < func->bbs.len; ++i){
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    var_cnt += insts.len;
    for (size_t j = 0; j < insts.len; ++j){
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if (inst->kind.tag == KOOPA_RVT_CALL){
        need_ra = 1;
        max_argc = max(max_argc, (int)inst->kind.data.call.args.len);
      }
      else if ((inst->kind.tag == KOOPA_RVT_ALLOC) && (inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)){
        // 数组所占空间并非为1，而是数组长度
        var_cnt--;
        int arr_size = 1;
        auto base = inst->ty->data.pointer.base;
        while (base->tag == KOOPA_RTT_ARRAY){
          arr_size *= base->data.array.len;
          array_dimension[inst].push_back(base->data.array.len);
          base = base->data.array.base;
        }
        var_cnt += arr_size;
      }
    }
  }
  stack_length = (var_cnt + need_ra + max(max_argc - 8, 0)) << 2;
  stack_used = (max(max_argc - 8, 0)) << 2;
  // 把返回地址ra加载到栈中
  if (need_ra){
    cout << "  li t0, -4" << endl;
    cout << "  add t0, sp, t0" << endl;
    cout << "  sw ra, (t0)" << endl;
  }
  if (stack_length != 0){
      cout << "  li t0, "<<-stack_length<<endl;
      cout << "  add sp, sp, t0" << endl;
  }
  // 访问所有基本块
  Visit(func->bbs);
  cout << endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  if (strcmp((bb->name) + 1, "entry") != 0){  // koopa的基本块名字前面都有一个%
    cout<<(bb->name) + 1<<":\n";
  }
  Visit(bb->insts);
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
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store, value);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      // 访问 global_alloc 指令
      Visit(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_GET_PTR:
      // 访问 get_ptr 指令
      Visit(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      // 访问 get_elem_ptr 指令
      Visit(kind.data.get_elem_ptr, value);
      break;
    default:
      if (kind.tag != KOOPA_RVT_ALLOC){
        assert(false);
      }
      // 访问 alloc 指令
      auto base = value->ty->data.pointer.base;
      if (base->tag == KOOPA_RTT_ARRAY){
        // 在这里已经将数组的维度存储在array_dimension中，不用重复计算
        int arr_size = 4;
        for (size_t i = 0; i < array_dimension[value].size(); ++i){
          arr_size *= array_dimension[value][i];
        }
        stack_location[value] = stack_used;
        stack_used += arr_size;
      }
      else if (base->tag == KOOPA_RTT_POINTER){
        // 将*转化为大小为1的维度
        while (base->tag == KOOPA_RTT_POINTER){
          array_dimension[value].push_back(1);
          base = base->data.pointer.base;
        }
        while (base->tag == KOOPA_RTT_ARRAY){
          array_dimension[value].push_back(base->data.array.len);
          base = base->data.array.base;
        }
        stack_location[value] = stack_used;
        stack_used += 4;
      }
      else if (base->tag == KOOPA_RTT_INT32){
        stack_location[value] = stack_used;
        stack_used += 4;
      }
      break;
  }
}

// 将value的地址加载进寄存器
static void Load_Addr_To_Reg(const koopa_raw_value_t &value, const string &reg) {
  if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    cout << "  la " << reg << ", " << value->name + 1 << endl;
  }
  else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
    const auto &index = value->kind.data.func_arg_ref.index;
    if (index < 8){
      cout << "  mv " << reg << ", a" << index << endl;
    }
    else {
      cout << "  li t2, " << stack_length + ((index - 8) << 2) << endl;
      cout << "  add " << reg << ", sp, t2" << endl;
    }
  }
  else {
    cout << "  li t2, " << stack_location[value] << endl;
    cout << "  add " << reg << ", sp, t2" << endl;
  }
}

// 将寄存器内容存入指令对应的栈位置中
static void Save_Reg_To_Stack(const koopa_raw_value_t &value, const string &reg) {
  if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    cout << "  la t2, " << value->name + 1 << endl;
    cout << "  sw " << reg << ", (t2)" << endl;
  }
  else {
    cout << "  li t2, " << stack_location[value] << endl;
    cout << "  add t2, sp, t2" << endl;
    cout << "  sw " << reg << ", (t2)" << endl;
  }
}


// 将内容加载进寄存器
static void Load_Data_To_Reg(const koopa_raw_value_t &value, const string &reg) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    cout << "  li " << reg << ", " << value->kind.data.integer.value << endl;
  }
  // 需要将函数参数加载进寄存器时，需要特殊处理
  else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    const auto &index = value->kind.data.func_arg_ref.index;
    if (index < 8){
      cout << "  mv " << reg << ", a" << index << endl;
    }
    else {
      cout << "  li t2, " << stack_length + ((index - 8) << 2) << endl;
      cout << "  add t2, sp, t2" << endl;
      cout << "  lw " << reg << ", (t2)" << endl;
    }
  }
  // 加载的是全局变量时需要load address
  else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    cout << "  la t2, " << value->name + 1 << endl;
    cout << "  lw " << reg << ", (t2)" << endl;
  }
  else {
    cout << "  li t2, " << stack_location[value] << endl;
    cout << "  add t2, sp, t2" << endl;
    cout << "  lw " << reg << ", (t2)" << endl;
  }
}

static bool Is_Pointer(const koopa_raw_value_t &value){
  return (value->kind.tag == KOOPA_RVT_GET_PTR) || (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
          || (value->kind.tag == KOOPA_RVT_LOAD && 
              value->kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC && 
              value->kind.data.load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER);
}

void Visit(const koopa_raw_branch_t &branch) {
  Load_Data_To_Reg(branch.cond, "t0");
  cout << "  bnez t0, " << "STATION_" << jump_station << endl;
  cout << "  j " << branch.false_bb->name+1 << endl;
  cout << "STATION_" << jump_station << ":" << endl;
  jump_station++;
  cout << "  j " << branch.true_bb->name+1 << endl;
}

void Visit(const koopa_raw_jump_t &jump) {
  cout << "  j " << jump.target->name+1 << endl;
}

static void aggregate(const koopa_raw_value_t value){
  if (value->kind.tag == KOOPA_RVT_INTEGER){
    cout << "  .word " << value->kind.data.integer.value << endl;
    return;
  }
  for (size_t i = 0; i < value->kind.data.aggregate.elems.len; ++i){
    auto elem = reinterpret_cast<koopa_raw_value_t>(value->kind.data.aggregate.elems.buffer[i]);
    aggregate(elem);
  }
}

// 访问 global_alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  
  cout << "  .data" << endl;
  cout << "  .globl " << value->name + 1 << endl; // global_alloc.name 跳过 '%' 符号
  cout << value->name + 1 << ":" << endl;
  
  // 输出分配的初始值（如果有的话）
  if (global_alloc.init->kind.tag != KOOPA_RVT_ZERO_INIT) {
    if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
      cout << "  .word " << global_alloc.init->kind.data.integer.value << endl;
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
      auto base = value->ty->data.pointer.base;
      while (base->tag == KOOPA_RTT_ARRAY){
        array_dimension[value].push_back(base->data.array.len);
        base = base->data.array.base;
      }
      aggregate(global_alloc.init);
    }
  } else {
    auto base = value->ty->data.pointer.base;
    if (base->tag == KOOPA_RTT_INT32){
      cout << "  .word 0" << endl; // 默认初始化为0
    }
    else {
      int zero_num = 4;
      while (base->tag == KOOPA_RTT_ARRAY){
        array_dimension[value].push_back(base->data.array.len);
        zero_num *= base->data.array.len;
        base = base->data.array.base;
      }
      cout << "  .zero " << zero_num << endl;
    }
  }
  cout << endl;
}

// 访问 getptr 指令
void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
  auto dimension =  array_dimension[get_ptr.src];
  int offset = 4;
  for (size_t i = 1; i < dimension.size(); ++i){
    offset *= dimension[i];
  }
  Load_Addr_To_Reg(get_ptr.src, "t0");
  if (Is_Pointer(get_ptr.src)){
    cout << "  lw t0, (t0)" << endl;
  }
  Load_Data_To_Reg(get_ptr.index, "t1");
  cout << "  li t2, " << offset << endl;
  cout << "  mul t1, t1, t2" << endl;
  cout << "  add t0, t0, t1" << endl;
  if (value->ty->tag != KOOPA_RTT_UNIT){
    array_dimension[value] = vector<int>(dimension.begin() + 1, dimension.end());
    stack_location[value] = stack_used;
    stack_used += 4;
    Save_Reg_To_Stack(value, "t0");
  }
}

// 访问 get_elem_ptr 指令
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
  auto dimension =  array_dimension[get_elem_ptr.src];
  int offset = 4;
  for (size_t i = 1; i < dimension.size(); ++i){
    offset *= dimension[i];
  }
  Load_Addr_To_Reg(get_elem_ptr.src, "t0");
  if (Is_Pointer(get_elem_ptr.src)){
    cout << "  lw t0, (t0)" << endl;
  }
  Load_Data_To_Reg(get_elem_ptr.index, "t1");
  cout << "  li t2, " << offset << endl;
  cout << "  mul t1, t1, t2" << endl;
  cout << "  add t0, t0, t1" << endl;
  if (value->ty->tag != KOOPA_RTT_UNIT){
    array_dimension[value] = vector<int>(dimension.begin() + 1, dimension.end());
    stack_location[value] = stack_used;
    stack_used += 4;
    Save_Reg_To_Stack(value, "t0");
  }
}

// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
  // 将参数加载进寄存器
  for (size_t i = 0; i < call.args.len; ++i){
    if (i<8){
      Load_Data_To_Reg(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), "a" + to_string(i));
    }
    else{
      Load_Data_To_Reg(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), "t0");
      cout << "  li t2, " << ((i - 8) << 2) << endl;
      cout << " add t2, sp, t2" << endl;
      cout << "  sw t0, " <<  "(t2)" << endl;
    }
  }
  cout << "  call " << call.callee->name+1 << endl;
  // 如果有返回值要压栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    stack_location[value] = stack_used;
    stack_used += 4;
    Save_Reg_To_Stack(value, "a0");
  }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &value) {
    if (value.value != nullptr){
      Load_Data_To_Reg(value.value, "a0");
    }
    // 要恢复ra寄存器
    if (need_ra) {
      cout << "  li t0, " << stack_length - 4 << endl;
      cout << "  add t0, sp, t0" << endl;
      cout << "  lw ra, (t0)" << endl;
    }
    if (stack_length != 0){
      cout << "  li t0, "<< stack_length <<endl;
      cout << "  add sp, t0, sp" << endl;
    }
    cout<<"  ret\n";
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &value) {
    cout<<"  li a0, "<<value.value<<"\n";
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store, const koopa_raw_value_t &value) {
  Load_Data_To_Reg(store.value, "t0");
  if (Is_Pointer(store.value)){
    cout << "  lw t0, (t0)" << endl;
  }
  Load_Addr_To_Reg(store.dest, "t1");
  if (Is_Pointer(store.dest)){
    cout << "  lw t1, (t1)" << endl;
  }
  cout << "  sw t0, (t1)" << endl;
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t & value){
  Load_Data_To_Reg(load.src, "t0");
  if (Is_Pointer(load.src)){
    cout << "  lw t0, (t0)" << endl;
  }
  // 把load出来的值存入栈中
  if (array_dimension.find(load.src) != array_dimension.end()){
    array_dimension[value] = array_dimension[load.src];
  }
  stack_location[value] = stack_used;
  stack_used += 4;
  Save_Reg_To_Stack(value, "t0");
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
  // t0和t1是用来临时运算的，并最后存回t0
  Load_Data_To_Reg(binary.lhs, "t0");
  Load_Data_To_Reg(binary.rhs, "t1");
  switch (binary.op) {
    case KOOPA_RBO_NOT_EQ:
      cout << "  xor t0, t0, t1" << endl;
      cout << "  snez t0, t0" << endl;
      break;
    case KOOPA_RBO_EQ:
      cout << "  xor t0, t0, t1" << endl;
      cout << "  seqz t0, t0" << endl;
      break;
    case KOOPA_RBO_GT: 
      cout << "  sgt t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_GE: 
      cout << "  slt t0, t0, t1" << endl;
      cout << "  xori t0, t0, 1" << endl;
      break;
    case KOOPA_RBO_LT: 
      cout << "  slt t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_LE: 
      cout << "  sgt t0, t0, t1" << endl;
      cout << "  xori t0, t0, 1" << endl;
      break;
    case KOOPA_RBO_ADD:
      cout << "  add t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_SUB:
      cout << "  sub t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_MUL:
      cout << "  mul t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_DIV:
      cout << "  div t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_MOD:
      cout << "  rem t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_AND:
      cout << "  and t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_OR: 
      cout << "  or t0, t0, t1" << endl;
      break;    
    case KOOPA_RBO_XOR:
      cout << "  xor t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_SHL:
      cout << "  sll t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_SHR:
      cout << "  srl t0, t0, t1" << endl;
      break;
    case KOOPA_RBO_SAR:
      cout << "  sra t0, t0, t1" << endl;
      break;
    default:
      assert(false);
    }
  stack_location[value] = stack_used;
  stack_used += 4;
  Save_Reg_To_Stack(value, "t0");
}


// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...