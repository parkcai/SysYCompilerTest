#pragma once

#include "koopa.h"
#include <string>


void generate_koopa(const char* IR);
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);

//////////////////////
///  instructions  ///
//////////////////////
void Visit(const koopa_raw_return_t &ret);

void Visit(const koopa_raw_integer_t &integer, const koopa_raw_value_t &value); 

void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value);

void Visit(const koopa_raw_store_t &store);

void Visit(const koopa_raw_branch_t &branch);

void Visit(const koopa_raw_jump_t &jump);

void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value);

void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);

void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value);

void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value);


//////////////////////
///   registers   ////
//////////////////////

enum RCONDITION { TEMP, ALLOCATED, GLOBAL, FREE };

struct Register
{
    char name[4];
    int can_evict;
    int condition;
    std::string map;
    Register *next_avilable;
};

void init_registers();
void free_registers(Register *_register);
Register *alloc_registers(const char *map = "\%temp", int condition = TEMP);
void evict_registers();

Register *load2registers(const koopa_raw_value_t &value);


///////////////////////
///   stackframe   ////
///////////////////////

struct StackFrame
{
    std::unordered_map<std::string, int> map;
    int allocated;
    int size;
};

void init_stackframe(int size, int field_offset);
void free_stackframe();

int alloc_stackframe(const char* map, int size = 4);
int get_stack_offset(const std::string &map);
int get_stack_offset(const char *map);

void save_reg2stack(Register *reg, int offset);
void load_stack2reg(Register *reg, int offset, bool load_mem = true);
