#include "koopa.h"
#include "utils.h"
#include <map>
#include <vector>
#pragma once
using namespace std;

extern map<string, koopa_raw_value_data_t*> loaded_koopa_register;

void Optimize_program(koopa_raw_program_t &program);
void Optimize_functions(koopa_raw_slice_t &functions);
void Optimize_function(koopa_raw_function_data_t &function);
void Optimize_block(koopa_raw_basic_block_data_t &block);
void Optimize_instruction(koopa_raw_value_data_t &instruction);
void Transfer_reference(koopa_raw_value_data_t &user_instruction, koopa_raw_value_data_t &desserted_instruction, koopa_raw_value_data_t &new_instruction);

void Optimize_program(koopa_raw_program_t &program) {
    Optimize_functions(program.funcs);
}

void Optimize_functions(koopa_raw_slice_t &functions) {
    assert(functions.kind == KOOPA_RSIK_FUNCTION);
    for (size_t i = 0; i < functions.len; i += 1) {
        auto ptr = const_cast<void*>(functions.buffer[i]);
        Optimize_function(*reinterpret_cast<koopa_raw_function_data_t*>(ptr));
    }
}

void Optimize_function(koopa_raw_function_data_t &function) {
    for (size_t i = 0; i < function.bbs.len; i += 1) {
        auto ptr = const_cast<void*>(function.bbs.buffer[i]);
        Optimize_block(*reinterpret_cast<koopa_raw_basic_block_data_t*>(ptr));
    }
}

void Optimize_block(koopa_raw_basic_block_data_t &block) {
    loaded_koopa_register.clear();
    for (size_t i = 0; i < block.insts.len; i += 1) {
        auto ptr = const_cast<void*>(block.insts.buffer[i]);
        Optimize_instruction(*reinterpret_cast<koopa_raw_value_data_t*>(ptr));
    }
    for (size_t i = 0; i < block.insts.len; i += 1) {
        while (i < block.insts.len) {
            auto ptr = const_cast<void*>(block.insts.buffer[i]);
            koopa_raw_value_data_t &instruction = *reinterpret_cast<koopa_raw_value_data_t*>(ptr);
            if (instruction.kind.tag == KOOPA_RVT_LOAD) {
                if (instruction.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || instruction.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                    if (instruction.used_by.len) {
                        break;
                    }else{
                        for (size_t j = i+1; j < block.insts.len; j += 1) {
                            block.insts.buffer[j-1] = block.insts.buffer[j];
                        }
                        block.insts.len -= 1;
                    }
                }else{
                    break;
                }
            }else{
                break;
            }
        }
    }
}

void Optimize_instruction(koopa_raw_value_data_t &instruction) {
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
        
    }else if (instruction.kind.tag == KOOPA_RVT_LOAD) {
        koopa_raw_value_data_t& source = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.load.src);
        if (source.kind.tag == KOOPA_RVT_GET_PTR) {

        }else if (source.kind.tag == KOOPA_RVT_GET_ELEM_PTR) {

        }else if (source.kind.tag == KOOPA_RVT_ALLOC) {
            string variable_name(source.name);
            if (loaded_koopa_register.find(variable_name) == loaded_koopa_register.end()) {
                loaded_koopa_register[variable_name] = &instruction;
            }
        }else if (source.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            string variable_name(source.name);
            if (loaded_koopa_register.find(variable_name) == loaded_koopa_register.end()) {
                loaded_koopa_register[variable_name] = &instruction;
            }
        }else if (source.kind.tag == KOOPA_RVT_BINARY) {

        }else{
            int load_instruction_source_type_exhausted = 0;
            assert(load_instruction_source_type_exhausted);
        }
    }else if (instruction.kind.tag == KOOPA_RVT_STORE) {
        koopa_raw_value_data_t& source = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.store.value);
        if (source.kind.tag == KOOPA_RVT_LOAD) {
            if (source.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || source.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                string variable_name(source.kind.data.load.src->name);
                if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                    if (loaded_koopa_register[variable_name] != &instruction) {
                        koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                        instruction.kind.data.store.value = available_old_instruction;
                        Transfer_reference(instruction, source, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                    }
                }
            }
        }
        koopa_raw_value_data_t& dest = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.store.dest);
        if (dest.kind.tag == KOOPA_RVT_GET_PTR) {

        }else if (dest.kind.tag == KOOPA_RVT_GET_ELEM_PTR) {

        }else if (dest.kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            string variable_name(dest.name);
            if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                loaded_koopa_register.erase(loaded_koopa_register.find(variable_name));
            }
        }else if (dest.kind.tag == KOOPA_RVT_ALLOC) {
            string variable_name(dest.name);
            if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                loaded_koopa_register.erase(loaded_koopa_register.find(variable_name));
            }
        }else{
            int store_instruction_dest_type_exhausted = 0;
            assert(store_instruction_dest_type_exhausted);
        }

    }else if (instruction.kind.tag == KOOPA_RVT_GET_PTR) {
        koopa_raw_value_data_t& index = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.get_ptr.index);
        if (index.kind.tag == KOOPA_RVT_LOAD) {
            if (index.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || index.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                string variable_name(index.kind.data.load.src->name);
                if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                    if (loaded_koopa_register[variable_name] != &instruction) {
                        koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                        instruction.kind.data.get_ptr.index = available_old_instruction;
                        Transfer_reference(instruction, index, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                    }
                }
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
        koopa_raw_value_data_t& index = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.get_elem_ptr.index);
        if (index.kind.tag == KOOPA_RVT_LOAD) {
            if (index.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || index.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                string variable_name(index.kind.data.load.src->name);
                if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                    if (loaded_koopa_register[variable_name] != &instruction) {
                        koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                        instruction.kind.data.get_elem_ptr.index = available_old_instruction;
                        Transfer_reference(instruction, index, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                    }
                }
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_BINARY) {
        koopa_raw_value_data_t& lhs = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.binary.lhs);
        if (lhs.kind.tag == KOOPA_RVT_LOAD) {
            if (lhs.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || lhs.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                string variable_name(lhs.kind.data.load.src->name);
                if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                    if (loaded_koopa_register[variable_name] != &instruction) {
                        koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                        instruction.kind.data.binary.lhs = available_old_instruction;
                        Transfer_reference(instruction, lhs, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                    }
                }
            }
        }
        koopa_raw_value_data_t& rhs = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.binary.rhs);
        if (rhs.kind.tag == KOOPA_RVT_LOAD) {
            if (rhs.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || rhs.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                string variable_name(rhs.kind.data.load.src->name);
                if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                    if (loaded_koopa_register[variable_name] != &instruction) {
                        koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                        instruction.kind.data.binary.rhs = available_old_instruction;
                        Transfer_reference(instruction, rhs, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                    }
                }
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_BRANCH) {
        koopa_raw_value_data_t& condition = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.branch.cond);
        if (condition.kind.tag == KOOPA_RVT_LOAD) {
            if (condition.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || condition.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                string variable_name(condition.kind.data.load.src->name);
                if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                    if (loaded_koopa_register[variable_name] != &instruction) {
                        koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                        instruction.kind.data.branch.cond = available_old_instruction;
                        Transfer_reference(instruction, condition, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                    }
                }
            }
        }
    }else if (instruction.kind.tag == KOOPA_RVT_JUMP) {

    }else if (instruction.kind.tag == KOOPA_RVT_CALL) {
        
    }else if (instruction.kind.tag == KOOPA_RVT_RETURN) {
        if (instruction.kind.data.ret.value != NULL) {
            koopa_raw_value_data_t& ret_value = *const_cast<koopa_raw_value_data_t*>(instruction.kind.data.ret.value);
            if (ret_value.kind.tag == KOOPA_RVT_LOAD) {
                if (ret_value.kind.data.load.src->kind.tag == KOOPA_RVT_ALLOC || ret_value.kind.data.load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                    string variable_name(ret_value.kind.data.load.src->name);
                    if (loaded_koopa_register.find(variable_name) != loaded_koopa_register.end()) {
                        if (loaded_koopa_register[variable_name] != &instruction) {
                            koopa_raw_value_t available_old_instruction = loaded_koopa_register[variable_name];
                            instruction.kind.data.ret.value = available_old_instruction;
                            Transfer_reference(instruction, ret_value, *const_cast<koopa_raw_value_data_t*>(available_old_instruction));
                        }
                    }
                }
            }
        }
    }else{
        int instruction_type_exhausted = 0;
        assert(instruction_type_exhausted);
    }
}

// 注：由于懒狗的缘故，这一函数只维护了used_by的len，务必注意。
void Transfer_reference(koopa_raw_value_data_t &user_instruction, koopa_raw_value_data_t &desserted_instruction, koopa_raw_value_data_t &new_instruction) {
    desserted_instruction.used_by.len -= 1;
    new_instruction.used_by.len += 1;
}
