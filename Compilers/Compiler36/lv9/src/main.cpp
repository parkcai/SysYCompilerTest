#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include "visit.h"
#include "koopa.h"
#include "ast.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[])
{
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  // 打开输出文件, 准备写入
  ofstream fout(output);
  assert(fout.is_open());
  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);
  // 打印 AST
  if (string(mode) == "-dump")
  {
    streambuf *oldcoutbuf = cout.rdbuf(fout.rdbuf());
    ast->Dump();
    cout.rdbuf(oldcoutbuf);
    fout.close();
    return 0;
  }
  // 生成 IR
  koopa_raw_program_t raw = *(koopa_raw_program_t *)ast->GenerateIR_ret();
  koopa_program_t program;
  koopa_error_code_t error = koopa_generate_raw_to_koopa(&raw, &program);
  if (error != KOOPA_EC_SUCCESS)
  {
    std::cout << "generate raw to koopa error: " << error << std::endl;
    return 0;
  }
  size_t len = 0;
  koopa_dump_to_string(program, nullptr, &len);
  char *buf = new char[len + 1];
  len++;
  error = koopa_dump_to_string(program, buf, &len);
  if (error != KOOPA_EC_SUCCESS)
  {
    std::cout << "dump to string error: " << error << std::endl;
    return 0;
  }
  koopa_parse_from_string(buf, &program);

  if (string(mode) == "-koopa")
  {
    koopa_dump_to_file(program, output);
    koopa_delete_program(program);
  }
  else if (string(mode) == "-riscv")
  {
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
    streambuf *oldcoutbuf = cout.rdbuf(fout.rdbuf());
    string ret = "";
    ret += Visit(raw);
    cout << ret;
    cout.rdbuf(oldcoutbuf);
    fout.close();
    koopa_delete_raw_program_builder(builder);
  }
  else if (string(mode) == "-perf")
  {
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
    streambuf *oldcoutbuf = cout.rdbuf(fout.rdbuf());
    string ret = "";
    ret += Visit(raw);
    cout << ret;
    cout.rdbuf(oldcoutbuf);
    fout.close();
    koopa_delete_raw_program_builder(builder);
  }
  return 0;
}
