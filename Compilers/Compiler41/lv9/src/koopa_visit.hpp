#pragma once
#include <cassert>
#include <string>
#include <sstream>
#include <stack>
#include <set>
#include <unordered_map>
#include <vector>
#include <variant>
#include "koopa.h" // 假设包含 koopa 相关定义

using StackValue = std::variant<std::string, int>;

class KoopaFunctionStack {
public:
    void addMapping(koopa_raw_value_t value);
    void addFunctionMapping(koopa_raw_value_t value, int num);
    void stackSizeRoundUp();
    void setStackSize(int size);
    void addStackSize(int size);
    int getStackSize() const;
    bool hasRA() const;
    void setRA(bool ra);
    void setRetAddrOffset();
    int getRetAddrOffset() const;
    StackValue getStackValue(koopa_raw_value_t value) const;

private:
    int stack_size = 0;
    int ret_addr_offset = 0;
    bool has_ra = false;
    std::unordered_map<koopa_raw_value_t, StackValue> value_to_stack_map;
};

class KoopaVisitor {
public:
  void Visit(const koopa_raw_program_t &program);
  void Visit(const koopa_raw_slice_t &slice);
  void Visit(const koopa_raw_function_t &func);
  void Visit(const koopa_raw_basic_block_t &bb);
  void Visit(const koopa_raw_value_t &value);
  void FunctionPrologue(const koopa_raw_function_t &func);
  void FunctionEpilogue(const koopa_raw_function_t &func);
  std::string Convert(const koopa_raw_program_t &program);

private:
  void RegSetInit();
  void VisitReturn(const koopa_raw_return_t &ret);
  void VisitInteger(const koopa_raw_integer_t &integer);
  void VisitBinary(const koopa_raw_binary_t &binary);
  void VisitStore(const koopa_raw_store_t &store);
  void VisitLoad(const koopa_raw_load_t &load);
  void VisitBranch(const koopa_raw_branch_t &branch);
  void VisitJump(const koopa_raw_jump_t &jump);
  void VisitCall(const koopa_raw_call_t &call);
  void VisitGlobalAlloc(const koopa_raw_global_alloc_t &alloc);
  void VisitAggregate(const koopa_raw_value_t &value);
  void VisitGetPtr(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value);
  void VisitGetElemPtr(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value);
  std::string getNextReg();
  std::string getTmpReg(std::string reg="");
  std::string getLastReg();
  std::string getStackAddressReg(koopa_raw_value_t value);
  std::string getStackAddressReg(int offset);
  std::string getPtrStackAddressReg(koopa_raw_value_t value);
  bool isIntPtr(koopa_raw_value_t value);
  std::string getRetReg(std::string reg1, std::string reg2);
  void RegCollect(std::string reg);
  void RegPreparation(const koopa_raw_value_t &value);
  void FuncArgRegPreparation(const koopa_raw_value_t &value);

  std::string risv_code;
  std::stack <std::string> reg_stack;
  std::set <std::string> reg_set;
  KoopaFunctionStack* function_stack;
  // int current_id = 0;
};

int getAllocSize(koopa_raw_type_t type);
int getPtrEleSize(koopa_raw_type_t type);
int getPtrSize(koopa_raw_type_t type);
void checkType(koopa_raw_type_t type);