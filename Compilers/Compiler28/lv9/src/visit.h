#pragma once
#include "koopa.h"
#include "utils.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <string>
#include <map>

int get_addr(const koopa_raw_value_t &value);
void Visit(const koopa_raw_program_t &program, std::ostream &cout);
void Visit(const koopa_raw_slice_t &slice, std::ostream &cout);
void Visit(const koopa_raw_function_t &function, std::ostream &cout);
void Visit(const koopa_raw_basic_block_t &basic_block, std::ostream &cout);
void Visit(const koopa_raw_value_t &value, std::ostream &cout);
void Visit(const koopa_raw_integer_t &integer, std::ostream &cout);
void Visit(const koopa_raw_return_t &ret, std::ostream &cout);
void Visit(const koopa_raw_binary_t &binary_op, int dest_addr, std::ostream &cout);
void Visit(const koopa_raw_load_t &load, int dest_addr, std::ostream &cout);
void Visit(const koopa_raw_store_t &store, std::ostream &cout);
void Visit(const koopa_raw_branch_t &branch, std::ostream &cout);
void Visit(const koopa_raw_jump_t &jump, std::ostream &cout);
void Visit(const koopa_raw_call_t &call, int dest_addr, std::ostream &cout);
void Visit(const koopa_raw_get_ptr_t &get_ptr, int dest_addr, std::ostream &cout);
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, int dest_addr, std::ostream &cout);
void Visit(const koopa_raw_aggregate_t &aggregate, std::ostream &cout);

void aggregate_init(const koopa_raw_value_t &value, std::ostream &cout);

void global_var(const koopa_raw_value_t &value, std::ostream &cout);

void load(const koopa_raw_value_t &value, std::string r, std::ostream &cout);

int func_size(const koopa_raw_function_t &func, bool &call);
int bb_size(const koopa_raw_basic_block_t &bb, bool &call, int &arg_count);
int inst_size(const koopa_raw_value_t &inst, bool &call, int &arg_count);
int type_size(const koopa_raw_type_t &ty);
int array_size(const koopa_raw_type_t &ty);

void store_to_stack(std::string reg, int addr, std::ostream &cout);
void load_from_stack(std::string reg, int addr, std::ostream &cout);