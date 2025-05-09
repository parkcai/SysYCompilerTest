#pragma once
#include <string>
#include "koopa.h"

void Visit(const koopa_raw_program_t &program);//raw program
void Visit(const koopa_raw_slice_t &slice);//raw slice
void Visit(const koopa_raw_function_t &func);//函数
void Visit(const koopa_raw_basic_block_t &bb);//基本块
void Visit(const koopa_raw_value_t &value);//指令
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);//整数
void VisitBinary(const koopa_raw_value_t &value);//二元运算

void VisitAlloc(const koopa_raw_value_t &value);
void Visit(const koopa_raw_store_t &store);
void VisitLoad(const koopa_raw_value_t &value);

void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);

void VisitCall(const koopa_raw_value_t &value);
void VisitGlobalAlloc(const koopa_raw_value_t &value);

void VisitGetElemPtr(const koopa_raw_value_t &value);
void VisitGetPtr(const koopa_raw_value_t &value);