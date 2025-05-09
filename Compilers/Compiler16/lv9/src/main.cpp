// src/main.cpp
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include "ast.h"
#include "riscv.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler <mode> <input_file> -o <output_file>
    if (argc != 5) {
        cerr << "Usage: compiler <mode> <input_file> -o <output_file>\n";
        cerr << "Modes:\n";
        cerr << "  -koopa     Generate Koopa IR\n";
        cerr << "  -riscv  Generate RISC-V assembly\n";
        return 1;
    }
    string mode = argv[1];
    string input = argv[2];
    string output_flag = argv[3];
    string output = argv[4];

    // 检查输出标志是否为 -o
    if (output_flag != "-o") {
        cerr << "Error: Expected '-o' flag before output file.\n";
        return 1;
    }

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input.c_str(), "r");
    if (!yyin) {
        cerr << "Error: Cannot open input file " << input << "\n";
        return 1;
    }

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    if (ret) {
        cerr << "Error: Parsing failed.\n";
        fclose(yyin);
        return 1;
    }
    fclose(yyin); // 解析完成后关闭输入文件

    // 生成 Koopa IR
    stringstream ss;
    ast->GenerateIR(ss);
    string ir_code = ss.str();

    // 根据模式决定输出
    if (mode == "-koopa") {
        // 仅生成 Koopa IR
        FILE *out_file = fopen(output.c_str(), "w");
        if (!out_file) {
            cerr << "Error: Cannot open output file " << output << "\n";
            return 1;
        }
        fprintf(out_file, "%s", ir_code.c_str());
        fclose(out_file);
    }
    else if (mode == "-riscv" || mode == "-perf") {
        // 生成 RISC-V 汇编代码

        // 解析 Koopa IR
        koopa_program_t program;
        koopa_error_code_t err = koopa_parse_from_string(ir_code.c_str(), &program);
        if (err != KOOPA_EC_SUCCESS) {
            cerr << "Error: Failed to parse Koopa IR.\n";
            return 1;
        }

        // 将 Koopa IR 程序转换为 raw program
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw_program = koopa_build_raw_program(builder, program);

        // 释放 Koopa IR 程序占用的内存
        koopa_delete_program(program);

        // 生成 RISC-V 汇编代码
        stringstream riscv_ss;
        if (mode == "-riscv"){
            GenerateRISCV(raw_program, riscv_ss, false);
        }
        else{
            GenerateRISCV(raw_program, riscv_ss, true);
        }
        
        string riscv_code = riscv_ss.str();

        // 将汇编代码输出到指定文件
        FILE *out_file = fopen(output.c_str(), "w");
        if (!out_file) {
            cerr << "Error: Cannot open output file " << output << "\n";
            koopa_delete_raw_program_builder(builder);
            return 1;
        }
        fprintf(out_file, "%s", riscv_code.c_str());
        fclose(out_file);

        // 释放 raw program builder 占用的内存
        koopa_delete_raw_program_builder(builder);
    }
    else {
        cerr << "Error: Unknown mode '" << mode << "'. Supported modes are '-koopa', '-riscv' and '-perf'.\n";
        return 1;
    }

    return 0;
}
