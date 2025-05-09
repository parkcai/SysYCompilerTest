#pragma once

#include <vector>
#include "koopa.h"
#include "ast.hpp"
#include <string.h>
using namespace std;
void addItemToSlice(koopa_raw_slice_t &slice, void *item);
void addItemToSlice(koopa_raw_slice_t &slice, vector<const void*> &buffer);
void addItemToSlice(koopa_raw_slice_t &slice, vector<void*> &buffer);
koopa_raw_slice_t createSlice(koopa_raw_slice_item_kind_t kind);
koopa_raw_type_kind_t *createTypeKind(koopa_raw_type_tag_t tag);
koopa_raw_value_data_t *createValueData(
    koopa_raw_value_tag_t tag, 
    const char *name, 
    koopa_raw_type_t ty, 
    koopa_raw_slice_item_kind_t used_by_kind);
koopa_raw_function_data_t *createFuncData(
    const char *name, 
    koopa_raw_type_t ty, 
    koopa_raw_slice_item_kind_t params_kind, 
    koopa_raw_slice_item_kind_t bbs_kind);
koopa_raw_basic_block_data_t *createBasicBlockData(
    const char *name, 
    koopa_raw_slice_item_kind_t params_kind, 
    koopa_raw_slice_item_kind_t used_by_kind, 
    koopa_raw_slice_item_kind_t insts_kind);
koopa_raw_value_data_t *createIntegerValueData(int num);
koopa_raw_value_data_t *createBinaryValueData(koopa_raw_binary_op_t op, 
    koopa_raw_value_data_t *lhs, 
    koopa_raw_value_data_t *rhs);