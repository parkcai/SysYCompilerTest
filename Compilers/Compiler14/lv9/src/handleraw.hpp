#pragma once
#include <string>
#include "koopa.h"

void Visit_raw_program(const koopa_raw_program_t &program);    // 访问 raw program

void Visit_raw_slice(const koopa_raw_slice_t &slice);        // 访问 raw slice

void Visit_function(const koopa_raw_function_t &func);      // 访问函数

void Visit_block(const koopa_raw_basic_block_t &bb);     // 访问基本块

void Visit_instruction(const koopa_raw_value_t &value);        // 访问指令

void Visit_binary(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);   // 访问 binary 指令

void Visit_integer(const koopa_raw_integer_t &integer);    // 访问integer

void Visit_return(const koopa_raw_return_t &ret);       // 访问return指令

void Visit_load(const koopa_raw_load_t &load, const koopa_raw_value_t &value);    // 访问load指令

void Visit_store(const koopa_raw_store_t &store);       // 访问store指令

void Visit_branch(const koopa_raw_branch_t &branch);    // 访问branch指令

void Visit_jump(const koopa_raw_jump_t &jump);          // 访问jump指令

void Visit_call(const koopa_raw_call_t &call, const koopa_raw_value_t &value);    // 访问call指令

void Visit_local_alloc(const koopa_raw_value_t &value);    // 访问local_alloc指令

void Visit_global_alloc(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);    // 访问global_alloc指令

void Visit_getptr(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value);     // 访问getptr指令

void Visit_getelemptr(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value);    // 访问getelemptr指令