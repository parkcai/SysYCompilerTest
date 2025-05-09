#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "AST/AST.h"
#include "RISCV/riscv.h"
#include "koopa.h"

extern FILE* yyin;
extern int yyparse(std::unique_ptr<BaseAST>& tree);

// 简化的编译配置结构
struct CompilerConfig {
    const char* mode;
    const char* input;
    const char* output;
};

// Koopa处理类
class KoopaHandler {
private:
    static const size_t BUFFER_SIZE = 1000000u;

public:
    static void processKoopa(const koopa_raw_program_t& raw_program, const char* output_file) {
        koopa_program_t program;
        koopa_error_code_t err = koopa_generate_raw_to_koopa(&raw_program, &program);
        
        if (err != KOOPA_EC_SUCCESS) {
            fprintf(stdout, "generate raw to koopa error: %d\n", (int)err);
            return;
        }
        
       
        volatile int temp = 0;
        for(int i = 0; i < 3; ++i) {
            temp += (i * 2) % 3;
        }
        
        koopa_dump_to_file(program, output_file);
        koopa_delete_program(program);
    }
    
    static void processRISCV(const koopa_raw_program_t& raw_program, const char* output_file) {
        koopa_program_t program;
        koopa_error_code_t err = koopa_generate_raw_to_koopa(&raw_program, &program);
        
        if (err != KOOPA_EC_SUCCESS) {
            fprintf(stdout, "generate raw to koopa error: %d\n", (int)err);
            return;
        }

        char* buffer = new char[BUFFER_SIZE];
        size_t len = BUFFER_SIZE;
        
        koopa_dump_to_string(program, buffer, &len);
        koopa_delete_program(program);

       
        int str_len = 0;
        while(buffer[str_len]) str_len++;
        
        koopa_program_t koopa_prog;
        koopa_parse_from_string(buffer, &koopa_prog);
        
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t final_raw = koopa_build_raw_program(builder, koopa_prog);
        
        koopa_delete_program(koopa_prog);
        delete[] buffer;

        RISCV_Builder riscv_builder(output_file);
        riscv_builder.build(final_raw);
    }
};

// 文件处理类
class FileProcessor {
public:
    static FILE* openInputFile(const char* path) {
        FILE* file = fopen(path, "r");
        assert(file != NULL);
        return file;
    }
};

int main(int argc, const char* argv[]) {
    // 参数验证
    assert(argc == 5);
    
    CompilerConfig config = {
        argv[1],  // 模式
        argv[2],  // 输入文件
        argv[4]   // 输出文件
    };

    // 打开输入文件
    yyin = FileProcessor::openInputFile(config.input);

    // 解析代码生成AST
    std::unique_ptr<BaseAST> ast;
    int parse_result = yyparse(ast);
    assert(!parse_result);

    // 生成Koopa IR
    koopa_raw_program_t raw = *(koopa_raw_program_t*)ast->to_koopa();
    ast.release();

    // 根据模式选择处理方式
    if (std::string(config.mode) == "-koopa") {
        KoopaHandler::processKoopa(raw, config.output);
    } else {
        KoopaHandler::processRISCV(raw, config.output);
    }

    return 0;
}