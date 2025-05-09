#include <cassert>      
#include <cstdio>       
#include <iostream>     
#include <sstream>     
#include <fstream>      
#include <memory>     
#include <string>       
#include <cstring>     
#include "koopa.h"      
#include "ast.hpp"      
#include "visitraw.hpp" 

using namespace std; 

extern FILE *yyin;                  
extern int yyparse(unique_ptr<BaseAST> &ast);  

int main(int argc, const char *argv[]) {
  assert(argc == 5);  
  auto mode = argv[1];  // 获取编译模式，如 -koopa, -riscv 等
  auto input_file = argv[2]; 
  auto output_file = argv[4];

  // 打开输入文件，并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input_file, "r");  
  assert(yyin);             

  // 打开输出文件，准备写入编译结果
  ofstream output_stream(output_file); 
  assert(output_stream);      

  // 调用 parser 函数进行语法分析，parser 函数会进一步调用 lexer 解析输入文件
  unique_ptr<BaseAST> ast;    
  auto result = yyparse(ast);    
  assert(!result);               

  // 先将 KoopaIR 输出到 stringstream 中
  stringstream koopa_output;                              
  streambuf *original_cout_buf = cout.rdbuf(koopa_output.rdbuf()); 
  ast->KoopaIR();                              

  while (string(mode) != "-koopa" && string(mode) != "-riscv" && string(mode) != "-perf") {  // 如果编译模式不是 "-koopa" 或 "-riscv" 或 "-perf"
    break; 
  }

  while (string(mode) == "-koopa") {  // 如果编译模式为 "-koopa"
    cout.rdbuf(output_stream.rdbuf());  
    cout << koopa_output.str();               
    break; 
  }

  while (string(mode) == "-riscv" || string(mode) == "-perf") {  // 如果编译模式为 "-riscv" 或 "-perf"
    char *buf = new char[koopa_output.str().length() + 1];  
    strcpy(buf, koopa_output.str().c_str());  
    koopa_program_t program;
    koopa_error_code_t parse_result = koopa_parse_from_string(buf, &program);  
    delete[] buf;  
    assert(parse_result == KOOPA_EC_SUCCESS);  

    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw_program = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    cout.rdbuf(output_stream.rdbuf()); 
    Visit(raw_program);  

    koopa_delete_raw_program_builder(builder);
    break;  
  }

  cout.rdbuf(original_cout_buf);
  output_stream.close();  
  return 0;          
}
