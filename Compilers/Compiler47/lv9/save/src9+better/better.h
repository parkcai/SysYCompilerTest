#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <assert.h>
using namespace std;

static const int REG_NUM = 20;
std::string reg_names[REG_NUM] = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    //0     1     2    3      4     5     6
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0", "temp", "ra", "sp","s11"
    //7    8      9    10    11    12    13    14    15    16      17    18   19
};
struct instruction {
    string instruct;
    int dest = -1;
    int pos1 = -1;
    int pos2 = -1;
    string label;
    int imm = -1;
    //int imm2 = -1;
    int op;
    string str;
    string str2;
};

vector<instruction> instructions;
int regs[32] = {0};//保存了寄存器存储的地址
map<int,int> reg_map;//保存了寄存器存储的地址
int get_reg()
{

}
int get_temp_reg()
{
    return 19;
}
bool check(int pos)
{

}
bool small_num(int pos)
{
    return pos >= -2048 && pos <= 2047;
}
string lw()
{

}
void sw(int reg)
{
    //do sth
}
void optimize(vector<instruction>&instructions)
{
    ofstream perf("perf.txt");
    for (int i = 0; i < instructions.size(); i++)
    {
        string str;
        if (instructions[i].instruct == "str")
        {
            str = instructions[i].str;
        }
        else if (instructions[i].instruct == "addsp")
        {
            if(small_num(instructions[i].imm))
            {
                str = "\taddi sp, sp, " + to_string(instructions[i].imm) + "\n";
            }
            else
            {
                int reg = get_temp_reg();
                perf << "\tli "<<reg_names[reg] << ", " << instructions[i].imm << endl;
                perf << "\tadd sp, sp, " << reg_names[reg] << endl;
            }
        }
        else if (instructions[i].instruct == "swsp")
        {
            if(small_num(instructions[i].imm))
            {
                perf << "\taddi sp, sp, " << instructions[i].imm << endl;
            }
            else
            {
                int reg = get_temp_reg();
                perf << "\tli " << reg_names[reg] << ", " << instructions[i].imm << endl;
                if(small_num(instructions[i].dest))
                {
                    perf << "\tsw " << reg_names[reg] <<", "<<instructions[i].dest<<"(sp)" << endl;
                }
                else
                {
                    int reg2 = get_temp_reg();
                    perf << "\tli " << reg_names[reg2] << ", " << instructions[i].dest << endl;
                    perf << "\tsw " << reg_names[reg] <<", 0("<<reg_names[reg2]<<")"<< endl;
                }
            }
        }
        else if(instructions[i].instruct == "ret")
        {
            if(small_num(instructions[i].pos1))
            {
                perf << "\tlw " << "a0" <<", "<<instructions[i].pos1<<"(sp)" << endl;
            }
            else
            {
                int reg = get_temp_reg();
                perf << "\tli " << reg_names[reg] << ", " << instructions[i].pos1 << endl;
                perf << "\tlw " << "a0" <<", 0("<<reg_names[reg]<<")"<< endl;
            }
        }
        else if(instructions[i].instruct == "binary")
        {

        }
        else if(instructions[i].instruct == "int")
        {

        }
        else
        {
            assert(0);
        }

    }
}
//第一遍扫描：以函数为单位，删去冗余sw
//第二遍扫描：以函数为单位，若lw