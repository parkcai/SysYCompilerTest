#pragma once
#include <string>
using namespace std;

void insert_value(const string &name, int value);
void define_type(const string &name, const string &type);
void define_func_type(const string &name, const string &type);
//第一个int是定义在第几个block，第二个int是值
pair<int, int>  query_symbol(const string &name);
string query_symbol_type (const string &name);
string query_func_type(const string &name);
string get_name(int block_id, const string &name);