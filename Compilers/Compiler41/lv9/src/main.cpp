#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "ast.hpp"
#include "koopa.h"
#include "koopa_visit.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
  assert(argc == 5);
  std::string mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  /* Debug: output the content of input file */
  bool debug = false;
  if (debug) {
    yyin = fopen(input, "r");
    char ch;
    while ((ch = fgetc(yyin)) != EOF) {
      std::cout << ch;
    }
    fclose(yyin);
  }

  /* parse input file to AST */
  yyin = fopen(input, "r");
  assert(yyin);
  unique_ptr<BaseAST> ast;
  auto ret_parse_ast = yyparse(ast);
  if (debug || ret_parse_ast) {
    cout << "Error in parsing AST!" << endl;
    return 100;
  }
  assert(!ret_parse_ast);

  /* convert AST to IR */
  AST2IRConverter ast2ir;
  std::string koopa_ir;
  ast2ir.init();
  koopa_ir = ast2ir.Convert(ast.get());
  if (mode == "-koopa") 
  {
    freopen(output, "w", stdout);
    cout << koopa_ir << endl;
    fclose(stdout);
    return 0;
  }

  /* convert IR to RISCV */
  koopa_program_t program;
  koopa_error_code_t ret_parse_koopa = koopa_parse_from_string(koopa_ir.c_str(), &program);
  // make sure the parsing is successful
  if (ret_parse_koopa != KOOPA_EC_SUCCESS) {
    cout << "Error in parsing koopa IR!" << endl;
    return 200;
  }
  assert(ret_parse_koopa == KOOPA_EC_SUCCESS);  
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  koopa_delete_program(program);

  KoopaVisitor visitor;
  std::string risv_code = visitor.Convert(raw);

  // free memory
  koopa_delete_raw_program_builder(builder);

  // dump IR to output file
  if (mode == "-riscv" || mode == "-perf") {
    freopen(output, "w", stdout);
    cout << risv_code << endl;
    fclose(stdout);
    return 0;
  }
  freopen(output, "w", stdout);
  cout << "unknown mode" << endl;
  fclose(stdout);
  return 0;
}