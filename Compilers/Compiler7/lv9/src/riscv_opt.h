#pragma once
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "koopa.h"
using namespace std;

struct instinfo{
    std::string opcode;
    std::string src;
    std::string dest;
    std::string line;
};

void RiscvOpt_load(std::ifstream& in, std::ostream& out){
    std::string line;
    std::vector<struct instinfo> insts;
    while(std::getline(in,line)){
        struct instinfo inst_now;
        inst_now.line =line;
        int start = 0;
        int flag = 0;
        std::vector<std::string> tmp;
        for(int i=0;i<line.size();i++){
            if(flag){
                if (line[i]==' '||line[i]==','){
                    tmp.push_back(line.substr(start, i-start));
                    flag = 0;
                }
            }
            else {
                if (line[i]!=' '&&line[i]!=','){
                    start = i;
                    flag = 1;
                }
            }
            
        }
        if(flag){
            tmp.push_back(line.substr(start));
        }
        if(!tmp.empty())
            {inst_now.opcode = tmp[0];}
        if (inst_now.opcode == "sw"){
            inst_now.src = tmp[1];
            inst_now.dest = tmp[2];
        }
        else if (inst_now.opcode == "lw"){
            inst_now.src = tmp[2];
            inst_now.dest = tmp[1];
        }
        insts.push_back(inst_now);
    }
    int save = 0;
    for (int i = 0; i<insts.size()-1;i++){
        if(save == 0){
            out<<insts[i].line<<"\n";
        }
        else if(save == 2){
            out<<"  mv    "<<insts[i].dest<<", "<<insts[i-1].src<<"\n";
        }
        else{

        }
        if (insts[i].opcode == "sw"&&insts[i+1].opcode == "lw"){
            if (insts[i].dest == insts[i+1].src){
                if (insts[i].src == insts[i+1].dest){
                    save = 1;
                }
                else{
                    save = 2;
                }
            }
        }
        else{
            save = 0;
        }
    }
    out<<endl;
}