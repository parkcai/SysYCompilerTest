#include <unordered_map>
#include <vector>
#include <cassert>
#include "symtable.hpp"

static int symbol_table_cnt = 0;

typedef std::unordered_map<std::string, std::shared_ptr<symbol_value>> symbol_table_t;
typedef std::vector<std::pair<int, symbol_table_t*>> symbol_table_stack_t;
static symbol_table_stack_t symbol_table_stack;

void enter_code_block() {
  auto* ptr = new symbol_table_t();
  symbol_table_stack.emplace_back(symbol_table_cnt, ptr);
  symbol_table_cnt++;
}

void exit_code_block() {
  auto& top = symbol_table_stack.back();
  delete top.second;
  symbol_table_stack.pop_back();
}

std::string current_code_block() {
  return "SYM_TABLE_" + std::to_string(symbol_table_stack.back().first) + "_";
}

static std::pair<int, symbol_table_t::iterator> find_iter(const std::string& symbol) {
  auto rit = symbol_table_stack.rbegin();
  while (rit != symbol_table_stack.rend()) {
    auto it = rit->second->find(symbol);
    if (it != rit->second->end()) {
      return {rit->first, it};
    }
    ++rit;
  }
  return {-1, symbol_table_stack.back().second->end()};
}

void insert_symbol(const std::string& symbol, symbol_type type, int value) {
  int symtid;
  symbol_table_t::iterator it;
  std::tie(symtid, it) = find_iter(symbol);

  auto* symval = new symbol_value();
  symval->type = type;
  symval->value = value;
  (*(symbol_table_stack.back().second))[symbol] = std::shared_ptr<symbol_value>(symval);
}

int exist_symbol(const std::string& symbol) {
  int symtid;
  symbol_table_t::iterator it;
  std::tie(symtid, it) = find_iter(symbol);

  return symtid != -1;
}

std::pair<std::string, std::shared_ptr<const symbol_value>> query_symbol(const std::string& symbol) {
  int symtid;
  symbol_table_t::iterator it;
  std::tie(symtid, it) = find_iter(symbol);

  std::string str = "SYM_TABLE_" + std::to_string(symtid) + "_";

  if (symtid == -1) {
    auto* symval = new symbol_value();
    symval->type = SYM_TYPE_UND;
    symval->value = -1;
    return {str, std::shared_ptr<const symbol_value>(symval)};
  }

  return {str, std::shared_ptr<const symbol_value>(it->second)};
}
