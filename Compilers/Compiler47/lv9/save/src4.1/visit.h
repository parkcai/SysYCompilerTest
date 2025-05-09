#pragma once

#include "koopa.h"
#include <fstream>
#include <assert.h>
#include <map>

using std::ofstream, std::map, std::cout,std::cerr;

#define debug(x) out<<__LINE__<<' '<<x<<'\n';
#define LINE out<<__LINE__<<std::endl;
extern ofstream out;
map<const koopa_raw_value_t, int> value_map;
std::string reg_names[16] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                             "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0"
                             };
static int cnt_reg = 0;
int reg_state[16];//0 is temp
int get_reg_num(string name)
{
    for (int i = 0; i < 16; i++)
        if (reg_names[i] == name)
            return i;
    return -1;
}
int get_reg()
{
    for (int i = 0; i < 16; i++)
        if (reg_state[i] == 0)
            {reg_state[i] = 1;return i;}
    out<<"寄存器用完了\n";
    assert(0);
    return -1;
}
void visit(const koopa_raw_program_t &program);
void visit(const koopa_raw_slice_t &values);
void visit(koopa_raw_function_t func);
void visit(koopa_raw_basic_block_t bb);
int visit(koopa_raw_value_t value);
void visit(const koopa_raw_return_t &ret);
int visit(const koopa_raw_integer_t &integer);
int visit(const koopa_raw_binary_t &binary);

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
    out << ".globl " << func->name + 1 << '\n';
    bug(func->bbs.len);
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        visit((koopa_raw_basic_block_t)func->bbs.buffer[i]);
        //out<<"bbs.buffer["<<i<<"] is ok\n";
    }
}
void visit(koopa_raw_basic_block_t bb)
{
    out << bb->name + 1 << ":\n";
    for (size_t i = 0; i < bb->insts.len; ++i)
    {
        visit((koopa_raw_value_t)bb->insts.buffer[i]);
        //out<<"insts.buffer["<<i<<"] is ok\n";
    }
    //cout<<__LINE__<<bb->insts.len<<'\n';
}
int visit(koopa_raw_value_t value)
{
    if(value_map.count(value))
        return value_map[value];
    int reg= 0; // = cnt_reg++;
    auto kind = value->kind;
    //debug(value->kind.tag);
    switch (value->kind.tag)
    {
    case KOOPA_RVT_RETURN:
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        reg = visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        reg = visit(kind.data.binary);
        value_map[value] = reg;
        break;
    }  
    //LINE;
    return reg;
    cout<<__LINE__<<'\n';
    koopa_raw_value_t ret_value = value->kind.data.ret.value;
    // 示例程序中, ret_value 一定是一个 integer
    assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
    // 于是我们可以按照处理 integer 的方式处理 ret_value
    // integer 中, value 代表整数的数值
    int32_t int_val = ret_value->kind.data.integer.value;
    // 示例程序中, 这个数值一定是 0
    out << "li a0, " << int_val << '\n';
    out << "ret\n";
}
void visit(const koopa_raw_return_t &ret)
{
    int reg = visit(ret.value);
    if(reg!=7)
        out << "\tmv a0, " << reg_names[reg] <<'\n';
    out << "\tret" << '\n';
}
int visit(const koopa_raw_binary_t &binary)
{
    int left = visit(binary.lhs);
    int right = visit(binary.rhs);
    int new_reg = get_reg();
    std::string left_name = reg_names[left];
    std::string right_name = reg_names[right];
    std::string result_name = reg_names[new_reg];
    //debug(left_name+right_name+result_name);
    switch (binary.op)
    {
    case 0:  // ne
        out << "\txor   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        out << "\tsnez  " << result_name << ", " << result_name <<
            std::endl;
        break;
    case 1:  // eq
        out << "\txor   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        out << "\tseqz  " << result_name << ", " << result_name <<
            std::endl;
        break;
    case 2:  // gt
        out << "\tsgt   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 3:  // lt
      
        out << "\tslt   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
           
        break;
    case 4:  // ge
        out << "\tslt   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        out << "\txori  " << result_name << ", " << result_name << ", 1"
            << std::endl;
        break;
    case 5:  // le
        out << "\tsgt   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        out << "\txori  " << result_name << ", " << result_name << ", 1"
            << std::endl;
        break;
    case 6:  // add
        out << "\tadd   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 7:  // sub
        out << "\tsub   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 8:  // mul
        out << "\tmul   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 9:  // div
        out << "\tdiv   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 10:  // mod
        out << "\trem   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 11:  // and
        out << "\tand   " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    case 12:  // or
        out << "\tor    " << result_name << ", " << left_name << ", " <<
            right_name << std::endl;
        break;
    }
    return new_reg;
}
int visit(const koopa_raw_integer_t &integer)
{
    int int_val = integer.value;
    int reg;
    // if(int_val == 0)
    // {    reg = 15;
    // return reg;
    // }
    reg = get_reg();
    out<<"\tli  "<<reg_names[reg]<<", "<<int_val<<'\n';
    return reg;
}