#pragma once
//注意栈参数被破坏
#include "koopa.h"
#include <fstream>
#include <assert.h>
#include <map>
#include "math.h"
using std::ofstream, std::map, std::cout,std::cerr;

#define debug(x) cout<<__LINE__<<' '<<x<<'\n';
#define LINE out<<__LINE__<<std::endl;
#define C //cout<<__LINE__<<endl;
extern ofstream out;
map<const koopa_raw_value_t, int> value_map;
std::string reg_names[17] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                             "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0", "temp"
                             };
static int cnt_pos = 400;
int reg_state[16];//0 is temp
int name_num = 0;
std::map<const koopa_raw_value_t, std::string> global_values;
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
void visit(koopa_raw_basic_block_t bb, bool with_addi = false);
int visit(koopa_raw_value_t value);
void visit(const koopa_raw_return_t &ret);
int visit(const koopa_raw_integer_t &integer);
int visit(const koopa_raw_binary_t &binary);
int visit(const koopa_raw_load_t &load);
void visit(const koopa_raw_store_t &store);
void visit(const koopa_raw_branch_t &branch);
void visit(const koopa_raw_jump_t &jump);
int visit(const koopa_raw_call_t &call);
int visit(const koopa_raw_func_arg_ref_t &ret);
string visit(const koopa_raw_global_alloc_t&global);



void move(int before,int after)
{
    /*debug*/ out<<"\t#moving"<<endl;
    out<<"\tlw    "<<reg_names[0]<<", "<<before<<"(sp)\n"<<flush;
    out<<"\tsw   "<< reg_names[0] <<", "<<after<<"(sp)\n"<<flush;
}
void visit(const koopa_raw_program_t &program)
{
    visit(program.values);
    visit(program.funcs);
}
void visit(const koopa_raw_slice_t &values)
{
    debug(values.len);
    for (size_t i = 0; i < values.len; ++i)
    {
        if (values.kind == KOOPA_RSIK_FUNCTION)
            visit((koopa_raw_function_t)values.buffer[i]);
        else if(values.kind == KOOPA_RSIK_VALUE)

            visit((koopa_raw_value_t)values.buffer[i]);
        else
            assert(0);
    }
}
void visit(koopa_raw_function_t func)
{
    if(func->bbs.len==0)return;
    C
    out << ".text\n"<<flush;
    out << ".globl " << func->name + 1 << '\n'<<flush;
    //out << "\taddi sp, sp, -256"<<endl;
    bug(func->bbs.len);
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        visit((koopa_raw_basic_block_t)func->bbs.buffer[i], !i);
        //out<<"bbs.buffer["<<i<<"] is ok\n";
    }
}
void visit(koopa_raw_basic_block_t bb, bool with_addi)
{
    out << bb->name + 1 << ":\n"<<flush;
    if (with_addi)
        out << "\taddi sp, sp, -1024"<<"#!!!"<<endl;
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
    int pos = 16;
    auto kind = value->kind;
    //debug(value->kind.tag);
    debug(value->kind.tag);
    switch (value->kind.tag)
    {
    case KOOPA_RVT_RETURN://16
        visit(kind.data.ret);
        C
        break;
    case KOOPA_RVT_INTEGER://0
        pos = visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY://12
        pos = visit(kind.data.binary);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_ALLOC://6
        pos = cnt_pos+=4;
        //assert(value->ty->tag == KOOPA_RTT_POINTER);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_LOAD://8
        pos = visit(kind.data.load);
        C
        value_map[value] = pos;
        break;
    case KOOPA_RVT_STORE://9
        visit(kind.data.store);
        break;
    case KOOPA_RVT_BRANCH://13
        visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP://14
        visit(kind.data.jump);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC://7
        global_values[value] = visit(kind.data.global_alloc);
        break;
    case KOOPA_RVT_CALL://15
        pos = visit(kind.data.call);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_GET_ELEM_PTR://11
        assert(0);
        break;
    case KOOPA_RVT_GET_PTR://10
        assert(0);
        break;
    case KOOPA_RVT_FUNC_ARG_REF://4
        pos = visit(kind.data.func_arg_ref);
        break;
    default:
        ;
        assert(false);
    }  
    //cout<<value->kind.tag<<endl;
    //cout<<__LINE__<<endl;
    return pos;
}
void visit(const koopa_raw_return_t &ret)
{
    if(ret.value)
    {
        int pos = visit(ret.value);
        out << "\tlw a0, " << pos <<"(sp)\n"<<flush;
    }
    out<< "\taddi sp, sp, 1024"<<endl;
    out << "\tret" << '\n'<<flush;
}
int visit(const koopa_raw_binary_t &binary)
{
    C
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
    int new_pos = cnt_pos+=4;
    if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        out << "\tla " << reg_names[0] << ", " << global_values[src] << endl;
        out << "\tlw " << reg_names[0] << ", 0("<< reg_names[0] << ")" << endl;
        out << "\tsw " << reg_names[0] << ", "<<new_pos<<"(sp)\n"<<flush;
        return new_pos;
    }
    int pos = value_map[src];
    out<<"\t#loading... src: "<<src<<"  pos: "<<pos<<endl;
    out<<"\tlw    "<<reg_names[0]<<", "<<pos<<"(sp)\n"<<flush;
    out<<"\tsw   "<< reg_names[0] <<", "<<new_pos<<"(sp)\n"<<flush;
    return new_pos;
}
void visit(const koopa_raw_store_t &store)
{
    out<<"#koopa_raw_store_t: \n";
    int pos = visit(store.value);
    //cout<<pos<<' '<<cnt_pos<<endl;
    koopa_raw_value_t dest = store.dest;
    if (dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        out << "\tla s11, " << global_values[dest] << std::endl;
        out<<"\tlw    "<<reg_names[0]<<", "<<pos<<"(sp)\n"<<flush;
        out << "\tsw    " << reg_names[0] << ", 0(s11)" <<
            std::endl;
        return;
    }
    int store_pos = value_map[dest];

    out<<"\tlw    "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
    out<<"\tsw    "<< reg_names[0] <<", "<<store_pos<<"(sp)\n"<<flush;
}
void visit(const koopa_raw_branch_t &branch)
{
    std::string true_label = branch.true_bb->name + 1;
    std::string false_label = branch.false_bb->name + 1;
    int pos = visit(branch.cond);
    out<<"\tlw    "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
    out << "\tbnez  " << reg_names[0] << ", " << true_label
        << std::endl;
    out << "\tj     " << false_label << std::endl;
}
void visit(const koopa_raw_jump_t &jump)
{
    string door = jump.target->name + 1;
    out << "\tj     " << door << std::endl;
}
int visit(const koopa_raw_call_t &call)
{
    int save[8];//保存寄存器
    int ra_save=cnt_pos+=4;//保存ra
    out <<"\tsw    ra" <<", "<<ra_save<<"(sp)\n"<<flush;
    for (size_t i = 0; i < call.args.len; i++)
    {
        if(i<8)
        {
            save[i] = cnt_pos+=4;
            out <<"\tsw    "<< reg_names[7+i] <<", "<<save[i]<<"(sp)\n"<<flush;
        }
        auto ptr = call.args.buffer[i];
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(ptr);
        int pos = visit(arg);
        if(i<8)
        out<<"\tlw    "<< reg_names[7+i] <<", "<<pos<<"(sp)"<<endl;
        else
        {
            move(pos,4*i-32);
        }
    }
    out << "\tcall  " << call.callee->name + 1 << std::endl;
    int new_pos = cnt_pos+=4;
    out<<"\tsw    a0" <<", "<<new_pos<<"(sp)\n"<<flush;
    for (int i = 0; i < min((int)call.args.len,8); i++)
        out <<"\tlw    "<< reg_names[7+i] <<", "<<save[i]<<"(sp)\n"<<flush;
    out <<"\tlw    ra" <<", "<<ra_save<<"(sp)\n"<<flush;
    out<<"\t#"<<__LINE__<<" new_pos"<<new_pos<<endl;
    return new_pos;
}
int visit(const koopa_raw_func_arg_ref_t &ret)
{
    int index = ret.index;
    if(index<8)
    {
        int pos = cnt_pos+=4;
        out << "\tsw "<<reg_names[7+index]<<", "<<pos<<"(sp)\n"<<flush;
        return pos;
    }
    else
    {
        return 4*(index-8)+1024;
    }
}
string visit(const koopa_raw_global_alloc_t&global)
{
    string name = "var_" + to_string(name_num++);
    out << "\t.data" << std::endl;
    out << "\t.globl " << name << std::endl;
    out << name << ":" << std::endl;
    switch (global.init->kind.tag)
    {
    case KOOPA_RVT_ZERO_INIT:
        assert(0); // TODO
        break;
    case KOOPA_RVT_INTEGER:
        out << "\t.word " << global.init->kind.data.integer.value <<endl;
        break;
    case KOOPA_RVT_AGGREGATE:
        assert(0); // TODO
        break;
    default:
        assert(false);
    }
    return name;
}