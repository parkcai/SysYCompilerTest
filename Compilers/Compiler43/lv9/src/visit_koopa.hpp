#pragma once
#include "koopa.h"
#include "helper.hpp"
#include "stack_frame.hpp"
#include <fstream>
#include <vector>
#include <stack>
#include <set>
#include <algorithm>

using namespace std;

void visit(ofstream &out, const koopa_raw_program_t raw);
void visit(ofstream &out, const koopa_raw_slice_t &slice);
void visit(ofstream &out, const koopa_raw_slice_t &slice, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_value_t &value);
void visit(ofstream &out, const koopa_raw_function_t &func);
void visit(ofstream &out, const koopa_raw_basic_block_t &bb, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_return_t &ret, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_integer_t &integer);
void visit(ofstream &out, const koopa_raw_binary_t &integer, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_load_t &load, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_store_t &store, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_branch_t &branch, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_jump_t &jump, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_call_t &call, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_get_elem_ptr_t &getelem, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_get_ptr_t &getptr, StackFrame &stackFrame);
void getStackLength(const koopa_raw_function_t &func, StackFrame &stackFrame);
int getArraySize(const koopa_raw_type_t &ty);
void prologue(ofstream &out, StackFrame &stackFrame);
void epilogue(ofstream &out, StackFrame &stackFrame);
string getReg(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame, bool checkNum = true);
int newReg();
void visitParams(ofstream &out, const koopa_raw_slice_t &params, StackFrame &stackFrame);
void preprocess(const koopa_raw_slice_t &bbs, StackFrame &stackFrame);