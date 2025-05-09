#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <cassert>
#include <algorithm>
#include <koopa.h>

#define IMM12_MAX 2048
#define REG_NUM 32
#define FREE_REG_NUM 13
#define SAVED_REG_NUM 12
#define ZERO_REG 0
#define A0_REG 10
#define T0_REG 5
#define T5_REG 30
#define T6_REG 31

using namespace std;
class RISCV
{
    class Env  //监听
    {
        int total_size;    
        map<koopa_raw_value_t, int> addr;

        public:
            int cur;
            bool has_call;

            void NewEnv(int size, bool _has_call)
            {
                total_size = cur = size;
                has_call = _has_call;
                addr.clear();
            }
            int GetTotalSize()
            {
                return total_size;
            }
            int GetAddr(koopa_raw_value_t value)
            {
                if (addr.count(value))
                    return addr[value];
                int t = calc_inst_size(value);
                if (t == 0)
                    return 2333333;
                cur -= t;
                addr[value] = cur;
                return cur;
            }
        };
    public:

        int magic_cnt_num = 0;
        Env env;
        string current_func_name;
        ostream &output;
        ~RISCV() = default;
        RISCV(ostream &out) : output(out) {}

        static int calc_type_size(koopa_raw_type_t ty); //
        static int calc_inst_size(koopa_raw_value_t value);
        static int calc_blk_size(koopa_raw_basic_block_t block, bool &has_call);
        static int calc_func_size(koopa_raw_function_t func, bool &has_call); //计算函数要保存的size，并计算has_call


        void load_to_reg(koopa_raw_value_t kval, const char *reg);
        void store_to_stack(int addr, const char *reg);
        void gen_riscv_value_aggregate(koopa_raw_value_t kval);
        void gen_riscv_value_global_alloc(koopa_raw_value_t kalloc);
        void gen_riscv_value_load(const koopa_raw_load_t *kload, int addr);
        void gen_riscv_value_store(const koopa_raw_store_t *kstore);
        void gen_riscv_value_get_ptr(const koopa_raw_get_ptr_t *kget, int addr);
        void gen_riscv_value_get_elem_ptr(const koopa_raw_get_elem_ptr_t *kget, int addr);
        void gen_riscv_value_binary(const koopa_raw_binary_t *kbinary, int addr);
        void gen_riscv_value_branch(const koopa_raw_branch_t *kbranch);
        void gen_riscv_value_jump(const koopa_raw_jump_t *kjump);
        void gen_riscv_value_call(const koopa_raw_call_t *kcall, int addr);
        void gen_riscv_value_return(const koopa_raw_return_t *kret);

        void value_to_riscv(koopa_raw_value_t value)
        {
            int addr = env.GetAddr(value);
            switch(value->kind.tag)
            {
            //整数直接输出
            case KOOPA_RVT_INTEGER:
                output << value->kind.data.integer.value;
                break;
            //局部变量，
            case KOOPA_RVT_ALLOC:
                break;
            //数据段（全局变量）
            case KOOPA_RVT_GLOBAL_ALLOC:
                gen_riscv_value_global_alloc(value);
                break;
                
            //生成load指令的汇编
            case KOOPA_RVT_LOAD:
                gen_riscv_value_load(&value->kind.data.load, addr);
                break;
            case KOOPA_RVT_STORE:
                gen_riscv_value_store(&value->kind.data.store);
                break;
            case KOOPA_RVT_GET_PTR:
                gen_riscv_value_get_ptr(&value->kind.data.get_ptr, addr);
                break;
            case KOOPA_RVT_GET_ELEM_PTR:
                gen_riscv_value_get_elem_ptr(&value->kind.data.get_elem_ptr, addr);
                break;
            case KOOPA_RVT_BINARY:
                gen_riscv_value_binary(&value->kind.data.binary, addr);
                break;
            case KOOPA_RVT_BRANCH:
                gen_riscv_value_branch(&value->kind.data.branch);
                break;
            case KOOPA_RVT_JUMP:
                gen_riscv_value_jump(&value->kind.data.jump);
                break;
            case KOOPA_RVT_CALL:
                gen_riscv_value_call(&value->kind.data.call, value->ty->tag == KOOPA_RTT_UNIT ? -1 : addr);
                break;
            case KOOPA_RVT_RETURN:
                gen_riscv_value_return(&value->kind.data.ret);
                break;
            }
        }
        void block_to_riscv(koopa_raw_basic_block_t block)
        {
            output << endl;
            output << current_func_name << "_" << block->name + 1 << ":" << endl;
            slice_to_riscv(&block->insts);
        }

        void func_to_riscv(koopa_raw_function_t func)
        {
            //如果是一个函数定义:bbs.len=0
            if(func->bbs.len == 0)
                return;
            //把函数头前面的@去掉
            const char *name = func->name + 1;
            output << ".globl " << name << endl;
            output << name << ":" << endl;
            bool has_call = false;
            int size = calc_func_size(func, has_call); //传的是引用，注意！
            if(size != 0)
            {
                size = ((size - 1) / 16 + 1) * 16;
                //超出范围，用临时寄存器
                if(-size < -2048 || -size > 2047)
                {
                    //把-size 加载到t0
                    output << "\tli t0, " << -size << endl;
                    output << "\tadd sp, sp, t0" << endl;
                }
                else
                    output << "\taddi sp, sp, " << -size << endl;
            }
            if(has_call)
            {
                int offset = size - 4;
                if(offset < -2048 || offset > 2047)
                {
                    output << "\tli t0, " << offset << endl;
                    //把返回地址存到sp+offset处
                    output << "\tadd t0, sp, t0" << endl;
                    output << "\tsw ra, 0(t0)" << endl;
                }
                else
                    output << "\tsw ra, " << offset << "(sp)" << endl;
            }
            //把临时变量存到env中，开一个Newenv
            env.NewEnv(size, has_call);
            //剩下的env的空间
            env.cur -= (has_call ? 4 : 0);
            // blocks
            current_func_name = func->name + 1;
            slice_to_riscv(&func->bbs);
        }

        void slice_to_riscv(const koopa_raw_slice_t *rs){
            for (uint32_t i = 0; i < rs->len; i++)
            {
                const void *data = rs->buffer[i];
                switch (rs->kind)
                {
                case KOOPA_RSIK_FUNCTION:
                    func_to_riscv((koopa_raw_function_t)data);
                    break;
                case KOOPA_RSIK_BASIC_BLOCK:
                    block_to_riscv((koopa_raw_basic_block_t)data);
                    break;
                case KOOPA_RSIK_VALUE:
                    value_to_riscv((koopa_raw_value_t)data);
                    break;
                }
            }
        }
        void to_string(const koopa_raw_program_t *raw){
            output << ".data" << endl;
            slice_to_riscv(&raw->values);
            output << ".text" << endl;
            slice_to_riscv(&raw->funcs);
        }
};