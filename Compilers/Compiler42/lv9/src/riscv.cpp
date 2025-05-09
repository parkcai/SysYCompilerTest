#include "riscv.h"

#include <random>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cassert>

using namespace std;

//(1)函数返回值为基本块中所有指令的总大小。
//&call，检查基本块中是否存在函数调用，&max_arg记录最大参数数量。
int RISCV_Module::bb_size(koopa_raw_basic_block_t bb, bool &call,
                           int &max_arg) {
  int size = 0;
  for (size_t i = 0; i < bb->insts.len; ++i) {
    auto ptr = bb->insts.buffer[i];
    if (((koopa_raw_value_t)ptr)->kind.tag == KOOPA_RVT_CALL) {
      call = true;
      max_arg = ((koopa_raw_value_t)ptr)->kind.data.call.args.len > max_arg
                    ? ((koopa_raw_value_t)ptr)->kind.data.call.args.len
                    : max_arg;
    }

    size += inst_size(reinterpret_cast<koopa_raw_value_t>(ptr));
  }
  return size;
}

//计算单条指令的大小
int RISCV_Module::inst_size(koopa_raw_value_t inst) {
  //如果指令是 ALLOC（分配内存），则计算其基类型的大小
  if (inst->kind.tag == KOOPA_RVT_ALLOC) {
    return type_size(inst->ty->data.pointer.base);
  }
  //对于非 ALLOC 指令，直接计算指令的数据类型的大小
  return type_size(inst->ty);
}

//遍历函数中的所有基本块，计算每个基本块的大小，并累加得到函数的总大小
//如果函数中存在函数调用，将 &call 设置为 true
int RISCV_Module::func_size(koopa_raw_function_t func, bool &call) {
  int size = 0;
  int max_arg = 0;
  for (size_t i = 0; i < func->bbs.len; ++i) {
    auto ptr = func->bbs.buffer[i];
    size += bb_size(reinterpret_cast<koopa_raw_basic_block_t>(ptr), call, max_arg);
  }
  //为函数调用的参数分配栈空间。每个参数占用 4 字节（在 RISC-V 中，参数通常通过栈传递）
  size += max_arg * 4;
  //为函数本身的参数分配栈空间。在 RISC-V 中，前 8 个参数通过寄存器传递，超出部分通过栈传递。每个额外的参数占用 4 字节。
  size += (func->params.len > 8 ? func->params.len - 8 : 0) * 4;
  //如果函数中存在函数调用，为返回地址分配 4 字节的栈空间。
  size += call ? 4 : 0;
  return size;
}

int RISCV_Module::type_size(koopa_raw_type_t ty) {
  switch (ty->tag) {
  case KOOPA_RTT_INT32:
    return 4;
  case KOOPA_RTT_UNIT:
    return 0;
  case KOOPA_RTT_POINTER:
    return 4;
  //LV9,数组
  case KOOPA_RTT_ARRAY:
    return array_size(ty);
  default:
    return 0;
  }
}

int RISCV_Module::array_size(koopa_raw_type_t ty) {
  if (ty->tag == KOOPA_RTT_ARRAY) {
    return array_size(ty->data.array.base) * ty->data.array.len;
  } else {
    return 4;
  }
}

void RISCV_Module::Env::init(int size, bool call) {
  stack_size = size;
  is_call = call;
  cur_size = 0;  
  addr_map.clear();
}

//为每条指令分配一个唯一的地址，并记录指令与其地址的映射关系
int RISCV_Module::Env::get_addr(koopa_raw_value_t raw) {
  //如果指令已经分配了地址，直接返回该地址。
  if (addr_map.find(raw) != addr_map.end()) {
    return addr_map[raw];
  } else {
  //如果指令尚未分配地址，计算指令的大小，为其分配一个新的地址，并记录地址与指令的映射关系。
    int t = inst_size(raw);
    if (t == 0)
      return -1;
    addr_map[raw] = cur_size;
    cur_size += t;
    return addr_map[raw];
  }
}

 /*
  * 将给定的值加载到指定的寄存器中。
  * 此函数根据值的类型，将一个 Koopa 中间表示的值加载到 RISC-V 寄存器中。
  * 它处理两种情况：整数立即数和存储在栈上的值。
  * @param value 要加载的 Koopa 中间表示值。
  * @param reg 目标 RISC-V 寄存器的名称。
  */
void RISCV_Module::load_register(koopa_raw_value_t value, string reg) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
      out << "  li " + reg + ", " +
                 to_string(value->kind.data.integer.value) + "\n";
    } else {
      int addr = env.get_addr(value);
      if (addr != -1) {
        if (addr < 2048 && addr >= -2048) {
          out << "  lw " + reg + ", " + to_string(addr) + "(sp)\n";
        } else {
          out << "  li t3, " + to_string(addr) + "\n";
          out << "  add t3, sp, t3\n";
          out << "  lw " + reg + ", 0(t3)\n";
        }
      } else {
        assert(false);
    }
  }
}

 /*
  * 将寄存器的值存储到栈中
  * 此函数根据给定的偏移地址和寄存器名称，生成相应的指令将寄存器的值存储到栈中。
  * 如果偏移地址超出了某些范围，则需要使用额外的指令来计算地址。
  * 如果给定的偏移地址为-1，则视为无效输入并断言失败。
  * @addr: 相对于栈指针(sp)的偏移地址
  * @reg:  需要存储的寄存器名称
  */
void RISCV_Module::store_stack(int addr, string reg) {
  if (addr != -1) {
    if (addr < 2048 && addr >= -2048) {
      out << "  sw " + reg + ", " + to_string(addr) + "(sp)\n";
    } else {
      out << "  li t3, " + to_string(addr) + "\n";
      out << "  add t3, sp, t3\n";
      out << "  sw " + reg + ", 0(t3)\n";
    }
    } else {
    assert(false);
  }
}

//解析program。包括两步骤：（1）解析程序中的全局变量；（2）解析functions。
void RISCV_Module::raw_analyze(const koopa_raw_program_t &raw) {
  if (raw.values.len != 0) {
    //如果程序中有全局变量，生成数据段并分析全局变量。
    out << "  .data\n";
    raw_analyze(raw.values);//raw.values的类型为 koopa_raw_slice_t
  }
  // 生成代码段并分析函数。
  out << "\n  .text\n";
  // 解析所有函数
  raw_analyze(raw.funcs);
}

// 解析 raw slice
void RISCV_Module::raw_analyze(const koopa_raw_slice_t &slice) {
  //string ret = "";
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 解析函数
        raw_analyze(reinterpret_cast<const koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 解析基本块
        raw_analyze(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 解析指令
        raw_analyze(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        assert(false);
    }
  }
}

// 解析函数
void RISCV_Module::raw_analyze(const koopa_raw_function_t &func) {
  // CRay: params
  if (func->bbs.len == 0)
    return;

  out << "\n  .globl  " << string(func->name + 1) + "\n";
  out << string(func->name + 1) + ":\n";
  bool call = false;
  int size = func_size(func, call);

  size = (size + 15) / 16 * 16 * 2;
  if (size < 2048 && size >= -2048) {
    out << "  addi sp, sp, -" + to_string(size) + "\n";
  } else if (size > 0) {
    out << "  li t0, -" + to_string(size) + "\n";
    out << "  add sp, sp, t0\n";
  }
  if (call) {
    if (size - 4 < 2048 && size - 4 >= -2048)
      out << "  sw ra, " + to_string(size - 4) + "(sp)\n";
    else {
      out << "  li t0, " + to_string(size - 4) + "\n";
      out << "  add t0, sp, t0\n";
      out << "  sw ra, 0(t0)\n";
    }
  }
  env.init(size, call);
  env.stack_size -= call ? 4 : 0;
  env.cur_size += (func->params.len > 8 ? func->params.len - 8 : 0) * 4;

  // 解析基本块
  raw_analyze(func->bbs);
}

// 解析基本块
void RISCV_Module::raw_analyze(const koopa_raw_basic_block_t &bb) {
  // 解析指令
  //cout << bb->name << endl;
  string name = bb->name + 1;
  if (name != "entry")
    out << name << ":\n";
  raw_analyze(bb->insts);
}

// 解析指令
void RISCV_Module::raw_analyze(const koopa_raw_value_t &value) {

  const auto &kind = value->kind;
  int addr = env.get_addr(value);

  // 根据指令类型判断后续需要如何访问
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 1. Function return
      raw_analyze(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 解析 integer 指令
    break;
    case KOOPA_RVT_ALLOC:
      // Local memory allocation
      break;
    case KOOPA_RVT_LOAD:
      // 2. Memory load
      raw_analyze(kind.data.load, addr);
      break;
    case KOOPA_RVT_STORE:
      // 3. Memory store
      raw_analyze(kind.data.store);      
      break;
    case KOOPA_RVT_BINARY:
      // 4. Binary operation
      raw_analyze(kind.data.binary, addr);
      break;
    //LV6 CRAY 20241207
    case KOOPA_RVT_BRANCH:
      // 5. Conditional branch
      raw_analyze(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 6. Unconditional jump
      raw_analyze(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 7. Function call
      raw_analyze(kind.data.call, addr);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      // 8. Global memory allocation
      global_alloc(value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      // 9. Element pointer calculation
      raw_analyze(kind.data.get_elem_ptr, addr);
      break;
    case KOOPA_RVT_GET_PTR:
      // 10. Pointer calculation
      raw_analyze(kind.data.get_ptr, addr);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 1. Function return
void RISCV_Module::raw_analyze(const koopa_raw_return_t &ret_value) {
  if (ret_value.value != nullptr) {
    load_register(ret_value.value, "a0");
  }
  if (env.is_call) {
    if (env.stack_size < 2048 && env.stack_size >= -2048)
      out << "  lw ra, " + to_string(env.stack_size) + "(sp)\n";
    else {
      out << "  li t0, " + to_string(env.stack_size) + "\n";
      out << "  add t0, sp, t0\n";
      out << "  lw ra, 0(t0)\n";
    }
  }
  int size = env.stack_size;
  size += env.is_call ? 4 : 0;
  if (size < 2048 && size >= -2048) {
    out << "  addi sp, sp, " + to_string(size) + "\n";
  } else if (size > 0) {
    out << "  li t0, " + to_string(size) + "\n";
    out << "  add sp, sp, t0\n";
  }
  out << "  ret\n";
}

// 2. Memory load
void RISCV_Module::raw_analyze(const koopa_raw_load_t &l_value, int addr) {
  string rs1 = "t0";  
  
  if (l_value.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    out << "  la " + rs1 + ", " + string(l_value.src->name + 1) + "\n";
    out << "  lw " + rs1 + ", 0(" + rs1 + ")\n";
  } else if (l_value.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
             l_value.src->kind.tag == KOOPA_RVT_GET_PTR) {
    load_register(l_value.src, rs1);
    out << "  lw " + rs1 + ", 0(" + rs1 + ")\n";
  } else {
    load_register(l_value.src, rs1);
  }
  store_stack(addr, rs1);
}

// 3. Memory store
void RISCV_Module::raw_analyze(const koopa_raw_store_t &s_value) {
  if (s_value.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    out << "  la t1, " + string(s_value.dest->name + 1) + "\n";
    load_register(s_value.value, "t0");
    out << "  sw t0, 0(t1)\n";
  } else if (s_value.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
             s_value.dest->kind.tag == KOOPA_RVT_GET_PTR) {
    load_register(s_value.dest, "t1");
    load_register(s_value.value, "t0");
    out << "  sw t0, 0(t1)\n";
  } else {
    int addr = env.get_addr(s_value.dest);
    if (s_value.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
      if (s_value.value->kind.data.func_arg_ref.index < 8) {
        store_stack(
            addr,
            "a" + to_string(s_value.value->kind.data.func_arg_ref.index));
      } else {
        if ((s_value.value->kind.data.func_arg_ref.index - 8) * 4 < 2048 &&
            (s_value.value->kind.data.func_arg_ref.index - 8) * 4 >= -2048)
          out << "  lw t0, "
              << (s_value.value->kind.data.func_arg_ref.index - 8) * 4
              << "(sp)\n";
        else {
          out << "  li t3, "
              << (s_value.value->kind.data.func_arg_ref.index - 8) * 4 << "\n";
          out << "  add t3, sp, t3\n";
          out << "  lw t0, 0(t3)\n";
        }
        store_stack(addr, "t0");
      }
    } else {
      load_register(s_value.value, "t0");
      store_stack(addr, "t0");
    }
  }
}

// 4. Binary operation
void RISCV_Module::raw_analyze(const koopa_raw_binary_t &b_value, int addr) {
  string rd = "t0";
  string rs1 = "t0";
  string rs2 = "t1";
  load_register(b_value.lhs, rs1);
  load_register(b_value.rhs, rs2);
  switch (b_value.op) {
  case KOOPA_RBO_ADD:
    out << "  add " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_SUB:
    out << "  sub " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_MUL:
    out << "  mul " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_DIV:
    out << "  div " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_MOD:
    out << "  rem " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_AND:
    out << "  and " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_OR:
    out << "  or " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_EQ:
    out << "  xor " + rd + ", " + rs1 + ", " + rs2 + "\n";
    out << "  seqz " + rd + ", " + rd + "\n";
    break;
  case KOOPA_RBO_NOT_EQ:
    out << "  xor " + rd + ", " + rs1 + ", " + rs2 + "\n";
    out << "  snez " + rd + ", " + rd + "\n";
    break;
  case KOOPA_RBO_GT:
    out << "  sgt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_LT:
    out << "  slt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    break;
  case KOOPA_RBO_GE:
    out << "  slt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    out << "  seqz " + rd + ", " + rd + "\n";
    break;
  case KOOPA_RBO_LE:
    out << "  sgt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    out << "  seqz " + rd + ", " + rd + "\n";
    break;
  default:
    break;
  }
  store_stack(addr, rd);
}

// 5. Conditional branch
//LV6 CRAY 20241207
//根据分支指令的条件和跳转目标，生成相应的 RISC-V 汇编代码
void RISCV_Module::raw_analyze(const koopa_raw_branch_t &b_value) {
  //加载条件值到寄存器
  load_register(b_value.cond, "t0");
  // bnez 是 RISC-V 中的条件跳转指令，表示“如果不等于零则跳转”
  out << "  bnez t0, " + string(b_value.true_bb->name + 1) + "_tmp" + "\n";
  //j 是 RISC-V 中的无条件跳转指令，表示“跳转”
  //b_value.false_bb->name + 1 是 false 分支的基本块名称，去掉第一个字符（通常为 %）
  out << "  j " + string(b_value.false_bb->name + 1) + "\n";
  // "_tmp" 是 true 分支的临时标签。使用 j 指令跳转到 true 分支的实际标签
  out << string(b_value.true_bb->name + 1) + "_tmp:\n";
  out << "  j " + string(b_value.true_bb->name + 1) + "\n";
}

// 6. Unconditional jump
void RISCV_Module::raw_analyze(const koopa_raw_jump_t &j_value) {
  out << "  j " + string(j_value.target->name + 1) + "\n";
}

// 7. Function call
void RISCV_Module::raw_analyze(const koopa_raw_call_t &c_value, int addr) {
  for (int i = 0; i < c_value.args.len && i < 8; ++i) {
    auto ptr = c_value.args.buffer[i];
    load_register(reinterpret_cast<koopa_raw_value_t>(ptr),
                  "a" + to_string(i));
  }
  bool call = false;
  int size = func_size(c_value.callee, call);
  size = (size + 15) / 16 * 16 * 2;
  for (int i = 8; i < c_value.args.len; ++i) {
    auto ptr = c_value.args.buffer[i];
    load_register(reinterpret_cast<koopa_raw_value_t>(ptr), "t0");
    store_stack((i - 8) * 4 - size, "t0");
  }
  out << "  call " + string(c_value.callee->name + 1) + "\n";
  if (addr != -1)
    store_stack(addr, "a0");
}

// 8. Global memory allocation
/*
根据全局变量的类型和初始化值，生成相应的 RISC-V 汇编代码。
如果全局变量是整数类型，生成 .word 指令。
如果全局变量是零初始化类型，生成 .zero 指令。
如果全局变量是聚合类型（如数组或结构体），递归处理其初始化值。
*/
void RISCV_Module::global_alloc(const koopa_raw_value_t &g_value) {
  //转成汇编语言 .global
  out << "\n  .global " + string(g_value->name + 1) + "\n";
  out << string(g_value->name + 1) + ":\n";
  if (g_value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_INTEGER)
    out << "  .word " +
               to_string(g_value->kind.data.global_alloc.init->kind.data
                                  .integer.value) +
               "\n";
  if (g_value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    out << "  .zero " +
               to_string(type_size(g_value->ty->data.pointer.base)) + "\n";
  }
  if (g_value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
    // aggregate
    raw_analyze(g_value->kind.data.global_alloc.init->kind.data.aggregate);
  }
}
// aggregate
void RISCV_Module::raw_analyze(const koopa_raw_aggregate_t &agg_value) {
  for (size_t i = 0; i < agg_value.elems.len; ++i) {
    auto ptr = agg_value.elems.buffer[i];
    koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
      out << "  .word " + to_string(value->kind.data.integer.value) + "\n";
    } else if (value->kind.tag == KOOPA_RVT_AGGREGATE) {
      raw_analyze(value->kind.data.aggregate);
    } else
      assert(false);
  }
}

// 9. Element pointer calculation
void RISCV_Module::raw_analyze(const koopa_raw_get_elem_ptr_t &gep_value,
                              int addr) {
  if (gep_value.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    out << "  la t0, " + string(gep_value.src->name + 1) + "\n";
  } else {
    int src_addr = env.get_addr(gep_value.src);
    if (src_addr != -1) {
      if (src_addr < 2048 && src_addr >= -2048)
        out << "  addi t0, sp, " + to_string(src_addr) + "\n";
      else {
        out << "  li t3, " + to_string(src_addr) + "\n";
        out << "  add t0, sp, t3\n";
      }
    } else {
      assert(false);
    }
    if (gep_value.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
        gep_value.src->kind.tag == KOOPA_RVT_GET_PTR) {
      out << "  lw t0, 0(t0)\n";
    }
  }
  load_register(gep_value.index, "t1");
  int size = array_size(gep_value.src->ty->data.pointer.base->data.array.base);
  out << "  li t2, " + to_string(size) + "\n";
  out << "  mul t1, t1, t2\n";
  out << "  add t0, t0, t1\n";
  store_stack(addr, "t0");
}

// 10. Pointer calculation
void RISCV_Module::raw_analyze(const koopa_raw_get_ptr_t &gp_value, int addr) {
  int src_addr = env.get_addr(gp_value.src);
  if (src_addr != -1) {
    if (src_addr < 2048 && src_addr >= -2048)
      out << "  addi t0, sp, " + to_string(src_addr) + "\n";
    else {
      out << "  li t3, " + to_string(src_addr) + "\n";
      out << "  add t0, sp, t3\n";
    }
  } else {
    assert(false);
  }
  out << "  lw t0, 0(t0)\n";
  load_register(gp_value.index, "t1");
  int size = array_size(gp_value.src->ty->data.pointer.base);
  out << "  li t2, " + to_string(size) + "\n";
  out << "  mul t1, t1, t2\n";
  out << "  add t0, t0, t1\n";
  store_stack(addr, "t0");
}

// 根据上述函数的嵌套调用，将raw以汇编格式输出到文件
void RISCV_Module::raw_dump_to_riscv(koopa_raw_program_t raw) {
  raw_analyze(raw);
  out.close();
}