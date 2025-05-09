#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <typeinfo>
#include <map>
#include "koopa.h"

struct Reg { 
    int index; 
    int offset;

    Reg(int _index, int _offset) : index(_index), offset(_offset) {}
    Reg() {}
};

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_return_t &ret);
Reg Visit(const koopa_raw_value_t &value);
Reg Visit(const koopa_raw_integer_t &integer);
Reg Visit(const koopa_raw_binary_t &binary);
Reg Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
Reg Visit(const koopa_raw_call_t &call);
Reg Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr);
Reg Visit(const koopa_raw_get_ptr_t &get_ptr);
std::string Visit(const koopa_raw_global_alloc_t &global);

void instOperation(std::string inst, std::string reg, int32_t imm);
void instOperation(std::string inst, std::string reg1, std::string reg2, int32_t imm);
void instOperation(std::string inst, std::string reg1, std::string reg2);
void instOperation(std::string inst, std::string reg, std::string reg1, std::string reg2);
