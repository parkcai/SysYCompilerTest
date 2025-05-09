#include "optim.h"

void optimize_riscv_code() {
    std::vector<int> to_remove_idx;
    for(int i=0;i<riscv_code_list.size()-1;++i){
        if(riscv_code_list[i].find("sw")!=std::string::npos && riscv_code_list[i+1].find("lw")!=std::string::npos){
            std::string word;
            std::vector<std::string> sw,lw;
            std::stringstream ss(riscv_code_list[i]);
            while(ss>>word){
                sw.push_back(word);
            }
            std::stringstream ss2(riscv_code_list[i+1]);
            while(ss2>>word){
                lw.push_back(word);
            }
            if(sw[2]!=lw[2]){
               continue;
            }
            if(sw[1]==lw[1]){
                to_remove_idx.push_back(i+1);
            }
            else{
                riscv_code_list[i+1] = "\tmv "+lw[1].substr(0,2)+", "+sw[1].substr(0,2)+"\n";
            }
            i++;
        }
    }
    sort(to_remove_idx.rbegin(),to_remove_idx.rend());
    for(int idx:to_remove_idx){
        riscv_code_list.erase(riscv_code_list.begin()+idx);
    }
}