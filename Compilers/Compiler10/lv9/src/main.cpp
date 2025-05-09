#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "ast.h"
#include "koopa.h"
#include <cstring>
#include <unordered_map>
//#define DEBUG 1
#define OUTPUT_TO_FILE 1
// #define MOVE_INPUT_FILE 1

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
const char tmp_reg_name[15][3] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
#define ull unsigned long long
static unordered_map<koopa_raw_value_t, int> reg_map;
static koopa_raw_value_t reg_addr[7];
static int inst_id = 0;
static int globl_stack_sz = 0;
static int globl_stack_pos = 0;
static int globl_func_called = 0;
static int local_calc_stackaddr = 0;
static koopa_raw_value_t local_proc_value;
static unordered_map<koopa_raw_value_t, int> stack_symbol_table;
static string globl_func_name; 

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary);
void Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_get_ptr_t &get_ptr);
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
void Visit(const koopa_raw_call_t &call, int is_unit);
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr);
void Visit(const koopa_raw_get_ptr_t &get_ptr);
void Visit_global_alloc(const koopa_raw_value_t &value);
void get_cur_addr(const koopa_raw_value_t &value);
int get_stack_addr(const koopa_raw_value_t &value);
int alloc_reg_name();
int calc_funcsz(const koopa_raw_function_t &func, int& called);
int calc_blksz(const koopa_raw_basic_block_t &bb, int& called);
int calc_instsz(const koopa_raw_value_t &value);
int calc_typesz(const koopa_raw_type_t &type);

int get_stack_addr(const koopa_raw_value_t &value){
  if(stack_symbol_table.count(value)){
    return stack_symbol_table[value];
  }
  int ret = globl_stack_pos;
  int sz = calc_instsz(value);
  if(!sz) return -1;
  globl_stack_pos -= sz;
  ret -= sz;
  stack_symbol_table[value] = ret;
  return ret;
}

void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  printf("  .data\n");
  Visit(program.values);
  // 访问所有函数
  printf("  .text\n");
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
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

int calc_funcsz(const koopa_raw_function_t &func, int& called){
  int sum = 0;
  for (uint32_t i = 0; i < func->bbs.len; i++){
    const void *data = func->bbs.buffer[i];
    sum += calc_blksz((koopa_raw_basic_block_t)data, called);
  }
  if(called) sum += 4;
  return sum;
}
int calc_blksz(const koopa_raw_basic_block_t &bb, int& called){
  int sum = 0;
  for (uint32_t i = 0; i < bb->insts.len; i++){
    const void *data = bb->insts.buffer[i];
    if(((koopa_raw_value_t)data)->kind.tag == KOOPA_RVT_CALL) called = 1;
    sum += calc_instsz((koopa_raw_value_t)data);
  }
  return sum;
}
int calc_instsz(const koopa_raw_value_t &value){
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_ALLOC:
      return calc_typesz(value->ty->data.pointer.base);
    default:
      return calc_typesz(value->ty);
  }
  return 0;
}

int calc_typesz(const koopa_raw_type_t &type){
  switch (type->tag) {
    case KOOPA_RTT_POINTER:
      return 4;
    case KOOPA_RTT_INT32:
      return 4;
    case KOOPA_RTT_ARRAY:
      return type->data.array.len * calc_typesz(type->data.array.base);
    case KOOPA_RTT_UNIT:
      return 0;
    case KOOPA_RTT_FUNCTION:
      assert(false);
  }
  return 0;
}

void store_to_stack(int addr, int reg_name){
  assert(0<=reg_name && reg_name<=4 || reg_name == 7);
  if(-2048 <= addr && addr <= 2047){
    printf("  sw %s, %d(sp)\n", tmp_reg_name[reg_name], addr);
  }else{
    printf("  li %s, %d\n", tmp_reg_name[5], addr);
    printf("  add %s, %s, sp\n", tmp_reg_name[5], tmp_reg_name[5]);
    printf("  sw %s, 0(%s)\n", tmp_reg_name[reg_name], tmp_reg_name[5]);
  }
}

void add_val_to_reg(koopa_raw_value_t val, int reg_name, int stackaddr){
  #ifdef DEBUG
    printf("storing reg %d\n", reg_name);
  #endif
  reg_map[val] = reg_name;
  reg_addr[reg_name] = val;
}
void evict_reg(int reg){
  #ifdef DEBUG
    printf("evicting reg %d\n", reg);
  #endif
  if(reg_map.count(reg_addr[reg])){
    store_to_stack(stack_symbol_table[reg_addr[reg]], reg);
    reg_map.erase(reg_addr[reg]);
  }
}
void evict_all_regs(){
  vector<int> reg_to_evict;
  for( auto &reg : reg_map){
    reg_to_evict.push_back(reg.second);
  }
  for(auto &reg : reg_to_evict){
    evict_reg(reg);
  }
}
/*
如果 default_reg != -1, 说明是临时存放，如果需要新开寄存器，就用default_reg
*/
void load_raw_value(const koopa_raw_value_t &val, char* reg, int default_reg = -1){
  #ifdef DEBUG
    printf("loading value name: \"%s\", default_reg = %d\n", val->name, default_reg);
  #endif
  if(reg_map.count(val)){
    if(default_reg >= 7){
      printf("  mv %s, %s\n", tmp_reg_name[default_reg], tmp_reg_name[reg_map[val]]);
      sprintf(reg, "%s", tmp_reg_name[default_reg]);
    }else if(default_reg >= 5){
      sprintf(reg, "%s", tmp_reg_name[reg_map[val]]);
    }else{
      int reg_name = reg_map[val];
      sprintf(reg, "%s", tmp_reg_name[reg_name]);
      if(val != local_proc_value && local_calc_stackaddr != -1){
        evict_reg(reg_name);
        add_val_to_reg(local_proc_value, reg_name, local_calc_stackaddr);
      }
    }
    return;
  }
  if(default_reg != -1){
    sprintf(reg, "%s", tmp_reg_name[default_reg]);
  }else{
    int new_inst_reg = alloc_reg_name();
    add_val_to_reg(local_proc_value, new_inst_reg, get_stack_addr(val));
    sprintf(reg, "%s", tmp_reg_name[new_inst_reg]);
  }
  if(val->kind.tag == KOOPA_RVT_INTEGER){
    int32_t int_val = val->kind.data.integer.value;
    printf("  li %s, %d\n", reg, int_val);
  }else if (val->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    printf("  la %s, %s\n", reg, val->name + 1);
    printf("  lw %s, 0(%s)\n", reg, reg);
  }else{
    int addr = get_stack_addr(val);
    if(-2048 <= addr && addr <= 2047){
      printf("  lw %s, %d(sp)\n", reg, addr);
    }else{
      printf("  li %s, %d\n", reg, addr);
      printf("  add %s, %s, sp\n", reg, reg);
      printf("  lw %s, 0(%s)\n", reg, reg);
    }
  }
}

void load_raw_ptr(const koopa_raw_value_t &val, char* reg){
  int new_inst_reg = alloc_reg_name();
  sprintf(reg, "%s", tmp_reg_name[new_inst_reg]);
  if (val->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    printf("  la %s, %s\n", reg, val->name + 1);
  }else{
    int addr = get_stack_addr(val);
    if(-2048 <= addr && addr <= 2047){
      if(reg_map.count(val)){
        printf("  mv %s, %s\n", reg, tmp_reg_name[reg_map[val]]);
      }else{
        printf("  addi %s, sp, %d\n", reg, addr);
        if(val->kind.tag == KOOPA_RVT_GET_ELEM_PTR || val->kind.tag == KOOPA_RVT_GET_PTR || val->kind.tag == KOOPA_RVT_LOAD){
          printf("  lw %s, 0(%s)\n", reg, reg);
        }
      }
    }else{
      if(reg_map.count(val)){
        printf("  mv %s, %s\n", reg, tmp_reg_name[reg_map[val]]);
      }else{
        printf("  li %s, %d\n", reg, addr);
        printf("  add %s, %s, sp\n", reg, reg);
        if(val->kind.tag == KOOPA_RVT_GET_ELEM_PTR || val->kind.tag == KOOPA_RVT_GET_PTR || val->kind.tag == KOOPA_RVT_LOAD){
          printf("  lw %s, 0(%s)\n", reg, reg);
        }
      }
    }
  }
}

int alloc_reg_name(){
  int new_inst_reg = inst_id;
  inst_id = (inst_id + 1) % 5;
  evict_reg(new_inst_reg);
  return new_inst_reg;
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  if(func->bbs.len == 0) return;
  printf("\n  .globl %s\n", (func->name)+1 );
  printf("%s:\n", (func->name)+1 );
  int func_called = 0;
  int total_sz = calc_funcsz(func, func_called);
  if(total_sz){
      total_sz = ((total_sz + 15) / 16) * 16;
      if(-2048 <= total_sz && total_sz <= 2047){
          printf("  addi sp, sp, %d\n", -total_sz);
      }else{
          printf("  li t0, %d\n", -total_sz);
          printf("  add sp, sp, t0\n");
      }
  }
  if(func_called){
      int offset = total_sz - 4;
      if(-2048 <= offset && offset <= 2047){
          printf("  sw ra, %d(sp)\n", offset);
      }else{
          printf("  li t0, %d\n", offset);
          printf("  add t0, sp, t0\n");
          printf("  sw ra, 0(t0)\n");
      }
  }
  stack_symbol_table.clear();
  reg_map.clear();
  globl_stack_sz = total_sz;
  globl_stack_pos = total_sz - func_called * 4;
  globl_func_called = func_called;
  globl_func_name = func->name + 1;
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  printf("\n%s_block_%s:\n", globl_func_name.c_str(), bb->name+1);
  Visit(bb->insts);
  evict_all_regs();
}

void Visit(const koopa_raw_get_ptr_t &get_ptr){
  char cur_src_reg[3];
  load_raw_ptr(get_ptr.src, cur_src_reg);
  
  char cur_index_reg[3];
  load_raw_value(get_ptr.index, cur_index_reg, 6);
  int type_length = calc_typesz(get_ptr.src->ty->data.pointer.base);
  printf("  li %s, %d\n", tmp_reg_name[5], type_length);
  printf("  mul %s, %s, %s\n", cur_index_reg, cur_index_reg, tmp_reg_name[5]);
  printf("  add %s, %s, %s\n", cur_src_reg, cur_src_reg, cur_index_reg);

  // int new_inst_reg = alloc_reg_name();
  // add_val_to_reg(local_proc_value, new_inst_reg, local_calc_stackaddr);
  add_val_to_reg(local_proc_value, (inst_id+4)%5, local_calc_stackaddr);
}

void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr){
  char cur_src_reg[3];
  load_raw_ptr(get_elem_ptr.src, cur_src_reg);
  char cur_index_reg[3];
  load_raw_value(get_elem_ptr.index, cur_index_reg, 6);
  int type_length = calc_typesz(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
  printf("  li %s, %d\n", tmp_reg_name[5], type_length);
  printf("  mul %s, %s, %s\n", cur_index_reg, cur_index_reg, tmp_reg_name[5]);
  printf("  add %s, %s, %s\n", cur_src_reg, cur_src_reg, cur_index_reg);
  // int new_inst_reg = alloc_reg_name();
  // add_val_to_reg(local_proc_value, new_inst_reg, local_calc_stackaddr);
  add_val_to_reg(local_proc_value, (inst_id+4)%5, local_calc_stackaddr);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void Visit(const koopa_raw_call_t &call, int is_unit){
  for(int i=0;i<min(call.args.len, 8);i++){
    char cur_reg[3];
    load_raw_value((koopa_raw_value_t)(call.args.buffer[i]), cur_reg, i+7);
  }
  int func_called = 0;
  int total_size = calc_funcsz(call.callee, func_called);
  if(total_size) total_size = ((total_size + 15) / 16) * 16;
  for(int i=8;i<call.args.len;i++){
    char cur_reg[3];
    load_raw_value((koopa_raw_value_t)(call.args.buffer[i]), cur_reg, 6);
    int pos = (i-8)*4 - total_size;
    if(-2048 <= pos && pos <= 2047){
      printf("  sw %s, %d(sp)\n", cur_reg, pos);
    }else{
      printf("  li %s, %d\n", tmp_reg_name[5], pos);
      printf("  add %s, sp, %s\n", tmp_reg_name[5], tmp_reg_name[5]);
      printf("  sw %s, 0(%s)\n", cur_reg, tmp_reg_name[5]);
    }
  }
  #ifdef DEBUG
    printf("  reg_map_size: %lu\n", reg_map.size());
  #endif
  evict_all_regs();
  printf("  call %s\n", call.callee->name + 1);
  if(!is_unit){
    store_to_stack(local_calc_stackaddr, 7);
  }
}

static int globl_branch_cnt = 0;
void Visit(const koopa_raw_branch_t &branch){
  char cur_reg[3];
  load_raw_value(branch.cond, cur_reg, 6);
  evict_all_regs();
  printf("  beqz %s, %s_block_%s_near%d\n", cur_reg, globl_func_name.c_str(), branch.false_bb->name + 1, globl_branch_cnt);
  printf("  j %s_block_%s\n", globl_func_name.c_str(), branch.true_bb->name + 1);
  printf("\n%s_block_%s_near%d:\n", globl_func_name.c_str(), branch.false_bb->name + 1, globl_branch_cnt);
  printf("  j %s_block_%s\n", globl_func_name.c_str(), branch.false_bb->name + 1);
  globl_branch_cnt++;
}

void Visit(const koopa_raw_jump_t &jump){
  evict_all_regs();
  printf("  j %s_block_%s\n", globl_func_name.c_str(), jump.target->name + 1);
}

void Visit(const koopa_raw_load_t &load){
  if(load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || load.src->kind.tag == KOOPA_RVT_GET_PTR){
    char cur_reg[3];
    load_raw_value(load.src, cur_reg);
    //evict_reg((inst_id+4)%5);
    printf("  lw %s, 0(%s)\n", cur_reg, cur_reg);
    //add_val_to_reg(local_proc_value, (inst_id+4)%5, local_calc_stackaddr);
  }else{
    char cur_reg[3];
    load_raw_value(load.src, cur_reg);
    //evict_reg((inst_id+4)%5);
    //add_val_to_reg(local_proc_value, (inst_id+4)%5, local_calc_stackaddr);
  }
}

void Visit(const koopa_raw_store_t &store) {
  auto tag = store.dest->kind.tag;
  string store_dest = "";
  if(tag == KOOPA_RVT_GLOBAL_ALLOC){
    printf("  la %s, %s\n", tmp_reg_name[5], store.dest->name + 1);
    store_dest = string_format("0(%s)", tmp_reg_name[5]);
  }else if(tag == KOOPA_RVT_GET_ELEM_PTR || tag == KOOPA_RVT_GET_PTR){
    char cur_reg[3];
    load_raw_value(store.dest, cur_reg, 5);
    store_dest = string_format("0(%s)", cur_reg);
  }else{
    #ifdef DEBUG
      cout<< store.dest->name << " " << get_stack_addr(store.dest) << endl;
    #endif
    int stack_addr = get_stack_addr(store.dest);
    #ifdef DEBUG
      printf("store in stack %d\n", stack_addr);
    #endif
    if(-2048 <= stack_addr && stack_addr <= 2047){
      store_dest = string_format("%d(sp)", stack_addr);
    }else{
      printf("  li %s, %d\n", tmp_reg_name[5], stack_addr);
      printf("  add %s, sp, %s\n", tmp_reg_name[5], tmp_reg_name[5]);
      store_dest = string_format("0(%s)", tmp_reg_name[5]);
    }
  }
  if(store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
    if(store.value->kind.data.func_arg_ref.index < 8){
      printf("  sw a%d, %s\n", (int)store.value->kind.data.func_arg_ref.index, store_dest.c_str());
    }else{
      int pos = (store.value->kind.data.func_arg_ref.index - 8) * 4;
      if(-2048 <= pos && pos <= 2047){
        printf("  lw %s, %d(sp)\n", tmp_reg_name[6], pos);
      }else{
        printf("  li %s, %d\n", tmp_reg_name[6], pos);
        printf("  add %s, sp, %s\n", tmp_reg_name[6], tmp_reg_name[6]);
        printf("  lw %s, 0(%s)\n", tmp_reg_name[6], tmp_reg_name[6]);
      }
      printf("  sw %s, %s\n", tmp_reg_name[6], store_dest.c_str());
    }
  }else if(store.value->kind.tag == KOOPA_RVT_ZERO_INIT){
    return;
    // may be wrong
  }else{
    char cur_reg[3];
    load_raw_value(store.value, cur_reg, 6);
    printf("  sw %s, %s\n", cur_reg, store_dest.c_str());
  } 
}

void Visit_aggregate(const koopa_raw_value_t &value){
  if(value->ty->tag == KOOPA_RTT_ARRAY){
    for(int i = 0; i < value->kind.data.aggregate.elems.len; i++){
      Visit_aggregate((koopa_raw_value_t)(value->kind.data.aggregate.elems.buffer[i]));
    }
  }else printf("  .word %d\n", value->kind.data.integer.value); 
}

void Visit_global_alloc(const koopa_raw_value_t &value){
  printf(".globl %s\n", value->name + 1);
  printf("%s:\n", value->name + 1);
  if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
    printf("  .zero %d\n", calc_typesz(value->ty->data.pointer.base));
  }else if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE){
    Visit_aggregate(value->kind.data.global_alloc.init);
  }else{
    printf("  .word %d\n", value->kind.data.global_alloc.init->kind.data.integer.value);
  }
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  get_cur_addr(value);
  if(local_calc_stackaddr != -1){
    stack_symbol_table[value] = local_calc_stackaddr;
  }
  #ifdef DEBUG
    printf("%d\n",local_calc_stackaddr);
  #endif 
  local_proc_value = value;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      #ifdef DEBUG
        printf("inst return\n");
      #endif
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      #ifdef DEBUG
        printf("inst integer\n");
      #endif
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      #ifdef DEBUG
        printf("inst binary\n");
      #endif
      Visit(kind.data.binary);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      #ifdef DEBUG
        printf("inst load\n");
      #endif
      Visit(kind.data.load);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      #ifdef DEBUG
        printf("inst store\n");
      #endif
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_ALLOC:
      #ifdef DEBUG
        printf("inst alloc\n");
      #endif
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      #ifdef DEBUG
        printf("inst global alloc\n");
      #endif
      Visit_global_alloc(value);
      break;
    case KOOPA_RVT_GET_PTR:
      #ifdef DEBUG
        printf("inst get ptr\n");
      #endif
      Visit(kind.data.get_ptr);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      #ifdef DEBUG
        printf("inst get elem ptr\n");
      #endif
      Visit(kind.data.get_elem_ptr);
      break;
    case KOOPA_RVT_CALL:
      #ifdef DEBUG
        printf("inst call\n");
      #endif
      Visit(kind.data.call, value->ty->tag == KOOPA_RTT_UNIT);
      break;
    case KOOPA_RVT_BRANCH:
      #ifdef DEBUG
        printf("inst branch\n");
      #endif
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      #ifdef DEBUG
        printf("inst jump\n");
      #endif
      Visit(kind.data.jump);
      break;
    default:
      printf("%d\n", kind.tag);
      assert(false);
  }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret) {
  // 访问返回值
  if(ret.value){
    char cur_reg[3];
    load_raw_value(ret.value, cur_reg, 7);
  }
  if(globl_func_called){ // restore ra
    int ra_pos = globl_stack_sz - 4;
    if(-2048 <= ra_pos && ra_pos <= 2047){
      printf("  lw ra, %d(sp)\n", ra_pos);
    }else{
      printf("  li %s, %d\n", tmp_reg_name[5], ra_pos);
      printf("  add %s, sp, %s\n", tmp_reg_name[5], tmp_reg_name[5]);
      printf("  lw ra, 0(%s)\n", tmp_reg_name[5]);
    }
  }
  if(globl_stack_sz){
    if(-2048 <= globl_stack_sz && globl_stack_sz <= 2047){
      printf("  addi sp, sp, %d\n", globl_stack_sz);
    }else{
      printf("  li %s, %d\n", tmp_reg_name[5], globl_stack_sz);
      printf("  add sp, sp, %s\n", tmp_reg_name[5]);
    }
  }
  printf("  ret\n");  
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer) {
  // 访问整数值
  int32_t value = integer.value;
  printf("%d\n", value);
  //printf("  li a0, %d\n", value);
}

void get_cur_addr(const koopa_raw_value_t &value){
  int ret = get_stack_addr(value);
  local_calc_stackaddr = ret;
}

void Visit(const koopa_raw_binary_t &binary) {
  // 访问二元运算
  //printf("%s\n", binary.op);
  //printf("%s\n", binary.lhs->name);
  //printf("%s\n", binary.rhs->name);
  char lhs_reg[3];
  // Visit(binary.lhs);
  load_raw_value(binary.lhs, lhs_reg, 5);
  char rhs_reg[3];
  load_raw_value(binary.rhs, rhs_reg, 6);
  int new_inst_reg = alloc_reg_name();
  add_val_to_reg(local_proc_value, new_inst_reg, local_calc_stackaddr);
  switch(binary.op){
    case KOOPA_RBO_ADD:
      printf("  add %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_SUB:
      printf("  sub %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_EQ:
      printf("  xor %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      printf("  seqz %s, %s\n", tmp_reg_name[new_inst_reg], tmp_reg_name[new_inst_reg]);
      break;
    case KOOPA_RBO_MUL:
      printf("  mul %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_DIV:
      printf("  div %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_MOD:
      printf("  rem %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_NOT_EQ:
      printf("  xor %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      printf("  snez %s, %s\n", tmp_reg_name[new_inst_reg], tmp_reg_name[new_inst_reg]);
      break;
    case KOOPA_RBO_GT:
      printf("  sgt %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_LT:
      printf("  slt %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_GE:
      printf("  slt %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      printf("  xori %s, %s, 1\n", tmp_reg_name[new_inst_reg], tmp_reg_name[new_inst_reg]);
      break;
    case KOOPA_RBO_LE:
      printf("  sgt %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      printf("  xori %s, %s, 1\n", tmp_reg_name[new_inst_reg], tmp_reg_name[new_inst_reg]);
      break;
    case KOOPA_RBO_AND:
      printf("  and %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_OR:
      printf("  or %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_XOR:
      printf("  xor %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_SHL:
      printf("  sll %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_SHR:
      printf("  srl %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    case KOOPA_RBO_SAR:
      printf("  sra %s, %s, %s\n", tmp_reg_name[new_inst_reg], lhs_reg, rhs_reg);
      break;
    default:
      printf("UNKNOWN BINARY OP %d\n", binary.op);
      assert(false);
  }
}

int main(int argc, const char *argv[])
{
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  #ifdef MOVE_INPUT_FILE
  char command[256];
  snprintf(command, sizeof(command), "cp %s ./debug/", input);
  system(command);
  #endif

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  #ifdef OUTPUT_TO_FILE
    freopen(output, "w", stdout);
  #endif
  assert(yyin);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto yyparse_ret = yyparse(ast);
  assert(!yyparse_ret);

  // dump AST
  // ast->Dump();
  // cout << endl;
  BlockAST *compunit = dynamic_cast<BlockAST *>(ast.get());
  unique_ptr<string> exprir = compunit->GetIR();
  const char *str = exprir->c_str();
  if (strcmp(mode, "-koopa") == 0) {
    printf("%s", str);
    return 0;
  }
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(str, &program);
  assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  if(strcmp(mode, "-riscv") == 0 || strcmp(mode, "-perf") == 0)  {
    Visit(raw);
  }


  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);
  return 0;
}
