#include <cassert>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include "visitraw.hpp"
#include "koopa.h"

// 类型为 koopa_raw_value 的有返回值的语句 -> 该语句相对于 sp 的存储位置
static std::unordered_map<koopa_raw_value_t, int> loc;
// 栈帧长度
static int stack_frame_length = 0;
// 已经使用的栈帧长度
static int stack_frame_used = 0;
// 当前正在访问的函数有没有保存ra
static int saved_ra = 0;
// 生成数组/指针的 (global) alloc 语句 -> 该数组/指针的维数.
// [[i32, 2], 3] -> 3, 2; **[[i32, 2], 3] ->  1, 1, 3, 2
static std::unordered_map<koopa_raw_value_t, std::vector<int> > dimvec;
// getelemptr 和 getptr 语句 -> 生成的指针的维数,
// 表示为 dimvec 中的 vector 的某段的 begin 和 end
typedef std::pair<std::vector<int>::iterator, std::vector<int>::iterator> pvitvit;
static std::unordered_map<koopa_raw_value_t, pvitvit> dimlr;

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
  size_t i = 0;  
  while (i < slice.len) {  
    auto ptr = slice.buffer[i];  // 获取当前索引 i 处的元素指针，存储在 ptr 中
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {  // 根据 slice.kind 的不同，决定如何处理 ptr
      case KOOPA_RSIK_FUNCTION:  // 如果 slice.kind 是 KOOPA_RSIK_FUNCTION
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));  // 将 ptr 强制转换为 koopa_raw_function_t 类型，并访问该函数
        break;  // 结束当前 case 语句
      case KOOPA_RSIK_BASIC_BLOCK:  // 如果 slice.kind 是 KOOPA_RSIK_BASIC_BLOCK
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));  // 将 ptr 强制转换为 koopa_raw_basic_block_t 类型，并访问该基本块
        break;  // 结束当前 case 语句
      case KOOPA_RSIK_VALUE:  // 如果 slice.kind 是 KOOPA_RSIK_VALUE
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));  // 将 ptr 强制转换为 koopa_raw_value_t 类型，并访问该指令
        break;  // 结束当前 case 语句
      default:  // 如果 slice.kind 不匹配任何已知的类型
        assert(false);  // 触发断言错误，表示不应出现其他类型
    }
    ++i;  
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 忽略函数声明
  if(func->bbs.len == 0) {  // 如果函数没有基本块（即只是声明而没有实际代码）
    return;  // 直接返回，不进行后续处理
  }

  // 执行一些其他的必要操作
  std::cout << "  .text" << std::endl;  // 打印 .text，标识接下来的代码段
  std::cout << "  .globl " << func->name+1 << std::endl;  // 输出函数名，注意加1是为了去掉名称前的某些无关字符
  std::cout << func->name+1 << ":" << std::endl;  // 打印函数的标识符，用于生成汇编代码

  // 清空栈帧的长度和使用情况
  stack_frame_length = 0;  // 初始化栈帧的总长度
  stack_frame_used = 0;  // 初始化已使用的栈空间

  // 计算栈帧长度需要的值
  // 局部变量个数
  int local_var = 0;  // 初始化局部变量的数量
  // 是否需要为 ra 分配栈空间
  int return_addr = 0;  // 用来标记是否需要为返回地址分配栈空间
  // 需要为传参预留几个变量的栈空间
  int arg_var = 0;  // 用来标记需要为传递的参数分配栈空间

  size_t i = 0;
  // 遍历基本块，计算局部变量的数量、返回地址和参数的栈空间需求
  while (i < func->bbs.len) {  
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;  // 获取当前基本块的指令
    local_var += insts.len;  // 将当前基本块中的指令数量加到局部变量的数量上
    
    size_t j = 0;
    while (j < insts.len) {  
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);  // 获取指令
      if(inst->ty->tag == KOOPA_RTT_UNIT) {  // 如果指令的类型是 KOOPA_RTT_UNIT，表示不返回值的指令
        local_var--;  // 不需要为该指令分配栈空间
      }
      if(inst->kind.tag == KOOPA_RVT_CALL) {  // 如果指令是调用函数的指令
        return_addr = 1;  // 需要为返回地址分配栈空间
        arg_var = std::max(arg_var, std::max(0, int(inst->kind.data.call.args.len) - 8));  // 计算需要为传入参数预留多少栈空间（8为寄存器传参的上限）
      }
      else if(inst->kind.tag == KOOPA_RVT_ALLOC &&  // 如果指令是内存分配指令，且分配的是数组
        inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {  // 判断分配的是否为数组
        local_var--;  // 为数组分配的内存不算在局部变量中
        int arrmem = 1;  // 初始化数组占用的内存为1
        auto base = inst->ty->data.pointer.base;  // 获取数组的基类型
        while(base->tag == KOOPA_RTT_ARRAY) {  // 如果当前指针指向的还是一个数组
          dimvec[inst].push_back(base->data.array.len);  // 将当前数组的长度加入数组维度向量中
          arrmem *= base->data.array.len;  // 计算数组占用的总内存
          base = base->data.array.base;  // 继续访问数组的基类型
        }
        dimlr[inst] = std::make_pair(dimvec[inst].begin(), dimvec[inst].end());  // 记录数组的维度信息
        local_var += arrmem;  // 为数组分配的内存增加到局部变量的内存需求中
      }
      j++; 
    }
    i++;  
  }

  // 计算最终的栈帧长度（局部变量 + 返回地址 + 传参所需空间）
  stack_frame_length = (local_var + return_addr + arg_var) << 2;  // 每个变量占用4字节，因此需要乘以4
  // 将栈帧长度对齐到16字节
  stack_frame_length = (stack_frame_length + 16 - 1) & (~(16 - 1));  // 对齐到16字节的倍数
  stack_frame_used = arg_var << 2;  // 计算已使用的栈空间，传递参数所需的栈空间

  // 如果栈帧长度不为0，调整栈指针
  if (stack_frame_length != 0) {  // 如果栈帧需要占用空间
    std::cout << "  li t0, " << -stack_frame_length << std::endl;  // 将栈帧长度负值加载到寄存器t0中
    std::cout << "  add sp, sp, t0" << std::endl;  // 调整栈指针，减去栈帧长度
  }

  // 如果需要为返回地址分配栈空间
  if(return_addr) {  // 如果返回地址需要存储
    std::cout << "  li t0, " << stack_frame_length - 4 << std::endl;  // 计算返回地址的位置
    std::cout << "  add t0, t0, sp" << std::endl;  // 将返回地址的位置加到栈指针中
    std::cout << "  sw ra, 0(t0)" << std::endl;  // 将返回地址保存到栈中
    saved_ra = 1;  // 标记返回地址已保存
  }
  else {
    saved_ra = 0;  // 如果不需要保存返回地址，标记为未保存
  }

  // 访问所有基本块
  Visit(func->bbs);  // 访问函数中的所有基本块
  std::cout << std::endl;  // 输出换行
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // 当前块的label, %entry开头的不打印
  if(strncmp(bb->name+1, "entry", 5))
    std::cout << bb->name+1 << ":" << std::endl;
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_ALLOC:
      // 访问 alloc 指令
      Visit("alloc", value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      // 访问 global alloc 指令
      Visit(value->kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_GET_PTR:
      // 访问 getptr 指令
      Visit(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      // 访问 getelemptr 指令
      Visit(kind.data.get_elem_ptr, value);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    default:
      // 其他类型暂时遇不到
      // assert(false);
      break;
  }
}

// value->kind.tag 为 KOOPA_RVT_GET_PTR 或 KOOPA_RVT_GET_ELEM_PTR,
// 或者是 KOOPA_RVT_GET_LOAD 且 load 的是函数开始保存数组参数的位置
// 判断给定的值是否是指针类型
static int value_is_ptr(const koopa_raw_value_t &value) {
  int result = 0;  // 默认不是指针类型

  while (result == 0) {
    // 如果值的类型是 GET_PTR，表示它是一个指针类型
    if (value->kind.tag == KOOPA_RVT_GET_PTR) {
      result = 1;  // 设置为指针类型
      break;
    }

    // 如果值的类型是 GET_ELEM_PTR，表示它是一个元素指针类型
    if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
      result = 1;  // 设置为指针类型
      break;
    }

    // 如果值的类型是 LOAD，检查其源是否为指针类型
    if (value->kind.tag == KOOPA_RVT_LOAD) {
      const auto& load = value->kind.data.load;  // 获取LOAD指令的数据
      // 如果LOAD源是一个ALLOC指令，并且它是一个指向指针的数组
      if (load.src->kind.tag == KOOPA_RVT_ALLOC) {
        if(load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
          result = 1;  // 设置为指针类型
        }
      }
      break;
    }

    break; 
  }
  return result;  // 返回是否是指针类型
}

// 判断值是否是一个合法的 imm12，值范围在 [-2048, 2047] 之间
static int value_is_imm12(int value) {
  int result = 0;
  while (result == 0) {
    // 如果值在 [-2048, 2047] 范围内，返回1，表示它是一个合法的 imm12
    if ((value >= -2048) && (value < 2048)) {
      result = 1;
    }
    break;  
  }
  return result;  // 返回是否是合法的 imm12
}

// 将一个整数值加载到指定的寄存器中
static void load2reg(int value, const std::string &reg) {
  // 输出指令，将值加载到寄存器中
  std::cout << "  li " << reg << ", " << value << std::endl;
}

// 将给定值的地址加载到指定寄存器中
static void loadaddr2reg(const koopa_raw_value_t &value, const std::string &reg) {
  while (true) {
    // 如果值是函数参数引用
    if(value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
      const auto& index = value->kind.data.func_arg_ref.index;  // 获取函数参数的索引
      assert(index >= 8);  // 确保索引大于等于8，因为前8个参数是通过寄存器传递的
      int imm = stack_frame_length + (index - 8) * 4;  // 计算该参数在栈中的偏移量

      // 如果偏移量是合法的 imm12，则直接使用 addi 指令
      if (value_is_imm12(imm)) {
        std::cout << "  addi " << reg << ", sp, " << imm << std::endl;
      }
      else {  // 否则，将偏移量加载到寄存器中，再与栈指针相加
        load2reg(imm, reg);  // 将偏移量加载到寄存器中
        std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;  // 加上栈指针得到最终地址
      }
      break;  
    }
    // 如果值是全局变量的地址
    else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
      // 使用 la 指令将全局变量的地址加载到寄存器中
      std::cout << "  la " << reg << ", " << value->name+1 << std::endl;
      break;  
    }
    else {
      int imm = loc[value];  // 获取值在栈中的位置
      // 如果位置是合法的 imm12，则直接使用 addi 指令
      if (value_is_imm12(imm)) {
        std::cout << "  addi " << reg << ", sp, " << imm << std::endl;
      }
      else {  // 否则，将位置加载到寄存器中，再与栈指针相加
        load2reg(imm, reg);  // 将位置加载到寄存器中
        std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;  // 加上栈指针得到最终地址
      }
      break;  
    }
  }
}

// 将 value 的值放置在标号为 reg 的寄存器中
static void load2reg(const koopa_raw_value_t &value, const std::string &reg) {
  // 判断 value 的类型是否是整型
  while (value->kind.tag == KOOPA_RVT_INTEGER) {
    // 如果是整型，则将整型值加载到寄存器中
    load2reg(value->kind.data.integer.value, reg);
    return; // 返回
  }

  // 判断 value 的类型是否是函数参数引用
  while (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    const auto& index = value->kind.data.func_arg_ref.index;
    // 如果函数参数索引小于8
    while (index < 8) {
      // 将参数直接从 a 寄存器移动到目标寄存器
      std::cout << "  mv " << reg << ", a" << index << std::endl;
      return; // 返回
    }
    // 如果参数索引大于等于8，将地址加载到 t6 寄存器中
    loadaddr2reg(value, "t6");
    // 从栈中加载数据到目标寄存器
    std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
    return; // 返回
  }

  // 如果是其他类型，加载地址到 t6 寄存器中
  while (true) {
    loadaddr2reg(value, "t6");
    // 从栈中加载数据到目标寄存器
    std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
    return; // 返回
  }
}

// 将标号为 reg 的寄存器中的 value 的值保存在内存中
static void save2mem(const koopa_raw_value_t &value, const std::string &reg) {
  // 确保 value 不是整型
  assert(value->kind.tag != KOOPA_RVT_INTEGER);
  // 加载 value 的地址到 t6 寄存器
  loadaddr2reg(value, "t6");
  // 将寄存器中的数据存储到内存地址
  std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
}

// 访问 integer 指令，输出指令到控制台
void Visit(const koopa_raw_integer_t &integer) {
  // 输出将整数加载到 a0 寄存器的汇编指令
  std::cout << "  li a0, " << integer.value << std::endl;
}

// 遍历聚合类型的元素并输出为 .word 格式
static void dfs_aggregate(const koopa_raw_value_t& value) {
  // 如果 value 是整型
  if(value->kind.tag == KOOPA_RVT_INTEGER) {
    // 输出整型值到控制台
    std::cout << "  .word " << value->kind.data.integer.value << std::endl;
  }
  // 如果 value 是聚合类型
  else if(value->kind.tag == KOOPA_RVT_AGGREGATE) {
    const auto& agg = value->kind.data.aggregate;
    size_t i = 0;
    // 遍历聚合的元素
    while (i < agg.elems.len) {
      // 递归调用处理聚合元素
      dfs_aggregate(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
      i++; // 递增索引
    }
  }
}

// 访问 global alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  // 输出 .data 段
  std::cout << "  .data" << std::endl;
  // 输出全局符号的名称
  std::cout << "  .globl " << value->name+1 << std::endl;
  // 输出全局变量的标签
  std::cout << value->name+1 << ":" << std::endl;

  // 检查 global_alloc.init 的类型并处理
  while (true) {
    if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
      // 如果初始化为整型值，则直接输出 .word 指令
      std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
      break; 
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
      // 如果初始化为零，检查类型并输出相应的 .zero 指令
      auto base = value->ty->data.pointer.base;
      while (true) {
        if (base->tag == KOOPA_RTT_INT32) {
          std::cout << "  .zero 4" << std::endl;
          break;
        } else if (base->tag == KOOPA_RTT_ARRAY) {
          int zeromem = 4; // 初始假设每个元素占 4 字节
          while(base->tag == KOOPA_RTT_ARRAY) {
            dimvec[value].push_back(base->data.array.len); // 保存数组维度长度
            zeromem *= base->data.array.len; // 更新内存大小
            base = base->data.array.base; // 继续遍历数组的基类型
          }
          // 记录数组的维度信息
          dimlr[value] = std::make_pair(dimvec[value].begin(), dimvec[value].end());
          std::cout << "  .zero " << zeromem << std::endl;
          break;
        }
      }
      break; 
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
      // 如果初始化为聚合类型，则递归处理每个元素
      auto base = value->ty->data.pointer.base;
      while(base->tag == KOOPA_RTT_ARRAY) {
        dimvec[value].push_back(base->data.array.len); // 保存数组维度长度
        base = base->data.array.base; // 继续遍历数组的基类型
      }
      // 记录数组的维度信息
      dimlr[value] = std::make_pair(dimvec[value].begin(), dimvec[value].end());
      // 递归输出聚合类型的初始化内容
      dfs_aggregate(global_alloc.init);
      break; 
    }
  }
  // 输出一个空行
  std::cout << std::endl;
}

// 访问没有 data 的 mode 指令
void Visit(const std::string &mode, const koopa_raw_value_t &value) {
  // 如果 mode 是 "alloc"，则处理分配操作
  while (true) {
    if (mode == "alloc") {
      auto base = value->ty->data.pointer.base;
      // 如果类型为 int32，则分配 4 字节内存
      if (base->tag == KOOPA_RTT_INT32) {
        loc[value] = stack_frame_used; // 记录内存位置
        stack_frame_used += 4; // 更新栈帧已使用的空间
        break;
      }
      // 如果类型为数组，计算数组所需的内存空间
      else if (base->tag == KOOPA_RTT_ARRAY) {
        // 在计算栈帧长度时已经计算过数组的维度
        int arrmem = 4; // 假设数组的每个元素占 4 字节
        // 遍历数组的每一维，计算内存大小
        for (int i = 0; i < dimvec[value].size(); ++i) {
          arrmem *= dimvec[value][i]; // 更新内存大小
        }
        loc[value] = stack_frame_used; // 记录内存位置
        stack_frame_used += arrmem; // 更新栈帧已使用的空间
        break;
      }
      // 如果类型为指针类型
      else if (base->tag == KOOPA_RTT_POINTER) {
        // 计算指针的维度信息，处理多级指针
        while (base->tag == KOOPA_RTT_POINTER) {
          dimvec[value].push_back(1); // 每一级指针维度大小为 1
          base = base->data.array.base; // 继续遍历指针的基类型
        }
        while(base->tag == KOOPA_RTT_ARRAY) {
          dimvec[value].push_back(base->data.array.len); // 保存数组维度长度
          base = base->data.array.base; // 继续遍历数组的基类型
        }
        // 记录数组的维度信息
        dimlr[value] = std::make_pair(dimvec[value].begin(), dimvec[value].end());
        loc[value] = stack_frame_used; // 记录内存位置
        stack_frame_used += 4; // 更新栈帧已使用的空间
        break;
      }
    }
  }
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
  load2reg(load.src, "t0");  // 将 load.src 的值加载到寄存器 t0 中
  if(value_is_ptr(load.src))  // 如果 load.src 是指针类型
    std::cout << "  lw t0, 0(t0)" << std::endl;  // 从 t0 指向的地址加载数据到 t0 寄存器

  // 若有返回值则保存到栈里
  if(value->ty->tag != KOOPA_RTT_UNIT) {  // 如果 value 类型不是单元类型（没有返回值）
    // 保存 dimlr
    if(dimlr.find(load.src) != dimlr.end())  // 如果 load.src 的维度信息存在
      dimlr[value] = dimlr[load.src];  // 将 load.src 的维度信息赋值给 value
    // 存入栈
    loc[value] = stack_frame_used;  // 为 value 分配栈帧位置
    stack_frame_used += 4;  // 增加栈帧使用的空间（假设为 4 字节）
    save2mem(value, "t0");  // 将 t0 的内容存储到栈中
  }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store) {
  load2reg(store.value, "t0");  // 将 store.value 加载到寄存器 t0 中
  if(value_is_ptr(store.value))  // 如果 store.value 是指针类型
    std::cout << "  lw t0, 0(t0)" << std::endl;  // 从 t0 指向的地址加载数据到 t0 寄存器

  // 这里不能直接调用 save2mem
  loadaddr2reg(store.dest, "t6");  // 将 store.dest 的地址加载到寄存器 t6 中
  if(value_is_ptr(store.dest))  // 如果 store.dest 是指针类型
    std::cout << "  lw t6, 0(t6)" << std::endl;  // 从 t6 指向的地址加载数据到 t6 寄存器
  std::cout << "  sw t0, 0(t6)" << std::endl;  // 将 t0 寄存器的内容存储到 t6 指向的地址
}

// 计算 offset, 为 4 * Prod from begin+1 to end
int calc_offset(const pvitvit& begin_end) {
  int offset = 4;  // 初始化 offset 为 4
  auto it = begin_end.first;  // 获取 begin_end 的起始迭代器
  ++it;  // 将迭代器指向下一个元素（begin + 1）
  
  while (it != begin_end.second) {  // 当迭代器未到达 end 时
    offset *= (*it);  // 将每个元素值与 offset 相乘
    ++it;  
  }

  return offset;  // 返回计算的 offset
}

// 访问 getptr 指令
void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
  const auto& begin_end = dimlr[get_ptr.src];  // 获取 get_ptr.src 的维度信息，保存为 begin_end
  loadaddr2reg(get_ptr.src, "t0");  // 将 get_ptr.src 的地址加载到寄存器 t0 中
  if(value_is_ptr(get_ptr.src)) {  // 如果 get_ptr.src 是指针类型
    std::cout << "  lw t0, 0(t0)" << std::endl;  // 从 t0 指向的地址加载数据到 t0 寄存器
  }

  load2reg(get_ptr.index, "t1");  // 将 get_ptr.index 的值加载到寄存器 t1 中
  load2reg(calc_offset(begin_end), "t2");  // 计算偏移量，并将结果加载到寄存器 t2 中
  std::cout << "  mul t1, t1, t2" << std::endl;  // 将 t1 和 t2 相乘，得到偏移量乘以索引值
  std::cout << "  add t0, t0, t1" << std::endl;  // 将 t0 和 t1 相加，得到最终的地址

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {  // 如果 value 类型不是 KOOPA_RTT_UNIT（没有返回值）
    // 保存 dimlr
    auto first = begin_end.first;  // 获取 begin_end 的第一个元素（维度信息的开始）
    ++first;  // 将 first 移动到下一个元素
    dimlr[value] = std::make_pair(first, begin_end.second);  // 更新 dimlr 中对应 value 的维度信息（从第一个维度开始到结束）
    
    // 存入栈
    loc[value] = stack_frame_used;  // 为 value 分配栈空间
    stack_frame_used += 4;  // 增加栈空间使用（假设每个变量占 4 字节）
    save2mem(value, "t0");  // 将 t0 寄存器中的值保存到栈中
  }
}

// 访问 getelemptr 指令
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
  const auto& begin_end = dimlr[get_elem_ptr.src];  // 获取 get_elem_ptr.src 的维度信息，保存为 begin_end
  loadaddr2reg(get_elem_ptr.src, "t0");  // 将 get_elem_ptr.src 的地址加载到寄存器 t0 中
  if(value_is_ptr(get_elem_ptr.src)) {  // 如果 get_elem_ptr.src 是指针类型
    std::cout << "  lw t0, 0(t0)" << std::endl;  // 从 t0 指向的地址加载数据到 t0 寄存器
  }

  load2reg(get_elem_ptr.index, "t1");  // 将 get_elem_ptr.index 的值加载到寄存器 t1 中
  load2reg(calc_offset(begin_end), "t2");  // 计算偏移量，并将结果加载到寄存器 t2 中
  std::cout << "  mul t1, t1, t2" << std::endl;  // 将 t1 和 t2 相乘，得到偏移量乘以索引值
  std::cout << "  add t0, t0, t1" << std::endl;  // 将 t0 和 t1 相加，得到最终的地址

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {  // 如果 value 类型不是 KOOPA_RTT_UNIT（没有返回值）
    // 保存 dimlr
    auto first = begin_end.first;  // 获取 begin_end 的第一个元素（维度信息的开始）
    ++first;  // 将 first 移动到下一个元素
    dimlr[value] = std::make_pair(first, begin_end.second);  // 更新 dimlr 中对应 value 的维度信息（从第一个维度开始到结束）
    
    // 存入栈
    loc[value] = stack_frame_used;  // 为 value 分配栈空间
    stack_frame_used += 4;  // 增加栈空间使用（假设每个变量占 4 字节）
    save2mem(value, "t0");  // 将 t0 寄存器中的值保存到栈中
  }
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
  
  // 将运算数存入 t0 和 t1
  load2reg(binary.lhs, "t0");  // 将 binary 左操作数 (lhs) 加载到寄存器 t0 中
  load2reg(binary.rhs, "t1");  // 将 binary 右操作数 (rhs) 加载到寄存器 t1 中

  // 进行运算，结果存入 t0
  switch(binary.op) {
    case KOOPA_RBO_NOT_EQ:  // 如果操作是不等于 (!=)
      std::cout << "  xor t0, t0, t1" << std::endl;  // 将 t0 和 t1 进行异或操作 (相等则为 0，不等则为 1)
      std::cout << "  snez t0, t0" << std::endl;  // 如果 t0 不为零，设置 t0 为 1 (即不等于操作)
      break;

    case KOOPA_RBO_EQ:  // 如果操作是等于 (==)
      std::cout << "  xor t0, t0, t1" << std::endl;  // 将 t0 和 t1 进行异或操作 (相等则为 0，不等则为 1)
      std::cout << "  seqz t0, t0" << std::endl;  // 如果 t0 为零，设置 t0 为 1 (即相等操作)
      break;

    case KOOPA_RBO_GT:  // 如果操作是大于 (>)
      std::cout << "  sgt t0, t0, t1" << std::endl;  // 如果 t0 > t1，则设置 t0 为 1，否则为 0
      break;

    case KOOPA_RBO_LT:  // 如果操作是小于 (<)
      std::cout << "  slt t0, t0, t1" << std::endl;  // 如果 t0 < t1，则设置 t0 为 1，否则为 0
      break;

    case KOOPA_RBO_GE:  // 如果操作是大于等于 (>=)
      std::cout << "  slt t0, t0, t1" << std::endl;  // 如果 t0 < t1，则设置 t0 为 1
      std::cout << "  xori t0, t0, 1" << std::endl;  // 将 t0 取反，得到 t0 >= t1 的结果
      break;

    case KOOPA_RBO_LE:  // 如果操作是小于等于 (<=)
      std::cout << "  sgt t0, t0, t1" << std::endl;  // 如果 t0 > t1，则设置 t0 为 1
      std::cout << "  xori t0, t0, 1" << std::endl;  // 将 t0 取反，得到 t0 <= t1 的结果
      break;

    case KOOPA_RBO_ADD:  // 如果操作是加法 (+)
      std::cout << "  add t0, t0, t1" << std::endl;  // 将 t0 和 t1 相加，结果存入 t0
      break;

    case KOOPA_RBO_SUB:  // 如果操作是减法 (-)
      std::cout << "  sub t0, t0, t1" << std::endl;  // 将 t0 减去 t1，结果存入 t0
      break;

    case KOOPA_RBO_MUL:  // 如果操作是乘法 (*)
      std::cout << "  mul t0, t0, t1" << std::endl;  // 将 t0 和 t1 相乘，结果存入 t0
      break;

    case KOOPA_RBO_DIV:  // 如果操作是除法 (/)
      std::cout << "  div t0, t0, t1" << std::endl;  // 将 t0 除以 t1，结果存入 t0
      break;

    case KOOPA_RBO_MOD:  // 如果操作是取模 (%)
      std::cout << "  rem t0, t0, t1" << std::endl;  // 将 t0 对 t1 取余，结果存入 t0
      break;

    case KOOPA_RBO_AND:  // 如果操作是按位与 (AND)
      std::cout << "  and t0, t0, t1" << std::endl;  // 将 t0 和 t1 进行按位与操作，结果存入 t0
      break;

    case KOOPA_RBO_OR:  // 如果操作是按位或 (OR)
      std::cout << "  or t0, t0, t1" << std::endl;  // 将 t0 和 t1 进行按位或操作，结果存入 t0
      break;

    case KOOPA_RBO_XOR:  // 如果操作是按位异或 (XOR)
      std::cout << "  xor t0, t0, t1" << std::endl;  // 将 t0 和 t1 进行按位异或操作，结果存入 t0
      break;

    case KOOPA_RBO_SHL:  // 如果操作是左移 (SHL)
      std::cout << "  sll t0, t0, t1" << std::endl;  // 将 t0 左移 t1 位，结果存入 t0
      break;

    case KOOPA_RBO_SHR:  // 如果操作是逻辑右移 (SHR)
      std::cout << "  srl t0, t0, t1" << std::endl;  // 将 t0 右移 t1 位，结果存入 t0
      break;

    case KOOPA_RBO_SAR:  // 如果操作是算术右移 (SAR)
      std::cout << "  sra t0, t0, t1" << std::endl;  // 将 t0 算术右移 t1 位，结果存入 t0
      break;

    default:
      // 未知操作符的情况（可选择报错或处理）
      std::cerr << "Unknown binary operation" << std::endl;
      break;
  }

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {  // 如果 value 不是单位类型（即有返回值）
    loc[value] = stack_frame_used;  // 将 value 的位置分配到当前的栈帧
    stack_frame_used += 4;  // 增加栈帧的使用空间（4 字节）
    save2mem(value, "t0");  // 将寄存器 t0 的值保存到栈中
  }
}

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch) {
  load2reg(branch.cond, "t0");  // 将条件值（branch.cond）加载到寄存器 t0 中
  while (true) {  
    std::cout << "  bnez t0, DOUBLE_JUMP_" << branch.true_bb->name+1 << std::endl;  // 如果 t0 不等于零，则跳转到指定的 true_bb（布尔值为 true 的基本块）
    break; 
  }
  while (true) {  
    std::cout << "  j " << branch.false_bb->name+1 << std::endl;  // 如果条件为 false，则无条件跳转到 false_bb（布尔值为 false 的基本块）
    break; 
  }
  std::cout << "DOUBLE_JUMP_" << branch.true_bb->name+1 << ":" << std::endl;  // 标签 DOUBLE_JUMP_后接 true_bb 的基本块名称，用于跳转到此基本块
  std::cout << "  j " << branch.true_bb->name+1 << std::endl;  // 无条件跳转到 true_bb
}

// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump) {
  std::cout << "  j " << jump.target->name+1 << std::endl;  // 跳转到指定目标基本块（jump.target）
}

// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
  size_t i = 0;
  while (i < call.args.len) {  
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);  // 获取参数
    if (i < 8) {  // 如果参数的索引小于 8，将参数加载到寄存器 a0 到 a7 中
      load2reg(arg, "a" + std::to_string(i));  // 将参数加载到对应的寄存器
    }
    else {  // 如果参数索引大于或等于 8，将参数存入栈中
      load2reg(arg, "t0");  // 将参数加载到 t0 寄存器
      std::cout << "  li t6, " << (i - 8) * 4 << std::endl;  // 计算偏移量，将栈空间分配给多余的参数
      std::cout << "  add t6, t6, sp" << std::endl;  // 计算栈中实际存储参数的位置
      std::cout << "  sw t0, 0(t6)" << std::endl;  // 将 t0 中的参数存入栈
    }
    ++i;  
  }
  // 调用函数
  std::cout << "  call " << call.callee->name+1 << std::endl;  // 调用指定的函数（call.callee）

  // 若有返回值则将 a0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {  // 如果返回值不为空（即有返回值）
    loc[value] = stack_frame_used;  // 将返回值的地址分配到当前栈帧
    stack_frame_used += 4;  // 增加栈帧的使用空间
    save2mem(value, "a0");  // 将寄存器 a0 中的返回值存入栈
  }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret) {
  // 返回值存入 a0
  if(ret.value != nullptr)  // 如果有返回值
    load2reg(ret.value, "a0");  // 将返回值加载到寄存器 a0 中
  // 从栈帧中恢复 ra 寄存器
  if (saved_ra) {  // 如果保存了返回地址（ra 寄存器）
    std::cout << "  li t0, " << stack_frame_length - 4 << std::endl;  // 计算返回地址的偏移量
    std::cout << "  add t0, t0, sp" << std::endl;  // 计算栈中返回地址的位置
    std::cout << "  lw ra, 0(t0)" << std::endl;  // 恢复 ra 寄存器的值
  }
  // 恢复栈帧
  if (stack_frame_length != 0) {  // 如果栈帧的长度不为零
    std::cout << "  li t0, " << stack_frame_length << std::endl;  // 将栈帧的长度加载到 t0 寄存器
    std::cout << "  add sp, sp, t0" << std::endl;  // 恢复栈指针（sp），释放栈空间
  }
  std::cout << "  ret" << std::endl;  // 执行函数返回
}
