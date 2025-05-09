// src/symtab.h
#ifndef SYMTAB_H
#define SYMTAB_H

#include <string>
#include <map>
#include <vector>
#include <memory>

using namespace std;

// 评估结果结构体
struct EvalResult {
    int value;
    bool success;

    EvalResult() : value(0), success(false) {}
    EvalResult(int v) : value(v), success(true) {}
};

// 符号类型枚举
enum class SymbolType {
    CONST,        // 常量
    VAR,          // 变量
    PTR,          // 指针
    FUNC,         // 函数
    ARRAY,        // 变量数组
    CONST_ARRAY   // 常量数组
};

// 辅助函数，将 SymbolType 枚举值转换为字符串
inline string PrintSymbolType(SymbolType type) {
    switch (type) {
        case SymbolType::CONST:        return "CONST";
        case SymbolType::VAR:          return "VAR";
        case SymbolType::PTR:          return "PTR";
        case SymbolType::FUNC:         return "FUNC";
        case SymbolType::ARRAY:        return "ARRAY";
        case SymbolType::CONST_ARRAY:  return "CONST_ARRAY";
        default:                       return "UNKNOWN";
    }
}

// 符号表条目类
class SymbolEntry {
public:
    SymbolType type;

    // 对于常量
    int const_value;

    // 对于变量和数组
    string var_addr;

    // 对于函数
    vector<string> param_types;
    string ret_type;

    // 对于数组和常量数组
    vector<int> array_dimensions;
    int ptr_dim;

    // 构造函数

    // 常量构造函数
    SymbolEntry(int value);

    // 变量构造函数
    SymbolEntry(const string& addr, bool is_ptr, int ptr_dim_);

    // 函数构造函数
    SymbolEntry(const vector<string>& param_types_, const string& ret_type_);

    // 数组和常量数组构造函数
    SymbolEntry(const string& addr, const vector<int>& array_dims, bool is_const);
};

/* 函数声明 */

// 进入一个新的作用域
void EnterScope();

// 退出当前作用域
void ExitScope();

// 获取当前符号表层级
int GetSymtabLevel();
int GetSymtabID();

// 在当前作用域插入符号

// 插入常量
bool InsertConst(const string& ident, const int& value);

// 插入变量（非数组）
bool InsertVar(const string& ident, const string& addr, bool is_ptr=false, int ptr_dim=0);

// 插入数组（常量数组或变量数组）
bool InsertArray(const string& ident, const string& addr, const vector<int>& array_dims, bool is_const);

// 插入全局常量
bool InsertGlobalConst(const string& ident, const int& value);

// 插入全局变量（非数组）
bool InsertGlobalVar(const string& ident, const string& addr, bool is_ptr=false, int ptr_dim=0);

// 插入全局数组（常量数组或变量数组）
bool InsertGlobalArray(const string& ident, const string& addr, const vector<int>& array_dims, bool is_const);

// 查找符号
SymbolEntry* LookupSymbol(const string& ident);

// 插入和查找函数（全局作用域）
bool InsertFunc(const string& ident, const vector<string>& param_types, const string& ret_type);
SymbolEntry* LookupFunc(const string& ident);

// 初始化 SysY 标准库函数
void InitSysYLibFuncs();

#endif // SYMTAB_H
