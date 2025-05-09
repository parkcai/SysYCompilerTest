
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <ast.hpp>
#include <cstring>
#include "koopa_gen.hpp"
#include "koopa.h"
#include <unordered_map>

using std::cout;

const int REGISTERS_CNT = 14;
static Register temp_register;
static Register registers[REGISTERS_CNT];
static Register *register_free_head;
static Register register_zero;
// use callee saved regs to store vars
const int VAR_REGISTERS_CNT = 12;
static Register var_registers[VAR_REGISTERS_CNT];

static int have_call = 0;

static StackFrame *stack_frame;

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
    // 执行一些其他的必要操作
    // ...
    cout << "  .text\n";
    cout << "  .globl main\n";

    // 访问所有全局变量
    Visit(program.values);
    // 访问所有函数
    Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
    // 执行一些其他的必要操作
    // ...
    // lib functions
    if (func->bbs.len == 0) {
        return;
    }
    cout << "  .text\n";
    cout << func->name+1 << ":\n";

    // prologue
    // calc the size of stack frame
    int var_cnt = 0;
    int arg_cnt = 0, arg_offet = 0;
    int stack_size;
    // calc each var's use of times
    std::unordered_map<std::string, int> var_use;
    for (size_t i = 0; i < func->bbs.len; ++i) {
        const auto &insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
        for (size_t j = 0; j < insts.len; ++j) {
            const auto &inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
            if (inst->ty->tag != KOOPA_RTT_UNIT) {
                // calc the size of array
                if (inst->kind.tag == KOOPA_RVT_ALLOC &&
                    inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
                    int mem = 1;
                    auto ptr = inst->ty->data.pointer.base;
                    while (ptr->tag == KOOPA_RTT_ARRAY) {
                        mem *= ptr->data.array.len;
                        ptr = ptr->data.array.base;
                    }
                    var_cnt += mem;
                }
                else {
                    var_cnt++;
                }
            }
            if (inst->kind.tag == KOOPA_RVT_CALL) {
                have_call = 1;
                if (inst->kind.data.call.args.len > arg_cnt)
                    arg_cnt = inst->kind.data.call.args.len;
            }
            const char *name = NULL;
            switch (inst->kind.tag) 
            {
            case KOOPA_RVT_ALLOC:
                name = inst->name;
                break;
            case KOOPA_RVT_LOAD:
                if (inst->kind.data.load.src->name[0] == '@') {
                    name = inst->kind.data.load.src->name;
                }
                break;
            case KOOPA_RVT_STORE:
                if (inst->kind.data.store.dest->name[0] == '@') {
                    name = inst->kind.data.store.dest->name;
                }
                break;
            default:
                break;
            }
            if (name != NULL && name[1] == '_') {
                if (var_use.find(name) == var_use.end()) {
                    var_use[name] = 1;
                }
                else {
                    var_use[name]++;
                }
            }
        }
    }
    arg_offet = arg_cnt > 8 ? (arg_cnt - 8) * 4 : 0;
    stack_size = var_cnt * 4 + have_call * 4 + arg_offet + VAR_REGISTERS_CNT * 4;
    stack_size = (stack_size / 16 + !!(stack_size % 16)) * 16;
    init_stackframe(stack_size, arg_offet);
    init_registers();

    if (stack_frame->size > 2040) {
        Register *temp_reg = &temp_register;
        cout << "  li " << temp_reg->name << ", " << -stack_frame->size << "\n";
        cout << "  add sp, sp, " << temp_reg->name << "\n";
        free_registers(temp_reg);
    }
    else {
        cout << "  add sp, sp, -" << stack_frame->size << "\n";
    }
    if (have_call) {
        if (stack_frame->size > 2044) {
            Register *temp_reg = &temp_register;
            cout << "  li " << temp_reg->name << ", " << stack_frame->size-4 << "\n";
            cout << "  add " << temp_reg->name << ", sp, " << temp_reg->name << "\n";
            cout << "  sw ra, " << "(" << temp_reg->name << ")\n";
            free_registers(temp_reg);
        }
        else {
            cout << "  sw ra, " << stack_frame->size-4 << "(sp)\n";
        }
    }

    printf("var frequancy:\n");
    for (auto i = var_use.begin(); i != var_use.end(); i++) {
        printf("\t%s: %d\n", i->first.c_str(), i->second);
    }
    // save all callee saved registers
    for (int i = 0; i < VAR_REGISTERS_CNT && var_use.size() > 0; i++) {
        int max = 0; 
        std::string max_name;
        for (auto j = var_use.begin(); j != var_use.end(); j++) {
            if (j->second > max) {
                max = j->second;
                max_name = j->first;
            }
        }
        save_reg2stack(&var_registers[i], alloc_stackframe(var_registers[i].name, 4));
        // global
        if (max_name[max_name.size()-1] == '0' && max_name[max_name.size()-2] == '_'){
            var_registers[i].condition = GLOBAL;
            var_registers[i].map = max_name;
            cout << "  la " << var_registers[i].name << ", " << max_name.c_str()+1 << "\n";
            cout << "  lw " << var_registers[i].name << ", " << "(" << var_registers[i].name << ")\n";
        }
        else {
            var_registers[i].condition = ALLOCATED;
            var_registers[i].map = max_name;
        }
        var_use.erase(max_name);
    }
    // 访问所有基本块
    Visit(func->bbs);

    printf("used %d/%d in func %s\n", stack_frame->allocated, stack_frame->size, func->name+1);

    free_stackframe();
    have_call = 0;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
    // 执行一些其他的必要操作

    if (strcmp(bb->name+1, "entry"))
        cout << bb->name+1 << ":\n";

    // 访问所有指令
    Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    int size = 4;
    const koopa_raw_type_kind *ptr;
    switch (kind.tag) {
        case KOOPA_RVT_ALLOC:
            // 访问 alloc 指令
            if (value->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
                ptr = value->ty->data.array.base;
                while (ptr->tag == KOOPA_RTT_ARRAY) {
                    size *= ptr->data.array.len;
                    ptr = ptr->data.array.base;
                }
            }
            alloc_stackframe(value->name, size);
            break;
        case KOOPA_RVT_LOAD:
            // 访问 load 指令
            Visit(kind.data.load, value);
            break;
        case KOOPA_RVT_STORE:
            // 访问 store 指令
            Visit(kind.data.store);
            break;
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            Visit(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            Visit(kind.data.integer, value);
            break;
        case KOOPA_RVT_BINARY:
            Visit(kind.data.binary, value);
            break;
        case KOOPA_RVT_BRANCH:
            Visit(kind.data.branch);
            break;
        case KOOPA_RVT_JUMP:
            Visit(kind.data.jump);
            break;
        case KOOPA_RVT_CALL:
            Visit(kind.data.call, value);
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            Visit(kind.data.global_alloc, value);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            Visit(kind.data.get_elem_ptr, value);
            break;
        case KOOPA_RVT_GET_PTR:
            Visit(kind.data.get_ptr, value);
            break;
        default:
            // 其他类型暂时遇不到
            assert(false);
    }
}


///////////////////
/// instruction ///
///////////////////

void Visit(const koopa_raw_return_t &ret) {
    Register *ret_reg;
    if (ret.value) {
        if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
            cout << "  li a0, " << ret.value->kind.data.integer.value << '\n';
        }
        else if (ret.value){
            ret_reg = load2registers(ret.value);
            if (strcmp(ret_reg->name, "a0") != 0) {
                cout << "  mv "  << "a0, " << ret_reg->name << '\n';
            }
            free_registers(ret_reg);
        }
    }

    // epilogue
    // restore all callee saved registers
    for (int i = 0; i < VAR_REGISTERS_CNT; i++) {
        if (var_registers[i].condition == GLOBAL) {
            cout << "  la " << temp_register.name << ", " << var_registers[i].map.c_str()+1 << '\n';
            cout << "  sw " << var_registers[i].name << ", (" << temp_register.name << ")\n";
        }
        if (stack_frame->map.find(var_registers[i].name) != stack_frame->map.end()) {
            load_stack2reg(&var_registers[i], get_stack_offset(var_registers[i].name), true);
        }
    }
    if (have_call) {
        if (stack_frame->size > 2044) {
            cout << "  li ra, " << stack_frame->size-4 << "\n";
            cout << "  add ra, sp, ra\n";
            cout << "  lw ra, (ra)\n";
        }
        else {
            cout << "  lw ra, " << stack_frame->size-4 << "(sp)\n";
        }
    }
    if (stack_frame->size > 2040) {
        Register *temp_reg = &temp_register;
        cout << "  li " << temp_reg->name << ", " << stack_frame->size << "\n";
        cout << "  add sp, sp, " << temp_reg->name << "\n";
        free_registers(temp_reg);
    }
    else {
        cout << "  add sp, sp, " << stack_frame->size << "\n";
    }
    cout << "  ret\n";
}

void Visit(const koopa_raw_integer_t &integer, const koopa_raw_value_t &value) {
    Register *reg = alloc_registers(value->name, TEMP);
    cout << "  li " << reg->name << ", " << integer.value << '\n';
    alloc_stackframe(value->name);
}

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
    Register *load_reg;
    // if we want to load from a temp value, that means we need to dereference its address
    if (load.src->name[0] == '%') {
        load_reg = load2registers(load.src);
        cout << "  lw " << load_reg->name << ", (" << load_reg->name << ")\n";
        alloc_stackframe(value->name);
        load_reg->map = value->name;
        load_reg->condition = TEMP;
    }
    else {
        load_reg = load2registers(load.src);
        alloc_stackframe(value->name);
        load_reg->map = value->name;
        load_reg->condition = TEMP;
    }
}

void Visit(const koopa_raw_store_t &store) {
    Register *store_reg, *value_arg;
    // if we want to store to a temp value, that means we need to store it into its address
    if (store.dest->name[0] == '%') {
        value_arg = load2registers(store.value);
        value_arg->can_evict = 0;
        store_reg = load2registers(store.dest);
        cout << "  sw " << value_arg->name << ", (" << store_reg->name << ")\n";
        free_registers(store_reg);
        free_registers(value_arg);
    }
    else {
        // if the store dest is a variable that we reserved 
        for (int i = 0; i < VAR_REGISTERS_CNT; i++) {
            if (var_registers[i].condition != FREE && var_registers[i].map == store.dest->name) {
                value_arg = load2registers(store.value);
                cout << "  mv " << var_registers[i].name << ", " << value_arg->name << '\n';
                free_registers(value_arg);
                return;
            }
        }

        if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            value_arg = load2registers(store.value);
            value_arg->can_evict = 0;
            store_reg = alloc_registers();
            cout << "  la " << store_reg->name << ", " << store.dest->name+1 << '\n';
            cout << "  sw " << value_arg->name << ", (" << store_reg->name << ")\n";
            free_registers(store_reg);
            free_registers(value_arg);
            return;
        }
        int offset = get_stack_offset(store.dest->name);
        value_arg = load2registers(store.value);
        value_arg->can_evict = 0;
        save_reg2stack(value_arg, offset);
        free_registers(value_arg);
    }
}

void Visit(const koopa_raw_branch_t &branch) {
    Register *cond_reg;
    cond_reg = load2registers(branch.cond);
    cond_reg->can_evict = 0;
    // save all registers before jmp
    for (int i = 0; i < REGISTERS_CNT; i++) {
        if (registers[i].condition != FREE && registers[i].name != cond_reg->name) {
            save_reg2stack(&registers[i], get_stack_offset(registers[i].map));
            free_registers(&registers[i]);
        }
    }
    cout << "  bnez " << cond_reg->name << ", " << branch.true_bb->name+1 << '\n';
    cout << "  j " << branch.false_bb->name+1 << '\n';
    free_registers(cond_reg);
}

void Visit(const koopa_raw_jump_t &jump) {
    // save all registers before jmp
    for (int i = 0; i < REGISTERS_CNT; i++) {
        if (registers[i].condition != FREE) {
            save_reg2stack(&registers[i], get_stack_offset(registers[i].map));
            free_registers(&registers[i]);
        }
    }
    cout << "  j " << jump.target->name+1 << '\n';
}

void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
    Register *temp_reg;
    int arg_cnt = call.args.len;
    // before call, we should save all caller saved registers
    // do something
    for (int i = 0; i < arg_cnt; i++) {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (i < 8) {
            if (registers[i].condition != FREE) {
                if (registers[i].map == arg->name) {
                    registers[i].condition = TEMP;
                    registers[i].map = "";
                    registers[i].can_evict = 0;
                    continue;
                }
                else {
                    save_reg2stack(&registers[i], get_stack_offset(registers[i].map));
                    free_registers(&registers[i]);
                }
            }
            assert(registers[i].condition == FREE);
            registers[i].condition = TEMP;
            registers[i].map = "";
            registers[i].can_evict = 0;
            temp_reg = load2registers(arg);
            cout << "  mv " << registers[i].name << ", " << temp_reg->name << '\n';
            free_registers(temp_reg);
        }
        else {
            temp_reg = load2registers(arg);
            save_reg2stack(temp_reg, (i-8)*4);
            free_registers(temp_reg);
        }
    }

    // save all caller saved registers
    for (int i = arg_cnt < 8 ? arg_cnt : 8; i < REGISTERS_CNT; i++) {
        if (registers[i].condition != FREE) {
            save_reg2stack(&registers[i], get_stack_offset(registers[i].map));
            free_registers(&registers[i]);
        }
    }

    // update global values 
    for (int i = 0; i < VAR_REGISTERS_CNT; i++) {
        if (var_registers[i].condition == GLOBAL) {
            cout << "  la " << temp_register.name << ", " << var_registers[i].map.c_str()+1 << '\n';
            cout << "  sw " << var_registers[i].name << ", (" << temp_register.name << ")\n";
        }
    }

    cout << "  call " << call.callee->name+1 << '\n';

    // update global value registers
    for (int i = 0; i < VAR_REGISTERS_CNT; i++) {
        if (var_registers[i].condition == GLOBAL) {
            cout << "  la " << temp_register.name << ", " << var_registers[i].map.c_str()+1 << '\n';
            cout << "  lw " << var_registers[i].name << ", (" << temp_register.name << ")\n";
        }
    }

    for (int i = 0; i < arg_cnt; i++) {
        if (i < 8) {
            registers[i].condition = FREE;
            registers[i].map = "";
            registers[i].can_evict = 1;
        }
    }

    if (value->ty->tag != KOOPA_RTT_UNIT) {
        temp_reg = alloc_registers(value->name, TEMP);
        alloc_stackframe(value->name);
        cout << "  mv " << temp_reg->name << ", a0" << '\n';
    }
}

// recursively cout contents of aggregate
static int tot_zeros;
void global_aggregate_init(const koopa_raw_value_t &value) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        if (value->kind.data.integer.value == 0) {
            tot_zeros += 4;
            return;
        }
        else {
            if (tot_zeros > 0) {
                cout << "  .zero " << tot_zeros << '\n';
                tot_zeros = 0;
            }
            cout << "  .word " << value->kind.data.integer.value << '\n';
            return;
        }
    }
    else if (value->kind.tag == KOOPA_RVT_AGGREGATE) {
        auto aggregate = value->kind.data.aggregate;
        for (int i = 0; i < aggregate.elems.len; i++) {
            auto elem = reinterpret_cast<koopa_raw_value_t>(aggregate.elems.buffer[i]);
            global_aggregate_init(elem);
        }
        return;
    }
    else {
        assert(false);
    }
}


void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
    cout << "  .data\n";
    cout << "  .globl " << value->name+1 << '\n';
    cout << value->name+1 << ":\n";

    if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
        cout << "  .word " << global_alloc.init->kind.data.integer.value << '\n';
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
        auto ptr = value->ty->data.pointer.base;
        if (ptr->tag == KOOPA_RTT_INT32) {
            cout << "  .zero 4\n";
        }
        else if (ptr->tag == KOOPA_RTT_ARRAY) {
            int size = 4;
            while (ptr->tag == KOOPA_RTT_ARRAY)
            {
                size *= ptr->data.array.len;
                ptr = ptr->data.array.base;
            }
            cout << "  .zero " << size << '\n';
        }
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
        tot_zeros = 0;
        global_aggregate_init(global_alloc.init);
        if (tot_zeros > 0) {
            cout << "  .zero " << tot_zeros << '\n';
        }
    }
}

void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
    assert(get_elem_ptr.src->ty->tag == KOOPA_RTT_POINTER);
    Register *out_reg, *index_reg, *offset_reg;
    int size = 1;
    auto ptr = get_elem_ptr.src->ty->data.pointer.base->data.array.base;
    // the global variable is an abstract pointer, not actually on stack
    if (get_elem_ptr.src->name[0] == '@') {
        if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            out_reg = alloc_registers();
            cout << "  la " << out_reg->name << ", " << get_elem_ptr.src->name+1 << '\n';
        }
        else {
            out_reg = alloc_registers();
            int offset = get_stack_offset(get_elem_ptr.src->name);
            if (offset > 2044) {
                cout << "  li " << out_reg->name << ", " << offset << '\n';
                cout << "  add " << out_reg->name << ", sp, " << out_reg->name << '\n';
            }
            else {
                cout << "  add " << out_reg->name << ", sp, " << offset << '\n';
            }
        }
    }
    // all temp value are saved on stack
    else {
        out_reg = load2registers(get_elem_ptr.src);
    }
    while (ptr->tag == KOOPA_RTT_ARRAY)
    {
        size *= ptr->data.array.len;
        ptr = ptr->data.array.base;
    }
    out_reg->can_evict = 0;
    if (get_elem_ptr.index->kind.tag != KOOPA_RVT_INTEGER) {
        offset_reg = alloc_registers();
        offset_reg->can_evict = 0;
        cout << "  li " << offset_reg->name << ", " << size * 4 << '\n';
        index_reg = load2registers(get_elem_ptr.index);
        cout << "  mul " << offset_reg->name << ", " << offset_reg->name << ", " << index_reg->name << '\n';
        free_registers(index_reg);
        cout << "  add " << out_reg->name << ", " << out_reg->name << ", " << offset_reg->name << '\n';
        free_registers(offset_reg);
        alloc_stackframe(value->name);
        out_reg->map = value->name;
        out_reg->condition = TEMP;
        out_reg->can_evict = 1;
    }
    else {
        int offset = get_elem_ptr.index->kind.data.integer.value * size * 4;
        if (offset == 0) {
            /* do nothing */
        }
        else if (offset < 2044) {
            cout << "  addi " << out_reg->name << ", " << out_reg->name << ", " << offset << '\n';
        }
        else {
            cout << "  li " << temp_register.name << ", " << offset << '\n';
            cout << "  add " << out_reg->name << ", " << out_reg->name << ", " << temp_register.name << '\n';
        }
        alloc_stackframe(value->name);
        out_reg->map = value->name;
        out_reg->condition = TEMP;
        out_reg->can_evict = 1;
    }
}

void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
    assert(get_ptr.src->ty->tag == KOOPA_RTT_POINTER);
    // here all value in getptr must be temp value
    assert(get_ptr.src->name[0] == '%');
    Register *out_reg, *index_reg, *offset_reg;
    out_reg = load2registers(get_ptr.src);
    int size = 1;
    auto ptr = get_ptr.src->ty->data.pointer.base;
    while (ptr->tag == KOOPA_RTT_ARRAY)
    {
        size *= ptr->data.array.len;
        ptr = ptr->data.array.base;
    }
    out_reg->can_evict = 0;
    if (get_ptr.index->kind.tag != KOOPA_RVT_INTEGER) {
        offset_reg = alloc_registers();
        offset_reg->can_evict = 0;
        cout << "  li " << offset_reg->name << ", " << size * 4 << '\n';
        index_reg = load2registers(get_ptr.index);
        cout << "  mul " << offset_reg->name << ", " << offset_reg->name << ", " << index_reg->name << '\n';
        free_registers(index_reg);
        cout << "  add " << out_reg->name << ", " << out_reg->name << ", " << offset_reg->name << '\n';
        free_registers(offset_reg);
        alloc_stackframe(value->name);
        out_reg->map = value->name;
        out_reg->condition = TEMP;
        out_reg->can_evict = 1;
    }
    else {
        int offset = get_ptr.index->kind.data.integer.value * size * 4;
        if (offset == 0) {
            /* do nothing */
        }
        else if (offset < 2044) {
            cout << "  addi " << out_reg->name << ", " << out_reg->name << ", " << offset << '\n';
        }
        else {
            cout << "  li " << temp_register.name << ", " << offset << '\n';
            cout << "  add " << out_reg->name << ", " << out_reg->name << ", " << temp_register.name << '\n';
        }
        alloc_stackframe(value->name);
        out_reg->map = value->name;
        out_reg->condition = TEMP;
        out_reg->can_evict = 1;
    }
}

void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
    Register *out_reg, *lhs_reg, *rhs_reg;

    // process calc two numbers specially
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER && 
        binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        int output = 0;
        out_reg = alloc_registers(value->name, TEMP);
        alloc_stackframe(value->name);
        switch (binary.op)
        {
        case KOOPA_RBO_ADD:
            output = binary.lhs->kind.data.integer.value + binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_SUB:
            output = binary.lhs->kind.data.integer.value - binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_MUL:
            output = binary.lhs->kind.data.integer.value * binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_DIV:
            output = binary.lhs->kind.data.integer.value / binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_MOD:
            output = binary.lhs->kind.data.integer.value % binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_EQ:
            output = binary.lhs->kind.data.integer.value == binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_NOT_EQ:
            output = binary.lhs->kind.data.integer.value != binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_LT:
            output = binary.lhs->kind.data.integer.value < binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_GT:
            output = binary.lhs->kind.data.integer.value > binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_AND:
            output = binary.lhs->kind.data.integer.value && binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_OR:
            output = binary.lhs->kind.data.integer.value || binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_LE:
            output = binary.lhs->kind.data.integer.value <= binary.rhs->kind.data.integer.value;
            break;
        case KOOPA_RBO_GE:
            output = binary.lhs->kind.data.integer.value >= binary.rhs->kind.data.integer.value;
            break;
        default:
            assert(!"not implemented op!\n");
            break;
        }
        cout << "  li " << out_reg->name << ", " 
             << output << '\n';
        return;
    }
    // one of them is an integer
    else if ((binary.lhs->kind.tag == KOOPA_RVT_INTEGER || binary.rhs->kind.tag == KOOPA_RVT_INTEGER) && 
        (binary.op == KOOPA_RBO_ADD)
        ) {
        auto integer = (binary.lhs->kind.tag == KOOPA_RVT_INTEGER ? binary.lhs : binary.rhs);
        auto var = (binary.lhs->kind.tag == KOOPA_RVT_INTEGER ? binary.rhs : binary.lhs);
        if (integer->kind.data.integer.value < 2044 && integer->kind.data.integer.value > -2044) {
            lhs_reg = load2registers(var);
            cout << "  addi " << lhs_reg->name << ", " << lhs_reg->name << ", " << integer->kind.data.integer.value << '\n';
            lhs_reg->map = value->name;
            lhs_reg->condition = TEMP;
            return;
        }
    }
    // temp varibles can only be used for one time
    lhs_reg = load2registers(binary.lhs);
    lhs_reg->can_evict = 0;
    rhs_reg = load2registers(binary.rhs);
    lhs_reg->can_evict = 1;
    if (rhs_reg->condition == TEMP){
        free_registers(rhs_reg);
    }
    if (lhs_reg->condition == TEMP) {
        free_registers(lhs_reg);
    }
    out_reg = alloc_registers(value->name, TEMP);
    alloc_stackframe(value->name);
    switch (binary.op)
    {
    case KOOPA_RBO_ADD:
        cout << "  add " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_SUB:
        cout << "  sub " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_MUL:
        cout << "  mul " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_DIV:
        cout << "  div " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_MOD:
        cout << "  rem " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;    
    case KOOPA_RBO_EQ:
        cout << "  xor " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        cout << "  seqz " << out_reg->name << ", " << out_reg->name << '\n';
        break;
    case KOOPA_RBO_NOT_EQ:
        cout << "  xor " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        cout << "  snez " << out_reg->name << ", " << out_reg->name << '\n';
        break;
    case KOOPA_RBO_GT:
        cout << "  sgt " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_LT:
        cout << "  slt " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_GE:  // not LT
        cout << "  slt " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        cout << "  xori " << out_reg->name << ", " << out_reg->name << ", 1\n"; 
        break;
    case KOOPA_RBO_LE:  // not GT
        cout << "  sgt " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        cout << "  xori " << out_reg->name << ", " << out_reg->name << ", 1\n"; 
        break;
    case KOOPA_RBO_AND:
        cout << "  and " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_OR:
        cout << "  or " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_XOR:
        cout << "  xor " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_SHL:
        cout << "  sll " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_SHR:
        cout << "  srl " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;
    case KOOPA_RBO_SAR:
        cout << "  sra " << out_reg->name << ", " << lhs_reg->name << ", " << rhs_reg->name << '\n';
        break;

    default:
        assert("binary: no such operation in koopa!\n");
        break;
    }
}

//////////////////////
///   registers   ////
//////////////////////

// alloc a new temp register to store value from memory
Register *load2registers(const koopa_raw_value_t &value) {
    Register *ret;
    int offset;
    for (int i = 0; i < REGISTERS_CNT; i++) {
        if (value->name && registers[i].map == value->name) {
            // printf("repeatly choose %s:%s\n", registers[i].name, value->name);
            return &registers[i];
        }
    }
    for (int i = 0; i < VAR_REGISTERS_CNT; i++) {
        if (value->name && var_registers[i].condition != FREE && value->name == var_registers[i].map) {
            ret = alloc_registers();
            cout << "  mv " << ret->name << ", " << var_registers[i].name << '\n';
            return ret;
        }
    }

    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        if (value->kind.data.integer.value == 0) {
            return &register_zero;
        }
        ret = alloc_registers();
        cout << "  li " << ret->name << ", " << value->kind.data.integer.value << '\n';
        return ret;
    }
    else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        ret = alloc_registers();
        cout << "  la " << ret->name << ", " << value->name+1 << '\n';
        cout << "  lw " << ret->name << ", (" << ret->name << ")\n";
        return ret;
    }
    else if (value->name[0] == '@' && value->name[1] == 'a') {
        offset = atoi(value->name + 2);
        if (offset < 8) {
            ret = alloc_registers();
            cout << "  mv " << ret->name << ", a" << offset << '\n';
        }
        else {
            offset = stack_frame->size + (offset-8)*4;
            ret = alloc_registers();
            load_stack2reg(ret, offset, true);
        }
        return ret;
    }
    else {
        ret = alloc_registers();
        offset = get_stack_offset(value->name);
        load_stack2reg(ret, offset, true);
        return ret;
    }
}

void free_registers(Register *_register) {
    if (_register == &register_zero || _register == &temp_register) {
        return;
    }
    if (_register->condition == FREE) {
        printf("warning: free a free register %s\n", _register->name);
        _register->condition = FREE;
        _register->map = "";
        _register->can_evict = 1;
        return;
    }
    _register->condition = FREE;
    _register->map = "";
    _register->can_evict = 1;
    _register->next_avilable = register_free_head;
    register_free_head = _register;
}

// evict a register
void evict_registers() {
    // the allocated register is evicted first
    for (int i = 0; i < REGISTERS_CNT; i++) {
        if (registers[i].can_evict == 0) {
            continue;
        }
        if (registers[i].condition == TEMP) {
            save_reg2stack(&registers[i], get_stack_offset(registers[i].map));
            free_registers(&registers[i]);
            return;
        }
    }
    assert(!"no register can be evicted!\n");
}

void init_registers() {
    register_free_head = NULL;
    char name[4];
    name[0] = 'a', name[2] = '\0';
    for (int i = 0; i < 8; i++) {
        name[1] = '0' + i;
        strcpy(registers[i].name, name);
        registers[i].condition = TEMP;
        free_registers(registers+i);
    }
    name[0] = 't';
    for (int i = 0; i < 6; i++) {
        name[1] = '0' + i;
        strcpy(registers[i+8].name, name);
        registers[i+8].condition = TEMP;
        free_registers(registers+i+8);
    }

    name[0] = 's';
    for (int i = 0; i < VAR_REGISTERS_CNT; i++) {
        if (i < 10) {
            name[1] = '0' + i;
        }
        else {
            name[1] = '1';
            name[2] = '0' + i - 10;
            name[3] = '\0';
        }
        strcpy(var_registers[i].name, name);
        var_registers[i].condition = FREE;
        var_registers[i].map = "";
        var_registers[i].can_evict = 1;
    }
    // t6 is saved for temp use
    strcpy(temp_register.name, "t6");
    temp_register.condition = ALLOCATED;
    temp_register.next_avilable = NULL;
    strcpy(register_zero.name, "x0");
    register_zero.condition = ALLOCATED;
    register_zero.next_avilable = NULL;
}

Register *alloc_registers(const char *map, int condition) {
    assert(map != NULL);
    while (register_free_head && register_free_head->condition != FREE)
    {
        register_free_head = register_free_head->next_avilable;
    }
    if (register_free_head == NULL)
        evict_registers();
    assert(register_free_head != NULL);
    Register * ret = register_free_head;
    register_free_head = register_free_head->next_avilable;
    ret->condition = condition;
    ret->can_evict = 1;
    ret->map = map;
    return ret;
}


///////////////////////
///   stackframe   ////
///////////////////////

void init_stackframe(int size, int field_offset) {
    if (size % 16) {
        printf("stackframe size is not 16 aligned!\n");
        assert(false);
    }
    // now there's only i32 numbers
    stack_frame = new StackFrame;
    stack_frame->allocated = field_offset;
    stack_frame->size = size + stack_frame->allocated;
}
void free_stackframe() {
    delete stack_frame;
}

int alloc_stackframe(const char* map, int size) {
    // printf("alloc_stackframe %s on %d-%d\n", map, stack_frame->allocated, stack_frame->allocated + size);
    stack_frame->map[map] = stack_frame->allocated;
    stack_frame->allocated += size;
    return stack_frame->map[map];
}
int get_stack_offset(const char *map) {
    return stack_frame->map[std::string(map)];
}
int get_stack_offset(const std::string &map) {
    return stack_frame->map[map];
}

void load_stack2reg(Register *reg, int offset, bool load_mem) {
    if (offset > 2044) {
        cout << "  li " << reg->name << ", " << offset << '\n'
             << "  add " << reg->name << ", " << reg->name << ", sp\n";
        if (load_mem) {
            cout << "  lw " << reg->name << ", (" << reg->name << ")\n";
        }
    }
    else {
        if (load_mem) {
            cout << "  lw " << reg->name << ", " << offset << "(sp)\n";
        }
        else {
            cout << "  add " << reg->name << ", sp, " << offset << '\n';
        }
    }
}
void save_reg2stack(Register *reg, int offset) {
    if (offset > 2044) {
        Register *temp_reg = &temp_register;
        cout << "  li " << temp_reg->name << ", " << offset << '\n'
             << "  add " << temp_reg->name << ", " << temp_reg->name << ", sp\n"
             << "  sw " << reg->name << ", (" << temp_reg->name << ")\n";
        free_registers(temp_reg);
    }
    else {
        cout << "  sw " << reg->name << ", " << offset << "(sp)\n";
    }
}