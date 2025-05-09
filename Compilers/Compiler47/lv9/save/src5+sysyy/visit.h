#pragma once

#include "koopa.h"
#include <fstream>
#include <assert.h>
#include <map>

using std::ofstream, std::map, std::cout,std::cerr;

#define debug(x) out<<__LINE__<<' '<<x<<'\n';
#define LINE out<<__LINE__<<std::endl;
#define C cout<<__LINE__<<endl;
extern ofstream out;
map<const koopa_raw_value_t, int> value_map;
std::string reg_names[16] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                             "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0"
                             };
static int cnt_pos = -4;
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
int visit(const koopa_raw_load_t &load);
void visit(const koopa_raw_store_t &store);

void visit(const koopa_raw_program_t &program)
{
    visit(program.values);
    out << ".text\n"<<flush;
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
    out << ".globl " << func->name + 1 << '\n'<<flush;
    out << "   addi sp, sp, -256"<<endl;
    bug(func->bbs.len);
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        visit((koopa_raw_basic_block_t)func->bbs.buffer[i]);
        //out<<"bbs.buffer["<<i<<"] is ok\n";
    }
}
void visit(koopa_raw_basic_block_t bb)
{
    out << bb->name + 1 << ":\n"<<flush;
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
    int pos = 0;
    auto kind = value->kind;
    //debug(value->kind.tag);
    C
    cout<<' '<<value->kind.tag<<'\n';
    switch (value->kind.tag)
    {
    case KOOPA_RVT_RETURN:
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        pos = visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        pos = visit(kind.data.binary);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_ALLOC:
        pos = cnt_pos+=4;
        //assert(value->ty->tag == KOOPA_RTT_POINTER);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_LOAD:
        pos = visit(kind.data.load);
        C
        value_map[value] = pos;
        break;
    case KOOPA_RVT_STORE:
        visit(kind.data.store);
        break;
    default:
        assert(false);
    }  
    cout<<value->kind.tag<<endl;
    cout<<__LINE__<<endl;
    return pos;
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
    int pos = visit(ret.value);
    out << "\tlw a0, " << pos <<"(sp)\n"<<flush;
    out<< "\taddi sp, sp, 256"<<endl;
    out << "\tret" << '\n'<<flush;
}
int visit(const koopa_raw_binary_t &binary)
{
    int left = visit(binary.lhs);
    int right = visit(binary.rhs);
    int new_pos = cnt_pos+=4;
    std::string left_name = reg_names[0];
    out << "\tlw "<<left_name<<", "<<left<<"(sp)\n"<<flush;
    std::string right_name = reg_names[1];
    out << "\tlw "<<right_name<<", "<<right<<"(sp)\n"<<flush;
    std::string result_name = reg_names[2];
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
    default:
        assert(false);
    }
    out<<"\tsw "<< result_name <<", "<<new_pos<<"(sp)\n"<<flush;
    return new_pos;
}
int visit(const koopa_raw_integer_t &integer)
{
    int int_val = integer.value;
    int pos= cnt_pos+=4;
    out<<"\tli  "<<reg_names[0]<<", "<<int_val<<'\n'<<flush;
    out<<"\tsw  "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
    return pos;
}
int visit(const koopa_raw_load_t &load)
{
    koopa_raw_value_t src = load.src;
    int pos = value_map[src];
    int new_pos = cnt_pos+=4;
    out<<"\tlw    "<<reg_names[0]<<", "<<pos<<"(sp)\n"<<flush;
    out<<"\tsw   "<< reg_names[0] <<", "<<new_pos<<"(sp)\n"<<flush;
    return new_pos;
}
void visit(const koopa_raw_store_t &store)
{
    int pos = visit(store.value);
    cout<<pos<<' '<<cnt_pos<<endl;
    koopa_raw_value_t dest = store.dest;
    int store_pos = value_map[dest];

    out<<"\tlw    "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
    out<<"\tsw    "<< reg_names[0] <<", "<<store_pos<<"(sp)\n"<<flush;
}