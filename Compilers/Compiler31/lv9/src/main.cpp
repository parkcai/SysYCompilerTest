#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include "ast_def.h"
#include "riscv/riscv.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(ptr &ast);

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
  ptr ast;
  auto ret = yyparse(ast);
  assert(!ret);
  auto raw = *(koopa_raw_program_t *)ast->to_koopa();
  // ast.release();
  if (str(mode) == "-koopa"){
    koopa_program_t program;
    koopa_error_code_t eno = koopa_generate_raw_to_koopa(&raw, &program);
    if (eno != KOOPA_EC_SUCCESS) {
      std::cout << "generate raw to koopa error: " << (int)eno << std::endl;
      return 0;
    }
    koopa_dump_to_file(program, output);
    koopa_delete_program(program);
  } else {
    koopa_program_t program;
    koopa_error_code_t eno = koopa_generate_raw_to_koopa(&raw, &program);
    if (eno != KOOPA_EC_SUCCESS) {
      std::cout << "generate raw to koopa error: " << (int)eno << std::endl;
      return 0;
    }
    size_t len = 5000000;
    char buffer[len];
    koopa_dump_to_string(program, buffer, &len);
    koopa_parse_from_string(buffer, &program);
    raw = koopa_build_raw_program(koopa_new_raw_program_builder(), program);
    RISCV_builder riscv_builder;
    /* 把riscv_builder.build(raw)返回的string输出到output文件中 */
    ofstream outfile(output);
    outfile << riscv_builder.build(raw);
    koopa_delete_program(program);
  }
  ast->dump();
  return 0;
}
