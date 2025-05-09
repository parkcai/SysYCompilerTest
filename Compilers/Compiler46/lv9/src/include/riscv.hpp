#ifndef RISCV_H
#define RISCV_H

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <unordered_map>

#include "koopa.h"

// #define DEBUG
#ifdef DEBUG
#define dbg_printf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbg_printf(...)
#endif

void BuildRiscv(const std::string &koopa_str);
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_global_alloc_t &alloc);
void Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_binary_t &binary);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
void Visit(const koopa_raw_call_t &call);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_get_ptr_t &get_ptr);
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr);
void VisitGlobalAlloc(const koopa_raw_value_t value);
void GetInitVals(const koopa_raw_value_t &init, std::vector<int> &vals);
void Load(const std::string &dest, const koopa_raw_value_t &src);
void Store(const std::string &src, const koopa_raw_value_t &dest);
void Prologue();
void Epilogue();

class StackInfo
{
private:
    std::unordered_map<koopa_raw_value_t, int> val_offset;
    int stk_sz;
    int R;

public:
    /**
     * @brief 分配栈空间
     *
     * @param func
     */
    void alloc(const koopa_raw_function_t &func)
    {
        int S = 0, A = 0;
        auto bbs = func->bbs;
        for (int i = 0; i < bbs.len; ++i)
        {
            auto bb = bbs.buffer[i];
            auto insts = reinterpret_cast<koopa_raw_basic_block_t>(bb)->insts;
            for (int j = 0; j < insts.len; ++j)
            {
                auto inst =
                    reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
                if (inst->kind.tag == KOOPA_RVT_CALL)
                {
                    R = 4;
                    A = std::max(A, (static_cast<int>(inst->kind.data.call.args.len) - 8) * 4);
                }
            }
        }

        for (int i = 0; i < bbs.len; ++i)
        {
            auto bb = bbs.buffer[i];
            auto insts = reinterpret_cast<koopa_raw_basic_block_t>(bb)->insts;
            for (int j = 0; j < insts.len; ++j)
            {
                auto inst =
                    reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
                switch (inst->kind.tag)
                {
                case KOOPA_RVT_LOAD:
                case KOOPA_RVT_BINARY:
                case KOOPA_RVT_GET_PTR:
                case KOOPA_RVT_GET_ELEM_PTR:
                {
                    val_offset[inst] = S + A;
                    S += 4;
                    break;
                }
                case KOOPA_RVT_ALLOC:
                {
                    auto alloced_data = inst->ty->data.pointer.base;
                    auto data_size = size_of_type(alloced_data);
                    val_offset[inst] = S + A;
                    S += data_size;
                    break;
                }
                case KOOPA_RVT_CALL:
                {
                    auto callee = inst->kind.data.call.callee;
                    if (callee->ty->data.function.ret->tag != KOOPA_RTT_UNIT)
                    {
                        val_offset[inst] = S + A;
                        S += 4;
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }

        stk_sz = (S + R + A + 15) & ~15;
    }

    void free(const koopa_raw_function_t &func)
    {
        val_offset.clear();
        stk_sz = 0;
        R = 0;
    }

    bool has_val(const koopa_raw_value_t &value)
    {
        return val_offset.count(value) > 0;
    }

    int offset(const koopa_raw_value_t &value)
    {
        assert(has_val(value));
        return val_offset[value];
    }

    int size()
    {
        return stk_sz;
    }

    int size_of_R()
    {
        return R;
    }

    int size_of_type(koopa_raw_type_t ty)
    {
        switch (ty->tag)
        {
        case KOOPA_RTT_INT32:
            return 4;
        case KOOPA_RTT_UNIT:
            return 0;
        case KOOPA_RTT_ARRAY:
            return ty->data.array.len * size_of_type(ty->data.array.base);
        case KOOPA_RTT_POINTER:
            return 4;
        case KOOPA_RTT_FUNCTION:
            return 0;
        default:
            assert(false);
        }
        return 0;
    }
};

static StackInfo stk;

void BuildRiscv(const std::string &koopa_str)
{
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret =
        koopa_parse_from_string(koopa_str.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    Visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}

// 访问 raw program
// typedef struct
// {
//     /// Global values (global allocations only).
//     koopa_raw_slice_t values;
//     /// Function definitions.
//     koopa_raw_slice_t funcs;
// } koopa_raw_program_t;
void Visit(const koopa_raw_program_t &program)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有全局变量
    Visit(program.values);
    // 访问所有函数
    Visit(program.funcs);
}

// 访问 raw slice
// typedef struct
// {
//     /// Buffer of slice items.
//     const void **buffer;
//     /// Length of slice.
//     uint32_t len;
//     /// Kind of slice items.
//     koopa_raw_slice_item_kind_t kind;
// } koopa_raw_slice_t;
void Visit(const koopa_raw_slice_t &slice)
{
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_UNKNOWN:
            // dbg_printf("koopa_raw_slice_t kind = KOOPA_RSIK_UNKNOWN\n");
            break;
        case KOOPA_RSIK_TYPE:
            // 访问类型
            // dbg_printf("koopa_raw_slice_t kind = KOOPA_RSIK_TYPE\n");
            break;
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            // dbg_printf("koopa_raw_slice_t kind = KOOPA_RSIK_FUNCTION\n");
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            // dbg_printf("koopa_raw_slice_t kind = KOOPA_RSIK_BASIC_BLOCK\n");
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            // dbg_printf("koopa_raw_slice_t kind = KOOPA_RSIK_VALUE\n");
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
// typedef struct
// {
//     /// Type of function.
//     koopa_raw_type_t ty;
//     /// Name of function.
//     const char *name;
//     /// Parameters.
//     koopa_raw_slice_t params;
//     /// Basic blocks, empty if is a function declaration.
//     koopa_raw_slice_t bbs;
// } koopa_raw_function_data_t;
void Visit(const koopa_raw_function_t &func)
{
    dbg_printf("function name = %s\n", func->name);
    // 执行一些其他的必要操作
    if (func->bbs.len == 0)
    {
        return;
    }
    // lw和sw也要注意立即数的范围
    std::cout << "  .text" << std::endl;
    std::cout << "  .globl " << func->name + 1 << std::endl;
    std::cout << func->name + 1 << ":" << std::endl;
    // 扫描函数中的所有指令, 算出需要分配的栈空间总量S
    stk.alloc(func);
    Prologue();
    Visit(func->bbs);
    // 释放栈帧
    stk.free(func);
}

// 访问基本块
// typedef struct
// {
//     /// Name of basic block, null if no name.
//     const char *name;
//     /// Parameters.
//     koopa_raw_slice_t params;
//     /// Values that this basic block is used by.
//     koopa_raw_slice_t used_by;
//     /// Instructions in this basic block.
//     koopa_raw_slice_t insts;
// } koopa_raw_basic_block_data_t;
void Visit(const koopa_raw_basic_block_t &bb)
{
    dbg_printf("basic_block name = %s\n", bb->name);
    // 执行一些其他的必要操作
    if (strcmp(bb->name + 1, "entry"))
    {
        std::cout << bb->name + 1 << ":" << std::endl;
    }
    // 访问所有指令
    Visit(bb->insts);
}

// 访问指令
// struct koopa_raw_value_data
// {
//     /// Type of value.
//     koopa_raw_type_t ty;
//     /// Name of value, null if no name.
//     const char *name;
//     /// Values that this value is used by.
//     koopa_raw_slice_t used_by;
//     /// Kind of value.
//     koopa_raw_value_kind_t kind;
// };
void Visit(const koopa_raw_value_t &value)
{
    // switch (value->kind.tag)
    // {
    // case KOOPA_RTT_INT32:
    //     dbg_printf("instruction kind tag = KOOPA_RTT_INT32\n");
    //     break;
    // case KOOPA_RTT_UNIT:
    //     dbg_printf("instruction kind tag = KOOPA_RTT_UNIT\n");
    //     break;
    // case KOOPA_RTT_ARRAY:
    //     dbg_printf("instruction kind tag = KOOPA_RTT_ARRAY\n");
    //     break;
    // case KOOPA_RTT_POINTER:
    //     dbg_printf("instruction kind tag = KOOPA_RTT_POINTER\n");
    //     break;
    // case KOOPA_RTT_FUNCTION:
    //     dbg_printf("instruction kind tag = KOOPA_RTT_FUNCTION\n");
    //     break;
    // }
    // dbg_printf("instruction name = %s\n", value->name);
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        dbg_printf("value kind = KOOPA_RVT_INTEGER\n");
        break;
    case KOOPA_RVT_ZERO_INIT:
        // 访问 zero_init 指令
        dbg_printf("value kind = KOOPA_RVT_ZERO_INIT\n");
        break;
    case KOOPA_RVT_UNDEF:
        // 访问 undef 指令
        dbg_printf("value kind = KOOPA_RVT_UNDEF\n");
        break;
    case KOOPA_RVT_AGGREGATE:
        // 访问 aggregate 指令
        dbg_printf("value kind = KOOPA_RVT_AGGREGATE\n");
        break;
    case KOOPA_RVT_FUNC_ARG_REF:
        // 访问 func_arg_ref 指令
        dbg_printf("value kind = KOOPA_RVT_FUNC_ARG_REF\n");
        break;
    case KOOPA_RVT_BLOCK_ARG_REF:
        // 访问 block_arg_ref 指令
        dbg_printf("value kind = KOOPA_RVT_BLOCK_ARG_REF\n");
        break;
    case KOOPA_RVT_ALLOC:
        // 访问 alloc 指令
        dbg_printf("value kind = KOOPA_RVT_ALLOC\n");
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global_alloc 指令
        dbg_printf("value kind = KOOPA_RVT_GLOBAL_ALLOC\n");
        VisitGlobalAlloc(value);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        dbg_printf("value kind = KOOPA_RVT_LOAD\n");
        Visit(kind.data.load);
        Store("t0", value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        dbg_printf("value kind = KOOPA_RVT_STORE\n");
        Visit(kind.data.store);
        break;
    case KOOPA_RVT_GET_PTR:
        // 访问 get_ptr 指令
        dbg_printf("value kind = KOOPA_RVT_GET_PTR\n");
        Visit(kind.data.get_ptr);
        Store("t0", value);
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        // 访问 get_elem_ptr 指令
        dbg_printf("value kind = KOOPA_RVT_GET_ELEM_PTR\n");
        Visit(kind.data.get_elem_ptr);
        Store("t0", value);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        dbg_printf("value kind = KOOPA_RVT_BINARY\n");
        Visit(kind.data.binary);
        Store("t0", value);
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 branch 指令
        dbg_printf("value kind = KOOPA_RVT_BRANCH\n");
        Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令
        dbg_printf("value kind = KOOPA_RVT_JUMP\n");
        Visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
    {
        // 访问 call 指令
        dbg_printf("value kind = KOOPA_RVT_CALL\n");
        Visit(kind.data.call);
        auto callee = kind.data.call.callee;
        if (callee->ty->data.function.ret->tag != KOOPA_RTT_UNIT)
        {
            Store("a0", value);
        }
        break;
    }
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        Visit(kind.data.ret);
        break;

    default:
        // 其他类型暂时遇不到
        assert(false);
    }
    // std::cout << "----------------------------------------" << std::endl;
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load)
{
    switch (load.src->kind.tag)
    {
    case KOOPA_RVT_GLOBAL_ALLOC:
    case KOOPA_RVT_ALLOC:
    {
        Load("t0", load.src);
        break;
    }
    case KOOPA_RVT_GET_ELEM_PTR:
    case KOOPA_RVT_GET_PTR:
    {
        auto offset = stk.offset(load.src);
        if (offset > 2047)
        {
            std::cout << "  li t3, " << offset << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  lw t3, 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  lw t3, " << offset << "(sp)" << std::endl;
        }
        std::cout << "  lw t0, 0(t3)" << std::endl;
        break;
    }
    default:
    {
        assert(false);
    }
    }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store)
{
    Load("t0", store.value);
    switch (store.dest->kind.tag)
    {
    case KOOPA_RVT_GLOBAL_ALLOC:
    case KOOPA_RVT_ALLOC:
    {
        Store("t0", store.dest);
        break;
    }
    case KOOPA_RVT_GET_ELEM_PTR:
    case KOOPA_RVT_GET_PTR:
    {
        auto offset = stk.offset(store.dest);
        if (offset > 2047)
        {
            std::cout << "  li t3, " << offset << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  lw t3, 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  lw t3, " << offset << "(sp)" << std::endl;
        }
        std::cout << "  sw t0, 0(t3)" << std::endl;
        break;
    }
    default:
    {
        assert(false);
    }
    }
}

// 访问二元运算
void Visit(const koopa_raw_binary_t &binary)
{
    Load("t0", binary.lhs);
    Load("t1", binary.rhs);

    switch (binary.op)
    {
    case KOOPA_RBO_NOT_EQ:
        std::cout << "  sub t0, t0, t1" << std::endl;
        std::cout << "  snez t0, t0" << std::endl;
        break;
    case KOOPA_RBO_EQ:
        std::cout << "  sub t0, t0, t1" << std::endl;
        std::cout << "  seqz t0, t0" << std::endl;
        break;
    case KOOPA_RBO_GT:
        std::cout << "  sgt t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_LT:
        std::cout << "  slt t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_GE:
        std::cout << "  sub t0, t0, t1" << std::endl;
        std::cout << "  sgt t1, t0, x0" << std::endl;
        std::cout << "  seqz t0, t0" << std::endl;
        std::cout << "  or t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_LE:
        std::cout << "  sub t0, t0, t1" << std::endl;
        std::cout << "  slt t1, t0, x0" << std::endl;
        std::cout << "  seqz t0, t0" << std::endl;
        std::cout << "  or t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_ADD:
        std::cout << "  add t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SUB:
        std::cout << "  sub t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_MUL:
        std::cout << "  mul t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_DIV:
        std::cout << "  div t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_MOD:
        std::cout << "  rem t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_AND:
        std::cout << "  and t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_OR:
        std::cout << "  or t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_XOR:
        std::cout << "  xor t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SHL:
        std::cout << "  sll t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SHR:
        std::cout << "  srl t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SAR:
        std::cout << "  sra t0, t0, t1" << std::endl;
        break;
    default:
        assert(false);
    }
}

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch)
{
    if (branch.cond->kind.tag == KOOPA_RVT_INTEGER)
    {
        if (branch.cond->kind.data.integer.value == 0)
        {
            std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
        }
        else
        {
            std::cout << "  j " << branch.true_bb->name + 1 << std::endl;
        }
        return;
    }
    auto offset = stk.offset(branch.cond);
    if (offset > 2047)
    {
        std::cout << "  li t1, " << offset << std::endl;
        std::cout << "  add t1, sp, t1" << std::endl;
        std::cout << "  lw t0, 0(t1)" << std::endl;
    }
    else
    {
        std::cout << "  lw t0, " << offset << "(sp)" << std::endl;
    }
    std::cout << "  bnez t0, " << branch.true_bb->name + 1 << std::endl;
    std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
}

// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump)
{
    std::cout << "  j " << jump.target->name + 1 << std::endl;
}

// 访问 call 指令
void Visit(const koopa_raw_call_t &call)
{
    for (int i = 0; i < std::min(8, static_cast<int>(call.args.len)); ++i)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        Load("a" + std::to_string(i), arg);
    }

    for (int i = 8; i < static_cast<int>(call.args.len); ++i)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        Load("t0", arg);
        auto offset = (i - 8) * 4;
        if (offset > 2047)
        {
            std::cout << "  li t1, " << offset << std::endl;
            std::cout << "  add t1, sp, t1" << std::endl;
            std::cout << "  sw t0, 0(t1)" << std::endl;
        }
        else
        {
            std::cout << "  sw t0, " << (i - 8) * 4 << "(sp)" << std::endl;
        }
    }

    std::cout << "  call " << call.callee->name + 1 << std::endl;
}

void Visit(const koopa_raw_return_t &ret)
{
    // return 指令中, value 代表返回值
    koopa_raw_value_t ret_value = ret.value;

    if (ret_value != nullptr)
    {
        Load("a0", ret_value);
    }

    Epilogue();
}

void Visit(const koopa_raw_get_ptr_t &get_ptr)
{
    if (get_ptr.index->kind.tag == KOOPA_RVT_INTEGER)
    {
        auto elem_index = get_ptr.index->kind.data.integer.value;
        switch (get_ptr.src->kind.tag)
        {
        case KOOPA_RVT_GLOBAL_ALLOC:
        {
            std::cout << "  la t0, " << get_ptr.src->name + 1 << std::endl;
            auto elem_size = stk.size_of_type(get_ptr.src->ty->data.pointer.base);
            auto elem_offset = elem_size * elem_index;
            if (elem_offset > 2047)
            {
                std::cout << "  li t1, " << elem_offset << std::endl;
                std::cout << "  add t0, t0, t1" << std::endl;
            }
            else
            {
                std::cout << "  addi t0, t0, " << elem_offset << std::endl;
            }
            break;
        }
        case KOOPA_RVT_LOAD:
        case KOOPA_RVT_ALLOC:
        case KOOPA_RVT_GET_PTR:
        case KOOPA_RVT_GET_ELEM_PTR:
        {
            int offset = stk.offset(get_ptr.src);
            if (offset > 2047)
            {
                std::cout << "  li t0, " << offset << std::endl;
                std::cout << "  add t0, sp, t0" << std::endl;
                std::cout << "  lw t0, 0(t0)" << std::endl;
            }
            else
            {
                std::cout << "  lw t0, " << offset << "(sp)" << std::endl;
            }
            auto elem_size = stk.size_of_type(get_ptr.src->ty->data.pointer.base);
            auto elem_offset = elem_size * elem_index;
            if (elem_offset > 2047)
            {
                std::cout << "  li t1, " << elem_offset << std::endl;
                std::cout << "  add t0, t0, t1" << std::endl;
            }
            else
            {
                std::cout << "  addi t0, t0, " << elem_offset << std::endl;
            }
            break;
        }
        default:
        {
            assert(false);
        }
        }
    }
    else
    {
        auto index_offset = stk.offset(get_ptr.index);
        if (index_offset > 2047)
        {
            std::cout << "  li t3, " << index_offset << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  lw t3, 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  lw t3, " << index_offset << "(sp)" << std::endl;
        }
        auto elem_size = stk.size_of_type(get_ptr.src->ty->data.pointer.base);
        std::cout << "  li t2, " << elem_size << std::endl;
        std::cout << "  mul t3, t3, t2" << std::endl;
        switch (get_ptr.src->kind.tag)
        {
        case KOOPA_RVT_GLOBAL_ALLOC:
        {
            std::cout << "  la t0, " << get_ptr.src->name + 1 << std::endl;
            std::cout << "  add t0, t0, t3" << std::endl;
            break;
        }
        case KOOPA_RVT_LOAD:
        case KOOPA_RVT_ALLOC:
        case KOOPA_RVT_GET_PTR:
        case KOOPA_RVT_GET_ELEM_PTR:
        {
            auto offset = stk.offset(get_ptr.src);
            if (offset > 2047)
            {
                std::cout << "  li t0, " << offset << std::endl;
                std::cout << "  add t0, sp, t0" << std::endl;
                std::cout << "  lw t0, 0(t0)" << std::endl;
            }
            else
            {
                std::cout << "  lw t0, " << offset << "(sp)" << std::endl;
            }
            std::cout << "  add t0, t0, t3" << std::endl;
            break;
        }
        default:
        {
            assert(false);
        }
        }
    }
}

void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr)
{
    if (get_elem_ptr.index->kind.tag == KOOPA_RVT_INTEGER)
    {
        auto elem_index = get_elem_ptr.index->kind.data.integer.value;
        switch (get_elem_ptr.src->kind.tag)
        {
        case KOOPA_RVT_GLOBAL_ALLOC:
        {
            std::cout << "  la t0, " << get_elem_ptr.src->name + 1 << std::endl;
            auto elem_size = stk.size_of_type(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
            auto elem_offset = elem_size * elem_index;
            if (elem_offset > 2047)
            {
                std::cout << "  li t1, " << elem_offset << std::endl;
                std::cout << "  add t0, t0, t1" << std::endl;
            }
            else
            {
                std::cout << "  addi t0, t0, " << elem_offset << std::endl;
            }
            break;
        }
        case KOOPA_RVT_ALLOC:
        {
            auto inst_src_offset = stk.offset(get_elem_ptr.src);
            auto elem_size = stk.size_of_type(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
            auto inst_dest_offset = inst_src_offset + elem_size * elem_index;
            if (inst_dest_offset > 2047)
            {
                std::cout << "  li t0, " << inst_dest_offset << std::endl;
                std::cout << "  add t0, sp, t0" << std::endl;
            }
            else
            {
                std::cout << "  addi t0, sp, " << inst_dest_offset << std::endl;
            }
            break;
        }
        case KOOPA_RVT_GET_ELEM_PTR:
        case KOOPA_RVT_GET_PTR:
        {
            auto offset = stk.offset(get_elem_ptr.src);
            if (offset > 2047)
            {
                std::cout << "  li t1, " << offset << std::endl;
                std::cout << "  add t1, sp, t1" << std::endl;
                std::cout << "  lw t1, 0(t1)" << std::endl;
            }
            else
            {
                std::cout << "  lw t1, " << offset << "(sp)" << std::endl;
            }
            auto elem_size = stk.size_of_type(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
            auto elem_offset = elem_size * elem_index;
            if (elem_offset > 2047)
            {
                std::cout << "  li t2, " << elem_offset << std::endl;
                std::cout << "  add t0, t1, t2" << std::endl;
            }
            else
            {
                std::cout << "  addi t0, t1, " << elem_offset << std::endl;
            }
            break;
        }
        default:
        {
            assert(false);
        }
        }
    }
    else
    {
        auto index_offset = stk.offset(get_elem_ptr.index);
        if (index_offset > 2047)
        {
            std::cout << "  li t3, " << index_offset << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  lw t3, 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  lw t3, " << index_offset << "(sp)" << std::endl;
        }
        auto elem_size = stk.size_of_type(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
        std::cout << "  li t2, " << elem_size << std::endl;
        std::cout << "  mul t3, t3, t2" << std::endl;
        switch (get_elem_ptr.src->kind.tag)
        {
        case KOOPA_RVT_GLOBAL_ALLOC:
        {
            std::cout << "  la t0, " << get_elem_ptr.src->name + 1 << std::endl;
            std::cout << "  add t0, t0, t3" << std::endl;
            break;
        }
        case KOOPA_RVT_ALLOC:
        {
            auto offset = stk.offset(get_elem_ptr.src);
            std::cout << "  add t0, t3, " << offset << std::endl;
            std::cout << "  add t0, sp, t0" << std::endl;
            break;
        }
        case KOOPA_RVT_GET_ELEM_PTR:
        case KOOPA_RVT_GET_PTR:
        {
            auto offset = stk.offset(get_elem_ptr.src);
            if (offset > 2047)
            {
                std::cout << "  li t1, " << offset << std::endl;
                std::cout << "  add t1, sp, t1" << std::endl;
                std::cout << "  lw t1, 0(t1)" << std::endl;
            }
            else
            {
                std::cout << "  lw t1, " << offset << "(sp)" << std::endl;
            }
            std::cout << "  add t0, t1, t3" << std::endl;
            break;
        }
        default:
        {
            assert(false);
        }
        }
    }
}
// // 访问全局变量
// void Visit(const koopa_raw_global_alloc_t &alloc)
// {
//     // 访问全局变量
//     // dbg_printf("global_alloc name = %s\n", alloc->name);

//     // if (alloc.init->kind.tag == KOOPA_RVT_INTEGER)
//     // {
//     //     std::cout << "  .word " << alloc.init->kind.data.integer.value << std::endl;
//     // }
//     // else
//     // {
//     //     assert(alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT);
//     //     std::cout << "  .zero 4" << std::endl;
//     // }
//     switch (alloc.init->kind.tag)
//     {
//     case KOOPA_RVT_INTEGER:
//         std::cout << "  .word " << alloc.init->kind.data.integer.value << std::endl;
//         break;
//     case KOOPA_RVT_ZERO_INIT:
//         std::cout << "  .zero " <<
//     }
//     std::cout << std::endl;
// }

void VisitGlobalAlloc(const koopa_raw_value_t value)
{
    std::cout << "  .data" << std::endl;
    std::cout << "  .globl " << value->name + 1 << std::endl;
    std::cout << value->name + 1 << ":" << std::endl;
    auto init = value->kind.data.global_alloc.init;
    auto data_type = value->ty->data.pointer.base;
    switch (init->kind.tag)
    {
    case KOOPA_RVT_ZERO_INIT:
    {
        std::cout << "  .zero " << stk.size_of_type(init->ty) << std::endl;
        break;
    }
    case KOOPA_RVT_INTEGER:
    {
        std::cout << "  .word " << init->kind.data.integer.value << std::endl;
        break;
    }
    case KOOPA_RVT_AGGREGATE:
    {
        std::vector<int> init_vals;
        GetInitVals(init, init_vals);
        int zero_cnt = 0;
        for (auto &val : init_vals)
        {
            if (val == 0)
            {
                zero_cnt++;
            }
            else
            {
                if (zero_cnt > 0)
                {
                    std::cout << "  .zero " << zero_cnt * 4 << std::endl;
                    zero_cnt = 0;
                }
                std::cout << "  .word " << val << std::endl;
            }
        }
        if (zero_cnt > 0)
        {
            std::cout << "  .zero " << zero_cnt * 4 << std::endl;
        }
        break;
    }
    default:
    {
        assert(false);
    }
    }
    std::cout << std::endl;
}

void GetInitVals(const koopa_raw_value_t &init, std::vector<int> &vals)
{
    koopa_raw_slice_t elems = init->kind.data.aggregate.elems;
    for (int i = 0; i < elems.len; ++i)
    {
        auto elem = reinterpret_cast<koopa_raw_value_t>(elems.buffer[i]);
        switch (elem->kind.tag)
        {
        case KOOPA_RVT_INTEGER:
        {
            vals.push_back(elem->kind.data.integer.value);
            break;
        }
        case KOOPA_RVT_AGGREGATE:
        {
            GetInitVals(elem, vals);
            break;
        }
        default:
        {
            assert(false);
        }
        }
    }
}

void Load(const std::string &dest, const koopa_raw_value_t &src)
{
    switch (src->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
    {
        std::cout << "  li " << dest << ", " << src->kind.data.integer.value << std::endl;
        break;
    }
    case KOOPA_RVT_FUNC_ARG_REF:
    {
        auto index = src->kind.data.func_arg_ref.index;
        if (index < 8)
        {
            std::cout << "  mv " << dest << ", a" << index << std::endl;
        }
        else
        {
            auto offset = stk.size() + (index - 8) * 4;
            if (offset > 2047)
            {
                std::cout << "  li " << dest << ", " << offset << std::endl;
                std::cout << "  add " << dest << ", sp, " << dest << std::endl;
                std::cout << "  lw " << dest << ", 0(" << dest << ")" << std::endl;
            }
            else
            {
                std::cout << "  lw " << dest << ", " << offset << "(sp)" << std::endl;
            }
        }
        break;
    }
    case KOOPA_RVT_GLOBAL_ALLOC:
    {
        std::cout << "  la " << dest << ", " << src->name + 1 << std::endl;
        std::cout << "  lw " << dest << ", 0(" << dest << ")" << std::endl;
        break;
    }
    default:
    {
        auto offset = stk.offset(src);
        if (offset > 2047)
        {
            std::cout << "  li " << dest << ", " << offset << std::endl;
            std::cout << "  add " << dest << ", sp, " << dest << std::endl;
            std::cout << "  lw " << dest << ", 0(" << dest << ")" << std::endl;
        }
        else
        {
            std::cout << "  lw " << dest << ", " << stk.offset(src) << "(sp)" << std::endl;
        }
        break;
    }
    }
}

void Store(const std::string &src, const koopa_raw_value_t &dest)
{
    switch (dest->kind.tag)
    {
    case KOOPA_RVT_GLOBAL_ALLOC:
    {
        std::cout << "  la t3, " << dest->name + 1 << std::endl;
        std::cout << "  sw " << src << ", 0(t3)" << std::endl;
        break;
    }
    default:
    {
        auto offset = stk.offset(dest);
        if (offset > 2047)
        {
            std::cout << "  li t3, " << offset << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  sw " << src << ", 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  sw " << src << ", " << offset << "(sp)" << std::endl;
        }
        break;
    }
    }
}

void Prologue()
{
    // 分配栈帧，当立即数位于[-2048, 2047]时，使用addi指令，否则使用li指令和add指令
    if (stk.size() > 2047)
    {
        std::cout << "  li t3, " << stk.size() << std::endl;
        std::cout << "  sub sp, sp, t3" << std::endl;
    }
    else if (stk.size() > 0)
    {
        std::cout << "  addi sp, sp, -" << stk.size() << std::endl;
    }

    if (stk.size_of_R())
    {
        if (stk.size() - 4 > 2047)
        {
            std::cout << "  li t3, " << stk.size() - 4 << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  sw ra, 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  sw ra, " << stk.size() - 4 << "(sp)" << std::endl;
        }
    }
}

void Epilogue()
{
    if (stk.size_of_R())
    {
        if (stk.size() - 4 > 2047)
        {
            std::cout << "  li t3, " << stk.size() - 4 << std::endl;
            std::cout << "  add t3, sp, t3" << std::endl;
            std::cout << "  lw ra, 0(t3)" << std::endl;
        }
        else
        {
            std::cout << "  lw ra, " << stk.size() - 4 << "(sp)" << std::endl;
        }
    }

    if (stk.size() > 2047)
    {
        std::cout << "  li t3, " << stk.size() << std::endl;
        std::cout << "  add sp, sp, t3" << std::endl;
    }
    else if (stk.size() > 0)
    {
        std::cout << "  addi sp, sp, " << stk.size() << std::endl;
    }

    std::cout << "  ret" << std::endl;
    std::cout << std::endl;
}

#endif

// TODO: (sp)判断封装成函数