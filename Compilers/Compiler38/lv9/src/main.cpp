#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "ast_class.h"
#include "lr_dfs.h"
#include "koopa.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(CompUnitAST* &ast);

int main(int argc, const char *argv[])
{
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  yyin = fopen(input, "r");
  assert(yyin);

  CompUnitAST* ast;
  auto prase_ret = yyparse(ast);
  assert(!prase_ret);
  std::string koopa_ir_code = "";
  ast->setSymbolTable();
  ast->generate();
  ast->dump(koopa_ir_code);

  if (mode == string("-koopa"))
  {
    FILE *outfile = fopen(output, "w");
    if (!outfile)
    {
      fprintf(stderr, "Failed to open output file: %s\n", output);
      return 1;
    }
    fprintf(outfile, "%s\n", koopa_ir_code.c_str());
    fclose(outfile);
    return 0;
  }
  
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(koopa_ir_code.c_str(), &program);
  assert(ret == KOOPA_EC_SUCCESS);
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  koopa_delete_program(program);

  string risv_str = "";
  Root_Visit(raw, risv_str);
  FILE *outfile = fopen(output, "w");
  if (!outfile)
  {
    fprintf(stderr, "Failed to open output file: %s\n", output);
    return 1;
  }
  fprintf(outfile, "%s\n", risv_str.c_str());
  fclose(outfile);
  koopa_delete_raw_program_builder(builder);
  return 0;
}
