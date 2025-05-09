#include "visit.hpp"
#include "koopa.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string.h>
#include <vector>

static int StackFrameSize = 0;
static int StackFrameSizeUsed = 0;
static std::map<koopa_raw_value_t, int> StackFrameSizeMap;
static std::map<koopa_raw_value_t, std::vector<int>> ArraySizeMap;
static std::map<koopa_raw_value_t, std::pair<int, std::vector<int>>> ArrayIndexMap;

static bool ra_saved = false;
// static int cur_arg_num = 0;

void sw_imm12(std::string reg, int imm12)
{
  if (imm12 >= -2048 && imm12 <= 2047)
  {
    std::cout << "  sw " << reg << ", " << imm12 << "(sp)" << std::endl;
  }
  else
  {
    std::cout << "  li t4, " << imm12 << std::endl;
    std::cout << "  add t4, t4, sp" << std::endl;
    std::cout << "  sw " << reg << ", 0(t4)" << std::endl;
  }
}
void lw_imm12(std::string reg, int imm12)
{
  if (imm12 >= -2048 && imm12 <= 2047)
  {
    std::cout << "  lw " << reg << ", " << imm12 << "(sp)" << std::endl;
  }
  else
  {
    std::cout << "  li t5, " << imm12 << std::endl;
    std::cout << "  add t5, t5, sp" << std::endl;
    std::cout << "  lw " << reg << ", 0(t5)" << std::endl;
  }
}
void addi_imm12(std::string reg1, std::string reg2, int imm12)
{
  if (imm12 >= -2048 && imm12 <= 2047)
  {
    std::cout << "  addi " << reg1 << ", " << reg2 << ", " << imm12 << std::endl;
  }
  else
  {
    std::cout << "  li t6, " << imm12 << std::endl;
    std::cout << "  add " << reg1 << ", " << reg2 << ", t6" << std::endl;
  }
}

bool NeedAddressing(koopa_raw_value_t value)
{
  if(value->kind.tag==KOOPA_RVT_GET_ELEM_PTR||value->kind.tag==KOOPA_RVT_GET_PTR)
    return true;
  else if(value->kind.tag==KOOPA_RVT_LOAD)
  {
    auto &src=value->kind.data.load.src;
    if(src->kind.tag==KOOPA_RVT_ALLOC)
    {
      if(src->ty->data.pointer.base->tag==KOOPA_RTT_POINTER)
        return true;
    }
  }
  return false;
}

void Load(koopa_raw_value_t value, std::string reg)
{
  if (value->kind.tag == KOOPA_RVT_INTEGER)
  {
    std::cout << "  li " << reg << ", " << value->kind.data.integer.value << std::endl;
  }
  else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    std::cout << "  la " << reg << ", " << value->name + 1 << std::endl;
    std::cout << "  lw " << reg << ", 0(" << reg << ")" << std::endl;
  }
  else
  {
    int offset = StackFrameSizeMap[value];
    lw_imm12(reg, offset);
    if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR || value->kind.tag == KOOPA_RVT_GET_PTR)
    {
      std::cout << "  lw " << reg << ", 0(" << reg << ")" << std::endl;
    }
    else if (value->kind.tag == KOOPA_RVT_LOAD)
    {
      auto &src = value->kind.data.load.src;
      // std::cout<<"src->kind.tag:"<<src->kind.tag<<std::endl;
      if (src->kind.tag == KOOPA_RVT_ALLOC)
      {
        if (src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
          std::cout << "  lw t0, 0(t0)" << std::endl;
      }
    }
  }
}

void PrintStackFrameSizeMap()
{
  std::cout << "StackFrameSizeMap contents:" << std::endl;
  for (const auto &pair : StackFrameSizeMap)
  {
    const koopa_raw_value_t key = pair.first;
    int value = pair.second;

    // 输出 key 和 value
    std::cout << "Key (koopa_raw_value_t): " << key << ' ' << key->kind.tag << ", Value (offset): " << value << std::endl;
  }
}
void PrintArrayMap()
{
  std::cout << "ArraySizeMap contents:" << std::endl;
  for (const auto &pair : ArraySizeMap)
  {
    const koopa_raw_value_t key = pair.first;
    std::vector<int> value = pair.second;

    // 输出 key 和 value
    std::cout << "Key (koopa_raw_value_t): " << key << ' ' << key->kind.tag << ", Value (vec): ";
    for (auto i : value)
    {
      std::cout << i << ' ';
    }
    std::cout << std::endl;
  }
  std::cout << "ArrayIndexMap contents:" << std::endl;
  for (const auto &pair : ArrayIndexMap)
  {
    const koopa_raw_value_t key = pair.first;
    std::pair<int, std::vector<int>> value = pair.second;

    // 输出 key 和 value
    std::cout << "Key (koopa_raw_value_t): " << key << ' ' << key->kind.tag << ", Value (begin): ";
    std::cout << value.first << std::endl;
  }
}

// 函数声明略
// ...

// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
  // 执行一些其他的必要操作
  // ...
  // StackFrameSize=CalculateStackFrameSize(program);
  // std::cout<<"stack frame size: "<<StackFrameSize<<std::endl;
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice)
{
  for (size_t i = 0; i < slice.len; ++i)
  {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind)
    {
    case KOOPA_RSIK_FUNCTION:
      // 访问函数
      Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
      break;
    case KOOPA_RSIK_BASIC_BLOCK:
      // 访问基本块
      Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
      break;
    case KOOPA_RSIK_VALUE:
      // 访问指令
      Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
      break;
    default:
      // 我们暂时不会遇到其他内容, 于是不对其做任何处理
      assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func)
{
  if (func->bbs.len == 0) // 跳过函数声明
    return;

  // 计算栈帧大小
  int size = 0;
  bool is_called = false;
  int args_num = 0;
  for (size_t i = 0; i < func->bbs.len; ++i)
  {
    auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    for (size_t j = 0; j < bb->insts.len; ++j)
    {
      auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
      auto &kind = inst->kind;
      if (inst->ty->tag != KOOPA_RTT_UNIT)
        size += 4;
      switch (kind.tag)
      {
      case KOOPA_RVT_ALLOC:
      {
        auto base = inst->ty->data.pointer.base;
        if (base->tag == KOOPA_RTT_ARRAY)
        {
          int array_size = 1;
          while (base->tag == KOOPA_RTT_ARRAY)
          {
            ArraySizeMap[inst].push_back(base->data.array.len);
            array_size *= base->data.array.len;
            base = base->data.array.base;
          }
          ArrayIndexMap[inst] = std::make_pair(0, ArraySizeMap[inst]);
          size += array_size * 4;
        }
        // size += 4;
      }
      break;
      case KOOPA_RVT_CALL:
      {
        if (is_called == false)
        {
          is_called = true;
          size += 4;
        }
        args_num = std::max(int(kind.data.call.args.len - 8), args_num);
        break;
      }
      default:
        break;
      }
    }
  }
  size += args_num * 4;
  StackFrameSize = (size + 15) & ~15;
  StackFrameSizeUsed = args_num * 4;

  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  std::cout << "  .text" << std::endl;
  std::cout << "  .globl " << func->name + 1 << std::endl;
  std::cout << func->name + 1 << ':' << std::endl;
  if (StackFrameSize > 0)
  {
    addi_imm12("sp", "sp", -StackFrameSize);
  }
  if (is_called)
  { // 保存 ra
    sw_imm12("ra", StackFrameSize - 4);
    ra_saved = true;
  }
  else
    ra_saved = false;
  Visit(func->bbs);
  std::cout << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  bool is_entry = strcmp(bb->name + 1, "entry") == 0;
  if (!is_entry)
    std::cout << std::endl
              << bb->name + 1 << ':' << std::endl;
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag)
  {
  case KOOPA_RVT_RETURN:
    // 访问 return 指令
    Visit(kind.data.ret);
    break;
  case KOOPA_RVT_INTEGER:
    // 访问 integer 指令
    Visit(kind.data.integer);
    break;
  case KOOPA_RVT_BINARY:
    // 访问 binary 指令
    VisitBinary(value);
    break;
  case KOOPA_RVT_ALLOC:
    VisitAlloc(value);
    break;
  case KOOPA_RVT_STORE:
    Visit(kind.data.store);
    break;
  case KOOPA_RVT_LOAD:
    VisitLoad(value);
    break;
  case KOOPA_RVT_BRANCH:
    Visit(kind.data.branch);
    break;
  case KOOPA_RVT_JUMP:
    Visit(kind.data.jump);
    break;
  case KOOPA_RVT_CALL:
    VisitCall(value);
    break;
  case KOOPA_RVT_GLOBAL_ALLOC:
    VisitGlobalAlloc(value);
    break;
  case KOOPA_RVT_GET_ELEM_PTR:
    VisitGetElemPtr(value);
    break;
  case KOOPA_RVT_GET_PTR:
    VisitGetPtr(value);
    break;
  default:
    // 其他类型暂时遇不到
    break;
  }
}

void VisitBinary(const koopa_raw_value_t &value)
{
  auto &binary = value->kind.data.binary;
  Load(binary.lhs, "t0");
  Load(binary.rhs, "t1");
  switch (binary.op)
  {
  case KOOPA_RBO_NOT_EQ: // 不等于
    std::cout << "  xor t0, t0, t1" << std::endl
              << "  snez t0, t0" << std::endl;
    break;
  case KOOPA_RBO_EQ: // 等于
    std::cout << "  xor t0, t0, t1" << std::endl
              << "  seqz t0, t0" << std::endl;
    break;
  case KOOPA_RBO_GT: // 大于
    std::cout << "  sgt t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_LT: // 小于
    std::cout << "  slt t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_GE: // 大于等于
    std::cout << "  slt t0, t0, t1" << std::endl
              << "  xori t0, t0, 1" << std::endl;
    break;
  case KOOPA_RBO_LE: // 小于等于
    std::cout << "  sgt t0, t0, t1" << std::endl
              << "  xori t0, t0, 1" << std::endl;
    break;
  case KOOPA_RBO_ADD: // 加法
    std::cout << "  add t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_SUB: // 减法
    std::cout << "  sub t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_MUL: // 乘法
    std::cout << "  mul t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_DIV: // 除法
    std::cout << "  div t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_MOD: // 取模
    std::cout << "  rem t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_AND: // 与
    std::cout << "  and t0, t0, t1" << std::endl;
    break;
  case KOOPA_RBO_OR: // 或
    std::cout << "  or t0, t0, t1" << std::endl;
    break;
  default:
    break;
  }
  sw_imm12("t0", StackFrameSizeUsed);
  StackFrameSizeMap[value] = StackFrameSizeUsed;
  StackFrameSizeUsed += 4;
}

void Visit(const koopa_raw_integer_t &integer)
{
  std::cout << "  li a0, " << integer.value << std::endl;
}

void VisitAlloc(const koopa_raw_value_t &value)
{
  auto tag = value->ty->data.pointer.base->tag;
  switch (tag)
  {
  case KOOPA_RTT_INT32:
    StackFrameSizeMap[value] = StackFrameSizeUsed;
    StackFrameSizeUsed += 4;
    break;
  case KOOPA_RTT_ARRAY:
  {
    StackFrameSizeMap[value] = StackFrameSizeUsed;
    int size = 1;
    for (auto len : ArraySizeMap[value])
    {
      size *= len;
    }
    StackFrameSizeUsed += size * 4;
  }
  break;
  case KOOPA_RTT_POINTER:
  {
    StackFrameSizeMap[value] = StackFrameSizeUsed;
    StackFrameSizeUsed += 4;
    auto base = value->ty->data.pointer.base;
    while (base->tag == KOOPA_RTT_POINTER)
    {
      ArraySizeMap[value].push_back(1);
      base = base->data.pointer.base;
    }
    while (base->tag == KOOPA_RTT_ARRAY)
    {
      ArraySizeMap[value].push_back(base->data.array.len);
      base = base->data.array.base;
    }
    ArrayIndexMap[value] = std::make_pair(0, ArraySizeMap[value]);
  }
  break;
  default:
    break;
  }
}

void Visit(const koopa_raw_store_t &store)
{
  if (store.value->kind.tag == KOOPA_RVT_INTEGER)
  {
    std::cout << "  li t0, " << store.value->kind.data.integer.value << std::endl;
  }
  else
  {
    // PrintStackFrameSizeMap();
    if (store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF)
    {
      int index = store.value->kind.data.func_arg_ref.index;
      if (index < 8)
      {
        std::cout << "  mv t0, a" << index << std::endl;
      }
      else
      {
        int offset = (index - 8) * 4 + StackFrameSize;
        lw_imm12("t0", offset);
      }
    }
    else if (store.value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
      std::cout << "  la t0, " << store.value->name + 1 << std::endl;
      std::cout << "  lw t0, 0(t0)" << std::endl;
    }
    else
    {
      int offset = StackFrameSizeMap[store.value];
      lw_imm12("t0", offset);
      if(NeedAddressing(store.value))
      {
        std::cout << "  lw t0, 0(t0)" << std::endl;
      }
    }
  }
  if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    std::cout << "  la t1, " << store.dest->name + 1 << std::endl;
    std::cout << "  sw t0, 0(t1)" << std::endl;
  }
  else
  {
    int offset = StackFrameSizeMap[store.dest];
    if (NeedAddressing(store.dest))
    {
      lw_imm12("t1", offset);
      std::cout << "  sw t0, 0(t1)" << std::endl;
    }
    else
      sw_imm12("t0", offset);
  }
}

void VisitLoad(const koopa_raw_value_t &value)
{
  auto &src = value->kind.data.load.src;
  if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    std::cout << "  la t0, " << src->name + 1 << std::endl;
    std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  else
  {
    lw_imm12("t0", StackFrameSizeMap[src]);
    if(NeedAddressing(src))
    {
      std::cout << "  lw t0, 0(t0)" << std::endl;
    }
  }
  sw_imm12("t0", StackFrameSizeUsed);
  StackFrameSizeMap[value] = StackFrameSizeUsed;
  StackFrameSizeUsed += 4;
  if (ArrayIndexMap.find(src) != ArrayIndexMap.end())
  {
    ArrayIndexMap[value] = ArrayIndexMap[src];
  }
}

void Visit(const koopa_raw_branch_t &branch)
{
  if (branch.cond->kind.tag == KOOPA_RVT_INTEGER)
  {
    std::cout << "  li t0, " << branch.cond->kind.data.integer.value << std::endl;
  }
  else
  {
    int offset = StackFrameSizeMap[branch.cond];
    lw_imm12("t0", offset);
  }
  std::cout << "  bnez t0, " << branch.true_bb->name + 1 << std::endl;
  std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
  // std::cout<<branch.true_bb->name+1<<':'<<std::endl;
}

void Visit(const koopa_raw_jump_t &jump)
{
  std::cout << "  j " << jump.target->name + 1 << std::endl;
  // std::cout<<jump.target->name+1<<':'<<std::endl;
}

void VisitCall(const koopa_raw_value_t &value)
{
  auto &call = value->kind.data.call;

  for (size_t i = 0; i < call.args.len; ++i)
  {
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);

    if (i < 8)
    { // 加载到寄存器
      if (arg->kind.tag == KOOPA_RVT_INTEGER)
      {
        std::cout << "  li a" << i << ", " << arg->kind.data.integer.value << std::endl;
      }
      else
      {
        lw_imm12("a" + std::to_string(i), StackFrameSizeMap[arg]);
      }
    }
    else
    { // 加载到栈
      if (arg->kind.tag == KOOPA_RVT_INTEGER)
      {
        std::cout << "  li t0, " << arg->kind.data.integer.value << std::endl;
      }
      else
      {
        int offset = StackFrameSizeMap[arg];
        lw_imm12("t0", offset);
      }
      sw_imm12("t0", (i - 8) * 4);
    }
  }

  std::cout << "  call " << call.callee->name + 1 << std::endl;

  if (value->ty->tag != KOOPA_RTT_UNIT)
  {
    sw_imm12("a0", StackFrameSizeUsed);
    StackFrameSizeMap[value] = StackFrameSizeUsed;
    StackFrameSizeUsed += 4;
  }
}

void Visit(const koopa_raw_return_t &ret)
{
  if (ret.value != nullptr)
  {
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER)
    {
      std::cout << "  li a0, " << ret.value->kind.data.integer.value << std::endl;
    }
    else if (ret.value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
      std::cout << "  la t0, " << ret.value->name + 1 << std::endl;
      std::cout << "  lw t0, 0(a0)" << std::endl;
    }
    else
    {
      lw_imm12("a0", StackFrameSizeMap[ret.value]);
      // std::cout<<"ret.value->kind.tag: "<<ret.value->kind.tag<<std::endl;
      if (ret.value->kind.tag == KOOPA_RVT_GET_ELEM_PTR || ret.value->kind.tag == KOOPA_RVT_GET_PTR)
      {
        std::cout << "  lw a0, 0(a0)" << std::endl;
      }
      else if (ret.value->kind.tag == KOOPA_RVT_LOAD)
      {
        auto &src = ret.value->kind.data.load.src;
        // std::cout<<"src->kind.tag:"<<src->kind.tag<<std::endl;
        if (src->kind.tag == KOOPA_RVT_ALLOC)
        {
          if (src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
            std::cout << "  lw a0, 0(a0)" << std::endl;
        }
      }
    }
  }
  if (ra_saved)
  {
    lw_imm12("ra", StackFrameSize - 4);
  }
  if (StackFrameSize > 0)
  {
    addi_imm12("sp", "sp", StackFrameSize);
  }
  std::cout << "  ret " << std::endl;
}

void AggregateInit(const koopa_raw_value_t &value)
{
  auto elements = value->kind.data.aggregate.elems;
  for (size_t i = 0; i < elements.len; ++i)
  {
    auto element = reinterpret_cast<koopa_raw_value_t>(elements.buffer[i]);
    if (element->kind.tag == KOOPA_RVT_INTEGER)
    {
      std::cout << "  .word " << element->kind.data.integer.value << std::endl;
    }
    else
    {
      AggregateInit(element);
    }
  }
}

void VisitGlobalAlloc(const koopa_raw_value_t &value)
{
  std::cout << "  .data" << std::endl;
  std::cout << "  .globl " << value->name + 1 << std::endl;
  std::cout << value->name + 1 << ':' << std::endl;
  auto &alloc = value->kind.data.global_alloc;
  auto base = value->ty->data.pointer.base;

  switch (alloc.init->kind.tag)
  {
  case KOOPA_RVT_INTEGER:
    std::cout << "  .word " << alloc.init->kind.data.integer.value << std::endl;
    break;
  case KOOPA_RVT_ZERO_INIT:
  {
    if (base->tag == KOOPA_RTT_INT32)
      std::cout << "  .zero 4" << std::endl;
    else if (base->tag == KOOPA_RTT_ARRAY)
    {
      int size = 1;
      while (true)
      {
        if (base->tag == KOOPA_RTT_ARRAY)
        {
          ArraySizeMap[value].push_back(base->data.array.len);
          size *= base->data.array.len;
          base = base->data.array.base;
        }
        else
          break;
      }
      ArrayIndexMap[value] = std::make_pair(0, ArraySizeMap[value]);
      std::cout << "  .zero " << size * 4 << std::endl;
    }
  }
  break;
  case KOOPA_RVT_AGGREGATE:
  {
    while (true)
    {
      if (base->tag == KOOPA_RTT_ARRAY)
      {
        ArraySizeMap[value].push_back(base->data.array.len);
        base = base->data.array.base;
      }
      else
        break;
    }
    ArrayIndexMap[value] = std::make_pair(0, ArraySizeMap[value]);
    AggregateInit(alloc.init); // 递归处理
  }
  break;
  default:
    break;
  }
  std::cout << std::endl;
}

void VisitGetElemPtr(const koopa_raw_value_t &value)
{
  auto src = value->kind.data.get_elem_ptr.src;
  auto index = value->kind.data.get_elem_ptr.index;
  if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    std::cout << "  la t0, " << src->name + 1 << std::endl;
    // std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  else
  {
    addi_imm12("t0", "sp", StackFrameSizeMap[src]);
    if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src->kind.tag == KOOPA_RVT_GET_PTR)
    {
      // auto child_src = value->kind.data.get_elem_ptr.src;
      // if (child_src->kind.tag != KOOPA_RVT_GET_ELEM_PTR && child_src->kind.tag != KOOPA_RVT_GET_PTR)
      std::cout << "  lw t0, 0(t0)" << std::endl;
    }
    else if (src->kind.tag == KOOPA_RVT_LOAD)
    {
      auto child_src = src->kind.data.load.src;
      // std::cout<<"src->kind.tag:"<<src->kind.tag<<std::endl;
      if (child_src->kind.tag == KOOPA_RVT_ALLOC)
      {
        if (child_src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
          std::cout << "  lw t0, 0(t0)" << std::endl;
      }
    }
  }
  Load(index, "t1");
  // PrintArrayMap();
  int size = 4;
  auto begin_vec = ArrayIndexMap[src];
  auto vec = begin_vec.second;
  for (int i = begin_vec.first + 1; i < vec.size(); ++i)
  {
    size *= vec[i];
  }
  std::cout << "  li t2, " << size << std::endl;
  std::cout << "  mul t1, t1, t2" << std::endl;
  std::cout << "  add t0, t0, t1" << std::endl;

  if (value->ty->tag != KOOPA_RTT_UNIT)
  {
    sw_imm12("t0", StackFrameSizeUsed);
    StackFrameSizeMap[value] = StackFrameSizeUsed;
    ArrayIndexMap[value] = std::make_pair(begin_vec.first + 1, begin_vec.second);
    // PrintArrayMap();
    StackFrameSizeUsed += 4;
  }
}

void VisitGetPtr(const koopa_raw_value_t &value)
{
  auto src = value->kind.data.get_ptr.src;
  auto index = value->kind.data.get_ptr.index;
  if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    std::cout << "  la t0, " << src->name + 1 << std::endl;
    // std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  else
  {
    addi_imm12("t0", "sp", StackFrameSizeMap[src]);
    if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src->kind.tag == KOOPA_RVT_GET_PTR)
    {
      std::cout << "  lw t0, 0(t0)" << std::endl;
    }
    else if (src->kind.tag == KOOPA_RVT_LOAD)
    {
      auto child_src = src->kind.data.load.src;
      // std::cout<<"src->kind.tag:"<<src->kind.tag<<std::endl;
      if (child_src->kind.tag == KOOPA_RVT_ALLOC)
      {
        if (child_src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
          std::cout << "  lw t0, 0(t0)" << std::endl;
      }
    }
  }
  Load(index, "t1");
  int size = 4;
  auto begin_vec = ArrayIndexMap[src];
  auto vec = begin_vec.second;
  // PrintArrayMap();
  for (int i = begin_vec.first + 1; i < vec.size(); ++i)
  {
    size *= vec[i];
  }
  std::cout << "  li t2, " << size << std::endl;
  std::cout << "  mul t1, t1, t2" << std::endl;
  std::cout << "  add t0, t0, t1" << std::endl;

  if (value->ty->tag != KOOPA_RTT_UNIT)
  {
    sw_imm12("t0", StackFrameSizeUsed);
    StackFrameSizeMap[value] = StackFrameSizeUsed;
    ArrayIndexMap[value] = std::make_pair(begin_vec.first + 1, begin_vec.second);
    StackFrameSizeUsed += 4;
  }
}
