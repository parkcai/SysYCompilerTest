#pragma once
// 注意栈参数被破坏
#include "koopa.h"
#include <fstream>
#include <assert.h>
#include <map>
#include "math.h"
using std::ofstream, std::map, std::cout, std::cerr;

#define debug(x) cout << __LINE__ << ' ' << x << '\n';
#define LINE out << __LINE__ << std::endl;
#define C // cout<<__LINE__<<endl;
extern ofstream out;
map<const koopa_raw_value_t, int> value_map;
std::string reg_names[20] = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    //0     1     2    3      4     5     6
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0", "temp", "ra", "sp","s11"
    //7    8      9    10    11    12    13    14    15    16      17    18   19
};
static int cnt_pos = 400;
int reg_state[16]; // 0 is temp
int name_num = 0;
std::map<const koopa_raw_value_t, std::string> global_values;
int get_reg_num(string name)
{
    for (int i = 0; i < 20; i++)
        if (reg_names[i] == name)
            return i;
    assert(0);
    return -1;
}
int get_reg()
{
    for (int i = 0; i < 16; i++)
        if (reg_state[i] == 0)
        {
            reg_state[i] = 1;
            return i;
        }
    out << "寄存器用完了\n";
    assert(0);
    return -1;
}
int cal_size(const koopa_raw_type_t &ty)
{
    assert(ty->tag != KOOPA_RTT_UNIT);
    if (ty->tag == KOOPA_RTT_ARRAY)
    {
        int prev = cal_size(ty->data.array.base);
        int len = ty->data.array.len;
        return len * prev;
    }
    return 4;
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
string visit(const koopa_raw_global_alloc_t &global);
int visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr);

void init_aggregate(const koopa_raw_value_t &aggr);
void li(int reg, int num)
{
    out << "\tli    " << reg_names[reg] << ", " << num << endl;
}
void add(int dest, int reg1, int reg2)
{
    out << "\tadd " << reg_names[dest] << ", " << reg_names[reg1] << ", " << reg_names[reg2] << endl;
}
void sw(int reg, int pos, int base = 18)
{
    if (pos >= -2048 && pos <= 2047)
        out << "\tsw   " << reg_names[reg] << ", " << pos 
        <<"("<< reg_names[base]<<")" << endl;
    else
    {
        li(3, pos);
        add(3, base, 3);
        sw(reg, 0, 3);
    }
}
void lw(int reg, int pos, int base = 18)
{
    if (pos >= -2048 && pos <= 2047)
        out << "\tlw   " << reg_names[reg] << ", " << pos
                <<"("<< reg_names[base]<<")" << endl;
    else
    {
        li(3, pos);
        add(3, base, 3);
        lw(reg, 0, 3);
    }
}
void move(int before, int after)
{
    /*debug*/ out << "\t#moving" << endl;
    lw(0, before);
    sw(0, after);
}
void addi(int dest, int reg, int num)
{
    if (num >= -2048 && num <= 2047)
        out << "\taddi   " << reg_names[dest] << ", "
            << reg_names[reg] << ", " << num << endl;
    else
    {
        li(3, num);
        add(dest, reg, 3);
    }
}
void reg2stack(int reg, int pos)
{
    sw(reg, pos);
}
void stack2reg(int pos, int reg)
{
    lw(reg, pos);
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
        else if (values.kind == KOOPA_RSIK_VALUE)

            visit((koopa_raw_value_t)values.buffer[i]);
        else
            assert(0);
    }
}
void visit(koopa_raw_function_t func)
{
    if (func->bbs.len == 0)
        return;
    C
            out
        << ".text\n"
        << flush;
    out << ".globl " << func->name + 1 << '\n'
        << flush;
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        visit((koopa_raw_basic_block_t)func->bbs.buffer[i], !i);
        // out<<"bbs.buffer["<<i<<"] is ok\n";
    }
}
void visit(koopa_raw_basic_block_t bb, bool with_addi)
{
    out << bb->name + 1 << ":\n"
        << flush;
    if (with_addi)
        addi(18, 18, -102400);
    // out << "\taddi sp, sp, -1024"<<"#!!!"<<endl;
    for (size_t i = 0; i < bb->insts.len; ++i)
    {
        visit((koopa_raw_value_t)bb->insts.buffer[i]);
        // out<<"insts.buffer["<<i<<"] is ok\n";
    }
    // cout<<__LINE__<<bb->insts.len<<'\n';
}
int visit(koopa_raw_value_t value)
{
    if (value_map.count(value))
        return value_map[value];
    int pos = 16;
    auto kind = value->kind;
    // debug(value->kind.tag);
    debug(value->kind.tag);
    switch (value->kind.tag)
    {
    case KOOPA_RVT_RETURN: // 16
        visit(kind.data.ret);
        C break;
    case KOOPA_RVT_INTEGER: // 0
        pos = visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY: // 12
        pos = visit(kind.data.binary);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_ALLOC: // 6
        pos = cnt_pos+4; 
        cnt_pos += cal_size(value->ty->data.pointer.base);
        out<<"# alloc at pos: "<<pos<<endl;
        // assert(value->ty->tag == KOOPA_RTT_POINTER);
        addi(0,18,pos);
        pos = cnt_pos += 4;
        sw(0,pos);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_LOAD: // 8
        pos = visit(kind.data.load);
        C
            value_map[value] = pos;
        break;
    case KOOPA_RVT_STORE: // 9
        visit(kind.data.store);
        break;
    case KOOPA_RVT_BRANCH: // 13
        visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP: // 14
        visit(kind.data.jump);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC: // 7
        global_values[value] = visit(kind.data.global_alloc);
        break;
    case KOOPA_RVT_CALL: // 15
        pos = visit(kind.data.call);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_GET_ELEM_PTR: // 11
        pos = visit(kind.data.get_elem_ptr);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_GET_PTR: // 10
        assert(0);
        break;
    case KOOPA_RVT_FUNC_ARG_REF: // 4
        pos = visit(kind.data.func_arg_ref);
        break;
    default:
        assert(false);
    }
    // cout<<value->kind.tag<<endl;
    // cout<<__LINE__<<endl;
    return pos;
}
void visit(const koopa_raw_return_t &ret)
{
    out << "#returning.."<<endl;
    if (ret.value)
    {
        int pos = visit(ret.value);
        out << "#returning.. get pos:"<<pos<<endl;
        lw(get_reg_num("a0"), pos);
        // out << "\tlw a0, " << pos <<"(sp)\n"<<flush;
    }
    // out<< "\taddi sp, sp, 1024"<<endl;
    addi(18, 18, 102400);
    out << "\tret" << '\n'
        << flush;
}
int visit(const koopa_raw_binary_t &binary)
{
    C int left = visit(binary.lhs);
    int right = visit(binary.rhs);
    int new_pos = cnt_pos += 4;
    std::string left_name = reg_names[0];
    lw(0, left);
    // out << "\tlw "<<left_name<<", "<<left<<"(sp)\n"<<flush;
    std::string right_name = reg_names[1];
    lw(1, right);
    // out << "\tlw "<<right_name<<", "<<right<<"(sp)\n"<<flush;
    std::string result_name = reg_names[2];
    switch (binary.op)
    {
    case 0: // ne
        out << "\txor   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        out << "\tsnez  " << result_name << ", " << result_name << std::endl;
        break;
    case 1: // eq
        out << "\txor   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        out << "\tseqz  " << result_name << ", " << result_name << std::endl;
        break;
    case 2: // gt
        out << "\tsgt   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 3: // lt

        out << "\tslt   " << result_name << ", " << left_name << ", " << right_name << std::endl;

        break;
    case 4: // ge
        out << "\tslt   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        out << "\txori  " << result_name << ", " << result_name << ", 1"
            << std::endl;
        break;
    case 5: // le
        out << "\tsgt   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        out << "\txori  " << result_name << ", " << result_name << ", 1"
            << std::endl;
        break;
    case 6: // add
        out << "\tadd   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 7: // sub
        out << "\tsub   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 8: // mul
        out << "\tmul   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 9: // div
        out << "\tdiv   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 10: // mod
        out << "\trem   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 11: // and
        out << "\tand   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 12: // or
        out << "\tor    " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    default:
        assert(false);
    }
    sw(2, new_pos);
    // out<<"\tsw "<< result_name <<", "<<new_pos<<"(sp)\n"<<flush;
    return new_pos;
}
int visit(const koopa_raw_integer_t &integer)
{
    int int_val = integer.value;
    int pos = cnt_pos += 4;
    li(4, int_val);
    // out<<"\tli  "<<reg_names[0]<<", "<<int_val<<'\n'<<flush;
    sw(4, pos);
    // out<<"\tsw  "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
    return pos;
}
int visit(const koopa_raw_load_t &load)
{
    koopa_raw_value_t src = load.src;
    int new_pos = cnt_pos += 4;
    out <<"#loading tag: "<<src->kind.tag<< endl;
    if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        assert(global_values.count(src));
        out << "\tla " << reg_names[0] << ", " << global_values[src] << endl;
        lw(0, 0, 0);
        // out << "\tlw " << reg_names[0] << ", 0("<< reg_names[0] << ")" << endl;
        sw(0, new_pos);
        // out << "\tsw " << reg_names[0] << ", "<<new_pos<<"(sp)\n"<<flush;
        return new_pos;
    }
    else if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
        src->kind.tag == KOOPA_RVT_GET_PTR)
    {
        //assert(0);
        int pos = visit(src);
        stack2reg(pos,0);
        lw(0,0,0);
        reg2stack(0,new_pos);
        return new_pos;
    }
    int pos = value_map[src];
    out << "\t#loading... src: " << src << "  pos: " << pos <<endl;
    lw(0, pos);
    //lw(0, 0, 0);//bug
    // out<<"\tlw    "<<reg_names[0]<<", "<<pos<<"(sp)\n"<<flush;
    sw(0, new_pos);
    // out<<"\tsw   "<< reg_names[0] <<", "<<new_pos<<"(sp)\n"<<flush;
    return new_pos;
}
void visit(const koopa_raw_store_t &store)
{
    int pos = visit(store.value);
    out << "#koopa_raw_store_t pos: "<<pos<<endl;
    // cout<<pos<<' '<<cnt_pos<<endl;
    koopa_raw_value_t dest = store.dest;
    if (dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        assert(global_values.count(dest));
        out << "\tla s11, " << global_values[dest] << std::endl;
        lw(0, pos);
        // out<<"\tlw    "<<reg_names[0]<<", "<<pos<<"(sp)\n"<<flush;
        sw(0, 0, get_reg_num("s11"));
        // out << "\tsw    " << reg_names[0] << ", 0(s11)" << std::endl;
        return;
    }
    else if (dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
        dest->kind.tag == KOOPA_RVT_GET_PTR)
    {
        int store_pos = value_map[dest];
        lw(0,pos);
        lw(1,store_pos);
        sw(0,0,1);
        return;
    }
    int store_pos = value_map[dest];
    lw(0, pos);
    // out<<"\tlw    "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
    sw(0, store_pos);
    // out<<"\tsw    "<< reg_names[0] <<", "<<store_pos<<"(sp)\n"<<flush;
    out<<"#store completed"<<endl;
}
void visit(const koopa_raw_branch_t &branch)
{
    std::string true_label = branch.true_bb->name + 1;
    std::string false_label = branch.false_bb->name + 1;
    int pos = visit(branch.cond);
    lw(0, pos);
    // out<<"\tlw    "<< reg_names[0] <<", "<<pos<<"(sp)\n"<<flush;
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
    int save[8];                // 保存寄存器
    int ra_save = cnt_pos += 4; // 保存ra
    sw(17, ra_save);
    // out <<"\tsw    ra" <<", "<<ra_save<<"(sp)\n"<<flush;
    for (size_t i = 0; i < call.args.len; i++)
    {
        if (i < 8)
        {
            save[i] = cnt_pos += 4;
            sw(7 + i, save[i]);
            // out <<"\tsw    "<< reg_names[7+i] <<", "<<save[i]<<"(sp)\n"<<flush;
        }
        auto ptr = call.args.buffer[i];
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(ptr);
        int pos = visit(arg);
        if (i < 8)
            lw(7 + i, pos);
        // out<<"\tlw    "<< reg_names[7+i] <<", "<<pos<<"(sp)"<<endl;
        else
        {
            move(pos, 4 * i - 32);
        }
    }
    out << "\tcall  " << call.callee->name + 1 << std::endl;
    int new_pos = cnt_pos += 4;
    sw(7, new_pos);
    //out << "\tsw    a0" << ", " << new_pos << "(sp)\n"<< flush;
    for (int i = 0; i < min((int)call.args.len, 8); i++)
        lw(7 + i, save[i]);
    // out <<"\tlw    "<< reg_names[7+i] <<", "<<save[i]<<"(sp)\n"<<flush;
    assert(reg_names[17] == "ra");
    lw(17, ra_save);
    // out <<"\tlw    ra" <<", "<<ra_save<<"(sp)\n"<<flush;
    out << "\t#" << __LINE__ << " new_pos" << new_pos << endl;
    return new_pos;
}
int visit(const koopa_raw_func_arg_ref_t &ret)
{
    int index = ret.index;
    if (index < 8)
    {
        int pos = cnt_pos += 4;
        sw(7 + index, pos);
        // out << "\tsw "<<reg_names[7+index]<<", "<<pos<<"(sp)\n"<<flush;
        return pos;
    }
    else
    {
        return 4 * (index - 8) + 102400;
    }
}
string visit(const koopa_raw_global_alloc_t &global)
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
        out << "\t.word " << global.init->kind.data.integer.value << endl;
        break;
    case KOOPA_RVT_AGGREGATE:
        init_aggregate(global.init);
        break;
    default:
        assert(false);
    }
    return name;
}
void init_aggregate(const koopa_raw_value_t &aggr)
{
    koopa_raw_slice_t elems = aggr->kind.data.aggregate.elems;
    for (size_t i = 0; i < elems.len; i++)
    {
        auto ptr = elems.buffer[i];
        assert(elems.kind == KOOPA_RSIK_VALUE);
        auto value = reinterpret_cast<koopa_raw_value_t>(ptr);
        if (value->kind.tag == KOOPA_RVT_INTEGER)
            out << "\t.word " << value->kind.data.integer.value << endl;
        else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
            init_aggregate(value);
        else
            assert(false);
    }
}
int visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr)
{
    out << "#generating pointer " << endl;
    koopa_raw_type_t arr = get_elem_ptr.src->ty->data.pointer.base;
    int total_size = cal_size(arr), len = arr->data.array.len;
    int elem_size = total_size / len;
    int offset = visit(get_elem_ptr.index);
    stack2reg(offset, 1); // reg[1]存储了偏移量
    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        assert(global_values.count(get_elem_ptr.src));
        out << "\tla    " << reg_names[0] << ", " << global_values[get_elem_ptr.src] << endl;
    }
    else if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || get_elem_ptr.src->kind.tag == KOOPA_RVT_ALLOC)
    {
        int pos = value_map[get_elem_ptr.src];
        out<<"#base pointer is at pos: "<<pos<<endl;
        lw(0,pos);
        //lw(0,0,0);
        //stack2reg(pos, 0);
        //addi(0,18,pos);
    }
    else
    {
        out << get_elem_ptr.src->kind.tag << endl;
        assert(0);
    }
    out << "\tli    s11, " << elem_size << std::endl;
    out << "\tmul   s11, s11, " << reg_names[1] << std::endl;
    out << "\tadd   " << reg_names[0] << ", " << reg_names[0] << ", s11" << std::endl;
    int new_pos = cnt_pos += 4;
    reg2stack(0, new_pos);
    out << "#pointer pos is " << new_pos << endl;
    return new_pos;
}