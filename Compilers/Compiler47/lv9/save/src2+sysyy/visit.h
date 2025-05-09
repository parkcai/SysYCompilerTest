#pragma once

#include "koopa.h"
#include <fstream>
#include <assert.h>

using std::ofstream;
extern ofstream out;
void visit(const koopa_raw_program_t &program);
void visit(const koopa_raw_slice_t &values);
void visit(koopa_raw_function_t func);
void visit(koopa_raw_basic_block_t bb);
void visit(koopa_raw_value_t value);
void visit(const koopa_raw_program_t &program)
{
    visit(program.values);
    out << ".text\n";
    visit(program.funcs);
}
void visit(const koopa_raw_slice_t &values)
{
    for (size_t i = 0; i < values.len; ++i)
    {
        if (values.kind == KOOPA_RSIK_FUNCTION)
            visit((koopa_raw_function_t)values.buffer[i]);
        else
        LINE;
    }
}
void visit(koopa_raw_function_t func)
{
    out << ".globl " << func->name+1 << '\n';
    bug(func->bbs.len);
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
    LINE;
        visit((koopa_raw_basic_block_t)func->bbs.buffer[i]);
    }
}
void visit(koopa_raw_basic_block_t bb)
{
    LINE;
    out << bb->name+1 << ":\n";
    //out<<"main:\n";
    //out<<"a1:\n";
    LINE;
    for (size_t i = 0; i < bb->insts.len; ++i)
    {
        visit((koopa_raw_value_t)bb->insts.buffer[i]);
    }
}
void visit(koopa_raw_value_t value)
{
    assert(value->kind.tag == KOOPA_RVT_RETURN);
    koopa_raw_value_t ret_value = value->kind.data.ret.value;
    // 示例程序中, ret_value 一定是一个 integer
    assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
    // 于是我们可以按照处理 integer 的方式处理 ret_value
    // integer 中, value 代表整数的数值
    int32_t int_val = ret_value->kind.data.integer.value;
    // 示例程序中, 这个数值一定是 0
    out << "li a0, " << int_val << '\n';
    out <<  "ret\n";
}