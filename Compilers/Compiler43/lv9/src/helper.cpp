#include "helper.hpp"
using namespace std;

void addItemToSlice(koopa_raw_slice_t &slice, void *item) {
    auto newbuf = new const void *[slice.len + 1];
    if(slice.buffer != nullptr) {
        memcpy(newbuf, slice.buffer, sizeof(const void *) * slice.len);
        delete[] slice.buffer;
    }
        
    newbuf[slice.len++] = item;
    slice.buffer = newbuf;
}
void addItemToSlice(koopa_raw_slice_t &slice, vector<const void*> &buffer) {
    auto newbuf = new const void *[slice.len + buffer.size()];
    if(slice.buffer != nullptr) {
        memcpy(newbuf, slice.buffer, sizeof(const void *) * slice.len);
        delete[] slice.buffer;
    }

    copy(buffer.begin(), buffer.end(), newbuf + slice.len);
    slice.len += buffer.size();
    slice.buffer = newbuf;
}
void addItemToSlice(koopa_raw_slice_t &slice, vector<void*> &buffer) {
    auto newbuf = new const void *[slice.len + buffer.size()];
    if(slice.buffer != nullptr) {
        memcpy(newbuf, slice.buffer, sizeof(const void *) * slice.len);
        delete[] slice.buffer;
    }

    copy(buffer.begin(), buffer.end(), newbuf + slice.len);
    slice.len += buffer.size();
    slice.buffer = newbuf;
}
koopa_raw_slice_t createSlice(koopa_raw_slice_item_kind_t kind) {
    koopa_raw_slice_t raw = {
        nullptr,
        0,
        kind
    };
    return raw;
}
koopa_raw_type_kind_t *createTypeKind(koopa_raw_type_tag_t tag)  {
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = tag;
    return ty;
}
koopa_raw_value_data_t *createValueData(
    koopa_raw_value_tag_t tag, 
    const char *name, 
    koopa_raw_type_t ty, 
    koopa_raw_slice_item_kind_t used_by_kind) {
    auto raw = new koopa_raw_value_data_t;
    raw->kind.tag = tag;
    raw->ty = ty;

    if(name != nullptr) {
        auto n = new char[strlen(name) + 1];
        strcpy(n, name);
        raw->name = n;
    }
    else {
        raw->name = nullptr;
    }

    raw->used_by = createSlice(used_by_kind);
    return raw;
}
koopa_raw_function_data_t *createFuncData(
    const char *name, 
    koopa_raw_type_t ty, 
    koopa_raw_slice_item_kind_t params_kind, 
    koopa_raw_slice_item_kind_t bbs_kind) {
    auto raw = new koopa_raw_function_data_t;

    if(name != nullptr) {
        auto n = new char[strlen(name) + 1];
        strcpy(n, name);
        raw->name = n;
    }
    else {
        raw->name = nullptr;
    }
    raw->ty = ty;
    raw->params = createSlice(params_kind);
    raw->bbs = createSlice(bbs_kind);
    return raw;
}
koopa_raw_basic_block_data_t *createBasicBlockData(
    const char *name, 
    koopa_raw_slice_item_kind_t params_kind, 
    koopa_raw_slice_item_kind_t used_by_kind, 
    koopa_raw_slice_item_kind_t insts_kind) {
    auto raw = new koopa_raw_basic_block_data_t;
    

    if(name != nullptr) {
        string name_str = name;
        SymbolTable::getAvailableName(name_str);
        auto n = new char[name_str.length() + 1];
        strcpy(n, name_str.c_str());
        raw->name = n;
    }
    else {
        raw->name = nullptr;
    }
    raw->params = createSlice(params_kind);
    raw->used_by = createSlice(used_by_kind);
    raw->insts = createSlice(insts_kind);
    return raw;
}
koopa_raw_value_data_t *createIntegerValueData(int num) {
    auto raw = createValueData(KOOPA_RVT_INTEGER, nullptr, createTypeKind(KOOPA_RTT_INT32), KOOPA_RSIK_VALUE);
    raw->kind.data.integer.value = num;
    return raw;
}
koopa_raw_value_data_t *createBinaryValueData(koopa_raw_binary_op_t op, 
    koopa_raw_value_data_t *lhs, 
    koopa_raw_value_data_t *rhs) {
    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, createTypeKind(KOOPA_RTT_INT32), KOOPA_RSIK_VALUE);
    raw->kind.data.binary.op = op;
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    return raw;
}

