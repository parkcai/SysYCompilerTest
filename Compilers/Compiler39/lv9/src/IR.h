#include<koopa.h>
#include<string.h>
#include<cassert>
#include<stdio.h>
#include<map>
using namespace std;
static int max_size = 0;
static int now_size = 0;
static bool if_call = false;
static int max_params = 0;
static bool if_alloc = 0;
static std::map<koopa_raw_value_t,int> st;
static int wxm = 0;

void Visit(const koopa_raw_program_t &program,FILE* fp);
void Visit(const koopa_raw_slice_t &slice,FILE* fp);
void Visit(const koopa_raw_function_t &func,FILE* fp) ;
void Visit(const koopa_raw_basic_block_t &bb,FILE* fp);
void Visit(const koopa_raw_value_t &value,FILE* c) ;
void Visit(const koopa_raw_return_t &value,FILE* c);
void Visit(const koopa_raw_integer_t &value,FILE* c);
void Visit(const koopa_raw_binary_t &value,FILE* fp);
void Visit(const koopa_raw_store_t &store,FILE* fp);
void Visit(const koopa_raw_load_t &load,FILE* fp);
void Visit(const koopa_raw_jump_t &jump,FILE* fp);
void Visit(const koopa_raw_branch_t &branch,FILE* fp);
void Visit(const koopa_raw_call_t &call,FILE* fp);
void Visit(const koopa_raw_global_alloc_t &alloc,FILE* fp);
void Visit(const koopa_raw_aggregate_t &agg,FILE* fp);
void Visit(const koopa_raw_get_elem_ptr_t &gep,FILE* fp);
void Visit(const koopa_raw_get_ptr_t &gp,FILE* fp);

int CalcSize(const koopa_raw_function_t &func);
int CalcSize(const koopa_raw_basic_block_t &bb);
int CalcSize(const koopa_raw_value_t &value);

int calc_len(const koopa_raw_type_kind_t *base){
    if(base->tag == KOOPA_RTT_INT32){
      return 4;
    }
    else if(base->tag == KOOPA_RTT_POINTER){
      return 4;
    }
    else{
      printf("ss%d\n", base->data.array.len);
      return (int)(base->data.array.len)*calc_len(base->data.array.base);
    }
}

int CalcSize(const koopa_raw_function_t &func){
  int size = 0;
  if_call = false;
  max_params = 0;
  for (size_t i = 0; i < func->bbs.len; ++i) {
    auto ptr = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    size += CalcSize(ptr);
  }
  if(if_call) size += 4;
  size += max_params*4;
  while(size % 16 != 0){
    size += 4;
  }
  
  return size;
}

int CalcSize(const koopa_raw_basic_block_t &bb){
  int size = 0;
  for (size_t i = 0; i < bb->insts.len; ++i) {
    auto ptr = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]);
    size += CalcSize(ptr);
  }
  return size;
}

int CalcSize(const koopa_raw_value_t &value){
  const auto &kind = value->kind;
  if(kind.tag == KOOPA_RVT_CALL){
    if_call = true;
    if(int(value->kind.data.call.args.len) - 8 > max_params) max_params = value->kind.data.call.args.len - 8;
  }
  if(kind.tag == KOOPA_RVT_ALLOC){
    return 4 + calc_len(value->ty->data.pointer.base);
  }
  return 4;
}



void distill(const koopa_raw_value_t& value,FILE* fp,int reg){
  if(value->kind.tag == KOOPA_RVT_INTEGER){
    fprintf(fp,"  li t%d, %d\n",reg,value->kind.data.integer.value);
  }
  else if(value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
    if(value->kind.data.func_arg_ref.index >= 8){
      fprintf(fp,"  li t%d, %d\n", reg, max_size + (int(value->kind.data.func_arg_ref.index)-8)*4);
      fprintf(fp,"  add t%d, sp, t%d\n", reg, reg);
      fprintf(fp,"  lw t%d, 0(t%d)\n", reg , reg);
    }
    else{
      fprintf(fp,"  mv t%d, a%d\n", reg, int(value->kind.data.func_arg_ref.index));
    }
  }
  else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
      fprintf(fp, "  la t%d, %s\n", reg, &value->name[1]);
  }

  else{
    fprintf(fp,"  li t%d, %d\n", reg, max_size-st[value]);
    fprintf(fp,"  add t%d, sp, t%d\n", reg , reg);
    fprintf(fp,"  lw t%d, 0(t%d)\n",reg , reg);
  }
}

// 访问 raw program
void Visit(const koopa_raw_program_t &program,FILE* fp) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values,fp);
  // 访问所有函数
  Visit(program.funcs,fp);
  ;
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice,FILE* fp) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr),fp);
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr),fp);
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr),fp);
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func,FILE* fp) {
  // 执行一些其他的必要操作
  if(func->bbs.len == 0) return;
  max_size = CalcSize(func);
  now_size = 0;
  fprintf(fp,"  \n.text\n");
  fprintf(fp,"  .globl %s\n%s:\n",&func->name[1],&func->name[1]);
  fprintf(fp,"  li t0, %d\n",max_size);
  fprintf(fp,"  sub sp, sp, t0\n");
  if(if_call){
    now_size += 4;
    fprintf(fp,"  li t1, %d\n",max_size-now_size);
    fprintf(fp,"  add t1, sp, t1\n");
    fprintf(fp,"  sw ra, 0(t1)\n");
  }
  // 访问所有基本块
  Visit(func->bbs,fp);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb,FILE* fp) {
  // 执行一些其他的必要操作
  if(strcmp(&bb->name[1], "entry")!=0) fprintf(fp,"%s:\n",&bb->name[1]);
  // 访问所有指令
  return Visit(bb->insts,fp);
}


// 访问指令
void Visit(const koopa_raw_value_t &value,FILE* fp) {
  // 根据指令类型判断后续需要如何访问
  //printf("fxxk\n");
  now_size += 4;
  st[value] = now_size;
  const auto &kind = value->kind;
  const koopa_raw_global_alloc_t &alloc = value->kind.data.global_alloc;

  //printf("%d\n",kind.tag);
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_BINARY:
      Visit(kind.data.binary,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_ALLOC:
      now_size += calc_len(value->ty->data.pointer.base);
      st[value] = now_size;
      fprintf(fp,"  li t0, %d\n",max_size-now_size);
      fprintf(fp,"  add t0, t0, sp\n");
      fprintf(fp,"  li t1, %d\n",max_size-now_size+4);
      fprintf(fp,"  add t1, t1, sp\n");
      fprintf(fp,"  sw t1, 0(t0)\n");
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_CALL:
      Visit(kind.data.call,fp);
      fprintf(fp,"\n");
      break;
    case KOOPA_RVT_AGGREGATE:
      Visit(kind.data.aggregate,fp);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr,fp);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr,fp);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      if_alloc = true;
      fprintf(fp, "  .data\n  .globl %s\n%s:\n", &value->name[1], &value->name[1]);
      if(alloc.init->kind.tag == KOOPA_RVT_INTEGER){
        fprintf(fp,"  .word %d\n",alloc.init->kind.data.integer.value);
      }
      if(alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
        if(value->ty->tag == KOOPA_RTT_INT32){
          fprintf(fp,"  .zero 4\n");
        }
        if(value->ty->tag == KOOPA_RTT_POINTER){
          fprintf(fp,"  .zero %d\n", calc_len(value->ty->data.pointer.base));
        }
      }
      if(alloc.init->kind.tag == KOOPA_RVT_AGGREGATE){
        Visit(alloc.init->kind.data.aggregate, fp);
      }
      if_alloc = false;
      fprintf(fp,"\n");
      break;
    default:
      return;
      // 其他类型暂时遇不到
      assert(false);
  }
  ;
}

void Visit(const koopa_raw_get_ptr_t &gp, FILE *fp){
    distill(gp.src, fp, 0);
    distill(gp.index, fp, 1);
    const auto &base = gp.src->ty->data.pointer.base;
    fprintf(fp, "  li t2, %d\n", calc_len(base));
    fprintf(fp, "  mul t1, t1, t2\n");
    fprintf(fp, "  add t0, t0, t1\n");
    fprintf(fp, "  li t1, %d\n", max_size-now_size);
    fprintf(fp, "  add t1, t1, sp\n");
    fprintf(fp, "  sw t0, 0(t1)\n");
    
}

void Visit(const koopa_raw_get_elem_ptr_t &gep, FILE* fp){
    distill(gep.src, fp, 0);
    distill(gep.index, fp, 1);
    const auto &base = gep.src->ty->data.pointer.base->data.array.base;
    fprintf(fp, "  li t2, %d\n", calc_len(base));
    fprintf(fp, "  mul t1, t1, t2\n");
    fprintf(fp, "  add t0, t0, t1\n");
    fprintf(fp, "  li t1, %d\n", max_size-now_size);
    fprintf(fp, "  add t1, t1, sp\n");
    fprintf(fp, "  sw t0, 0(t1)\n");
}

void Visit(const koopa_raw_aggregate_t &agg, FILE* fp){
    for(int i =0; i < agg.elems.len; i++){
      Visit(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]),fp);
    }
}

void Visit(const koopa_raw_global_alloc_t &alloc, FILE* fp){
    return;
}

void Visit(const koopa_raw_call_t &call,FILE* fp){
    fprintf(fp, "  mv t1, sp\n");
    for(int i = 0; i<8 && i<call.args.len; i++){
      distill(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]),fp,0);
      fprintf(fp,"  mv a%d, t0\n",i);
    }
    for(int i = 8; i<call.args.len; i++){
      distill(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]),fp,0);
      fprintf(fp,"  sw t0, 0(t1)\n");
      fprintf(fp,"  addi t1, t1, 4\n");
    }
    fprintf(fp, "  call %s\n", &call.callee->name[1]);
    fprintf(fp, "  li t1, %d\n",max_size-now_size);
    fprintf(fp, "  add t1, sp, t1\n");
    fprintf(fp, "  sw a0, 0(t1)\n");
}

void Visit(const koopa_raw_store_t &store,FILE* fp){
    distill(store.value,fp,0);
    distill(store.dest,fp,1);
    fprintf(fp,"  sw t0, 0(t1)\n");
}

void Visit(const koopa_raw_load_t &load,FILE* fp){
    distill(load.src,fp,0);
    fprintf(fp,"  lw t0, 0(t0)\n");
    fprintf(fp,"  li t1, %d\n",max_size-now_size);
    fprintf(fp,"  add t1, sp, t1\n");
    fprintf(fp,"  sw t0, 0(t1)\n");
}

void Visit(const koopa_raw_return_t &ret,FILE* fp){
    if(ret.value != NULL){
      distill(ret.value,fp,0);
      fprintf(fp,"  mv a0, t0\n");
    }
    if(if_call){
      fprintf(fp,"  li t1, %d\n",max_size-4);
      fprintf(fp,"  add t1, sp, t1\n");
      fprintf(fp,"  lw ra, 0(t1)\n");
    }
    fprintf(fp,"  li t1, %d\n",max_size);
    fprintf(fp,"  add sp, sp, t1\n");
    fprintf(fp,"  ret\n");
}

void Visit(const koopa_raw_integer_t &integer,FILE* fp){
    if(if_alloc){
      fprintf(fp, "  .word %d\n", integer.value);
    }
    return;
}

void Visit(const koopa_raw_binary_t &value,FILE* fp){
    koopa_raw_value_t l = value.lhs, r = value.rhs;
    switch(value.op){
      case KOOPA_RBO_EQ:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  xor t0, t0, t1\n");
        fprintf(fp,"  seqz t0, t0\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_SUB:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  sub t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_ADD:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  add t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_MUL:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  mul t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_MOD:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  rem t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_DIV:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  div t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_GE:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  slt t0, t0, t1\n");
        fprintf(fp,"  seqz t0, t0\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_LE:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  sgt t0, t0, t1\n");
        fprintf(fp,"  seqz t0, t0\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_GT:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  sgt t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_LT:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  slt t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_NOT_EQ:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  xor t0, t0, t1\n");
        fprintf(fp,"  seqz t0, t0\n");
        fprintf(fp,"  seqz t0, t0\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_AND:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  and t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      case KOOPA_RBO_OR:
        distill(l,fp,0);
        distill(r,fp,1);
        fprintf(fp,"  or t0, t0, t1\n");
        fprintf(fp,"  li t1, %d\n",max_size-now_size);
        fprintf(fp,"  add t1, sp, t1\n");
        fprintf(fp,"  sw t0, 0(t1)\n");
        break;
      default:
        break;
    }
}

void Visit(const koopa_raw_jump_t &jump,FILE* fp){
    fprintf(fp, "  j %s\n",&jump.target->name[1]);
}

void Visit(const koopa_raw_branch_t &branch,FILE* fp){
    distill(branch.cond,fp,0);
    fprintf(fp,"  bnez t0, jmpboard_%d\n",wxm);
    fprintf(fp,"  j %s\n",&branch.false_bb->name[1]);
    fprintf(fp,"jmpboard_%d:\n",wxm++);
    fprintf(fp,"  j %s\n",&branch.true_bb->name[1]);
}
