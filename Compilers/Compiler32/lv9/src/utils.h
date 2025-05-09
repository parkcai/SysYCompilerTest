#ifndef UTILS_H
#define UTILS_H

#include "koopa.h"
#include <map>
#include <string>
#include <vector>

// 类型别名定义 - 第一部分
using RawValue = koopa_raw_value_t;
using RawFunction = koopa_raw_function_t;
using RawBlock = koopa_raw_basic_block_t;
using RawBlockData = koopa_raw_basic_block_data_t;
using RawSlice = koopa_raw_slice_t;
using RawSliceKind = koopa_raw_slice_item_kind_t;
using RawTypeKind = koopa_raw_type_kind;
using RawTypeTag = koopa_raw_type_tag_t;
using RawValueData = koopa_raw_value_data;




// 值类型枚举
enum class ValueType { 
    Const, 
    Var, 
    Func, 
    Array, 
    Pointer
};

// 值的数据结构
struct Value {
    ValueType type;
    union SymbolListValue {
        int const_value;
        RawValue var_value;
        RawFunction func_value;
        RawValue array_value;
        RawValue pointer_value;
    } data;

    Value() = default;
    
    Value(ValueType type, int value) : type(type) { 
        data.const_value = value; 
    }
    
    Value(ValueType type, RawValue value) : type(type) {
        switch(type) {
            case ValueType::Var:
                data.var_value = value;
                break;
            case ValueType::Array:
                data.array_value = value;
                break;
            case ValueType::Pointer:
                data.pointer_value = value;
                break;
            default:
                break;
        }
    }
    
    Value(ValueType type, RawFunction value) : type(type) {
        data.func_value = value;
    }
};

// Value 定义之后的类型别名
using StringMap = std::map<std::string, Value>;

// 符号表管理类
class SymbolList {
private:
    std::vector<StringMap> symbol_list_vector;

public:
    ~SymbolList() = default;
    
    auto addSymbol(std::string symbol, Value value) -> void;
    auto getSymbol(std::string symbol) -> Value;
    auto newScope() -> void;
    auto delScope() -> void;
};

// 基本块管理类
class BlockManager {
private:
    std::vector<const void*>* block_list_vector;
    std::vector<const void*> tmp_inst_list;

public:
    auto init(std::vector<const void*>* block_list_vector) -> void;
    auto newBlock(RawBlockData* basic_block) -> void;
    auto delBlock() -> void;
    auto addInst(const void* inst) -> void;
    auto delUnreachableBlock() -> void;
    auto checkBlock() -> bool;
};

// 循环管理类
class LoopManager {
private:
    struct While {
        RawBlock head;
        RawBlock tail;
        
        While(RawBlock head, RawBlock tail)
            : head(head), tail(tail) {}
    };
    
    std::vector<While> while_list;

public:
    auto addWhile(RawBlock head, RawBlock tail) -> void;
    auto delWhile() -> void;
    auto getHead() -> RawBlock;
    auto getTail() -> RawBlock;
};

// 工具函数声明
auto slice(RawSliceKind kind = KOOPA_RSIK_UNKNOWN) -> RawSlice;
auto slice(std::vector<const void*>& vec, RawSliceKind kind = KOOPA_RSIK_UNKNOWN) -> RawSlice;
auto slice(const void* data, RawSliceKind kind = KOOPA_RSIK_UNKNOWN) -> RawSlice;

auto type_kind(RawTypeTag tag) -> RawTypeKind*;
auto pointer_type_kind(RawTypeTag tag) -> RawTypeKind*;
auto array_type_kind(RawTypeTag tag, std::vector<size_t> size_vec) -> RawTypeKind*;

auto jump_value(RawBlock tar) -> RawValueData*;
auto ret_value(RawTypeTag tag) -> RawValueData*;
auto zero_init(RawTypeKind* type) -> RawValueData*;

#endif // UTILS_H