#pragma once
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <queue>
#include "koopa.h"
using namespace std;
const char* optostr[] = {
    "", "", "sgt", "slt", "", "",
    "add", "sub", "mul", "div",
    "rem", "and", "or", "xor",
    "sll", "srl", "sra"
};

void sp_op(std::ostream& out, int length){
  if ((length >=-2048) && (length <=2047)){
    out<<"  addi  sp, sp, " <<length<<"\n";
  }
  else{
    out<<"  li    t0, " <<length<<"\n";
    out<<"  add   sp, sp, t0\n";
  }
};

void store_op(std::ostream& out, const std::string &r1, const std::string &r2, int off){
  if ((off >=-2048) && (off <=2047)){
    out<<"  sw    "<<r1<<", "<<off<<"("<<r2<<")\n";
  }
  else{
    out<< "  li    t3, " <<off<<"\n";
    out<<"  add   t3, t3, "<<r2<<"\n";
    out<<"  sw    "<<r1<<", 0(t3)\n";
  }
};

void load_op(std::ostream& out, const std::string &r1, const std::string &r2, int off){
  if ((off >=-2048) && (off <=2047)){
    out<<"  lw    "<<r1<<", "<<off<<"("<<r2<<")\n";
  }
  else{
    out<<"  li    t3, " <<off<<"\n";
    out<<"  add   t3, t3, "<<r2<<"\n";
    out<<"  lw    "<<r1<<", 0(t3)\n";
  }
};

size_t get_size(koopa_raw_type_t type){
    if (type->tag == KOOPA_RTT_INT32){
      return 4;
    }
    else if (type->tag == KOOPA_RTT_ARRAY){
      return type->data.array.len * get_size(type->data.array.base);
    }
    else if (type->tag == KOOPA_RTT_POINTER){
      return 4;
    }
    return 0;
}

struct reginfo{
  bool alloc;
  koopa_raw_value_t val;
};

struct valreginfo{
  bool valid;
  std::string reg;
};

class StackFrameAllocator{
  public:
    size_t R,A,S;
    size_t Align_S;
    unordered_map<koopa_raw_value_t, size_t>val_off;
  void reset(){
    R=0;
    A=0;
    S=0;
    Align_S=0;
    val_off.clear();
  }
  void print_sfa(){
    cout<<"S:"<<S<<endl;
  }
  void sfalloc(koopa_raw_value_t value, size_t size){
    val_off.insert(make_pair(value,S));
    S += size;
  }
  void set_R(){
    R = 4;
  }
  void set_A(size_t a){
    if (a>A){
      A=a;
    }
  }
  void align(){
    Align_S = 16*( (S+R+A+15)/16 );
  }


  void funcalloc(const koopa_raw_function_t& func){
    for(int i = 0; i < func->bbs.len; i++){
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        for(int j = 0; j < bb->insts.len; j++){
            auto value = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if(value->kind.tag == KOOPA_RVT_ALLOC ){
                int size = get_size(value->ty->data.pointer.base);
                this->sfalloc(value, size);
                //std::cout<<value<<":"<<this->val_off[value]<<endl;
                //cout<<func<<endl;
                continue;
            }
            else if(value->kind.tag == KOOPA_RVT_CALL){
              this->set_R();
              int extra_param = (int)value->kind.data.call.args.len-8;
              if (extra_param>0){
                this->set_A(extra_param*4);
              }
              size_t size = get_size(value->ty);
              if(size){
                this->sfalloc(value, size);
              }
            }
            else{
              size_t size = get_size(value->ty);
              if(size){
                  this->sfalloc(value, size);
              }
            }
            
        }
    }
  }
};
extern int Branch_Index;
extern StackFrameAllocator sfa;
extern koopa_raw_function_t func_now;
class RegisterAllocator{
  public:
    static const int totalRegisters =11;
    std::unordered_map<std::string, struct reginfo> availableRegisters; // 寄存器及其可用状态
    std::queue<std::string> allocatedRegisters;
    std::unordered_map<koopa_raw_value_t, struct valreginfo> var2reg;
  void reset() {
        // 初始化所有寄存器为可用
        while(!allocatedRegisters.empty()){
          allocatedRegisters.pop();
        }
        var2reg.clear();
        for (int i = 1; i < totalRegisters+1; ++i) {
            std::string regName = "s" + std::to_string(i);
            struct reginfo reg_info;
            reg_info.alloc = 0;
            availableRegisters[regName] = reg_info;
        }
    }
  int isalloc(koopa_raw_value_t value){
    if(var2reg.find(value)!=var2reg.end()){
      if (var2reg[value].valid) return 1;
    }
    return 0;
  }
  std::string allocateRegister(koopa_raw_value_t value, std::ostream& out) {
      if(!this->isalloc(value))  
        {for (auto& reg : availableRegisters) {
            if (!reg.second.alloc) { // 如果找到空闲寄存器
                reg.second.alloc = 1; // 标记为已使用
                reg.second.val =value;
                allocatedRegisters.push(reg.first); // 加入队列
                struct valreginfo info;
                info.valid = 1;
                info.reg = reg.first;
                var2reg[value]=info;
                return reg.first;
            }
        }
        std::string reusedReg = allocatedRegisters.front();
        koopa_raw_value_t val = availableRegisters[reusedReg].val;
        availableRegisters[reusedReg].val= value;
        store_op(out,reusedReg,"sp",sfa.val_off[val]+sfa.A);
        var2reg[val].valid = 0;
        allocatedRegisters.pop();
        allocatedRegisters.push(reusedReg); // 把这个寄存器重新放入队尾
        struct valreginfo info;
        info.valid = 1;
        info.reg = reusedReg;
        var2reg[value]=info;
        return reusedReg;}
      else{
        return var2reg[value].reg;
      }
  }
  void saveallRegister(std::ostream& out){
    for(auto& reg: availableRegisters){
      if(reg.second.alloc){
        store_op(out,reg.first,"sp",sfa.val_off[reg.second.val]+sfa.A);
      }
    }
    this->reset();
  }

};
extern RegisterAllocator rega;
void Visit(const koopa_raw_program_t &program, std::ostream& out);
void Visit(const koopa_raw_slice_t &slice, std::ostream& out);
void Visit(const koopa_raw_function_t &func, std::ostream& out);
void Visit(const koopa_raw_basic_block_t &bb, std::ostream& out);
void Visit(const koopa_raw_value_t &value, std::ostream& out);
void Visit(const koopa_raw_return_t &ret, std::ostream& out);
int Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary,const koopa_raw_value_t &value, std::ostream& out);
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value,std::ostream& out);
void Visit(const koopa_raw_store_t &store, std::ostream& out);
void Visit(const koopa_raw_branch_t &branch, std::ostream& out);
void Visit(const koopa_raw_jump_t &jump, std::ostream& out);
void Visit(const koopa_raw_call_t &call, std::ostream& out);
void Visit_Global(const koopa_raw_value_t &value, std::ostream& out);
void Visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr, const koopa_raw_value_t &value,std::ostream& out);
void Visit(const koopa_raw_get_ptr_t& get_ptr, const koopa_raw_value_t &value,std::ostream& out);
void my_init_array(const koopa_raw_value_t &value, std::ostream& out);

void KoopatoRiscv(FILE *in, std::ostream& out) {
    // 移动文件指针到文件末尾
    fseek(in, 0, SEEK_END);
    // 获取文件大小
    long fileSize = ftell(in);
    // 移动文件指针回文件开始
    rewind(in);
    // 分配内存以容纳整个文件
    char *str = (char *)malloc(fileSize + 1);
    //memset(str, 0, fileSize * sizeof(char));
    //fseek(in, 0, SEEK_SET); 
    if (str == NULL) {
        std::cerr << "内存分配失败" << std::endl;
    }

    // 读取文件
    size_t bytesRead = fread(str, sizeof(char), fileSize, in);
    if (bytesRead < fileSize) {
        std::cerr << "文件读取错误" << std::endl;
    }
    str[bytesRead] = '\0';  // 确保字符串终止
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    Visit(raw,out);
    out<<endl;
    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
};

// 函数声明略
// ...

// 访问 raw program
void Visit(const koopa_raw_program_t &program, std::ostream& out) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values, out);
  // 访问所有函数
  Visit(program.funcs, out);
};

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice, std::ostream& out) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        func_now=reinterpret_cast<koopa_raw_function_t>(ptr);
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr),out);
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr),out);
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr),out);
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
};

// 访问函数
void Visit(const koopa_raw_function_t &func, std::ostream& out) {
  // 执行一些其他的必要操作
  if(func->bbs.len == 0) return;
  out<<"\n  .text\n";
  out<<"  .globl " << string(func->name + 1) << "\n";
  out<<string(func->name + 1)+ ":\n";
  sfa.reset();
  sfa.funcalloc(func);
  sfa.align();
  if (sfa.Align_S){
    sp_op(out,-sfa.Align_S);
  }
  if (sfa.R){
    store_op(out,"ra", "sp", sfa.Align_S - 4);
  }
  rega.reset();
  size_t entry_id = 0;
    for(entry_id = 0; entry_id < func->bbs.len; ++entry_id){
        auto ptr = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[entry_id]);
        if(ptr->name && !strcmp(ptr->name,"%entry")){
            break;
        }
    }
    // 访问 entry block
    Visit(reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[entry_id]),out);
    //访问其他block
    for(size_t i = 0; i < func->bbs.len; ++i){
        if(i == entry_id) continue;
        auto ptr = func->bbs.buffer[i];
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr),out);
    }
};

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb, std::ostream& out) {
  // 执行一些其他的必要操作
  rega.reset();
 if(bb->name && strcmp(bb->name, "%entry")){
        out<<string(bb->name+1)<<":\n";
    }
  for(size_t i = 0; i < bb->insts.len; ++i){
        auto ptr = bb->insts.buffer[i];
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr),out);
  }
};

// 访问指令
void Visit(const koopa_raw_value_t &value, std::ostream& out) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret,out);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
        Visit(kind.data.binary,value,out);
        //store_op(out, "t0", "sp", sfa.val_off[value]+sfa.A);
        break;
    case KOOPA_RVT_ALLOC:
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load,value,out);
      //store_op(out, "t0", "sp", sfa.val_off[value]+sfa.A);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store,out);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch,out);
      break;
    case KOOPA_RVT_JUMP:
      rega.saveallRegister(out);
      Visit(kind.data.jump,out);
      break;
    case KOOPA_RVT_CALL:
      Visit(kind.data.call,out);
      if(kind.data.call.callee->ty->data.function.ret->tag == KOOPA_RTT_INT32){
        if(rega.isalloc(value)){
          out<<"  mv    a0, "<<rega.var2reg[value].reg<<"\n";
        }
        else{
          store_op(out,"a0", "sp", sfa.val_off[value]+sfa.A);
        }
      }
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit_Global(value,out);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr,value, out);
      //store_op(out,"t0","sp",sfa.val_off[value]+sfa.A);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr,value,out);
      //store_op(out,"t0","sp",sfa.val_off[value]+sfa.A);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
};

//访问return
void Visit(const koopa_raw_return_t &ret, std::ostream& out) {
    if(ret.value){
      if(ret.value->kind.tag == KOOPA_RVT_INTEGER){
        int i = Visit(ret.value->kind.data.integer);
        out<<"  li    a0, "<<i<< "\n";
      } 

      else{
        if(rega.isalloc(ret.value)){
          out<<"  mv    a0, "<<rega.var2reg[ret.value].reg<<"\n";
        }
        else{
          load_op(out,"a0","sp",sfa.val_off[ret.value]+sfa.A);
        }
        //load_op(out,"a0","sp",sfa.val_off[ret.value]+sfa.A);
      }
    }
    if (sfa.R){
      load_op(out, "ra","sp",sfa.Align_S-4);
    }
    if (sfa.Align_S){
      sp_op(out, sfa.Align_S);
    }
    out<< "  ret\n";
};

//访问integer
int Visit(const koopa_raw_integer_t &integer) {
    return integer.value;
};

void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value, std::ostream& out) {
    auto op=binary.op;
    auto left=binary.lhs;
    auto right=binary.rhs;
    int lflag=0;
    int rflag=0;
    int imm;
    std::string outreg= rega.allocateRegister(value,out);
    std::string lreg;
    std::string rreg;

    if(left->kind.tag == KOOPA_RVT_INTEGER){
      int i = Visit(left->kind.data.integer);
      if (i>=-2048&&i<=2047)
        {lflag=1;
        imm = i;}
      else{
        //lreg = rega.allocateRegister(left,out);
        lreg = "t0";
        out<<"  li    "<<lreg<<", "<<i<<"\n";
      }
    }
    else{
      if (rega.isalloc(left)){
        lreg = rega.var2reg[left].reg;
      }
      else
      {lreg = rega.allocateRegister(left,out);
      load_op(out, lreg, "sp", sfa.val_off[left]+sfa.A);}
    }
    if(right->kind.tag == KOOPA_RVT_INTEGER){
      int i = Visit(right->kind.data.integer);
      if (i>=-2048&&i<=2047&&lflag==0)
        {rflag=1;
        imm=i;}
      else{
        //rreg = rega.allocateRegister(right,out);
        rreg ="t1";
        out<<"  li    "<<rreg<<", "<<i<<"\n";
      }
    }
    else{
      if(rega.isalloc(right)){
        rreg = rega.var2reg[right].reg;
      }
      else
      {rreg = rega.allocateRegister(right,out);
      load_op(out, rreg, "sp", sfa.val_off[right]+sfa.A);}
    }
    if(op == KOOPA_RBO_EQ){
        
        if(lflag){
          out<<"  xori  "<<outreg<<", "<<rreg<<", "<<imm<<"\n";
        }
        else if (rflag){
          out<<"  xori  "<<outreg<<", "<<lreg<<", "<<imm<<"\n";
        }
        else{
          out<< "  xor   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
        }
        out<< "  seqz  "<<outreg<<", "<<outreg<<"\n";
    }
    else if(op == KOOPA_RBO_NOT_EQ){
        if(lflag){
          out<<"  xori  "<<outreg<<", "<<rreg<<", "<<imm<<"\n";
        }
        else if (rflag){
          out<<"  xori  "<<outreg<<", "<<lreg<<", "<<imm<<"\n";
        }
        else{
          out<< "  xor   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
        }
        out<<"  snez  "<<outreg<<", "<<outreg<<"\n";
    }
    else if(op == KOOPA_RBO_GE){
        if(lflag){
          //lreg = rega.allocateRegister(left,out);
          lreg = "t0";
          out<<"  li    "<<lreg<<", "<<imm<<"\n";
        }
        else if(rflag){
          //rreg = rega.allocateRegister(right,out);
          rreg = "t1";
          out<<"  li    "<<rreg<<", "<<imm<<"\n";
        }
        out<<"  slt   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
        out<<"  seqz  "<<outreg<<", "<<outreg<<"\n";
    }
    else if(op == KOOPA_RBO_LE){
        if(lflag){
          //lreg = rega.allocateRegister(left,out);
          lreg = "t0";
          out<<"  li    "<<lreg<<", "<<imm<<"\n";
        }
        else if(rflag){
          //rreg = rega.allocateRegister(right,out);
          rreg = "t1";
          out<<"  li    "<<rreg<<", "<<imm<<"\n";
        }
        out<<"  sgt   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
        out<<"  seqz  "<<outreg<<", "<<outreg<<"\n";
    }
    else if(op == KOOPA_RBO_ADD){
      if(lflag){
        out<<"  addi  "<<outreg<<", "<<rreg<<", "<<imm<<"\n";
      }
      else if(rflag){
        out<<"  addi  "<<outreg<<", "<<lreg<<", "<<imm<<"\n";
      }
      else{
        out<<"  add  "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
      }
      
    }
    else if(op == KOOPA_RBO_OR){
      if(lflag){
        out<<"  ori  "<<outreg<<", "<<rreg<<", "<<imm<<"\n";
      }
      else if(rflag){
        out<<"  ori  "<<outreg<<", "<<lreg<<", "<<imm<<"\n";
      }
      else{
        out<<"  or   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
      }
      
    }
    else if(op == KOOPA_RBO_XOR){
      if(lflag){
        out<<"  xori  "<<outreg<<", "<<rreg<<", "<<imm<<"\n";
      }
      else if(rflag){
        out<<"  xori  "<<outreg<<", "<<lreg<<", "<<imm<<"\n";
      }
      else{
        out<<"  xor   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
      }
      
    }
    else{
        if(lflag){
          //lreg = rega.allocateRegister(left,out);
          lreg = "t0";
          out<<"  li    "<<lreg<<", "<<imm<<"\n";
        }
        else if(rflag){
          //rreg = rega.allocateRegister(right,out);
          rreg = "t1";
          out<<"  li    "<<rreg<<", "<<imm<<"\n";
        }
        out<<"  "<<optostr[(int)op]<<"   "<<outreg<<", "<<lreg<<", "<<rreg<<"\n";
    }
};

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t& value, std::ostream& out){
    koopa_raw_value_t src = load.src;
    std::string outreg= rega.allocateRegister(value,out);

    if(src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
 
      out<<"  la    "<<outreg<<", "<<string(src->name + 1)<<"\n";
      load_op(out,outreg, outreg, 0);
    } 
    else if(src->kind.tag == KOOPA_RVT_ALLOC){
      if(rega.isalloc(src)){
        out<<"  mv    "<<outreg<<", "<<rega.var2reg[src].reg<<"\n";
      }
      else{
        load_op(out,outreg, "sp", sfa.val_off[src]+sfa.A);
      }
      //load_op(out,outreg, "sp", sfa.val_off[src]+sfa.A);
    } 
    else{
      if(rega.isalloc(src)){
        out<<"  mv    "<<outreg<<", "<<rega.var2reg[src].reg<<"\n";
      }
      else{
        load_op(out,outreg, "sp", sfa.val_off[src]+sfa.A);
      }
      load_op(out,outreg, outreg, 0);
    }
};

void Visit(const koopa_raw_store_t &store, std::ostream& out){
    koopa_raw_value_t value = store.value;
    koopa_raw_value_t dest = store.dest;
    std::string valuereg;
    std::string destreg;
    //params
    if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF){
      int index=0;
      for ( ; index<(func_now->params.len);index++){
        if (func_now->params.buffer[index]==value)
          break;
      }
      if(index<8){
        if(dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
          out<<"  la   t0 "<<string(dest->name +1)<<"\n";
          store_op(out,"a"+to_string(index),"t0",0);
        }
        else if (dest->kind.tag == KOOPA_RVT_ALLOC){
          //cout<<func_now<<endl;
          //cout<<value<<":"<<sfa.val_off[value]<<endl;
          destreg = rega.allocateRegister(dest,out);
          out<<"  mv    "<<destreg<<", a"<<index<<"\n";
          //store_op(out,"a"+to_string(index),"sp",sfa.val_off[dest]+sfa.A);
        }
        else{
          if(rega.isalloc(dest)){
            destreg = rega.allocateRegister(dest,out);
          }
          else{
            destreg = rega.allocateRegister(dest,out);
            //load_op(out, destreg, "sp", sfa.val_off[dest]+sfa.A);
          }
          store_op(out, "a"+to_string(index),destreg, 0);
        }
        return;
      }
      else{
        int off = (index-8)*4;
        load_op(out,"t0","sp",off+sfa.Align_S);
        valuereg="t0";
      }
    }
    
    else if(value->kind.tag == KOOPA_RVT_INTEGER){
        out<<"  li    t0, "<<Visit(value->kind.data.integer)<<"\n";
        valuereg="t0";
    } 
    else{
        if(rega.isalloc(value)){
          valuereg=rega.allocateRegister(value,out);
        }
        else{
          valuereg=rega.allocateRegister(value,out);
          load_op(out, valuereg, "sp", sfa.val_off[value]+sfa.A);
        }
    }

    if(dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        out<<"  la    t1, "<<string(dest->name + 1)<<"\n";
        store_op(out,valuereg, "t1", 0);
    } else if(dest->kind.tag == KOOPA_RVT_ALLOC){
      if(rega.isalloc(dest)){
        destreg = rega.allocateRegister(dest,out);
        out<<"  mv    "<<destreg<<", "<<valuereg<<"\n";
      }
      else{
        destreg = rega.allocateRegister(dest,out);
        out<<"  mv    "<<destreg<<", "<<valuereg<<"\n";
      }
    } else {
      if(rega.isalloc(dest)){
        destreg = rega.allocateRegister(dest,out);
      } 
      else{
        destreg = rega.allocateRegister(dest,out);
      }
        store_op(out,valuereg,destreg, 0);
    }
    return;
}

void Visit(const koopa_raw_jump_t &jump, std::ostream& out){
  out<<"  j     "<<string(jump.target->name +1)<<"\n";
  return;
}

void Visit(const koopa_raw_branch_t &branch, std::ostream& out){
  koopa_raw_value_t value = branch.cond;
  std::string valuereg;
  if (value->kind.tag == KOOPA_RVT_INTEGER){
    out<<"  li    t0, "<<Visit(value->kind.data.integer)<<"\n";
    valuereg = "t0";
  }
  else{
    if(rega.isalloc(value)){
      valuereg = rega.allocateRegister(value,out);
    }
    else{
      //valuereg = rega.allocateRegister(value,out);
      load_op(out,"t0","sp",sfa.val_off[value]+sfa.A);
      valuereg="t0";
    }
  }
  rega.saveallRegister(out);
  std::string branch_ = "branch_"+to_string(Branch_Index);
  Branch_Index+=1;
  out<<"  bnez  "<<valuereg<<", "<<branch_<<"\n";
  out<<"  j     "<<string(branch.false_bb->name +1)<<"\n";
  out<<branch_<<":\n";
  out<<"  j     "<<string(branch.true_bb->name+1)<<"\n";
  return;

}

void Visit(const koopa_raw_call_t &call, std::ostream& out){
    for(int i = 0; i < call.args.len; i++){
        koopa_raw_value_t value = (koopa_raw_value_t)call.args.buffer[i];
        if(value->kind.tag == KOOPA_RVT_INTEGER){
            int j = Visit(value->kind.data.integer);
            if(i < 8){
              out<<"  li    a"<<i<<", "<<j<<"\n";
            } else {
              out<<"  li    t0, "<<j<<"\n";
              store_op(out, "t0","sp",(i-8)*4);
            }
        } else{
            if(i < 8){
              if(rega.isalloc(value)){
                out<<"  mv    "<<"a"+to_string(i)<<", "<<rega.var2reg[value].reg<<"\n";
              }
              else {
                load_op(out, ("a"+to_string(i)),"sp",sfa.val_off[value]+sfa.A);
              }
              
            } else {
              if(rega.isalloc(value)){
                store_op(out,rega.var2reg[value].reg,"sp",(i-8)*4);
              }
              else{
                load_op(out,"t0","sp",sfa.val_off[value]+sfa.A);
                store_op(out,"t0","sp",(i-8)*4);
              }
              
            }
        }
    }
    rega.saveallRegister(out);
    out<<"  call  "<<string(call.callee->name +1)<<"\n";

    return;
}

void Visit_Global(const koopa_raw_value_t &value, std::ostream& out){
  out<<"\n  .data\n";
  out<<"  .globl "<<string(value->name+1)<<"\n";
  out<<string(value->name+1)<<":\n";
  auto type = value->ty->data.pointer.base;
  if(type->tag == KOOPA_RTT_INT32){
    if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
      out<<"  .zero 4\n";
    }
    else {
      int retn = Visit(value->kind.data.global_alloc.init->kind.data.integer);
      out<<"  .word "<<retn<<"\n";
    }
  }
  else{
    if(value->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
      out<<"  .zero "<<get_size(type)<<"\n";
    }
    else {
      my_init_array(value->kind.data.global_alloc.init,out);
    }
  }
  return;
}

void my_init_array(const koopa_raw_value_t &value, std::ostream& out){
  if(value->kind.tag == KOOPA_RVT_INTEGER){
    int val = Visit(value->kind.data.integer);
    out<<"  .word "<<val<<"\n";
  }
  else{
    auto elems = value->kind.data.aggregate.elems;
    for (int i =0;i<elems.len;i++){
      my_init_array(reinterpret_cast<koopa_raw_value_t>(elems.buffer[i]),out);
    }
  }
}

void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value, std::ostream& out){
  koopa_raw_value_t array = get_elem_ptr.src;
  koopa_raw_value_t offlist = get_elem_ptr.index;
  //std::string base;
  std::string arrayreg;
  std::string basereg;
  std::string indexreg;
  std::string outreg = rega.allocateRegister(value,out);
  int size = get_size(array->ty->data.pointer.base->data.array.base);
  if(array->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    out<<"  la    t0, "<<string(array->name +1)<<"\n";
    basereg = "t0";
  }
  else if (array->kind.tag == KOOPA_RVT_ALLOC){
    int off = sfa.val_off[array]+sfa.A;
    if (off>=-2048 && off<=2047){
      out<<"  addi  t0, sp, "<<off<<"\n";

    }
    else{
      out<<" li    t0, "<<off<<"\n";
      out<<"  add   t0, sp, t0\n";
    }
    basereg = "t0";
  }
  else{
    if(rega.isalloc(array)){
      arrayreg = rega.allocateRegister(array,out);
    }
    else{
      arrayreg = rega.allocateRegister(array,out);
      load_op(out,arrayreg,"sp",sfa.val_off[array]+sfa.A);
    }
    basereg = arrayreg;
  }
  if (offlist->kind.tag == KOOPA_RVT_INTEGER){
    int val=Visit(offlist->kind.data.integer);
    out<<"  li    t1, "<<val<<"\n";
    indexreg = "t1";
  }
  else {
    if(rega.isalloc(offlist)){
      indexreg = rega.allocateRegister(offlist,out);
    }
    else if (offlist->kind.tag !=KOOPA_RVT_GLOBAL_ALLOC){
      indexreg = rega.allocateRegister(offlist,out);
      load_op(out, indexreg, "sp",sfa.val_off[offlist]+sfa.A);
    }
    else{
      load_op(out, "t1", "sp",sfa.val_off[offlist]+sfa.A);
      indexreg = "t1";
    }
  }
  out<<"  li    t2, "<<size<<"\n";
  out<<"  mul   "<<indexreg<<", "<<indexreg<<", t2\n";
  out<<"  add   "<<outreg<<", "<<basereg<<", "<<indexreg<<"\n";
}

void Visit(const koopa_raw_get_ptr_t &get_ptr,const koopa_raw_value_t &value, std::ostream& out){
  koopa_raw_value_t array = get_ptr.src;
  koopa_raw_value_t offlist = get_ptr.index;
  std::string arrayreg;
  std::string basereg;
  std::string indexreg;
  std::string outreg = rega.allocateRegister(value,out);
  int size = get_size(array->ty->data.pointer.base);
  if(array->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    out<<"  la    t0, "<<string(array->name +1)<<"\n";
    basereg = "t0";
  }
  else if (array->kind.tag == KOOPA_RVT_ALLOC){
    int off = sfa.val_off[array]+sfa.A;
    if (off>=-2048 && off<=2047){
      out<<"  addi  t0, sp, "<<off<<"\n";

    }
    else{
      out<<" li    t0, "<<off<<"\n";
      out<<"  add   t0, sp, t0\n";
    }
    basereg = "t0";
  }
  else{
    if(rega.isalloc(array)){
      arrayreg = rega.allocateRegister(array,out);
    }
    else{
      arrayreg = rega.allocateRegister(array,out);
      load_op(out,arrayreg,"sp",sfa.val_off[array]+sfa.A);
    }
    basereg = arrayreg;
  }
  if (offlist->kind.tag == KOOPA_RVT_INTEGER){
    int val=Visit(offlist->kind.data.integer);
    out<<"  li    t1, "<<val<<"\n";
    indexreg = "t1";
  }
  else {
     if(rega.isalloc(offlist)){
      indexreg = rega.allocateRegister(offlist,out);
    }
    else if (offlist->kind.tag !=KOOPA_RVT_GLOBAL_ALLOC){
      indexreg = rega.allocateRegister(offlist,out);
      load_op(out, indexreg, "sp",sfa.val_off[offlist]+sfa.A);
    }
    else{
      load_op(out, "t1", "sp",sfa.val_off[offlist]+sfa.A);
      indexreg = "t1";
    }
  }
  out<<"  li    t2, "<<size<<"\n";
  out<<"  mul   "<<indexreg<<", "<<indexreg<<", t2\n";
  out<<"  add   "<<outreg<<", "<<basereg<<", "<<indexreg<<"\n";
}
// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...