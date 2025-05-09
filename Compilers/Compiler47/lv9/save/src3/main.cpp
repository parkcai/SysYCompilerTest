#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "AST.h"
#include "../debug/debug.h"
#include "koopa.h"
#include "visit.h"
#include "string.h"
using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

extern ofstream debug;

ofstream out;
int entry_num = 0;
char IR[10000],RISCV[10000],FINAL[10000],Source[10000];
void IR2raw(const char *str);
void genSource(string input);
void genIR(string input);
void genRISCV();
int main(int argc, const char *argv[])
{
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];
  genSource(string(input));
  genIR(string(input));
  genRISCV();
  
  {
      std::ifstream input2("RISCV.txt");
      input2.getline(RISCV,10000,EOF);
      cout<<'\n'<<RISCV<<'\n';
  }
  {
      if (string(mode) == "-riscv")
        strcpy(FINAL,RISCV);
      else if (string(mode) == "-koopa")
        strcpy(FINAL, IR);
      std::ofstream outFile(output);
      outFile <<FINAL;
  }
  return 0;
}
void IR2raw(const char *str)
{
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(str, &program);
  assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  // analyse_raw(raw);
  visit(raw);

  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);
}
void genSource(string input)
{
    std::ifstream source(input);
    source.getline(Source,10000,EOF);
    std::ofstream outFile("source.txt");
    outFile <<Source;
}
void genIR(string input)
//IR
{
  std::ofstream outFile("IR.txt");
  out = std::move(outFile);
  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input.c_str(), "r");
  assert(yyin);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  cout << endl;
  ast->dump();

  out.close();
}
void genRISCV()
{
  std::ofstream outFile("RISCV.txt");
  //std::ofstream outFile(output);
  out = std::move(outFile);
  std::ifstream inputIR("IR.txt");
  inputIR.getline(IR,10000,EOF);
  //cout<<IR<<'\n';//exit(0);
  IR2raw(IR);
  out.close();
}