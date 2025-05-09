#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "koopa.h"

using namespace std;

//RISCV汇编生成模块
//封装了生成 RISC-V 汇编代码所需的各种功能，包括地址管理、指令生成、函数和基本块的处理等
class RISCV_Module {
  //内部类 Env,用于管理栈空间和地址映射
  class Env {
    map<koopa_raw_value_t, int> addr_map;  //存储指令与其在栈中的地址的映射关系
   public:
    int stack_size = 0;   //当前栈空间的总大小
    int cur_size = 0;     //当前已分配的栈空间大小
    bool is_call = false; //当前函数是否包含函数调用

    void init(int size, bool call);   //初始化栈空间和函数调用标志
    int get_addr(koopa_raw_value_t value);    //获取指令在栈中的地址。如果指令尚未分配地址，则为其分配一个新的地址。

  };
  Env env;

  ofstream out;   //用于将生成的汇编代码写入文件
  //计算 Koopa IR 中类型的大小（字节数）
  static int type_size(koopa_raw_type_t ty);
  //计算数组类型的大小
  static int array_size(koopa_raw_type_t value);
  //计算基本块的大小
  static int bb_size(koopa_raw_basic_block_t bb, bool &call, int &max_arg);
  //计算单条指令的大小
  static int inst_size(koopa_raw_value_t value);
  //计算函数的大小
  static int func_size(koopa_raw_function_t func, bool &call);
  //将值从内存加载到寄存器
  void load_register(koopa_raw_value_t value, string reg);
  //将值从寄存器存储到栈中
  void store_stack(int addr, string reg);

  // 解析program
  void raw_analyze(const koopa_raw_program_t &raw);
  // 解析 raw slice
  void raw_analyze(const koopa_raw_slice_t &slice);
  // 解析函数
  void raw_analyze(const koopa_raw_function_t &func);
  // 解析基本块
  void raw_analyze(const koopa_raw_basic_block_t &bb); 
  // 解析指令
  void raw_analyze(const koopa_raw_value_t &value);
  
  // 1. Function return
  void raw_analyze(const koopa_raw_return_t &return_value);
  // 2. Memory load
  void raw_analyze(const koopa_raw_load_t &load_value, int addr);
  // 3. Memory store
  void raw_analyze(const koopa_raw_store_t &store_value);
  // 4. Binary operation
  void raw_analyze(const koopa_raw_binary_t &binary_value, int addr);
  // 5. Conditional branch
  void raw_analyze(const koopa_raw_branch_t &branch_value);
  // 6. Unconditional jump
  void raw_analyze(const koopa_raw_jump_t &jump_value);
  // 7. Function call
  void raw_analyze(const koopa_raw_call_t &call_value, int addr);
  // 8. Global memory allocation
  void global_alloc(const koopa_raw_value_t &global_alloc_value);
  // aggregate
  void raw_analyze(const koopa_raw_aggregate_t &aggregate_value);
  // 9. Element pointer calculation
  void raw_analyze(const koopa_raw_get_elem_ptr_t &get_elem_ptr_value, int addr);
  // 10. Pointer calculation
  void raw_analyze(const koopa_raw_get_ptr_t &get_ptr_value, int addr);

 public:
   RISCV_Module(const char *path) {
      out.open(path);
   };
  //将raw以RISCV汇编格式输出到文件 
  void raw_dump_to_riscv(koopa_raw_program_t raw);
};