// src/symtab.cpp

#include "symtab.h"
#include <stdexcept>

using namespace std;

int symtab_id = 0;

// 符号表栈，用于管理作用域
static vector<pair<int, map<string, SymbolEntry>>> symtab_stack;

// 全局符号表（包含变量、常量和函数）
static map<string, SymbolEntry> global_symtab;

// SymbolEntry 构造函数实现

// 常量构造函数
SymbolEntry::SymbolEntry(int value)
    : type(SymbolType::CONST), const_value(value), var_addr(""), param_types(), ret_type(), array_dimensions(), ptr_dim(0) {}

// 变量构造函数
SymbolEntry::SymbolEntry(const string& addr, bool is_ptr, int ptr_dim_)
    : type(is_ptr ? SymbolType::PTR : SymbolType::VAR), const_value(0), var_addr(addr), param_types(), ret_type(), array_dimensions(), ptr_dim(ptr_dim_) {}

// 函数构造函数
SymbolEntry::SymbolEntry(const vector<string>& param_types_, const string& ret_type_)
    : type(SymbolType::FUNC), const_value(0), var_addr(""), param_types(param_types_), ret_type(ret_type_), array_dimensions(), ptr_dim(0) {}

// 数组和常量数组构造函数
SymbolEntry::SymbolEntry(const string& addr, const vector<int>& array_dims, bool is_const)
    : type(is_const ? SymbolType::CONST_ARRAY : SymbolType::ARRAY), const_value(0), var_addr(addr), param_types(), ret_type(), array_dimensions(array_dims), ptr_dim(0) {}

/* 函数实现 */

// 进入一个新的作用域
void EnterScope() {
    symtab_stack.emplace_back(symtab_id, map<string, SymbolEntry>());
    symtab_id++;
}

// 退出当前作用域
void ExitScope() {
    if (!symtab_stack.empty()) {
        symtab_stack.pop_back();
    } else {
        throw runtime_error("Error: No scope to exit.");
    }
}

// 获取当前符号表层级
int GetSymtabLevel() {
    return symtab_stack.size();
}

int GetSymtabID(){
    return symtab_stack.back().first;
}

// 插入常量
bool InsertConst(const string& ident, const int& value) {
    if (symtab_stack.empty()) {
        return InsertGlobalConst(ident, value);
    }
    auto& current_scope = symtab_stack.back();

    // 检查当前作用域中是否已定义
    if (current_scope.second.find(ident) != current_scope.second.end()) {
        throw runtime_error("Constant redefinition in the same scope: " + ident);
    }

    // 插入符号
    current_scope.second.emplace(ident, SymbolEntry(value));
    return true;
}

// 插入全局常量
bool InsertGlobalConst(const string& ident, const int& value) {
    if (global_symtab.find(ident) != global_symtab.end()) {
        throw runtime_error("Global constant redefinition: " + ident);
    }

    global_symtab.emplace(ident, SymbolEntry(value));
    return true;
}

// 插入变量（非数组）
bool InsertVar(const string& ident, const string& addr, bool is_ptr, int ptr_dim) {
    if (symtab_stack.empty()) {
        return InsertGlobalVar(ident, addr, is_ptr, ptr_dim);
    }
    auto& current_scope = symtab_stack.back();

    // 检查当前作用域中是否已定义
    if (current_scope.second.find(ident) != current_scope.second.end()) {
        throw runtime_error("Variable redefinition in the same scope: " + ident);
    }

    // 插入符号
    current_scope.second.emplace(ident, SymbolEntry(addr, is_ptr, ptr_dim));
    return true;
}

// 插入数组（常量数组或变量数组）
bool InsertArray(const string& ident, const string& addr, const vector<int>& array_dims, bool is_const) {
    if (symtab_stack.empty()) {
        return InsertGlobalArray(ident, addr, array_dims, is_const);
    }
    auto& current_scope = symtab_stack.back();

    // 检查当前作用域中是否已定义
    if (current_scope.second.find(ident) != current_scope.second.end()) {
        throw runtime_error("Array redefinition in the same scope: " + ident);
    }

    // 插入符号
    current_scope.second.emplace(ident, SymbolEntry(addr, array_dims, is_const));
    return true;
}

// 插入全局变量（非数组）
bool InsertGlobalVar(const string& ident, const string& addr, bool is_ptr, int ptr_dim) {
    if (global_symtab.find(ident) != global_symtab.end()) {
        throw runtime_error("Global variable redefinition: " + ident);
    }

    global_symtab.emplace(ident, SymbolEntry(addr, is_ptr, ptr_dim));
    return true;
}

// 插入全局数组（常量数组或变量数组）
bool InsertGlobalArray(const string& ident, const string& addr, const vector<int>& array_dims, bool is_const) {
    if (global_symtab.find(ident) != global_symtab.end()) {
        throw runtime_error("Global array redefinition: " + ident);
    }

    global_symtab.emplace(ident, SymbolEntry(addr, array_dims, is_const));
    return true;
}

// 查找符号（变量、常量和数组）
SymbolEntry* LookupSymbol(const string& ident) {
    // 从内层作用域到外层作用域查找
    for (auto it = symtab_stack.rbegin(); it != symtab_stack.rend(); ++it) {
        map<string, SymbolEntry>& current_symtab = it->second;
        auto found = current_symtab.find(ident);
        if (found != current_symtab.end()) {
            return &(found->second);
        }
    }
    // 查找全局符号
    auto found = global_symtab.find(ident);
    if (found != global_symtab.end()) {
        return &(found->second);
    }
    return nullptr; // 未找到
}

// 插入函数到全局符号表
bool InsertFunc(const string& ident, const vector<string>& param_types, const string& ret_type) {
    // 检查全局符号表中是否已定义
    if (global_symtab.find(ident) != global_symtab.end()) {
        throw runtime_error("Function redefinition: " + ident);
    }

    // 插入函数
    global_symtab.emplace(ident, SymbolEntry(param_types, ret_type));
    return true;
}

// 查找函数
SymbolEntry* LookupFunc(const string& ident) {
    auto found = global_symtab.find(ident);
    if (found != global_symtab.end()) {
        if (found->second.type == SymbolType::FUNC) {
            return &(found->second);
        }
    }
    return nullptr; // 未找到或类型不符
}

// 初始化 SysY 标准库函数
void InitSysYLibFuncs() {
    // getint(): i32
    InsertFunc("getint", {}, "int");
    // getch(): i32
    InsertFunc("getch", {}, "int");
    // getarray(*i32): i32
    // 对于数组参数可采用 "*int" 或类似特殊标记，本例使用 "*int"
    InsertFunc("getarray", {"*int"}, "int");
    // putint(i32)
    InsertFunc("putint", {"int"}, "void");
    // putch(i32)
    InsertFunc("putch", {"int"}, "void");
    // putarray(i32, *i32)
    InsertFunc("putarray", {"int","*int"}, "void");
    // starttime()
    InsertFunc("starttime", {}, "void");
    // stoptime()
    InsertFunc("stoptime", {}, "void");
}
