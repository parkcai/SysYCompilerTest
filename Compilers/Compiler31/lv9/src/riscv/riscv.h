#ifndef __RISCV_H__
#define __RISCV_H__
#include "koopa.h"
#include "utils.h"
#include <sstream>
#include <string>
#include <map>
using std::stringstream;
using str = std::string;
using std::to_string;
class RISCV_builder {
    int current_size = 0;
    int current_call = 0;
    stringstream ss;
    map<koopa_raw_value_t, int> stack_map;
    map<koopa_raw_value_t, str> reg_map;
    vec<str> register_name = {"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t2", "t3", "t4", "t5" ,"t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
        
    void visit(const koopa_raw_program_t &program);
    void visit(const koopa_raw_slice_t &slice);
    void visit(const koopa_raw_function_t &func);
    void visit(const koopa_raw_basic_block_t &bb);
    void visit(const koopa_raw_value_t &value);
    void visit(const koopa_raw_return_t &ret);
    void visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);
    void visit(const koopa_raw_store_t &store);
    void visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value);
    void visit(const koopa_raw_branch_t &branch);
    void visit(const koopa_raw_jump_t &jump);
    void visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value);
    void visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);
    void visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value);
    void visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value);
    str get_register_name(const koopa_raw_value_t &value);
    int get_stack_idx(const koopa_raw_value_t &value);
    int stack_idx = 0;
    void load_to_register(const koopa_raw_value_t &value, str reg);
    void store_to_stack(str reg, int idx);

    
    int get_func_size(koopa_raw_function_t func, int *p_current_call);
    int get_basic_block_size(int &call, int &maxarg, koopa_raw_basic_block_t bb);
    int get_inst_size(koopa_raw_value_t inst);
    int get_array_size(const koopa_raw_type_kind_t *ty);

public:
    RISCV_builder() {}
    str build(const koopa_raw_program_t &raw);
};

#endif