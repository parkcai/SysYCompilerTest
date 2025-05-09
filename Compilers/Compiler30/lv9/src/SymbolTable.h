#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

enum Type {
    TYPE_CONST,
    TYPE_VAR,
    TYPE_ARRAY,
    TYPE_PTR,
    TYPE_FUN_INT,
    TYPE_FUN_VOID,
};

struct Var {
    Type type;
    int value;
    std::string ident;
    int alloc_id;
    int dimension;
};

void func_insert(std::string, Type);

Type func_type(std::string);

void reset_alloc_id();

/*
void backup_symbol_table(symbol_table_map_t &);

void restore_symbol_table(symbol_table_map_t &);
*/
// 进入新的作用域
void enter_new_scope();

// 离开作用域
void exit_scope();

// 当前作用域
int current_scope();

int symbol_scope(std::string ident);

std::string alloc_str(std::string ident);

// 插入符号
void insert_symbol(std::string ident, Type type, int value = 0, int dimension = 0);

// 判断某个符号是否存在
bool is_exist(std::string ident);

// 查找某元素
std::shared_ptr<Var> find_symbol(std::string ident);

inline void printErr(const int& err_code, const std::string& ident);

inline void printErr();
