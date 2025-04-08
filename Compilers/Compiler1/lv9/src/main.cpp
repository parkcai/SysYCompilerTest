#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "abstract_syntax_tree.h"
#include "utils.h"
#include "koopa.h"
#include "koopa_visitor.h"
#include "koopa_optimizor.h"
#include "koopa_disenchanter.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(shared_ptr<BaseAST> &ast);
SymbolTableList symbol_table_list;
FuncparamNote funcparam_note;
int current_max_register;
int current_if_index;
int current_while_index;
int current_or_index;
int current_and_index;
int next_if_index;
int next_while_index;
int next_or_index;
int next_and_index;
int lval_called_from_rhs;
int block_called_from_func_def;
string current_func_type;
int may_using_func_res;
int is_last_element;
int in_main_stream;
int main_stream_returned;
int getint_overridden = 0;
int getch_overridden = 0;
int getarray_overridden = 0;
int putint_overridden = 0;
int putch_overridden = 0;
int putarray_overridden = 0;
int starttime_overridden = 0;
int stoptime_overridden = 0;
int array_total_length = 0;
int dealing_with_globals = 0;
string array_stack_position = "Tsinghua";
string currently_dealt_btype = "int";
unordered_map<string, int> is_int_ptr;
int inside_array_init = 0;
map<string, vector<int>> array_dimension_note;
string global_dest_str;
string global_save_mode;
string segmentation = "\n\n";
int stack_space_R;
int stack_space_S;
int stack_space_A;
int stack_space_total;
int current_bias;
map<koopa_raw_value_data_t*, int> bias_from_last_rsp_of;
int max_call_inst_len;
int debug_shown_in_riscv = 1;
string currently_dealt_func_name;
int imm12_max = 2047;
int imm12_min = -2048;
map<string, koopa_raw_value_data_t*> loaded_koopa_register;
int forward_step = 0;
int OR_HEAD = 1;
int AND_HEAD = 2;
int instruction_set_type = 0;

void SysY_to_Koopa(char* c_input, string& output);
void Koopa_to_Riscv(string& koopa_path, string& output);
void Riscv_to_Exe(string& riscv_path, string& output);
void Koopa_Disenchant(string& koopa_path);

/*
* 本编译器项目提供从.c到.koopa的转换，以及从.koopa到.s的转换
* 各模式的原则：最大限度地利用本编译器项目提供的转换
* 最后的文件向output输出
* 中间文件向input相应修改后缀名的路径输出
* 注意：
* 1. 可能产生意料之外的文件修改效果。
*    譬如，在参数-exe hello.c -o executable_hello下，hello.koopa hello.s和hello.o如果事先存在，会被删除
* 2. 本编译器项目实现的从.koopa到.s的转换不是通用的，仅保证成功转换由本项目生成的KoopaIR文件
*/
int main(int argc, const char *argv[]) {

    symbol_table_list.verbose = false;

    // 获取输入输出文件位置和编译模式
    assert(argc == 5);
    auto c_mode = argv[1];
    char* c_input = const_cast<char*>(argv[2]);
    char* c_output = const_cast<char*>(argv[4]);
    string mode(c_mode);
    string input(c_input);
    string output(c_output);

    // koopa模式，输入.c 输出.koopa
    if (mode == "-koopa") {
        if (get_file_extension(input) == "c") {
            SysY_to_Koopa(c_input, output);
            Koopa_Disenchant(output);
            return 0;
        }else{
            int file_valid_for_mode_koopa = 0;
            assert(file_valid_for_mode_koopa);
        }
    }

    // koopa2riscv模式，输入.c 输出.koopa和.s
    if (mode == "-koopa2riscv") {
        if (get_file_extension(input) == "c") {
            string koopa_path = input.substr(0, input.find_last_of('.')) + ".koopa";;
            SysY_to_Koopa(c_input, koopa_path);
            Koopa_to_Riscv(koopa_path, output);
            Koopa_Disenchant(koopa_path);
            return 0;
        }else{
            int file_valid_for_mode_koopa2riscv = 0;
            assert(file_valid_for_mode_koopa2riscv);
        }
    }

    // koopa2exe模式，输入.c 输出.koopa和可执行文件
    if (mode == "-koopa2exe") {
        if (get_file_extension(input) == "c") {
            string koopa_path = input.substr(0, input.find_last_of('.')) + ".koopa";
            string riscv_path = input.substr(0, input.find_last_of('.')) + ".s";
            SysY_to_Koopa(c_input, koopa_path);
            Koopa_to_Riscv(koopa_path, riscv_path);
            Riscv_to_Exe(riscv_path, output);
            delete_file(riscv_path);
            Koopa_Disenchant(koopa_path);
            return 0;
        }else{
            int file_valid_for_mode_koopa2exe = 0;
            assert(file_valid_for_mode_koopa2exe);
        }
    }

    // koopa2riscv2exe模式，输入.c 输出.koopa .s和可执行文件
    if (mode == "-koopa2riscv2exe") {
        if (get_file_extension(input) == "c") {
            string koopa_path = input.substr(0, input.find_last_of('.')) + ".koopa";
            string riscv_path = input.substr(0, input.find_last_of('.')) + ".s";
            SysY_to_Koopa(c_input, koopa_path);
            Koopa_to_Riscv(koopa_path, riscv_path);
            Riscv_to_Exe(riscv_path, output);
            Koopa_Disenchant(koopa_path);
            return 0;
        }else{
            int file_valid_for_mode_koopa2exe2riscv = 0;
            assert(file_valid_for_mode_koopa2exe2riscv);
        }
    }

    // riscv模式，输入.c/.koopa 输出.s
    if (mode == "-riscv" || mode == "-perf") {
        if (get_file_extension(input) == "c") {
            string koopa_path = input.substr(0, input.find_last_of('.')) + ".koopa";;
            SysY_to_Koopa(c_input, koopa_path);
            Koopa_to_Riscv(koopa_path, output);
            delete_file(koopa_path);
            return 0;
        }else if (get_file_extension(input) == "koopa") {

        }else{
            int file_valid_for_mode_riscv = 0;
            assert(file_valid_for_mode_riscv);
        }
    }

    // riscv2exe模式，输入.c/.koopa 输出.s和可执行文件
    if (mode == "-riscv2exe") {
        if (get_file_extension(input) == "c") {

        }else if (get_file_extension(input) == "koopa") {

        }else{

        }
    }

    // exe模式，输入.c/.koopa/.s 输出可执行文件
    if (mode == "-exe") {
        if (get_file_extension(input) == "c") {
            string koopa_path = input.substr(0, input.find_last_of('.')) + ".koopa";
            string riscv_path = input.substr(0, input.find_last_of('.')) + ".s";
            SysY_to_Koopa(c_input, koopa_path);
            Koopa_to_Riscv(koopa_path, riscv_path);
            Riscv_to_Exe(riscv_path, output);
            delete_file(riscv_path);
            delete_file(koopa_path);
            return 0;
        }else if (get_file_extension(input) == "koopa") {

        }else if (get_file_extension(input) == "s") {

        }else{

        }
    }

    int mode_valid = 0;
    assert(mode_valid);
    return 0;
}

void SysY_to_Koopa(char* c_input, string& output) {
    string koopa_dest_str;
    yyin = fopen(c_input, "r");
    assert(yyin);
    shared_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);
    ast->SaveKoopa(koopa_dest_str, "string");
    koopa_program_t program;
    koopa_parse_from_string(koopa_dest_str.c_str(), &program);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
    Optimize_program(raw);
    koopa_generate_raw_to_koopa(&raw, &program);
    koopa_delete_raw_program_builder(builder);
    refresh_file(output);
    koopa_dump_to_file(program, output.c_str());
    koopa_delete_program(program);
}

void Koopa_to_Riscv(string& koopa_path, string& output) {
    koopa_program_t program;
    koopa_error_code_t koopa_ret = koopa_parse_from_file(koopa_path.c_str(), &program);
    assert(koopa_ret == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
    global_dest_str = output;
    global_save_mode = "output_file";
    refresh_file(output);
    Visit_program(raw);
    koopa_delete_raw_program_builder(builder);
}

void Riscv_to_Exe(string& riscv_path, string& output) {
    string obj_path = riscv_path.substr(0, riscv_path.find_last_of('.')) + ".o";
    refresh_file(obj_path);
    string clang_command1 = "clang "+riscv_path+" -c -o "+obj_path+" -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32";
    execute_command(clang_command1);
    refresh_file(output);
    string clang_command2 = "ld.lld "+obj_path+" -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o "+output;
    execute_command(clang_command2);
    delete_file(obj_path);
}

void Koopa_Disenchant(string& koopa_path) {

}
