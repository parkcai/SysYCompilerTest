#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include "ast.h"
#include "visit.h"
#include <unordered_map>
#include <set>
#include "my_var.h"
#include <stack>
#include <sstream>
#include "optim.h"


using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
std::string str;


std::unordered_map<string, MyVar> base_symbol_table;
std::vector<std::unordered_map<string, MyVar>> symbol_tables(1, base_symbol_table);
std::unordered_map<string, bool> block_end;
std::string now_block;
int block_num=0;
std::vector<std::string> nested_while_stack;
std::unordered_map<string, bool> has_ret;
std::string now_func;
std::unordered_map<string,bool> func_ret;
std::set<string> const_symbol;
int res_cnt=0;
int ptr_cnt=0;
int test_val ;

std::string riscv_code="";
std::vector<std::string> riscv_code_list;

int main(int argc, const char *argv[]) {
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  printf("Parsing...\n");
  auto ret = yyparse(ast);
  printf("Parsing done.\n");
  assert(!ret);


  if(strcmp(mode, "-ast") == 0){
    ast->Dump();
    cout<<endl;
  }
  else if(strcmp(mode, "-test") == 0){
    // ast->koopa_ir();
    // std::cout<<str<<std::endl;
    // fclose(stdout);
    // freopen(output, "w", stdout);
    printf("ast->koopa_ir();\n");
    ast->koopa_ir();
    printf("done.\n");
    std::cout<<str<<std::endl;
    auto ir = str.c_str();

    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(ir, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    // ...
    Visit(raw);

    std::cout<<riscv_code<<std::endl;

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  }
  else if(strcmp(mode, "-koopa") == 0){
    freopen(output, "w", stdout);
    ast->koopa_ir();
    std::cout<<str<<std::endl;
    fclose(stdout);
    // std::cout<<"fun @main(): i32 {\n%entry:\n\tjump %while_entry_0\n%while_entry_0:\n\tbr 1, %while_body_0, %while_end_0\n%while_body_0:\n\tjump %while_entry_0\n%while_end_0:\n\tret 0\n}\n";
  }
  else if(strcmp(mode, "-riscv") == 0){
    freopen(output, "w", stdout);
    ast->koopa_ir();
    // std::cout<<str<<std::endl;
    auto ir = str.c_str();

    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(ir, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    // ...
    Visit(raw);
    // std::cout<<riscv_code<<std::endl;
    std::istringstream stream(riscv_code);
    std::string line;
    while(std::getline(stream, line)) riscv_code_list.push_back(line);
    optimize_riscv_code();
    for(auto i: riscv_code_list) std::cout<<i<<std::endl;

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  }
    else if(strcmp(mode, "-perf") == 0){
    freopen(output, "w", stdout);
    ast->koopa_ir();
    // std::cout<<str<<std::endl;
    auto ir = str.c_str();

    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(ir, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    // ...
    Visit(raw);
    // std::cout<<riscv_code<<std::endl;
    std::istringstream stream(riscv_code);
    std::string line;
    while(std::getline(stream, line)) riscv_code_list.push_back(line);
    optimize_riscv_code();
    for(auto i: riscv_code_list) std::cout<<i<<std::endl;

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  }
  else{
    cout<<"Unknown mode: "<<mode<<endl;
  }

  return 0;
}

