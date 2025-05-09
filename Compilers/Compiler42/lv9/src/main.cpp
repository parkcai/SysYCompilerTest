#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

//CRay增加引用
#include <fstream>
#include "AST.h"
#include "riscv.h"
#include "koopa.h"


using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
//CRay增加AST
//extern int yyparse(unique_ptr<string> &ast);
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
  //CRay  定义AST
  unique_ptr<BaseAST> ast;
  //调用语法分析器 yyparse，解析输入文件并生成抽象语法树（AST）
  auto ret = yyparse(ast);
  assert(!ret);

  //unique_ptr<CompUnitAST> comp_ast(dynamic_cast<CompUnitAST *>(ast.release()));

  // CRay按照测评要求输出
  koopa_raw_program_t raw = *(koopa_raw_program_t *)ast->to_koopa();
  //koopa_raw_program_t raw = *(koopa_raw_program_t *)comp_ast->to_koopa();
  ast.release();

  //CRay   -koopa
  if (string(mode) == "-koopa") {
    koopa_program_t program;
    //cout << "build koopa success" << endl;
    //调用 koopa_generate_raw_to_koopa，将 raw 转换为 program
    koopa_error_code_t eno = koopa_generate_raw_to_koopa(&raw, &program);
    if (eno != KOOPA_EC_SUCCESS) {
      cout << "generate raw to koopa error: " << (int)eno << endl;
      return 0;
    }
    //cout << "generate raw to koopa success" << endl;
    //fopen(output, "w");
    koopa_dump_to_file(program, output);
    koopa_delete_program(program);
  }
  //CRay  -riscv, 增加-perf参数
  if ((string(mode) == "-riscv")||(string(mode) == "-perf")) {
    koopa_program_t program;
    koopa_error_code_t eno = koopa_generate_raw_to_koopa(&raw, &program);
    if (eno != KOOPA_EC_SUCCESS) {
      cout << "generate raw to koopa error: " << (int)eno << endl;
      return 0;
    }
    size_t len = 1000000u;
    char *buf = new char[len];
    koopa_dump_to_string(program, buf, &len);
    koopa_delete_program(program);
    koopa_program_t kp;
    koopa_parse_from_string(buf, &kp);
    koopa_raw_program_builder_t builder_t = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder_t, kp);
    koopa_delete_program(kp);
    RISCV_Module riscv_module(output);
    riscv_module.raw_dump_to_riscv(raw);
  }
  return 0;
}
