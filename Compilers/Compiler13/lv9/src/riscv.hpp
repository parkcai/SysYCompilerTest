#pragma once

#include "koopa.h"
#include <string>
#include <map>
#include <set>

#define REG_t0   0
#define REG_t6   6
#define REG_a0   7
#define REG_a7   14
#define REG_x0   16
#define REG_sp   17
#define REG_ra   18

#define REG_num  8

#define OP_NEEQ  0
#define OP_NORM  1
#define OP_ECMP  2

using namespace std;

struct ProInfo {
  int frame_length;
  int max_arg_length;

  ProInfo () {}
  ProInfo (int f, int m) {
    frame_length = f;
    max_arg_length = m;
  }
};

static int br_cnt = 0;

string riscv;

map<koopa_raw_value_t, int>ins_to_off;
map<koopa_raw_value_t, vector<int> >ins_to_dim;
set<string> regUsed;

string  int2Reg(int);
string  findAvailableReg();
void    freeOccupiedReg(string reg);
string  findRegFromIns(koopa_raw_value_t value);
bool    isAvailableReg(int reg);
void    parseAgg(koopa_raw_aggregate_t agg);
void    parseAgg(koopa_raw_aggregate_t agg, int& offset);
void    binary_op(string riscv_op, int op_type, koopa_raw_value_t value, int& var_point);
void    alloc_op(koopa_raw_value_t value, int& var_point);
void    get_ptr_op(koopa_raw_value_t value, int&var_point);
void    get_elem_ptr_op(koopa_raw_value_t value, int&var_point);
void    store_op(koopa_raw_value_t value, int frame_length);
void    load_op(koopa_raw_value_t value, int& var_point);
void    branch_op(koopa_raw_value_t value);
void    jump_op(koopa_raw_value_t value, int& ref_point, map<string, int>& label_to_off);
void    call_op(koopa_raw_value_t value, int& var_point);
ProInfo prologue(koopa_raw_function_t func);
void    epilogue(int frame_length);

void product_riscv(string ins_type, string dest_reg, int offset, string src_reg);
void product_riscv(string ins_type, string dest_reg, string src_reg, int imm);
void product_riscv(string ins_type, string reg, string lreg, string rreg);
void product_riscv(string ins_type, string reg, int imm);
void product_riscv(string ins_type, string dest_reg, string src_reg);
void product_riscv(string ins_type, string reg, koopa_raw_basic_block_t true_bb, koopa_raw_basic_block_t false_bb);
void product_riscv(string ins_type, string reg, koopa_raw_value_t value);
void product_riscv(string ins_type, koopa_raw_basic_block_t bb);
void product_riscv(string ins_type, koopa_raw_call_t call);
void product_riscv(string ins_type);

void product_label(string label_name);

string koopa2riscv(string koopa) {
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  koopa_error_code_t IRret = koopa_parse_from_string(koopa.c_str(), &program);
  assert(IRret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);
  
  riscv += "  .data";
  for (size_t i = 0; i < raw.values.len; ++i) {
    koopa_raw_value_t value = (koopa_raw_value_t) raw.values.buffer[i];
    riscv += "\n  .globl "+ string(value->name).substr(1) +"\n";
    product_label(value->name);
    koopa_raw_value_t init_value = (koopa_raw_value_t) value->kind.data.global_alloc.init;

    if (value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
      if (init_value->kind.tag == KOOPA_RVT_ZERO_INIT)
        riscv += "  .zero 4\n";
      else if (init_value->kind.tag == KOOPA_RVT_INTEGER)
        riscv += "  .word " + to_string(init_value->kind.data.integer.value) + "\n";
    }
    else {
      vector<int> dim;
      dim.push_back(value->ty->data.pointer.base->data.array.len);
      koopa_raw_type_t arr_base = value->ty->data.pointer.base->data.array.base;
      while (arr_base->tag == KOOPA_RTT_ARRAY) {
        dim.push_back(arr_base->data.array.len);
        arr_base = arr_base->data.array.base;
      }
      for (int i=dim.size() - 2; i>=0 ; i--)
        dim[i] *= dim[i + 1];
      dim.push_back(1);
      ins_to_dim[value] = dim;

      if (init_value->kind.tag == KOOPA_RVT_ZERO_INIT) {
        riscv += "  .zero "+ to_string(4*dim[0]) + "\n";
      }
      else {
        parseAgg(init_value->kind.data.aggregate);
      }
    }
  }

  riscv += "\n  .text";
  for (size_t i = 0; i < raw.funcs.len; ++i) {
    map<koopa_raw_value_t, vector<int> > curr_ins_to_dim = ins_to_dim;

    koopa_raw_function_t func = (koopa_raw_function_t) raw.funcs.buffer[i];

    if (func->bbs.len == 0)
      continue;

    riscv += "\n  .globl "+ string(func->name).substr(1) +"\n";
    riscv += string(func->name).substr(1) + ":\n";
    
    ProInfo pro_info = prologue(func);
    int frame_length = pro_info.frame_length;
    int var_point = pro_info.max_arg_length;
    int ref_point = frame_length - 4 - 4; // ra
    map<string, int> label_to_off;

    for (size_t j = 0; j < func->bbs.len; ++j) {
      assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
      koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];

      if (j != 0)        
        product_label(bb->name);

      if (bb->params.len != 0)
        ins_to_off[(koopa_raw_value_t) bb->params.buffer[0]] = label_to_off[bb->name];

      for (size_t k = 0; k < bb->insts.len; ++k) { 
        koopa_raw_value_t value = (koopa_raw_value_t) bb->insts.buffer[k];
        
        if (value->kind.tag == KOOPA_RVT_RETURN) {
          koopa_raw_value_t ret_value = value->kind.data.ret.value;
          if (ret_value == NULL) {
            epilogue(frame_length);
            product_riscv("ret");
          }
          else if (ret_value->kind.tag == KOOPA_RVT_INTEGER) {
            int32_t int_val = ret_value->kind.data.integer.value;
            product_riscv("li", int2Reg(REG_a0), int_val);
            epilogue(frame_length);
            product_riscv("ret");
          }
          else if (ret_value->kind.tag == KOOPA_RVT_BINARY ||
                   ret_value->kind.tag == KOOPA_RVT_ALLOC ||
                   ret_value->kind.tag == KOOPA_RVT_LOAD ||
                   ret_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
                   ret_value->kind.tag == KOOPA_RVT_CALL) {
            int offset = ins_to_off[ret_value];
            product_riscv("lw", int2Reg(REG_a0), offset, int2Reg(REG_sp));
            epilogue(frame_length);
            product_riscv("ret");
          }
          // else cout<<"ret"<<ret_value->kind.tag<<endl;
        }
        else if (value->kind.tag == KOOPA_RVT_BINARY) {
          if (value->kind.data.binary.op == KOOPA_RBO_EQ) {
            binary_op("seqz", OP_NEEQ, value, var_point);
          }else if (value->kind.data.binary.op == KOOPA_RBO_NOT_EQ) {
            binary_op("snez", OP_NEEQ, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_ADD) {
            binary_op("add", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_SUB) {
            binary_op("sub", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_MUL) {
            binary_op("mul", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_DIV) {
            binary_op("div", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_MOD) {
            binary_op("rem", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_AND) {
            binary_op("and", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_OR) {
            binary_op("or", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_XOR) {
            binary_op("xor", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_GT) {
            binary_op("sgt", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_LT) {
            binary_op("slt", OP_NORM, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_GE) {
            binary_op("slt", OP_ECMP, value, var_point);
          }
          else if (value->kind.data.binary.op == KOOPA_RBO_LE) {
            binary_op("sgt", OP_ECMP, value, var_point);
          }
        }
        else if (value->kind.tag == KOOPA_RVT_ALLOC) {
          alloc_op(value, var_point);
        }
        else if (value->kind.tag == KOOPA_RVT_GET_PTR) {
          get_ptr_op(value, var_point);
        }
        else if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
          get_elem_ptr_op(value, var_point);
        }
        else if (value->kind.tag == KOOPA_RVT_STORE) {
          store_op(value, frame_length);
        }
        else if (value->kind.tag == KOOPA_RVT_LOAD) {
          load_op(value, var_point);
        }
        else if (value->kind.tag == KOOPA_RVT_BRANCH) {
          branch_op(value);
        }
        else if (value->kind.tag == KOOPA_RVT_JUMP) {
          jump_op(value, ref_point, label_to_off);
        }
        else if (value->kind.tag == KOOPA_RVT_CALL) {
          call_op(value, var_point);
        }
        // else cout<<"value"<<value->kind.tag<<endl;
      }
    }
    ins_to_off.clear();
    ins_to_dim = curr_ins_to_dim;
  }

  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);

  return riscv;
}

string int2Reg(int cnt) {
  if (cnt == REG_x0) 
    return "x0";
  else if (cnt == REG_sp)
    return "sp";
  else if (cnt == REG_ra)
    return "ra";
  if (cnt / 7 < 1)
    return "t" + to_string(cnt);
  else
    return "a" + to_string(cnt - 7);
}

bool isAvailableReg(int reg) {
  return !regUsed.count(int2Reg(reg));
}

string findAvailableReg() {
  for (int i=0; i<=REG_t6; i++) {
    if (isAvailableReg(i)) {
      regUsed.insert(int2Reg(i));
      return int2Reg(i);
    }
  }
  // cout << "reg not enough!" << endl;
  // exit(0);
}

void freeOccupiedReg(string reg) {
  if (regUsed.count(reg))
    regUsed.erase(reg);
}

string findRegFromIns(koopa_raw_value_t value) {
  string reg;
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    if (value->kind.data.integer.value == 0)
      reg = int2Reg(REG_x0);
    else {
      reg = findAvailableReg();
      product_riscv("li", reg, value->kind.data.integer.value);
    }
  }
  else if (value->kind.tag == KOOPA_RVT_BINARY || 
           value->kind.tag == KOOPA_RVT_ALLOC ||
           value->kind.tag == KOOPA_RVT_LOAD ||
           value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
           value->kind.tag == KOOPA_RVT_CALL) {
    int offset = ins_to_off[value];
    reg = findAvailableReg();
    product_riscv("lw", reg, offset, int2Reg(REG_sp));
  }
  return reg;
}

void parseAgg(koopa_raw_aggregate_t agg, int& offset) {
  for (int i=0; i<agg.elems.len; i++) {
    koopa_raw_value_t buffer = (koopa_raw_value_t) agg.elems.buffer[i];
    if (buffer->kind.tag == KOOPA_RVT_INTEGER) {
      string reg = findAvailableReg();
      product_riscv("li", reg, buffer->kind.data.integer.value);
      product_riscv("sw", reg, offset, int2Reg(REG_sp));
      offset += 4;
      freeOccupiedReg(reg);
    }
    else {
      parseAgg(buffer->kind.data.aggregate, offset);
    }
  }
}

void parseAgg(koopa_raw_aggregate_t agg) {
  for (int i=0; i<agg.elems.len; i++) {
    koopa_raw_value_t buffer = (koopa_raw_value_t) agg.elems.buffer[i];
    if (buffer->kind.tag == KOOPA_RVT_INTEGER) {
      riscv += "  .word " + to_string(buffer->kind.data.integer.value) + "\n";
    }
    else {
      parseAgg(buffer->kind.data.aggregate);
    }
  }
}

void alloc_op(koopa_raw_value_t value, int& var_point) {
  ins_to_off[value] = var_point;
  if (value->ty->data.pointer.base->tag == KOOPA_RTT_INT32)
    var_point += 4;
  else if (value->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
    vector<int> dim;
    if (value->ty->data.pointer.base->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
      dim.push_back(value->ty->data.pointer.base->data.pointer.base->data.array.len);
      koopa_raw_type_t arr_base = value->ty->data.pointer.base->data.pointer.base->data.array.base;
      while (arr_base->tag == KOOPA_RTT_ARRAY) {
        dim.push_back(arr_base->data.array.len);
        arr_base = arr_base->data.array.base;
      }

      for (int i=dim.size() - 2; i>=0 ; i--)
        dim[i] *= dim[i + 1];
    }
    dim.push_back(1);

    ins_to_dim[value] = dim;
    var_point += 4;
  }
  else if (value->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
    vector<int> dim;
    dim.push_back(value->ty->data.pointer.base->data.array.len);
    koopa_raw_type_t arr_base = value->ty->data.pointer.base->data.array.base;
    while (arr_base->tag == KOOPA_RTT_ARRAY) {
      dim.push_back(arr_base->data.array.len);
      arr_base = arr_base->data.array.base;
    }

    for (int i=dim.size() - 2; i>=0 ; i--)
      dim[i] *= dim[i + 1];
    dim.push_back(1);

    ins_to_dim[value] = dim;
    var_point += dim[0]*4;
  }
}

void get_ptr_op(koopa_raw_value_t value, int&var_point) {
  koopa_raw_value_t src_value = value->kind.data.get_ptr.src;
  koopa_raw_value_t idx_value = value->kind.data.get_ptr.index;
  string src_reg = findAvailableReg();
  string idx_reg = findAvailableReg();

  
  int src_offset = ins_to_off[src_value];
  product_riscv("lw", src_reg, src_offset, int2Reg(REG_sp));

  if (idx_value->kind.tag == KOOPA_RVT_BINARY ||
      idx_value->kind.tag == KOOPA_RVT_ALLOC ||
      idx_value->kind.tag == KOOPA_RVT_LOAD ||
      idx_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
      idx_value->kind.tag == KOOPA_RVT_CALL) {
    int idx_offset = ins_to_off[idx_value];
    product_riscv("lw", idx_reg, idx_offset, int2Reg(REG_sp));
  }
  else if (idx_value->kind.tag == KOOPA_RVT_INTEGER) {
    product_riscv("li", idx_reg, idx_value->kind.data.integer.value);
  }

  string dim_reg = findAvailableReg();

  product_riscv("li", dim_reg, ins_to_dim[src_value][0]*4);

  product_riscv("mul", idx_reg, idx_reg, dim_reg);
  product_riscv("add", src_reg, src_reg, idx_reg);

  product_riscv("sw", src_reg, var_point, int2Reg(REG_sp));

  ins_to_off[value] = var_point;
  var_point += 4;
  ins_to_dim[value] = ins_to_dim[src_value];

  freeOccupiedReg(dim_reg);
  freeOccupiedReg(idx_reg);
  freeOccupiedReg(src_reg);
}

void get_elem_ptr_op(koopa_raw_value_t value, int&var_point) {
  koopa_raw_value_t src_value = value->kind.data.get_elem_ptr.src;
  koopa_raw_value_t idx_value = value->kind.data.get_elem_ptr.index;
  string src_reg = findAvailableReg();
  string idx_reg = findAvailableReg();

  if (src_value->kind.tag == KOOPA_RVT_ALLOC) {
    int src_offset = ins_to_off[src_value];
    product_riscv("li", src_reg, src_offset);
    product_riscv("add", src_reg, src_reg, int2Reg(REG_sp));
  }
  else if (src_value->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src_value->kind.tag == KOOPA_RVT_GET_PTR) {
    int src_offset = ins_to_off[src_value];
    product_riscv("lw", src_reg, src_offset, int2Reg(REG_sp));
  }
  else if (src_value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    product_riscv("la", src_reg, src_value);
  } 

  if (idx_value->kind.tag == KOOPA_RVT_BINARY ||
      idx_value->kind.tag == KOOPA_RVT_ALLOC ||
      idx_value->kind.tag == KOOPA_RVT_LOAD ||
      idx_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
      idx_value->kind.tag == KOOPA_RVT_CALL) {
    int idx_offset = ins_to_off[idx_value];
    product_riscv("lw", idx_reg, idx_offset, int2Reg(REG_sp));
  }
  else if (idx_value->kind.tag == KOOPA_RVT_INTEGER) {
    product_riscv("li", idx_reg, idx_value->kind.data.integer.value);
  }

  string dim_reg = findAvailableReg();

  product_riscv("li", dim_reg, ins_to_dim[src_value][1]*4);

  product_riscv("mul", idx_reg, idx_reg, dim_reg);
  product_riscv("add", src_reg, src_reg, idx_reg);

  product_riscv("sw", src_reg, var_point, int2Reg(REG_sp));

  ins_to_off[value] = var_point;
  var_point += 4;
  ins_to_dim[value] = vector<int>(ins_to_dim[src_value].begin() + 1, ins_to_dim[src_value].end());

  freeOccupiedReg(dim_reg);
  freeOccupiedReg(idx_reg);
  freeOccupiedReg(src_reg);
}

void store_op(koopa_raw_value_t value, int frame_length) {
  koopa_raw_value_t src_value = value->kind.data.store.value;
  koopa_raw_value_t dest_value = value->kind.data.store.dest;
  if (src_value->kind.tag == KOOPA_RVT_AGGREGATE) {
    int dest_offset = ins_to_off[dest_value];
    parseAgg(src_value->kind.data.aggregate, dest_offset);
    return;
  }

  string reg = findAvailableReg();

  if (src_value->kind.tag == KOOPA_RVT_BINARY ||
      src_value->kind.tag == KOOPA_RVT_ALLOC ||
      src_value->kind.tag == KOOPA_RVT_LOAD ||
      src_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
      src_value->kind.tag == KOOPA_RVT_CALL) {
    int src_offset = ins_to_off[src_value];
    product_riscv("lw", reg, src_offset, int2Reg(REG_sp));
  }
  else if (src_value->kind.tag == KOOPA_RVT_INTEGER) {
    product_riscv("li", reg, src_value->kind.data.integer.value);
  }
  else if (src_value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    int index = src_value->kind.data.func_arg_ref.index;
    if (index < REG_num) {
      freeOccupiedReg(reg);
      reg = int2Reg(REG_a0 + index);
    }
    else {
      int src_offset = frame_length + (index - REG_num)*4;
      product_riscv("lw", reg, src_offset, int2Reg(REG_sp));
    }
  }

  if (dest_value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    string dest_reg = findAvailableReg();
    product_riscv("la", dest_reg, dest_value);
    product_riscv("sw", reg, 0, dest_reg);
    freeOccupiedReg(dest_reg);
  }
  else if (dest_value->kind.tag == KOOPA_RVT_GET_ELEM_PTR || dest_value->kind.tag == KOOPA_RVT_GET_PTR) {
    string dest_reg = findAvailableReg();
    int ptr_offset = ins_to_off[dest_value];
    product_riscv("lw", dest_reg, ptr_offset, int2Reg(REG_sp));
    product_riscv("sw", reg, 0, dest_reg);
    freeOccupiedReg(dest_reg);
  }
  else {
    int dest_offset = ins_to_off[dest_value];
    product_riscv("sw", reg, dest_offset, int2Reg(REG_sp));
  }

  freeOccupiedReg(reg);
}

void load_op(koopa_raw_value_t value, int& var_point) {
  koopa_raw_value_t src_value = value->kind.data.load.src;
  string reg = findAvailableReg();

  if (src_value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    product_riscv("la", reg, src_value);
    product_riscv("lw", reg, 0, reg);
  }
  else if (src_value->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src_value->kind.tag == KOOPA_RVT_GET_PTR) {
    int ptr_offset = ins_to_off[src_value];
    product_riscv("lw", reg, ptr_offset, int2Reg(REG_sp));
    product_riscv("lw", reg, 0, reg);
  }
  else {
    int src_offset = ins_to_off[src_value];
    product_riscv("lw", reg, src_offset, int2Reg(REG_sp));
  }

  product_riscv("sw", reg, var_point, int2Reg(REG_sp));
  ins_to_off[value] = var_point;
  var_point += 4;

  if (src_value->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
    ins_to_dim[value] = ins_to_dim[src_value];

  freeOccupiedReg(reg);
}

void branch_op(koopa_raw_value_t value) {
  koopa_raw_value_t cond_value = value->kind.data.branch.cond;
  koopa_raw_basic_block_t true_bb = value->kind.data.branch.true_bb;
  koopa_raw_basic_block_t false_bb = value->kind.data.branch.false_bb;
  
  string reg = findAvailableReg();

  if (cond_value->kind.tag == KOOPA_RVT_INTEGER) {
    product_riscv("li", reg, cond_value->kind.data.integer.value);
  }
  else if (cond_value->kind.tag == KOOPA_RVT_BINARY ||
           cond_value->kind.tag == KOOPA_RVT_ALLOC ||
           cond_value->kind.tag == KOOPA_RVT_LOAD ||
           cond_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
           cond_value->kind.tag == KOOPA_RVT_CALL) {
    int src_offset = ins_to_off[cond_value];
    product_riscv("lw", reg, src_offset, int2Reg(REG_sp));
  }
  
  product_riscv("bnez", reg, true_bb, false_bb);

  freeOccupiedReg(reg);
}

void jump_op(koopa_raw_value_t value, int& ref_point, map<string, int>& label_to_off) {
  koopa_raw_basic_block_t target_bb = value->kind.data.jump.target;
  koopa_raw_slice_t args_slice = value->kind.data.jump.args;
  
  if (args_slice.len != 0) {
    if (label_to_off.count(target_bb->name) == 0) {
      label_to_off[target_bb->name] = ref_point;
      ref_point -= 4;
    }
    string reg = findAvailableReg();
    int offset = ins_to_off[(koopa_raw_value_t) args_slice.buffer[0]];
    product_riscv("lw", reg, offset, int2Reg(REG_sp));
    product_riscv("sw", reg, label_to_off[target_bb->name], int2Reg(REG_sp));
    freeOccupiedReg(reg);
  }

  product_riscv("j", target_bb);
}

void binary_op(string riscv_op, int op_type, koopa_raw_value_t value, int& var_point) {
  koopa_raw_value_t l_value = value->kind.data.binary.lhs;
  koopa_raw_value_t r_value = value->kind.data.binary.rhs;
  string lreg = findRegFromIns(l_value);
  string rreg = findRegFromIns(r_value);
  string reg = findAvailableReg();

  if (op_type == OP_NEEQ) {
    product_riscv("xor", reg, lreg, rreg);
    product_riscv(riscv_op, reg, reg);
  }
  else if (op_type == OP_NORM)
    product_riscv(riscv_op, reg, lreg, rreg);
  else if (op_type == OP_ECMP) {
    product_riscv(riscv_op, reg, lreg, rreg);
    product_riscv("seqz", reg, reg);
  }  

  product_riscv("sw", reg, var_point, int2Reg(REG_sp));
  ins_to_off[value] = var_point;
  var_point += 4;

  freeOccupiedReg(reg);
  freeOccupiedReg(rreg);
  freeOccupiedReg(lreg);
}

void call_op(koopa_raw_value_t value, int& var_point) {
  koopa_raw_slice_t args = value->kind.data.call.args;
  for (int i=0; i<args.len; i++) {
    koopa_raw_value_t arg_value = (koopa_raw_value_t) args.buffer[i];
    if (i < REG_num) {
      if (arg_value->kind.tag == KOOPA_RVT_INTEGER)
        product_riscv("li", int2Reg(REG_a0 + i), arg_value->kind.data.integer.value);
      else if (arg_value->kind.tag == KOOPA_RVT_BINARY ||
               arg_value->kind.tag == KOOPA_RVT_ALLOC ||
               arg_value->kind.tag == KOOPA_RVT_LOAD ||
               arg_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
               arg_value->kind.tag == KOOPA_RVT_CALL ||
               arg_value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) 
        product_riscv("lw", int2Reg(REG_a0 + i), ins_to_off[arg_value], int2Reg(REG_sp));
    }
    else {
      string reg = findAvailableReg();

      if (arg_value->kind.tag == KOOPA_RVT_INTEGER)
        product_riscv("li", reg, arg_value->kind.data.integer.value);
      else if (arg_value->kind.tag == KOOPA_RVT_BINARY ||
               arg_value->kind.tag == KOOPA_RVT_ALLOC ||
               arg_value->kind.tag == KOOPA_RVT_LOAD ||
               arg_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF ||
               arg_value->kind.tag == KOOPA_RVT_CALL ||
               arg_value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) 
        product_riscv("lw", reg, ins_to_off[arg_value], int2Reg(REG_sp));
    
      product_riscv("sw", reg, (i - REG_num)*4, int2Reg(REG_sp));

      freeOccupiedReg(reg);
    }
  }

  product_riscv("call", value->kind.data.call);

  if (value->ty->tag != KOOPA_RTT_UNIT) {
    product_riscv("sw", int2Reg(REG_a0), var_point, int2Reg(REG_sp));
    ins_to_off[value] = var_point;
    var_point += 4;
  }
}

ProInfo prologue(koopa_raw_function_t func) {
  int frame_length = 4; // REG_ra
  int max_arg_length = 0;
  for (size_t j = 0; j < func->bbs.len; ++j) {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j]; 
    frame_length += 4 * bb->params.len;
    for (size_t k = 0; k < bb->insts.len; ++k) { 
      koopa_raw_value_t value = (koopa_raw_value_t) bb->insts.buffer[k];
      if (value->ty->tag == KOOPA_RTT_INT32 || value->ty->tag == KOOPA_RTT_POINTER) { 
        frame_length += 4;
      }

      if (value->kind.tag == KOOPA_RVT_ALLOC) {
        if (value->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
          int dims = value->ty->data.pointer.base->data.array.len;
          koopa_raw_type_t arr_base = value->ty->data.pointer.base->data.array.base;
          while (arr_base->tag == KOOPA_RTT_ARRAY) {
            dims *= arr_base->data.array.len;
            arr_base = arr_base->data.array.base;
          }
          frame_length += dims*4;
          // redundant ptr
          frame_length -=4;
        }
      }   
      else if (value->kind.tag == KOOPA_RVT_CALL) {
        max_arg_length = max((int(value->kind.data.call.args.len) - 8)*4, max_arg_length);
      }
    }
  }

  frame_length += max_arg_length;

  // 对齐16字节
  frame_length = frame_length%16 ? (frame_length/16 + 1)*16 : frame_length;

  product_riscv("addi", int2Reg(REG_sp), int2Reg(REG_sp), -frame_length);
  
  // store REG_ra
  product_riscv("sw", int2Reg(REG_ra), frame_length - 4, int2Reg(REG_sp));

  return ProInfo(frame_length, max_arg_length);
}

void epilogue(int frame_length) {
  // load REG_ra
  product_riscv("lw", int2Reg(REG_ra), frame_length - 4, int2Reg(REG_sp));

  product_riscv("addi", int2Reg(REG_sp), int2Reg(REG_sp), frame_length);
}

// lw/sw dest_reg, offset(src_reg)
void product_riscv(string ins_type, string dest_reg, int offset, string src_reg) {
  if (offset < -2048 || offset >2047) {
    string reg = findAvailableReg();
    product_riscv("li", reg, offset);
    product_riscv("add", reg, reg, src_reg);
    riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           dest_reg + ", " + to_string(0) + "(" + reg + ")" + "\n";
    freeOccupiedReg(reg);
  }
  else {
    riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           dest_reg + ", " + to_string(offset) + "(" + src_reg + ")" + "\n";
  }
}

// addi dest_reg, src_reg, imm
void product_riscv(string ins_type, string dest_reg, string src_reg, int imm) {
  if (imm < -2048 || imm > 2047) {
    string reg = findAvailableReg();
    product_riscv("li", reg, imm);
    product_riscv("add", dest_reg, src_reg, reg);
    freeOccupiedReg(reg);
  }
  else {
    riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           dest_reg + ", " + src_reg + ", " + to_string(imm) + "\n";
  }
}

// add reg, lreg, rreg
void product_riscv(string ins_type, string reg, string lreg, string rreg) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           reg + ", " + lreg + ", " + rreg + "\n";
}

// li reg, imm
void product_riscv(string ins_type, string reg, int imm) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           reg + ", " + to_string(imm) + "\n";
}

// mv dest_reg, src_reg
void product_riscv(string ins_type, string dest_reg, string src_reg) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           dest_reg + ", " + src_reg + "\n";
}

// bnez reg, bb_name
void product_riscv(string ins_type, string reg, koopa_raw_basic_block_t true_bb, koopa_raw_basic_block_t false_bb) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           reg + ", br_" + to_string(br_cnt) + "\n";
  product_riscv("j", false_bb);
  product_label("%br_" + to_string(br_cnt++));
  product_riscv("j", true_bb);
}

// la reg, addr
void product_riscv(string ins_type, string reg, koopa_raw_value_t value) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           reg + ", " + string(value->name).substr(1) + "\n";
}

// j bb_name
void product_riscv(string ins_type, koopa_raw_basic_block_t bb) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           string(bb->name).substr(1) + "\n";  
}

// call func_name
void product_riscv(string ins_type, koopa_raw_call_t call) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           string(call.callee->name).substr(1) + "\n";  
}

// ret
void product_riscv(string ins_type) {
  riscv += "  " + ins_type + "\n";
}

void product_label(string label_name) {
  riscv += label_name.substr(1) + ":\n";
}
