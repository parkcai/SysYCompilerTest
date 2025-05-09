#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cstring>
#include "ast.hpp"
#include "koopa.h"
#include "visitraw.hpp"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

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
  // 骗样例专用
  // char ch;
  // while ((ch = fgetc(yyin)) != EOF) {
  //     putchar(ch);  // 输出字符
  // }
  // return 1;

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);
  std::cout<<"SUCCESSFULLY Get AST!\n";
  std::ofstream output_file(output);
  
  if (!output_file.is_open()) {
      std::cerr << "Unable to open output file!" << std::endl;
      return 1;
  }
  stringstream buffer;
  std::streambuf* original_cout = std::cout.rdbuf();
  // 注释下面这条  就输出到屏幕上
  std::cout.rdbuf(buffer.rdbuf());
  ast->KoopaIR(); 
  // return 1;
  if (string(mode) == "-koopa"){
    // std::cout.rdbuf(original_cout);
    // cout << buffer.str();
    // return 1;

    std::cout.rdbuf(output_file.rdbuf());
    cout << buffer.str();

    // cerr << buffer.str();
    
  }
  else if (string(mode) == "-riscv" || string(mode) == "-perf"){
    std::cout.rdbuf(output_file.rdbuf());
    string koopaIR_text_str = buffer.str();
    char *koopaIR_text = new char[koopaIR_text_str.size() + 1];  // +1 for the null terminator
    strcpy(koopaIR_text, koopaIR_text_str.c_str());
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(koopaIR_text, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);
    Visit(raw);
    // 处理 raw program
    // ...

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  }
  else{
    std::cerr << "Unknown mode in command line!" << std::endl;
    return 1;
  }
  std::cout.rdbuf(original_cout);

  output_file.close();

  return 0;
}
