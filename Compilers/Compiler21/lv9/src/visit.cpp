#include "visit.hpp"
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

bool a[8] = { true, true, true, true, true, true, true, true };
bool t[7] = { true, true, true, true, true, true, true };
// value 与寄存器/?(sp)之间的映射
std::unordered_map<koopa_raw_value_t, std::string> ValueToReg;
std::unordered_map<koopa_raw_value_t, std::vector<int>> ValueArrayDim;
int stack_cnt = 0;
int stack_length = 0;
int ifHasCall = 0;
int num_global = 0;
std::string findReg(){
  for (int i = 0; i < 7; i++){
    if(t[i]){
      t[i] = false;
      return "t" + std::to_string(i);
    }
  }
  for (int i = 0; i < 8; i++){
    if(a[i]){
      a[i] = false;
      return "a" + std::to_string(i);
    }
  }
  assert(0);
}
void invalidReg(std::string reg){
  if(reg[0] == 't')
    t[reg[1] - '0'] = true;
  else if(reg[0] == 'a')
    a[reg[1] - '0'] = true;
}
void giveReg(const koopa_raw_value_t &value){
  // std::cout << "tag=" << value->kind.tag <<"  value->"<< std::endl;
  if(value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value != 0){
    std::string reg = findReg();
    std::cout << "  li " << reg << ", ";
    Visit(value->kind.data.integer);
    std::cout << "\n";
    ValueToReg[value] = reg;
  }
  else if(value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value == 0){
    ValueToReg[value] = "x0";
  }
  else if(value->kind.tag == KOOPA_RVT_ALLOC || value->kind.tag == KOOPA_RVT_LOAD || value->kind.tag == KOOPA_RVT_BINARY || value->kind.tag == KOOPA_RVT_CALL){
    std::string reg = findReg();
    // 判断偏移量
    // std::cout << "  lw " << reg << ", " << ValueToReg[value] << std::endl;
    lw_func(reg, ValueToReg[value]);
    ValueToReg[value] = reg;
  }
  else if(value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
    size_t index = value->kind.data.func_arg_ref.index;
    if(index > 7){
      std::string reg = findReg();
      lw_func(reg, ValueToReg[value]);
      ValueToReg[value] = "t0";
    }
  }
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
    // std::cout << "slice.buffer" << i << std::endl;
    // std::cout << slice.kind << std::endl;
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
  if(func->bbs.len == 0)
    return;
  std::cout << "  .text\n";
  std::cout << "  .globl " << func->name + 1 << std::endl;
  std::cout << func->name + 1 << ":" << std::endl;

  stack_cnt = 0;
  int alloc_cnt = 0;
  int func_params = 0;
  ifHasCall = 0;
  for (size_t i = 0; i < func->bbs.len; i++){
    const koopa_raw_slice_t &insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    for (size_t j = 0; j < insts.len; j++){
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if(inst->ty->tag != KOOPA_RTT_UNIT){
        alloc_cnt++;
      }
      if(inst->kind.tag == KOOPA_RVT_CALL){
        ifHasCall = 1;
        int now = max(int(inst->kind.data.call.args.len) - 8, 0);
        func_params = max(func_params, now);
      }
      if(inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY){
        int num = 1;
        auto base = inst->ty->data.pointer.base;
        while(base->tag == KOOPA_RTT_ARRAY){
          ValueArrayDim[inst].push_back(base->data.array.len);
          num *= base->data.array.len;
          base = base->data.array.base;
        }
        alloc_cnt += num;
      }
    }
  }
  stack_length = (alloc_cnt + ifHasCall + func_params) * 4;
  // 函数的参数在低地址
  stack_cnt = func_params;
  // 向上取整到16
  if(stack_length % 16 != 0){
    stack_length = ((stack_length / 16) + 1) * 16;
  }
  if(-stack_length < -2047 || -stack_length > 2047){
    std::string reg = findReg();
    std::cout << "  li " << reg << ", " << -stack_length << std::endl;
    std::cout << "  add sp, sp, " << reg << std::endl;
    invalidReg(reg);
  }
  else{
    std::cout << "  addi sp, sp, " << -stack_length << std::endl;
  }

  if(ifHasCall){
    sw_func("ra", std::to_string(stack_length - 4) + "(sp)");
  }

  // 函数参数
  for (size_t i = 0; i < func->params.len; i++){
    auto param = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);
    if(i < 8){
      ValueToReg[param] = "a" + std::to_string(i);
    }
    else{
      ValueToReg[param] = std::to_string(stack_length + (i - 8) * 4) + "(sp)";
    }
  }

    // 访问所有基本块
    Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 输出标签
  if(strcmp(bb->name + 1, "entry")){
    std::cout << bb->name + 1 << ":\n";
  }
  // 访问所有指令
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
      // alloc指令
      Visit(1, value);
      break;
    case KOOPA_RVT_STORE:
      // store指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_LOAD:
      // load指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_BRANCH:
      // branch指令
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // jump指令
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // call指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr, value);
      break;
    default:
      // 其他类型暂时遇不到
      // std::cout << kind.tag << std::endl;
      assert(false);
  }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...
void Visit(const koopa_raw_return_t &ret){
  if(ret.value != nullptr){
    if(ret.value->kind.tag == KOOPA_RVT_INTEGER){
      std::cout << "  li a0, ";
      Visit(ret.value->kind.data.integer);
      std::cout << "\n";
    }
    else if(ret.value->kind.tag == KOOPA_RVT_BINARY || ret.value->kind.tag == KOOPA_RVT_LOAD || ret.value->kind.tag == KOOPA_RVT_CALL){
      // std::cout << "  lw a0, " << ValueToReg[ret.value] << std::endl;
      lw_func("a0", ValueToReg[ret.value]);
    }
  }

  if(ifHasCall){
    lw_func("ra", std::to_string(stack_length - 4) + "(sp)");
  }

  if(-stack_length < -2047 || -stack_length > 2047){
    std::cout << "  li t0, " << stack_length << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }
  else{
    std::cout << "  addi sp, sp, " << stack_length << std::endl;
  }
  std::cout << "  ret\n\n";
}

void sw_func(std::string reg, std::string biassp){
  std::string bias_str = biassp.substr(0, biassp.length() - 4);
  int bias = std::stoi(bias_str);
  std::string regBias = findReg();
  if(bias < -2048 || bias > 2047){
    std::cout << "  li " << regBias << ", " << bias << std::endl;
    std::cout << "  add " << regBias << ", " << regBias << ", sp" << std::endl;
    std::cout << "  sw " << reg << ", " << "0(" << regBias << ")\n";
  }
  else{
    std::cout << "  sw " << reg << ", " << biassp << std::endl;
  }
  invalidReg(regBias);
}

void lw_func(std::string dest, std::string src){
  // src: bias(sp)
  std::string bias_str = src.substr(0, src.length() - 4);
  int bias = std::stoi(bias_str);
  std::string regBias = findReg();
  if(bias < -2048 | bias > 2047){
    std::cout << "  li " << regBias << ", " << bias << std::endl;
    std::cout << "  add " << regBias << ", " << regBias << ", sp" << std::endl;
    std::cout << "  lw " << dest << ", " << "0(" << regBias << ")\n";
  }
  else{
    std::cout << "  lw " << dest << ", " << src << std::endl;
  }
  invalidReg(regBias);
}

void Visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value){
  std::string reg = findReg();
  // std::cout << "  lw " << reg << ", " << ValueToReg[load.src] << std::endl;

  if(load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    std::cout << "  la " << reg << ", " << ValueToReg[load.src] << std::endl;
    std::cout << "  lw " << reg << ", 0(" << reg << ")\n";
  }
  else if(load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || load.src->kind.tag == KOOPA_RVT_GET_PTR){
    lw_func(reg, ValueToReg[load.src]);
    std::cout << "  lw " << reg << ", 0(" << reg << ")\n";
  }
  else{
    lw_func(reg, ValueToReg[load.src]);
  }
  // 先不写了，报错了再说（x
  // if(stack_cnt * 4 < -2048 || stack_cnt * 4 > 2047){
  //   // assert(0);
  //   // 真报错了，写吧。。。
  // }
  // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
  if(ValueArrayDim.find(load.src) != ValueArrayDim.end()){
    std::vector<int> vec;
    vec = ValueArrayDim[load.src];
    ValueArrayDim[value] = vec;
  }
  sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
  invalidReg(reg);
  ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
  stack_cnt++;
}

void Visit(const koopa_raw_store_t& store){
  giveReg(store.value);
  if(ValueToReg.find(store.dest) == ValueToReg.end()){
    ValueToReg[store.dest] = std::to_string(stack_cnt * 4) + "(sp)";
    stack_cnt++;
  }

  if(store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    std::string reg = findReg();
    std::cout << "  la " << reg << ", " << ValueToReg[store.dest] << std::endl;
    std::cout << "  sw " << ValueToReg[store.value] << ", 0(" << reg << ")\n";
    invalidReg(reg);
  }
  else if(store.dest->kind.tag == KOOPA_RVT_GET_PTR || store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR){
    std::string reg = findReg();
    lw_func(reg, ValueToReg[store.dest]);
    std::cout << "  sw " << ValueToReg[store.value] << ", 0(" << reg << ")\n";
    invalidReg(reg);
  }
  else{
    // std::cout << "  sw " << ValueToReg[store.value] << ", " << ValueToReg[store.dest] << std::endl;
    sw_func(ValueToReg[store.value], ValueToReg[store.dest]);
  }
  invalidReg(ValueToReg[store.value]);
}

void Visit(const koopa_raw_integer_t &integer){
    std::cout << integer.value;
}

void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value){
  switch(binary.op){
    /// Not equal to.
    case KOOPA_RBO_NOT_EQ: {
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      // neq: != 0 ; eq: == 0
      std::cout << "  xor " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      std::cout << "  snez " << reg << ", " << reg << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Equal to.
    case KOOPA_RBO_EQ:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      // neq: != 0 ; eq: == 0
      std::cout << "  xor " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      std::cout << "  seqz " << reg << ", " << reg << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Greater than.
    case KOOPA_RBO_GT:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  sgt " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Less than.
    case KOOPA_RBO_LT:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  slt " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Greater than or equal to.
    case KOOPA_RBO_GE:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  slt " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      std::cout << "  seqz " << reg << ", " << reg << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Less than or equal to.
    case KOOPA_RBO_LE:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  sgt " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      std::cout << "  seqz " << reg << ", " << reg << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Addition.
    case KOOPA_RBO_ADD:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  add " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Subtraction.
    case KOOPA_RBO_SUB:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  sub " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Multiplication.
    case KOOPA_RBO_MUL:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  mul " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Division.
    case KOOPA_RBO_DIV:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  div " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Modulo.
    case KOOPA_RBO_MOD:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  rem " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Bitwise AND.
    case KOOPA_RBO_AND:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  and " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    /// Bitwise OR.
    case KOOPA_RBO_OR:{
      giveReg(binary.lhs);
      giveReg(binary.rhs);
      std::string reg = findReg();
      std::cout << "  or " << reg << ", " << ValueToReg[binary.lhs] << ", " << ValueToReg[binary.rhs] << std::endl;
      invalidReg(ValueToReg[binary.lhs]);
      invalidReg(ValueToReg[binary.rhs]);
      // std::cout << "  sw " << reg << ", " << stack_cnt * 4 << "(sp)\n";
      sw_func(reg, std::to_string(stack_cnt * 4) + "(sp)");
      ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
      invalidReg(reg);
      stack_cnt++;
      break;
    }
    default:
      break;
  }
}

void Visit(const koopa_raw_branch_t &branch){
  giveReg(branch.cond);
  std::cout << "  bnez " << ValueToReg[branch.cond] << ", " << branch.true_bb->name + 1 << "_yyxtmp" << std::endl;
  std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
  std::cout << branch.true_bb->name + 1 << "_yyxtmp:\n";
  std::cout << "  j " << branch.true_bb->name + 1 << std::endl;
  invalidReg(ValueToReg[branch.cond]);
}

void Visit(const koopa_raw_jump_t &jump){
  std::cout << "  j " << jump.target->name + 1 << std::endl;
}

void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value){
  // 传参
  int params = int(call.args.len);
  for (int i = 0; i < min(8, params); i++){
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if(arg->kind.tag == KOOPA_RVT_INTEGER){
      std::cout << "  li a" << i << ", " << arg->kind.data.integer.value << std::endl;
    }
    else{
      lw_func("a" + std::to_string(i), ValueToReg[arg]);
      // std::cout << "  lw a" << i << ", " << ValueToReg[arg] << std::endl;
    }
  }
  for (int i = 8; i < params; i++){
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    std::string reg = findReg();
    if(arg->kind.tag == KOOPA_RVT_INTEGER){
      std::cout << "  li " << reg << ", " << arg->kind.data.integer.value << std::endl;
      sw_func(reg, std::to_string((i - 8) * 4) + "(sp)");
    }
    else{
      lw_func(reg, ValueToReg[arg]);
      sw_func(reg, std::to_string((i - 8) * 4) + "(sp)");
    }
    invalidReg(reg);
  }
  // call
  std::cout << "  call " << call.callee->name + 1 << std::endl;
  if(value->ty->tag != KOOPA_RTT_UNIT){
    sw_func("a0", std::to_string(stack_cnt * 4) + "(sp)");
    ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
    stack_cnt++;
  }
}
// 全局数组初始化
void printAgg(const koopa_raw_value_t &value){
  if(value->kind.tag == KOOPA_RVT_INTEGER){
    std::cout << "  .word " << value->kind.data.integer.value << std::endl;
  }
  else if(value->kind.tag == KOOPA_RVT_AGGREGATE){
    auto agg = value->kind.data.aggregate;
    for (int i = 0; i < agg.elems.len; i++){
      printAgg(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
    }
  }
}

void Visit(const koopa_raw_global_alloc_t& global, const koopa_raw_value_t& value){
  std::cout << "  .data\n";
  std::cout << "  .globl " << value->name + 1 << std::endl;
  std::cout << value->name + 1 << ":\n";
  if(global.init->kind.tag == KOOPA_RVT_INTEGER){
    std::cout << "  .word " << global.init->kind.data.integer.value << std::endl
              << std::endl;
  }
  else if(global.init->kind.tag == KOOPA_RVT_ZERO_INIT){
    auto base = value->ty->data.pointer.base;
    if(base->tag == KOOPA_RTT_INT32){
      std::cout << "  .zero 4\n\n";
    }
    else if(base->tag == KOOPA_RTT_ARRAY){
      int num = 1;
      while(base->tag == KOOPA_RTT_ARRAY){
        ValueArrayDim[value].push_back(base->data.array.len);
        num *= base->data.array.len;
        base = base->data.array.base;
      }
      std::cout << "  .zero " << num * 4 << std::endl
                << std::endl;
    }
  }
  else if(global.init->kind.tag == KOOPA_RVT_AGGREGATE){
    auto base = value->ty->data.pointer.base;
    while(base->tag == KOOPA_RTT_ARRAY){
      ValueArrayDim[value].push_back(base->data.array.len);
      base = base->data.array.base;
    }
    printAgg(global.init);
    std::cout << std::endl;
  }
  ValueToReg[value] = value->name + 1;
  // std::cout << "valueToReg = " << ValueToReg[value] << std::endl;
}

void Visit(int alloc, const koopa_raw_value_t &value){
  //std::cout << "alloc\n";
  //std::cout << stack_cnt << std::endl;
  // 之前alloc时先不分配，遇到store再分配，数组这样就不太好，于是顺便把非数组变量也分配了
  ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
  int num = getnum(value);
  stack_cnt += num;
}

void addi_func(std::string reg, std::string biassp){
  std::string bias_str = biassp.substr(0, biassp.length() - 4);
  int bias = std::stoi(bias_str);
  if(bias < -2048 || bias > 2047){
    std::string tmpreg = findReg();
    std::cout << "  li " << tmpreg << ", " << bias << std::endl;
    std::cout << "  add " << reg << ", sp, " << tmpreg << std::endl;
    invalidReg(tmpreg);
  }
  else{
    std::cout << "  addi " << reg << ", sp, " << bias << std::endl;
  }
}

int cal_arraysize(const koopa_raw_type_t &ty){
  if(ty->tag == KOOPA_RTT_ARRAY){
    return cal_arraysize(ty->data.array.base) * ty->data.array.len;
  }
  else{
    return 4;
  }
}

void Visit(const koopa_raw_get_elem_ptr_t &getelemptr, const koopa_raw_value_t &value){
  std::string reg1 = findReg();
  if(getelemptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    std::cout << "  la " << reg1 << ", " << ValueToReg[getelemptr.src] << std::endl;
  }
  else{
    addi_func(reg1, ValueToReg[getelemptr.src]);
    if(getelemptr.src->kind.tag == KOOPA_RVT_GET_PTR || getelemptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR){
      std::cout << "  lw " << reg1 << ", 0(" << reg1 << ")\n";
    }
  }
  std::string reg3 = findReg();
  giveReg(getelemptr.index);
  std::string reg2 = ValueToReg[getelemptr.index];
  int arraysize = cal_arraysize(getelemptr.src->ty->data.pointer.base->data.array.base);
  std::cout << "  li " << reg3 << ", " << arraysize << std::endl;
  std::cout << "  mul " << reg2 << ", " << reg2 << ", " << reg3 << std::endl;
  std::cout << "  add " << reg1 << ", " << reg1 << ", " << reg2 << std::endl;
  sw_func(reg1, std::to_string(stack_cnt * 4) + "(sp)");
  ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
  stack_cnt++;
  invalidReg(reg1);
  invalidReg(reg2);
  invalidReg(reg3);
}

void Visit(const koopa_raw_get_ptr_t &getptr, const koopa_raw_value_t &value){
  std::string reg1 = findReg();
  addi_func(reg1, ValueToReg[getptr.src]);
  std::cout << "  lw " << reg1 << ", 0(" << reg1 << ")\n";
  std::string reg3 = findReg();
  giveReg(getptr.index);
  std::string reg2 = ValueToReg[getptr.index];
  int arraysize = cal_arraysize(getptr.src->ty->data.pointer.base);
  std::cout << "  li " << reg3 << ", " << arraysize << std::endl;
  std::cout << "  mul " << reg2 << ", " << reg2 << ", " << reg3 << std::endl;
  std::cout << "  add " << reg1 << ", " << reg1 << ", " << reg2 << std::endl;
  sw_func(reg1, std::to_string(stack_cnt * 4) + "(sp)");
  ValueToReg[value] = std::to_string(stack_cnt * 4) + "(sp)";
  stack_cnt++;
  invalidReg(reg1);
  invalidReg(reg2);
  invalidReg(reg3);
}


int isptr(const koopa_raw_value_t &value){
  if(value->kind.tag == KOOPA_RVT_GET_PTR || value->kind.tag == KOOPA_RVT_GET_ELEM_PTR){
    return 1;
  }
  else if(value->kind.tag == KOOPA_RVT_LOAD){
    auto load = value->kind.data.load;
    if(load.src->kind.tag == KOOPA_RVT_ALLOC && load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER){
      return 1;
    }
  }
  return 0;
}

int getnum(const koopa_raw_value_t &value){
  if(value->kind.tag == KOOPA_RVT_ALLOC){
    return getnum2(value->ty->data.pointer.base);
  }
  else{
    return getnum2(value->ty);
  }
}

int getnum2(const koopa_raw_type_t &ty){
  if(ty->tag == KOOPA_RTT_INT32 || ty->tag == KOOPA_RTT_POINTER){
    return 1;
  }
  else if(ty->tag == KOOPA_RTT_ARRAY){
    return cal_arraysize(ty) / 4;
  }
  else{
    return 0;
  }
}