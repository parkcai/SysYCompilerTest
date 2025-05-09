#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <ast.hpp>
#include "koopa.h"
#include "koopa_gen.hpp"

using namespace std;

extern FILE *yyin, *yyout;
extern int yyparse(past_t &ast);

int exp_var_cnt = 0;
int if_cnt = 0;

int main(int argc, const char *argv[]){
	assert(argc == 5);

	auto mode = argv[1];
	auto input = argv[2];
	auto output = argv[4];

	ofstream outfile;
	streambuf *oldbuf;
	stringstream koopaIR;

	yyin = fopen(input, "r");
	assert(yyin);

	past_t ast;
	auto ret = yyparse(ast);
	assert(!ret);
	
	cout << "Parse over." << endl;

	// for debug
	if (string(mode) == "-debug") {
		
		if (string(argv[3]) == "-o") {
			outfile = ofstream(output);

			streambuf *old_ostream = cout.rdbuf();
			cout.rdbuf(outfile.rdbuf());

			ast->Dump();
			
			cout.rdbuf(old_ostream);
			
		}
		else {
			ast->Dump();
		}

		return 0;
	}


	oldbuf = cout.rdbuf();
	cout.rdbuf(koopaIR.rdbuf());

	ast->DumpIR();

	cout.rdbuf(oldbuf);

	cout << "Pre DumpIR complete.\n" << endl;

	if (string(mode) == "-koopa") {

		if (string(argv[3]) == "-o") {
			outfile = ofstream(output);
			
			oldbuf = cout.rdbuf();
			cout.rdbuf(outfile.rdbuf());

			cout << koopaIR.str();

			cout.rdbuf(oldbuf);
		}
		else {
			// for debug
			cout << koopaIR.str();
		}
	}
	else if (string(mode) == "-riscv" || string(mode) == "-perf") {

		if (string(argv[3]) == "-o") {
			outfile = ofstream(output);

			streambuf *old_ostream = cout.rdbuf();
			cout.rdbuf(outfile.rdbuf());

			generate_koopa(koopaIR.str().c_str());
			
			cout.rdbuf(old_ostream);
			
		}
		else {
			// for debug
			cout << koopaIR.str() << endl;
			generate_koopa(koopaIR.str().c_str());
		}
	}
		
	return 0;
}

void generate_koopa(const char* IR)
{
	// 解析字符串 str, 得到 Koopa IR 程序
	koopa_program_t program;
	koopa_error_code_t ret = koopa_parse_from_string(IR, &program);
	assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
	// 创建一个 raw program builder, 用来构建 raw program
	koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
	// 将 Koopa IR 程序转换为 raw program
	koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
	// 释放 Koopa IR 程序占用的内存
	koopa_delete_program(program);

	// 处理 raw program
	// ...
	init_registers();
	// printf("init_regiter end\n");
	Visit(raw);

	// 处理完成, 释放 raw program builder 占用的内存
	// 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
	// 所以不要在 raw program 处理完毕之前释放 builder
	koopa_delete_raw_program_builder(builder);
}
