#pragma once
#include <unordered_map>
#include <memory>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string.h>
#include "koopa.h"

// #define DEBUG
/**********************************************************************************************************/
/************************************************Stack*****************************************************/
/**********************************************************************************************************/

class Stack
{
public:
    int len;
    int pos;

    Stack()
    {
        len = 0;
        pos = 0;
    }
    void alloc_value(koopa_raw_value_t value, int loc);
    int get_loc(koopa_raw_value_t value);
    void init();

private:
    std::unordered_map<koopa_raw_value_t, int> value_loc;
};

static Stack stack;

static int ra_count = 0;
/**********************************************************************************************************/
/**********************************************PtrSizeVec**************************************************/
/**********************************************************************************************************/

class PtrSizeVec
{
public:
    int get_value_offset(koopa_raw_value_t src);
    int get_value_total_size(koopa_raw_value_t value);
    void push_size(koopa_raw_value_t value, int size);
    void copy_size_vec(koopa_raw_value_t dest, koopa_raw_value_t src);
    void copy_size_vec_ptr(koopa_raw_value_t dest, koopa_raw_value_t src);

private:
    std::unordered_map<koopa_raw_value_t, std::vector<int>> size_vec;
};

static PtrSizeVec ptr_size_vec;

/**********************************************************************************************************/
/************************************************Visit*****************************************************/
/**********************************************************************************************************/

// 访问 raw program
std::string Visit(const koopa_raw_program_t &program);

// 访问 raw slice
std::string Visit(const koopa_raw_slice_t &slice);

// 访问函数
std::string Visit(const koopa_raw_function_t &func);

// 访问基本块
std::string Visit(const koopa_raw_basic_block_t &bb);

// 访问指令
std::string Visit(const koopa_raw_value_t &value);

// 访问 integer 指令
std::string Visit(const koopa_raw_integer_t &integer);

// 访问 global alloc 指令
std::string Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);

// 访问 load 指令
std::string Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value);

// 访问 store 指令
std::string Visit(const koopa_raw_store_t &store);

// 访问 getptr 指令
std::string Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value);

// 访问 getelemptr 指令
std::string Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value);

// 访问 binary 指令
std::string Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);

// 访问 branch 指令
std::string Visit(const koopa_raw_branch_t &branch);

// 访问 jump 指令
std::string Visit(const koopa_raw_jump_t &jump);

// 访问 call 指令
std::string Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value);

// 访问 return 指令
std::string Visit(const koopa_raw_return_t &ret);

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

// 生成数组/指针的 (global) alloc 语句 -> 该数组/指针的维数.
// [[i32, 2], 3] -> 3, 2; **[[i32, 2], 3] ->  1, 1, 3, 2
static std::unordered_map<koopa_raw_value_t, std::vector<int>> dimvec;
// getelemptr 和 getptr 语句 -> 生成的指针的维数,
// 表示为 dimvec 中的 vector 的某段的 begin 和 end
typedef std::pair<std::vector<int>::iterator, std::vector<int>::iterator> pvitvit;
static std::unordered_map<koopa_raw_value_t, pvitvit> dimlr;

// 将 stack 中的值 加载到 reg 中
std::string loadstack_reg(const koopa_raw_value_t &value, const std::string &reg);

// 将 value 的存放地址加载到 reg 中
std::string loadaddr_reg(const koopa_raw_value_t &value, const std::string &reg);

// 将 int 加载到 reg 中
std::string loadint_reg(int value, const std::string &reg);

// 将 reg 中的值存回 value
std::string save_reg(const koopa_raw_value_t &value, const std::string &reg);

// 判断 value 是否为指针
bool value_is_ptr(const koopa_raw_value_t &value);

// 生成aggregate
std::string aggregate_init(const koopa_raw_value_t &value);

// 处理偏移量超出范围
std::string deal_offset_exceed(int offset, std::string inst, std::string reg);

bool is_sw(const std::string &line);
bool is_lw(const std::string &line);
std::string optimize_riscv_code(const std::string &riscv_code);
