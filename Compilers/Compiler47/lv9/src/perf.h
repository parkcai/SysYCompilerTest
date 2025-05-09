#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include<assert.h>
using namespace std;

//map<string, pos>;
vector<bool> f(vector<int>& pos,vector<char>&type, int para_num);
void delete_command();

vector<string> instructions;
vector<int>pos;
vector<char>type;
map<int,string>pos2str;
map<string,int>str2pos;
vector<bool> g();
bool dfs(int k);

void perf() {
    delete_command();
    ifstream input("RISCV.txt");
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
            instructions.clear();
            pos.clear();
            type.clear();
            pos2str.clear();
            str2pos.clear();

            instructions.push_back(temp);
            pos.push_back(0);
            type.push_back('x');
            for(int i = 1;;)
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
                    str2pos[temp.substr(0,temp.find(":"))] = i;
                }
                else if(temp.find("j  ") != string::npos)
                {
                    pos.push_back(0);
                    type.push_back('j');
                    int length = temp.length();
                    pos2str[i] = temp.substr(temp.find("j  ")+6,length - temp.find("j  ") - 6);
                }
                else if(temp.find("bnez") != string::npos)
                {
                    pos.push_back(0);
                    type.push_back('b');
                    int length = temp.length();
                    pos2str[i] = temp.substr(temp.find(", ")+2,length - temp.find(", ") - 2);
                }
                else
                {
                    pos.push_back(0);
                    type.push_back('x');
                }
                i++;
            }
            auto useless_instructions = g();
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
    ofstream output("perf.txt");
    for(auto & it:all)
        output << it <<'\n';
}
vector<bool> f(vector<int>& pos,vector<char>&type, int para_num)
{
    int l = pos.size();
    vector<bool>ret(l);
    for(int i= 1;i<l;i++)
    {
        if(type[i] == 's'&&type[i-1]=='w'&&pos[i]==pos[i-1])
            ret[i] = 1;
    }
    return ret;


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
    ifstream input("RISCV.txt");
    ofstream output("RISCV_no_command.txt");
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
vector<int>son[20000];
bool visited[20000];
int sw_pos;
vector<bool> g()
{
    int l = pos.size();
    cerr<<"start g() l = "<<l<<endl;
    for(auto it: str2pos)
        cerr <<it.first<<" "<<it.second<<'\n';
    for(auto it: pos2str)
        cerr <<it.first<<" "<<it.second<<'\n';
    vector<bool>ret(l);
    assert(l <=20000);
    assert(type.size() == l);

    for(int i = 0;i<l;i++)
    {
        son[i].clear();
    }
    for(auto it: pos2str)
    {
        int position = it.first;
        string symbol = it.second;
        assert(str2pos.count(symbol));
        son[position].push_back(str2pos[symbol]);
        cerr<<"son["<<position<<"] = "<<str2pos[symbol]<<endl;
    }
    for(int i =0;i<l-1;i++)
    {
        if(type[i] != 'j')
            son[i].push_back(i+1);
    }
    for(int i=0;i<l;i++)
    {
        if(type[i]!= 's')
            continue;
        for(int j = 0;j < l; j++)
        {
            visited[j] = false;
        }
        sw_pos = pos[i];
        
        ret[i] = !dfs(i);
    }
    cerr<<"end g()"<<endl;
    return ret;
}
bool dfs(int k)
{
    if(visited[k])
        return 0;
    visited[k] = 1;
    if(type[k] == 'l'&&pos[k] == sw_pos)
        return 1;
    bool ret = 0;
    for(int kid:son[k])
        ret |= dfs(kid);
    return ret;
}