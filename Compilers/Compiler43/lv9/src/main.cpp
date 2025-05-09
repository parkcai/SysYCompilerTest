#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ast.hpp"
#include "koopa.h"
#include "visit_koopa.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[])
{
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];
    yyin = fopen(input, "r");
    assert(yyin);
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);
    
    auto raw = ast->toKoopa();

    if(mode[1] == 'k') {
        koopa_program_t program;
        auto ret = koopa_generate_raw_to_koopa((koopa_raw_program_t *)raw, &program);
        if(ret != KOOPA_EC_SUCCESS) {
            cout << ret << endl;
        }
        koopa_dump_to_file(program, output);
        
    }
    else if (mode[1] == 'r' || mode[1] == 'p') {
        koopa_program_t program;
        auto ret1 = koopa_generate_raw_to_koopa((koopa_raw_program_t *)raw, &program);
        assert(ret1 == KOOPA_EC_SUCCESS);
        size_t len;
        auto ret2 = koopa_dump_to_string(program, nullptr, &len);
        assert(ret2 == KOOPA_EC_SUCCESS);
        char *s = new char[len + 1];
        auto ret3 = koopa_dump_to_string(program, s, &len + 1);
        s[len] = '\0';
        
        if(ret3 != KOOPA_EC_SUCCESS) {
            cout << "ERROR ! -" << ret << "\n" << len << endl;
        }
        auto ret4 = koopa_parse_from_string(s, &program);
        assert(ret4 == KOOPA_EC_SUCCESS);
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);

        koopa_generate_raw_to_koopa(&raw, &program);
        koopa_dump_to_file(program, "hello.koopa");

        ofstream out(output, ios::out | ios::trunc);
        assert(out.is_open());
        visit(out, raw);
        out.close();
        koopa_delete_raw_program_builder(builder);
    }
    

    return 0;
}
