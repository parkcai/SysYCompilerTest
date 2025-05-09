#include <cassert>
#include <cstdio>
#include <fstream>

#include "AST.h"
#include "koopa.h"
#include "visit.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    string mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    // 输出解析得到的 AST, 其实就是个字符串
    stringstream ss;
    // 保存当前cout缓冲区
    streambuf* oldCoutStreamBuf = std::cout.rdbuf();

    cout.rdbuf(ss.rdbuf());
    ast->GenerateIR();
    // 恢复cout缓冲区
    cout.rdbuf(oldCoutStreamBuf);

    string outString = ss.str();
    const char *str = outString.c_str();
    //cout << str << endl;
    if (mode == "-koopa") {
        ofstream outfile(output);
        outfile << str;
        outfile.close();
    }

    if (mode == "-riscv" || mode == "-perf") {
        /*
        ifstream infile("test.txt");
        stringstream buffer;
        buffer << infile.rdbuf();
        string strstring = buffer.str();
        infile.close();
        str = strstring.c_str();
        */
       
        // 解析字符串str, 得到koopa程序
        koopa_program_t program;
        koopa_error_code_t ret_error = koopa_parse_from_string(str, &program);
        assert(ret_error == KOOPA_EC_SUCCESS);
        // 创建一个 raw program builder, 用来构建 raw program
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        // 将 Koopa IR 程序转换为 raw program
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        // 释放 Koopa IR 程序占用的内存
        koopa_delete_program(program);

        // 处理 raw program
        // ...

        stringstream ss;
        // 保存当前cout缓冲区
        streambuf* oldCoutStreamBuf = std::cout.rdbuf();
        cout.rdbuf(ss.rdbuf());
        
        Visit(raw);

        // 恢复cout缓冲区
        cout.rdbuf(oldCoutStreamBuf);
        string outString = ss.str();
        //cout << "riscv" << endl;
        const char *str = outString.c_str();
        //cout << str << endl;
        ofstream outfile(output);
        outfile << str;
        outfile.close();

        // 处理完成, 释放 raw program builder 占用的内存
        // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
        // 所以不要在 raw program 处理完毕之前释放 builder
        koopa_delete_raw_program_builder(builder);
    }
    return 0;
}