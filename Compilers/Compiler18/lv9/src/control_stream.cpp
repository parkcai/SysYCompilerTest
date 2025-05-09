#include <string>
#include <vector>
#include <unordered_map>
#include "control_stream.hpp"

using namespace std;

// 当前流是否已返回
static int return_flag = 0;

// 下一个block的id
static int current_block_id = 0;
// 当前穿过了哪些block，越后面作用域越小，优先级也就越高
static vector<int> block_chain;

int get_return_flag(){
    return return_flag;
}
void change_return_flag(int x){
    return_flag = x;
}
int get_current_block_id(){
    return block_chain.back();
}
void enter_code_block(){
    block_chain.push_back(current_block_id);
    current_block_id++;
}
void exit_code_block(){
    block_chain.pop_back();
}
vector<int> get_current_block_chain(){
    return block_chain;
}