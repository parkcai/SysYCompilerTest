#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <set>
using namespace std;

// vector<string> riscv;
// vector<int>func_pos;
//vector<string>;
// static map<int,int>lw;
// static map<int,int>sw;
vector<bool> f(vector<int>& pos,vector<char>&type, int para_num);
void delete_command();
void perf() {
    delete_command();
    ifstream input("/root/compiler/RISCV.txt");
    int para_num;
    vector<string> all;
    while(1)
    {
        string temp;
        getline(input, temp);
        if(temp == "#end program")
            break;
        if(temp == ".text")
        {
            vector<string> instructions;
            vector<int>pos;
            vector<char>type;
            instructions.push_back(temp);
            pos.push_back(0);
            type.push_back('x');
            while(1)
            {
                getline(input, temp);
                if(temp == "#end func")
                    break;
                if(temp.find("max_args_num:")!= string::npos)
                    para_num = stoi(temp.substr(temp.find("max_args_num:")));
                if(temp.find('#')!= string::npos)
                    continue;
                instructions.push_back(temp);
                if(temp.find("lw") != string::npos&&temp.find("(sp)") != string::npos)
                {
                    int pos1 = temp.find(",") + 2;
                    int pos2 = temp.find("(");
                    pos.push_back(stoi(temp.substr(pos1, pos2 - pos1)));
                    type.push_back('l');
                }
                else if(temp.find("sw") != string::npos&&temp.find("(sp)") != string::npos)
                {
                    int pos1 = temp.find(",") + 2;
                    int pos2 = temp.find("(");
                    pos.push_back(stoi(temp.substr(pos1, pos2 - pos1)));
                    type.push_back('s');
                }
                else if(temp.find(":") != string::npos)
                {
                    pos.push_back(0);
                    type.push_back(':');
                }
                else
                {
                    pos.push_back(0);
                    type.push_back('x');
                }
            }
            auto useless_instructions = f(pos,type,para_num);
            int l = useless_instructions.size();
            for(int i=0;i<l;i++)
                if(!useless_instructions[i])
                {
                    all.push_back(instructions[i]);
                }
        }
        else
        {
            if(temp.find('#')!= string::npos)
                continue;
            all.push_back(temp);
        }
    }
    ofstream output("/root/compiler/perf.txt");
    for(auto & it:all)
        output << it <<'\n';
}
vector<bool> f(vector<int>& pos,vector<char>&type, int para_num)
{
    int l = pos.size();
    vector<bool>ret(l);
    set<int>cnt;
    //set<int>save;
    for(int i = l - 1; i >= 0; i--)
    {
        if(type[i] == 'l')
        {
            //save.erase(pos[i]);
            cnt.insert(pos[i]);
        }
        else if(type[i] == 's')
        {
            int position = pos[i];
            //if(save.count(position))\
                ret[i] = true;\
            else 
            if(position > para_num * 4)
            {
                if(!cnt.count(position))
                    ret[i] = true;
            }
        }
        //else if(type[i] == ':')\
            save.clear();
    }
    return ret;
}
void delete_command()
{
    ifstream input("/root/compiler/RISCV.txt");
    ofstream output("/root/compiler/RISCV_no_command.txt");
    while(1)
    {
        string temp;
        getline(input, temp);
        if(temp == "#end program")
            break;
        if(temp.find('#')!= string::npos)
            continue;
        output << temp <<"\n";
    }

}