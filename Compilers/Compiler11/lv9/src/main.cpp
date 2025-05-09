#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string.h>
#include <stdio.h>
#include "AST.hpp"
#include "riscv.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(Program **program);
extern void yyerror(Program **program,const char*);


int main(int argc,char* argv[])
{
    char *in_file = (char*)"../source.cpp";
    char *out_file = (char*)"../output.txt";
    int omode = RISCV_MODE;
    
    for (int i = 1; i < argc; i++) {
        if (!memcmp(argv[i], "-koopa", 6)) omode = KOOPA_MODE;
        else if (!memcmp(argv[i], "-riscv", 6)) omode = RISCV_MODE;
        else if (!memcmp(argv[i], "-o", 2)) out_file = argv[++i];
        else in_file = argv[i];
    }
    freopen(in_file,"r",stdin);
    Program * answer;
    yyparse(&answer);
    if(omode == KOOPA_MODE)
    {
        freopen(out_file,"w",stdout);
        answer->print_ir();
    }
    if(omode == RISCV_MODE)
    {
        freopen("../koopa.txt","w",stdout);
        answer->print_ir();
        cout<<endl;
        FILE* ff=fopen("../koopa.txt","r");
        char *buf=(char *)malloc(1000000);
        fread(buf, 1,1000000, ff);
        freopen(out_file,"w",stdout);
        print_riscv(buf);
    }
}
