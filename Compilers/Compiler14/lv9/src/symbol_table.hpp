#pragma once
#include <string>
#include <memory>

using namespace std;

// 符号表中符号的类型
enum symbol_type
{
  SYM_TYPE_CONST,       // 常量         value = 其值
  SYM_TYPE_VAR,         // 变量         value = 0  
  SYM_TYPE_FUNCVOID,    // void 函数    value = 0
  SYM_TYPE_FUNCINT,     // int 函数     value = 0
  SYM_TYPE_CONSTARRAY,  // 常量数组     value = 数组维数
  SYM_TYPE_ARRAY,       // 变量数组     value = 数组维数
  SYM_TYPE_PTR,         // 指针         value = 指针指向的类型的维数+1
  // 因为函数的数组参数中,数组第一维的长度省略不写,后序维度的长度是常量表达式,所以这里维度要+1
  SYM_TYPE_UND          // 符号不存在    value = -1
};

// 符号表中符号的值
struct symbol_value
{
  symbol_type type; // 符号的类型
  int value;        // 符号的值
};

// 进入新的作用域
void enter_code_block();

// 离开当前作用域
void exit_code_block();

// 返回当前符号表的作用域号
int get_current_code_block();

// 插入符号定义
int insert_symbol(const string &symbol, symbol_type type, int value);

// 确认符号定义是否存在, 若存在返回1, 否则返回0
int exist_symbol(const string &symbol);

// 查询符号定义, 返回该符号所在符号表作用域号和指向这个符号的值的指针.
// 若符号不存在, 返回的表号为-1, symbol_type为UND
pair<int, shared_ptr<const symbol_value>> query_symbol(const string &symbol);