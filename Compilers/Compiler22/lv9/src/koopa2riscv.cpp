#include "koopa2riscv.hpp"

#include "koopa.h"

#include <cassert>
#include <string.h>

#include <sstream>
#include <string>
#include <map>

#include <set>



static std::ostringstream func_seg, data_seg, text_seg;

std::set<koopa_raw_value_t> is_value_visit;
std::set<koopa_raw_basic_block_t> is_block_visit;
std::set<koopa_raw_value_t> is_global_var_visit;


std::map<koopa_raw_basic_block_t, std::string> block_to_identname;
std::string get_block_to_identname(const koopa_raw_basic_block_t & value) {
  assert(block_to_identname.find(value)!=block_to_identname.end());
  return block_to_identname[value];
}

void set_block_to_identname(const koopa_raw_basic_block_t & value, std::string s) {
  assert(block_to_identname.find(value)==block_to_identname.end());
  block_to_identname[value] = s;
}


std::map<koopa_raw_function_t, std::string> func_to_identname;
std::string get_func_to_identname(const koopa_raw_function_t &value) {
  assert(func_to_identname.find(value)!=func_to_identname.end());
  return func_to_identname[value];
}

void set_func_to_identname(const koopa_raw_function_t &value, std::string s) {
  assert(func_to_identname.find(value)==func_to_identname.end());
  func_to_identname[value] = s;
}



std::map<koopa_raw_value_t, std::string> global_value_to_identname;
std::string get_global_value_to_identname(const koopa_raw_value_t &value) {
  assert(global_value_to_identname.find(value)!=global_value_to_identname.end());
  return global_value_to_identname[value];
}
void set_global_value_to_identname(const koopa_raw_value_t &value, std::string s) {
  assert(global_value_to_identname.find(value)==global_value_to_identname.end());
  global_value_to_identname[value] = s;
}

static std::string get_a_new_blockname() {
  static int cnt = 0;
  ++cnt;
  return "AHFHAf32rjisdlppoipip23pjjfjf"+std::to_string(cnt);
}

namespace Mem
{
  const int reg_num = 14;
  std::string names[reg_num]={"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                       "a0", "a1", "a2", "a3", "a4", "a5", "a6"};
  
  bool available[reg_num];

  bool is_reg_name(std::string x) {
    for(int i = 0;i<reg_num;i++) {
      if(x==names[i]) return true;
    }
    return false;
  }

  int stack_size;

  // offset of a stack variable from fp (top of the stack frame)
  std::map<koopa_raw_value_t, int> stack_ident_to_offset;

  void set_stack_ident_to_offset(const koopa_raw_value_t & value, int offset) {
    assert(stack_ident_to_offset.find(value)==stack_ident_to_offset.end());
    stack_ident_to_offset[value]=offset;
  }

  int get_stack_offset_from_ident(const koopa_raw_value_t & value) {
    assert(stack_ident_to_offset.find(value)!=stack_ident_to_offset.end());
    return stack_ident_to_offset[value];
  }





  void func_init() {
    stack_size=0;
    stack_ident_to_offset.clear();
    for(int i = 0;i<reg_num;i++) available[i] = true;
  }

  void init() {
    func_init();
  }

  bool is_available(std::string x) {
    assert(is_reg_name(x));
    for(int i = 0;i<reg_num;i++) {
      if(x==names[i]) return available[i];
    }
    return false;
  }

  // return "" when no available
  std::string may_require_a_reg() {
    for(int i = 0;i<reg_num;i++) {
      if(available[i]) {
        available[i] = false;
        return names[i];
      }
    } 
    return "";
  }

  std::string must_require_a_reg() {
    auto ans = may_require_a_reg();
    assert(ans != "");
    return ans;
  }

  void release_a_reg(std::string x) {
    assert(is_reg_name(x));
    for(int i = 0;i<reg_num;i++) {
      if(names[i]==x) {
        assert(available[i]==false);
        available[i] = true;
        return;
      }
    }
  } 

  std::string zero_reg = "x0";

  void load_from_stack_to_reg(int offset, std::string reg) {
    std::string ans = reg;
    func_seg<<"li "<<ans<<", "<<-offset<<std::endl;
    func_seg<<"add "<<ans<<", "<<ans<<", fp"<<std::endl;
    func_seg<<"lw "<<ans<<", "<<"0("<<ans<<")"<<std::endl;
  }

  std::string load_from_stack(int offset) {
    // warning
    // assert(offset>=0&&offset<stack_size);
    std::string ans = must_require_a_reg();
    load_from_stack_to_reg(offset, ans);
    return ans;
  }

  // warning: the reg won't be released by this function
  void store_to_stack(std::string reg, int offset) {
    assert(is_reg_name(reg));
    std::string tmp = must_require_a_reg();
    func_seg<<"li "<<tmp<<", "<<-offset<<std::endl;
    func_seg<<"add "<<tmp<<", "<<tmp<<", fp"<<std::endl;
    func_seg<<"sw "<<reg<<", "<<"0("<<tmp<<")"<<std::endl;
    release_a_reg(tmp);
  }
    // return alloced size
  // add inst on sp
  int alloc_on_stack(int sz) {
    assert(sz>0);
    // 向16上取整
    sz = ((sz+15)/16)*16;
    stack_size += sz;

    // warning this may be wrong

    // std::string reg = must_require_a_reg();
    // func_seg<<"li "<<reg<<", "<<-sz<<std::endl;
    // func_seg<<"add sp, sp, "<<reg<<std::endl;
    // release_a_reg(reg);
    return sz;
  }


  // void release_on_stack(int sz) {
  //   assert(sz==stack_size);

  //   // warning this may be wrong
  //   stack_size-=sz;
  //   std::string reg = must_require_a_reg();
  //   func_seg<<"li "<<reg<<", "<<sz<<std::endl;
  //   func_seg<<"add sp, sp, "<<reg<<std::endl;
  //   release_a_reg(reg);    
  // }

} // namespace Mem



static void Visit(const koopa_raw_program_t &program);
static void Visit(const koopa_raw_slice_t &slice);
static void Visit(const koopa_raw_function_t &func);
static void Visit(const koopa_raw_basic_block_t &bb);
static void Visit(const koopa_raw_value_t &value);
static void Visit(const koopa_raw_return_t &value);
static void Visit(const koopa_raw_integer_t &value);
static void Visit(const koopa_raw_store_t &value);
static void Visit(const koopa_raw_branch_t &value);
static void Visit(const koopa_raw_global_alloc_t &value);



static std::string Visit(const koopa_raw_load_t &value);
static std::string Visit(const koopa_raw_binary_t &value);
static std::string Visit(const koopa_raw_call_t &value);
static std::string Visit(const koopa_raw_get_elem_ptr_t &value);
static std::string Visit(const koopa_raw_get_ptr_t &value);

void koopa2riscv(const std::string &from, std::ostream &out)
{
    is_value_visit.clear();
    block_to_identname.clear();
    func_to_identname.clear();
    global_value_to_identname.clear();
    is_block_visit.clear();
    is_global_var_visit.clear();
    Mem::init();
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(from.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);   

    // 处理 raw program
    /*
    最上层是 koopa_raw_program_t, 也就是 Program.
    之下是全局变量定义列表和函数定义列表.
    在 raw program 中, 列表的类型是 koopa_raw_slice_t.
    本质上这是一个指针数组, 其中的 buffer 字段记录了指针数组的地址 (类型是 const void **), len 字段记录了指针数组的长度, kind 字段记录了数组元素是何种类型的指针
    在访问时, 你可以通过 slice.buffer[i] 拿到列表元素的指针, 然后通过判断 kind 来决定把这个指针转换成什么类型.
    koopa_raw_function_t 代表函数, 其中是基本块列表.
    koopa_raw_basic_block_t 代表基本块, 其中是指令列表.
    koopa_raw_value_t 代表全局变量, 或者基本块中的指令.
    */
    text_seg = std::ostringstream();
    // func_seg = std::ostringstream();
    data_seg = std::ostringstream();
    Visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);

    for(auto it=block_to_identname.begin();it!=block_to_identname.end();++it) {
      assert(it->first!=NULL);
      assert(is_block_visit.find(it->first)!=is_block_visit.end());
    }
    for(auto it=global_value_to_identname.begin();it!=global_value_to_identname.end();++it) {
      assert(it->first!=NULL);
      assert(is_global_var_visit.find(it->first)!=is_global_var_visit.end());
    }

    out<<".text"<<std::endl;
    out<<text_seg.str();
 
    out<<".data"<<std::endl;
    out<<data_seg.str();
   
}

// 函数声明略
// ...

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

void alloc_func_stack_size(const koopa_raw_function_t &func) {

}

void push_to_stack(std::string reg, std::ostream& out) {
  out<<"addi sp, sp, -16"<<std::endl;
  out<<"sw "<<reg<<", 0(sp)"<<std::endl;
}
void pop_from_stack(std::string reg, std::ostream& out) {
  out<<"lw "<<reg<<", 0(sp)"<<std::endl;
  out<<"addi sp, sp, 16"<<std::endl;
}

void may_alloc_name_for_func(const koopa_raw_function_t &func) {
  if(func_to_identname.find(func)==func_to_identname.end()) {
    set_func_to_identname(func, get_a_new_blockname());
  }
}




// 访问函数
void Visit(const koopa_raw_function_t &func) {

  // 执行一些其他的必要操作

  Mem::func_init();
  

  func_seg = std::ostringstream();

  assert(func->name!=NULL);
  if(func->name[0] == '@') {
    set_func_to_identname(func, func->name+1);
    if(get_func_to_identname(func)!="main") return;
    text_seg<<".globl "<<get_func_to_identname(func)<<std::endl;
    text_seg<<get_func_to_identname(func)<<":"<<std::endl;
    
  } else {
    // warning this may be wrong
    may_alloc_name_for_func(func);
    text_seg<<".globl "<<get_func_to_identname(func)<<std::endl;
    text_seg<<get_func_to_identname(func)<<":"<<std::endl;
  }

  // 保存所有参数到栈上
  push_to_stack("a7", text_seg);
  push_to_stack("a6", text_seg);
  push_to_stack("a5", text_seg);
  push_to_stack("a4", text_seg);
  push_to_stack("a3", text_seg);
  push_to_stack("a2", text_seg);
  push_to_stack("a1", text_seg);
  push_to_stack("a0", text_seg);

  push_to_stack("ra", text_seg);
  push_to_stack("fp", text_seg);

  text_seg<<"add fp, sp, x0"<<std::endl;
  for(int i = 0;i<func->params.len;i++) {
    auto value = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);
    assert(value->kind.tag==KOOPA_RVT_FUNC_ARG_REF);
    koopa_raw_func_arg_ref_t arg_ref = value->kind.data.func_arg_ref;
    assert(arg_ref.index==i);
    Mem::set_stack_ident_to_offset(value, -(2+((int)arg_ref.index))*16);

    // warning: this may be wrong
    is_value_visit.insert(value);
  }


  alloc_func_stack_size(func);
  
  // 访问所有基本块
  Visit(func->bbs);

  text_seg<<"li a0, "<<-Mem::stack_size<<std::endl;
  text_seg<<"add sp, sp, a0"<<std::endl;


  text_seg<<func_seg.str();
}

void may_alloc_name_for_block(const koopa_raw_basic_block_t &bb) {
  if(block_to_identname.find(bb)==block_to_identname.end()) {
    set_block_to_identname(bb, get_a_new_blockname());
  }
}



// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 已经visit过了
  if(is_block_visit.find(bb)!=is_block_visit.end()) return;
  
  // 执行一些其他的必要操作
  may_alloc_name_for_block(bb);
  is_block_visit.insert(bb);

  func_seg<<get_block_to_identname(bb)<<":"<<std::endl;
  // 访问所有指令
  Visit(bb->insts);
}
void may_alloc_name_for_global_var(const koopa_raw_value_t &value) {
  if(global_value_to_identname.find(value)==global_value_to_identname.end()) {
    set_global_value_to_identname(value, get_a_new_blockname());
  }
}

void get_value_to_reg(const koopa_raw_value_t& value, std::string reg) {
  // warning
  const auto &kind = value->kind;
  int offset;
  switch (kind.tag)
  {
  case KOOPA_RVT_BINARY:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);

    break;
  case KOOPA_RVT_INTEGER:
    func_seg<<"li "<<reg<<", "<<value->kind.data.integer.value<<std::endl;
    break;
  
  case KOOPA_RVT_ALLOC:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);
    break;

  case KOOPA_RVT_LOAD:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);
    break;    
  case KOOPA_RVT_FUNC_ARG_REF:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);
    break;
  case KOOPA_RVT_CALL:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);
    break;
  case KOOPA_RVT_GLOBAL_ALLOC:
    may_alloc_name_for_global_var(value);
    func_seg<<"la "<<reg<<", "<<get_global_value_to_identname(value)<<std::endl;
    // func_seg<<"lw "<<reg<<", 0("+reg+")"<<std::endl;
    break;
  case KOOPA_RVT_GET_ELEM_PTR:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);
    // func_seg<<"lw "<<reg<<", 0("+reg+")"<<std::endl;
    break;
  case KOOPA_RVT_GET_PTR:
    offset = Mem::get_stack_offset_from_ident(value);
    Mem::load_from_stack_to_reg(offset, reg);
    // func_seg<<"lw "<<reg<<", 0("+reg+")"<<std::endl;
    break;    
  default:
    // 其他类型尚未处理
    assert(false);
    break;
  }

}

std::string get_value_to_reg(const koopa_raw_value_t&value) {
  auto reg = Mem::must_require_a_reg();
  get_value_to_reg(value, reg);
  return reg;
}


int calc_variable_size_bytes(const koopa_raw_type_kind *base) {
  assert(base !=NULL);
  assert(base ->tag == KOOPA_RTT_ARRAY || base->tag == KOOPA_RTT_INT32);
  if(base->tag == KOOPA_RTT_INT32) return 4;
  if(base->tag == KOOPA_RTT_ARRAY) {
    return base->data.array.len*calc_variable_size_bytes(base->data.array.base);
  }
  assert(false);
}

int get_elem_size(const koopa_raw_value_t &value) {
  assert(value->kind.tag==KOOPA_RVT_ALLOC ||
         value->kind.tag==KOOPA_RVT_GLOBAL_ALLOC || 
         value->kind.tag==KOOPA_RVT_GET_ELEM_PTR ||
         value->kind.tag==KOOPA_RVT_GET_PTR);
  if(value->kind.tag==KOOPA_RVT_ALLOC) {
    assert(value->ty->tag==KOOPA_RTT_POINTER);
    assert(value->ty->data.pointer.base->tag==KOOPA_RTT_ARRAY);
    return calc_variable_size_bytes(value->ty->data.pointer.base->data.array.base);
  } else if(value->kind.tag==KOOPA_RVT_GLOBAL_ALLOC) {
    // warning this may be wrong
    assert(value->ty->tag==KOOPA_RTT_POINTER);
    assert(value->ty->data.pointer.base->tag==KOOPA_RTT_ARRAY);
    return calc_variable_size_bytes(value->ty->data.pointer.base->data.array.base);
  } else if(value->kind.tag==KOOPA_RVT_GET_ELEM_PTR) {
    // warning this may be wrong
    assert(value->ty->tag==KOOPA_RTT_POINTER);
    assert(value->ty->data.pointer.base->tag==KOOPA_RTT_ARRAY);
    return calc_variable_size_bytes(value->ty->data.pointer.base->data.array.base);    
  } else if(value->kind.tag==KOOPA_RVT_GET_PTR) {
    // warning this may be wrong
    assert(value->ty->tag==KOOPA_RTT_POINTER);
    assert(value->ty->data.pointer.base->tag==KOOPA_RTT_ARRAY);
    return calc_variable_size_bytes(value->ty->data.pointer.base->data.array.base);    
  }
  assert(false);
  return 0;
}

void init_global_var(const koopa_raw_value_t &value) {

  if(value->kind.tag==KOOPA_RVT_AGGREGATE) {
    assert(value->kind.data.aggregate.elems.len>0);
    assert(value->kind.data.aggregate.elems.kind==KOOPA_RSIK_VALUE);
    for(int i = 0;i<value->kind.data.aggregate.elems.len;i++) {
      auto sub_value = reinterpret_cast<koopa_raw_value_t>(value->kind.data.aggregate.elems.buffer[i]);
      init_global_var(sub_value);
    }
    
  } else if(value->kind.tag==KOOPA_RVT_INTEGER) {
    data_seg<<".word "<<value->kind.data.integer.value<<std::endl;
  } else if(value->kind.tag==KOOPA_RVT_ZERO_INIT) {
    int sz = calc_variable_size_bytes(value->ty);
    data_seg<<".zero "<<sz<<std::endl;
  } else{
    assert(false);
  }
 
}


// 访问指令
void Visit(const koopa_raw_value_t &value) {
  if(is_value_visit.find(value)!=is_value_visit.end())
    return;
  is_value_visit.insert(value);

  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  std::string ans;
  
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
      ans = Visit(kind.data.binary);
      if(value->ty->tag!=KOOPA_RTT_UNIT) {
        // 有返回值
        Mem::alloc_on_stack(4);
        Mem::set_stack_ident_to_offset(value, Mem::stack_size);
        Mem::store_to_stack(ans, Mem::stack_size) ;
      }
      Mem::release_a_reg(ans);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_LOAD:
      ans = Visit(kind.data.load);
      if(value->ty->tag!=KOOPA_RTT_UNIT) {
        // 有返回值
        Mem::alloc_on_stack(4);
        Mem::set_stack_ident_to_offset(value, Mem::stack_size);
        Mem::store_to_stack(ans, Mem::stack_size) ;
      } 
      Mem::release_a_reg(ans);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_CALL:
      ans = Visit(kind.data.call);
      if(value->ty->tag!=KOOPA_RTT_UNIT) {
        // 有返回值
        Mem::alloc_on_stack(4);
        Mem::set_stack_ident_to_offset(value, Mem::stack_size);
        Mem::store_to_stack(ans, Mem::stack_size) ;
      } 
      Mem::release_a_reg(ans);
      break;
    case KOOPA_RVT_ALLOC:
      // warning this may be wrong
      // 假设只是alloc 一个int
      assert(value->ty->tag==KOOPA_RTT_POINTER);
      Mem::alloc_on_stack(calc_variable_size_bytes(value->ty->data.pointer.base));
      Mem::set_stack_ident_to_offset(value, Mem::stack_size);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit(kind.data.global_alloc);
      may_alloc_name_for_global_var(value);
      is_global_var_visit.insert(value);
      data_seg<<".globl "<<get_global_value_to_identname(value)<<std::endl;
      data_seg<<get_global_value_to_identname(value)<<":"<<std::endl;
      init_global_var(value->kind.data.global_alloc.init);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      ans = Visit(kind.data.get_elem_ptr);
      if(value->ty->tag!=KOOPA_RTT_UNIT) {
        // 有返回值
        Mem::alloc_on_stack(4);
        Mem::set_stack_ident_to_offset(value, Mem::stack_size);
        Mem::store_to_stack(ans, Mem::stack_size) ;
      } 
      Mem::release_a_reg(ans);
      break;
    case KOOPA_RVT_GET_PTR:
      ans = Visit(kind.data.get_ptr);
      if(value->ty->tag!=KOOPA_RTT_UNIT) {
        // 有返回值
        Mem::alloc_on_stack(4);
        Mem::set_stack_ident_to_offset(value, Mem::stack_size);
        Mem::store_to_stack(ans, Mem::stack_size) ;
      } 
      Mem::release_a_reg(ans);
      break;  
    default:
      // 其他类型暂时遇不到
      // may "jump" in koopa IR
      assert(false);
      break;
  }
}

void Visit(const koopa_raw_return_t &value)
{
    if(value.value!=NULL) {
        Visit(value.value);

        auto reg = get_value_to_reg(value.value);
        
        // warning: this may be wrong
        func_seg<<"add a0, x0, "<<reg<<std::endl;
        Mem::release_a_reg(reg);
    }
    // 返回前恢复栈
    // warning this may be wrong
    func_seg<<"add sp, x0, fp"<<std::endl;
    pop_from_stack("fp", func_seg);
    pop_from_stack("ra", func_seg);

    for(int i = 0;i<=7;i++) {
      // 不是为了恢复寄存器，而是为了恢复sp的值
      pop_from_stack("a1", func_seg);
    }

    func_seg<<"ret"<<std::endl;
}



// return a Mem name
void Visit(const koopa_raw_integer_t &value) {
  // nothing
}

void store_const_array_to_stack(const koopa_raw_value_t &value ,int offset) {
  // assert(offset>=0);
  // warning be careful about offset
  if(value->kind.tag==KOOPA_RVT_AGGREGATE) {
    assert(value->kind.data.aggregate.elems.len>0);
    assert(value->kind.data.aggregate.elems.kind==KOOPA_RSIK_VALUE);
    int elem_size = calc_variable_size_bytes((
        reinterpret_cast<koopa_raw_value_t>(value->kind.data.aggregate.elems.buffer[0])
      )->ty);
    for(int i = 0;i<value->kind.data.aggregate.elems.len;i++) {
      auto sub_value = reinterpret_cast<koopa_raw_value_t>(value->kind.data.aggregate.elems.buffer[i]);
      int next_offset = offset - i*elem_size;
      store_const_array_to_stack(sub_value, next_offset);
    }
  } else if(value->kind.tag==KOOPA_RVT_INTEGER) {
    auto tmp_reg = Mem::must_require_a_reg();
    func_seg<<"li "+tmp_reg+", "+std::to_string(value->kind.data.integer.value)<<std::endl;

    Mem::store_to_stack(tmp_reg, offset);
    Mem::release_a_reg(tmp_reg);
  } else {
    assert(false);
  }
}


void Visit(const koopa_raw_store_t &value) {
  if(value.value->kind.tag==KOOPA_RVT_AGGREGATE) {
    // warning be careful
    // local const array store
    int offset = Mem::get_stack_offset_from_ident(value.dest);
    store_const_array_to_stack(value.value, offset);
    return;
  }
  Visit(value.value);
  int offset;
  std::string ans;
  std::string tmp_reg;

  switch (value.dest->kind.tag)
  {
  case KOOPA_RVT_ALLOC:
    offset = Mem::get_stack_offset_from_ident(value.dest);
    ans = get_value_to_reg(value.value);
    Mem::store_to_stack(ans, offset);
    Mem::release_a_reg(ans);
    break;
  
  case KOOPA_RVT_GLOBAL_ALLOC:
    tmp_reg = Mem::must_require_a_reg();
    ans = get_value_to_reg(value.value);
    may_alloc_name_for_global_var(value.dest);
    func_seg<<"la "+tmp_reg+", "+get_global_value_to_identname(value.dest)<<std::endl;
    func_seg<<"sw "+ans<<+", 0("+tmp_reg+")"<<std::endl;
    Mem::release_a_reg(tmp_reg);
    Mem::release_a_reg(ans);
    break;
  case KOOPA_RVT_GET_ELEM_PTR:
    offset = Mem::get_stack_offset_from_ident(value.dest);
    tmp_reg = Mem::load_from_stack(offset);
    ans = get_value_to_reg(value.value);
    func_seg<<"sw "+ans<<+", 0("+tmp_reg+")"<<std::endl;
    Mem::release_a_reg(tmp_reg);
    Mem::release_a_reg(ans);    
    break;
  case KOOPA_RVT_GET_PTR:
    offset = Mem::get_stack_offset_from_ident(value.dest);
    tmp_reg = Mem::load_from_stack(offset);
    ans = get_value_to_reg(value.value);
    func_seg<<"sw "+ans<<+", 0("+tmp_reg+")"<<std::endl;
    Mem::release_a_reg(tmp_reg);
    Mem::release_a_reg(ans);    
    break;    
  default:

    // 暂时还不能处理
    assert(false);
    break;
  }

}

void Visit(const koopa_raw_branch_t &value) {
  Visit(value.cond);
  auto cond = get_value_to_reg(value.cond);

  assert(value.true_bb!=NULL);
  assert(value.false_bb!=NULL);
  may_alloc_name_for_block(value.true_bb);
  may_alloc_name_for_block(value.false_bb);

  
  // func_seg<<"bnez "<<cond<<", "<<get_block_to_identname(value.true_bb)<<std::endl;
  // func_seg<<"j "<<get_block_to_identname(value.false_bb)<<std::endl;

  auto tmp_ident = get_a_new_blockname();
  func_seg<<"bnez "<<cond<<", "<<tmp_ident<<std::endl;
  func_seg<<"j "<<get_block_to_identname(value.false_bb)<<std::endl;
  func_seg<<tmp_ident<<":"<<std::endl;
  func_seg<<"j "<<get_block_to_identname(value.true_bb)<<std::endl;
  

  Mem::release_a_reg(cond);
}

void Visit(const koopa_raw_global_alloc_t &value) {

}


// return a Mem name
std::string Visit(const koopa_raw_binary_t &value)
{
  Visit(value.lhs);
  Visit(value.rhs);

  auto left = get_value_to_reg(value.lhs);
  auto right = get_value_to_reg(value.rhs);

  switch (value.op)
  {
  /// Not equal to.
  case KOOPA_RBO_NOT_EQ:

    func_seg<<"sub "<<left<<", "<<left<<", "<<right<<std::endl;
    func_seg<<"slt "<<right<<", "<<left<<", "<<Mem::zero_reg<<std::endl;
    func_seg<<"slt "<<left<<", "<<Mem::zero_reg<<", "<<left<<std::endl;
    func_seg<<"or "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Equal to.
  case KOOPA_RBO_EQ:

    func_seg<<"sub "<<left<<", "<<left<<", "<<right<<std::endl;
    func_seg<<"slt "<<right<<", "<<left<<", "<<Mem::zero_reg<<std::endl;
    func_seg<<"slt "<<left<<", "<<Mem::zero_reg<<", "<<left<<std::endl;
    func_seg<<"or "<<left<<", "<<left<<", "<<right<<std::endl;
    func_seg<<"xori "<<left<<", "<<left<<", "<<1<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Greater than.
  case KOOPA_RBO_GT:
    func_seg<<"slt "<<left<<", "<<right<<", "<<left<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Less than.
  case KOOPA_RBO_LT:
    func_seg<<"slt "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Greater than or equal to.
  case KOOPA_RBO_GE:
    func_seg<<"slt "<<left<<", "<<left<<", "<<right<<std::endl;
    func_seg<<"xori "<<left<<", "<<left<<", "<<1<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Less than or equal to.
  case KOOPA_RBO_LE:
    func_seg<<"slt "<<left<<", "<<right<<", "<<left<<std::endl;
    func_seg<<"xori "<<left<<", "<<left<<", "<<1<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Addition.
  case KOOPA_RBO_ADD:
    func_seg<<"add "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Subtraction.
  case KOOPA_RBO_SUB:
    func_seg<<"sub "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Multiplication.
  case KOOPA_RBO_MUL:
    func_seg<<"mul "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Division.
  case KOOPA_RBO_DIV:
    func_seg<<"div "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Modulo.
  case KOOPA_RBO_MOD:
    func_seg<<"rem "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Bitwise AND.
  case KOOPA_RBO_AND:
    func_seg<<"and "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Bitwise OR.
  case KOOPA_RBO_OR:
    func_seg<<"or "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Bitwise XOR.
  case KOOPA_RBO_XOR:
    func_seg<<"xor "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Shift left logical.
  case KOOPA_RBO_SHL:
    func_seg<<"sll "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Shift right logical.
  case KOOPA_RBO_SHR:
    func_seg<<"srl "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  /// Shift right arithmetic.
  case KOOPA_RBO_SAR:
    func_seg<<"sra "<<left<<", "<<left<<", "<<right<<std::endl;
    Mem::release_a_reg(right);
    break;
  default:
    assert(false);
    break;
  }
  
  return left;
}


std::string Visit(const koopa_raw_load_t &value) {
  Visit(value.src);
  auto ans = get_value_to_reg(value.src);
  if(value.src->kind.tag!=KOOPA_RVT_ALLOC) {
    func_seg<<"lw "+ans+", 0("+ans+")"<<std::endl;
  }
  
  return ans;
}


// return stack size before
int caller_save() {
  int ans = Mem::stack_size;

  Mem::alloc_on_stack(16*16);
  auto tmp_reg = Mem::must_require_a_reg();
  Mem::release_a_reg(tmp_reg);

  func_seg<<"li "<<tmp_reg<<", "<<-ans<<std::endl;
  func_seg<<"add "<<tmp_reg<<", fp, "<<tmp_reg<<std::endl;

  for(int i = 0;i<=7;i++) {
    auto str_i = std::to_string(i);
    auto off=std::to_string(-(i+1)*16);
    func_seg<<"sw a"+str_i+", "+off+"("+tmp_reg+")"<<std::endl;
  }

  for(int i =0;i<=6;i++) {
    auto str_i = std::to_string(i);
    auto off=std::to_string(-(i+1+8)*16);
    func_seg<<"sw t"+str_i+", "+off+"("+tmp_reg+")"<<std::endl;    
  }

  return ans;
}

void caller_restore(int stack_size_before_save, std::string except_reg) {
  int ans = stack_size_before_save;

  // warning be careful
  
  std::string tmp_reg = "a7";

  func_seg<<"li "<<tmp_reg<<", "<<-ans<<std::endl;
  func_seg<<"add "<<tmp_reg<<", fp, "<<tmp_reg<<std::endl;

  for(int i = 0;i<=7;i++) {
    auto str_i = std::to_string(i);
    auto off=std::to_string(-(i+1)*16);
    if("a"+str_i == except_reg)
      continue;
    if("a"+str_i == tmp_reg)
      continue;
    func_seg<<"lw a"+str_i+", "+off+"("+tmp_reg+")"<<std::endl;
  }

  for(int i =0;i<=6;i++) {
    auto str_i = std::to_string(i);
    auto off=std::to_string(-(i+1+8)*16);
    if("t"+str_i == except_reg)
      continue;
    if("t"+str_i == tmp_reg)
      continue;
    func_seg<<"lw t"+str_i+", "+off+"("+tmp_reg+")"<<std::endl;    
  } 

}

std::string Visit(const koopa_raw_call_t &value) {
  assert(value.args.kind==KOOPA_RSIK_VALUE);
  Visit(value.args);


  int stack_size_before_save = caller_save();



  for(int i = (int(value.args.len))-1;i>=0;i--) {

    if(i>=8) {
      auto tmp = get_value_to_reg(reinterpret_cast<koopa_raw_value_t>(value.args.buffer[i]));
      push_to_stack(tmp, func_seg);
      Mem::release_a_reg(tmp);
    } else {
      get_value_to_reg(reinterpret_cast<koopa_raw_value_t>(value.args.buffer[i]), "a"+std::to_string(i));
    }

  }


  auto ans = Mem::must_require_a_reg();
  may_alloc_name_for_func(value.callee);

  func_seg<<"call "<<get_func_to_identname(value.callee)<<std::endl;

  func_seg<<"add "<<ans<<", a0, x0"<<std::endl;

  caller_restore(stack_size_before_save, ans);
  return ans;
}

std::string Visit(const koopa_raw_get_elem_ptr_t &value) {
  assert(value.src->kind.tag==KOOPA_RVT_ALLOC ||
         value.src->kind.tag==KOOPA_RVT_GLOBAL_ALLOC ||
         value.src->kind.tag==KOOPA_RVT_GET_ELEM_PTR ||
         value.src->kind.tag==KOOPA_RVT_GET_PTR);
  if(value.src->kind.tag==KOOPA_RVT_ALLOC) {
    int offset = Mem::get_stack_offset_from_ident(value.src);
    int elem_sz = get_elem_size(value.src);
    auto off_reg = get_value_to_reg(value.index);

    auto tmp_reg = Mem::must_require_a_reg();

    // warning this may be wrong

    // 计算偏移量
    func_seg<<"li "+tmp_reg+", "+std::to_string(elem_sz)<<std::endl;
    func_seg<<"mul "+off_reg + ", " + off_reg + ", "+tmp_reg<<std::endl;

    // 计算首地址
    func_seg<<"li "+tmp_reg+", "+std::to_string(-offset)<<std::endl;
    func_seg<<"add "+tmp_reg + ", fp, "+tmp_reg<<std::endl;

    //得到地址
    func_seg<<"add "+off_reg+", "+tmp_reg+", "+off_reg<<std::endl;

    Mem::release_a_reg(tmp_reg);

    return off_reg;

  } else if(value.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    int elem_sz = get_elem_size(value.src);

    auto off_reg = get_value_to_reg(value.index);

    auto tmp_reg = Mem::must_require_a_reg();

    // warning this may be wrong

    // 计算偏移量
    func_seg<<"li "+tmp_reg+", "+std::to_string(elem_sz)<<std::endl;
    func_seg<<"mul "+off_reg + ", " + off_reg + ", "+tmp_reg<<std::endl;

    // 计算首地址
    may_alloc_name_for_global_var(value.src);
    func_seg<<"la "+tmp_reg+", "+get_global_value_to_identname(value.src)<<std::endl;

    //得到地址
    func_seg<<"add "+off_reg+", "+tmp_reg+", "+off_reg<<std::endl;

    Mem::release_a_reg(tmp_reg);

    return off_reg;    
  } else if(value.src->kind.tag==KOOPA_RVT_GET_ELEM_PTR) {
    // warning this may be wrong
    int elem_sz = get_elem_size(value.src);

    auto off_reg = get_value_to_reg(value.index);

    auto tmp_reg = Mem::must_require_a_reg();

    // warning this may be wrong

    // 计算偏移量
    func_seg<<"li "+tmp_reg+", "+std::to_string(elem_sz)<<std::endl;
    func_seg<<"mul "+off_reg + ", " + off_reg + ", "+tmp_reg<<std::endl;

    // 计算首地址
    int offset = Mem::get_stack_offset_from_ident(value.src);
    Mem::load_from_stack_to_reg(offset, tmp_reg);

    //得到地址
    func_seg<<"add "+off_reg+", "+tmp_reg+", "+off_reg<<std::endl;

    Mem::release_a_reg(tmp_reg);

    return off_reg;    
  } else if(value.src->kind.tag==KOOPA_RVT_GET_PTR) {
    // warning this may be wrong
    int elem_sz = get_elem_size(value.src);

    auto off_reg = get_value_to_reg(value.index);

    auto tmp_reg = Mem::must_require_a_reg();

    // warning this may be wrong

    // 计算偏移量
    func_seg<<"li "+tmp_reg+", "+std::to_string(elem_sz)<<std::endl;
    func_seg<<"mul "+off_reg + ", " + off_reg + ", "+tmp_reg<<std::endl;

    // 计算首地址
    int offset = Mem::get_stack_offset_from_ident(value.src);
    Mem::load_from_stack_to_reg(offset, tmp_reg);

    //得到地址
    func_seg<<"add "+off_reg+", "+tmp_reg+", "+off_reg<<std::endl;

    Mem::release_a_reg(tmp_reg);

    return off_reg;        
  }
  assert(false);
  return "";
}

std::string Visit(const koopa_raw_get_ptr_t &value) {
  //warning this may be wrong
  assert(value.src->kind.tag==KOOPA_RVT_FUNC_ARG_REF || 
         value.src->kind.tag == KOOPA_RVT_GET_PTR);
  assert(value.src->ty->tag==KOOPA_RTT_POINTER);
  int elem_sz = calc_variable_size_bytes(value.src->ty->data.pointer.base);

  std::string tmp_reg, off_reg;

  tmp_reg = Mem::must_require_a_reg();
  off_reg = get_value_to_reg(value.index);

  // 计算偏移量
  func_seg<<"li "+tmp_reg+", "+std::to_string(elem_sz)<<std::endl;
  func_seg<<"mul "+off_reg + ", " + off_reg + ", "+tmp_reg<<std::endl;

  // 计算首地址
  int offset = Mem::get_stack_offset_from_ident(value.src);
  Mem::load_from_stack_to_reg(offset, tmp_reg);

  //得到地址
  func_seg<<"add "+off_reg+", "+tmp_reg+", "+off_reg<<std::endl;

  Mem::release_a_reg(tmp_reg);

  return off_reg;
}

// warning: 注意text seg 的位置

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...