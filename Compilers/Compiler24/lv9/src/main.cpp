#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "AST.hpp"
#include "riscv.hpp"
#include <map>

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {

  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  yyin = fopen(input, "r");
  assert(yyin);

  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  string koopaIR;
  ast->set_symbol_table(&global_table);
  ast->GenerateIR(koopaIR);

  if(strcmp(mode,"-koopa")==0){
    ofstream out(output);
    out << koopaIR;
    out.close();
  }
  else if(strcmp(mode,"-riscv")==0||strcmp(mode,"-perf")==0){
    koopa_program_t program;
    koopa_error_code_t ret=koopa_parse_from_string(koopaIR.c_str(),&program);
    assert(ret==KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder=koopa_new_raw_program_builder();
    koopa_raw_program_t raw=koopa_build_raw_program(builder,program);
    koopa_delete_program(program);
    string risc;
    Visit(raw,risc);
    ofstream out(output);
    out << risc;
    out.close();
    koopa_delete_raw_program_builder(builder);
  }
  else if(strcmp(mode,"-debug")==0){
    std::ofstream outFile("debug.txt");
    std::streambuf* originalBuf = std::cout.rdbuf();
    std::cout.rdbuf(outFile.rdbuf());
    ast->Dump();
    std::cout.rdbuf(originalBuf);
  }
  else{
    assert(false);
  }
  return 0;
}
