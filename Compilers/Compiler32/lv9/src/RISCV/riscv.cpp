#include "riscv.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <random>


struct BlockAnalyzer {
    int accumulated_size;
    int max_arguments;
    bool has_call;
    
    BlockAnalyzer() : accumulated_size(0), max_arguments(0), has_call(false) {}
    
    void update_call_info(const koopa_raw_value_t& value) {
        if (value->kind.tag == KOOPA_RVT_CALL) {
            has_call = true;
            size_t arg_count = value->kind.data.call.args.len;
            max_arguments = std::max(max_arguments, static_cast<int>(arg_count));
        }
    }
};


class TypeSizeCalculator {
public:
    // 计算数组大小
    static int compute_array_dimension(koopa_raw_type_t type) {
        return (type->tag == KOOPA_RTT_ARRAY) 
            ? compute_array_dimension(type->data.array.base) * type->data.array.len 
            : 4;
    }

    // 计算类型大小
    static int calculate(koopa_raw_type_t type) {
        if (type->tag == KOOPA_RTT_ARRAY) {
            return compute_array_dimension(type);
        }
        
        switch (type->tag) {
            case KOOPA_RTT_INT32:
            case KOOPA_RTT_POINTER:
                return 4;
            case KOOPA_RTT_UNIT:
                return 0;
            default:
                return 0;
        }
    }
};

int RISCV_Builder::func_size(koopa_raw_function_t func, bool& call) {
    BlockAnalyzer analyzer;
    
    // 处理所有基本块
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto block_ptr = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        analyzer.accumulated_size += bb_size(block_ptr, analyzer.has_call, 
                                          analyzer.max_arguments);
    }
    
    // 计算总大小
    int total_size = analyzer.accumulated_size;
    total_size += analyzer.max_arguments * 4;
    
    // 处理参数
    int param_overhead = 0;
    if (func->params.len > 8) {
        param_overhead = (func->params.len - 8) * 4;
    }
    total_size += param_overhead;
    
    // 设置调用标志并返回
    call = analyzer.has_call;
    return total_size + (analyzer.has_call ? 4 : 0);
}

int RISCV_Builder::bb_size(koopa_raw_basic_block_t bb, bool& call, int& max_arg) {
    BlockAnalyzer analyzer;
    
    // 遍历指令
    for (size_t idx = 0; idx < bb->insts.len; ++idx) {
        auto inst_value = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[idx]);
        analyzer.update_call_info(inst_value);
        analyzer.accumulated_size += inst_size(inst_value);
    }
    
    // 更新外部状态
    call |= analyzer.has_call;
    max_arg = std::max(max_arg, analyzer.max_arguments);
    
    return analyzer.accumulated_size;
}

int RISCV_Builder::inst_size(koopa_raw_value_t inst) {
    if (inst->kind.tag == KOOPA_RVT_ALLOC) {
        return TypeSizeCalculator::calculate(inst->ty->data.pointer.base);
    }
    return TypeSizeCalculator::calculate(inst->ty);
}

int RISCV_Builder::type_size(koopa_raw_type_t ty) {
    return TypeSizeCalculator::calculate(ty);
}

int RISCV_Builder::array_size(koopa_raw_type_t ty) {
    return TypeSizeCalculator::compute_array_dimension(ty);
}

void RISCV_Builder::Env::init(int size, bool call) {
    stack_size = size;
    is_call = call;
    cur_size = 0;
    addr_map.clear();
}

int RISCV_Builder::Env::get_addr(koopa_raw_value_t raw) {
    // 检查缓存
    auto iter = addr_map.find(raw);
    if (iter != addr_map.end()) {
        return iter->second;
    }
    
    // 计算新地址
    int size = inst_size(raw);
    if (size == 0) {
        return -1;
    }
    
    // 更新映射
    int new_addr = cur_size;
    addr_map[raw] = new_addr;
    cur_size += size;
    
    return new_addr;
}


////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::load_register(koopa_raw_value_t value, std::string reg) {
    // 处理整数常量的情况
    const bool is_integer = value->kind.tag == KOOPA_RVT_INTEGER;
    if (is_integer) {
        const int32_t val = value->kind.data.integer.value;
        out << "  li " << reg << ", " << val << "\n";
        return;
    }
    
    // 获取内存地址
    const int memory_loc = env.get_addr(value);
    if (memory_loc == -1) {
        assert(false);
        return;
    }
    
    // 判断偏移量范围并生成相应指令
    const bool is_small_offset = (memory_loc >= -2048 && memory_loc < 2048);
    if (is_small_offset) {
        out << "  lw " << reg << ", " << memory_loc << "(sp)\n";
    } else {
        const std::string temp_reg = "t3";
        out << "  li " << temp_reg << ", " << memory_loc << "\n"
            << "  add " << temp_reg << ", sp, " << temp_reg << "\n"
            << "  lw " << reg << ", 0(" << temp_reg << ")\n";
    }
}

void RISCV_Builder::store_stack(int addr, std::string reg) {
    // 检查地址有效性
    if (addr == -1) {
        assert(false);
        return;
    }
    
    // 根据偏移量大小选择存储策略
    const bool within_immediate_range = (addr >= -2048 && addr < 2048);
    if (within_immediate_range) {
        out << "  sw " << reg << ", " << addr << "(sp)\n";
    } else {
        const std::string addr_reg = "t3";
        out << "  li " << addr_reg << ", " << addr << "\n"
            << "  add " << addr_reg << ", sp, " << addr_reg << "\n"
            << "  sw " << reg << ", 0(" << addr_reg << ")\n";
    }
}

void RISCV_Builder::raw_visit(const koopa_raw_program_t &raw) {
    // 处理数据段
    const bool has_data = raw.values.len > 0;
    if (has_data) {
        out << "  .data\n";
        raw_visit(raw.values);
    }
    
    // 处理代码段
    out << "\n  .text\n";
    raw_visit(raw.funcs);
}

void RISCV_Builder::raw_visit(const koopa_raw_slice_t &slice) {
    // 遍历并处理每个元素
    for (size_t i = 0; i < slice.len; ++i) {
        const auto ptr = slice.buffer[i];
        
        // 根据类型分发到对应的处理函数
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                raw_visit(reinterpret_cast<const koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                raw_visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                raw_visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                assert(false);
        }
    }
}

void RISCV_Builder::raw_visit(const koopa_raw_function_t &func) {
    // 跳过空函数
    if (func->bbs.len == 0) return;
    
    // 输出函数声明
    const std::string func_name(func->name + 1);
    out << "\n  .globl  " << func_name << "\n"
        << func_name << ":\n";
    
    // 计算栈帧大小
    bool requires_call = false;
    int frame_size = func_size(func, requires_call);
    frame_size = ((frame_size + 15) / 16 * 16) * 2;
    
    // 调整栈指针
    const bool small_frame = (frame_size < 2048 && frame_size >= -2048);
    if (frame_size > 0) {
        if (small_frame) {
            out << "  addi sp, sp, -" << frame_size << "\n";
        } else {
            out << "  li t0, -" << frame_size << "\n"
                << "  add sp, sp, t0\n";
        }
    }
    
    // 保存返回地址
    if (requires_call) {
        const int ra_offset = frame_size - 4;
        if (ra_offset < 2048 && ra_offset >= -2048) {
            out << "  sw ra, " << ra_offset << "(sp)\n";
        } else {
            out << "  li t0, " << ra_offset << "\n"
                << "  add t0, sp, t0\n"
                << "  sw ra, 0(t0)\n";
        }
    }
    
    // 初始化环境
    env.init(frame_size, requires_call);
    env.stack_size -= requires_call ? 4 : 0;
    env.cur_size += (func->params.len > 8 ? func->params.len - 8 : 0) * 4;
    
    // 处理函数体
    raw_visit(func->bbs);
}


////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::raw_visit(const koopa_raw_basic_block_t &bb) {
    // 提取基本块名称并移除前导字符
    const std::string block_name = bb->name + 1;
    
    // 仅在非入口块时输出标签
    if (block_name.compare("entry") != 0) {
        out << block_name << ":\n";
    }
    
    // 处理基本块中的指令
    raw_visit(bb->insts);
}

void RISCV_Builder::raw_visit(const koopa_raw_value_t &value) {
    // 获取值的类型和地址
    const auto tag = value->kind.tag;
    const int memory_addr = env.get_addr(value);
    
    // 使用switch处理不同类型的指令
    switch (tag) {
        case KOOPA_RVT_RETURN: {
            raw_visit(value->kind.data.ret);
            break;
        }
        case KOOPA_RVT_LOAD: {
            raw_visit(value->kind.data.load, memory_addr);
            break;
        }
        case KOOPA_RVT_STORE: {
            raw_visit(value->kind.data.store);
            break;
        }
        case KOOPA_RVT_BINARY: {
            raw_visit(value->kind.data.binary, memory_addr);
            break;
        }
        case KOOPA_RVT_BRANCH: {
            raw_visit(value->kind.data.branch);
            break;
        }
        case KOOPA_RVT_JUMP: {
            raw_visit(value->kind.data.jump);
            break;
        }
        case KOOPA_RVT_CALL: {
            raw_visit(value->kind.data.call, memory_addr);
            break;
        }
        case KOOPA_RVT_GLOBAL_ALLOC: {
            global_alloc(value);
            break;
        }
        case KOOPA_RVT_GET_ELEM_PTR: {
            raw_visit(value->kind.data.get_elem_ptr, memory_addr);
            break;
        }
        case KOOPA_RVT_GET_PTR: {
            raw_visit(value->kind.data.get_ptr, memory_addr);
            break;
        }
        case KOOPA_RVT_INTEGER:
        case KOOPA_RVT_ALLOC:
            break;
        default:
            assert(false);
    }
}

void RISCV_Builder::raw_visit(const koopa_raw_return_t &ret_value) {
    // 处理返回值
    if (ret_value.value) {
        load_register(ret_value.value, "a0");
    }
    
    // 计算栈相关参数
    const int base_stack_size = env.stack_size;
    const int additional_stack = env.is_call ? 4 : 0;
    const int total_stack_size = base_stack_size + additional_stack;
    const bool is_small_stack = (base_stack_size >= -2048 && base_stack_size < 2048);
    
    // 如果有调用，恢复返回地址
    if (env.is_call) {
        if (is_small_stack) {
            out << "  lw ra, " << base_stack_size << "(sp)\n";
        } else {
            out << "  li t0, " << base_stack_size << "\n"
                << "  add t0, sp, t0\n"
                << "  lw ra, 0(t0)\n";
        }
    }
    
    // 调整栈指针
    if (total_stack_size > 0) {
        if (total_stack_size < 2048) {
            out << "  addi sp, sp, " << total_stack_size << "\n";
        } else {
            out << "  li t0, " << total_stack_size << "\n"
                << "  add sp, sp, t0\n";
        }
    }
    
    // 返回指令
    out << "  ret\n";
}


////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::raw_visit(const koopa_raw_binary_t &b_value, int addr) {
    // 定义寄存器映射关系
    const char* result_reg = "t0";
    const char* first_op = "t0";
    const char* second_op = "t1";
    
    // 构建指令模板映射表
    static const struct {
        koopa_raw_binary_op_t op_type;
        const char* template_str;
        bool needs_extra;
    } instruction_table[] = {
        {KOOPA_RBO_ADD, "  add %s, %s, %s\n", false},
        {KOOPA_RBO_SUB, "  sub %s, %s, %s\n", false},
        {KOOPA_RBO_MUL, "  mul %s, %s, %s\n", false},
        {KOOPA_RBO_DIV, "  div %s, %s, %s\n", false},
        {KOOPA_RBO_MOD, "  rem %s, %s, %s\n", false},
        {KOOPA_RBO_AND, "  and %s, %s, %s\n", false},
        {KOOPA_RBO_OR,  "  or %s, %s, %s\n",  false},
        {KOOPA_RBO_EQ,  "  xor %s, %s, %s\n", true},
        {KOOPA_RBO_NOT_EQ, "  xor %s, %s, %s\n", true},
        {KOOPA_RBO_GT,  "  sgt %s, %s, %s\n", false},
        {KOOPA_RBO_LT,  "  slt %s, %s, %s\n", false},
        {KOOPA_RBO_GE,  "  slt %s, %s, %s\n", true},
        {KOOPA_RBO_LE,  "  sgt %s, %s, %s\n", true}
    };

    // 加载操作数到寄存器
    load_register(b_value.lhs, first_op);
    load_register(b_value.rhs, second_op);

    // 指令生成
    bool found = false;
    for(const auto& instr : instruction_table) {
        if(instr.op_type == b_value.op) {
            // 格式化基础指令
            char buffer[50];
            snprintf(buffer, sizeof(buffer), instr.template_str, 
                    result_reg, first_op, second_op);
            out << buffer;
            
            // 处理需要额外指令的情况
            if(instr.needs_extra) {
                const char* extra_op = nullptr;
                switch(b_value.op) {
                    case KOOPA_RBO_EQ:
                        extra_op = "seqz";
                        break;
                    case KOOPA_RBO_NOT_EQ:
                        extra_op = "snez";
                        break;
                    case KOOPA_RBO_GE:
                    case KOOPA_RBO_LE:
                        extra_op = "seqz";
                        break;
                    default:
                        break;
                }
                if(extra_op) {
                    snprintf(buffer, sizeof(buffer), "  %s %s, %s\n",
                            extra_op, result_reg, result_reg);
                    out << buffer;
                }
            }
            found = true;
            break;
        }
    }

    // 确保操作类型有效
    if(!found) {
        // 处理未知操作类型
        return;
    }

    // 存储结果
    store_stack(addr, result_reg);
}


////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::raw_visit(const koopa_raw_store_t &s_value) {
    // 寄存器别名定义
    const char temp_reg[] = "t0";
    const char addr_reg[] = "t1";
    const char aux_reg[] = "t3";
    
    auto handle_global_store = [&](const char* tr, const char* ar) {
        std::string target_name(s_value.dest->name + 1);
        out << "  la " << ar << ", " << target_name << "\n";
        load_register(s_value.value, tr);
        out << "  sw " << tr << ", 0(" << ar << ")\n";
    };
    
    auto handle_pointer_store = [&](const char* tr, const char* ar) {
        load_register(s_value.dest, ar);
        load_register(s_value.value, tr);
        out << "  sw " << tr << ", 0(" << ar << ")\n";
    };
    
    auto handle_arg_store = [&](int addr, const char* tr, const char* ar) {
        int arg_index = s_value.value->kind.data.func_arg_ref.index;
        
        if (arg_index < 8) {
            std::string arg_reg = "a" + std::to_string(arg_index);
            store_stack(addr, arg_reg);
            return;
        }
        
        int offset = (arg_index - 8) * 4;
        if (offset >= -2048 && offset < 2048) {
            out << "  lw " << tr << ", " << offset << "(sp)\n";
        } else {
            out << "  li " << ar << ", " << offset << "\n";
            out << "  add " << ar << ", sp, " << ar << "\n";
            out << "  lw " << tr << ", 0(" << ar << ")\n";
        }
        store_stack(addr, tr);
    };
    
    // 处理全局分配情况
    if (s_value.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        handle_global_store(temp_reg, addr_reg);
        return;
    }
    
    // 处理指针操作情况
    if (s_value.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || 
        s_value.dest->kind.tag == KOOPA_RVT_GET_PTR) {
        handle_pointer_store(temp_reg, addr_reg);
        return;
    }
    
    // 处理其他情况
    int target_addr = env.get_addr(s_value.dest);
    
    // 处理函数参数引用
    if (s_value.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        handle_arg_store(target_addr, temp_reg, aux_reg);
        return;
    }
    
    // 处理普通情况
    load_register(s_value.value, temp_reg);
    store_stack(target_addr, temp_reg);
}

void RISCV_Builder::raw_visit(const koopa_raw_load_t &l_value, int addr) {
    // 寄存器别名定义
    const char load_reg[] = "t0";
    
    auto handle_global_load = [&](const char* reg) {
        std::string target_name(l_value.src->name + 1);
        out << "  la " << reg << ", " << target_name << "\n";
        out << "  lw " << reg << ", 0(" << reg << ")\n";
    };
    
    auto handle_pointer_load = [&](const char* reg) {
        load_register(l_value.src, reg);
        out << "  lw " << reg << ", 0(" << reg << ")\n";
    };
    
    // 根据源类型选择加载策略
    if (l_value.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        handle_global_load(load_reg);
    }
    else if (l_value.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
             l_value.src->kind.tag == KOOPA_RVT_GET_PTR) {
        handle_pointer_load(load_reg);
    }
    else {
        load_register(l_value.src, load_reg);
    }
    
    // 存储结果
    store_stack(addr, load_reg);
}

////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::raw_visit(const koopa_raw_branch_t &b_value) {
    // 条件跳转的优化实现
    const std::string condition_reg = "t0";
    const auto process_branch = [this](const char* src_name, 
                                     const char* dest_name,
                                     bool is_tmp = false) {
        std::string processed_name(src_name + 1);
        return processed_name + (is_tmp ? "_tmp" : "");
    };
    
    // 加载条件值
    load_register(b_value.cond, condition_reg.c_str());
    
    // 生成跳转指令序列
    const std::string true_label = process_branch(b_value.true_bb->name, nullptr);
    const std::string false_label = process_branch(b_value.false_bb->name, nullptr);
    const std::string temp_label = true_label + "_tmp";
    
    std::vector<std::string> instructions = {
        "  bnez " + condition_reg + ", " + temp_label,
        "  j " + false_label,
        temp_label + ":",
        "  j " + true_label
    };
    
    // 输出生成的指令
    for(const auto& instr : instructions) {
        out << instr + "\n";
    }
}

void RISCV_Builder::raw_visit(const koopa_raw_jump_t &j_value) {
    // 无条件跳转的实现
    const auto generate_jump = [this](const char* target) {
        return "  j " + std::string(target + 1);
    };
    
    out << generate_jump(j_value.target->name) << "\n";
}

void RISCV_Builder::raw_visit(const koopa_raw_call_t &c_value, int addr) {
    // 函数调用的优化实现
    const int MAX_REG_ARGS = 8;
    const std::string TEMP_REG = "t0";
    
    // 处理寄存器参数
    for(int i = 0; i < std::min(static_cast<int>(c_value.args.len), MAX_REG_ARGS); ++i) {
        auto arg_ptr = reinterpret_cast<koopa_raw_value_t>(c_value.args.buffer[i]);
        load_register(arg_ptr, "a" + std::to_string(i));
    }
    
    // 计算栈大小
    bool has_call = false;
    int total_size = func_size(c_value.callee, has_call);
    total_size = ((total_size + 15) / 16) * 32;  // 对齐优化
    
    // 处理栈参数
    for(int i = MAX_REG_ARGS; i < c_value.args.len; ++i) {
        auto arg_ptr = reinterpret_cast<koopa_raw_value_t>(c_value.args.buffer[i]);
        load_register(arg_ptr, TEMP_REG.c_str());
        store_stack((i - MAX_REG_ARGS) * 4 - total_size, TEMP_REG.c_str());
    }
    
    // 生成函数调用
    std::string func_name = std::string(c_value.callee->name + 1);
    out << "  call " << func_name << "\n";
    
    // 处理返回值
    if(addr != -1) {
        store_stack(addr, "a0");
    }
}

void RISCV_Builder::global_alloc(const koopa_raw_value_t &g_value) {
    // 全局变量分配的优化实现
    const auto emit_directive = [this](const std::string& directive, 
                                     const std::string& value) {
        out << "  " << directive << " " << value << "\n";
    };
    
    const auto get_global_name = [](const char* name) {
        return std::string(name + 1);
    };
    
    // 输出全局标签
    std::string var_name = get_global_name(g_value->name);
    out << "\n  .global " << var_name << "\n";
    out << var_name << ":\n";
    
    // 处理不同类型的初始化
    auto init = g_value->kind.data.global_alloc.init;
    if (init->kind.tag == KOOPA_RVT_INTEGER) {
        emit_directive(".word", 
            std::to_string(init->kind.data.integer.value));
    }
    else if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
        emit_directive(".zero", 
            std::to_string(type_size(g_value->ty->data.pointer.base)));
    }
    else if (init->kind.tag == KOOPA_RVT_AGGREGATE) {
        raw_visit(init->kind.data.aggregate);
    }
}


////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::raw_visit(const koopa_raw_aggregate_t &agg_value) {
    const auto process_integer = [this](int64_t val) {
        out << "  .word " + std::to_string(val) + "\n";
    };
    
    const auto process_element = [this, &process_integer]
                               (const koopa_raw_value_t& elem) {
        switch(elem->kind.tag) {
            case KOOPA_RVT_INTEGER: {
                process_integer(elem->kind.data.integer.value);
                return true;
            }
            case KOOPA_RVT_AGGREGATE: {
                raw_visit(elem->kind.data.aggregate);
                return true;
            }
            default:
                return false;
        }
    };
    
    // 使用循环方式处理数组元素
    for (size_t i = 0; i < agg_value.elems.len; ++i) {
        auto current_elem = reinterpret_cast<koopa_raw_value_t>
                          (agg_value.elems.buffer[i]);
        
        if (!process_element(current_elem)) {
            assert(false);
        }
    }
}

void RISCV_Builder::raw_visit(const koopa_raw_get_elem_ptr_t &gep_value,
                             int addr) {
    // 常量定义
    const int SMALL_OFFSET_LIMIT = 2048;
    const int NEGATIVE_OFFSET_LIMIT = -2048;
    const char* BASE_REG = "t0";
    const char* INDEX_REG = "t1";
    const char* SIZE_REG = "t2";
    const char* TEMP_REG = "t3";
    
    // 源地址处理
    auto handle_src_address = [&]() {
        if (gep_value.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            out << "  la " << BASE_REG << ", " 
                << std::string(gep_value.src->name + 1) << "\n";
            return;
        }
        
        int src_addr = env.get_addr(gep_value.src);
        if (src_addr == -1) {
            assert(false);
            return;
        }
        
        // 地址加载优化
        if (src_addr >= NEGATIVE_OFFSET_LIMIT && src_addr < SMALL_OFFSET_LIMIT) {
            out << "  addi " << BASE_REG << ", sp, " 
                << std::to_string(src_addr) << "\n";
        } else {
            out << "  li " << TEMP_REG << ", " 
                << std::to_string(src_addr) << "\n";
            out << "  add " << BASE_REG << ", sp, " << TEMP_REG << "\n";
        }
        
        // 处理特殊情况
        bool needs_load = (gep_value.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
                          gep_value.src->kind.tag == KOOPA_RVT_GET_PTR);
        if (needs_load) {
            out << "  lw " << BASE_REG << ", 0(" << BASE_REG << ")\n";
        }
    };
    
    // 计算偏移量
    auto calculate_offset = [&]() {
        // 加载索引
        load_register(gep_value.index, INDEX_REG);
        
        // 计算基本类型大小
        int elem_size = array_size(
            gep_value.src->ty->data.pointer.base->data.array.base
        );
        
        // 生成偏移量计算指令
        out << "  li " << SIZE_REG << ", " 
            << std::to_string(elem_size) << "\n";
        out << "  mul " << INDEX_REG << ", " 
            << INDEX_REG << ", " << SIZE_REG << "\n";
        out << "  add " << BASE_REG << ", " 
            << BASE_REG << ", " << INDEX_REG << "\n";
    };
    
    // 执行主要逻辑
    handle_src_address();
    calculate_offset();
    store_stack(addr, BASE_REG);
}


////////////////////////////////////////////////////////////////////////////////

void RISCV_Builder::raw_visit(const koopa_raw_get_ptr_t &gp_value, int addr) {
    // 寄存器分配表
    struct RegInfo {
        const char* base;    // 基址寄存器
        const char* offset;  // 偏移寄存器
        const char* temp;    // 临时寄存器
        const char* size;    // 大小寄存器
    };
    
    // 设置使用的寄存器
    const RegInfo regs = {
        "t0",   // 基址寄存器
        "t1",   // 偏移寄存器
        "t3",   // 临时寄存器
        "t2"    // 大小寄存器
    };
    
    // 地址范围检查和处理
    auto handle_address = [this, &regs](int address) -> void {
        const int ADDR_THRESHOLD = 2048;
        
        // 地址无效检查
        if (address == -1) {
            assert(false);
            return;
        }
        
        // 根据地址范围选择不同的加载策略
        bool use_direct_load = (address > -ADDR_THRESHOLD && 
                              address < ADDR_THRESHOLD);
                              
        if (use_direct_load) {
            // 直接加载
            out << "  addi " << regs.base << ", sp, " 
                << std::to_string(address) << "\n";
        } else {
            // 间接加载
            out << "  li " << regs.temp << ", " 
                << std::to_string(address) << "\n";
            out << "  add " << regs.base << ", sp, " 
                << regs.temp << "\n";
        }
    };
    
    // 计算偏移量
    auto compute_offset = [this, &regs, &gp_value]() -> void {
        // 获取元素大小
        int element_size = array_size(gp_value.src->ty->data.pointer.base);
        
        // 加载索引到寄存器
        load_register(gp_value.index, regs.offset);
        
        // 计算实际偏移量
        out << "  li " << regs.size << ", " 
            << std::to_string(element_size) << "\n";
        out << "  mul " << regs.offset << ", " 
            << regs.offset << ", " << regs.size << "\n";
    };
    
    // 完成指针计算
    auto finalize_pointer = [this, &regs]() -> void {
        out << "  add " << regs.base << ", " 
            << regs.base << ", " << regs.offset << "\n";
    };
    
    // 主要执行逻辑
    int source_addr = env.get_addr(gp_value.src);
    handle_address(source_addr);
    
    // 加载指针值
    out << "  lw " << regs.base << ", 0(" << regs.base << ")\n";
    
    // 计算最终地址
    compute_offset();
    finalize_pointer();
    
    // 存储结果
    store_stack(addr, regs.base);
}

void RISCV_Builder::build(koopa_raw_program_t raw) {
    // 处理程序并关闭输出流
    auto process_and_close = [this](koopa_raw_program_t program) {
        raw_visit(program);
        out.close();
    };
    
    process_and_close(raw);
}