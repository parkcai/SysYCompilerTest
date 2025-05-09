// src/riscv.cpp

#include "riscv.h"
#include <map>
#include <string>
#include <cassert>
#include <iostream>
#include <vector>
#include <stack>
#include <stdexcept>
#include <functional>

using namespace std;

// 栈帧总大小
int stack_size = 0;

// 映射 Koopa IR 的返回值到栈帧偏移量
unordered_map<koopa_raw_value_t, int> value_stack_map;
unordered_map<koopa_raw_value_t, int > ptr_offset_map;

// 全局跳转指令辅助标签计数器
int label_br_helper_id = 0;

// 函数是否调用了其他函数
bool non_leaf_function;

// 申请临时寄存器
int temp_reg_id = 0;
string AllocTempReg() {
    if(temp_reg_id > 7) throw runtime_error("AllocTempReg Failed: Out of all temporary registers (t0-t7).");
    return "t" + to_string(temp_reg_id++);
}

void FreeTempReg(){
    if(temp_reg_id == 0) throw runtime_error("FreeTempReg Failed: No temporary register is in use.");
    temp_reg_id--;
}

bool IsInstrPtr(const koopa_raw_value_t &inst){
    if (inst->kind.tag == KOOPA_RVT_GET_PTR){
        return true;
    }
    if (inst->kind.tag == KOOPA_RVT_GET_ELEM_PTR){
        return true;
    }
    return false;
}

bool IsGlobalAlloc(koopa_raw_value_t inst){
    while(inst->kind.tag == KOOPA_RVT_GET_ELEM_PTR || inst->kind.tag == KOOPA_RVT_GET_PTR || inst->kind.tag == KOOPA_RVT_LOAD){
        if(inst->kind.tag == KOOPA_RVT_GET_ELEM_PTR){
            inst = inst->kind.data.get_elem_ptr.src;
        }
        else if(inst->kind.tag == KOOPA_RVT_GET_PTR){
            inst = inst->kind.data.get_ptr.src;
        }
        else{
            inst = inst->kind.data.load.src;
        }
    }
    if(inst->kind.tag == KOOPA_RVT_ALLOC){
        return false;
    }
    else if (inst->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        return true;
    }
    else{
        throw runtime_error("IsGlobalAlloc: Unexpected ptr src: " + to_string(inst->kind.tag) );
    }
}

// 计算给定类型的大小（以字节为单位）
int ComputeTypeSize(const koopa_raw_type_t &ty) {
    switch (ty->tag) {
        case KOOPA_RTT_INT32:
            return 4;
        case KOOPA_RTT_UNIT:
            return 0; // void 类型不占用空间
        case KOOPA_RTT_POINTER:
            return 4; // RV32I 指针大小为 4 字节
        case KOOPA_RTT_ARRAY: {
            // 递归计算元素类型的大小并乘以数组长度
            int elem_size = ComputeTypeSize(ty->data.array.base);
            return elem_size * static_cast<int>(ty->data.array.len);
        }
        case KOOPA_RTT_FUNCTION:
            // 函数类型不占用栈空间，指针大小已处理
            return 0;
        default:
            throw runtime_error("Unsupported type in ComputeTypeSize");
    }
}

void TraverseAggregate(koopa_raw_value_t agg_val, vector<int> &elements) {
    if (agg_val->kind.tag == KOOPA_RVT_INTEGER) {
        elements.push_back(agg_val->kind.data.integer.value);
    }
    else if (agg_val->kind.tag == KOOPA_RVT_AGGREGATE) {
        for (size_t i = 0; i < agg_val->kind.data.aggregate.elems.len; ++i) {
            koopa_raw_value_t elem = reinterpret_cast<koopa_raw_value_t>(agg_val->kind.data.aggregate.elems.buffer[i]);
            TraverseAggregate(elem, elements);
        }
    }
    else {
        throw runtime_error("Unsupported initializer element in global aggregate.");
    }
}

// 计算栈帧大小并分配偏移量
void AllocStack(const koopa_raw_function_t &func) {
    int offset = 0;
    int max_arg_cnt = 0;
    // 遍历所有基本块
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);

        // 遍历基本块中的所有指令
        for (size_t j = 0; j < bb->insts.len; ++j) {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);

            if (inst->ty->tag != KOOPA_RTT_UNIT){
                value_stack_map[inst] = offset;
                offset += 4;
            }

            if (inst->kind.tag == KOOPA_RVT_CALL){
                non_leaf_function = true;
                int args_num = inst->kind.data.call.args.len;
                max_arg_cnt = max(max_arg_cnt, args_num - 8);
            }

            if (inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY){
                // 分配数组
                offset -= 4;
                int size = ComputeTypeSize(inst->ty->data.pointer.base);
                offset += size;
            }

            if (inst->kind.tag == KOOPA_RVT_GET_ELEM_PTR || inst->kind.tag == KOOPA_RVT_GET_PTR) {
                // 获取指针类型
                const koopa_raw_type_t src_type = inst->kind.tag == KOOPA_RVT_GET_ELEM_PTR ?
                                                 inst->kind.data.get_elem_ptr.src->ty :
                                                 inst->kind.data.get_ptr.src->ty;

                if (src_type->tag != KOOPA_RTT_POINTER) {
                    throw runtime_error("getelemptr/getptr's src is not a pointer type.");
                }

                // 获取元素类型 T 并计算 sizeof(T)
                int sizeof_T;
                if(inst->kind.tag == KOOPA_RVT_GET_PTR){
                    sizeof_T = ComputeTypeSize(src_type->data.pointer.base);
                }
                else {
                    sizeof_T = ComputeTypeSize(src_type->data.pointer.base) / src_type->data.pointer.base->data.array.len;
                }
                // 将 sizeof_T 存入 inst_dim_map
                ptr_offset_map[inst] = sizeof_T;
            }
        }
    }

    int arg_offset = max_arg_cnt * 4;
    offset += arg_offset;
    for(auto& pair : value_stack_map){
        pair.second += arg_offset; 
    }

    if(non_leaf_function) offset += 4;

    stack_size = ((offset + 15) / 16) * 16;
}

// 更新 GenerateRISCV 函数的实现
void GenerateRISCV(const koopa_raw_program_t &raw, ostream &out, bool enable_optimize) {
    // 清空映射表
    value_stack_map.clear();
    ptr_offset_map.clear();

    // 创建 RiscVProgram 实例
    RiscVProgram program;

    // 访问 raw program, 填充 RiscVProgram
    Visit(raw, program);

    if(enable_optimize){
        PeepholeOptimize(program);
    }

    // 生成汇编代码到输出流
    program.Emit(out);
}


// 实现 RiscVProgram::Emit
void RiscVProgram::Emit(ostream& out) const {
    for (const auto& instr : instructions) {
        switch (instr.opcode) {
            case RiscVOpcode::ADD:
                out << "  add " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::ADDI:
                out << "  addi " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::SUB:
                out << "  sub " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::MUL:
                out << "  mul " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::DIV:
                out << "  div " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::REM:
                out << "  rem " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::AND:
                out << "  and " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
            case RiscVOpcode::OR:
                out << "  or " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::XOR:
                out << "  xor " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::XORI:
                out << "  xori " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::SLLI:
                out << "  slli " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::SRAI:
                out << "  srai " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::SEQZ:
                out << "  seqz " << instr.operands[0] << ", " << instr.operands[1] << "\n";
                break;
            case RiscVOpcode::SNEZ:
                out << "  snez " << instr.operands[0] << ", " << instr.operands[1] << "\n";
                break;
            case RiscVOpcode::SLT:
                out << "  slt " << instr.operands[0] << ", " << instr.operands[1] << ", " << instr.operands[2] << "\n";
                break;
            case RiscVOpcode::LW:
                out << "  lw " << instr.operands[0] << ", " << instr.operands[1] << "(" << instr.operands[2] << ")\n";
                break;
            case RiscVOpcode::SW:
                out << "  sw " << instr.operands[0] << ", " << instr.operands[1] << "(" << instr.operands[2] << ")\n";
                break;
            case RiscVOpcode::MV:
                out << "  mv " << instr.operands[0] << ", " << instr.operands[1] << "\n";
                break;
            case RiscVOpcode::LI:
                out << "  li " << instr.operands[0] << ", " << instr.operands[1] << "\n";
                break;
            case RiscVOpcode::LA:
                out << "  la " << instr.operands[0] << ", " << instr.operands[1] << "\n";
                break;
            case RiscVOpcode::J:
                out << "  j " << instr.operands[0] << "\n";
                break;
            case RiscVOpcode::BNEZ:
                out << "  bnez " << instr.operands[0] << ", " << instr.operands[1] << "\n";
                break;
            case RiscVOpcode::CALL:
                out << "  call " << instr.operands[0] << "\n";
                break;
            case RiscVOpcode::RET:
                out << "  ret\n";
                break;
            case RiscVOpcode::DECL:
                out << "  .decl " << instr.operands[0] << "\n";
                break;
            case RiscVOpcode::GLOBL:
                out << "  .globl " << instr.operands[0] << "\n";
                break;
            case RiscVOpcode::ZERO:
                out << "  .zero " << instr.operands[0] << "\n";
                break;
            case RiscVOpcode::WORD:
                out << "  .word " << instr.operands[0] << "\n";
                break;
            case RiscVOpcode::TEXT:
                out << "  .text\n";
                break;
            case RiscVOpcode::DATA:
                out << "  .data\n";
                break;
            case RiscVOpcode::LABEL:
                out << instr.operands[0] << ":\n";
                break;
            default:
                throw runtime_error("Unknown Instr");
                break;
        }
    }
}

void PeepholeOptimize(RiscVProgram &prog) {
    vector<RiscVInstr> &instrs = prog.instructions;
    vector<RiscVInstr> optimized;
    optimized.reserve(instrs.size());

    auto IsPowerOfTwo = [&](int64_t x) -> bool {
        return (x > 0) && ((x & (x - 1)) == 0);
    };

    auto Log2Int = [&](int64_t x) -> int {
        int n = 0;
        while ((1LL << n) < x) ++n;
        return n;
    };

    // 记录最后一次把某个寄存器加载为某个立即数的指令序号
    // 用于去重连续的 li
    unordered_map<string, pair<int64_t, size_t>> last_li_map; 
    // 比如 last_li_map["t0"] = {100, 10} 表示在 i=10 的指令处，t0被li写成100

    // 记录内存位置 -> 最后存储的指令idx
    // 用于进行简单“dead store”消除
    // 这里将 base+offset 组合成 key
    auto MemoryKey = [&](const string &base, const string &offset) {
        return base + ":" + offset;
    };
    unordered_map<string,size_t> last_store_map; 

    for (size_t i = 0; i < instrs.size(); ++i) {
        RiscVInstr &curr = instrs[i];

        //-----------------------------------------
        // 1) 冗余 sw-lw 消除 (已实现，基本同前)
        //-----------------------------------------
        if (curr.opcode == RiscVOpcode::SW) {
            // 看下一条
            if (i + 1 < instrs.size() && instrs[i+1].opcode == RiscVOpcode::LW) {
                auto &sw_oper = curr.operands;    // [sw_src, offset, base]
                auto &lw_oper = instrs[i+1].operands;  // [lw_dst, offset, base]
                bool same_mem = (sw_oper[1] == lw_oper[1]) && (sw_oper[2] == lw_oper[2]);
                string sw_src_reg = sw_oper[0];
                string lw_dst_reg = lw_oper[0];
                if (same_mem) {
                    if (sw_src_reg == lw_dst_reg) {
                        // lw 冗余
                        optimized.push_back(curr); // sw
                        i++;
                        continue; // skip lw
                    } else {
                        // mv dst, src
                        optimized.push_back(curr); 
                        RiscVInstr mv_instr(RiscVOpcode::MV,{lw_dst_reg, sw_src_reg});
                        optimized.push_back(mv_instr);
                        i++;
                        continue; 
                    }
                }
            }
            // 无法合并,正常保留
            optimized.push_back(curr);

            // 记录 memory store
            auto &op = curr.operands; // [src, offset, base]
            string memkey = MemoryKey(op[2], op[1]);
            last_store_map[memkey] = optimized.size() - 1; 
            continue;
        }

        //-----------------------------------------
        // 2) 代数化简 + 强度消减
        //-----------------------------------------
        bool handled = false;
        if (curr.opcode == RiscVOpcode::ADD ||
            curr.opcode == RiscVOpcode::SUB ||
            curr.opcode == RiscVOpcode::MUL ||
            curr.opcode == RiscVOpcode::DIV)
        {
            auto &op = curr.operands; 
            if (op.size() == 3) {
                string &dst = op[0], &lhs = op[1], &rhs = op[2];
                RiscVOpcode old_op = curr.opcode;
                // 尝试解析rhs为立即数
                bool is_int = true;
                int64_t imm_val = 0;
                try {
                    imm_val = stoll(rhs); 
                } catch (...) {
                    is_int = false;
                }
                if (is_int) {
                    // (1) 代数化简
                    if ((old_op == RiscVOpcode::ADD || old_op == RiscVOpcode::SUB) && imm_val == 0) {
                        // add x, y, 0 -> mv x, y
                        if (dst == lhs) {
                            handled = true; 
                            // 直接删掉
                        } else {
                            RiscVInstr mv(RiscVOpcode::MV,{dst, lhs});
                            optimized.push_back(mv);
                            handled = true;
                        }
                    }
                    else if ((old_op == RiscVOpcode::MUL || old_op == RiscVOpcode::DIV) && imm_val == 1) {
                        // mul x, y, 1 / div x, y, 1 -> mv
                        if (dst == lhs) {
                            handled = true; 
                        } else {
                            RiscVInstr mv(RiscVOpcode::MV,{dst, lhs});
                            optimized.push_back(mv);
                            handled = true;
                        }
                    }
                    // (2) 强度消减：2^n
                    else if ((old_op == RiscVOpcode::MUL || old_op == RiscVOpcode::DIV) && IsPowerOfTwo(imm_val)) {
                        int n = Log2Int(imm_val);
                        if (old_op == RiscVOpcode::MUL) {
                            // sll
                            RiscVInstr sll(RiscVOpcode::SLLI,{dst, lhs, to_string(n)});
                            optimized.push_back(sll);
                        } else {
                            // sra
                            RiscVInstr sra(RiscVOpcode::SRAI,{dst, lhs, to_string(n)});
                            optimized.push_back(sra);
                        }
                        handled = true;
                    }
                }
            }
        }
        if (handled) {
            continue;
        }

        //-----------------------------------------
        // 3) Jump 指令相关优化
        //-----------------------------------------
        // 简化: 如果 j labelA 后面紧跟 labelA，就删掉 j
        if (curr.opcode == RiscVOpcode::J && curr.operands.size() == 1) {
            string targetLabel = curr.operands[0];
            // 看看下一条是不是 label
            if (i + 1 < instrs.size()) {
                if (instrs[i+1].opcode == RiscVOpcode::LABEL && 
                    instrs[i+1].operands.size() == 1 &&
                    instrs[i+1].operands[0] == targetLabel) {
                    // 冗余
                    continue;
                }
            }
        }

        // 如果都没匹配 => 默认保留
        optimized.push_back(curr);
    }

    // 最后将处理完的指令替换
    instrs.swap(optimized);
}


// 生成 addi 指令，处理偏移量范围
void GenerateAddImm(int imm, const string& src_reg, const string& dest_reg, RiscVProgram &rv_program) {
    if (imm >= -2048 && imm <= 2047) {
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::ADDI, {dest_reg, src_reg, to_string(imm)}));
    } else {
        string temp_reg = AllocTempReg();
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(imm)}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::ADD, {dest_reg, src_reg, temp_reg}));
        FreeTempReg();
    }
}

// 生成 lw 指令，处理偏移量范围
void GenerateLoad(int offset, const string& base_reg, const string& dest_reg, RiscVProgram &rv_program) {
    if (offset >= -2048 && offset <= 2047) {
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LW, {dest_reg, to_string(offset), base_reg}));
    } else {
        string temp_reg = AllocTempReg();
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(offset)}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::ADD, {temp_reg, base_reg, temp_reg}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LW, {dest_reg, to_string(0), temp_reg}));
        FreeTempReg();
    }
}

// 生成 sw 指令，处理偏移量范围
void GenerateStore(int offset, const string& base_reg, const string& src_reg, RiscVProgram &rv_program) {
    if (offset >= -2048 && offset <= 2047) {
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::SW, {src_reg, to_string(offset), base_reg}));
    } else {
        string temp_reg = AllocTempReg();
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(offset)}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::ADD, {temp_reg, base_reg, temp_reg}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::SW, {src_reg, to_string(0), temp_reg}));
        FreeTempReg();
    }
}

// 访问 raw program
void Visit(const koopa_raw_program_t &program, RiscVProgram &rv_program) {
    // 首先处理全局变量/常量
    if (program.values.len > 0) {
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::DATA,{}));
        Visit(program.values, rv_program);
    }
    // 访问所有函数
    Visit(program.funcs, rv_program);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice, RiscVProgram &rv_program) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr), rv_program);
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), rv_program);
                break;
            case KOOPA_RSIK_VALUE: {
                auto val = reinterpret_cast<koopa_raw_value_t>(ptr);
                Visit(val, rv_program);
                break;
            }
            default:
                cerr << "Error: Unsupported slice kind: " << slice.kind << endl;
                assert(false);
        }
    }
}

// 访问函数
void Visit(const koopa_raw_function_t &func, RiscVProgram &rv_program) {
    if (func->bbs.len == 0) {
        // 函数声明，忽略
        return;
    }
    // 移除函数名中的 '@' 前缀
    string func_name = func->name;
    if (!func_name.empty() && func_name[0] == '@') {
        func_name = func_name.substr(1);
    }
    // 输出函数的全局声明和标签
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::TEXT,{}));
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::GLOBL,{func_name}));
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::LABEL,{func_name}));

    // **新增部分**：计算栈帧大小和偏移量
    stack_size = 0;
    value_stack_map.clear();
    non_leaf_function = false;
    AllocStack(func);

    // 生成函数的 prologue
    if (stack_size > 0) {
        GenerateAddImm(-stack_size, "sp", "sp", rv_program);
    }

    if (non_leaf_function) {
        GenerateStore(stack_size-4, "sp", "ra", rv_program);
    }

    // 访问所有基本块
    Visit(func->bbs, rv_program);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb, RiscVProgram &rv_program) {
    string bb_name = bb->name + 1;

    // entry基本块标签无需输出
    if(bb_name != "entry"){
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LABEL,{bb_name}));
    }
    Visit(bb->insts, rv_program);
}

// 访问指令
void Visit(const koopa_raw_value_t &value, RiscVProgram &rv_program) {
    const auto &kind = value->kind;

    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            break;
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_LOAD:
            Visit(kind.data.load, value, rv_program);
            break;
        case KOOPA_RVT_STORE:
            Visit(kind.data.store, rv_program);
            break;
        case KOOPA_RVT_BINARY:
            Visit(kind.data.binary, value, rv_program);
            break;
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret, rv_program);
            break;
        case KOOPA_RVT_BRANCH:
            Visit(kind.data.branch, rv_program);
            break;
        case KOOPA_RVT_JUMP:
            Visit(kind.data.jump, rv_program);
            break;
        case KOOPA_RVT_CALL:
            Visit(kind.data.call, value, rv_program);
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            Visit(kind.data.global_alloc, value, rv_program);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            Visit(kind.data.get_elem_ptr.src, kind.data.get_elem_ptr.index, value, rv_program);
            break;
        case KOOPA_RVT_GET_PTR:
            Visit(kind.data.get_ptr.src, kind.data.get_ptr.index, value, rv_program);
            break;
        default:
            cerr << "Error: Unsupported instruction kind: " << kind.tag << endl;
            assert(false);
    }
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value, RiscVProgram &rv_program) {
    auto src = load.src;

    if(src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        string temp_reg = AllocTempReg();
        string global_name = src->name;
        if(!global_name.empty() && global_name[0] == '@')
            global_name = global_name.substr(1);
        
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LA, {temp_reg, global_name}));
        GenerateLoad(0, temp_reg, temp_reg, rv_program);
        int dst_offset = value_stack_map[value];
        GenerateStore(dst_offset, "sp", temp_reg, rv_program);
        FreeTempReg();
    }
    else{
        if(IsInstrPtr(src)){              
            int ptr_offset = value_stack_map[src];
            string ptr_reg = AllocTempReg();
            string temp_reg = AllocTempReg();
            GenerateLoad(ptr_offset, "sp", ptr_reg, rv_program);
            GenerateLoad(0, ptr_reg, temp_reg, rv_program);
            int dst_offset = value_stack_map[value];
            GenerateStore(dst_offset, "sp", temp_reg, rv_program);
            FreeTempReg();
            FreeTempReg();
        }
        else{
            string temp_reg = AllocTempReg();
            int src_offset = value_stack_map[src];
            GenerateLoad(src_offset, "sp", temp_reg, rv_program);
            int dst_offset = value_stack_map[value];
            GenerateStore(dst_offset, "sp", temp_reg, rv_program);
            FreeTempReg();
        }
    }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store, RiscVProgram &rv_program) {
    auto value = store.value;
    auto dest = store.dest;

    if(dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        string global_sym = dest->name;
        if (!global_sym.empty() && global_sym[0] == '@') {
            global_sym.erase(0,1);
        }
        string base_reg = AllocTempReg();
        string temp_reg = AllocTempReg();
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LA, {base_reg, global_sym}));
        if (value->kind.tag == KOOPA_RVT_INTEGER) {
            int imm = value->kind.data.integer.value;
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(imm)}));
        } else {
            int value_offset = value_stack_map[value];
            GenerateLoad(value_offset, "sp", temp_reg, rv_program);
        }
        GenerateStore(0, base_reg, temp_reg, rv_program);
        FreeTempReg();
        FreeTempReg();
    } 
    else {
        if(value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
            int dest_offset = value_stack_map[dest];
            int arg_idx = value->kind.data.func_arg_ref.index;

            if (arg_idx >=0 && arg_idx < 8){
                string arg_reg = "a" + to_string(arg_idx); 
                GenerateStore(dest_offset, "sp", arg_reg, rv_program);

            } else {
                int arg_offset = stack_size + (arg_idx - 8) * 4;
                string temp_reg = AllocTempReg();
                GenerateLoad(arg_offset, "sp", temp_reg, rv_program);
                GenerateStore(dest_offset, "sp", temp_reg, rv_program);
                FreeTempReg();
            }
        } 
        else {
            if (IsInstrPtr(dest)){
                int ptr_offset = value_stack_map[dest];
                string dest_reg = AllocTempReg();
                string src_reg = AllocTempReg();
                GenerateLoad(ptr_offset, "sp", dest_reg, rv_program);

                if (value->kind.tag == KOOPA_RVT_INTEGER) {
                    int imm = value->kind.data.integer.value;
                    rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {src_reg, to_string(imm)}));
                } else {
                    int value_offset = value_stack_map[value];
                    GenerateLoad(value_offset, "sp", src_reg, rv_program);
                }

                GenerateStore(0, dest_reg, src_reg, rv_program);
                FreeTempReg();
                FreeTempReg();   
            }
            else{
                int dest_offset = value_stack_map[dest];
                string temp_reg = AllocTempReg();

                if (value->kind.tag == KOOPA_RVT_INTEGER) {
                    int imm = value->kind.data.integer.value;
                    rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(imm)}));
                } else {
                    int value_offset = value_stack_map[value];
                    GenerateLoad(value_offset, "sp", temp_reg, rv_program);
                }
                GenerateStore(dest_offset, "sp", temp_reg, rv_program);
                FreeTempReg();
            }
        }
    }
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value, RiscVProgram &rv_program) {
    // 处理左操作数
    string lhs_reg = AllocTempReg();
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER) {
        int imm = binary.lhs->kind.data.integer.value;
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {lhs_reg, to_string(imm)}));
    } else {
        int lhs_offset = value_stack_map[binary.lhs];
        GenerateLoad(lhs_offset, "sp", lhs_reg, rv_program);
    }

    // 处理右操作数
    string rhs_reg = AllocTempReg();
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER) {
        int imm = binary.rhs->kind.data.integer.value;
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {rhs_reg, to_string(imm)}));
    } else {
        int rhs_offset = value_stack_map[binary.rhs];
        GenerateLoad(rhs_offset, "sp", rhs_reg, rv_program);
    }

    string result_reg = lhs_reg;
    switch (binary.op) {
        case KOOPA_RBO_ADD:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::ADD, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_SUB:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SUB, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_MUL:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::MUL, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_DIV:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::DIV, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_MOD:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::REM, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_AND:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::AND, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_OR:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::OR, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_XOR:
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::XOR, {result_reg, lhs_reg, rhs_reg}));
            break;
        case KOOPA_RBO_EQ: {
            // eq: (lhs == rhs) ? 1 : 0
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::XOR, {result_reg, lhs_reg, rhs_reg}));
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SEQZ, {result_reg, result_reg}));
            break;
        }
        case KOOPA_RBO_NOT_EQ: {
            // ne: (lhs != rhs) ? 1 : 0
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::XOR, {result_reg, lhs_reg, rhs_reg}));
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SNEZ, {result_reg, result_reg}));
            break;
        }
        case KOOPA_RBO_GT: {
            // gt: (lhs > rhs) ? 1 : 0
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SLT, {result_reg, rhs_reg, lhs_reg}));
            break;
        }
        case KOOPA_RBO_LT: {
            // lt: (lhs < rhs) ? 1 : 0
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SLT, {result_reg, lhs_reg, rhs_reg}));
            break;
        }
        case KOOPA_RBO_GE: {
            // ge: (lhs >= rhs) ? 1 : 0
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SLT, {result_reg, lhs_reg, rhs_reg}));
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::XOR, {result_reg, result_reg, to_string(1)}));
            break;
        }
        case KOOPA_RBO_LE: {
            // le: (lhs <= rhs) ? 1 : 0
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::SLT, {result_reg, rhs_reg, lhs_reg}));
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::XOR, {result_reg, result_reg, to_string(1)}));
            break;
        }
        default:
            cerr << "Error: Unsupported binary operation: " << binary.op << "\n";
            assert(false);
    }

    // 将结果存储到当前指令对应的栈帧位置
    int dst_offset = value_stack_map[value];
    GenerateStore(dst_offset, "sp", result_reg, rv_program);

    FreeTempReg();
    FreeTempReg();
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret, RiscVProgram &rv_program) {
    koopa_raw_value_t ret_value = ret.value;
    if (ret_value != nullptr) {
        if (ret_value->kind.tag == KOOPA_RVT_INTEGER) {
            int imm = ret_value->kind.data.integer.value;
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {"a0", to_string(imm)}));
        } else {
            int ret_offset = value_stack_map[ret_value];
            GenerateLoad(ret_offset, "sp", "a0", rv_program);
        }
    }

    // 生成函数的 epilogue
    if (non_leaf_function) {
        GenerateLoad(stack_size-4, "sp", "ra", rv_program);
    }

    if (stack_size > 0) {
        GenerateAddImm(stack_size, "sp", "sp", rv_program);
    }

   rv_program.AddInstr(RiscVInstr(RiscVOpcode::RET, {}));
}

// 访问br指令
void Visit(const koopa_raw_branch_t &branch, RiscVProgram &rv_program) {
    
    auto cond = branch.cond;
    string temp_reg = AllocTempReg();
    
    // 处理条件表达式
    if (cond->kind.tag == KOOPA_RVT_INTEGER) {
        int imm = cond->kind.data.integer.value;
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(imm)}));
    } else {
        int cond_offset = value_stack_map[cond];
        GenerateLoad(cond_offset, "sp", temp_reg, rv_program);
    }

    string true_label = branch.true_bb->name + 1;
    string false_label = branch.false_bb->name + 1;

    
    if (stack_size >= 4096){
        string helper_label = "LABEL_BR_HELPER_" + to_string(label_br_helper_id);
        label_br_helper_id++;

        // 使用bnez跳转到辅助标签
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::BNEZ, {temp_reg, helper_label}));
        
        // 如果条件不满足，跳转到false_label
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::J, {false_label}));
        
        // 辅助标签，执行跳转到true_label
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LABEL, {helper_label}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::J, {true_label}));

    } else {
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::BNEZ, {temp_reg, true_label}));
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::J, {false_label}));
    }
    
    FreeTempReg();
}

// 访问jump指令
void Visit(const koopa_raw_jump_t &jump, RiscVProgram &rv_program) {
    string target_label = jump.target->name + 1;
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::J, {target_label}));
}

// 访问call指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value, RiscVProgram &rv_program) {
    string callee_name = call.callee->name;
    if(!callee_name.empty() && callee_name[0] == '@') {
        callee_name = callee_name.substr(1);
    }

    size_t arg_count = call.args.len;
    for (size_t i = 0; i < arg_count; i++) {
        auto arg_val = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if(i < 8){
            string arg_reg = "a" + to_string(i);
            if(arg_val->kind.tag == KOOPA_RVT_INTEGER){
                int imm = arg_val->kind.data.integer.value;
                rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {arg_reg, to_string(imm)}));
            } else {
                int offset = value_stack_map[arg_val];
                GenerateLoad(offset, "sp", arg_reg, rv_program);
            }
        } else {
            string temp_reg = AllocTempReg();
            if(arg_val->kind.tag == KOOPA_RVT_INTEGER){
                int imm = arg_val->kind.data.integer.value;
                rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {temp_reg, to_string(imm)}));
            } else {
                int offset = value_stack_map[arg_val];
                GenerateLoad(offset, "sp", temp_reg, rv_program);
            }
            int offset = 4 * (i - 8);
            GenerateStore(offset, "sp", temp_reg, rv_program);
            FreeTempReg();
        }

    }

    rv_program.AddInstr(RiscVInstr(RiscVOpcode::CALL, {callee_name}));
    if (value->ty->tag != KOOPA_RTT_UNIT) {
        int dst_offset = value_stack_map[value];
        GenerateStore(dst_offset, "sp", "a0", rv_program);
    }
}

// 处理全局 alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value, RiscVProgram &rv_program) {
    string symbol_name = value->name;
    if (!symbol_name.empty() && symbol_name[0] == '@') {
        symbol_name = symbol_name.substr(1);
    }

    koopa_raw_value_t init_val = global_alloc.init;

    // 输出伪指令: .globl var
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::GLOBL, {symbol_name}));
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::LABEL, {symbol_name}));


    // 如果初始值为空(zeroinit)或 Koopa IR 里是 zero_init
    if (!init_val || init_val->kind.tag == KOOPA_RVT_ZERO_INIT) {
        int type_size = ComputeTypeSize(value->ty->data.pointer.base);
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::ZERO, {to_string(type_size)}));
    }
    // 如果初始值是整数类型, 则输出 .word init_value
    else if (init_val->kind.tag == KOOPA_RVT_INTEGER) {
        int imm = init_val->kind.data.integer.value;
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::WORD, {to_string(imm)}));
    }
    // 如果初始值是聚合类型, 递归处理并生成对应的 .word 和 .zero 指令
    else if (init_val->kind.tag == KOOPA_RVT_AGGREGATE) {
        // 计算数组总元素数量
        vector<int> elements; // 存储初始化的元素值
        TraverseAggregate(init_val, elements);

        // 生成 .word 指令
        for (int val : elements) {
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::WORD, {to_string(val)}));
        }
    }
    else {
        throw runtime_error("Unsupported global initializer for symbol: " + symbol_name);
    }
}

// 访问 getptr/getelemptr 指令
void Visit(const koopa_raw_value_t& src, const koopa_raw_value_t& index, const koopa_raw_value_t &value, RiscVProgram &rv_program) {

    // 申请临时寄存器
    string ptr_reg = AllocTempReg();
    string offset_reg = AllocTempReg();
    string size_reg = AllocTempReg();
    
    // 加载源指针地址
    if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        string global_name = src->name;
        if (!global_name.empty() && global_name[0] == '@') {
            global_name = global_name.substr(1);
        }
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LA, {ptr_reg, global_name}));
    }
    else if (src->kind.tag == KOOPA_RVT_ALLOC){
        int src_offset = value_stack_map[src];
        GenerateAddImm(src_offset, "sp", ptr_reg, rv_program);
    }
    else {
        int src_offset = value_stack_map[src];
        GenerateLoad(src_offset, "sp", ptr_reg, rv_program);
    }

    // 申请另一个临时寄存器用于存储索引
    
    if (index->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        string global_name = src->name;
        if (!global_name.empty() && global_name[0] == '@') {
            global_name = global_name.substr(1);
        }
        rv_program.AddInstr(RiscVInstr(RiscVOpcode::LA, {offset_reg, global_name}));
        GenerateLoad(0, offset_reg, offset_reg, rv_program);
    }
    else {
        if (index->kind.tag == KOOPA_RVT_INTEGER) {
            int imm = index->kind.data.integer.value;
            rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {offset_reg, to_string(imm)}));
        }
        else {
            int index_offset = value_stack_map[index];
            GenerateLoad(index_offset, "sp", offset_reg, rv_program);
        }
    }
    
    // 获取元素类型 T 并计算 sizeof(T)
    int sizeof_T = ptr_offset_map[value];
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::LI, {size_reg, to_string(sizeof_T)}));
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::MUL, {offset_reg, offset_reg, size_reg}));
    

    // 计算新的指针地址: new_ptr = base_reg + offset_reg
    rv_program.AddInstr(RiscVInstr(RiscVOpcode::ADD, {ptr_reg, ptr_reg, offset_reg}));

    // 将新指针存储到对应的栈帧位置
    int dst_offset = value_stack_map[value];
    GenerateStore(dst_offset, "sp", ptr_reg, rv_program);

    // 释放临时寄存器
    FreeTempReg();
    FreeTempReg();
    FreeTempReg(); 
}