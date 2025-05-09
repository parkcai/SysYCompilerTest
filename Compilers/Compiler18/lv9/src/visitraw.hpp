#pragma once
#include "koopa.h"
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
// 访问 return 指令
void Visit(const koopa_raw_return_t &value);
// 访问 integer 指令
void Visit(const koopa_raw_integer_t &value);
// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);
// 访问 store 指令
void Visit(const koopa_raw_store_t &store, const koopa_raw_value_t &value);
// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t & value);
// 访问 branch 指令
void Visit(const koopa_raw_branch_t &store);
// 访问 jump 指令
void Visit(const koopa_raw_jump_t &load);
// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value);
// 访问 global_alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);
// 访问 getptr 指令
void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value);
// 访问 getelementptr 指令
void Visit(const koopa_raw_get_elem_ptr_t &get_element_ptr, const koopa_raw_value_t &value);