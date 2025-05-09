#define DEBUG 0

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <functional>
#include "ast.h"
#include "koopa.h"
#include "raw_prog_visitor.h"
#include "riscv_compile.h"


using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

void parseProgramFromString(const char* str,function<void(koopa_raw_program_t&)> callback){
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    // ...
    callback(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}

string redirectOstream(const ostream& out,function<void(void)> runnable){
    ostringstream oss;
    streambuf* originBuf =cout.rdbuf();
    cout.rdbuf(oss.rdbuf());
    runnable();
    cout.rdbuf(originBuf);
    return oss.str();
}
class TestBase{
    public:
        std::string value;
        TestBase(const std::string& val):value(val){
            _DEBUG(value);
            _DEBUG(getClass())
        }
        virtual std::string getClass(){
            return "Base";
        }
};
class TestDerived: TestBase{
    public:
        virtual std::string getClass(){
            return "Derived";
        }
        TestDerived():TestBase(getClass()){

        }
};
class TestDerived2: TestDerived{
    public:
        virtual std::string getClass(){
            return "Derived";
        }
        TestDerived2():TestDerived(){

        }
};


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
    ofstream outFile(output);
    assert(outFile);
    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    if(ret){
        cerr<<input<<"\n"; 
    }
    assert(!ret);
    //asked from gpt
    

    
    string koopaString=redirectOstream(cout,[&ast](){
        ScopeStack scope;
        ast->toKoopaString(scope);
        cout << "\n";
    });
    cerr<<"toKoopaString finished\n";
    string compilerMode(mode);
    if(compilerMode=="-koopa"){
        outFile << koopaString;
    }
    else if(compilerMode=="-riscv"||compilerMode=="-perf"){
        string riscvOut=redirectOstream(cout,[&koopaString](){
            parseProgramFromString(koopaString.c_str(),[](koopa_raw_program_t& rawProgram){

                //cout<<"parse Success\n";
                // for(size_t i=0;i<rawProgram.funcs.len;++i){
                //     assert(rawProgram.funcs.kind == KOOPA_RSIK_FUNCTION);
                //     koopa_raw_function_t func = (koopa_raw_function_t)rawProgram.funcs.buffer[i];
                //     for (size_t j=0;j<func->bbs.len;++j){
                //         assert(func->bbs.kind==KOOPA_RSIK_BASIC_BLOCK);
                //         koopa_raw_basic_block_t bb=(koopa_raw_basic_block_t)func->bbs.buffer[j];
                //         for(size_t k=0;k<bb->insts.len;++k){
                //             assert(bb->insts.kind==KOOPA_RSIK_VALUE);
                //             koopa_raw_value_t value=(koopa_raw_value_t)bb->insts.buffer[k];
                //             assert(value->kind.tag ==KOOPA_RVT_RETURN);
                //             koopa_raw_value_t ret_value=value->kind.data.ret.value;
                //             assert(ret_value->kind.tag==KOOPA_RVT_INTEGER);
                //             int32_t int_val=ret_value->kind.data.integer.value;
                //             assert(int_val==0);
                //         }
                //     }
                // }
                
                unique_ptr<KoopaRawProgramVisiter> visitor=make_unique< KoopaRawProgramVisiter>(InfoPreReadKoopaProgramProcessor::INSTANCE);
                visitor->visit(rawProgram);
                _DEBUG("First visit end")
                unique_ptr<KoopaRawProgramVisiter> visitor2=make_unique< KoopaRawProgramVisiter>(new SimpleKoopaRawProgramProcessor(InfoPreReadKoopaProgramProcessor::INSTANCE));
                visitor2->visit(rawProgram);
            // cout<<"Assertion passed\n";
            });
        });
        outFile <<riscvOut;
    }else{

    }
    outFile.close();
    cout << "finished\n";
    return 0;
}
