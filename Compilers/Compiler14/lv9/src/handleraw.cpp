//本文件作用是将KoopaIR通过koopa.h库转换成RISC-V汇编代码
#include <cassert>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include "handleraw.hpp"
#include "koopa.h"

using namespace std;

// 记录KoopaIR中所有有返回值的语句的相对栈帧指针sp的存储位置
static unordered_map<koopa_raw_value_t, int> loc;

// 记录生成数组/指针的alloc语句及其对应的数组/指针的维数
// [[i32, 2], 3] -> vector{3, 2}; **[[i32, 2], 3] ->  vector{1, 1, 3, 2}
static unordered_map<koopa_raw_value_t, vector<int> > array_dim;

static int stack_frame_length = 0;  // 用于记录栈帧的长度
static int stack_frame_used = 0;  // 用于记录栈帧已经使用的空间
static int saved_ra = 0;  // 用于记录是否需要为ra分配栈空间

// 对于getelemptr和getptr语句,其生成的指针可以用vector的一部分来表示
// 例如,对于数组a[2][2]
// @a = alloc [[i32, 2], 2] // @a的类型为*[[i32, 2], 2]
// 对a[1][1]的访问应该返回一个int,需要经过2次getelemptr,与index_list=2的大小相同
// %0 = getelemptr @a, 1  // %0的类型为*[i32, 2]
// %1 = getelemptr %0, 1  // %1的类型为*i32
// %2 = load %1  // %2的类型为i32

typedef pair<vector<int>::iterator, vector<int>::iterator> dim_range;
static unordered_map<koopa_raw_value_t, dim_range> array_ptr;



// value 是 imm12, 属于 [-2048, 2047]
static int value_is_imm12(int value) {
  return (value >= -2048) && (value < 2048);
}

// 判断指令为 KOOPA_RVT_GET_PTR 或 KOOPA_RVT_GET_ELEM_PTR,
// 或者是 KOOPA_RVT_GET_LOAD 且 load 的是函数开始保存数组参数的位置
static int value_is_ptr(const koopa_raw_value_t &value) {
  if (value->kind.tag == KOOPA_RVT_GET_PTR || value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
    return 1;
  }

  if (value->kind.tag == KOOPA_RVT_LOAD) {
    const auto& load = value->kind.data.load;
    if (load.src->kind.tag == KOOPA_RVT_ALLOC && load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
      return 1;
    }
  }
  return 0;
}

// 计算偏移量: 4 * dimension from begin+1 to end
static int calc_offset(const dim_range& begin_end) {
  int offset = 4;
  auto it = begin_end.first;
  ++it;
  for(; it != begin_end.second; ++it) {
    offset *= (*it);
  }
  return offset;
}



// 访问 raw program
void Visit_raw_program(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...

  // 访问所有全局变量
  Visit_raw_slice(program.values);
  // 访问所有函数
  Visit_raw_slice(program.funcs);
}

// 访问 raw slice
void Visit_raw_slice(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问function
        Visit_function(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问basic block
        Visit_block(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问instruction
        Visit_instruction(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问function
void Visit_function(const koopa_raw_function_t &func) {

  // 忽略函数声明
  if (func->bbs.len == 0) {
    return;
  }


  cout << "  .text" << endl;
  cout << "  .globl " << func->name+1 << endl;  // 去掉函数名前面的 @
  cout << func->name+1 << ":" << endl;

  stack_frame_length = 0;
  stack_frame_used = 0;

  // 计算栈帧长度需要的值

  int var_num = 0;  // 用于记录局部变量的个数
  int return_addr = 0; // 是否需要为 ra 分配栈空间
  int arg_num = 0;  // 需要为传参预留几个变量的栈空间

  // 遍历基本块
  for (size_t i = 0; i < func->bbs.len; ++i)
  {
    // 取基本块i中的所有指令
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    var_num += insts.len; // 先假设每条指令都有返回值,则每条指令对应一个局部变量

    // 遍历指令
    for (size_t j = 0; j < insts.len; ++j)
    { 
      // 取第j条指令
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT) { // 如果是unit类型,说明指令没有返回值,则不需要分配栈空间
        var_num--;
      }

      if(inst->kind.tag == KOOPA_RVT_CALL) {
        return_addr = true; //如果函数中出现了call,需要为ra分配大小为4的栈空间
        arg_num = max(arg_num, max(0, int(inst->kind.data.call.args.len) - 8)); //计算需要为传参预留几个变量的公式(-8是因为a0-a7是参数寄存器)
      }

      else if(inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
        // 如果是alloc语句,且生成的是数组,则需要为数组分配栈空间
        var_num--;
        int array_size = 1;
        auto base = inst->ty->data.pointer.base;
        while(base->tag == KOOPA_RTT_ARRAY) { 
          // 记录数组的维数
          array_dim[inst].push_back(base->data.array.len);
          array_size *= base->data.array.len;
          base = base->data.array.base;
        }
        array_ptr[inst] = make_pair(array_dim[inst].begin(), array_dim[inst].end());
        var_num += array_size;
      }

    }
  }

  stack_frame_length = (var_num + return_addr + arg_num) << 2;

  // 将栈帧长度对齐到 16
  stack_frame_length = (stack_frame_length + 16 - 1) & (~(16 - 1));
  stack_frame_used = arg_num << 2;  // 先为传参预留空间
 
  // 移动栈指针sp
  // 立即数一旦超过[-2048, 2047]这个范围, 必须用 li 加载立即数到一个临时寄存器 (比如 t0),
  // 然后用 add 指令来更新 sp 的值
  if (stack_frame_length != 0) {
    cout << "  li t0, " << -stack_frame_length << endl;
    cout << "  add sp, sp, t0" << endl;
  }

  if (return_addr) { // 如果需要为ra分配栈空间,则将ra保存在栈中,位置为sp+栈帧长度-4(即该函数栈帧最高地址处)
    cout << "  li t0, " << stack_frame_length - 4 << endl;
    cout << "  add t0, t0, sp" << endl;
    cout << "  sw ra, 0(t0)" << endl;
    saved_ra = 1;
  }
  else {
    saved_ra = 0;
  }

  // 访问所有basic block
  Visit_raw_slice(func->bbs);
  cout << endl;

}

// 访问basic block
void Visit_block(const koopa_raw_basic_block_t &bb) {

  // 输出当前块的label, %entry开头的不打印
  if(strncmp(bb->name+1, "entry", 5)){
    cout << bb->name+1 << ":" << endl;
  }
  
  // 访问所有instruction
  Visit_raw_slice(bb->insts);
}

// 访问instruction
void Visit_instruction(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_ALLOC:
      // 访问 local_alloc 指令
      Visit_local_alloc(value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      // 访问 global_alloc 指令
      Visit_global_alloc(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit_load(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit_store(kind.data.store);
      break;
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit_return(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit_integer(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit_binary(kind.data.binary, value);
      break;
    case KOOPA_RVT_GET_PTR:
      // 访问 getptr 指令
      Visit_getptr(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      // 访问 getelemptr 指令
      Visit_getelemptr(kind.data.get_elem_ptr, value);
      break;
    case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      Visit_branch(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      Visit_jump(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit_call(kind.data.call, value);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}


// 将整数放在标号为 reg 的寄存器中
static void load2reg(int value, const string &reg) {
  cout << "  li " << reg << ", " << value << endl;
}

// 将 value 的值放置在标号为 reg 的寄存器中
static void load2reg(const koopa_raw_value_t &value, const string &reg) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) { // value 是立即数
    load2reg(value->kind.data.integer.value, reg);
  }
  else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) { // value 是全局变量
    cout << "  la " << reg << ", " << value->name+1 << endl;  // 先将value的地址加载到reg
    cout << "  lw " << reg << ", 0(" << reg << ")" << endl;  // 再将这个地址中的值加载到reg
  }
  else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    // 函数参数
    const auto& index = value->kind.data.func_arg_ref.index;  // 参数序号
    if (index < 8) {  // 前8个参数,从寄存器复制到本函数栈帧里
      cout << "  mv " << reg << ", a" << index << endl;
    }
    else {
      int imm = stack_frame_length + (index - 8) * 4; // 到原函数栈帧去寻找
      if (value_is_imm12(imm)) {
        cout << "  addi " << reg << ", sp, " << imm << endl;
      }
      else {
        load2reg(imm, reg); // 先将imm加载到一个临时寄存器
        cout << "  add " << reg << ", " << reg << ", sp" << endl; // 再将sp加上这个立即数,得到的地址存入reg
      }
      cout << "  lw " << reg << ", 0(" << reg << ")" << endl; // 将这个地址中的值存入reg
        
    }
  }
  else {  // value是局部变量,首先根据value判断其在栈中的位置,然后将该位置的值存入寄存器
    int imm = loc[value];
    if (value_is_imm12(imm)) {
      cout << "  lw " << reg << ", " << imm << "(sp)" << endl;
    } 
    else {
      load2reg(imm, "t6"); // 先将imm加载到一个临时寄存器t6
      cout << "  add t6, t6, sp" << endl; // 再将sp加上这个立即数,得到的地址存入t6
      cout << "  lw " << reg << ", 0(t6)" << endl; // 最后将寄存器中的值存入这个地址
    }
  }
}
//这里之所以要做成同名函数,是希望下面访问binary指令时,无论binary.lhs是什么类型,都能够调用这个函数

// 将标号为 reg 的寄存器中的值保存在内存中
// 首先根据value判断其在栈中的位置,然后将寄存器中的值存入该位置
static void reg2mem(const koopa_raw_value_t &value, const string &reg) {
  assert(value->kind.tag != KOOPA_RVT_INTEGER);
  if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {  // value是全局变量
    cout << "  la t6, " << value->name+1 << endl;  // 先将value的地址加载到t6
    cout << "  sw " << reg << ", 0(t6)" << endl;  // 再将寄存器中的值存入这个地址
  }

  else {
    int imm = loc[value];
    if (value_is_imm12(imm)) {
      cout << "  sw " << reg << ", " << imm << "(sp)" << endl;
    } 
    else {
      load2reg(imm, "t6"); // 先将imm加载到一个临时寄存器t6
      cout << "  add t6, t6, sp" << endl; // 再将sp加上这个立即数,得到的地址存入t6
      cout << "  sw " << reg << ", 0(t6)" << endl; // 最后将寄存器中的值存入这个地址
    }
  }
}

// 访问 local_alloc 指令
void Visit_local_alloc(const koopa_raw_value_t &value) {

  auto base = value->ty->data.pointer.base;

  if (base->tag == KOOPA_RTT_INT32) {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
  }

  else if (base->tag == KOOPA_RTT_ARRAY) {
    // array_dim在Visit_Function中计算栈帧长度时已经算过
    int array_size = 4; // 一个int占4个字节
    for (auto i: array_dim[value]) {
      array_size *= i;
    }
    loc[value] = stack_frame_used;
    stack_frame_used += array_size;
  }
  else if (base->tag == KOOPA_RTT_POINTER) {
    // 对于形如**[[i32, 3], 2]类型的数组,前面有几个*就补几个 1
    while(base->tag == KOOPA_RTT_POINTER) {
      array_dim[value].push_back(1);
      base = base->data.array.base;
    }
    while(base->tag == KOOPA_RTT_ARRAY) {
      array_dim[value].push_back(base->data.array.len);
      base = base->data.array.base;
    }
    array_ptr[value] = make_pair(array_dim[value].begin(), array_dim[value].end());
    loc[value] = stack_frame_used;
    stack_frame_used += 4;  // 一个指针占4个字节
  }
  
}

// 递归实现全局数组的初始化
static void print_aggregate(const koopa_raw_value_t& value) {
  if(value->kind.tag == KOOPA_RVT_INTEGER) {
    cout << "  .word " << value->kind.data.integer.value << endl;
  }
  else if(value->kind.tag == KOOPA_RVT_AGGREGATE) {
    const auto& agg = value->kind.data.aggregate;
    for(int i = 0; i < agg.elems.len; i++) {
      print_aggregate(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
    }
  }
}

// 访问 global_alloc 指令
void Visit_global_alloc(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  
  cout << "  .data" << endl;
  cout << "  .globl " << value->name+1 << endl;
  cout << value->name+1 << ":" << endl;

  if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    // 初始化为 int
    cout << "  .word " << global_alloc.init->kind.data.integer.value << endl;
    cout << endl;
  }
  else if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    // 初始化为 zero

    auto base = value->ty->data.pointer.base;
    if (base->tag == KOOPA_RTT_INT32) {
      cout << "  .zero 4" << endl;
    }
    else if (base->tag == KOOPA_RTT_ARRAY) {
      int array_size = 4;
      while(base->tag == KOOPA_RTT_ARRAY) {
        array_dim[value].push_back(base->data.array.len);
        array_size *= base->data.array.len;
        base = base->data.array.base;
      }
      array_ptr[value] = make_pair(array_dim[value].begin(), array_dim[value].end());
      cout << "  .zero " << array_size << endl;
    }
    cout << endl;
  }

  else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
    // 数组初始化为 Aggregate
    auto base = value->ty->data.pointer.base;
    while(base->tag == KOOPA_RTT_ARRAY) {
      array_dim[value].push_back(base->data.array.len);
      base = base->data.array.base;
    }
    array_ptr[value] = make_pair(array_dim[value].begin(), array_dim[value].end());
    print_aggregate(global_alloc.init);
    cout << endl;
  }
}

// 访问 binary 指令
void Visit_binary(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
   
  // 将运算数存入 t0 和 t1
  load2reg(binary.lhs, "t0");
  load2reg(binary.rhs, "t1");
  

  // 进行运算,结果存入 t0
  if(binary.op == KOOPA_RBO_NOT_EQ) {
    cout << "  xor t0, t0, t1" << endl;
    cout << "  snez t0, t0" << endl;
  }
  else if(binary.op == KOOPA_RBO_EQ) {
    cout << "  xor t0, t0, t1" << endl;
    cout << "  seqz t0, t0" << endl;
  }
  else if(binary.op == KOOPA_RBO_GT) {
    cout << "  sgt t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_LT) {
     cout << "  slt t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_GE) {
    cout << "  slt t0, t0, t1" << endl;
    cout << "  xori t0, t0, 1" << endl;
  }
  else if(binary.op == KOOPA_RBO_LE) {
    cout << "  sgt t0, t0, t1" << endl;
    cout << "  xori t0, t0, 1" << endl;
  }
  else if(binary.op == KOOPA_RBO_ADD) {
    cout << "  add t0, t0, t1" << endl;  
  }
  else if(binary.op == KOOPA_RBO_SUB) {
    cout << "  sub t0, t0, t1" << endl; 
  }
  else if(binary.op == KOOPA_RBO_MUL) {
    cout << "  mul t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_DIV) {
    cout << "  div t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_MOD) {
    cout << "  rem t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_AND) {
    cout << "  and t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_OR) {
    cout << "  or t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_XOR) {
    cout << "  xor t0, t0, t1" << endl;
  }

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    loc[value] = stack_frame_used;  // 记录该变量的存储位置
    stack_frame_used += 4;  // 栈帧已使用空间增加4
    reg2mem(value, "t0");
  }

}

//访问 integer 指令
void Visit_integer(const koopa_raw_integer_t &integer) {
  cout << "  li a0, " << integer.value << endl;
}


// 访问 load 指令
void Visit_load(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {

  if (value_is_ptr(load.src)) {  // src是指针,需要先加载出数组元素的地址,再加载出值
    int imm = loc[load.src];
    if(value_is_imm12(imm)) {
      cout << "  lw t6, " << imm << "(sp)" << endl;
    }
    else {
      load2reg(imm, "t6"); // 先将imm加载到一个临时寄存器t0
      cout << "  add t6, t6, sp" << endl;
      cout << "  lw t6, 0(t6)" << endl;
    }

    cout << "  lw t0, 0(t6)" << endl;
  }

  else {
    // 查找load.src的存储位置,并将其值存入 t0
    load2reg(load.src, "t0");
    
  }
  
  // 若有返回值则保存到栈里
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    
    if(array_ptr.find(load.src) != array_ptr.end()) {
      array_ptr[value] = array_ptr[load.src];
    }
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    reg2mem(value, "t0");
  }
}

// 访问 store 指令
void Visit_store(const koopa_raw_store_t &store) {

  if (value_is_ptr(store.value)) {  // src是指针,需要先加载出数组元素的地址,再加载出值
    int imm = loc[store.value];
    if(value_is_imm12(imm)) {
      cout << "  lw t6, " << imm << "(sp)" << endl;
    }
    else {
      load2reg(imm, "t6"); // 先将imm加载到一个临时寄存器t0
      cout << "  add t6, t6, sp" << endl;
      cout << "  lw t6, 0(t6)" << endl;
    }

    cout << "  lw t0, 0(t6)" << endl;
  }

  else {
    // 查找store.value的存储位置,并将其值存入 t0
    load2reg(store.value, "t0");
  }

  if (value_is_ptr(store.dest)) {  // dest是指针,需要先加载出数组元素的地址,再存入值
    int imm = loc[store.dest];
    if (value_is_imm12(imm)) {
      cout << "  lw t6, " << imm << "(sp)" << endl;
    }
    else {
      load2reg(imm, "t6"); // 先将imm加载到一个临时寄存器t6
      cout << "  add t6, t6, sp" << endl;
      cout << "  lw t6, 0(t6)" << endl;
    }
    cout << "  sw t0, 0(t6)" << endl;
  }
  
  else {
    // 查找store.dest的存储位置,并将 t0 的值存入其中
    reg2mem(store.dest, "t0");
  }
  
  
}

void Visit_getelemptr(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
  const auto& begin_end = array_ptr[get_elem_ptr.src];
  if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    // 如果所操作的数组是全局变量,则直接将其地址加载到t0
    cout << "  la t0, " << get_elem_ptr.src->name+1 << endl; 
  }
  else if (get_elem_ptr.src->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    const auto& index = get_elem_ptr.src->kind.data.func_arg_ref.index;  
    // 参数序号
    if (index < 8) {  // 前8个参数
      cout << "  mv t0, a" << index << endl;
    }
    else {
      int imm = stack_frame_length + (index - 8) * 4; // 到原函数栈帧去寻找
      if (value_is_imm12(imm)) {
        cout << "  addi t0, sp, " << imm << endl;
      }
      else {
        load2reg(imm, "t0"); // 先将imm加载到一个临时寄存器
        cout << "  add t0, t0, sp" << endl; // 再将sp加上这个立即数,得到的地址存入reg
      }
      cout << "  lw t0, 0(t0)" << endl; // 将这个地址中的值存入reg
        
    }
  }
  else {
    int imm = loc[get_elem_ptr.src];
    if(value_is_imm12(imm)) {
      cout << "  addi t0, sp, " << imm << endl;
    }
    else {
      load2reg(imm, "t0");
      cout << "  add t0, t0, sp" << endl;
    }
  }
  

  if(value_is_ptr(get_elem_ptr.src)) {
    cout << "  lw t0, 0(t0)" << endl;
  }

  load2reg(get_elem_ptr.index, "t1");
  load2reg(calc_offset(begin_end), "t2");
  cout << "  mul t1, t1, t2" << endl;
  cout << "  add t0, t0, t1" << endl;

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    auto first = begin_end.first;
    ++first;
    array_ptr[value] = make_pair(first, begin_end.second);
    // 存入栈
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    reg2mem(value, "t0");
  }
}

// 与Visit_getelemptr逻辑完全相同
void Visit_getptr(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
  const auto& begin_end = array_ptr[get_ptr.src];
  
  if (get_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    // 如果所操作的数组是全局变量,则直接将其地址加载到t0
    cout << "  la t0, " << get_ptr.src->name+1 << endl; 
  }
  else if (get_ptr.src->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    const auto& index = get_ptr.src->kind.data.func_arg_ref.index;  
    // 参数序号
    if (index < 8) {  // 前8个参数
      cout << "  mv t0, a" << index << endl;
    }
    else {
      int imm = stack_frame_length + (index - 8) * 4; // 到原函数栈帧去寻找
      if (value_is_imm12(imm)) {
        cout << "  addi t0, sp, " << imm << endl;
      }
      else {
        load2reg(imm, "t0"); // 先将imm加载到一个临时寄存器
        cout << "  add t0, t0, sp" << endl; // 再将sp加上这个立即数,得到的地址存入reg
      }
      cout << "  lw t0, 0(t0)" << endl; // 将这个地址中的值存入reg
        
    }
  }
  else {
    int imm = loc[get_ptr.src];
    if(value_is_imm12(imm)) {
      cout << "  addi t0, sp, " << imm << endl;
    }
    else {
      load2reg(imm, "t0");
      cout << "  add t0, t0, sp" << endl;
    }
  }
  

  if(value_is_ptr(get_ptr.src)) {
    cout << "  lw t0, 0(t0)" << endl;
  }

  load2reg(get_ptr.index, "t1");
  load2reg(calc_offset(begin_end), "t2");
  cout << "  mul t1, t1, t2" << endl;
  cout << "  add t0, t0, t1" << endl;

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    auto first = begin_end.first;
    ++first;
    array_ptr[value] = make_pair(first, begin_end.second);
    // 存入栈
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    reg2mem(value, "t0");
  }
}


// 访问 branch 指令
void Visit_branch(const koopa_raw_branch_t &branch) {
  load2reg(branch.cond, "t0");
  cout << "  bnez t0, " << branch.true_bb->name+1 << endl;
  cout << "  j " << branch.false_bb->name+1 << endl;
}


// 访问 jump 指令
void Visit_jump(const koopa_raw_jump_t &jump) {
  cout << "  j " << jump.target->name+1 << endl;
}

// 访问 call 指令
void Visit_call(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
  
  // 处理待传参数
  for (int i = 0; i < call.args.len; ++i) {
    if (i < 8) {  // a0-a7
      load2reg(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), "a" + to_string(i));
    }
    else {  // 保存在栈中
      load2reg(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), "t0");
      cout << "  li t6, " << (i - 8) * 4 << endl;
      cout << "  add t6, t6, sp" << endl;
      cout << "  sw t0, 0(t6)" << endl;
    }
  }

  // 调用函数
  cout << "  call " << call.callee->name+1 << endl;

  // 将返回值存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    int imm = loc[value];
    if (value_is_imm12(imm)) {
      cout << "  sw a0, " << imm << "(sp)" << endl;
    }
    else {
      load2reg(imm, "t6");
      cout << "  add t6, t6, sp" << endl;
      cout << "  sw a0, 0(t6)" << endl;
    }
  }

}


// 访问 return 指令
void Visit_return(const koopa_raw_return_t &ret) { 
  // 返回值存入 a0
  if(ret.value != nullptr) {  //返回值为立即数
    load2reg(ret.value, "a0");
  }

  if (saved_ra) { // 将ra恢复
    cout << "  li t0, " << stack_frame_length - 4 << endl;
    cout << "  add t0, t0, sp" << endl;
    cout << "  lw ra, 0(t0)" << endl;
  }

   // 恢复栈帧
  if (stack_frame_length != 0) {
    cout << "  li t0, " << stack_frame_length << endl;
    cout << "  add sp, sp, t0" << endl;
  }

  cout << "  ret" << endl;
}