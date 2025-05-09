#pragma once

#include <string>
#include <memory>
#include <unordered_map>

enum SymType {
    S_CONST, S_VAR, S_UNDEF, S_FUNC, S_ARRAY, S_POINTER
};

enum FuncType {
    F_INT, F_VOID
};

struct Symbol
{
    SymType type;
    std::string name;
    int value;
};

struct SymbolTable
{
    std::unordered_map<std::string, std::shared_ptr<Symbol>> table;
    int cnt;
    SymbolTable *parent;
};

struct LoopTable
{
    int if_cnt;
    LoopTable *parent;
};

void enter_loop(int if_cnt);
void exit_loop( );
int cur_loop_cnt();

void init_symtab();
void destroy_symtab();
int cur_symtab_cnt();

void enter_block();
void exit_block();

std::shared_ptr<Symbol> symbol_insert(const std::string &name, SymType type, int value);

bool symbol_exist(const std::string &name);

std::shared_ptr<Symbol> symbol_query(const std::string &name);