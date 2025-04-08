#include "koopa.h"
#include "utils.h"
#include <map>
#include <vector>
#pragma once
using namespace std;

// Global Variables
extern int is_last_element;
extern string segmentation;
extern int stack_space_R;
extern int stack_space_S;
extern int stack_space_A;
extern int stack_space_total;
extern int current_bias;
extern map<koopa_raw_value_data_t*, int> bias_from_last_rsp_of;
extern int max_call_inst_len;
extern int debug_shown_in_riscv;
extern string currently_dealt_func_name;
extern int imm12_max;
extern int imm12_min;
extern int forward_step;
extern int OR_HEAD;
extern int AND_HEAD;
extern int instruction_set_type;

// Forward Declaration: Main Functions
void Visit_program(koopa_raw_program_t &program);
void Visit_globals(koopa_raw_slice_t &globals);
void Visit_functions(koopa_raw_slice_t &functions);
void Visit_global(koopa_raw_value_data_t &global);
void Visit_function(koopa_raw_function_data_t &function);
void Visit_block(koopa_raw_basic_block_data_t &block);
void Visit_instruction(koopa_raw_value_data_t &instruction);
void Scan_block(koopa_raw_basic_block_data_t &block);
void Scan_instruction(koopa_raw_value_data_t &instruction);

// Helper Functions


void Visit_program(koopa_raw_program_t &program) {
    is_last_element = !program.funcs.len;
    Visit_globals(program.values);
    is_last_element = 1;
    Visit_functions(program.funcs);
}

void Visit_globals(koopa_raw_slice_t &globals) {
    assert(globals.kind == KOOPA_RSIK_VALUE);
    int backup = is_last_element;
    for (size_t i = 0; i < globals.len; i += 1) {
        auto ptr = const_cast<void*>(globals.buffer[i]);
        is_last_element = !(i != globals.len-1 || !backup);
        Visit_global(*reinterpret_cast<koopa_raw_value_data_t*>(ptr));
    }
    is_last_element = backup;
}

void Visit_functions(koopa_raw_slice_t &functions) {
    assert(functions.kind == KOOPA_RSIK_FUNCTION);
    int backup = is_last_element;
    for (size_t i = 0; i < functions.len; i += 1) {
        auto ptr = const_cast<void*>(functions.buffer[i]);
        is_last_element = !(i != functions.len-1 || !backup);
        Visit_function(*reinterpret_cast<koopa_raw_function_data_t*>(ptr));
    }
    is_last_element = backup;
}

void Visit_global(koopa_raw_value_data_t &global) {
    save_string("  .data\n");
    assert(global.kind.tag == KOOPA_RVT_GLOBAL_ALLOC);
    string global_name = string(global.name).erase(0, 1);
    koopa_raw_value_data_t &global_init = *const_cast<koopa_raw_value_data_t*>(global.kind.data.global_alloc.init);
    if (global_init.ty->tag == KOOPA_RTT_INT32) {
        if (global_init.kind.tag == KOOPA_RVT_INTEGER) {
            save_string("  .global "+global_name+"\n");
            save_string(global_name+":\n");
            save_string("  .word "+to_string(global_init.kind.data.integer.value)+"\n");
        }else if (global_init.kind.tag == KOOPA_RVT_ZERO_INIT) {
            save_string("  .global "+global_name+"\n");
            save_string(global_name+":\n");
            save_string("  .zero 4\n");
        }else{
            int global_int_init_type_exhausted = 0;
            assert(global_int_init_type_exhausted);
        }
    }else if (global_init.ty->tag == KOOPA_RTT_ARRAY) {
        if (global_init.kind.tag == KOOPA_RVT_AGGREGATE) {
            save_string("  .global "+global_name+"\n");
            save_string(global_name+":\n");
            koopa_raw_slice_t &global_initarray = global_init.kind.data.aggregate.elems;
            assert(global_initarray.kind == KOOPA_RSIK_VALUE);
            for (size_t i = 0; i < global_initarray.len; i += 1) {
                koopa_raw_value_data_t &integer_element = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(global_initarray.buffer[i]));
                assert(integer_element.kind.tag == KOOPA_RVT_INTEGER);
                save_string("  .word "+to_string(integer_element.kind.data.integer.value)+"\n");
            }
        }else if (global_init.kind.tag == KOOPA_RVT_ZERO_INIT) {
            save_string("  .global "+global_name+"\n");
            save_string(global_name+":\n");
            save_string("  .zero "+to_string(4 * global_init.ty->data.array.len)+"\n");
        }else{
            int global_array_init_type_exhausted = 0;
            assert(global_array_init_type_exhausted);
        }
    }else{
        int global_type_exhausted = 0;
        assert(global_type_exhausted);
    }
    if (!is_last_element) save_string(segmentation);
}

void Visit_function(koopa_raw_function_data_t &function) {
    // 如果没有基本块，则为函数声明，不用理会
    if (!function.bbs.len) return;
    // Scan函数计算Prologue参数并记录变量在栈上的位置
    stack_space_S = 0;
    stack_space_A = 0;
    current_bias = 8; // current bias为当前所遇变量相对caller rsp的bias
    max_call_inst_len = -1;
    bias_from_last_rsp_of.clear();
    for (size_t i = 0; i < function.bbs.len; i += 1) {
        auto ptr = const_cast<void*>(function.bbs.buffer[i]);
        Scan_block(*reinterpret_cast<koopa_raw_basic_block_data_t*>(ptr));
    }
    if (max_call_inst_len == -1) {
        stack_space_R = 0;
    }else{
        stack_space_R = 4;
    }
    stack_space_A = (max_call_inst_len - 8 > 0 ? max_call_inst_len - 8 : 0) * 4;
    stack_space_total = stack_space_R + stack_space_S + stack_space_A;
    if (stack_space_total % 16) stack_space_total += 16 - (stack_space_total % 16);
    for (auto& pair: bias_from_last_rsp_of) {
        pair.second = stack_space_total - pair.second + (4-stack_space_R);
    }
    for (size_t i = 0; i < function.params.len; i += 1) {
        auto ptr = const_cast<void*>(function.params.buffer[i]);
        bias_from_last_rsp_of[reinterpret_cast<koopa_raw_value_data_t*>(ptr)] = - (int)(i+1);
    }
    string func_name = string(function.name).erase(0, 1);
    currently_dealt_func_name = func_name;
    save_string("  .text\n");
    save_string("  .global "+func_name+"\n");
    save_string(func_name+":\n");
    if (debug_shown_in_riscv) save_string("\n  // function prologue\n");
    if (-stack_space_total >= imm12_min) {
        if (stack_space_total) save_string("  addi sp, sp, "+to_string(-stack_space_total)+"\n");
    }else{
        save_string("  li t3, "+to_string(-stack_space_total)+"\n");
        save_string("  add sp, sp, t3\n");
    }
    if (stack_space_R) {
        if (stack_space_total-4 <= imm12_max) {
            save_string("  sw ra, "+to_string(stack_space_total-4)+"(sp)"+"\n");
        }else{
            save_string("  li t3, "+to_string(stack_space_total-4)+"\n");
            save_string("  add t3, t3, sp\n");
            save_string("  sw ra, 0(t3)\n");
        }
    }
    

    for (size_t i = 0; i < function.bbs.len; i += 1) {
        auto ptr = const_cast<void*>(function.bbs.buffer[i]);
        Visit_block(*reinterpret_cast<koopa_raw_basic_block_data_t*>(ptr));
    }
    if (!is_last_element) save_string(segmentation);
}

void Scan_block(koopa_raw_basic_block_data_t &block){
    for (size_t i = 0; i < block.insts.len; i += 1) {
        i += forward_step;
        forward_step = 0;
        auto ptr = const_cast<void*>(block.insts.buffer[i]);
        Scan_instruction(*reinterpret_cast<koopa_raw_value_data_t*>(ptr));
    }
}

void Scan_instruction(koopa_raw_value_data_t &instruction) {
    if (instruction.kind.tag == KOOPA_RVT_INTEGER) {
        int none_integer_instruction = 0;
        assert(none_integer_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_ZERO_INIT) {
        int none_zeroinit_instruction = 0;
        assert(none_zeroinit_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_UNDEF) {
        int none_undef_instruction = 0;
        assert(none_undef_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_AGGREGATE) {
        int none_aggregate_instruction = 0;
        assert(none_aggregate_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        int none_func_arg_ref_instruction = 0;
        assert(none_func_arg_ref_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_BLOCK_ARG_REF) {
        int none_block_arg_ref_instruction = 0;
        assert(none_block_arg_ref_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_ALLOC) {
        if (instruction.ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
            stack_space_S += 4;
            bias_from_last_rsp_of[&instruction] = current_bias;
            current_bias += 4;
        }else if (instruction.ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
            int array_len = instruction.ty->data.pointer.base->data.array.len;
            stack_space_S += 4 * array_len;
            current_bias += 4 * (array_len - 1);
            bias_from_last_rsp_of[&instruction] = current_bias;
            current_bias += 4;
        }else if (instruction.ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
            stack_space_S += 4;
            bias_from_last_rsp_of[&instruction] = current_bias;
            current_bias += 4;
        }else{
            int alloc_instruction_type_exhausted = 0;
            assert(alloc_instruction_type_exhausted);
        }
    }else if (instruction.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        int none_global_alloc_instruction = 0;
        assert(none_global_alloc_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_LOAD) {
        stack_space_S += 4;
        bias_from_last_rsp_of[&instruction] = current_bias;
        current_bias += 4;
    }else if (instruction.kind.tag == KOOPA_RVT_STORE) {

    }else if (instruction.kind.tag == KOOPA_RVT_GET_PTR) {
        stack_space_S += 4;
        bias_from_last_rsp_of[&instruction] = current_bias;
        current_bias += 4;
    }else if (instruction.kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
        stack_space_S += 4;
        bias_from_last_rsp_of[&instruction] = current_bias;
        current_bias += 4;
    }else if (instruction.kind.tag == KOOPA_RVT_BINARY) {
        if (instruction.kind.data.binary.op == KOOPA_RBO_SHR) {
            if (instruction.kind.data.binary.rhs->kind.data.integer.value == OR_HEAD) {
                forward_step = 1;
                return;
            }else if(instruction.kind.data.binary.rhs->kind.data.integer.value == AND_HEAD) {
                forward_step = 2;
                return;
            }else{
                int shr_safe_and_enchanter_exhausted = 0;
                assert(shr_safe_and_enchanter_exhausted);
            }
        }else{
            stack_space_S += 4;
            bias_from_last_rsp_of[&instruction] = current_bias;
            current_bias += 4;
        }
    }else if (instruction.kind.tag == KOOPA_RVT_BRANCH) {

    }else if (instruction.kind.tag == KOOPA_RVT_JUMP) {

    }else if (instruction.kind.tag == KOOPA_RVT_CALL) {
        if (instruction.ty->tag != KOOPA_RTT_UNIT) {
            stack_space_S += 4;
            bias_from_last_rsp_of[&instruction] = current_bias;
            current_bias += 4;
        }
        int current_call_args_len = instruction.kind.data.call.args.len;
        if (current_call_args_len > max_call_inst_len) max_call_inst_len = current_call_args_len;
    }else if (instruction.kind.tag == KOOPA_RVT_RETURN) {

    }else{
        int instruction_type_exhausted = 0;
        assert(instruction_type_exhausted);
    }
}

void Visit_block(koopa_raw_basic_block_data_t &block){
    string block_name = string(block.name).erase(0, 1);
    if (block_name != "entry_label") save_string(block_name+":\n");
    for (size_t i = 0; i < block.insts.len; i += 1) {
        auto ptr = const_cast<void*>(block.insts.buffer[i]);
        Visit_instruction(*reinterpret_cast<koopa_raw_value_data_t*>(ptr));
        if (instruction_set_type == OR_HEAD) {
            if (debug_shown_in_riscv) save_string("\n  //atomic instruction set: or\n");
            koopa_raw_value_data_t& instruction1 = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(block.insts.buffer[i+1]));
            koopa_raw_value_data_t& instruction2 = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(block.insts.buffer[i+2]));
            koopa_raw_value_data_t& lhs1 = *const_cast<koopa_raw_value_data_t*>(instruction1.kind.data.binary.lhs);
            koopa_raw_value_data_t& rhs1 = *const_cast<koopa_raw_value_data_t*>(instruction1.kind.data.binary.rhs);
            if (bias_from_last_rsp_of[&lhs1] <= imm12_max) {
                save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&lhs1])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&lhs1])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
            if (bias_from_last_rsp_of[&rhs1] <= imm12_max) {
                save_string("  lw t1, "+to_string(bias_from_last_rsp_of[&rhs1])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&rhs1])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t1, 0(t3)\n");
            }
            save_string("  or t0, t0, t1\n");
            save_string("  snez t0, t0\n");
            if (bias_from_last_rsp_of[&instruction2] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction2])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction2])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
            i += 2;
            instruction_set_type = 0;
        }else if (instruction_set_type == AND_HEAD) {
            if (debug_shown_in_riscv) save_string("\n  //atomic instruction set: and\n");
            koopa_raw_value_data_t& instruction1 = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(block.insts.buffer[i+1]));
            koopa_raw_value_data_t& instruction2 = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(block.insts.buffer[i+2]));
            koopa_raw_value_data_t& instruction3 = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(block.insts.buffer[i+3]));
            koopa_raw_value_data_t& rhs1 = *const_cast<koopa_raw_value_data_t*>(instruction1.kind.data.binary.rhs);
            koopa_raw_value_data_t& rhs2 = *const_cast<koopa_raw_value_data_t*>(instruction2.kind.data.binary.rhs);
            if (bias_from_last_rsp_of[&rhs1] <= imm12_max) {
                save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&rhs1])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&rhs1])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
            if (bias_from_last_rsp_of[&rhs2] <= imm12_max) {
                save_string("  lw t1, "+to_string(bias_from_last_rsp_of[&rhs2])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&rhs2])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t1, 0(t3)\n");
            }
            save_string("  snez t0, t0\n");
            save_string("  snez t1, t1\n");
            save_string("  and t0, t0, t1\n");
            if (bias_from_last_rsp_of[&instruction3] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction3])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction3])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
            i += 3;
            instruction_set_type = 0;
        }else{
            int i_am_logic = !instruction_set_type;
            assert(i_am_logic);
        }
    }
}

void Visit_instruction(koopa_raw_value_data_t &instruction) {
    if (instruction.kind.tag == KOOPA_RVT_BINARY) {
        if (instruction.kind.data.binary.op == KOOPA_RBO_SHR) {
            if (instruction.kind.data.binary.rhs->kind.data.integer.value == OR_HEAD) {
                instruction_set_type = OR_HEAD;
                return;
            }else if(instruction.kind.data.binary.rhs->kind.data.integer.value == AND_HEAD) {
                instruction_set_type = AND_HEAD;
                return;
            }else{
                int i_am_logic = 0;
                assert(i_am_logic);
            }
        }
    }
    if (instruction.kind.tag == KOOPA_RVT_INTEGER) {
        int none_integer_instruction = 0;
        assert(none_integer_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_ZERO_INIT) {
        int none_zeroinit_instruction = 0;
        assert(none_zeroinit_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_UNDEF) {
        int none_undef_instruction = 0;
        assert(none_undef_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_AGGREGATE) {
        int none_aggregate_instruction = 0;
        assert(none_aggregate_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        int none_func_arg_ref_instruction = 0;
        assert(none_func_arg_ref_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_BLOCK_ARG_REF) {
        int none_block_arg_ref_instruction = 0;
        assert(none_block_arg_ref_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_ALLOC) {
    
    }else if (instruction.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        int none_global_alloc_instruction = 0;
        assert(none_global_alloc_instruction);
    }else if (instruction.kind.tag == KOOPA_RVT_LOAD) {
        koopa_raw_value_data_t& source = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.load.src);
        if (source.kind.tag == KOOPA_RVT_GET_PTR || source.kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
            if (debug_shown_in_riscv) save_string("\n  //load instruction: load from pointed position\n");
            if (bias_from_last_rsp_of[&source] <= imm12_max) {
                save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&source])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&source])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
            save_string("  lw t0, 0(t0)\n");
            if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
        }else{
            if (source.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                if (debug_shown_in_riscv) save_string("\n  //load instruction: load from global variable\n");
                string global_name = string(source.name).erase(0,1);
                save_string("  la t0, "+global_name+"\n");
                save_string("  lw t0, 0(t0)\n");
                if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                    save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  sw t0, 0(t3)\n");
                }
            }else{
                if (debug_shown_in_riscv) save_string("\n  //load instruction: load from normal variable\n");
                if (bias_from_last_rsp_of[&source] <= imm12_max) {
                    save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&source])+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&source])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  lw t0, 0(t3)\n");
                }
                if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                    save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  sw t0, 0(t3)\n");
                }
            } 
        }
    }else if (instruction.kind.tag == KOOPA_RVT_STORE) {
        // 只有store指令会遇见函数参数
        koopa_raw_value_data_t& source = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.store.value);
        koopa_raw_value_data_t& dest = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.store.dest);
        if (dest.kind.tag == KOOPA_RVT_GET_PTR || dest.kind.tag == KOOPA_RVT_GET_ELEM_PTR ) {
            if (source.kind.tag == KOOPA_RVT_INTEGER) {
                if (debug_shown_in_riscv) save_string("\n  //store instruction: store integer to pointed position\n");
                int dest_bias = bias_from_last_rsp_of[&dest];
                save_string("  li t0, "+to_string(source.kind.data.integer.value)+"\n");
                if (dest_bias <= imm12_max) {
                    save_string("  lw t1, "+to_string(dest_bias)+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(dest_bias)+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  lw t1, 0(t3)\n");
                }
                save_string("  sw t0, 0(t1)\n");
            }else{
                if (debug_shown_in_riscv) save_string("\n  //store instruction: store normal variable to pointed position\n");
                int source_bias = bias_from_last_rsp_of[&source];
                int dest_bias = bias_from_last_rsp_of[&dest];
                assert(source_bias >= 0);
                if (source_bias <= imm12_max) {
                    save_string("  lw t0, "+to_string(source_bias)+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(source_bias)+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  lw t0, 0(t3)\n");
                }
                if (dest_bias <= imm12_max) {
                    save_string("  lw t1, "+to_string(dest_bias)+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(dest_bias)+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  lw t1, 0(t3)\n");
                }
                save_string("  sw t0, 0(t1)\n");
            }
        }else{
            if (source.kind.tag == KOOPA_RVT_INTEGER) {
                if (dest.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                    if (debug_shown_in_riscv) save_string("\n  //store instruction: store integer to global variable\n");
                    save_string("  li t0, "+to_string(source.kind.data.integer.value)+"\n");
                    string global_name = string(dest.name).erase(0,1);
                    save_string("  la t3, "+global_name+"\n");
                    save_string("  sw t0, 0(t3)\n");
                }else{
                    if (debug_shown_in_riscv) save_string("\n  //store instruction: store integer to normal variable\n");
                    int dest_bias = bias_from_last_rsp_of[&dest];
                    save_string("  li t0, "+to_string(source.kind.data.integer.value)+"\n");
                    if (dest_bias <= imm12_max) {
                        save_string("  sw t0, "+to_string(dest_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(dest_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw t0, 0(t3)\n");
                    }
                }
            }else{
                int source_bias = bias_from_last_rsp_of[&source];
                int dest_bias = bias_from_last_rsp_of[&dest];
                if (source_bias >= 0) {
                    if (dest.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                        if (debug_shown_in_riscv) save_string("\n  //store instruction: store normal variable to global variable\n");
                        if (source_bias <= imm12_max) {
                            save_string("  lw t0, "+to_string(source_bias)+"(sp)\n");
                        }else{
                            save_string("  li t3, "+to_string(source_bias)+"\n");
                            save_string("  add t3, t3, sp\n");
                            save_string("  lw t0, 0(t3)\n");
                        }
                        string global_name = string(dest.name).erase(0,1);
                        save_string("  la t3, "+global_name+"\n");
                        save_string("  sw t0, 0(t3)\n");
                    }else{
                        if (debug_shown_in_riscv) save_string("\n  //store instruction: store normal variable to normal variable\n");
                        if (source_bias <= imm12_max) {
                            save_string("  lw t0, "+to_string(source_bias)+"(sp)\n");
                        }else{
                            save_string("  li t3, "+to_string(source_bias)+"\n");
                            save_string("  add t3, t3, sp\n");
                            save_string("  lw t0, 0(t3)\n");
                        }
                        if (dest_bias <= imm12_max) {
                            save_string("  sw t0, "+to_string(dest_bias)+"(sp)\n");
                        }else{
                            save_string("  li t3, "+to_string(dest_bias)+"\n");
                            save_string("  add t3, t3, sp\n");
                            save_string("  sw t0, 0(t3)\n");
                        }
                    }
                }else if (source_bias >= -8) {
                    if (debug_shown_in_riscv) save_string("\n  //store instruction: store func param 1~8 to normal variable\n");
                    if (dest_bias <= imm12_max) {
                        save_string("  sw a"+ to_string(-source_bias-1)+", "+to_string(dest_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(dest_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw a"+ to_string(-source_bias-1)+", 0(t3)\n");
                    }
                }else{
                    if (debug_shown_in_riscv) save_string("\n  //store instruction: store func param >= 9 to normal variable\n");
                    if ((-source_bias-9)*4+stack_space_total <= imm12_max) {
                        save_string("  lw t0, "+to_string((-source_bias-9)*4+stack_space_total)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string((-source_bias-9)*4+stack_space_total)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  lw t0, 0(t3)\n");
                    }
                    if (dest_bias <= imm12_max) {
                        save_string("  sw t0, "+to_string(dest_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(dest_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw t0, 0(t3)\n");
                    }
                }
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_GET_PTR) {
        koopa_raw_value_data_t& array_base = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.get_ptr.src);
        koopa_raw_value_data_t& index = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.get_ptr.index);
        if (index.kind.tag == KOOPA_RVT_INTEGER) {
            if (debug_shown_in_riscv) save_string("\n  //getptr instruction: integer index of pointed array\n");
            if (bias_from_last_rsp_of[&array_base] <= imm12_max) {
                save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&array_base])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&array_base])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
            if (index.kind.data.integer.value*4 <= imm12_max) {
                save_string("  addi t0, t0, "+to_string(index.kind.data.integer.value*4)+"\n");
            }else{
                save_string("  li t3, "+to_string(index.kind.data.integer.value*4)+"\n");
                save_string("  add t0, t0, t3\n");
            }
            if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
        }else{
            if (debug_shown_in_riscv) save_string("\n  //getptr instruction: variable index of pointed array\n");
            if (bias_from_last_rsp_of[&array_base] <= imm12_max) {
                save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&array_base])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&array_base])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
            if (bias_from_last_rsp_of[&index] <= imm12_max) {
                save_string("  lw t1, "+to_string(bias_from_last_rsp_of[&index])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&index])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t1, 0(t3)\n");
            }
            save_string("  li t3, 2\n");
            save_string("  sll t1, t1, t3\n");
            save_string("  add t0, t0, t1\n");
            if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
        koopa_raw_value_data_t& array_base = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.get_elem_ptr.src);
        koopa_raw_value_data_t& index = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.get_elem_ptr.index);
        if (index.kind.tag == KOOPA_RVT_INTEGER) {
            if (bias_from_last_rsp_of.find(&array_base) != bias_from_last_rsp_of.end()) {
                if (debug_shown_in_riscv) save_string("\n  //getelemptr instruction: integer index of local array\n");
                if (bias_from_last_rsp_of[&array_base] <= imm12_max) {
                    save_string("  addi t0, sp, "+to_string(bias_from_last_rsp_of[&array_base])+"\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&array_base])+"\n");
                    save_string("  add t0, sp, t3\n");
                }
            }else{
                if (debug_shown_in_riscv) save_string("\n  //getelemptr instruction: integer index of global array\n");
                save_string("  la t0, "+string(array_base.name).erase(0,1)+"\n");
            }
            if (index.kind.data.integer.value*4 <= imm12_max) {
                save_string("  addi t0, t0, "+to_string(index.kind.data.integer.value*4)+"\n");
            }else{
                save_string("  li t3, "+to_string(index.kind.data.integer.value*4)+"\n");
                save_string("  add t0, t0, t3\n");
            }
            if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
        }else{
            if (bias_from_last_rsp_of.find(&array_base) != bias_from_last_rsp_of.end()) {
                if (debug_shown_in_riscv) save_string("\n  //getelemptr instruction: variable index of local array\n");
                if (bias_from_last_rsp_of[&array_base] <= imm12_max) {
                    save_string("  addi t0, sp, "+to_string(bias_from_last_rsp_of[&array_base])+"\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&array_base])+"\n");
                    save_string("  add t0, sp, t3\n");
                }
            }else{
                if (debug_shown_in_riscv) save_string("\n  //getelemptr instruction: variable index of global array\n");
                save_string("  la t0, "+string(array_base.name).erase(0,1)+"\n");
            }
            if (bias_from_last_rsp_of[&index] <= imm12_max) {
                save_string("  lw t1, "+to_string(bias_from_last_rsp_of[&index])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&index])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t1, 0(t3)\n");
            }
            save_string("  li t3, 2\n");
            save_string("  sll t1, t1, t3\n");
            save_string("  add t0, t0, t1\n");
            if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                save_string("  sw t0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw t0, 0(t3)\n");
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_BINARY) {
        switch (instruction.kind.data.binary.op) {
            case KOOPA_RBO_NOT_EQ:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: not equal\n");
                break;
            case KOOPA_RBO_EQ:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: equal\n");
                break;
            case KOOPA_RBO_GE:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: greater equal\n");
                break;
            case KOOPA_RBO_LE:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: less equal\n");
                break;
            case KOOPA_RBO_GT:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: greater than\n");
                break;
            case KOOPA_RBO_LT:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: less than\n");
                break;
            case KOOPA_RBO_ADD:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: add\n");
                break;
            case KOOPA_RBO_SUB:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: sub\n");
                break;
            case KOOPA_RBO_MUL:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: mul\n");
                break;
            case KOOPA_RBO_DIV:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: div\n");
                break;
            case KOOPA_RBO_MOD:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: mod\n");
                break;
            case KOOPA_RBO_AND:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: and\n");
                break;
            case KOOPA_RBO_OR:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: or\n");
                break;
            case KOOPA_RBO_XOR:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: xor\n");
                break;
            case KOOPA_RBO_SHL:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: shift left\n");
                break;
            case KOOPA_RBO_SHR:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: shift right\n");
                break;
            case KOOPA_RBO_SAR:
                if (debug_shown_in_riscv) save_string("\n  //binary instruction: shift right arithmetically\n");
                break;
            default:
                assert(false);
        }
        koopa_raw_value_data_t& lhs = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.binary.lhs);
        if (lhs.kind.tag == KOOPA_RVT_INTEGER) {
            save_string("  li t0, " + to_string(lhs.kind.data.integer.value) + "\n");
        } else {
            int lhs_bias = bias_from_last_rsp_of[&lhs];
            if (lhs_bias <= imm12_max) {
                save_string("  lw t0, " + to_string(lhs_bias) + "(sp)\n");
            }else{
                save_string("  li t3, "+to_string(lhs_bias)+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
        }
        koopa_raw_value_data_t& rhs = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.binary.rhs);
        if (rhs.kind.tag == KOOPA_RVT_INTEGER) {
            save_string("  li t1, " + to_string(rhs.kind.data.integer.value) + "\n");
        } else {
            int rhs_bias = bias_from_last_rsp_of[&rhs];
            if (rhs_bias <= imm12_max) {
                save_string("  lw t1, " + to_string(rhs_bias) + "(sp)\n");
            }else{
                save_string("  li t3, "+to_string(rhs_bias)+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t1, 0(t3)\n");
            }
        }
        string binary_op;
        switch (instruction.kind.data.binary.op) {
            case KOOPA_RBO_NOT_EQ:
                save_string("  sub t1, t0, t1\n");
                save_string("  snez t0, t1\n");
                if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                    save_string("  sw t0, " + to_string(bias_from_last_rsp_of[&instruction]) + "(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  sw t0, 0(t3)\n");
                }
                return;
            case KOOPA_RBO_EQ:
                save_string("  sub t1, t0, t1\n");
                save_string("  seqz t0, t1\n");
                if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                    save_string("  sw t0, " + to_string(bias_from_last_rsp_of[&instruction]) + "(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  sw t0, 0(t3)\n");
                }
                return;
            // 对">="和"<="这二元运算，需要额外引入一个中间变量
            case KOOPA_RBO_GE:
                save_string("  sgt t2, t0, t1\n");
                save_string("  sub t1, t0, t1\n");
                save_string("  seqz t0, t1\n");
                save_string("  or t0, t0, t2\n");
                if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                    save_string("  sw t0, " + to_string(bias_from_last_rsp_of[&instruction]) + "(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  sw t0, 0(t3)\n");
                }
                return;
            case KOOPA_RBO_LE:
                save_string("  slt t2, t0, t1\n");
                save_string("  sub t1, t0, t1\n");
                save_string("  seqz t0, t1\n");
                save_string("  or t0, t0, t2\n");
                if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                    save_string("  sw t0, " + to_string(bias_from_last_rsp_of[&instruction]) + "(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  sw t0, 0(t3)\n");
                }
                return;
            case KOOPA_RBO_GT:
                binary_op = "sgt";
                break;
            case KOOPA_RBO_LT:
                binary_op = "slt";
                break;
            case KOOPA_RBO_ADD:
                binary_op = "add";
                break;
            case KOOPA_RBO_SUB:
                binary_op = "sub";
                break;
            case KOOPA_RBO_MUL:
                binary_op = "mul";
                break;
            case KOOPA_RBO_DIV:
                binary_op = "div";
                break;
            case KOOPA_RBO_MOD:
                binary_op = "rem";
                break;
            case KOOPA_RBO_AND:
                binary_op = "and";
                break;
            case KOOPA_RBO_OR:
                binary_op = "or";
                break;
            case KOOPA_RBO_XOR:
                binary_op = "xor";
                break;
            case KOOPA_RBO_SHL:
                binary_op = "sll";
                break;
            case KOOPA_RBO_SHR:
                binary_op = "srl";
                break;
            case KOOPA_RBO_SAR:
                binary_op = "sra";
                break;
            default:
                assert(false);
        }
        save_string("  " + binary_op + " t0, t0, t1\n");
        if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
            save_string("  sw t0, " + to_string(bias_from_last_rsp_of[&instruction]) + "(sp)\n");
        }else{
            save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
            save_string("  add t3, t3, sp\n");
            save_string("  sw t0, 0(t3)\n");
        }
        return;
    }else if (instruction.kind.tag == KOOPA_RVT_BRANCH) {
        koopa_raw_value_data_t& condition = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.branch.cond);
        koopa_raw_basic_block_t& true_block = instruction.kind.data.branch.true_bb;
        koopa_raw_basic_block_t& false_block = instruction.kind.data.branch.false_bb;
        string true_block_name = string(true_block->name).erase(0, 1);
        if (true_block_name == "entry_label") true_block_name = currently_dealt_func_name;
        string false_block_name = string(false_block->name).erase(0, 1);
        if (false_block_name == "entry_label") false_block_name = currently_dealt_func_name;
        if (condition.kind.tag == KOOPA_RVT_INTEGER) {
            if (condition.kind.data.integer.value) {
                if (debug_shown_in_riscv) save_string("\n  //branch instruction: integer condition, jump for sure\n");
                save_string("  j "+true_block_name+"\n");
            }else{
                if (debug_shown_in_riscv) save_string("\n  //branch instruction: integer condition, not jump for sure\n");
                save_string("  j "+false_block_name+"\n");
            }
        }else{
            if (debug_shown_in_riscv) save_string("\n  //branch instruction: variable condition\n");
            if (bias_from_last_rsp_of[&condition] <= imm12_max) {
                save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&condition])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&condition])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw t0, 0(t3)\n");
            }
            save_string("  bnez t0, "+true_block_name+"_near\n");
            save_string("  j "+false_block_name+"\n");
            save_string(true_block_name+"_near:\n");
            if(debug_shown_in_riscv) save_string("\n  // Nearby label set up because bnez instruction cannot jump far\n");
            save_string("  j "+true_block_name+"\n");
        }
    }else if (instruction.kind.tag == KOOPA_RVT_JUMP) {
        if (debug_shown_in_riscv) save_string("\n  //jump instruction\n");
        string target_block_name = string(instruction.kind.data.jump.target->name).erase(0, 1);
        if (target_block_name == "entry_label") target_block_name = currently_dealt_func_name;
        save_string("  j "+target_block_name+"\n");
    }else if (instruction.kind.tag == KOOPA_RVT_CALL) {
        string callee_name = string(instruction.kind.data.call.callee->name).erase(0,1);
        koopa_raw_slice_t& args = instruction.kind.data.call.args;
        if (instruction.ty->tag == KOOPA_RTT_UNIT) {
            if (debug_shown_in_riscv) save_string("\n  //call instruction: no return value\n");
            for (size_t i = 0; i < ((args.len < 8) ? args.len : 8); i += 1) {
                koopa_raw_value_data_t& arg = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(args.buffer[i]));
                if (arg.kind.tag == KOOPA_RVT_INTEGER) {
                    save_string("  li a"+to_string(i)+", "+to_string(arg.kind.data.integer.value)+"\n");
                }else{
                    if (bias_from_last_rsp_of[&arg] <= imm12_max) {
                        save_string("  lw a"+to_string(i)+", "+to_string(bias_from_last_rsp_of[&arg])+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(bias_from_last_rsp_of[&arg])+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  lw a"+to_string(i)+", 0(t3)\n");
                    }      
                }
            }
            int current_spilt_param_bias = 0;
            for (size_t i = 8; i < args.len; i += 1) {
                koopa_raw_value_data_t& arg = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(args.buffer[i]));
                if (arg.kind.tag == KOOPA_RVT_INTEGER) {
                    save_string("  li t0, "+to_string(arg.kind.data.integer.value)+"\n");
                    if (current_spilt_param_bias <= imm12_max) {
                        save_string("  sw t0, "+to_string(current_spilt_param_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(current_spilt_param_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw t0, 0(t3)\n");
                    }
                    current_spilt_param_bias += 4;
                }else{
                    if (bias_from_last_rsp_of[&arg] <= imm12_max) {
                        save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&arg])+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(bias_from_last_rsp_of[&arg])+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  lw t0, 0(t3)\n");
                    }
                    if (current_spilt_param_bias <= imm12_max) {
                        save_string("  sw t0, "+to_string(current_spilt_param_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(current_spilt_param_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw t0, 0(t3)\n");
                    }
                    current_spilt_param_bias += 4;
                }
            }
            save_string("  call "+callee_name+"\n");
        }else{
            if (debug_shown_in_riscv) save_string("\n  //call instruction: has return value\n");
            for (size_t i = 0; i < ((args.len < 8) ? args.len : 8); i += 1) {
                koopa_raw_value_data_t& arg = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(args.buffer[i]));
                if (arg.kind.tag == KOOPA_RVT_INTEGER) {
                    save_string("  li a"+to_string(i)+", "+to_string(arg.kind.data.integer.value)+"\n");
                }else{
                    if (bias_from_last_rsp_of[&arg] <= imm12_max) {
                        save_string("  lw a"+to_string(i)+", "+to_string(bias_from_last_rsp_of[&arg])+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(bias_from_last_rsp_of[&arg])+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  lw a"+to_string(i)+", 0(t3)\n");
                    }      
                }
            }
            int current_spilt_param_bias = 0;
            for (size_t i = 8; i < args.len; i += 1) {
                koopa_raw_value_data_t& arg = *reinterpret_cast<koopa_raw_value_data_t*>(const_cast<void*>(args.buffer[i]));
                if (arg.kind.tag == KOOPA_RVT_INTEGER) {
                    save_string("  li t0, "+to_string(arg.kind.data.integer.value)+"\n");
                    if (current_spilt_param_bias <= imm12_max) {
                        save_string("  sw t0, "+to_string(current_spilt_param_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(current_spilt_param_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw t0, 0(t3)\n");
                    }
                    current_spilt_param_bias += 4;
                }else{
                    if (bias_from_last_rsp_of[&arg] <= imm12_max) {
                        save_string("  lw t0, "+to_string(bias_from_last_rsp_of[&arg])+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(bias_from_last_rsp_of[&arg])+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  lw t0, 0(t3)\n");
                    }
                    if (current_spilt_param_bias <= imm12_max) {
                        save_string("  sw t0, "+to_string(current_spilt_param_bias)+"(sp)\n");
                    }else{
                        save_string("  li t3, "+to_string(current_spilt_param_bias)+"\n");
                        save_string("  add t3, t3, sp\n");
                        save_string("  sw t0, 0(t3)\n");
                    }
                    current_spilt_param_bias += 4;
                }
            }
            save_string("  call "+callee_name+"\n");
            if (bias_from_last_rsp_of[&instruction] <= imm12_max) {
                save_string("  sw a0, "+to_string(bias_from_last_rsp_of[&instruction])+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(bias_from_last_rsp_of[&instruction])+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  sw a0, 0(t3)\n");
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_RETURN) {
        if (instruction.kind.data.ret.value != NULL) {
            if (instruction.kind.data.ret.value->kind.tag == KOOPA_RVT_INTEGER) {
                if (debug_shown_in_riscv) save_string("\n  //return instruction: return integer\n");
                save_string("  li a0, "+to_string(instruction.kind.data.ret.value->kind.data.integer.value)+"\n");
            }else{
                if (debug_shown_in_riscv) save_string("\n  //return instruction: return variable\n");
                int res_bias = bias_from_last_rsp_of[const_cast<koopa_raw_value_data_t*>(instruction.kind.data.ret.value)];
                if (res_bias <= imm12_max) {
                    save_string("  lw a0, "+to_string(res_bias)+"(sp)\n");
                }else{
                    save_string("  li t3, "+to_string(res_bias)+"\n");
                    save_string("  add t3, t3, sp\n");
                    save_string("  lw a0, 0(t3)\n");
                }
            }
        }else{
            if (debug_shown_in_riscv) save_string("\n  //return instruction: no return value\n");
        }
        if (stack_space_R) {
            if (stack_space_total-4 <= imm12_max) {
                save_string("  lw ra, "+to_string(stack_space_total-4)+"(sp)\n");
            }else{
                save_string("  li t3, "+to_string(stack_space_total-4)+"\n");
                save_string("  add t3, t3, sp\n");
                save_string("  lw ra, 0(t3)\n");
            }
        }
        if (stack_space_total <= imm12_max) {
            if(stack_space_total) save_string("  addi sp, sp, "+to_string(stack_space_total)+"\n");
        }else{
            save_string("  li t3, "+to_string(stack_space_total)+"\n");
            save_string("  add sp, sp, t3\n");
        }
        save_string("  ret\n");
    }else{
        int instruction_type_exhausted = 0;
        assert(instruction_type_exhausted);
    }
}