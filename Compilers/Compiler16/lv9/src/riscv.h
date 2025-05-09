// src/riscv.h
#ifndef RISCV_H
#define RISCV_H

#include "koopa.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

// RISC-V 操作码枚举
enum class RiscVOpcode {
    ADD,
    ADDI,
    SUB,
    MUL,
    DIV,
    REM,
    AND,
    OR,
    XOR,
    XORI,
    SLLI,
    SRAI,
    SLT,
    SEQZ,
    SNEZ,
    LW,
    SW,
    MV,
    LI,
    LA,
    J,
    BNEZ,
    CALL,
    RET,
    DECL,
    GLOBL,
    ZERO,
    WORD,
    TEXT,
    DATA,
    LABEL
};

struct RiscVInstr {
    RiscVOpcode opcode;
    std::vector<std::string> operands;

    RiscVInstr(RiscVOpcode op, const std::vector<std::string>& ops)
        : opcode(op), operands(ops) {}

    RiscVInstr(RiscVOpcode op, std::initializer_list<std::string> ops)
        : opcode(op), operands(ops) {}
};

// RISC-V 程序类
class RiscVProgram {
public:
    vector<RiscVInstr> instructions;

    // 添加指令到程序中
    void AddInstr(const RiscVInstr& instr) {
        instructions.emplace_back(instr);
    }

    // 将指令列表输出为汇编代码
    void Emit(std::ostream& out) const;
};


// 函数声明：生成 RISC-V 汇编代码
void GenerateRISCV(const koopa_raw_program_t &raw, ostream &out, bool enable_optimize);

// program
void Visit(const koopa_raw_program_t &program, RiscVProgram &rv_program);
// slice
void Visit(const koopa_raw_slice_t &slice, RiscVProgram &rv_program);
// function
void Visit(const koopa_raw_function_t &func, RiscVProgram &rv_program);
// block
void Visit(const koopa_raw_basic_block_t &bb, RiscVProgram &rv_program);
// instr
void Visit(const koopa_raw_value_t &value, RiscVProgram &rv_program);
// return
void Visit(const koopa_raw_return_t &ret, RiscVProgram &rv_program);
// binary: add, sub, mul...
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value, RiscVProgram &rv_program);
// load
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value, RiscVProgram &rv_program);
// store
void Visit(const koopa_raw_store_t &store, RiscVProgram &rv_program);
// branch
void Visit(const koopa_raw_branch_t &branch, RiscVProgram &rv_program);
// jump
void Visit(const koopa_raw_jump_t &jump, RiscVProgram &rv_program);
// call
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value, RiscVProgram &rv_program);
// global alloc
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value, RiscVProgram &rv_program);
// getelemptr 
void Visit(const koopa_raw_value_t& src, const koopa_raw_value_t& index, const koopa_raw_value_t &value, RiscVProgram &rv_program);


void PeepholeOptimize(RiscVProgram &prog);

#endif // RISCV_H
