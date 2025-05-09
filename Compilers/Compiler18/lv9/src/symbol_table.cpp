#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <iostream>
#include "symbol_table.hpp"
#include "control_stream.hpp"

using namespace std;

static unordered_map<string, int> table;
static unordered_map<string, string> table_type;
static unordered_map<string, string> table_func_type;

// 根据变量的名字和第几块，组合出一个名字
string get_name(int block_id, const string &name){
    return "BLOCK_" + to_string(block_id) + "_" + name;
}

// 给出常量名(不需要包括块id，因为一定是在当前块定义)，为其赋予一个初值，如果给变量则只是挂个号，记录一下第几块
void insert_value(const string &name, int value){
    int block_id = get_current_block_id();
    table[get_name(block_id, name)] = value;
}

// 给出常量/变量名(不需要包括块id，因为一定是在当前块定义)，为其定义类型
void define_type(const string&name, const string& type){
    int block_id = get_current_block_id();
    table_type[get_name(block_id, name)] = type;
}

// 定义函数类型
void define_func_type(const string&name, const string& type){
    table_func_type[name] = type;
}

// 返回的是一个pair<int,int>，第一个int是定义在第几个block，第二个int是值
pair<int, int> query_symbol(const string &name){
    vector<int> block_chain = get_current_block_chain();
    for (int i = block_chain.size()-1; i>=0; i--){
        if (table.find(get_name(block_chain[i], name)) != table.end()){
            return make_pair(block_chain[i], table[get_name(block_chain[i], name)]);
        }
    }
    assert(false);
}

string query_symbol_type(const string &name){
    int block_id = query_symbol(name).first;
    return table_type[get_name(block_id, name)];
}

string query_func_type(const string &name){
    return table_func_type[name];
}