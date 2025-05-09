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
const int N =  300000000;
ofstream out;
int entry_num = 0;
char IR[N],RISCV[N],FINAL[N ],Source[N];
void IR2raw(const char *str);
void genSource(string input);
void genIR(string input);
void genRISCV();
std::string getCurrentTimeString();//获取当前时间
void useMyRISCV(string output);
bool allow_cout = false;
int main(int argc, const char *argv[])
{
  assert(argc == 5);
  auto mode = argv[1];
  string input = argv[2];
  string output = argv[4];
  bool useMyInput = false;
  if(useMyInput)
  input ="/root/compiler/hello.c";
  
  //useMyRISCV(output);return 0;
  //genSource(string(input));return 0;
  genIR(string(input));//exit(0);
  //if (string(mode) == "-riscv")
  cerr<<"IR is OK\n";
  std::ifstream inputIR("IR.txt");
  inputIR.getline(IR, N,EOF);
  genRISCV();
  cerr<<"RISCV is OK"<<endl;
  {
      std::ifstream input2("RISCV.txt");
      input2.getline(RISCV, N,EOF);
      if(allow_cout)cout<<'\n'<<RISCV<<'\n';
  }
  {
      if (string(mode) == "-riscv"||string(mode) == "-perf")
        strcpy(FINAL,RISCV);
      else if (string(mode) == "-koopa")
        strcpy(FINAL, IR);
      std::ofstream outFile(output);
      outFile <<FINAL;
      //cerr<<FINAL<<endl;
      //ofstream fout("/root/compiler/OutFile.txt");
      //fout<<FINAL;
  }
  return 0;
}
void IR2raw(const char *str)
{
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  cerr<<__LINE__<<endl;
  koopa_error_code_t ret = koopa_parse_from_string(str, &program);
  cerr<<__LINE__<<endl;
  assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  cerr<<__LINE__<<endl;
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);
  // analyse_raw(raw);
  visit(raw);
  cerr<<__LINE__<<endl;
  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);
}
void genSource(string input)
{
    std::ifstream source(input);
    source.getline(Source, N,EOF);
    string outpos = "./source/"+getCurrentTimeString()+".txt";
    std::ofstream outFile(outpos);
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
  ast->dump();

  out.close();
}
void genRISCV()
{
  std::ofstream outFile("RISCV.txt");
  //std::ofstream outFile(output);
  out = std::move(outFile);
  if(allow_cout)cout<<IR<<'\n';//exit(0);
  cerr<<"begin IR2raw"<<endl;
  IR2raw(IR);
  out.close();
}
// 获取当前时间字符串，精确到微秒
std::string getCurrentTimeString() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 转换为时间点
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    // 获取微秒部分
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) %  1000000;
    
    // 转换为本地时间
    std::tm* localTime = std::localtime(&in_time_t);
    
    // 格式化时间字符串
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S");
    oss << "_" << std::setfill('0') << std::setw(6) << micros.count();
    
    return oss.str();
}
void useMyRISCV(string output)
{
  std::ifstream input2("/root/compiler/myRISCV.txt");
  input2.getline(FINAL, N,EOF);
  std::ofstream outFile(output);
  outFile <<FINAL;
}