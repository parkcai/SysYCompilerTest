#pragma once
#include <iostream>
#include <cassert>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>
#include "koopa.h"

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);

void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);

std::string findReg();
void giveReg(const koopa_raw_value_t &value);
void invalidReg(std::string reg);

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value);
void Visit(const koopa_raw_store_t &store);

void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
void sw_func(std::string reg, std::string biassp);
void lw_func(std::string dest, std::string src);

void Visit(const koopa_raw_call_t& call, const koopa_raw_value_t& value);
void Visit(const koopa_raw_global_alloc_t& global, const koopa_raw_value_t& value);
void Visit(const koopa_raw_get_elem_ptr_t &getelemptr, const koopa_raw_value_t &value);
void Visit(const koopa_raw_get_ptr_t &getptr, const koopa_raw_value_t &value);
void printAgg(const koopa_raw_value_t &value);
void Visit(int alloc, const koopa_raw_value_t &value);
int isptr(const koopa_raw_value_t &value);
void addi_func(std::string reg, std::string biassp);
int cal_arraysize(const koopa_raw_type_t &ty);
int getnum(const koopa_raw_value_t &value);
int getnum2(const koopa_raw_type_t &ty);