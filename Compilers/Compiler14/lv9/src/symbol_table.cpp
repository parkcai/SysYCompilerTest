


#include <unordered_map>
#include <vector>
#include <cassert>
#include "symbol_table.hpp"

using namespace std;

// 作用域形成了一个树形结构, 而每个时刻需要保存的数个作用域形成了树上从根到某个节点的一条链
// 使用一个 vector 来维护这个链, 形成一个栈
// 进入一个作用域时, 将<作用域号,对应的符号表>压入栈顶
// 离开一个作用域时, 将栈顶元素弹出


// 作用域计数, 已经使用了了多少张符号表
static int symbol_table_cnt = 0;

// 单张符号表, 保存了当前作用域内所有符号的定义
typedef unordered_map<string, shared_ptr<symbol_value> > symbol_table_t;

// 作用域栈, 里面元素为 pair<作用域号，对应的符号表>
typedef vector< pair<int, symbol_table_t*> > symbol_table_stack_t;
static symbol_table_stack_t symbol_table_stack;

// 进入新的作用域
void enter_code_block() {
  symbol_table_t* ptr = new symbol_table_t();   // 新建一个符号表
  symbol_table_stack.push_back(make_pair(symbol_table_cnt, ptr));
  symbol_table_cnt++;
}

// 离开当前作用域
void exit_code_block() {
  delete symbol_table_stack.back().second;  // 删除当前符号表,释放内存
  symbol_table_stack.pop_back();        // 弹出栈顶元素
}

// 返回当前的作用域号
int get_current_code_block() {
  return symbol_table_stack.back().first;
}

// 在作用域栈中寻找符号, 返回其所在作用域号和其本身的iterator
static pair<int, symbol_table_t::iterator> find_iter(const string &symbol) {
  for(auto iter = symbol_table_stack.rbegin(); iter != symbol_table_stack.rend(); ++iter) {
    
    // 从栈顶开始查找
    auto it = iter->second->find(symbol);   // 在单张符号表中查找
    if(it == iter->second->end()) { 
        continue;
    }
    return make_pair(iter->first, it);
  }

  // 没找到,返回作用域号=-1
  return make_pair(-1, symbol_table_stack.back().second->end());
}


// 插入符号定义,返回所插入的作用域号
int insert_symbol(const string &symbol, symbol_type type, int value) {

    // 先在当前作用域查找有没有定义过相同符号
    auto iter = symbol_table_stack.back().second->find(symbol);
    if(iter != symbol_table_stack.back().second->end()) {
        // 符号已经存在
        return symbol_table_stack.back().first;
    }
    
    // 符号不存在, 插入
    auto symval = new symbol_value();
    symval->type = type;
    symval->value = value;
    symbol_table_stack.back().second->insert(make_pair(symbol, shared_ptr<symbol_value>(symval)));
    return symbol_table_stack.back().first;
}

// 确认符号定义是否存在, 若存在返回1, 否则返回0
int exist_symbol(const string &symbol) {
    
    auto iter = find_iter(symbol);  // 调用find_iter函数即可
    return (iter.first != -1);
}

// 查询符号定义, 返回该符号所在符号表表号和指向这个符号的值的指针.
// 若符号不存在, 返回的表号为-1, symbol_type为UND
pair<int, shared_ptr<const symbol_value>> query_symbol(const string &symbol) {
  
  auto iter = find_iter(symbol);
  int symtid = iter.first;

  // 若符号不存在
  if(symtid == -1) {
    auto symval = new symbol_value();
    symval->type = SYM_TYPE_UND;
    symval->value = -1;
    return make_pair(-1, shared_ptr<const symbol_value>(symval));
  }

  // 符号存在, 返回其value
  return make_pair(symtid, shared_ptr<const symbol_value>(iter.second->second));
}