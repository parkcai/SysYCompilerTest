#ifndef RISCV_H
#define RISCV_H

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "koopa.h"

// 类型别名定义
using RawProgram = koopa_raw_program_t;
using RawSlice = koopa_raw_slice_t;
using RawFunction = koopa_raw_function_t;
using RawBasicBlock = koopa_raw_basic_block_t;
using RawValue = koopa_raw_value_t;
using RawType = koopa_raw_type_t;

class RISCV_Builder {
private:
    // 环境管理类
    class Env {
    private:
        // 存储值到地址的映射
        using AddrMap = std::map<RawValue, int>;
        AddrMap addr_map;

    public:
        bool is_call = false;
        int stack_size = 0;
        int cur_size = 0;

        // 环境初始化
        auto init(int size, bool call) -> void;
        
        // 获取地址映射
        auto get_addr(RawValue value) -> int;
    };

    // 成员变量
    Env env;
    std::ofstream out;

    // 静态工具函数
    static auto func_size(RawFunction func, bool& call) -> int;
    static auto bb_size(RawBasicBlock bb, bool& call, int& max_arg) -> int;
    static auto inst_size(RawValue value) -> int;
    static auto type_size(RawType ty) -> int;
    static auto array_size(RawType value) -> int;

    // 寄存器和栈操作
    auto load_register(RawValue value, std::string reg) -> void;
    auto store_stack(int addr, std::string reg) -> void;

    // 访问者模式实现
    auto raw_visit(const RawProgram& raw) -> void;
    auto raw_visit(const RawSlice& slice) -> void;
    auto raw_visit(const RawFunction& func) -> void;
    auto raw_visit(const RawBasicBlock& bb) -> void;
    auto raw_visit(const RawValue& value) -> void;
    
    // 具体指令访问
    auto raw_visit(const koopa_raw_return_t& return_value) -> void;
    auto raw_visit(const koopa_raw_binary_t& binary_value, int addr) -> void;
    auto raw_visit(const koopa_raw_load_t& load_value, int addr) -> void;
    auto raw_visit(const koopa_raw_store_t& store_value) -> void;
    auto raw_visit(const koopa_raw_branch_t& branch_value) -> void;
    auto raw_visit(const koopa_raw_jump_t& jump_value) -> void;
    auto raw_visit(const koopa_raw_call_t& call_value, int addr) -> void;
    
    // 全局变量和聚合类型处理
    auto global_alloc(const RawValue& global_alloc_value) -> void;
    auto raw_visit(const koopa_raw_aggregate_t& aggregate_value) -> void;
    auto raw_visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr_value, int addr) -> void;
    auto raw_visit(const koopa_raw_get_ptr_t& get_ptr_value, int addr) -> void;

public:
    // 构造函数
    explicit RISCV_Builder(const char* path) : out(path) {}
    
    // 主构建函数
    auto build(RawProgram raw) -> void;
};

#endif  // RISCV_H