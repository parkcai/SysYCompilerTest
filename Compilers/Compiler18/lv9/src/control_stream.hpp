#pragma once
#include <string>
using namespace std;

void enter_code_block();
void exit_code_block();
int get_current_block_id();
vector<int> get_current_block_chain();

int get_return_flag();
void change_return_flag(int x);

