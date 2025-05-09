#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.h"
#include "koopa.h"
#include "riscv.h"
#include "riscv_opt.h"
using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
StackFrameAllocator sfa;
RegisterAllocator rega;
int Branch_Index=0;
int is_ret=0;
int Block_Index = 0;
int Koopa_Index = -1;
int Val_Index=0;
int global_libfunc = 0;
extern int yyparse(unique_ptr<BaseAST> &ast);
koopa_raw_function_t func_now;
std::vector<std::unordered_map<std::string,struct valinfo>> Koopa_blocklist;
std::vector<struct whileinfo> while_stack;
std::unordered_map<std::string, struct funcinfo>func_list;
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
  // parse input file
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  string risc="-riscv";
  string koopa="-koopa";
  string perf="-perf";
  if(!koopa.compare(mode)){
    std::ofstream outFile(output);
    ast->ToIR(outFile);
    outFile << endl;
  }

  else if (!risc.compare(mode)){
    std::ofstream outFile("mid.koopa");
    ast->ToIR(outFile);
    outFile << endl;
    FILE *koopaout = fopen("mid.koopa","r");
    std::ofstream riscvout(output);
    KoopatoRiscv(koopaout,riscvout);
    fclose(koopaout);
  }
  else if (!perf.compare(mode)){
    std::ofstream outFile("mid.koopa");
    ast->ToIR(outFile);
    outFile << endl;
    FILE *koopain = fopen("mid.koopa","r");
    std::ofstream midout("mid.S");
    KoopatoRiscv(koopain,midout);
    std::ifstream midin("mid.S");
    std::ofstream riscvout(output);
    RiscvOpt_load(midin,riscvout);
    fclose(koopain);
  }
  // dump AST
  ast->Dump();
  cout << endl;
  return 0;
}
