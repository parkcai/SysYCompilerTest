
#include <string>
#include <memory>
#include <cassert>
#include <set>
#include "symtab.hpp"

using namespace std;

static LoopTable *cur_loop_tab;

void enter_loop(int if_cnt) {
    LoopTable *parent_loop_tab = cur_loop_tab;
    cur_loop_tab = new LoopTable();
    cur_loop_tab->parent = parent_loop_tab;
    cur_loop_tab->if_cnt = if_cnt;
}

void exit_loop() {
    LoopTable *parent_loop_tab = cur_loop_tab->parent;
    delete cur_loop_tab;
    cur_loop_tab = parent_loop_tab;
}

int cur_loop_cnt() {
    assert(cur_loop_tab);
    return cur_loop_tab->if_cnt;
}

// this is the global symbol table, never delete it
static SymbolTable *cur_sym_tab;
static SymbolTable *global_sym_tab;
static int symtab_cnt;

int cur_symtab_cnt() {
    return cur_sym_tab->cnt;
}

void init_symtab() {
    cur_sym_tab = global_sym_tab = new SymbolTable();
    symtab_cnt = 1;
}

void destroy_symtab() {
    delete global_sym_tab;
}

void enter_block() {
    auto new_sym_tab = new SymbolTable();
    new_sym_tab->parent = cur_sym_tab;
    new_sym_tab->cnt = symtab_cnt++;
    cur_sym_tab = new_sym_tab;
}

void exit_block() {
    auto sym_tab = cur_sym_tab;
    cur_sym_tab = sym_tab->parent;
    delete sym_tab;
}

std::shared_ptr<Symbol> symbol_insert(const std::string &name, SymType type, int value) {
    if (cur_sym_tab->table.find(name) != cur_sym_tab->table.end()) {
        printf("symbol_insert: symbol %s already exist!\n", name.c_str());
        assert(false);
    }
    auto symbol = new Symbol();
    if (type == S_VAR || type == S_ARRAY) {
        symbol->name = name + "_" + to_string(cur_sym_tab->cnt);
    }
    else {
        symbol->name = name;
    }
    symbol->type = type;
    symbol->value = value;
    cur_sym_tab->table[name] = shared_ptr<Symbol>(symbol);
    return cur_sym_tab->table[name];
}

bool symbol_exist(const std::string &name) {
    SymbolTable *sym_tab = cur_sym_tab;
    while (sym_tab && sym_tab->table.find(name) == sym_tab->table.end()) {
        sym_tab = sym_tab->parent;
    }
    if (sym_tab) {
        return true;
    }
    return false;
}

std::shared_ptr<Symbol> symbol_query(const std::string &name) {
    SymbolTable *sym_tab = cur_sym_tab;
    while (sym_tab && sym_tab->table.find(name) == sym_tab->table.end()) {
        sym_tab = sym_tab->parent;
    }
    
    if (sym_tab == nullptr) {
        printf("symbol_query: symbol %s not found!\n", name.c_str());
        assert(false);
    }

    return sym_tab->table[name];
}
