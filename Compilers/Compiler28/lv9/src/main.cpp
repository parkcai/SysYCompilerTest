#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.h"
#include "visit.h"

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

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  // 输出解析得到的 AST
  //ast->Dump();
  cout<<endl;

  // 将 AST 转换为 IR
  koopa_raw_program_t *raw_program = (koopa_raw_program_t *)ast->toKoopaIR();

  koopa_program_t inter_program, program;
  koopa_error_code_t eno = koopa_generate_raw_to_koopa(raw_program, &inter_program);
  if (eno != KOOPA_EC_SUCCESS) {
    std::cout << "generate raw to koopa error: " << (int)eno << std::endl;
    return 0;
  }

  //输出到文件
  if(std::string(mode) == "-koopa") {
    koopa_dump_to_file(inter_program, output);
    koopa_delete_program(inter_program);
  }
  else if(std::string(mode) == "-riscv" || std::string(mode) == "-perf") 
  {
    size_t len = 1000000u;
    char *buf = new char[len];
    koopa_dump_to_string(inter_program, buf, &len);
    koopa_delete_program(inter_program);
    koopa_parse_from_string(buf, &program);
    koopa_raw_program_builder_t raw_builder= koopa_new_raw_program_builder();
    koopa_raw_program_t Raw_Program = koopa_build_raw_program(raw_builder, program);
    koopa_delete_program(program);
    std::ofstream file(output);
    if (file.is_open()) {
      Visit(Raw_Program, file);
      file.close();
    }

  }
  std::cout<<"stored in "<<output<<"\n";

  return 0;
}
