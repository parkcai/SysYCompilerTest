#include "koopa_visit.hpp"

void KoopaFunctionStack::addMapping(koopa_raw_value_t value) {
  if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    printf("Warning: func arg ref\n");
    assert(false);
  }
  else {
    value_to_stack_map[value] = stack_size;
    stack_size += getAllocSize(value->ty);
    //printf("stack_size: %d\n", stack_size);
  }
}

void KoopaFunctionStack::addFunctionMapping(koopa_raw_value_t value, int num) {
  if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    if (num < 8) {
      value_to_stack_map[value] = "a" + std::to_string(num);
    }
    else {
      // since the argument is either integer or pointer, we can use 4 bytes to store it
      value_to_stack_map[value] = stack_size + 4 * (num - 8);
    }
  }
  else {
    printf("Warning: unknown value type: %d\n", value->kind.tag);
    assert(false);
  }
}

StackValue KoopaFunctionStack::getStackValue(koopa_raw_value_t value) const {
  auto it = value_to_stack_map.find(value);
  if (it == value_to_stack_map.end()) {
    printf("value not found\n");
    assert(false);
    return -1;
  }
  return it->second;
}

void KoopaFunctionStack::stackSizeRoundUp() {
  stack_size = (stack_size + 15) / 16 * 16;
}

void KoopaFunctionStack::setStackSize(int size) {
  if (has_ra) return;
  stack_size = size;
  if (size == 4) {
    has_ra = true;
  }
}

void KoopaFunctionStack::addStackSize(int size) {
  stack_size += size;
}

int KoopaFunctionStack::getStackSize() const {
  return stack_size;
}

bool KoopaFunctionStack::hasRA() const {
  return has_ra;
}

void KoopaFunctionStack::setRA(bool ra) {
  has_ra = ra;
}

void KoopaFunctionStack::setRetAddrOffset() {
  assert(has_ra);
  ret_addr_offset = stack_size;
  stack_size += 4;
}

int KoopaFunctionStack::getRetAddrOffset() const {
  return ret_addr_offset;
}

std::string KoopaVisitor::getStackAddressReg(koopa_raw_value_t value) {
  if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::string tmp_reg = getTmpReg("t6");
    std::string global_var = value->name;
    if (!global_var.empty() && global_var[0] == '@') {
      global_var.erase(0, 1);
    }
    risv_code += "\tla " + tmp_reg + ", " + global_var + "\n";
    return "0(" + tmp_reg + ")";
  }
  StackValue stack_value = function_stack->getStackValue(value);
  if (std::holds_alternative<int>(stack_value)) {
    int offset = std::get<int>(stack_value);
    return getStackAddressReg(offset);
  }
  else {
    return std::get<std::string>(stack_value);
  }
}

std::string KoopaVisitor::getStackAddressReg(int offset) {
  if (offset >= -2048 && offset <= 2047) {
     return std::to_string(offset) + "(sp)";
  }
  else {
    std::string reg = getTmpReg("t6");
    risv_code += "\tli " + reg + ", " + std::to_string(offset) + "\n";
    risv_code += "\tadd " + reg + ", sp, " + reg + "\n";
    return "0(" + reg + ")";
  }
}

std::string KoopaVisitor::getPtrStackAddressReg(koopa_raw_value_t value) {
  StackValue stack_value = function_stack->getStackValue(value);
  if (std::holds_alternative<int>(stack_value)) {
    int offset = std::get<int>(stack_value);
    std::string reg = getTmpReg("t6");
    if (offset >= -2048 && offset <= 2047) {
      risv_code += "\tlw " + reg + ", " + std::to_string(offset) + "(sp)\n";
      return "0(" + reg + ")";
    }
    else {
      risv_code += "\tli " + reg + ", " + std::to_string(offset) + "\n";
      risv_code += "\tadd " + reg + ", sp, " + reg + "\n";
      risv_code += "\tlw " + reg + ", 0(" + reg + ")\n";
      return "0(" + reg + ")";
    }
  }
  else {
    return std::get<std::string>(stack_value);
  }
}

bool KoopaVisitor::isIntPtr(koopa_raw_value_t value) {
  if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR || value->kind.tag == KOOPA_RVT_GET_PTR) {
    if (value->ty->tag == KOOPA_RTT_POINTER && value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
      return true;
    }
  }
  return false;
}

void KoopaVisitor::RegSetInit() {
  for (int i = 0; i < 7; ++i) {
    reg_set.insert("t" + std::to_string(i));
  }
  for (int i = 0; i < 8; ++i) {
    reg_set.insert("a" + std::to_string(i));
  }
}

std::string KoopaVisitor::getNextReg() {
  // if (current_id <= 6) {
  //   return "t" + std::to_string(current_id++);
  // }
  // else if (current_id > 6 && current_id <= 14) {
  //   return "a" + std::to_string(current_id++ - 7);
  // }
  // else {
  //   return "register not enough";
  // }
  if (reg_set.empty()) {
    //assert(false);
    return "";
  }
  std::string next_reg = *reg_set.begin();
  reg_set.erase(next_reg);
  return next_reg;
}

std::string KoopaVisitor::getTmpReg(std::string reg) {
  if (reg != "") {
    if (reg_set.find(reg) == reg_set.end()) {
      printf("reg %s not found in tmpReg\n", reg.c_str());
      assert(false);
      return "";
    }
    return reg;
  }
  if (reg_set.empty()) {
    printf("no register in tmpReg\n");
    assert(false);
    return "";
  }
  std::string tmp_reg = *reg_set.begin();
  return tmp_reg;
}

std::string KoopaVisitor::getLastReg() {
  if (reg_stack.empty()) {
    return "no register";
  }
  std::string last_reg = reg_stack.top();
  reg_stack.pop();
  return last_reg;
}

std::string KoopaVisitor::getRetReg(std::string reg1, std::string reg2) {
  if (reg1 != "x0") {
    return reg1;
  }
  else if (reg2 != "x0") {
    return reg2;
  }
  else {
    return getNextReg();
  }
}

void KoopaVisitor::RegCollect(std::string reg) {
  if (reg != "x0") {
    // insert reg into the beginning of the set
    reg_set.insert(reg);
  }
}

void KoopaVisitor::RegPreparation(const koopa_raw_value_t &value) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    Visit(value);
  }
  else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    std::string reg = getStackAddressReg(value);
    if (reg[0] == 'a') {
      reg_stack.push(reg);
    }
    else {
      std::string st_reg = getNextReg();
      risv_code += "\tlw " + st_reg + ", " + reg + "\n";
      reg_stack.push(st_reg);
    }
  }
  else {
    std::string reg = getNextReg();
    std::string stack_reg = getStackAddressReg(value);
    risv_code += "\tlw " + reg + ", " + stack_reg + "\n";
    reg_stack.push(reg);
  }
}

void KoopaVisitor::FuncArgRegPreparation(const koopa_raw_value_t &value) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    if (value->kind.data.integer.value == 0) {
      reg_stack.push("x0");
    }
    else {
      std::string reg = getTmpReg("t6");
      risv_code += "\tli " + reg + ", " + std::to_string(value->kind.data.integer.value) + "\n";
      reg_stack.push(reg);
    }
  }
  else {
    std::string reg = getTmpReg("t6");
    std::string stack_reg = getStackAddressReg(value);
    risv_code += "\tlw " + reg + ", " + stack_reg + "\n";
    reg_stack.push(reg);
  }
}

std::string KoopaVisitor::Convert(const koopa_raw_program_t &program) {
  RegSetInit();
  Visit(program);
  return risv_code;
}

void KoopaVisitor::Visit(const koopa_raw_program_t &program) {
  // TODO
  Visit(program.values);
  Visit(program.funcs);
}

void KoopaVisitor::Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION: 
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        printf("unknown slice kind: %d\n", slice.kind);
        assert(false);
    }
  }
}

void KoopaVisitor::Visit(const koopa_raw_function_t &func) {
  int bbs_len = func->bbs.len;
  if (bbs_len == 0) {
    return;
  }
  std::string func_name = func->name;
  if (!func_name.empty() && func_name[0] == '@') {
    func_name.erase(0, 1);
  }
  std::stringstream ss;
  ss << "\t.text\n";
  ss << "\t.globl " << func_name << "\n";
  ss << func_name << ":\n";
  risv_code += ss.str();
  FunctionPrologue(func);
  Visit(func->bbs);
  delete function_stack;
  risv_code += "\n";
}

void KoopaVisitor::Visit(const koopa_raw_basic_block_t &bb) {
  std::string block_name = std::string(bb->name);
  block_name.erase(0, 1);
  if (block_name != "entry") {
    risv_code += block_name + ":\n";
  }
  Visit(bb->insts);
}

void KoopaVisitor::Visit(const koopa_raw_value_t &value) {
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      VisitReturn(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      VisitInteger(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      VisitBinary(kind.data.binary);
      break; 
    case KOOPA_RVT_ALLOC:
      // do nothing
      break;
    case KOOPA_RVT_STORE:
      VisitStore(kind.data.store);
      break;
    case KOOPA_RVT_LOAD:
      VisitLoad(kind.data.load);
      break;
    case KOOPA_RVT_BRANCH:
      VisitBranch(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      VisitJump(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      VisitCall(kind.data.call);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC: {
      std::string global_var = value->name;
      if (!global_var.empty() && global_var[0] == '@') {
        global_var.erase(0, 1);
      }
      risv_code += "\t.data\n";
      risv_code += "\t.globl " + global_var + "\n";
      risv_code += global_var + ":\n";
      VisitGlobalAlloc(kind.data.global_alloc);
      risv_code += "\n";
      break;
    }
    case KOOPA_RVT_GET_PTR:
      VisitGetPtr(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      VisitGetElemPtr(kind.data.get_elem_ptr, value);
      break;
    default:
      printf("kind.tag: %d\n", kind.tag);
      //assert(false);
  }

  if (value->kind.tag == KOOPA_RVT_LOAD || value->kind.tag == KOOPA_RVT_BINARY) {
    std::string reg = getLastReg();
    std::string dest_reg = getStackAddressReg(value);
    risv_code += "\tsw " + reg + ", " + dest_reg + "\n";
    RegCollect(reg);
  }
  else if (value->kind.tag == KOOPA_RVT_CALL && value->ty->tag != KOOPA_RTT_UNIT) {
    std::string reg = getLastReg();
    std::string dest_reg = getStackAddressReg(value);
    risv_code += "\tsw " + reg + ", " + dest_reg + "\n";
    RegCollect(reg);
  }
}

void KoopaVisitor::VisitReturn(const koopa_raw_return_t &ret) {
  if (ret.value != NULL) {
    RegPreparation(ret.value);
    std::string reg = getLastReg();
    if (reg != "a0") {
      risv_code += "\tmv a0, " + reg + "\n";
      RegCollect(reg);
      reg_set.erase("a0");
    }
  }
  FunctionEpilogue(NULL);
  if (ret.value != NULL) {
    RegCollect("a0");
  }
  risv_code += "\tret\n";
}

void KoopaVisitor::VisitInteger(const koopa_raw_integer_t &integer) {
  if (integer.value == 0) {
    reg_stack.push("x0");
    return;
  }
  std::string reg = getNextReg();
  risv_code += "\tli " + reg + ", " + std::to_string(integer.value) + "\n";
  reg_stack.push(reg);
}

void KoopaVisitor::VisitBinary(const koopa_raw_binary_t &binary) {
  RegPreparation(binary.lhs);
  std::string reg1 = getLastReg();
  RegPreparation(binary.rhs);
  std::string reg2 = getLastReg();
  std::string ret_reg = getRetReg(reg1, reg2);
  switch (binary.op) {
    case KOOPA_RBO_SUB:
      risv_code += "\tsub " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_EQ:
      risv_code += "\txor " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      risv_code += "\tseqz " + ret_reg + ", " + ret_reg + "\n";
      break;
    case KOOPA_RBO_NOT_EQ:
      risv_code += "\txor " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      risv_code += "\tsnez " + ret_reg + ", " + ret_reg + "\n";
      break;
    case KOOPA_RBO_AND:
      risv_code += "\tand " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_OR:
      risv_code += "\tor " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_ADD:
      risv_code += "\tadd " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_MUL:
      risv_code += "\tmul " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_DIV:
      risv_code += "\tdiv " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_MOD:
      risv_code += "\trem " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_GT:
      risv_code += "\tsgt " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_LT:
      risv_code += "\tslt " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      break;
    case KOOPA_RBO_GE:
      risv_code += "\tslt " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      risv_code += "\tseqz " + ret_reg + ", " + ret_reg + "\n";
      break;
    case KOOPA_RBO_LE:
      risv_code += "\tsgt " + ret_reg + ", " + reg1 + ", " + reg2 + "\n";
      risv_code += "\tseqz " + ret_reg + ", " + ret_reg + "\n";
      break;
    default:
      printf("unknown binary op: %d\n", binary.op);
      assert(false);
  }
  reg_stack.push(ret_reg);
  if (ret_reg != reg1) {
    RegCollect(reg1);
  }
  if (ret_reg != reg2) {
    RegCollect(reg2);
  }
}

void KoopaVisitor::VisitStore(const koopa_raw_store_t &store) {
  RegPreparation(store.value);
  std::string src_reg = getLastReg(), dest_reg;
  bool is_int_ptr = isIntPtr(store.dest);
  if (is_int_ptr) {
    dest_reg = getPtrStackAddressReg(store.dest);
  }
  else {
    dest_reg = getStackAddressReg(store.dest);
  }
  risv_code += "\tsw " + src_reg + ", " + dest_reg + "\n";
  RegCollect(src_reg);
}

void KoopaVisitor::VisitLoad(const koopa_raw_load_t &load) {
  std::string reg = getNextReg();
  std::string src_reg;
  bool is_int_ptr = isIntPtr(load.src);
  if (is_int_ptr) {
    src_reg = getPtrStackAddressReg(load.src);
  }
  else {
    src_reg = getStackAddressReg(load.src);
  }
  risv_code += "\tlw " + reg + ", " + src_reg + "\n";
  reg_stack.push(reg);
}

void KoopaVisitor::VisitBranch(const koopa_raw_branch_t &branch) {
  RegPreparation(branch.cond);
  std::string reg = getLastReg();
  std::string true_bb = branch.true_bb->name;
  true_bb.erase(0, 1);
  std::string false_bb = branch.false_bb->name;
  false_bb.erase(0, 1);
  risv_code += "\tbeqz " + reg + ", " + false_bb + "\n";
  risv_code += "\tj " + true_bb + "\n";
  RegCollect(reg);
  //Visit(branch.true_bb);
  //Visit(branch.false_bb);
}

void KoopaVisitor::VisitJump(const koopa_raw_jump_t &jump) {
  std::string target_bb = jump.target->name;
  target_bb.erase(0, 1);
  risv_code += "\tj " + target_bb + "\n";
}

void KoopaVisitor::VisitCall(const koopa_raw_call_t &call) {
  // we should first modify sp value first
  int arg_num_stack_size = (call.args.len - 8) * 4;
  if (arg_num_stack_size < 0) {
    arg_num_stack_size = 0;
  }
  int reg_stack_size = reg_stack.size() * 4;
  int stack_size = arg_num_stack_size + reg_stack_size;
  stack_size = (stack_size + 15) / 16 * 16;
  // int total_stack_size = function_stack->getStackSize() + stack_size;
  // save all the reg in reg_stack
  std::vector<std::string> reg_vec;
  for (size_t i = 0; i < reg_stack.size(); ++i) {
    std::string reg = reg_stack.top();
    reg_stack.pop();
    reg_vec.push_back(reg);
    int offset = -4 * i;
    std::string stack_reg = getStackAddressReg(offset);
    risv_code += "\tsw " + reg + ", " + stack_reg + "\n";
  }
  // save the arguments
  for (size_t i = 0; i < call.args.len; ++i) {
    auto ptr = call.args.buffer[i];
    auto arg = reinterpret_cast<koopa_raw_value_t>(ptr);
    FuncArgRegPreparation(arg);
    std::string reg = getLastReg();
    if (i < 8) {
      risv_code += "\tmv a" + std::to_string(i) + ", " + reg + "\n";
    }
    else {
      int offset = - (stack_size - 4 * (i - 8));
      std::string stack_reg = getStackAddressReg(offset);
      risv_code += "\tsw " + reg + ", " + stack_reg + "\n";
    }
  }
  stack_size = -stack_size;
  if (stack_size >= -2048 && stack_size <= 2047) {
    risv_code += "\taddi sp, sp, " + std::to_string(stack_size) + "\n";
  }
  else {
    std::string tmp_reg = getTmpReg();
    risv_code += "\tli " + tmp_reg + ", " + std::to_string(-stack_size) + "\n";
    risv_code += "\tadd sp, sp, " + tmp_reg + "\n";
  }
  // call the function
  std::string func_name = call.callee->name;
  if (!func_name.empty() && func_name[0] == '@') {
    func_name.erase(0, 1);
  }
  risv_code += "\tcall " + func_name + "\n";
  // ret_value
  std::string ret_reg = "";
  if (call.callee->ty->data.function.ret->tag == KOOPA_RTT_INT32) {
    ret_reg = getNextReg();
    risv_code += "\tmv " + ret_reg + ", a0\n";
  }
  // restore sp value
  stack_size = -stack_size;
  if (stack_size >= -2048 && stack_size <= 2047) {
    risv_code += "\taddi sp, sp, " + std::to_string(stack_size) + "\n";
  }
  else {
    std::string tmp_reg = getTmpReg();
    risv_code += "\tli " + tmp_reg + ", " + std::to_string(stack_size) + "\n";
    risv_code += "\tadd sp, sp, " + tmp_reg + "\n";
  }
  // load all the reg in reg_stack
  for (size_t i = 0; i < reg_vec.size(); ++i) {
    std::string reg = reg_vec[i];
    int offset = -4 * i;
    std::string stack_reg = getStackAddressReg(offset);
    risv_code += "\tlw " + reg + ", " + stack_reg + "\n";
    reg_stack.push(reg);
  }
  if (ret_reg != "") {
    reg_stack.push(ret_reg);
  }
}

void KoopaVisitor::VisitGlobalAlloc(const koopa_raw_global_alloc_t &alloc) {
  // TODO
  if (alloc.init != NULL) {
    if (alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
      risv_code += "\t.word " + std::to_string(alloc.init->kind.data.integer.value) + "\n";
    }
    else if (alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
      VisitAggregate(alloc.init);
    }
    else if (alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
      auto base = alloc.init->ty;
      if (base->tag == KOOPA_RTT_INT32) {
        risv_code += "\t.zero 4\n";
      }
      else if (base->tag == KOOPA_RTT_ARRAY) {
        int size = 4;
        while(base->tag == KOOPA_RTT_ARRAY)
        {
          size *= base->data.array.len;
          base = base->data.array.base;
        }
        risv_code += "\t.zero " + std::to_string(size) + "\n";
      }
    }
  }
  else {
    printf("Warning: global alloc init is NULL\n");
    assert(false);
  }
}

void KoopaVisitor::VisitAggregate(const koopa_raw_value_t &value) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    risv_code += "\t.word " + std::to_string(value->kind.data.integer.value) + "\n";
  }
  else if (value->kind.tag == KOOPA_RVT_AGGREGATE) {
    const auto &agg = value->kind.data.aggregate;
    for (int i = 0; i < agg.elems.len; ++i) {
      auto ptr = agg.elems.buffer[i];
      auto elem = reinterpret_cast<koopa_raw_value_t>(ptr);
      VisitAggregate(elem);
    }
  }
}

void KoopaVisitor::VisitGetPtr(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
  // lw t6 0(t0) ->sw t6 x(sp)
  std::string src_reg = getNextReg(), index_reg, size_reg, dst_reg;
  if (get_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::string global_var = get_ptr.src->name;
    if (!global_var.empty() && global_var[0] == '@') {
      global_var.erase(0, 1);
    }
    risv_code += "\tla " + src_reg + ", " + global_var + "\n";
  }
  else if (get_ptr.src->kind.tag == KOOPA_RVT_ALLOC) {
    StackValue stack_value = function_stack->getStackValue(get_ptr.src);
    if (std::holds_alternative<int>(stack_value)) {
      int offset = std::get<int>(stack_value);
      if (offset >= -2048 && offset <= 2047) {
        risv_code += "\taddi " + src_reg + ", sp, " + std::to_string(offset) + "\n";
      }
      else {
        risv_code += "\tli " + src_reg + ", " + std::to_string(offset) + "\n";
        risv_code += "\tadd " + src_reg + ", sp, " + src_reg + "\n";
      }
    }
    else {
      //src_reg = std::get<std::string>(stack_value);
      printf("Warning: get_ptr src is not int\n");
      assert(false);
    }
  }
  else {
    StackValue stack_value = function_stack->getStackValue(get_ptr.src);
    if (std::holds_alternative<int>(stack_value)) {
      int offset = std::get<int>(stack_value);
      if (offset >= -2048 && offset <= 2047) {
        risv_code += "\tlw " + src_reg + ", " + std::to_string(offset) + "(sp)\n";
      }
      else {
        risv_code += "\tli " + src_reg + ", " + std::to_string(offset) + "\n";
        risv_code += "\tadd " + src_reg + ", sp, " + src_reg + "\n";
        risv_code += "\tlw " + src_reg + ", 0(" + src_reg + ")\n";
      }
    }
  }
  RegPreparation(get_ptr.index);
  index_reg = getLastReg();
  size_reg = getTmpReg("t6");
  int size = getPtrSize(get_ptr.src->ty);
  risv_code += "\tli " + size_reg + ", " + std::to_string(size) + "\n";
  risv_code += "\tmul " + index_reg + ", " + index_reg + ", " + size_reg + "\n";
  risv_code += "\tadd " + src_reg + ", " + src_reg + ", " + index_reg + "\n";
  // for KOOPA_RTT_INT32, we should load the value from the addr
  // checkType(value->ty);
  // if (value->ty->tag == KOOPA_RTT_POINTER && value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
  //   risv_code += "\tlw " + src_reg + ", 0(" + src_reg + ")\n";
  // }
  dst_reg = getStackAddressReg(value);
  risv_code += "\tsw " + src_reg + ", " + dst_reg + "\n";
  RegCollect(src_reg);
  RegCollect(index_reg);
}

void KoopaVisitor::VisitGetElemPtr(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
  // lw t6 0(t0) ->sw t6 x(sp)
  std::string src_reg = getNextReg(), index_reg, size_reg, dst_reg;
  if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::string global_var = get_elem_ptr.src->name;
    if (!global_var.empty() && global_var[0] == '@') {
      global_var.erase(0, 1);
    }
    risv_code += "\tla " + src_reg + ", " + global_var + "\n";
  }
  else if (get_elem_ptr.src->kind.tag == KOOPA_RVT_ALLOC) {
    StackValue stack_value = function_stack->getStackValue(get_elem_ptr.src);
    if (std::holds_alternative<int>(stack_value)) {
      int offset = std::get<int>(stack_value);
      if (offset >= -2048 && offset <= 2047) {
        risv_code += "\taddi " + src_reg + ", sp, " + std::to_string(offset) + "\n";
      }
      else {
        risv_code += "\tli " + src_reg + ", " + std::to_string(offset) + "\n";
        risv_code += "\tadd " + src_reg + ", sp, " + src_reg + "\n";
      }
    }
    else {
      //src_reg = std::get<std::string>(stack_value);
      printf("Warning: get_elem_ptr src is not int\n");
      assert(false);
    }
  }
  else {
    StackValue stack_value = function_stack->getStackValue(get_elem_ptr.src);
    if (std::holds_alternative<int>(stack_value)) {
      int offset = std::get<int>(stack_value);
      if (offset >= -2048 && offset <= 2047) {
        risv_code += "\tlw " + src_reg + ", " + std::to_string(offset) + "(sp)\n";
      }
      else {
        risv_code += "\tli " + src_reg + ", " + std::to_string(offset) + "\n";
        risv_code += "\tadd " + src_reg + ", sp, " + src_reg + "\n";
        risv_code += "\tlw " + src_reg + ", 0(" + src_reg + ")\n";
      }
    }
  }
  RegPreparation(get_elem_ptr.index);
  index_reg = getLastReg();
  size_reg = getTmpReg("t6");
  int size = getPtrEleSize(get_elem_ptr.src->ty);
  risv_code += "\tli " + size_reg + ", " + std::to_string(size) + "\n";
  risv_code += "\tmul " + index_reg + ", " + index_reg + ", " + size_reg + "\n";
  risv_code += "\tadd " + src_reg + ", " + src_reg + ", " + index_reg + "\n";
  // for KOOPA_RTT_INT32, we should load the value from the addr
  // checkType(value->ty);
  // if (value->ty->tag == KOOPA_RTT_POINTER && value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
  //   risv_code += "\tlw " + src_reg + ", 0(" + src_reg + ")\n";
  // }
  dst_reg = getStackAddressReg(value);
  risv_code += "\tsw " + src_reg + ", " + dst_reg + "\n";
  RegCollect(src_reg);
  RegCollect(index_reg);
}

void KoopaVisitor::FunctionPrologue(const koopa_raw_function_t &func) {
  function_stack = new KoopaFunctionStack();
  koopa_raw_slice_t bbs = func->bbs;
  std::string ra_save_code = "";
  for (size_t i = 0; i < bbs.len; ++i) {
    auto ptr = bbs.buffer[i];
    auto bb = reinterpret_cast<koopa_raw_basic_block_t>(ptr);
    koopa_raw_slice_t insts = bb->insts;
    for (size_t j = 0; j < insts.len; ++j) {
      auto ptr = insts.buffer[j];
      auto inst = reinterpret_cast<koopa_raw_value_t>(ptr);
      if (inst->kind.tag == KOOPA_RVT_ALLOC) {
        function_stack->addMapping(inst);
      }
      else if (inst->kind.tag == KOOPA_RVT_CALL) {
        function_stack->setRA(true);
        // koopa_raw_call_t func_call = inst->kind.data.call;
        // int arg_num = func_call.args.len;
        // But maybe we shoud do it in call function?
        if (inst->ty->tag != KOOPA_RTT_UNIT) {
          function_stack->addMapping(inst);
        }
      }
      else if (inst->ty->tag != KOOPA_RTT_UNIT) {
        function_stack->addMapping(inst);
      }
    }
  }
  if (function_stack->hasRA()) {
    function_stack->setRetAddrOffset();
  }
  // round up to 16 bytes
  function_stack->stackSizeRoundUp();

  // Note here we decrease the stack pointer
  int stack_size = -function_stack->getStackSize();
  if (stack_size >= -2048 && stack_size <= 2047) {
    risv_code += "\taddi sp, sp, " + std::to_string(stack_size) + "\n";
  }
  else {
    // We don't want to use a0-a7 here, for they are used to pass arguments
    std::string tmp_reg = getTmpReg("t6");
    risv_code += "\tli " + tmp_reg + ", " + std::to_string(stack_size) + "\n";
    risv_code += "\tadd sp, sp, " + tmp_reg + "\n";
  }

  bool has_ra = function_stack->hasRA();
  if (has_ra) {
    int offset = function_stack->getRetAddrOffset();
    std::string ra_reg = getStackAddressReg(offset);
    risv_code += "\tsw ra, " + ra_reg + "\n";
  }

  koopa_raw_slice_t params = func->params;
  for (size_t i = 0; i < params.len; ++i) {
    auto ptr = params.buffer[i];
    auto param = reinterpret_cast<koopa_raw_value_t>(ptr);
    function_stack->addFunctionMapping(param, i);
  }
}

void KoopaVisitor::FunctionEpilogue(const koopa_raw_function_t &func) {
  int stack_size = function_stack->getStackSize();
  bool has_ra = function_stack->hasRA();
  if (has_ra) {
    int offset = function_stack->getRetAddrOffset();
    std::string ra_reg = getStackAddressReg(offset);
    risv_code += "\tlw ra, " + ra_reg + "\n";
  }
  if (stack_size >= -2048 && stack_size <= 2047) {
    risv_code += "\taddi sp, sp, " + std::to_string(stack_size) + "\n";
  }
  else {
    std::string tmp_reg = getTmpReg();
    risv_code += "\tli " + tmp_reg + ", " + std::to_string(stack_size) + "\n";
    risv_code += "\tadd sp, sp, " + tmp_reg + "\n";
  }
}

int getAllocSize(koopa_raw_type_t type) {
  if (type->tag == KOOPA_RTT_POINTER) {
    type = type->data.pointer.base;
  }
  //printf("getAllocSize tag: %d\n", type->tag);

  if (type->tag == KOOPA_RTT_ARRAY) {
    int num = type->data.array.len;
    return num * getAllocSize(type->data.array.base);
  }
  else if (type->tag == KOOPA_RTT_POINTER) {
    return 4;
  }
  else if (type->tag == KOOPA_RTT_INT32) {
    return 4;
  }
  else {
    assert(false);
  }
}

int getPtrEleSize(koopa_raw_type_t type) {
  if (type->tag == KOOPA_RTT_POINTER) {
    type = type->data.pointer.base;
  }
  if (type->tag == KOOPA_RTT_ARRAY) {
    type = type->data.array.base;
  }
  
  return getAllocSize(type);
}

int getPtrSize(koopa_raw_type_t type) {
  if (type->tag == KOOPA_RTT_POINTER) {
    type = type->data.pointer.base;
  }
  
  return getAllocSize(type);
}

void checkType(koopa_raw_type_t type) {
  int layer = 0;
  while (type->tag != KOOPA_RTT_INT32) {
    printf("layer: %d, tag: %d\n", layer, type->tag);
    if (type->tag == KOOPA_RTT_POINTER) {
      type = type->data.pointer.base;
    }
    else if (type->tag == KOOPA_RTT_ARRAY) {
      type = type->data.array.base;
    }
    else {
      break;
    }
    ++layer;
  }
  printf("layer: %d, tag: %d\n", layer, type->tag);
}