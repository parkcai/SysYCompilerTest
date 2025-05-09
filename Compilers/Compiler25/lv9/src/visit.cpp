#include "visit.h"
#include <unordered_map>
#include <set>

int val_stack_cnt = 0;
int val_stack_idx = 0;
bool has_call = false;
int func_arg_idx = 0;
std::unordered_map<koopa_raw_value_t, int> val_stack;
std::set<koopa_raw_value_t> is_ptr;

extern std::string riscv_code;

bool idx_imm(int idx){
  return idx<2048&&idx>=-2048;
}

std::string li(std::string reg, int imm){
  return "\tli " + reg + ", " + std::to_string(imm)+"\n";
}
std::string sub(std::string reg, std::string reg1, std::string reg2){
  return "\tsub " + reg + ", " + reg1 + ", " + reg2 + "\n";
}
std::string sw(std::string reg, int imm, std::string reg2){
  if(idx_imm(imm))
    return "\tsw " + reg + ", " + std::to_string(imm) + "("+reg2+")\n";
  else{
    return "\tli t3, "+ std::to_string(imm)+"\n"
          +"\tadd t3, t3, "+reg2+"\n"
          +"\tsw "+reg+", 0(t3)\n";
  }
}
std::string lw(std::string reg, int imm, std::string reg2){
  if(idx_imm(imm))
    return "\tlw " + reg + ", " + std::to_string(imm) + "("+reg2+")\n";
  else{
    // return "\tli t0, "+std::to_string(imm)+"\n"+"\tlw "+reg+", t0("+reg2+")\n";
    return "\tli t3, "+ std::to_string(imm)+"\n"
          +"\tadd t3, t3, "+reg2+"\n"
          +"\tlw "+reg+", 0(t3)\n";
  }
}
std::string la(std::string reg, std::string label){
  return "\tla " + reg + ", " + label + "\n";
}
std::string addi(std::string reg, std::string reg1, int imm){
  if(idx_imm(imm))
    return "\taddi " + reg + ", " + reg1 + ", " + std::to_string(imm) + "\n";
  else{
    return "\tli t3, "+std::to_string(imm)+"\n"+"\tadd " + reg + ", " + reg1 + ", t3\n";
  }
}
std::string add(std::string reg, std::string reg1, std::string reg2){
  return "\tadd " + reg + ", " + reg1 + ", " + reg2 + "\n";
}
std::string xor_(std::string reg, std::string reg1, std::string reg2){
  return "\txor " + reg + ", " + reg1 + ", " + reg2 + "\n";
}
std::string seqz(std::string reg, std::string reg1){
  return "\tseqz " + reg + ", " + reg1 + "\n";
}
std::string mul(std::string reg, std::string reg1, std::string reg2){
  return "\tmul " + reg + ", " + reg1 + ", " + reg2 + "\n";
}
std::string div(std::string reg, std::string reg1, std::string reg2){
  return "\tdiv " + reg + ", " + reg1 + ", " + reg2 + "\n";
}
std::string snez(std::string reg, std::string reg1){
  return "\tsnez "+reg+", "+reg1+"\n";
}
std::string rem(std::string reg, std::string reg1, std::string reg2){
  return "\trem "+reg+", "+reg1+", "+reg2+"\n";
}
std::string slt(std::string reg, std::string reg1, std::string reg2){
  return "\tslt "+reg+", "+reg1+", "+reg2+"\n";
}
std::string sgt(std::string reg, std::string reg1, std::string reg2){
  return "\tsgt "+reg+", "+reg1+", "+reg2+"\n";
}

size_t getTypeSize(koopa_raw_type_t ty){
  switch(ty->tag){
    case KOOPA_RTT_INT32:
      return 4;
    case KOOPA_RTT_ARRAY:
      return ty->data.array.len * getTypeSize(ty->data.array.base);
    case KOOPA_RTT_POINTER:
      return 4;
    default:
      return 0;
  }
  return 0;
}

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
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

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  if(func->bbs.len ==0){
    return ;
  }
  val_stack_idx = 0;
  val_stack_cnt = 0;
  has_call = false;
  func_arg_idx = 0;
  int args_more_than_8 = 0;
  for(size_t i=0;i<func->bbs.len;i++){
    auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    for(size_t j=0;j<bb->insts.len;j++){
      auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
      if(inst->kind.tag == KOOPA_RVT_BINARY||inst->kind.tag == KOOPA_RVT_LOAD||inst->kind.tag == KOOPA_RVT_BRANCH||inst->kind.tag == KOOPA_RVT_GET_ELEM_PTR||inst->kind.tag == KOOPA_RVT_GET_PTR){
        val_stack_cnt += 4;
      }
      else if(inst->kind.tag == KOOPA_RVT_CALL){
        has_call = true;
        if(inst->kind.data.call.callee->ty->data.function.ret->tag == KOOPA_RTT_INT32)
          val_stack_cnt += 4;
        if(int(inst->kind.data.call.args.len-8-args_more_than_8) > 0){
            args_more_than_8 = inst->kind.data.call.args.len-8;
        }
      }
      else if(inst->kind.tag == KOOPA_RVT_ALLOC){
        val_stack_cnt += getTypeSize(inst->ty->data.pointer.base);
      }
    }
  }
  if(has_call){
    val_stack_cnt += 4;
  }
  val_stack_cnt += 4*args_more_than_8;
  val_stack_idx += 4*args_more_than_8;
  val_stack.clear();
  if(func->bbs.len > 0){
    riscv_code += "\t.text\n";
    riscv_code += "\t.globl " + std::string(func->name + 1) + "\n";
    riscv_code += std::string(func->name + 1) + ":\n";
    riscv_code += addi("sp", "sp", -val_stack_cnt);
    if(has_call){
      riscv_code += sw("ra", val_stack_cnt-4,"sp");
    }
  }
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  if(std::string(bb->name+1) != "entry")
    riscv_code += std::string(bb->name+1)+":\n";
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
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
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_ALLOC:
      if(val_stack.find(value)==val_stack.end()){
        val_stack[value] = val_stack_idx;
        val_stack_idx+=getTypeSize(value->ty->data.pointer.base);
        if(value->ty->data.pointer.base->tag==KOOPA_RTT_POINTER){
          is_ptr.insert(value);
        }
      }
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store, value);
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr, value);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr, value);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

void Visit(const koopa_raw_return_t& ret) {
  if (ret.value != nullptr) {
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
      riscv_code += li("a0", ret.value->kind.data.integer.value);
    }
    else{
      riscv_code += lw("a0", val_stack[ret.value], "sp");
    }
  }
  if(has_call){
    riscv_code += lw("ra", val_stack_cnt-4, "sp");
  }
  riscv_code += addi("sp", "sp", val_stack_cnt);
  riscv_code += "\tret\n\n";
}

void Visit(const koopa_raw_integer_t& integer) {
  std::cout << integer.value;
}

void Visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value){
  koopa_raw_value_t l = binary.lhs, r = binary.rhs;
  if(l->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += li("t0", l->kind.data.integer.value);
  }
  else{
    riscv_code += lw("t0", val_stack[l], "sp");
  }
  if(r->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += li("t1", r->kind.data.integer.value);
  }
  else{
    riscv_code += lw("t1", val_stack[r], "sp");
  }
  switch(binary.op){
    case KOOPA_RBO_ADD:
      riscv_code += add("t0", "t0", "t1");
      break;
    case KOOPA_RBO_SUB:
      riscv_code += sub("t0", "t0", "t1");
      break;
    case KOOPA_RBO_EQ:
      riscv_code += xor_("t0", "t0", "t1");
      riscv_code += seqz("t0", "t0");
      break;
    case KOOPA_RBO_NOT_EQ:
      riscv_code += xor_("t0", "t0", "t1");
      riscv_code += snez("t0", "t0");
      break;
    case KOOPA_RBO_MUL:
      riscv_code += mul("t0", "t0", "t1");
      break;
    case KOOPA_RBO_DIV:
      riscv_code += div("t0", "t0", "t1");
      break;
    case KOOPA_RBO_MOD:
      riscv_code += rem("t0", "t0", "t1");
      break;
    case KOOPA_RBO_LT:
      riscv_code += slt("t0", "t0", "t1");
      break;
    case KOOPA_RBO_GT:
      riscv_code += slt("t0", "t1", "t0");
      break;
    case KOOPA_RBO_LE:
      riscv_code += sgt("t0", "t0", "t1");
      riscv_code += seqz("t0", "t0");
      break;
    case KOOPA_RBO_GE:
      riscv_code += slt("t0", "t0", "t1");
      riscv_code += seqz("t0", "t0");
      break;
    default:
      assert(false);
      break;
  }
  val_stack[value] = val_stack_idx;
  riscv_code += sw("t0", val_stack_idx,"sp");
  val_stack_idx += 4;
}

void Visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value){
  if(load.src->kind.tag == KOOPA_RVT_ALLOC){
    riscv_code += lw("t0", val_stack[load.src],"sp");
  }
  else if(load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    riscv_code += la("t0", load.src->name+1);
    riscv_code += lw("t0", 0, "t0");
  }
  else if(load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR||load.src->kind.tag == KOOPA_RVT_GET_PTR){
    riscv_code += lw("t0", val_stack[load.src],"sp");
    riscv_code += lw("t0", 0, "t0");
  }
  else{
    assert(false);
  }

  if(val_stack.find(value)!=val_stack.end()){
    riscv_code += sw("t0", val_stack[value],"sp");
  }
  else{
    val_stack[value] = val_stack_idx;
    riscv_code += sw("t0", val_stack_idx,"sp");
    val_stack_idx += 4;
  }
}

void Visit(const koopa_raw_store_t& store, const koopa_raw_value_t& value){
  if(val_stack.find(store.dest)==val_stack.end()&&store.dest->kind.tag != KOOPA_RVT_GLOBAL_ALLOC){
    assert(false);
  }

  if(store.value->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += li("t0", store.value->kind.data.integer.value);
  }
  else if(store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
  }
  else{
    riscv_code += lw("t0", val_stack[store.value], "sp");
  }

  if(store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
    if(func_arg_idx < 8){
      riscv_code += sw("a"+std::to_string(func_arg_idx), val_stack[store.dest],"sp");
    }
    else{
      riscv_code += lw("t0", val_stack_cnt+(func_arg_idx-8)*4, "sp");
      riscv_code += sw("t0", val_stack[store.dest],"sp");
    }
    func_arg_idx++;
  }
  else if(store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    riscv_code += la("t1", store.dest->name+1);
    riscv_code += sw("t0", 0, "t1");
  }
  else if(store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR||store.dest->kind.tag == KOOPA_RVT_GET_PTR){
    riscv_code += lw("t1", val_stack[store.dest],"sp");
    riscv_code += sw("t0", 0, "t1");
  }
  else{
    riscv_code += sw("t0", val_stack[store.dest],"sp");
  }
}

void Visit(const koopa_raw_jump_t& jump){
  riscv_code += "\tj "+std::string(jump.target->name+1)+"\n";
}

void Visit(const koopa_raw_branch_t& branch){
  if(branch.cond->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += li("t0", branch.cond->kind.data.integer.value);
  }
  else{
    riscv_code += lw("t0", val_stack[branch.cond], "sp");
  }
  riscv_code += "\tbnez t0, "+std::string(branch.true_bb->name+1)+"\n";
  riscv_code += "\tj "+std::string(branch.false_bb->name+1)+"\n";
}

void Visit(const koopa_raw_call_t& call, const koopa_raw_value_t& value){
  int a_reg_cnt = 0;
  int args_stack_idx = 0;
  for(size_t i=0;i<call.args.len;i++){
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if(i<8){
      if(arg->kind.tag == KOOPA_RVT_INTEGER){
        riscv_code += li("a"+std::to_string(a_reg_cnt), arg->kind.data.integer.value);
      }
      else{
        riscv_code += lw("a"+std::to_string(a_reg_cnt), val_stack[arg],"sp");
      }
      a_reg_cnt++;
    }
    else{
      if(arg->kind.tag == KOOPA_RVT_INTEGER){
        riscv_code += li("t0", arg->kind.data.integer.value);
        riscv_code += sw("t0", args_stack_idx,"sp");
      }
      else{
        riscv_code += lw("t0", val_stack[arg],"sp");
        riscv_code += sw("t0", args_stack_idx,"sp");
      }
      args_stack_idx += 4;
    }
  }
  riscv_code += "\tcall "+std::string(call.callee->name+1)+"\n";
  if(call.callee->ty->data.function.ret->tag == KOOPA_RTT_INT32){
    val_stack[value] = val_stack_idx;
    riscv_code += sw("a0", val_stack_idx,"sp");
    val_stack_idx += 4;
  }
}

bool all_zero(const koopa_raw_value_t& value){
  if(value->kind.tag == KOOPA_RVT_INTEGER){
    if(value->kind.data.integer.value==0)
      return true;
    else
      return false;

  }
  else{
    auto elem = value->kind.data.aggregate.elems;
    for(int i=0;i<elem.len;i++){
      if(!all_zero(reinterpret_cast<koopa_raw_value_t>(elem.buffer[i]))){
        return false;
      }
    }
    return true;
  }
}

void init_global_array(const koopa_raw_value_t& value){
  if(all_zero(value)){
    riscv_code += "\t.zero " + std::to_string(getTypeSize(value->ty))+'\n';
    return;
  }
  if(value->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += "\t.word "+std::to_string(value->kind.data.integer.value)+"\n";
  }
  else{
    auto elem = value->kind.data.aggregate.elems;
    for(int i=0;i<elem.len;i++)
      init_global_array(reinterpret_cast<koopa_raw_value_t>(elem.buffer[i]));
  }
}
void Visit(const koopa_raw_global_alloc_t& global_alloc, const koopa_raw_value_t& value){
  riscv_code += "\t.data\n";
  riscv_code += "\t.global "+std::string(value->name+1)+"\n";
  riscv_code += std::string(value->name+1)+":\n";
  if(global_alloc.init->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += "\t.word "+std::to_string(global_alloc.init->kind.data.integer.value)+"\n";
  }
  else if(global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE){
    init_global_array(global_alloc.init);
  }
  else if(global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
    riscv_code += "\t.zero " + std::to_string(getTypeSize(global_alloc.init->ty))+'\n';
  }
  riscv_code += "\n";
}

void Visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr, const koopa_raw_value_t& value){
  if(get_elem_ptr.src->kind.tag == KOOPA_RVT_ALLOC)
    riscv_code += addi("t0", "sp", val_stack[get_elem_ptr.src]);
  else if(get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR||get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_PTR){
    riscv_code += lw("t0", val_stack[get_elem_ptr.src],"sp");
  }
  else if(get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    riscv_code += la("t0", get_elem_ptr.src->name+1);
  }

  if(get_elem_ptr.index->kind.tag == KOOPA_RVT_INTEGER)
    riscv_code += li("t1", get_elem_ptr.index->kind.data.integer.value);
  else{
    riscv_code += lw("t1", val_stack[get_elem_ptr.index],"sp");
  }
  riscv_code += li("t2", getTypeSize(get_elem_ptr.src->ty->data.pointer.base->data.array.base));
  riscv_code += mul("t1", "t1", "t2");
  riscv_code += add("t0", "t0", "t1");
  if(val_stack.find(value)==val_stack.end()){
    val_stack[value] = val_stack_idx;
    val_stack_idx += 4;
  }
  riscv_code += sw("t0", val_stack[value],"sp");
}

void Visit(const koopa_raw_get_ptr_t& get_ptr, const koopa_raw_value_t& value){
  if(get_ptr.src->kind.tag == KOOPA_RVT_ALLOC){
    riscv_code += addi("t0", "sp", val_stack[get_ptr.src]);
  }
  else if(get_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    riscv_code += la("t0", get_ptr.src->name+1);
  }
  else if(get_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR||get_ptr.src->kind.tag == KOOPA_RVT_GET_PTR){
    riscv_code += lw("t0", val_stack[get_ptr.src],"sp");
  }
  else{
    riscv_code += lw("t0", val_stack[get_ptr.src],"sp");
  }

  if(get_ptr.index->kind.tag == KOOPA_RVT_INTEGER){
    riscv_code += li("t1", get_ptr.index->kind.data.integer.value);
  }
  else{
    riscv_code += lw("t1", val_stack[get_ptr.index],"sp");
  }
  riscv_code += li("t2", getTypeSize(get_ptr.src->ty->data.pointer.base));
  riscv_code += mul("t1", "t1", "t2");
  riscv_code += add("t0", "t0", "t1");
  if(val_stack.find(value)==val_stack.end()){
    val_stack[value] = val_stack_idx;
    val_stack_idx += 4;
  }
  riscv_code += sw("t0", val_stack[value],"sp");
}