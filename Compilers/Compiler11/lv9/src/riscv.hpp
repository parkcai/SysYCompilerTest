#include "koopa.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

#define REG_t0 0
#define REG_a0 7
#define REG_ra 15
#define REG_sp 16

static int middle_block = 0;
static int temp_offset = 0;
static int replace_reg = REG_t0;

static map<int,const char*> RegName;
static map<koopa_raw_value_t,int> VarOffset;
static map<koopa_raw_value_t,int> Instr_loc;
static map<int,koopa_raw_value_t> Reg_occ;
static map<koopa_raw_value_t,koopa_raw_basic_block_t> DoneInstr; 
static set<koopa_raw_basic_block_t> DoneBlock;
static map<koopa_raw_value_t,vector<int>> ArraySize;


/*工具函数*/
static int findInstrReg(koopa_raw_value_t value);
static void genInstr(const char* op,int reg1,int reg2,int reg3);
static int findAvailableReg(koopa_raw_value_t need_reg);

static void genMove(int src_reg, int dest_reg)
{
    if (src_reg != dest_reg) 
        printf("  mv %s, %s\n", RegName[dest_reg], RegName[src_reg]);
}

static void genStoreStack(int reg, int offset)
{
    if (offset > 2047) {
        int reg2 = findAvailableReg(NULL);
        printf("  li %s, %d\n", RegName[reg2], offset);
        genInstr("add", reg2, REG_sp, reg2);
        printf("  sw %s, 0(%s)\n", RegName[reg], RegName[reg2]);
    } else {
        printf("  sw %s, %d(sp)\n", RegName[reg], offset);
    }
}

static void genLoadFromStack(int reg, int offset)
{
    if (offset > 2047) {
        int reg2 = findAvailableReg(NULL);
        printf("  li %s, %d\n", RegName[reg2], offset);
        genInstr("add", reg2, REG_sp, reg2);
        printf("  lw %s, 0(%s)\n", RegName[reg], RegName[reg2]);
    } else {
        printf("  lw %s, %d(sp)\n", RegName[reg], offset);
    }
}


static bool RegIsAvailable(int reg, koopa_raw_value_t need_reg)
{
    koopa_raw_value_t value = Reg_occ[reg];
    if (!value) return true;

    for (int i = 0; i < value->used_by.len; i++) {
        koopa_raw_value_t used = (koopa_raw_value_t)value->used_by.buffer[i];
        if (!DoneInstr.count(used)) {
            if (used == need_reg) continue;
            return false;
        }
        koopa_raw_basic_block_t block = DoneInstr[used];
        if (block != DoneInstr[value]) {
            for (int k = 0; k < block->used_by.len; k++) {
                if (!DoneBlock.count((koopa_raw_basic_block_t)block->used_by.buffer[k]))
                    return false;
            }
        }
    }
    return true;
}


static int findAvailableReg(koopa_raw_value_t need_reg)
{
    for (int reg = 0; reg < REG_ra; reg++)
        if (RegIsAvailable(reg, need_reg)) return reg;

    if (++replace_reg == REG_ra) replace_reg = REG_t0;
    koopa_raw_value_t value = Reg_occ[replace_reg];
    VarOffset[value] = temp_offset;
    Instr_loc[value] = -1;
    genStoreStack(replace_reg, temp_offset);
    temp_offset += 4;
    Reg_occ[replace_reg] = NULL;
    return replace_reg;
}


static int findInstrReg(koopa_raw_value_t value)
{
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        if (value->kind.data.integer.value == 0) return -1;
        int reg = findAvailableReg(value);
        printf("  li %s, %d\n", RegName[reg], value->kind.data.integer.value);
        Reg_occ[reg] = value;
        return reg;
    }
    if (value->kind.tag == KOOPA_RVT_ZERO_INIT) return -1;

    int reg = Instr_loc[value];
    if (reg == -1) {
        reg = findAvailableReg(value);
        genLoadFromStack(reg, VarOffset[value]);
    }
    Instr_loc[value] = reg;
    Reg_occ[reg] = value;
    return reg;
}


static void genInstr(const char* op, int reg1, int reg2, int reg3)
{
    printf("  %s %s, %s, %s\n", op, RegName[reg1], RegName[reg2], RegName[reg3]);
}

static void genInit(koopa_raw_value_t init, vector<int> lens)
{
    if (init->kind.tag == KOOPA_RVT_INTEGER) {
        printf("  .word %d\n", init->kind.data.integer.value);
        return;
    }
    int len = lens.back(); lens.pop_back();
    if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
        printf("  .zero %d\n", len << 2);
    } else {
        for (int i = 0; i < init->kind.data.aggregate.elems.len; i++)
            genInit((koopa_raw_value_t)init->kind.data.aggregate.elems.buffer[i], lens);
    }
}


static void print_binary(koopa_raw_value_t value)
{
    koopa_raw_binary_t bin = value->kind.data.binary;
    int lreg = findInstrReg(bin.lhs), rreg = findInstrReg(bin.rhs), reg = findAvailableReg(value);
    const char *reg_name = RegName[reg];

    switch (bin.op) {
        case KOOPA_RBO_NOT_EQ:
            genInstr("xor", reg, lreg, rreg);
            printf("  snez %s, %s\n", reg_name, reg_name);
            break;
        case KOOPA_RBO_EQ:
            genInstr("xor", reg, lreg, rreg);
            printf("  seqz %s, %s\n", reg_name, reg_name);
            break;
        case KOOPA_RBO_GT: genInstr("sgt", reg, lreg, rreg); break;
        case KOOPA_RBO_LT: genInstr("slt", reg, lreg, rreg); break;
        case KOOPA_RBO_GE:
            genInstr("slt", reg, lreg, rreg);
            printf("  seqz %s, %s\n", reg_name, reg_name);
            break;
        case KOOPA_RBO_LE:
            genInstr("sgt", reg, lreg, rreg);
            printf("  seqz %s, %s\n", reg_name, reg_name);
            break;
        case KOOPA_RBO_ADD: genInstr("add", reg, lreg, rreg); break;
        case KOOPA_RBO_SUB: genInstr("sub", reg, lreg, rreg); break;
        case KOOPA_RBO_MUL: genInstr("mul", reg, lreg, rreg); break;
        case KOOPA_RBO_DIV: genInstr("div", reg, lreg, rreg); break;
        case KOOPA_RBO_MOD: genInstr("rem", reg, lreg, rreg); break;
        case KOOPA_RBO_AND: genInstr("and", reg, lreg, rreg); break;
        case KOOPA_RBO_OR:  genInstr("or", reg, lreg, rreg); break;
        case KOOPA_RBO_XOR: genInstr("xor", reg, lreg, rreg); break;
        default: break;
    }

    Instr_loc[value] = reg;
    Reg_occ[reg] = value;
}


static void print_call(koopa_raw_value_t value, int offset)
{
    koopa_raw_slice_t args = value->kind.data.call.args;
    vector<int> save_reg;

    // Save registers
    for (int reg = REG_t0; reg < REG_ra; reg++)
        if (!RegIsAvailable(reg, NULL)) {
            genStoreStack(reg, (offset + reg) * 4);
            save_reg.push_back(reg);
        }

    // Store function arguments on stack
    for (int i = 8; i < args.len; i++) {
        koopa_raw_value_t param = (koopa_raw_value_t)args.buffer[i];
        if (param->kind.tag == KOOPA_RVT_INTEGER) {
            printf("  li %s, %d\n", RegName[REG_t0], param->kind.data.integer.value);
            genStoreStack(REG_t0, (i - 8) * 4);
        } else {
            int param_reg = findInstrReg(param);
            genLoadFromStack(REG_t0, param_reg != -1 ? (offset + param_reg) * 4 : VarOffset[param]);
            genStoreStack(REG_t0, (i - 8) * 4);
        }
    }

    // Store function arguments in registers
    for (int i = 0; i < args.len && i < 8; i++) {
        koopa_raw_value_t param = (koopa_raw_value_t)args.buffer[i];
        if (param->kind.tag == KOOPA_RVT_INTEGER)
            printf("  li %s, %d\n", RegName[i + REG_a0], param->kind.data.integer.value);
        else {
            int reg = findInstrReg(param);
            genLoadFromStack(i + REG_a0, reg != -1 ? (offset + reg) * 4 : VarOffset[param]);
        }
    }

    koopa_raw_function_t callee = value->kind.data.call.callee;
    printf("  call %s\n", callee->name + 1);

    // Handle function return value
    int reg = -1;
    if (callee->ty->tag != KOOPA_RTT_UNIT) {
        if (!RegIsAvailable(REG_a0, value)) {
            reg = findAvailableReg(value);
            Instr_loc[value] = reg; Reg_occ[reg] = value;
            genMove(REG_a0, reg);
        } else {
            Instr_loc[value] = REG_a0; Reg_occ[REG_a0] = value;
            reg = REG_a0;
        }
    }

    // Restore saved registers
    for (int i = 0; i < save_reg.size(); i++)
        if (save_reg[i] != reg && !RegIsAvailable(save_reg[i], value))
            genLoadFromStack(save_reg[i], (offset + save_reg[i]) * 4);
}


static void print_func(koopa_raw_function_t func)
{
    printf("  .globl %s\n", func->name + 1);
    if (func->bbs.len == 0) return;
    printf("%s:\n", func->name + 1);

    VarOffset.clear(); Instr_loc.clear(); Reg_occ.clear();

    int stack_frame_space = 0, max_params_num = 0, extra_params_num = 0, func_params_num = 0, exist_within_call = 0;
    for (size_t i = 0; i < func->bbs.len; ++i) 
    for (size_t k = 0; k < ((koopa_raw_basic_block_t)func->bbs.buffer[i])->insts.len; ++k) {
      koopa_raw_value_t value = (koopa_raw_value_t)((koopa_raw_basic_block_t)func->bbs.buffer[i])->insts.buffer[k];
      if (value->kind.tag == KOOPA_RVT_CALL) 
        exist_within_call = 1, max_params_num = max(max_params_num, (int)value->kind.data.call.args.len);
    }
    
    if (max_params_num > 8) stack_frame_space += (max_params_num - 8) * 4, extra_params_num = max_params_num - 8;
    stack_frame_space += 4; 
    if (exist_within_call) stack_frame_space += 60;

    for (size_t i = 0; i < func->bbs.len; ++i)
    for (size_t k = 0; k < ((koopa_raw_basic_block_t)func->bbs.buffer[i])->insts.len; ++k) {
      koopa_raw_value_t value = (koopa_raw_value_t)((koopa_raw_basic_block_t)func->bbs.buffer[i])->insts.buffer[k];
      if (value->kind.tag == KOOPA_RVT_ALLOC) {
        VarOffset[value] = stack_frame_space;
        const koopa_raw_type_kind *base = value->ty->data.pointer.base;
        if(base->tag == KOOPA_RTT_POINTER) base = base->data.pointer.base;
        int len = 1; vector<int> temp;
        while (base->tag == KOOPA_RTT_ARRAY) temp.push_back(base->data.array.len), len *= base->data.array.len, base = base->data.array.base;
        vector<int> lens; int tlen = 1;
        for (auto i = temp.rbegin(); i != temp.rend(); ++i) lens.push_back(tlen), tlen *= *i;
        if (value->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) lens.push_back(tlen);
        if (len > 1) ArraySize[value] = lens;
        stack_frame_space += len << 2;
      }
    }

    temp_offset = stack_frame_space;
        stack_frame_space = ((stack_frame_space + 255) / 16) << 4;
    if (stack_frame_space > 2047) {
        int reg = findAvailableReg(NULL);
        printf("  li %s, -%d\n", RegName[reg], stack_frame_space);
        genInstr("add", REG_sp, REG_sp, reg);
    } else printf("  addi sp, sp, -%d\n", stack_frame_space);
    genStoreStack(REG_ra, extra_params_num * 4);

    // 翻译指令部分
    for(size_t i = 0;i < func->bbs.len;i++)
    {
        koopa_raw_basic_block_t block = (koopa_raw_basic_block_t)func->bbs.buffer[i];
        DoneBlock.emplace(block);
        if (block->name && i != 0) printf("%s:\n", block->name + 1);
        for(size_t k = 0;k < block->insts.len;k++)
        {
            koopa_raw_value_t value = (koopa_raw_value_t)block->insts.buffer[k];
            koopa_raw_value_tag_t tag = (value->kind).tag;
            if (tag == KOOPA_RVT_LOAD) {
                koopa_raw_value_t load_src = value->kind.data.load.src;
                int reg = findAvailableReg(value);
                ArraySize[value] = ArraySize[load_src];
                if (load_src->kind.tag == KOOPA_RVT_ALLOC) genLoadFromStack(reg, VarOffset[load_src]);
                else if (load_src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) printf("  la %s, %s\n  lw %s, 0(%s)\n", RegName[reg], load_src->name + 1, RegName[reg], RegName[reg]);
                else printf("  lw %s, 0(%s)\n", RegName[reg], RegName[findInstrReg(load_src)]);
                Instr_loc[value] = reg; Reg_occ[reg] = value;
            } else if (tag == KOOPA_RVT_STORE) {
                koopa_raw_value_t store_value = value->kind.data.store.value;
                koopa_raw_value_t store_dest = value->kind.data.store.dest;
                int reg = findInstrReg(store_value);

                if (store_value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
                if (func_params_num < 8) genStoreStack(func_params_num + REG_a0, VarOffset[store_dest]);
                else {
                    int reg = findAvailableReg(NULL);
                    genLoadFromStack(reg, stack_frame_space + (func_params_num - 8) * 4);
                    genStoreStack(reg, VarOffset[store_dest]);
                }
                func_params_num++;
                } else if (store_dest->kind.tag == KOOPA_RVT_ALLOC) genStoreStack(reg, VarOffset[store_dest]);
                else if (store_dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                int temp_reg = findAvailableReg(NULL);
                printf("  la %s, %s\n  sw %s, 0(%s)\n", RegName[temp_reg], store_dest->name + 1, RegName[reg], RegName[temp_reg]);
                } else printf("  sw %s, 0(%s)\n", RegName[reg], RegName[findInstrReg(store_dest)]);
            }
            else if(tag == KOOPA_RVT_BINARY)
            {
                print_binary(value);
                DoneInstr.emplace(value->kind.data.binary.lhs,block);
                DoneInstr.emplace(value->kind.data.binary.rhs,block);
            }
            else if(tag == KOOPA_RVT_JUMP)
            {
                printf("  j %s\n", value->kind.data.jump.target->name + 1);
            }
            else if(tag == KOOPA_RVT_BRANCH)
            {
                koopa_raw_value_t cond = value->kind.data.branch.cond;
                koopa_raw_basic_block_t true_block = value->kind.data.branch.true_bb;
                koopa_raw_basic_block_t false_block = value->kind.data.branch.false_bb;
                int reg = findInstrReg(cond);
                if (true_block->insts.len <= 500) {
                printf("  bnez %s, %s\n  j %s\n", RegName[reg], true_block->name + 1, false_block->name + 1);
                } else {
                string middle = string("middle") + to_string(middle_block++);
                printf("  bnez %s, %s\n  j %s\n%s:\n  j %s\n", RegName[reg], middle.c_str(), false_block->name + 1, middle.c_str(), true_block->name + 1);
                }
            }
            else if (tag == KOOPA_RVT_RETURN) {
                if (value->kind.data.ret.value) {
                    int reg = findInstrReg(value->kind.data.ret.value);
                    genMove(reg, REG_a0);
                    Reg_occ[reg] = NULL;
                }
                genLoadFromStack(REG_ra, extra_params_num * 4);
                if (stack_frame_space > 2047) {
                    int reg = findAvailableReg(NULL);
                    printf("  li %s, %d\n", RegName[reg], stack_frame_space);
                    genInstr("add", REG_sp, REG_sp, reg);
                } else printf("  addi sp, sp, %d\n", stack_frame_space);
                printf("  ret\n");
            }
            else if(tag == KOOPA_RVT_CALL)
            {
                print_call(value,extra_params_num+1);
            }
            else if(tag == KOOPA_RVT_GET_ELEM_PTR)
            {
                koopa_raw_value_t src = (koopa_raw_value_t)value->kind.data.get_elem_ptr.src;
                koopa_raw_value_t index = (koopa_raw_value_t) value->kind.data.get_elem_ptr.index;
                int reg = findAvailableReg(NULL); Reg_occ[reg] = value; int reg2 = findAvailableReg(NULL); Reg_occ[reg2] = value;
                if(src->kind.tag == KOOPA_RVT_ALLOC)
                {
                    printf("  li %s, %d\n", RegName[reg2], VarOffset[src]);
                    genInstr("add",reg,REG_sp,reg2);
                }
                else if(src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
                {
                    printf("  la %s, %s\n", RegName[reg], src->name + 1);
                }
                else
                {
                    Reg_occ[reg] = NULL;
                    reg = findInstrReg(src);
                    Reg_occ[reg] = value;
                }
                int index_reg = findInstrReg(index);
                vector<int> lens = ArraySize[src]; int len = *lens.rbegin(); lens.pop_back();
                ArraySize[value] = lens;
                printf("  li %s, %d\n", RegName[reg2], len * 4);
                genInstr("mul",reg2,reg2,index_reg);
                genInstr("add",reg,reg,reg2);
                Instr_loc[value] = reg;
                Reg_occ[reg2] = NULL;
            }
            else if(tag == KOOPA_RVT_GET_PTR)
            {
                koopa_raw_value_t src = (koopa_raw_value_t)value->kind.data.get_ptr.src;
                koopa_raw_value_t index = (koopa_raw_value_t) value->kind.data.get_ptr.index;
                int reg = findInstrReg(src); Reg_occ[reg] = value;
                int reg2 = findAvailableReg(NULL); Reg_occ[reg2] = value;
                int index_reg = findInstrReg(index);
                vector<int> lens = ArraySize[src];
                int len;
                if(lens.size() == 0)
                    len = 1;
                else
                {
                    len = *(lens.rbegin());
                    lens.pop_back();
                }
                ArraySize[value] = lens;
                printf("  li %s, %d\n", RegName[reg2], len << 2);
                genInstr("mul",reg2,reg2,index_reg);
                genInstr("add",reg,reg,reg2);
                Instr_loc[value] = reg;
                Reg_occ[reg2] = NULL;
            }
            DoneInstr.emplace(value,block);
        }
    }
    cout<<endl;
};

static void system_init()
{
    const std::vector<std::pair<int, const char*>> regNames = {
        {-1, "x0"}, {0, "t0"}, {1, "t1"}, {2, "t2"}, {3, "t3"}, {4, "t4"}, {5, "t5"},
        {6, "t6"}, {7, "a0"}, {8, "a1"}, {9, "a2"}, {10, "a3"}, {11, "a4"}, {12, "a5"},
        {13, "a6"}, {14, "a7"}, {15, "ra"}, {16, "sp"}
    };
    
    for (const auto& [reg, name] : regNames) {
        RegName.emplace(reg, name);
    }

    for (int i = 0; i < 15; ++i) {
        Reg_occ.emplace(i, (koopa_raw_value_t)NULL);
    }
}

static void print_riscv(const char *buf)
{
    koopa_program_t program;
    assert(koopa_parse_from_string(buf, &program) == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    system_init();
    printf("  .data\n");
    for(int i = 0;i < raw.values.len;i++)
    {
        koopa_raw_value_t value = (koopa_raw_value_t) raw.values.buffer[i];
        printf("  .global %s\n", value->name + 1);
        koopa_raw_value_t init = value->kind.data.global_alloc.init;
        printf("%s:\n", value->name + 1);
        if (init->ty->tag == KOOPA_RTT_INT32)
            printf("  .word %d\n", init->kind.data.integer.value);
        else if(init->ty->tag == KOOPA_RTT_ARRAY)
        {
            const koopa_raw_type_kind *base = init->ty;
            int whole_len = 1;
            vector<int> temp;
            while(base->tag == KOOPA_RTT_ARRAY)
            {
                temp.push_back(base->data.array.len);
                whole_len *= base->data.array.len;
                base = base->data.array.base;
            }
            vector<int> lens;
            int tlen = 1;
            for(auto i = temp.rbegin();i != temp.rend();i++)
            {
                tlen *= *i;
                lens.push_back(tlen);
            }
            genInit(init,lens);
            lens.pop_back();
            lens.insert(lens.begin(),1);
            if(whole_len > 1) ArraySize[value] = lens;
        }
    }
    printf("\n  .text\n");

    for (size_t i = 0;i < raw.funcs.len; i++)
        print_func((koopa_raw_function_t) raw.funcs.buffer[i]);
    koopa_delete_raw_program_builder(builder);
}
