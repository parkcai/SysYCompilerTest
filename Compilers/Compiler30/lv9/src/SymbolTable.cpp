#include "SymbolTable.h"

using namespace std;

typedef std::unordered_map<std::string, std::shared_ptr<Var>> symbol_table_t;
typedef std::map<int, std::shared_ptr<symbol_table_t>> symbol_table_map_t;

// 当前作用域层级
static int symbol_scope_level = 0;

// 符号表
static symbol_table_map_t symbol_table_map;
static unordered_map<string, Type> func_map;
// var alloc id : ident_id
static unordered_map<string, int> var_alloc_id;

void func_insert(string ident, Type type) {
    func_map.insert(make_pair(ident, type));
}

Type func_type(string ident) {
    return func_map[ident];
}

void reset_alloc_id() {
    var_alloc_id.clear();
    for (const auto& global_symbol : *symbol_table_map[0]) {
        var_alloc_id[global_symbol.first] = 1;
    }
}

void backup_symbol_table(symbol_table_map_t &symbol_table_map_bak) {
    symbol_table_map_bak.clear();
    for (const auto& scope_pair : symbol_table_map) {
        auto scope_level = scope_pair.first;
        auto symbol_table_ptr = std::make_shared<symbol_table_t>();
        for (const auto& symbol_pair : *scope_pair.second) {
            auto ident = symbol_pair.first;
            auto var_ptr = std::make_shared<Var>(*symbol_pair.second);
            (*symbol_table_ptr)[ident] = var_ptr;
        }
        symbol_table_map_bak[scope_level] = symbol_table_ptr;
    }
}

void restore_symbol_table(symbol_table_map_t &symbol_table_map_bak) {
    symbol_table_map.clear();
    for (const auto& scope_pair : symbol_table_map_bak) {
        auto scope_level = scope_pair.first;
        auto symbol_table_ptr = std::make_shared<symbol_table_t>();
        for (const auto& symbol_pair : *scope_pair.second) {
            auto ident = symbol_pair.first;
            auto var_ptr = std::make_shared<Var>(*symbol_pair.second);
            (*symbol_table_ptr)[ident] = var_ptr;
        }
        symbol_table_map[scope_level] = symbol_table_ptr;
    }
}

void enter_new_scope() {
    shared_ptr<symbol_table_t> symbol_table_ptr = make_shared<symbol_table_t>();
    symbol_table_map.insert(make_pair(symbol_scope_level, symbol_table_ptr));
    symbol_scope_level++;
}

void exit_scope() {
    if(symbol_scope_level > 0) {
        symbol_scope_level--;
        symbol_table_map.erase(symbol_scope_level);
    }
    else
        printErr();
}

int current_scope() {
    return symbol_scope_level;
}

int symbol_scope(std::string ident) {
    printErr();
    for (int scope = symbol_scope_level - 1; scope >= 0; --scope) {
        auto it = symbol_table_map.find(scope);
        if (it != symbol_table_map.end()) {
            if ((*it).second->count(ident)) {
                return scope + 1;
            }
        }
    }
    return 0;
}

std::string alloc_str(std::string ident) {
    printErr();
    for (int scope = symbol_scope_level - 1; scope >= 0; --scope) {
        auto it = symbol_table_map.find(scope);
        if (it != symbol_table_map.end()) {
            auto var = (*it).second->find(ident);
            if ((*it).second->count(ident)) {
                //return "@" + ident + "_" + std::to_string(scope + 1);
                return "@" + ident + "_" + std::to_string(var->second->alloc_id);
            }
        }
    }
    std::cerr << "err" << std::endl;
    exit(1);
    // return "@" + ident + "_" + std ::to_string(symbol_scope_level);
}

void insert_symbol(std::string ident, Type type, int value, int dimension) {
    printErr();
    if(!var_alloc_id.count(ident))
        var_alloc_id[ident] = 0;
    shared_ptr<Var> var = make_shared<Var>();
    var->type = type; 
    var->value = value; 
    var->ident = ident;
    var->alloc_id = ++var_alloc_id[ident];
    var->dimension = dimension;
    auto current_table = symbol_table_map[symbol_scope_level - 1];
    if (current_table->find(ident) != current_table->end()) { 
        printErr(2, ident); // redefined symbol error return; 
    }
    (*current_table)[ident] = var;
}

bool is_exist(string ident) {
    for (int scope = symbol_scope_level - 1; scope >= 0; --scope) {
        auto it = symbol_table_map.find(scope);
        if (it != symbol_table_map.end()) {
            if ((*it).second->count(ident)) {
                return true;
            }
        }
    }
    return false;
}

shared_ptr<Var> find_symbol(std::string ident) {
    for (int i = symbol_scope_level - 1; i >= 0; --i) { 
        auto current_table = symbol_table_map[i]; 
        if (current_table->find(ident) != current_table->end()) { 
            return (*current_table)[ident]; 
        } 
    }
    printErr(1, ident);
    exit(1);
}

void printErr(const int& err_code, const string& ident) {
    if (err_code == 0) {
        return;
    }
    if (err_code == 1) {
        cerr << "err : undefined symbol " << "\"" << ident << "\"" << endl;
        exit(1);
    }
    if (err_code == 2) {
        cerr << "err : redefined symbol " << "\"" << ident << "\"" << endl;
        exit(2);
    }
    if (err_code == 3) {
        cerr << "err : " << "\"" << ident << "\"" << " is a const var" << endl;
        exit(3);
    }
}

void printErr() {
    if(symbol_scope_level == 0) {
        cerr << "err : symbol table does not exist" << endl;
        exit(4);
    }
}
