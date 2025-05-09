#include "utils.h"
#include <cstring>
#include <vector>
using std::vector;
using std::copy;
koopa_raw_value_data_t *koopa_jump_value_new(koopa_raw_basic_block_t tar) {    
    auto *jmp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    jmp->ty = ty;
    jmp->name = nullptr;
    jmp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    jmp->kind.tag = KOOPA_RVT_JUMP;
    jmp->kind.data.jump.args = koopa_slice_new(KOOPA_RSIK_VALUE);
    jmp->kind.data.jump.target = tar;
    return jmp;
}
koopa_raw_value_data_t *koopa_const_number_value_new(int num) {
    auto *val = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    val->ty = ty;
    val->name = nullptr;
    val->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    val->kind.tag = KOOPA_RVT_INTEGER;
    val->kind.data.integer.value = num;
    return val;
}
koopa_raw_slice_t koopa_slice_new(vector<const void *> slices, koopa_raw_slice_item_kind_t tag)
{
    auto ret = new koopa_raw_slice_t();
    ret->len = slices.size();
    ret->kind = tag;
    ret->buffer = new const void *[slices.size()];
    copy(slices.begin(), slices.end(), ret->buffer);
    return *ret;
}
koopa_raw_slice_t koopa_slice_new(const void *slice, koopa_raw_slice_item_kind_t tag)
{
    auto ret = new koopa_raw_slice_t();
    ret->len = 1;
    ret->kind = tag;
    ret->buffer = new const void *[1];
    ret->buffer[0] = slice;
    return *ret;
}
koopa_raw_slice_t koopa_slice_new(koopa_raw_slice_item_kind_t tag)
{
    auto ret = new koopa_raw_slice_t();
    ret->len = 0;
    ret->kind = tag;
    ret->buffer = nullptr;
    return *ret;
}
