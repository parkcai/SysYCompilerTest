#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "../head/ast.hpp"
#include "../head/riscv.hpp"

using namespace std;

// 声明 lexer 的输入，以及 parser 函数
// 为什么不用 sysy.tab.hpp 呢？因为这里面没有 yyin 的定义
// 其次，因为这个文件不是我们自己写的，而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件，然后会给你报错（虽然编译不会出错）
// 看起来会很烦人，于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 解析命令行参数，测试脚本/评测平台要求你的编译器能接收以下参数：
    // - 模式文件 - 输入文件 - 输出文件
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    // 打开输入文件，并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 调用 parser 函数，parser 函数会进一步调用 lexer 解析输入文件的

    std::unique_ptr<BaseAST> ast;

    auto ret = yyparse(ast);

    if(ret)
    {
        std::cout << "yyparse error: " << ret << std::endl;
        assert(!ret);
    }

    // dump AST
    ast->show();

    std::unique_ptr<CompUnitAST> comp_ast(dynamic_cast<CompUnitAST *>(ast.release()));
    koopa_raw_program_t krp = comp_ast->to_koopa_raw_program();
    if(strcmp(mode, "-koopa") == 0)
    {  
        std::cout << "generate koopa file..." << std::endl;
        koopa_program_t kp;
        koopa_error_code_t eno = koopa_generate_raw_to_koopa(&krp, &kp);
        if (eno != KOOPA_EC_SUCCESS)
        {
            std::cout << "generate raw to koopa error: " << (int)eno << std::endl;
            return 0;
        }
        koopa_dump_to_file(kp, output);
        koopa_dump_to_stdout(kp);
    }
     else if(strcmp(mode, "-riscv") == 0 || strcmp(mode, "-perf") == 0)
    {
        std::cout << "generate koopa file..." << std::endl;
        koopa_program_t kp;
        koopa_error_code_t eno = koopa_generate_raw_to_koopa(&krp, &kp);
        char *buffer = new char[1000000];
        size_t sz = 1000000u;
        eno = koopa_dump_to_string(kp, buffer, &sz);
        if (eno != KOOPA_EC_SUCCESS)
        {
            std::cout << "koopa dump to string error: " << (int)eno << std::endl;
            return 0;
        }
        koopa_delete_program(kp);
        koopa_program_t new_kp;
        
        eno = koopa_parse_from_string(buffer, &new_kp);
        if (eno != KOOPA_EC_SUCCESS)
        {
            std::cout << "generate raw to koopa error: " << (int)eno << std::endl;
            return 0;
        }

        koopa_raw_program_builder_t kp_builder = koopa_new_raw_program_builder();
        koopa_raw_program_t new_krp = koopa_build_raw_program(kp_builder, new_kp);
        koopa_delete_program(new_kp);

        std::cout << "generate riscv file..." << std::endl;
        std::ofstream out(output);
        //RISCV builder(std::cout);
        
        RISCV builder(out);
        builder.to_string(&new_krp);
        out.close();
        koopa_delete_raw_program_builder(kp_builder);
        
    }

    return 0;
}

