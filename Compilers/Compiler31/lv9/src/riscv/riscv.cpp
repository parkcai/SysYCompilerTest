#include "riscv.h"
#include <cstring>
#include <cassert>
#include <algorithm>
#include <functional>
#include <iostream>
#define def_visit(...) \
    void RISCV_builder::visit(__VA_ARGS__)
int max(int x, int y) { return x > y ? x : y; }
int RISCV_builder::get_func_size(koopa_raw_function_t func, int *p_current_call) {
    size_t sz = 0;
    int call = 0, maxarg = 0;
    for (auto i = 0; i < func->bbs.len; ++i) {
        auto p = func->bbs.buffer[i];
        sz += get_basic_block_size(call, maxarg, reinterpret_cast<koopa_raw_basic_block_t>(p));
    }
    //                    call                      this's arg num
    sz += call * 4 + max(maxarg - 8, 0) * 4 + max(func->params.len - 8, 0) * 4;
    if (p_current_call != nullptr)
        *p_current_call = call;
    
    return sz;
}
int RISCV_builder::get_basic_block_size(int &call, int &maxarg, koopa_raw_basic_block_t bb) {
    size_t sz = 0;
    for (auto i = 0; i < bb->insts.len; ++i) {
        auto p = bb->insts.buffer[i];
        if (koopa_raw_value_t(p)->kind.tag == KOOPA_RVT_CALL) {
            call = 1;
            maxarg = max(maxarg, koopa_raw_value_t(p)->kind.data.call.args.len);
        }
        // array allocate in asp
        // auto *next_array = new koopa_raw_type_kind_t();
        // auto next_array_ty = new koopa_raw_type_kind_t();
        // next_array->tag = KOOPA_RTT_ARRAY;
        // next_array->data.array.len = dims.back();
        // next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        // for (int i = int(dims.size()) - 2; i >= 0; --i) {
        //     auto *now_array = new koopa_raw_type_kind_t();
        //     now_array->tag = KOOPA_RTT_ARRAY;
        //     now_array->data.array.len = dims[i];
        //     now_array->data.array.base = next_array;
        //     next_array = now_array;
        // }
        // ty->tag = KOOPA_RTT_POINTER;
        // ty->data.pointer.base = next_array;
        // ret->ty = ty;

        int array_size = 0;
        // if (koopa_raw_value_t(p)->kind.tag == KOOPA_RVT_ALLOC) {
            
        //     auto ptr = koopa_raw_value_t(p)->ty->data.pointer.base;
        //     while (ptr->tag == KOOPA_RTT_ARRAY) {
        //         array_size *= ptr->data.array.len;
        //         ptr = ptr->data.array.base;
        //     }
        // }
        sz += 4;
        if (koopa_raw_value_t(p)->kind.tag == KOOPA_RVT_ALLOC) {
            sz += array_size;
            sz += get_array_size(koopa_raw_value_t(p)->ty->data.pointer.base);
            printf("array's size %d\n", get_array_size(koopa_raw_value_t(p)->ty->data.pointer.base));
        }
    }
    return sz;
}
int RISCV_builder::get_array_size(const koopa_raw_type_kind_t *ty) {
    std::function<int(const koopa_raw_type_kind_t *ty)> fun = [&](const koopa_raw_type_kind_t *ty)->int{
        if (ty->tag == KOOPA_RTT_ARRAY) return ty->data.array.len * fun(ty->data.array.base);
        if (ty->tag == KOOPA_RTT_INT32) return 4;
        return 4;
    };
    return fun(ty);
}
str RISCV_builder::build(const koopa_raw_program_t &raw) {
    visit(raw);
    return ss.str();
}
void RISCV_builder::load_to_register(const koopa_raw_value_t &value, str reg) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        ss << "  li " + reg + ", " << value->kind.data.integer.value << "\n";
    } else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF && value->kind.data.func_arg_ref.index < 8) {
        ss << "  mv " + reg + ", a" << value->kind.data.func_arg_ref.index << "\n";
    } else {
        int idx = get_stack_idx(value);
        if (idx >= -2048 && idx < 2048) {
            ss << "  lw " + reg + ", " << idx << "(sp)\n";
        } else {
            // ss << "  li " + reg + ", " << idx << "\n";
            // ss << "  add " + reg + ", " + reg + ", sp\n";
            // ss << "  lw " + reg + ", 0(" << reg << ")\n";
            ss << "  li t2, " << idx << "\n";
            ss << "  add t2, t2, sp\n";
            ss << "  lw " + reg + ", 0(t2)\n";
        }
    }
}
void RISCV_builder::store_to_stack(str reg, int idx) {
    if (idx >= -2048 && idx < 2048) {
        ss << "  sw " + reg + ", " << idx << "(sp)\n";
    } else {
        // ss << "  li " + reg + ", " << idx << "\n";
        // ss << "  add " + reg + ", " + reg + ", sp\n";
        // ss << "  sw " + reg + ", 0(" << reg << ")\n";
        ss << "  li t2, " << idx << "\n";
        ss << "  add t2, t2, sp\n";
        ss << "  sw " + reg + ", 0(t2)\n";
    }
}

def_visit(const koopa_raw_program_t &program) {
    visit(program.values);
    ss << "  .text\n";
    visit(program.funcs);
}
def_visit(const koopa_raw_slice_t &slice) {
    for (int i = 0; i < slice.len; ++i) {
        auto item = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                visit(reinterpret_cast<const koopa_raw_function_t>(item));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
               visit(reinterpret_cast<const koopa_raw_basic_block_t>(item));
               break;
            case KOOPA_RSIK_VALUE:
               visit(reinterpret_cast<const koopa_raw_value_t>(item));
               break;
            default:
                assert(false);
        }
    }
}
def_visit(const koopa_raw_function_t &func) {
    if (func->bbs.len == 0) {
        // a lib function decl
        return;
    }
    auto name = func->name + 1;
    if (func->bbs.len == 0)
        return;
    ss << "  .global " << name << "\n";
    ss << name << ":\n";

    size_t sz = get_func_size(func, &current_call);
    sz = (sz + 15) / 16 * 16;
    if (sz < 2048) {
        ss << "  addi sp, sp, -" << sz << '\n';
    } else {
        ss << "  li t0, " << sz << "\n";
        ss << "  sub sp, sp, t0\n";
    }
    current_size = sz;
    if (current_call) {
        int ra_idx = current_size - 4;
        if (ra_idx < 2048) {
            ss << "  sw ra, " << ra_idx << "(sp)\n";
        } else {
            ss << "  li t0, " << ra_idx << "\n";
            ss << "  add t0, sp, t0\n";
            ss << "  sw ra, 0(t0)\n";
        }
    }

    // reserve location for args
    stack_idx = max(0, func->params.len - 8) * 4;
    visit(func->bbs);

}
def_visit(const koopa_raw_basic_block_t &bb) {
    auto name = bb->name + 1;
    if (strcmp(name, "entry")) ss << name << ":\n";
    visit(bb->insts);
}
def_visit(const koopa_raw_value_t &value) {
    const auto &kind = value->kind;
    switch(kind.tag) {
        case KOOPA_RVT_RETURN:
            visit(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            // visit(kind.data.integer);
            break;
        case KOOPA_RVT_BINARY:
            visit(kind.data.binary, value);
            break;
        case KOOPA_RVT_ALLOC:
            // visit alloc
            break;
        case KOOPA_RVT_STORE:
            visit(kind.data.store);
            break;
        case KOOPA_RVT_LOAD:
            visit(kind.data.load, value);
            break;
        case KOOPA_RVT_BRANCH:
            visit(kind.data.branch);
            break;
        case KOOPA_RVT_JUMP:
            visit(kind.data.jump);
            break;
        case KOOPA_RVT_CALL:
            visit(kind.data.call, value);
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            visit(kind.data.global_alloc, value);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            visit(kind.data.get_elem_ptr, value);
            break;
        case KOOPA_RVT_GET_PTR:
            visit(kind.data.get_ptr, value);
            break;
        default:
            assert(false);
    }
}
def_visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        ss << "  la t0, " << get_elem_ptr.src->name + 1 << "\n";
        // get index
        load_to_register(get_elem_ptr.index, "t1");
        if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_PTR
            || get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
                ss << "  lw t0, 0(t0)\n";
        // ss << "  li t2, " << get_array_size(get_elem_ptr.src->ty->data.pointer.base) << "\n"; 原数组
        ss << "  li t2, " << get_array_size(get_elem_ptr.src->ty->data.pointer.base->data.array.base) << "\n";
        ss << "  mul t1, t1, t2\n";
        ss << "  add t0, t0, t1\n";
        store_to_stack("t0", get_stack_idx(value));
        return;
    }
    int idx = get_stack_idx(get_elem_ptr.src);
    // get arr
    if (idx >= -2048 && idx < 2048)
        ss << "  addi t0, sp, " << idx << "\n";
    else {
        ss << "  li t0, " << idx << "\n";
        ss << "  add t0, t0, sp\n";
    }
    // get index
    load_to_register(get_elem_ptr.index, "t1");
    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_PTR
        || get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
            ss << "  lw t0, 0(t0)\n";
    // ss << "  li t2, " << get_array_size(get_elem_ptr.src->ty->data.pointer.base) << "\n"; 原数组
    ss << "  li t2, " << get_array_size(get_elem_ptr.src->ty->data.pointer.base->data.array.base) << "\n";
    ss << "  mul t1, t1, t2\n";
    ss << "  add t0, t0, t1\n";
    store_to_stack("t0", get_stack_idx(value));
}

def_visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
    int idx = get_stack_idx(get_ptr.src);
    // get arr
    if (idx >= -2048 && idx < 2048)
        ss << "  addi t0, sp, " << idx << "\n";
    else {
        ss << "  li t0, " << idx << "\n";
        ss << "  add t0, t0, sp\n";
    }
    // get index
    load_to_register(get_ptr.index, "t1");
    // if (get_ptr.src->kind.tag == KOOPA_RVT_GET_PTR
    //     || get_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
            ss << "  lw t0, 0(t0)\n";
    // ss << "  li t2, " << get_array_size(get_elem_ptr.src->ty->data.pointer.base) << "\n"; 
    // 原数组，在get_ptr中为原指针，即第二层的数组指针
    ss << "  li t2, " << get_array_size(get_ptr.src->ty->data.pointer.base) << "\n";
    ss << "  mul t1, t1, t2\n";
    ss << "  add t0, t0, t1\n";
    store_to_stack("t0", get_stack_idx(value));
}
def_visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
    ss << "  .data\n" << "  .global " << value->name + 1 << "\n";
    ss << value->name + 1 << ":\n";
    if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
        std::function<int(const koopa_raw_type_kind_t *ty)> get_size = [&](const koopa_raw_type_kind_t *ty) -> int {
            printf("##########%d", ty->tag);
            if (ty->tag == KOOPA_RTT_ARRAY)
                return get_size(ty->data.array.base) * ty->data.array.len;
            else
                return 1;
        };
        ss << "  .zero " << 4 * get_size(global_alloc.init->ty) << "\n";
    } else if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
        ss << "  .word " << global_alloc.init->kind.data.integer.value << "\n";
    } else {
        std::function<void(const koopa_raw_aggregate_t &agg)> func = [&](const koopa_raw_aggregate_t &agg) {
            for (int i = 0; i < agg.elems.len; ++i) {
                auto item = (koopa_raw_value_t)agg.elems.buffer[i];
                if (item->kind.tag == KOOPA_RVT_INTEGER)
                    ss << "  .word " << item->kind.data.integer.value << "\n";
                else {
                    assert(item->kind.tag == KOOPA_RVT_AGGREGATE);
                    func(item->kind.data.aggregate);
                }
            }
        };
        auto agg = global_alloc.init->kind.data.aggregate;
        func(agg);
    }
}
def_visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
    // mv first 8 args into a0~a7
    for (int i = 0; i < 8 && i < call.args.len; ++i)
        load_to_register(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), 
                        "a" + to_string(i));
    // store args into stack
    int sz = get_func_size(call.callee, nullptr);
    sz = (sz + 15) / 16 * 16;
    for (int i = 8; i < call.args.len; ++i) {
        load_to_register(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), 
                        "t2");
        store_to_stack("t2", -sz + (i - 8) * 4); // (sp - sz) be sp for callee
    }
    ss << "  call " << call.callee->name + 1 << "\n";
    int idx = get_stack_idx(value);
    store_to_stack("a0", idx);
}
def_visit(const koopa_raw_branch_t &branch) {
    load_to_register(branch.cond, "t0");
    // ss << "  bnez t0, " << branch.true_bb->name + 1 << "\n";
    // ss << "  j " << branch.false_bb->name + 1 << "\n";
    ss << "  bnez t0, " << branch.true_bb->name + 1 << "_temp" << "\n";
    ss << "  j " << branch.false_bb->name + 1 << "\n";
    ss << branch.true_bb->name + 1 << "_temp:\n";
    ss << "  j " << branch.true_bb->name + 1 << "\n";
}
def_visit(const koopa_raw_jump_t &jump) {
    ss << "  j " << jump.target->name + 1 << "\n";
}
def_visit(const koopa_raw_store_t &store) {
    load_to_register(store.value, "t3");
    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        ss << "  la t2, " << store.dest->name + 1 << "\n"; //global var's addr
        ss << "  sw t3, 0(t2)\n";
        return;
    }
    if (store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR
        || store.dest->kind.tag == KOOPA_RVT_GET_PTR) {
        load_to_register(store.dest, "t2");
        ss << "  sw t3, 0(t2)\n";
        return;
    }
    int idx = get_stack_idx(store.dest);
    store_to_stack("t3", idx);
}
def_visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
    int idx = get_stack_idx(value);
    if (load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR
        || load.src->kind.tag == KOOPA_RVT_GET_PTR) {
        load_to_register(load.src, "t3");
        ss << "  lw t3, 0(t3)\n";
    } else if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        ss << "  la t3, " << load.src->name + 1 << "\n";
        ss << "  lw t3, 0(t3)\n";
    } else {
        load_to_register(load.src, "t3");
    }
    store_to_stack("t3", idx);
}
def_visit(const koopa_raw_return_t &ret) {
    if (current_call) {
        int ra_idx = current_size - 4;
        if (ra_idx < 2048) {
            ss << "  lw ra, " << ra_idx << "(sp)\n";
        } else {
            ss << "  li t0, " << ra_idx << "\n";
            ss << "  add t0, sp, t0\n";
            ss << "  lw ra, 0(t0)\n";
        }
    }
    if (ret.value == nullptr) {
        ;
    } else if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        ss << "  li a0, " << ret.value->kind.data.integer.value << "\n";
    } else {
        // ss << "  mv a0, " << get_register_name(ret.value) << "\n";
        load_to_register(ret.value, "a0");
    }
    
    // free stack
    if (current_size < 2048) {
        ss << "  addi sp, sp, " << current_size << "\n";
    } else {
        ss << "  li t0, " << current_size << "\n";
        ss << "  add sp, sp, t0\n";
    }
    ss << "  ret\n";
}
str RISCV_builder::get_register_name(const koopa_raw_value_t &value) {
    // Deprecated
    if (reg_map.find(value) == reg_map.end()) {
        if (register_name.empty()) {
            register_name = {"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t2", "t3", "t4", "t5" ,"t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
        }
        std::random_shuffle(register_name.begin(), register_name.end());
        reg_map[value] = *register_name.rbegin();
        register_name.pop_back();
        return reg_map[value];
    } else {
        auto ret = reg_map[value];
        reg_map.erase(value);
        register_name.push_back(ret);
        return ret;
    }
}
int RISCV_builder::get_stack_idx(const koopa_raw_value_t &value) {
    if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        assert(value->kind.data.func_arg_ref.index >= 8);
        return /*stack_map[value] = */4 * (value->kind.data.func_arg_ref.index - 8);
    }
    auto it = stack_map.find(value);
    if (it != stack_map.end())
        return it->second;
    stack_map[value] = stack_idx;
    if (value->kind.tag == KOOPA_RVT_ALLOC)
        stack_idx += get_array_size(value->ty->data.pointer.base);
    else
        stack_idx += 4; // inst length
    return stack_map[value];
}
def_visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER) {
        ss << "  li t0, " << binary.lhs->kind.data.integer.value << "\n";
    } else if (binary.lhs->kind.tag == KOOPA_RVT_BINARY) {
        // ss << "  mv t0, " << get_register_name(binary.lhs) << "\n";
        // ss << "  lw t0, " << get_stack_idx(binary.lhs) << "(sp)\n";
        load_to_register(binary.lhs, "t0");
    } else if (binary.lhs->kind.tag == KOOPA_RVT_LOAD) {
        // ss << "  lw t0, " << get_stack_idx(binary.lhs) << "(sp)\n";
        load_to_register(binary.lhs, "t0");
    } else {
        load_to_register(binary.lhs, "t0");
    }
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER) {
        ss << "  li t1, " << binary.rhs->kind.data.integer.value << "\n";
    } else if (binary.rhs->kind.tag == KOOPA_RVT_BINARY) {
        // ss << "  mv t1, " << get_register_name(binary.rhs) << "\n";
        // ss << "  lw t1, " << get_stack_idx(binary.rhs) << "(sp)\n";
        load_to_register(binary.rhs, "t1");
    } else if (binary.rhs->kind.tag == KOOPA_RVT_LOAD) {
        // ss << "  lw t1, " << get_stack_idx(binary.rhs) << "(sp)\n";
        load_to_register(binary.rhs, "t1");
    } else {
        load_to_register(binary.rhs, "t1");
    }
    switch (binary.op) {
        case KOOPA_RBO_SUB:
            ss << "  sub t0, t0, t1\n";
            break;
        case KOOPA_RBO_EQ:
            ss << "  xor t0, t0, t1\n";
            ss << "  seqz t0, t0" << "\n";
            break;
        case KOOPA_RBO_NOT_EQ:
            ss << "  xor t0, t0, t1\n";
            ss << "  snez t0, t0" << "\n";
            break;
        case KOOPA_RBO_ADD:
            ss << "  add t0, t0, t1\n";
            break;
        case KOOPA_RBO_MUL:
            ss << "  mul t0, t0, t1\n";
            break;
        case KOOPA_RBO_DIV:
            ss << "  div t0, t0, t1\n";
            break;
        case KOOPA_RBO_MOD:
            ss << "  rem t0, t0, t1\n";
            break;
        case KOOPA_RBO_LE:
            ss << "  slt t0, t1, t0\n"; // <= 即,not(>)，先>
            ss << "  seqz t0, t0\n"; // 取反
            break;
        case KOOPA_RBO_LT:
            ss << "  slt t0, t0, t1\n";
            break;
        case KOOPA_RBO_GE:
            ss << "  slt t0, t0, t1\n";
            ss << "  seqz t0, t0\n";
            break;
        case KOOPA_RBO_GT:
            ss << "  slt t0, t1, t0\n";
            break;
        case KOOPA_RBO_AND:
            ss << "  and t0, t0, t1\n";
            break;
        case KOOPA_RBO_OR:
            ss << "  or t0, t0, t1\n";
            break;
        case KOOPA_RBO_XOR:
            ss << "  xor t0, t0, t1\n";
            break;
    }
    // ss << "  mv " << get_register_name(value) << ", t0\n";
    // ss << "  sw t0, " << get_stack_idx(value) << "(sp)\n";
    store_to_stack("t0", get_stack_idx(value));
}