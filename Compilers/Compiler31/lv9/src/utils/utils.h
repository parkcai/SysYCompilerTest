#ifndef UTILS_H
#define UTILS_H
#include "koopa.h"
#include <vector>
#include <string>
#include <map>
#include <cassert>
#define vec std::vector
#define map std::map
using str = std::string;

koopa_raw_slice_t koopa_slice_new(vec<const void *> slices, koopa_raw_slice_item_kind_t tag);
koopa_raw_slice_t koopa_slice_new(const void *slice, koopa_raw_slice_item_kind_t tag);
koopa_raw_slice_t koopa_slice_new(koopa_raw_slice_item_kind_t tag);
koopa_raw_value_data_t *koopa_jump_value_new(koopa_raw_basic_block_t tar);
koopa_raw_value_data_t *koopa_const_number_value_new(int num);
struct GetValueTable {
private:
    map<const void*, int>::iterator last_query;
public:
    map<const void*, int> table;
    int exist(const void *x) { last_query = table.find(x); return last_query != table.end(); }
    int get_value(const void *x) {
        assert(last_query != table.end());
        return last_query->second;
    }
    int set_value(const void *x, int y) {
        table[x] = y;
        return y;
    }
};
struct BlockManager {
    vec<const void*> insts;
    vec<const void*> *blocks; // 外部blocks指针
    void set_blocks(vec<const void*> *blocks) { this->blocks = blocks; }
    void insert_inst(const void *inst) { insts.push_back(inst); }
    void insert_block(koopa_raw_basic_block_data_t *block)
    {
        finish_block();
        block->insts.buffer = nullptr;
        block->insts.len = 0;
        blocks->push_back(block);
    }
    void finish_block()
    {
        if (blocks->empty()) return;
        // if (insts.empty()) {
        //     auto *block = (koopa_raw_basic_block_data_t*)blocks->back();
        //     block->insts = koopa_slice_new(insts, KOOPA_RSIK_VALUE);
        // } else {
            auto *block = (koopa_raw_basic_block_data_t*)blocks->back();
            int found = 0;
            for (auto i = 0; i < insts.size(); ++i) {
                auto inst = (koopa_raw_value_t)insts[i];
                if (inst->kind.tag == KOOPA_RVT_RETURN ||
                    inst->kind.tag == KOOPA_RVT_JUMP ||
                    inst->kind.tag == KOOPA_RVT_BRANCH) {
                    insts.resize(i + 1);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                // add a return stmt
                auto *ret = new koopa_raw_value_data_t();
                auto ty = new koopa_raw_type_kind_t();
                ty->tag = KOOPA_RTT_UNIT;
                ret->ty = ty;
                ret->name = nullptr;
                ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
                ret->kind.tag = KOOPA_RVT_RETURN;
                ret->kind.data.ret.value = koopa_const_number_value_new(0);
                insts.push_back(ret);
            }
            if (block->insts.buffer == nullptr)
                block->insts = koopa_slice_new(insts, KOOPA_RSIK_VALUE);
            insts.clear();
        // }
    }

};
enum SymbolValueType {
    SVT_CONST,
    SVT_VAR,
    SVT_FUNC,
    SVT_ARRAY,
    SVT_POINTER,
};
struct SymbolValue {
    SymbolValueType type;
    union {
        int const_value;
        koopa_raw_value_t var_value;
        koopa_raw_function_t func_value;
        koopa_raw_value_t array_value;
        koopa_raw_value_t pointer_value;
    } data;
    SymbolValue() {}
    SymbolValue(SymbolValueType type, int const_value) : type(type), data{.const_value = const_value} {}
    SymbolValue(SymbolValueType type, koopa_raw_value_t var_value) : type(type) {
        if (type == SVT_VAR) data.var_value = var_value;
        else if (type == SVT_ARRAY) data.array_value = var_value;
        else if (type == SVT_POINTER) data.pointer_value = var_value;
    }
    SymbolValue(SymbolValueType type, koopa_raw_function_t func_value) : type(type), data{.func_value = func_value} {}
};
struct SymbolTable {
    vec<map<str, SymbolValue>> scopes;
    void insert_symbol(str name, SymbolValue value) {
        assert(!scopes.empty());
        scopes.back()[name] = value;
    }
    SymbolValue get_symbol(str name) {
        assert(!scopes.empty());
        for (auto i = scopes.size() - 1; i >= 0; --i) {
            auto iter = scopes[i].find(name);
            if (iter != scopes[i].end())
                return iter->second;
        }
        assert(0);
    }
    void insert_scope() {
        scopes.push_back({});
    }
    void pop_back_scope() {
        scopes.pop_back();
    }
};
struct LoopStack {
    vec<std::pair<koopa_raw_basic_block_t, koopa_raw_basic_block_t> > loops;
    // first->while_entry block,   second->while_end block
    void push_loop(koopa_raw_basic_block_t while_entry, koopa_raw_basic_block_t while_end) {
        loops.push_back({while_entry, while_end});
    }
    koopa_raw_basic_block_t get_loop_entry() {
        assert(!loops.empty());
        return loops.back().first;
    }
    koopa_raw_basic_block_t get_loop_end() {
        assert(!loops.empty());
        return loops.back().second;
    }
    void pop_loop() {
        assert(!loops.empty());
        loops.pop_back();
    }
    // check if in loop
    bool in_loop() {
        return !loops.empty();
    }
};
#endif