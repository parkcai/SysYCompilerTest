#include <iostream>
#include <fstream>
#include <memory>
#include <string.h>
#include <AST.h>
#include <koopa.h>
#include <IR.h>
using namespace std;



// 声明 lexer 的输入, 以及 parser 
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
  if(ret){
    return 0;
  }

  FILE* file;

  file=fopen(output,"w");
  (*ast).Dump(file,true);
  fclose(file);

  if(strcmp(mode,"-riscv")==0 || strcmp(mode,"-perf")==0){
    file=fopen(output,"r");
    fseek(file, 0, SEEK_END); // 将文件指针移动到文件末尾
    long fileSize = ftell(file); // 获取文件指针当前位置即文件大小
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);
    printf("%s\n",buffer);
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(buffer, &program);
    assert(ret == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    // 处理 raw program
    file=fopen(output,"w");
    Visit(raw,file);
    fclose(file);
    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  }


  return 0;
}
