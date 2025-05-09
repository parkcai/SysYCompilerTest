#pragma once
#include "koopa.h"
#include "visit.h"
#include <map>
#include <vector>
#include <cstring>
#include <stdio.h>

struct LoopInfo {
  koopa_raw_basic_block_data_t *loop_entry;
  koopa_raw_basic_block_data_t *loop_end;
};

class LoopManager {
private:
  std::vector<LoopInfo> loop_stack;
public:
  void pushLoop(koopa_raw_basic_block_data_t *loop_entry, koopa_raw_basic_block_data_t *loop_end);
  void popLoop();
  LoopInfo getLoop();
};

class BlockManager {
private:
  std::vector<const void *> *block_list;
  std::vector<const void *> inst_buf;
public:
  void init(std::vector<const void *> *block_list);
  void newBlock(koopa_raw_basic_block_data_t *basic_block);
  void popBuffer();
  void addInst(const void *inst);
  bool willBlockReturn();
  void Dump() const;
};

koopa_raw_value_data *jumpInst(koopa_raw_basic_block_t target);

enum ValueType { Const, Var, Func, Array, Pointer};

struct Value {
  ValueType type;
  union SymbolListValue {
    int const_value;
    koopa_raw_value_t var_value;
    koopa_raw_function_t func_value;
    koopa_raw_value_t array_value;
    koopa_raw_value_t pointer_value;
  } data;
  Value() = default;
  Value(ValueType type, int value) : type(type) { data.const_value = value; }
  Value(ValueType type, koopa_raw_value_t value) : type(type) {
    if (type == Var)
      data.var_value = value;
    else if (type == Array)
      data.array_value = value;
    else if(type == Pointer)
      data.pointer_value = value;
  }
  Value(ValueType type, koopa_raw_function_t value) : type(type) {
    if (type == Func)
      data.func_value = value;
  }
};

class SymbolList {
private:
  std::vector<std::map<std::string, Value>> symbol_list_array;

public:
  ~SymbolList() = default;
  void addSymbol(std::string symbol, Value value);
  Value getSymbol(std::string symbol);
  void newScope();
  void deleteScope();
  void Dump() const;
};

koopa_raw_slice_t make_slice(std::vector<const void *> *buf, koopa_raw_slice_item_kind_t kind);

koopa_raw_type_kind_t *make_func_ty(std::vector<const void *> *buf, const koopa_raw_type_kind *ret);

koopa_raw_type_kind_t *make_ty(koopa_raw_type_tag_t tag);

koopa_raw_type_kind_t *make_array_ty(koopa_raw_type_tag_t tag, std::vector<int> dim_size);

koopa_raw_type_kind_t *make_pointer_ty(koopa_raw_type_tag_t tag);

koopa_raw_type_kind_t *make_array_pointer_ty(koopa_raw_type_tag_t tag, std::vector<int> dim_sizes);

char *add_prefix(const char *prefix, const char *name);

char *remove_prefix(const char *prefix, const char *name);

void koopa_dump_riscv_to_file(koopa_raw_program_t &program, const char *path);

int round_up(int numToRound, int multiple);
