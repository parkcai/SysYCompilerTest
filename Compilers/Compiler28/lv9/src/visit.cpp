#include "visit.h"

int cnt = 0;
int cur_size = 0;
int funcSize = 0;
bool func_call = false;
std::map<koopa_raw_value_t, int> addr_map;

void Visit(const koopa_raw_program_t &program, std::ostream &cout) {
  cout<<"\t.data\n";
  Visit(program.values, cout);
  Visit(program.funcs, cout);
}

void Visit(const koopa_raw_slice_t &slice, std::ostream &cout) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr), cout);
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), cout);
        break;
      case KOOPA_RSIK_VALUE:
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr), cout);
        break;
      default:
        assert(false);
    }
  }
}

//访问函数
void Visit(const koopa_raw_function_t &func, std::ostream &cout) {
  if(func->bbs.len == 0) {
    return;
  }
  func_call = false;
  funcSize = func_size(func, func_call);
  cout<<"\n\t.text\n";
  cout<<"\t.global "<<remove_prefix("@", func->name)<<"\n";
  cout<<remove_prefix("@", func->name)<<":\n";
  if(funcSize != 0 && funcSize >= -2048 && funcSize < 2048)
    cout<<"\taddi  sp, sp, -"<<funcSize<<"\n";
  else if(funcSize != 0)
  {
    cout<<"\tli    t0, -"<<funcSize<<"\n";
    cout<<"\tadd   sp, sp, t0\n";
  }
  if(func_call) {
    store_to_stack("ra", funcSize-4, cout);
  }
  // 访问所有基本块
  cur_size = 0;
  if(func->params.len > 8)
  {
    cur_size = 4*(func->params.len-8);
  }
  Visit(func->bbs, cout);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb, std::ostream &cout) {
  
  if(bb->name != nullptr && std::string(bb->name) != "%entry") {
    cout<<remove_prefix("%", bb->name)<<":\n";
  }
  
  // 访问所有指令
  Visit(bb->insts, cout);
}

int value_cnt = 0;

// 访问指令
void Visit(const koopa_raw_value_t &value, std::ostream &cout) {
  // 根据指令类型判断后续需要如何访问
  int dest_addr = -1;
  if(value->ty->tag != KOOPA_RTT_UNIT)
    dest_addr = get_addr(value);
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret, cout);
      value_cnt += 1;
      if(value_cnt == 7) {
        value_cnt = 1;
      }
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer, cout);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, dest_addr, cout);
      value_cnt += 1;
      if(value_cnt == 7) {
        value_cnt = 1;
      }
      break;
    case KOOPA_RVT_ALLOC:
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store, cout);
      value_cnt += 1;
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, dest_addr, cout);
      break;
    case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      Visit(kind.data.branch, cout);
      break;
    case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      Visit(kind.data.jump, cout);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, dest_addr, cout);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      global_var(value, cout);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr, dest_addr, cout);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr, dest_addr, cout);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

void global_var(const koopa_raw_value_t &value, std::ostream &cout) {
  cout<<"\t.global "<<remove_prefix("@", value->name)<<"\n";
  cout<<remove_prefix("@", value->name)<<":\n";
  if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    cout<<"\t.word "<<value->kind.data.global_alloc.init->kind.data.integer.value<<"\n";
  }
  else if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
  {
    cout<<"\t.zero "<<type_size(value->ty->data.pointer.base)<<"\n";
  }
  else if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE)
  {
    Visit(value->kind.data.global_alloc.init->kind.data.aggregate, cout);
    //aggregate_init(value->kind.data.global_alloc.init, cout);
  }
}

//aggregate指令
void Visit(const koopa_raw_aggregate_t &aggregate, std::ostream &cout) {
  for (size_t i = 0; i < aggregate.elems.len; ++i) {
    auto ptr = reinterpret_cast<koopa_raw_value_t>(aggregate.elems.buffer[i]);
    if(ptr->kind.tag == KOOPA_RVT_INTEGER) {
      cout<<"\t.word "<<ptr->kind.data.integer.value<<"\n";
    }
    else if(ptr->kind.tag == KOOPA_RVT_AGGREGATE)
    {
      Visit(ptr->kind.data.aggregate, cout);
    }
    else
    {
      assert(false);
    }
  }
}

void aggregate_init(const koopa_raw_value_t &value, std::ostream &cout)
{
    if (value->kind.tag == KOOPA_RVT_INTEGER)
    {
        cout<<"  .word " + std::to_string(value->kind.data.integer.value) + "\n";
    }
    else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
    {
        const auto &agg = value->kind.data.aggregate;
        for (int i = 0; i < agg.elems.len; i++)
        {
          aggregate_init(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]), cout);
        }
    }
}

// get element pointer
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, int dest_addr, std::ostream &cout) {
  // 访问 get element pointer 指令
  if(get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    cout<<"\tla    t0, "<<remove_prefix("@", get_elem_ptr.src->name)<<"\n";
  }
  else {
    int src_addr = get_addr(get_elem_ptr.src);
    assert(src_addr != -1);
    if(src_addr >= -2048 && src_addr < 2048)
      cout<<"\taddi  t0, sp, "<<src_addr<<"\n";
    else
    {
      cout<<"\tli    t1, "<<src_addr<<"\n";
      cout<<"\tadd   t0, sp, t1\n";
    }

    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_PTR)
      cout<<"\tlw    t0, 0(t0)\n";
    
  }

  if(get_elem_ptr.index->kind.tag == KOOPA_RVT_INTEGER) {
    cout<<"\tli    t1, "<<get_elem_ptr.index->kind.data.integer.value<<"\n";
  }
  else {
    load_from_stack("t1", get_addr(get_elem_ptr.index), cout);
  }
  int size = array_size(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
  cout<<"\tli    t2, "<<size<<"\n";
  cout<<"\tmul   t1, t1, t2\n";
  cout<<"\tadd   t0, t0, t1\n";
  store_to_stack("t0", dest_addr, cout);

}

// get pointer
void Visit(const koopa_raw_get_ptr_t &get_ptr, int dest_addr, std::ostream &cout) {
  // 访问 get pointer 指令
  int src_addr = get_addr(get_ptr.src);
  assert(src_addr != -1);
  if(src_addr >= -2048 && src_addr < 2048)
    cout<<"\taddi  t0, sp, "<<src_addr<<"\n";
  else
  {
    cout<<"\tli    t1, "<<src_addr<<"\n";
    cout<<"\tadd   t0, sp, t1\n";
  }

  cout<<"\tlw    t0, 0(t0)\n";

  if(get_ptr.index->kind.tag == KOOPA_RVT_INTEGER) {
    cout<<"\tli    t1, "<<get_ptr.index->kind.data.integer.value<<"\n";
  }
  else {
    load_from_stack("t1", get_addr(get_ptr.index), cout);
  }

  int size = array_size(get_ptr.src->ty->data.pointer.base);
  cout<<"\tli    t2, "<<size<<"\n";
  cout<<"\tmul   t1, t1, t2\n";
  cout<<"\tadd   t0, t0, t1\n";
  store_to_stack("t0", dest_addr, cout);

}

// 访问函数调用指令
void Visit(const koopa_raw_call_t &call, int dest_addr, std::ostream &cout) {
  // 访问函数调用指令

  // 保存前8个参数
  for (size_t i = 0; i < call.args.len && i < 8; ++i) {
    auto ptr = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if(ptr->kind.tag == KOOPA_RVT_INTEGER) {
      cout<<"\tli    a"<<i<<", "<<ptr->kind.data.integer.value<<"\n";
    }
    else {
      load_from_stack("a"+std::to_string(i), get_addr(ptr), cout);
    }
  }
  // 保存剩余参数
  bool callee_call = false;
  int callee_size = func_size(call.callee, callee_call);
  for(int i = 8; i < call.args.len; ++i) {
    auto ptr = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if(ptr->kind.tag == KOOPA_RVT_INTEGER) {
      cout<<"\tli    t0, "<<ptr->kind.data.integer.value<<"\n";
    }
    else {
      load_from_stack("t0", get_addr(ptr), cout);
    }
    store_to_stack("t0", (i-8)*4 - callee_size, cout);
  }

  cout<<"\tcall  "<<remove_prefix("@", call.callee->name)<<"\n";
  if(dest_addr != -1) {
    store_to_stack("a0", dest_addr, cout);
  }
}

// 访问跳转指令
void Visit(const koopa_raw_jump_t &jump, std::ostream &cout) {
  // 访问跳转指令
  cout<<"\tj     "<<remove_prefix("%", jump.target->name)<<"\n";
}

// 访问分支指令
void Visit(const koopa_raw_branch_t &branch, std::ostream &cout) {
  // 访问分支指令
  if(branch.cond->kind.tag == KOOPA_RVT_INTEGER) {
    cout<<"\tli    t0, "<<branch.cond->kind.data.integer.value<<"\n";
  }
  else {
    load_from_stack("t0", get_addr(branch.cond), cout);
  }
  cout<<"\tbnez  t0, "<<remove_prefix("%", branch.true_bb->name)<<"_trampoline\n";
  cout<<"\tj     "<<remove_prefix("%", branch.false_bb->name)<<"\n";
  cout<<remove_prefix("%", branch.true_bb->name)<<"_trampoline:\n";
  cout<<"\tj     "<<remove_prefix("%", branch.true_bb->name)<<"\n";
}

// 访问返回指令
void Visit(const koopa_raw_return_t &ret, std::ostream &cout) {
  if(ret.value != nullptr) {
    if(ret.value->kind.tag == KOOPA_RVT_INTEGER) {
      cout<<"\tli    a0, "<<ret.value->kind.data.integer.value<<"\n";
    }
    else {
      load_from_stack("a0", get_addr(ret.value), cout);
    }
  }
  if(func_call) {
    load_from_stack("ra", funcSize-4, cout);
  }
  if(funcSize != 0 && funcSize >= -2048 && funcSize < 2048)
    cout<<"\taddi  sp, sp, "<<funcSize<<"\n";
  else if(funcSize != 0)
  {
    cout<<"\tli    t0, "<<funcSize<<"\n";
    cout<<"\tadd   sp, sp, t0\n";
  }
  cout<<"\tret\n";
}

//Store指令
void Visit(const koopa_raw_store_t &store, std::ostream &cout) {
  // 加载参数
  if(store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF)
  {
    if(store.value->kind.data.func_arg_ref.index < 8) {
      store_to_stack("a"+std::to_string(store.value->kind.data.func_arg_ref.index), get_addr(store.dest), cout);
    }
    else {
      load_from_stack("t0", (store.value->kind.data.func_arg_ref.index - 8) * 4, cout);
      store_to_stack("t0", get_addr(store.dest), cout);
    }
  }
  else//非参数
  {
    //加载source
    if(store.value->kind.tag == KOOPA_RVT_INTEGER) {
      cout<<"\tli    t0"<<", "<<store.value->kind.data.integer.value<<"\n";
    }
    else if(store.value->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
      return;
    }
    else {
      load_from_stack("t0", get_addr(store.value), cout);
    }

    //加载dest
    if(store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
      cout<<"\tla    t1, "<<remove_prefix("@", store.dest->name)<<"\n";
      cout<<"\tsw    t0, 0(t1)\n";
    }
    else if(store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || store.dest->kind.tag == KOOPA_RVT_GET_PTR) {
      load_from_stack("t1", get_addr(store.dest), cout);
      cout<<"\tsw    t0, 0(t1)\n";
    }
    else
      store_to_stack("t0", get_addr(store.dest), cout);
  }
}

//Load指令
void Visit(const koopa_raw_load_t &load, int dest_addr, std::ostream &cout) {
  // 访问加载指令
  if(load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    cout<<"\tla    t0, "<<remove_prefix("@", load.src->name)<<"\n";
    cout<<"\tlw    t0, 0(t0)\n";
  }
  else if(load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || load.src->kind.tag == KOOPA_RVT_GET_PTR) {
    load_from_stack("t0", get_addr(load.src), cout);
    cout<<"\tlw    t0, 0(t0)\n";
  }
  else
    load_from_stack("t0", get_addr(load.src), cout);
  store_to_stack("t0", dest_addr, cout);
}

// 访问整数常量
void Visit(const koopa_raw_integer_t &integer, std::ostream &cout) {
  cout<<integer.value<<"\n";
}

void Visit(const koopa_raw_binary_t &binary_op, int dest_addr, std::ostream &cout) {
  // 访问操作符
  std::string lreg, rreg;
  std::string dest_reg = "t" + std::string(1, value_cnt+'0');

  if(binary_op.lhs->kind.tag == KOOPA_RVT_INTEGER) {
    cout<<"\tli    t0, "<<binary_op.lhs->kind.data.integer.value<<"\n";
  }
  else {
    load_from_stack("t0", get_addr(binary_op.lhs), cout);
  }

  if(binary_op.rhs->kind.tag == KOOPA_RVT_INTEGER) {
    cout<<"\tli    t1, "<<binary_op.rhs->kind.data.integer.value<<"\n";
  }
  else {
    load_from_stack("t1", get_addr(binary_op.rhs), cout);
  }

  switch (binary_op.op) 
  {
    case KOOPA_RBO_EQ:
      cout<<"\txor   t0, t0, t1\n";
      cout<<"\tseqz  t0, t0\n";
      break;
    case KOOPA_RBO_NOT_EQ:
      cout<<"\txor   t0, t0, t1\n";
      cout<<"\tsnez  t0, t0\n";
      break;
    case KOOPA_RBO_ADD:
      cout<<"\tadd   t0, t0, t1\n";
      break;
    case KOOPA_RBO_SUB:
      cout<<"\tsub   t0, t0, t1\n";
      break;
    case KOOPA_RBO_MUL:
      cout<<"\tmul   t0, t0, t1\n";
      break;
    case KOOPA_RBO_DIV:
      cout<<"\tdiv   t0, t0, t1\n";
      break;
    case KOOPA_RBO_MOD:
      cout<<"\trem   t0, t0, t1\n";
      break;
    case KOOPA_RBO_LT:
      cout<<"\tslt   t0, t0, t1\n";
      break;
    case KOOPA_RBO_LE:
      cout<<"\tsgt   t0, t0, t1\n";
      cout<<"\tseqz  t0, t0\n";
      break;
    case KOOPA_RBO_GT:
      cout<<"\tsgt   t0, t0, t1\n";
      break;
    case KOOPA_RBO_GE:
      cout<<"\tslt   t0, t0, t1\n";
      cout<<"\tseqz  t0, t0\n";
      break;
    case KOOPA_RBO_AND:
      cout<<"\tand   t0, t0, t1\n";
      break;
    case KOOPA_RBO_OR:
      cout<<"\tor    t0, t0, t1\n";
      break;
    default:
      assert(false);
  }
  store_to_stack("t0", dest_addr, cout);
}

void load(const koopa_raw_value_t &value, std::string r, std::ostream &cout) {
  cout<<"\tli    "<<r<<", ";
  Visit(value, cout);
}

int func_size(const koopa_raw_function_t &func, bool &call) {
  int size = 0;
  int arg_count = 0;
  for (size_t i = 0; i < func->bbs.len; ++i) {
    auto ptr = func->bbs.buffer[i];
    size += bb_size(reinterpret_cast<koopa_raw_basic_block_t>(ptr), call, arg_count);
  }
  if(call) {
    size += 4;
  }
  if(arg_count > 8) {
    size += 4 * (arg_count - 8);
  }
  if(func->params.len > 8) {
    size += 4 * (func->params.len - 8);
  }
  return round_up(size, 16);
}

int bb_size(const koopa_raw_basic_block_t &bb, bool &call, int &arg_count) {
  int size = 0;
  for (size_t i = 0; i < bb->insts.len; ++i) {
    auto ptr = bb->insts.buffer[i];
    size += inst_size(reinterpret_cast<koopa_raw_value_t>(ptr), call, arg_count);
  }
  return size;
}

int inst_size(const koopa_raw_value_t &inst, bool &call, int &arg_count) {
  if(inst->kind.tag == KOOPA_RVT_CALL) {
    call = true;
    if(arg_count < inst->kind.data.call.args.len) {
      arg_count = inst->kind.data.call.args.len;
    }
  }
  if(inst->kind.tag == KOOPA_RVT_ALLOC)
    return type_size(inst->ty->data.pointer.base);
  return type_size(inst->ty);
}

int type_size(const koopa_raw_type_t &ty) {
  switch(ty->tag) {
    case KOOPA_RTT_INT32:
      return 4;
    case KOOPA_RTT_UNIT:
      return 0;
    case KOOPA_RTT_POINTER:
      return 4;
    case KOOPA_RTT_ARRAY:
      return array_size(ty);
    default:
      assert(false);
  }
}

int array_size(const koopa_raw_type_t &ty) {
  int size = 4;
  auto ptr = ty;
  while(ptr->tag == KOOPA_RTT_ARRAY) {
    size *= ptr->data.array.len;
    ptr = ptr->data.array.base;
  }
  return size;
}

int get_addr(const koopa_raw_value_t &value) {
  if (addr_map.find(value) == addr_map.end()) {
    int t;
    if(value->kind.tag == KOOPA_RVT_ALLOC)
      t = type_size(value->ty->data.pointer.base);
    else
      t = type_size(value->ty);
    addr_map[value] = cur_size;
    cur_size += t;
  }
  return addr_map[value];
}

void store_to_stack(std::string reg, int addr, std::ostream &cout) {
  if(addr >= -2048 && addr < 2048)
    cout<<"\tsw    "<<reg<<", "<<addr<<"(sp)\n";
  else {
    cout<<"\tli    t3, "<<addr<<"\n";
    cout<<"\tadd   t3, t3, sp\n";
    cout<<"\tsw    "<<reg<<", (t3)\n";
  }
}

void load_from_stack(std::string reg, int addr, std::ostream &cout) {
  if(addr >= -2048 && addr < 2048)
    cout<<"\tlw    "<<reg<<", "<<addr<<"(sp)\n";
  else {
    cout<<"\tli    t3, "<<addr<<"\n";
    cout<<"\tadd   t3, t3, sp\n";
    cout<<"\tlw    "<<reg<<", (t3)\n";
  }
}