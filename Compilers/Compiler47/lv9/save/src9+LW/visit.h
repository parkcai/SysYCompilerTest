#pragma once
// 注意栈参数被破坏
#include "koopa.h"
#include <fstream>
#include <assert.h>
#include <map>
#include "math.h"
#include <iostream>
#include <string>
#include <stack>
#include <math.h>
using namespace std;

#define debug(x) cerr << __LINE__ << ' ' << x << '\n';
#define LINE if(allow_out)out << __LINE__ << std::endl;
extern ofstream out;
map<const koopa_raw_value_t, int> value_map;
map<const koopa_raw_value_t, int> value_map_copy;
const int REG_NUM = 20;
std::string reg_names[REG_NUM] = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    //0     1     2    3      4     5     6
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0", "temp", "ra", "sp","s11"
    //7    8      9    10    11    12    13    14    15    16      17    18   19
};

static int cnt_pos = 400;
int reg_state[16]; // 0 is temp
int name_num = 0;
std::map<const koopa_raw_value_t, std::string> global_values;
std::map<const koopa_raw_value_t, std::string> global_values_copy;
map<const koopa_raw_function_t, int> func_map;
map<const koopa_raw_function_t, int> func_map_copy;
bool allow_out = true;
int max_args_num = 0;
stack<int> stack_size;
int timestamp = 0;
int reg_stamp[REG_NUM];
int reg_save[REG_NUM];//保存每个寄存器的值
vector<map<int,int>>reg_map; //reg_map[x] = y: x(sp)保存在寄存器y中
int zero_init;
void init_zero()
{
    if(allow_out&&zero_init>0)
    {
        out << ".zero "<<zero_init*4<<endl;
        zero_init = 0;
    }
}
void copy()
{
    value_map_copy = value_map;
    global_values_copy = global_values;
}
void write()
{
    value_map = value_map_copy;
    global_values = global_values_copy;
}
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
    static int cnt = 0;
    return (cnt++)%7;
    // for (int i = 0; i < 16; i++)
    //     if (reg_state[i] == 0)
    //     {
    //         reg_state[i] = 1;
    //         return i;
    //     }
    // if(allow_out)out << "寄存器用完了\n";
    // assert(0);
    // return -1;
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
int visit(const koopa_raw_get_ptr_t &get_ptr);

void init_aggregate(const koopa_raw_value_t &aggr);
void li(int reg, int num)
{
    if(allow_out)out << "\tli    " << reg_names[reg] << ", " << num << endl;
}
void add(int dest, int reg1, int reg2)
{
    if(allow_out)out << "\tadd " << reg_names[dest] << ", " << reg_names[reg1] << ", " << reg_names[reg2] << endl;
}
void sw(int reg, int pos, int base = get_reg_num("sp"))
{
    if (pos >= -2048 && pos <= 2047)
    {
        if(allow_out)out << "\tsw   " << reg_names[reg] << ", " << pos 
        <<"("<< reg_names[base]<<")" << endl;
    }
    else
    {
        int new_reg = get_reg();
        li(new_reg, pos);
        add(new_reg, base, new_reg);
        sw(reg, 0, new_reg);
    }
}
void lw(int reg, int pos, int base = get_reg_num("sp"))
{
    if (pos >= -2048 && pos <= 2047)
    {
        if(allow_out)out << "\tlw   " << reg_names[reg] << ", " << pos
                <<"("<< reg_names[base]<<")" << endl;
    }
    else
    {
        int new_reg = get_reg();
        li(new_reg, pos);
        add(new_reg, base, new_reg);
        lw(reg, 0, new_reg);
    }
}
int LW(int pos)
{
    int dest = get_reg();
    lw(dest, pos);
    return dest;
}
void move(int before, int after)
{
    /*debug*/ if(allow_out)out << "\t#moving" << endl;
    int reg = LW(before);
    sw(reg, after);
}
void addi(int dest, int reg, int num)
{
    if (num >= -2048 && num <= 2047)
    {
        if(allow_out)out << "\taddi   " << reg_names[dest] << ", "
            << reg_names[reg] << ", " << num << endl;
    }
    else
    {
        int new_reg = get_reg();
        li(new_reg, num);
        add(dest, reg, new_reg);
    }
}
int ADDI(int reg, int num)
{
    int dest = get_reg();
    addi(dest, reg, num);
    return dest;
}
void mul(int dest, int reg1, int reg2)
{
    if(allow_out)out << "\tmul " << reg_names[dest] << ", " << reg_names[reg1] << ", " << reg_names[reg2] << endl;
}
void visit(const koopa_raw_program_t &program)
{
    stack_size.push(0);
    visit(program.values);
    visit(program.funcs);
}
void visit(const koopa_raw_slice_t &values)
{
    debug(values.len);
    for (size_t i = 0; i < values.len; ++i)
    {
        if (values.kind == KOOPA_RSIK_FUNCTION)
        {
            allow_out = false;
            cnt_pos = 0;
            max_args_num = 0;
            copy();
            visit((koopa_raw_function_t)values.buffer[i]);
            write();
            //func_map[(koopa_raw_function_t)values.buffer[i]] = cnt_pos + 4 * max_args_num;
            stack_size.push(cnt_pos + 4 * max_args_num);
            allow_out = true;
            cnt_pos = 4 * max_args_num;
            visit((koopa_raw_function_t)values.buffer[i]);
            stack_size.pop();
        }
        else if (values.kind == KOOPA_RSIK_VALUE)
        {
            visit((koopa_raw_value_t)values.buffer[i]);
        }
        else
            assert(0);
    }
}
void visit(koopa_raw_function_t func)
{
    //init_zero();
    if (func->bbs.len == 0)
        return;
    if(allow_out)out
        << ".text\n"
        << flush;
    if(allow_out)out << ".globl " << func->name + 1 << '\n'
        << flush;
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        visit((koopa_raw_basic_block_t)func->bbs.buffer[i], !i);
    }
}
void visit(koopa_raw_basic_block_t bb, bool with_addi)
{
    if(allow_out)out << bb->name + 1 << ":\n"
        << flush;
    //access sp
    if (with_addi)
        addi(get_reg_num("sp"), get_reg_num("sp"), -stack_size.top());//102400
    for (size_t i = 0; i < bb->insts.len; ++i)
    {
        visit((koopa_raw_value_t)bb->insts.buffer[i]);
    }
}
int visit(koopa_raw_value_t value)
{
    if (value_map.count(value))
        return value_map[value];
    int pos = 16;
    auto kind = value->kind;
    debug(value->kind.tag);
    switch (value->kind.tag)
    {
    case KOOPA_RVT_RETURN: // 16
        visit(kind.data.ret);
        break;
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
        if(allow_out)out<<"# alloc at pos: "<<pos<<endl;
        {
            int reg = ADDI(get_reg_num("sp"),pos);
            pos = cnt_pos += 4;
            sw(reg,pos);
        }

        value_map[value] = pos;
        break;
    case KOOPA_RVT_LOAD: // 8
        pos = visit(kind.data.load);
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
        pos = visit(kind.data.get_ptr);
        value_map[value] = pos;
        break;
    case KOOPA_RVT_FUNC_ARG_REF: // 4
        pos = visit(kind.data.func_arg_ref);
        break;
    default:
        assert(false);
    }
    return pos;
}
void visit(const koopa_raw_return_t &ret)
{
    if(allow_out)out << "#returning.."<<endl;
    if (ret.value)
    {
        int pos = visit(ret.value);
        if(allow_out)out << "#returning.. get pos:"<<pos<<endl;
        //可以采用窥孔优化
        lw(get_reg_num("a0"), pos);
    }
    addi(get_reg_num("sp"), get_reg_num("sp"), stack_size.top());
    if(allow_out)out << "\tret" << '\n'
        << flush;
}
int visit(const koopa_raw_binary_t &binary)
{
    int left = visit(binary.lhs);
    int right = visit(binary.rhs);
    int new_pos = cnt_pos += 4;
    std::string left_name = reg_names[LW(left)];
    // if(allow_out)out << "\tlw "<<left_name<<", "<<left<<"(sp)\n"<<flush;
    std::string right_name = reg_names[LW(right)];
    // if(allow_out)out << "\tlw "<<right_name<<", "<<right<<"(sp)\n"<<flush;
    int reg = get_reg();
    std::string result_name = reg_names[reg];
    switch (binary.op)
    {
    case 0: // ne
        if(allow_out)out << "\txor   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        if(allow_out)out << "\tsnez  " << result_name << ", " << result_name << std::endl;
        break;
    case 1: // eq
        if(allow_out)out << "\txor   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        if(allow_out)out << "\tseqz  " << result_name << ", " << result_name << std::endl;
        break;
    case 2: // gt
        if(allow_out)out << "\tsgt   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 3: // lt

        if(allow_out)out << "\tslt   " << result_name << ", " << left_name << ", " << right_name << std::endl;

        break;
    case 4: // ge
        if(allow_out)out << "\tslt   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        if(allow_out)out << "\txori  " << result_name << ", " << result_name << ", 1"
            << std::endl;
        break;
    case 5: // le
        if(allow_out)out << "\tsgt   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        if(allow_out)out << "\txori  " << result_name << ", " << result_name << ", 1"
            << std::endl;
        break;
    case 6: // add
        if(allow_out)out << "\tadd   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 7: // sub
        if(allow_out)out << "\tsub   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 8: // mul
        if(allow_out)out << "\tmul   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 9: // div
        if(allow_out)out << "\tdiv   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 10: // mod
        if(allow_out)out << "\trem   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 11: // and
        if(allow_out)out << "\tand   " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    case 12: // or
        if(allow_out)out << "\tor    " << result_name << ", " << left_name << ", " << right_name << std::endl;
        break;
    default:
        assert(false);
    }
    sw(reg, new_pos);
    return new_pos;
}
int visit(const koopa_raw_integer_t &integer)
{
    int int_val = integer.value;
    int pos = cnt_pos += 4;
    int reg = get_reg();
    li(reg, int_val);
    sw(reg, pos);
    return pos;
}
int visit(const koopa_raw_load_t &load)
{
    koopa_raw_value_t src = load.src;
    int new_pos = cnt_pos += 4;
    if(allow_out)out <<"#loading tag: "<<src->kind.tag<< endl;
    if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        assert(global_values.count(src));
        int reg = get_reg();
        if(allow_out)out << "\tla " << reg_names[reg] << ", " << global_values[src] << endl;
        lw(reg, 0, reg);
        sw(reg, new_pos);
        return new_pos;
    }
    else if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
        src->kind.tag == KOOPA_RVT_GET_PTR)
    {
        //assert(0);
        int pos = visit(src);
        int reg = get_reg();
        lw(reg, pos);
        lw(reg,0,reg);
        sw(reg, new_pos);
        return new_pos;
    }
    int pos = value_map[src];
    if(allow_out)out << "\t#loading... src: " << src << "  pos: " << pos <<endl;
    //return pos;
    int reg = LW(pos);
    sw(reg, new_pos);
    return new_pos;
}
void visit(const koopa_raw_store_t &store)
{
    int pos = visit(store.value);
    if(allow_out)out << "#koopa_raw_store_t pos: "<<pos<<endl;
    koopa_raw_value_t dest = store.dest;
    if (dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        assert(global_values.count(dest));
        int reg = get_reg();
        int reg2 = get_reg();
        if(allow_out)out << "\tla "<<reg_names[reg]<<", " << global_values[dest] << std::endl;
        lw(reg2, pos);
        sw(reg2, 0, reg);
        return;
    }
    else if (dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
        dest->kind.tag == KOOPA_RVT_GET_PTR)
    {
        int store_pos = value_map[dest];
        int reg = get_reg();
        int reg2 = get_reg();
        lw(reg,pos);
        lw(reg2,store_pos);
        sw(reg,0,reg2);
        return;
    }
    int store_pos = value_map[dest];
    int reg = get_reg();
    lw(reg, pos);
    sw(reg, store_pos);
    if(allow_out)out<<"#store completed"<<endl;
}
void visit(const koopa_raw_branch_t &branch)
{
    std::string true_label = branch.true_bb->name + 1;
    std::string false_label = branch.false_bb->name + 1;
    int pos = visit(branch.cond);
    int reg = LW(pos);
    if(allow_out)out << "\tbnez  " << reg_names[reg] << ", " << true_label
        << std::endl;
    if(allow_out)out << "\tj     " << false_label << std::endl;
}
void visit(const koopa_raw_jump_t &jump)
{
    string door = jump.target->name + 1;
    if(allow_out)out << "\tj     " << door << std::endl;
}
int visit(const koopa_raw_call_t &call)
{
    int save[8];                // 保存寄存器
    int ra_save = cnt_pos += 4; // 保存ra
    sw(get_reg_num("ra"), ra_save);
    max_args_num = max((const uint32_t)max_args_num, call.args.len);
    for (size_t i = 0; i < call.args.len; i++)
    {
        if (i < 8)
        {
            save[i] = cnt_pos += 4;
            sw(7 + i, save[i]);
        }
        auto ptr = call.args.buffer[i];
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(ptr);
        int pos = visit(arg);
        if (i < 8)
            lw(7 + i, pos);
        else
        {
            move(pos, 4 * i - 32);
        }
    }
    if(allow_out)out << "\tcall  " << call.callee->name + 1 << std::endl;
    int new_pos = cnt_pos += 4;
    sw(7, new_pos);
    for (int i = 0; i < min((int)call.args.len, 8); i++)
        lw(7 + i, save[i]);
    lw(get_reg_num("ra"), ra_save);
    if(allow_out)out << "\t#" << __LINE__ << " new_pos" << new_pos << endl;
    return new_pos;
}
int visit(const koopa_raw_func_arg_ref_t &ret)
{
    int index = ret.index;
    if (index < 8)
    {
        int pos = cnt_pos += 4;
        sw(7 + index, pos);
        return pos;
    }
    else
    {
        return 4 * (index - 8) + stack_size.top();
    }
}
string visit(const koopa_raw_global_alloc_t &global)
{
    //init_zero();
    string name = "var_" + to_string(name_num++);
    if(allow_out)out << "\t.data" << std::endl;
    if(allow_out)out << "\t.globl " << name << std::endl;
    if(allow_out)out << name << ":" << std::endl;
    switch (global.init->kind.tag)
    {
    case KOOPA_RVT_ZERO_INIT:
        out<<"\t.zero "<< cal_size(global.init->ty)<<endl;
        break;
    case KOOPA_RVT_INTEGER:
        if(allow_out)
        {
            int value = global.init->kind.data.integer.value;
            // if(value)
            // {
            //     init_zero();
                out << "\t.word " << value << endl;
            // }
            // else
            //     ++zero_init;
        }
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
        {
            //if(allow_out)out << "\t.word " << value->kind.data.integer.value << endl;
            if(allow_out)
            {
                int init_value = value->kind.data.integer.value;
                if(init_value)
                {
                    init_zero();
                    out << "\t.word " << init_value << endl;
                }
                else
                    ++zero_init;
            }
        }
        else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
            init_aggregate(value);
        else
            assert(false);
    }
    init_zero();
}
int visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr)
{
    if(allow_out)out << "#generating pointer " << endl;
    koopa_raw_type_t arr = get_elem_ptr.src->ty->data.pointer.base;
    int total_size = cal_size(arr), len = arr->data.array.len;
    int elem_size = total_size / len;
    int offset = visit(get_elem_ptr.index);
    int reg = LW(offset);
    int reg2 = get_reg();
    int reg3 = get_reg();
    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        assert(global_values.count(get_elem_ptr.src));
        if(allow_out)out << "\tla    " << reg_names[reg2] << ", " << global_values[get_elem_ptr.src] << endl;
    }
    else if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || get_elem_ptr.src->kind.tag == KOOPA_RVT_ALLOC)
    {
        int pos = value_map[get_elem_ptr.src];
        if(allow_out)out<<"#base pointer is at pos: "<<pos<<endl;
        lw(reg2,pos);
    }
    else
    {
        int pos = value_map[get_elem_ptr.src];
        if(allow_out)out<<"#base pointer is at pos: "<<pos<<endl;
        lw(reg2,pos);
    }
    if(allow_out)out << "\tli    "<<reg_names[reg3]<<", " << elem_size << std::endl;
    if(allow_out)out << "\tmul   "<<reg_names[reg3]<<", "<<reg_names[reg3]<<", " << reg_names[reg] << std::endl;
    if(allow_out)out << "\tadd   " << reg_names[reg2] << ", " << reg_names[reg2] << ", "<<reg_names[reg3] << std::endl;
    int new_pos = cnt_pos += 4;
    sw(reg2, new_pos);
    if(allow_out)out << "#pointer pos is " << new_pos << endl;
    return new_pos;
}
int visit(const koopa_raw_get_ptr_t &get_ptr)
{
    int pos = value_map[get_ptr.src];
    koopa_raw_type_t arr = get_ptr.src->ty->data.pointer.base;
    int elem_size = cal_size(arr);
    int index_pos = visit(get_ptr.index);
    int reg = get_reg();
    li(reg, elem_size);
    int reg2 = LW(index_pos);
    mul(reg, reg, reg2);
    int reg3 = LW(pos);
    add(reg, reg, reg3);
    sw(reg, cnt_pos += 4);
    return cnt_pos;
}