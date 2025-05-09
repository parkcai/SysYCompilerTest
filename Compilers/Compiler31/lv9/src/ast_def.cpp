#include "ast_def.h"
#include "utils.h"
#include <cstdio>
#include <memory.h>
#include <cassert>
#include <typeinfo>
#include <functional>

static BlockManager block_manager;
static SymbolTable symbol_table;
static LoopStack loop_stack;
static GetValueTable get_value_table;

// ==================================================
// get_val() 方法实现
// ==================================================
int ExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    return get_value_table.set_value(this, exp->get_val());
}
int PrimaryExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    return exp->get_val();
}
int NumberAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    return val;
}
int UnaryExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "" || op == "+")
        return get_value_table.set_value(this, exp->get_val());
    if (op == "-")
        return get_value_table.set_value(this, -exp->get_val());
    if (op == "!")
        return get_value_table.set_value(this, !exp->get_val());
    assert(0);
}
int AddExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "")
        return get_value_table.set_value(this, exp2->get_val());
    if (op == "+")
        return get_value_table.set_value(this, exp1->get_val() + exp2->get_val());
    if (op == "-")
        return get_value_table.set_value(this, exp1->get_val() - exp2->get_val());
    return 0;
}
int MulExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "")
        return get_value_table.set_value(this, exp2->get_val());
    if (op == "*")
        return get_value_table.set_value(this, exp1->get_val() * exp2->get_val());
    if (op == "/")
        return get_value_table.set_value(this, exp1->get_val() / exp2->get_val());
    if (op == "%")
        return get_value_table.set_value(this, exp1->get_val() % exp2->get_val());
    return 0;
}
int RelExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "")
        return get_value_table.set_value(this, exp2->get_val());
    if (op == "<")
        return get_value_table.set_value(this, exp1->get_val() < exp2->get_val());
    if (op == "<=")
        return get_value_table.set_value(this, exp1->get_val() <= exp2->get_val());
    if (op == ">")
        return get_value_table.set_value(this, exp1->get_val() > exp2->get_val());
    if (op == ">=")
        return get_value_table.set_value(this, exp1->get_val() >= exp2->get_val());
    return 0;
}
int EqExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "")
        return get_value_table.set_value(this, exp2->get_val());
    if (op == "==")
        return get_value_table.set_value(this, exp1->get_val() == exp2->get_val());
    if (op == "!=")
        return get_value_table.set_value(this, exp1->get_val() != exp2->get_val());
    return 0;
}
int LAndExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "")
        return get_value_table.set_value(this, exp2->get_val());
    if (op == "&&")
        return get_value_table.set_value(this, exp1->get_val() && exp2->get_val());
    return 0;
}
int LOrExpAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    if (op == "")
        return get_value_table.set_value(this, exp2->get_val());
    if (op == "||")
        return get_value_table.set_value(this, exp1->get_val() || exp2->get_val());
    return 0;
}
int LValAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    auto value = symbol_table.get_symbol(ident);
    return get_value_table.set_value(this, value.data.const_value);
}
int ConstInitValAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    return get_value_table.set_value(this,  exp->get_val());
}
int InitValAST::get_val() const {
    if (get_value_table.exist(this)) return get_value_table.get_value(this);
    return get_value_table.set_value(this, exp->get_val());
}

// ==================================================
// to_koopa() 方法实现
// ==================================================
void *CompUnitAST::to_koopa() const {
    auto *raw_program = new koopa_raw_program_t();
    symbol_table.insert_scope();
    raw_program->values = koopa_slice_new(KOOPA_RSIK_VALUE);
    vec<const void *> funcs;
    vec<const void *> values;
    // add basic function
    auto add_basic_function = [&](str name) {        
        auto *raw_func = new koopa_raw_function_data_t();
        if (name == "getint") {
            // getint
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = koopa_slice_new(KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_INT32;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else if (name == "getch") {
            // getch
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = koopa_slice_new(KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_INT32;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else if (name == "getarray") {
            // getarray
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            vec<const void *> params_type;
            auto arg_ty = new koopa_raw_type_kind_t(), arg_base_ty = new koopa_raw_type_kind_t();
            arg_base_ty->tag = KOOPA_RTT_INT32;
            arg_ty->tag = KOOPA_RTT_POINTER;
            arg_ty->data.pointer.base = arg_base_ty;
            params_type.push_back(arg_ty);
            ty->data.function.params = koopa_slice_new(params_type, KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_INT32;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else if (name == "putint") {
            // putint
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            vec<const void *> params_type;
            auto arg_ty = new koopa_raw_type_kind_t();
            arg_ty->tag = KOOPA_RTT_INT32;
            params_type.push_back(arg_ty);
            ty->data.function.params = koopa_slice_new(params_type, KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_UNIT;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else if (name == "putch") {
            // putch
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            vec<const void *> params_type;
            auto arg_ty = new koopa_raw_type_kind_t();
            arg_ty->tag = KOOPA_RTT_INT32;
            params_type.push_back(arg_ty);
            ty->data.function.params = koopa_slice_new(params_type, KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_UNIT;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else if (name == "putarray") {
            // putarray
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            vec<const void *> params_type;

            auto arg_ty = new koopa_raw_type_kind_t();
            arg_ty->tag = KOOPA_RTT_INT32;
            params_type.push_back(arg_ty); // array len

            arg_ty = new koopa_raw_type_kind_t();
            auto arg_base_ty = new koopa_raw_type_kind_t();
            arg_base_ty->tag = KOOPA_RTT_INT32;
            arg_ty->tag = KOOPA_RTT_POINTER;
            arg_ty->data.pointer.base = arg_base_ty;
            params_type.push_back(arg_ty); // array pointer

            ty->data.function.params = koopa_slice_new(params_type, KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_UNIT;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);

        } else if (name == "starttime") {
            // starttime
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = koopa_slice_new(KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_UNIT;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else if (name == "stoptime") {
            // stoptime
            auto ty = new koopa_raw_type_kind_t(), ret_ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = koopa_slice_new(KOOPA_RSIK_TYPE);
            ret_ty->tag = KOOPA_RTT_UNIT;
            ty->data.function.ret = ret_ty;
            raw_func->ty = ty;
            raw_func->bbs = koopa_slice_new(KOOPA_RSIK_BASIC_BLOCK);
            raw_func->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        } else {
            assert(0);
        }

        char *temp_name = new char[name.length() + 2];
        strcpy(temp_name, ("@"+name).c_str());
        raw_func->name = temp_name;
        symbol_table.insert_symbol(name, SymbolValue(SymbolValueType::SVT_FUNC, raw_func));
        funcs.push_back(raw_func);
    };
    vec<str> basic_function = {"getint", "getch", "getarray", "putint", "putch", "putarray", "starttime", "stoptime"};
    for (auto &basic_function_name : basic_function) add_basic_function(basic_function_name);

    for (auto &item : *arr) {
        if (item->show_type() == "FuncDefAST")
            funcs.push_back(item->to_koopa());
        else
            item->to_koopa(&values);
    }
    raw_program->values = koopa_slice_new(values, KOOPA_RSIK_VALUE);
    raw_program->funcs = koopa_slice_new(funcs, KOOPA_RSIK_FUNCTION);
    symbol_table.pop_back_scope();
    return raw_program;
}
void *FuncDefAST::to_koopa() const {
    auto *raw_func = new koopa_raw_function_data_t();
    // add symbol
    SymbolValue func_value(SymbolValueType::SVT_FUNC, raw_func);
    symbol_table.insert_symbol(ident, func_value);
    // Type of function
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    // set params_type in function_ty
    vec<const void *> params_type;
    for (auto &item : *arr)
        params_type.push_back(item->to_type());
    ty->data.function.params = koopa_slice_new(params_type, KOOPA_RSIK_TYPE);
    // set return
    ty->data.function.ret = (koopa_raw_type_kind*)func_type->to_koopa();
    raw_func->ty = ty;
    // Name
    auto name = new char[ident.length() + 2];
    memcpy(name, ("@" + ident).c_str(), ident.length() + 2);
    raw_func->name = name;
    // Params
    vec<const void *> params;
    for (int idx = 0; idx < arr->size(); ++idx)
        params.push_back(arr->at(idx)->to_koopa(idx));
    raw_func->params = koopa_slice_new(params, KOOPA_RSIK_VALUE);
    // entry, basic_blocks
    auto blocks = *new vec<const void *>();
    block_manager.set_blocks(&blocks);
    auto *entry = new koopa_raw_basic_block_data_t();
    entry->name = "%entry";
    entry->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    entry->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);

    symbol_table.insert_scope();
    block_manager.insert_block(entry);
    // store params in stack
    for (auto &item : params) {
        // first allocate memory for params
        auto *alloc = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = ((koopa_raw_value_t)item)->ty;
        alloc->ty = ty;
        alloc->name = ((koopa_raw_value_t)item)->name;
        alloc->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        alloc->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.insert_inst(alloc);
        if (((koopa_raw_value_t)item)->ty->tag == KOOPA_RTT_INT32)
            symbol_table.insert_symbol(str(alloc->name+1), SymbolValue(SymbolValueType::SVT_VAR, alloc));
        else if (((koopa_raw_value_t)item)->ty->tag == KOOPA_RTT_POINTER || ((koopa_raw_value_t)item)->ty->tag == KOOPA_RTT_ARRAY) {
            symbol_table.insert_symbol(str(alloc->name+1), SymbolValue(SymbolValueType::SVT_POINTER, alloc));
        }
        // second, store in stack
        auto *store = new koopa_raw_value_data_t();
        ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_UNIT;
        store->ty = ty;
        store->name = nullptr;
        store->kind.data.store.dest = alloc;
        store->kind.data.store.value = (koopa_raw_value_t)item;
        store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        store->kind.tag = KOOPA_RVT_STORE;
        block_manager.insert_inst(store);
    }
    block->to_koopa();//函数block
    // add a return stmt in case of no return
    auto *ret = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    ret->ty = ty;
    ret->name = nullptr;
    ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_RETURN;
    auto ret_type = ((koopa_raw_type_t)func_type->to_koopa())->tag;
    if (ret_type == KOOPA_RTT_INT32)
        ret->kind.data.ret.value = koopa_const_number_value_new(0);
    else if (ret_type == KOOPA_RTT_UNIT);
    block_manager.insert_inst(ret);
    symbol_table.pop_back_scope();
    block_manager.finish_block();
    for (int i = 1; i < blocks.size(); ++i) {
        auto *block = (koopa_raw_basic_block_data_t*)blocks[i];
        str block_name = "%" + ident + "_" + (block->name + 1);
        char *name = new char[block_name.length() + 1];
        strcpy(name, block_name.c_str());
        block->name = name;
    }

    raw_func->bbs = koopa_slice_new(blocks, KOOPA_RSIK_BASIC_BLOCK);
    return raw_func;
}
void *FuncTypeAST::to_koopa() const {
    auto *raw_ty = new koopa_raw_type_kind_t();
    if (ident == "int")
        raw_ty->tag = KOOPA_RTT_INT32;
    else
        raw_ty->tag = KOOPA_RTT_UNIT;
    return raw_ty;
}
void *FuncFParamAST::to_type() const {
    if (arr != nullptr && arr->size()) {
        //array
        auto ty = new koopa_raw_type_kind_t();
        auto *next_array = new koopa_raw_type_kind_t();
        auto next_array_ty = new koopa_raw_type_kind_t();
        next_array->tag = KOOPA_RTT_INT32;
        // next_array->data.array.len = arr->size() == 1 ? 100 : arr->back()->get_val();
        // next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        for (int i = int(arr->size()) - 1; i >= 1; --i) {
            auto *now_array = new koopa_raw_type_kind_t();
            now_array->tag = KOOPA_RTT_ARRAY;
            if (i == 0)
                now_array->data.array.len = 100;
            else
                now_array->data.array.len = arr->at(i)->get_val();
            now_array->data.array.base = next_array;
            next_array = now_array;
        }
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = next_array;
        return ty;
    }
    return btype->to_koopa();
}
void *FuncFParamAST::to_koopa(int index) const {
    auto *ret = new koopa_raw_value_data_t();
    if (arr != nullptr && arr->size()) {
        //array
        auto ty = new koopa_raw_type_kind_t();
        auto *next_array = new koopa_raw_type_kind_t();
        auto next_array_ty = new koopa_raw_type_kind_t();
        next_array->tag = KOOPA_RTT_INT32;
        // next_array->data.array.len = arr->size() == 1 ? 100 : arr->back()->get_val();
        // next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        for (int i = int(arr->size()) - 1; i >= 1; --i) {
            auto *now_array = new koopa_raw_type_kind_t();
            now_array->tag = KOOPA_RTT_ARRAY;
            if (i == 0)
                now_array->data.array.len = 114;
            else
                now_array->data.array.len = arr->at(i)->get_val();
            now_array->data.array.base = next_array;
            next_array = now_array;
        }
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = next_array;
        ret->ty = ty;
    } else {
        // int param
        ret->ty = (koopa_raw_type_t)btype->to_koopa();
    }
    auto name = new char[ident.length() + 2];
    memcpy(name, ("@" + ident).c_str(), ident.length() + 2);
    ret->name = name;
    ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
    ret->kind.data.func_arg_ref.index = index;
    return ret;
}
void *BlockAST::to_koopa() const {
    for (auto &item : *arr) item->to_koopa();
    return nullptr;
}
void *StmtAST::to_koopa() const {
    auto *ret = new koopa_raw_value_data_t();
    // Type of Value
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    ret->ty = ty;
    // Name
    ret->name = nullptr;
    // used_by
    ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    if (kind == RETURN) {
        auto *raw_val = new koopa_raw_value_data_t();
        // Type of Value
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_UNIT;
        raw_val->ty = ty;
        // Name
        raw_val->name = nullptr;
        // used_by
        raw_val->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        // kind of value
        if (exp != nullptr)
            raw_val->kind.data.ret.value = (koopa_raw_value_t)exp->to_koopa();
        raw_val->kind.tag = KOOPA_RVT_RETURN;
        block_manager.insert_inst(raw_val);
        return raw_val;
    } else if (kind == ASSIGN) {
        auto *store = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_UNIT;
        store->ty = ty;
        store->name = nullptr;
        store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        store->kind.tag = KOOPA_RVT_STORE;
        if (stmt != nullptr) ;//cout << "assign : " << (koopa_raw_value_t)stmt->to_koopa_leftvalue() << endl;
        if (exp != nullptr) ;//cout << "value:   " << exp->get_val() << endl;
        if (stmt != nullptr) ;//cout << "tag:     " << ((koopa_raw_value_t)stmt->to_koopa_leftvalue())->ty->tag << endl;
        store->kind.data.store.dest = (koopa_raw_value_t)stmt->to_koopa_leftvalue();
        store->kind.data.store.value = (koopa_raw_value_t)exp->to_koopa();
        block_manager.insert_inst(store);
        return store;
    } else if (kind == EXP) {
        symbol_table.insert_scope();
        exp->to_koopa();
        symbol_table.pop_back_scope();
    } else if (kind == EMPTY) {
        // return nullptr;
    } else if (kind == BLOCK) {
        symbol_table.insert_scope();
        stmt->to_koopa();
        symbol_table.pop_back_scope();
    } else if (kind == IF) {
        auto *ret = (koopa_raw_value_data_t*)exp->to_koopa();
        auto *bb = new koopa_raw_basic_block_data_t();
        bb->name = "%false";
        bb->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        bb->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.data.branch.false_bb = bb;
        ret->kind.data.branch.false_args = koopa_slice_new(KOOPA_RSIK_VALUE);
        if (stmt == nullptr) {
            auto *jmp = koopa_jump_value_new(bb);
            block_manager.insert_inst(jmp);
            block_manager.insert_block(bb);
        } else {
            auto *end = new koopa_raw_basic_block_data_t();
            end->name = "%end";
            end->params = koopa_slice_new(KOOPA_RSIK_VALUE);
            end->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            auto *jmp = koopa_jump_value_new(end);
            block_manager.insert_inst(jmp); // jump to end block at the end of true block
            block_manager.insert_block(bb); // false block
            symbol_table.insert_scope();
            stmt->to_koopa();
            symbol_table.pop_back_scope();
            block_manager.insert_inst(koopa_jump_value_new(end)); // jump to end block at the end of false block
            printf("%d\n", block_manager.blocks->size());
            block_manager.insert_block(end);
            printf("%d\n", block_manager.blocks->size());
        }
    } else if (kind == WHILE) {
        auto *while_entry = new koopa_raw_basic_block_data_t();
        while_entry->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        while_entry->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        while_entry->name = "%while_loop_cond";

        auto *jmp = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_UNIT;
        jmp->ty = ty;
        jmp->name = nullptr;
        jmp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        auto kindtmp = &jmp->kind;
        kindtmp->tag = KOOPA_RVT_JUMP;
        kindtmp->data.jump.args = koopa_slice_new(KOOPA_RSIK_VALUE);
        kindtmp->data.jump.target = while_entry;
        // first jump to while_entry
        block_manager.insert_inst(jmp);
        block_manager.insert_block(while_entry);
        // start of while_entry block
        ret->kind.tag = KOOPA_RVT_BRANCH;
        ret->kind.data.branch.cond = (koopa_raw_value_t)exp->to_koopa();
        auto *while_body = new koopa_raw_basic_block_data_t();
        while_body->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        while_body->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        while_body->name = "%while_loop_body";
        ret->kind.data.branch.true_bb = while_body;
        ret->kind.data.branch.true_args = koopa_slice_new(KOOPA_RSIK_VALUE);
        auto *while_end = new koopa_raw_basic_block_data_t();
        while_end->params = koopa_slice_new(KOOPA_RSIK_VALUE);
        while_end->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        while_end->name = "%while_loop_end";
        ret->kind.data.branch.false_args = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.data.branch.false_bb = while_end;
        loop_stack.push_loop(while_entry, while_end);// insert entry and end block into loop_stack for break and continue command
        
        printf("%d\n", loop_stack.loops.size());
        block_manager.insert_inst(ret);
        block_manager.insert_block(while_body);

        // start of while body
        stmt->to_koopa();

        block_manager.insert_inst(koopa_jump_value_new(while_entry));// back to while_entry
        block_manager.insert_block(while_end);
        loop_stack.pop_loop();
        printf("%d\n", loop_stack.loops.size());
        // start of while end
    } else if (kind == CONTINUE) {
        // jump to while_entry
        assert(loop_stack.in_loop());
        block_manager.insert_inst(koopa_jump_value_new(loop_stack.get_loop_entry()));
    } else if (kind == BREAK) {
        // jump to while_end
        assert(loop_stack.in_loop());
        block_manager.insert_inst(koopa_jump_value_new(loop_stack.get_loop_end()));
    }
    return ret;
}
void *IfAST::to_koopa() const {
    auto *ret = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    ret->ty = ty;
    ret->name = nullptr;
    ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_BRANCH;
    ret->kind.data.branch.cond = (koopa_raw_value_t)exp->to_koopa();
    auto *bb = new koopa_raw_basic_block_data_t();
    ret->kind.data.branch.true_bb = bb;
    ret->kind.data.branch.true_args = koopa_slice_new(KOOPA_RSIK_VALUE);
    bb->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    bb->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    bb->name = "%true";
    block_manager.insert_inst(ret);
    block_manager.insert_block(bb);
    stmt->to_koopa();
    return ret;
}
void *NumberAST::to_koopa() const {
    auto *raw_val = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    raw_val->ty = ty;
    raw_val->name = nullptr;
    raw_val->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    raw_val->kind.tag = KOOPA_RVT_INTEGER;
    raw_val->kind.data.integer.value = val;
    return raw_val;
}
void *ExpAST::to_koopa() const {
    return exp->to_koopa();
}
void *PrimaryExpAST::to_koopa() const {
    return exp->to_koopa();
}
void *UnaryExpAST::to_koopa() const {
    if (op == "" || op == "+") {
        // PrimaryExp or + exp
        return exp->to_koopa();
    }
    if (op != "-" && op != "!") {
        // function call
        auto *ret = new koopa_raw_value_data_t();
        auto func = symbol_table.get_symbol(op);
        ret->ty = func.data.func_value->ty->data.function.ret;
        ret->name = nullptr;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_CALL;
        ret->kind.data.call.callee = func.data.func_value;
        vec<const void *> args;
        for (auto &arg : *arr) args.push_back(arg->to_koopa());
        ret->kind.data.call.args = koopa_slice_new(args, KOOPA_RSIK_VALUE);
        block_manager.insert_inst(ret);
        return ret;
    }
    auto *unary_exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    unary_exp->ty = ty;
    unary_exp->kind.tag = KOOPA_RVT_BINARY;
    unary_exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    unary_exp->name = nullptr;
    auto &binary = unary_exp->kind.data.binary;
    if (op != "-" && op != "!") {
        printf("op is %s\n", op.c_str());
        assert(0);
    }
    if (op == "-") {
        binary.op = KOOPA_RBO_SUB;
        binary.lhs = (koopa_raw_value_t)NumberAST(0).to_koopa();
        binary.rhs = (koopa_raw_value_t)exp->to_koopa();
    } else if (op == "!") {
        binary.op = KOOPA_RBO_EQ;
        binary.lhs = (koopa_raw_value_t)NumberAST(0).to_koopa();
        binary.rhs = (koopa_raw_value_t)exp->to_koopa();
    } else
        assert(0);
    block_manager.insert_inst(unary_exp);
    return unary_exp;
}
void *AddExpAST::to_koopa() const {
    if (op == "")
        return exp2->to_koopa();
    else {
        assert(exp1 != nullptr);
        assert(exp2 != nullptr);
    }
    auto *exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    exp->ty = ty;
    exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    exp->kind.tag = KOOPA_RVT_BINARY;
    exp->name = nullptr;
    auto binary = &exp->kind.data.binary;
    if (op == "+")
        binary->op = KOOPA_RBO_ADD;
    else if (op == "-")
        binary->op = KOOPA_RBO_SUB;
    binary->lhs = (koopa_raw_value_t)exp1->to_koopa();
    binary->rhs = (koopa_raw_value_t)exp2->to_koopa();
    block_manager.insert_inst(exp);
    return exp;
}
void *MulExpAST::to_koopa() const {
    if (op == "")
        return exp2->to_koopa();
    else {
        assert(exp1 != nullptr);
        assert(exp2 != nullptr);
    }
    auto *exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    exp->ty = ty;
    exp->name = nullptr;
    exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    exp->kind.tag = KOOPA_RVT_BINARY;
    auto *binary = &exp->kind.data.binary;
    if (op == "*")
        binary->op = KOOPA_RBO_MUL;
    else if (op == "/")
        binary->op = KOOPA_RBO_DIV;
    else if (op == "%")
        binary->op = KOOPA_RBO_MOD;
    binary->lhs = (koopa_raw_value_t)exp1->to_koopa();
    binary->rhs = (koopa_raw_value_t)exp2->to_koopa();
    block_manager.insert_inst(exp);
    return exp;
}
void *RelExpAST::to_koopa() const {
    if (op == "")
        return exp2->to_koopa();
    else {
        assert(exp1 != nullptr);
        assert(exp2 != nullptr);
    }
    auto *exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    exp->ty = ty;
    exp->name = nullptr;
    exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    exp->kind.tag = KOOPA_RVT_BINARY;
    auto *binary = &exp->kind.data.binary;
    if (op == "<")
        binary->op = KOOPA_RBO_LT;
    else if (op == "<=")
        binary->op = KOOPA_RBO_LE;
    else if (op == ">")
        binary->op = KOOPA_RBO_GT;
    else if (op == ">=")
        binary->op = KOOPA_RBO_GE;
    binary->lhs = (koopa_raw_value_t)exp1->to_koopa();
    binary->rhs = (koopa_raw_value_t)exp2->to_koopa();
    block_manager.insert_inst(exp);
    return exp;    
}
void *EqExpAST::to_koopa() const {
    if (op == "")
        return exp2->to_koopa();
    else {
        assert(exp1 != nullptr);
        assert(exp2 != nullptr);
    }
    auto *exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    exp->ty = ty;
    exp->name = nullptr;
    exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    exp->kind.tag = KOOPA_RVT_BINARY;
    auto binary = &exp->kind.data.binary;
    if (op == "==")
        binary->op = KOOPA_RBO_EQ;
    else if (op == "!=")
        binary->op = KOOPA_RBO_NOT_EQ;
    binary->lhs = (koopa_raw_value_t)exp1->to_koopa();
    binary->rhs = (koopa_raw_value_t)exp2->to_koopa();
    block_manager.insert_inst(exp);
    return exp;
}
void *to_bool(koopa_raw_value_t val) {
    auto *exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    exp->ty = ty;
    exp->name = nullptr;
    exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    exp->kind.tag = KOOPA_RVT_BINARY;
    auto binary = &exp->kind.data.binary;
    binary->op = KOOPA_RBO_NOT_EQ;
    binary->lhs = (koopa_raw_value_t)NumberAST(0).to_koopa();
    binary->rhs = (koopa_raw_value_t)val;
    block_manager.insert_inst(exp);
    return exp;
}
void *LAndExpAST::to_koopa() const {
    if (op == "")
        return exp2->to_koopa();     
    else {
        assert(exp1 != nullptr);
        assert(exp2 != nullptr);
    }   
    // alloc i32 for temp value
    auto *temp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t(), ty2 = new koopa_raw_type_kind_t();
    ty2->tag = KOOPA_RTT_INT32;
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = ty2; //INT32
    temp->ty = ty;
    temp->name = "@LAnd_temp";
    temp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    temp->kind.tag = KOOPA_RVT_ALLOC;
    block_manager.insert_inst(temp);
    symbol_table.insert_symbol("LAnd_temp", SymbolValue(SymbolValueType::SVT_VAR, temp));
    // init temp value with 0
    auto *init = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    init->ty = ty;
    init->name = nullptr;
    init->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    init->kind.tag = KOOPA_RVT_STORE;
    init->kind.data.store.value = (koopa_raw_value_t)NumberAST(0).to_koopa();
    init->kind.data.store.dest = (koopa_raw_value_t)temp;
    block_manager.insert_inst(init);
    // branch to check exp1
    auto *branch = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    branch->ty = ty;
    branch->name = nullptr;
    branch->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    branch->kind.tag = KOOPA_RVT_BRANCH;
    branch->kind.data.branch.cond = (koopa_raw_value_t)to_bool((koopa_raw_value_t)exp1->to_koopa());
    auto *true_block = new koopa_raw_basic_block_data_t();
    true_block->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    true_block->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    true_block->name = "%LAndExp_true_block_";
    branch->kind.data.branch.true_bb = true_block;
    branch->kind.data.branch.true_args = koopa_slice_new(KOOPA_RSIK_VALUE);
    auto *end_block = new koopa_raw_basic_block_data_t();
    end_block->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    end_block->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    end_block->name = "%LAndExp_end_block_";
    branch->kind.data.branch.false_bb = end_block;
    branch->kind.data.branch.false_args = koopa_slice_new(KOOPA_RSIK_VALUE);
    block_manager.insert_inst(branch);
    block_manager.insert_block(true_block);
    // in true_block
    // let temp = exp2
    auto *true_store = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    true_store->ty = ty;
    true_store->name = nullptr;
    true_store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    true_store->kind.tag = KOOPA_RVT_STORE;
    true_store->kind.data.store.value = (koopa_raw_value_t)to_bool((koopa_raw_value_t)exp2->to_koopa());
    true_store->kind.data.store.dest = (koopa_raw_value_t)temp;
    block_manager.insert_inst(true_store);
    block_manager.insert_inst(koopa_jump_value_new(end_block));
    block_manager.insert_block(end_block);
    // in end_block
    // load temp value to ret
    auto *ret = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    ret->ty = ty;
    ret->name = nullptr;
    ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_LOAD;
    ret->kind.data.load.src = (koopa_raw_value_t)temp;
    block_manager.insert_inst(ret);
    return ret;
}
void *LOrExpAST::to_koopa() const {
    if (op == "")
        return exp2->to_koopa();      
    else {
        assert(exp1 != nullptr);
        assert(exp2 != nullptr);
    }  
    // alloc i32 for temp value
    auto *temp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t(), ty2 = new koopa_raw_type_kind_t();
    ty2->tag = KOOPA_RTT_INT32;
    ty->tag = KOOPA_RTT_POINTER;
    ty->data.pointer.base = ty2; //INT32
    temp->ty = ty;
    temp->name = "@LOr_temp";
    temp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    temp->kind.tag = KOOPA_RVT_ALLOC;
    block_manager.insert_inst(temp);
    symbol_table.insert_symbol("LOr_temp", SymbolValue(SymbolValueType::SVT_VAR, temp));
    // init temp value with 1
    auto *init = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    init->ty = ty;
    init->name = nullptr;
    init->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    init->kind.tag = KOOPA_RVT_STORE;
    init->kind.data.store.value = (koopa_raw_value_t)NumberAST(1).to_koopa();
    init->kind.data.store.dest = (koopa_raw_value_t)temp;
    block_manager.insert_inst(init);
    // branch to check exp1
    auto *branch = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    branch->ty = ty;
    branch->name = nullptr;
    branch->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    branch->kind.tag = KOOPA_RVT_BRANCH;
    branch->kind.data.branch.cond = (koopa_raw_value_t)to_bool((koopa_raw_value_t)exp1->to_koopa());
    auto *end_block = new koopa_raw_basic_block_data_t();
    branch->kind.data.branch.true_bb = end_block;
    branch->kind.data.branch.true_args = koopa_slice_new(KOOPA_RSIK_VALUE);
    end_block->name = "%LOrExp_end_block_";
    end_block->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    end_block->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    auto *false_block = new koopa_raw_basic_block_data_t();
    branch->kind.data.branch.false_bb = false_block;
    branch->kind.data.branch.false_args = koopa_slice_new(KOOPA_RSIK_VALUE);
    false_block->name = "%LOrExp_false_block_";
    false_block->params = koopa_slice_new(KOOPA_RSIK_VALUE);
    false_block->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    block_manager.insert_inst(branch);
    block_manager.insert_block(false_block);
    // in false_block
    // let temp = exp2
    auto *false_store = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    false_store->ty = ty;
    false_store->name = nullptr;
    false_store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    false_store->kind.tag = KOOPA_RVT_STORE;
    false_store->kind.data.store.value = (koopa_raw_value_t)to_bool((koopa_raw_value_t)exp2->to_koopa());
    false_store->kind.data.store.dest = (koopa_raw_value_t)temp;
    block_manager.insert_inst(false_store);
    block_manager.insert_inst(koopa_jump_value_new(end_block));
    block_manager.insert_block(end_block);
    // in end_block
    // load temp value to ret
    auto *ret = new koopa_raw_value_data_t();
    ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_UNIT;
    ret->ty = ty;
    ret->name = nullptr;
    ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_LOAD;
    ret->kind.data.load.src = (koopa_raw_value_t)temp;
    block_manager.insert_inst(ret);
    return ret;
}
// lv4.1
void *ConstDeclAST::to_koopa() const {
    auto type = koopa_raw_type_t(btype->to_koopa());
    for (auto &def : *arr) def->to_koopa(type);
    return nullptr;
}
void *ConstDeclAST::to_koopa(vec<const void *> *global) const {
    auto type = koopa_raw_type_t(btype->to_koopa());
    for (auto &def : *arr) def->to_koopa(type, global);
    return nullptr;
}
void *BTypeAST::to_koopa() const {   
    auto *raw_ty = new koopa_raw_type_kind_t();
    raw_ty->tag = KOOPA_RTT_INT32;
    return raw_ty;
}
void *ConstDefAST::to_koopa(koopa_raw_type_t type) const {
    if (kind == 0) {
        //var
        SymbolValue val = SymbolValue(SymbolValueType::SVT_CONST, exp->get_val());
        // printf("*****:%s %d\n", ident.c_str(), exp->get_val());
        symbol_table.insert_symbol(ident, val);
        return nullptr;
    } else {
        // array
        vec<int> dims;
        int sz = 1;
        for (auto &dim : *arr) {
            dims.push_back(dim->get_val());
            sz *= dim->get_val();
        }
        // allocate for array
        auto *ret = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        
        auto *next_array = new koopa_raw_type_kind_t();
        auto next_array_ty = new koopa_raw_type_kind_t();
        next_array->tag = KOOPA_RTT_ARRAY;
        next_array->data.array.len = dims.back();
        next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        for (int i = int(dims.size()) - 2; i >= 0; --i) {
            auto *now_array = new koopa_raw_type_kind_t();
            now_array->tag = KOOPA_RTT_ARRAY;
            now_array->data.array.len = dims[i];
            now_array->data.array.base = next_array;
            next_array = now_array;
        }
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = next_array;
        ret->ty = ty;
        char *temp_name = new char[ident.length() + 2];
        strcpy(temp_name, ("@"+ident).c_str());
        ret->name = temp_name;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.insert_inst(ret);
        symbol_table.insert_symbol(ident, SymbolValue(SymbolValueType::SVT_ARRAY, ret));

        // get init array value
        auto initval = (ConstInitValAST *)(exp.get());
        auto *arr_init = new vec<const void *>();
        if (initval == nullptr) {
            for (int i = 0; i < sz; ++i) arr_init->push_back(NumberAST(0).to_koopa());
        } else {
            initval->get_arr_init(arr_init, dims);
        }
        // init
        vec<koopa_raw_value_t> ptr_stack;
        ptr_stack.push_back(ret); // 原数组
        for (int i = 0; i < sz; ++i) {
            int sz_of_j = sz, now = i, idx = 0;
            for (int j = 0; j < dims.size(); ++j) {
                sz_of_j /= dims[j];
                idx = now / sz_of_j;
                now %= sz_of_j;
                if (j + 1 < ptr_stack.size()) {
                    if (ptr_stack[j + 1]->kind.data.get_elem_ptr.index->kind.data.integer.value == idx) continue;
                    else while (j + 1 < ptr_stack.size()) ptr_stack.pop_back();
                }
                // get index pointer 
                auto *index = new koopa_raw_value_data_t();
                ty = new koopa_raw_type_kind_t();
                ty->tag = KOOPA_RTT_INT32;
                index->ty = ty;
                index->name = nullptr;
                index->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
                index->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                index->kind.data.get_elem_ptr.src = ptr_stack[j];
                index->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(idx).to_koopa();
                block_manager.insert_inst(index);
                ptr_stack.push_back(index);
            }
            // store
            auto *store = new koopa_raw_value_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_UNIT;
            store->ty = ty;
            store->name = nullptr;
            store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            store->kind.tag = KOOPA_RVT_STORE;
            // auto initval = (ConstInitValAST *)(exp.get());
            // if (initval == nullptr || initval->arr == nullptr || initval->arr->size() <= i)
            //     store->kind.data.store.value = (koopa_raw_value_t)NumberAST(0).to_koopa();
            // else
            //     store->kind.data.store.value = (koopa_raw_value_t)NumberAST(initval->arr->at(i)->get_val()).to_koopa();
            store->kind.data.store.value = (koopa_raw_value_t)arr_init->at(i);
            store->kind.data.store.dest = (koopa_raw_value_t)ptr_stack[dims.size()];
            block_manager.insert_inst(store);
        }
        return nullptr;
    }
    assert(0);
}
void *ConstDefAST::to_koopa(koopa_raw_type_t type, vec<const void *> *global) const {
    if (kind == 0) {
        //var
        SymbolValue val = SymbolValue(SymbolValueType::SVT_CONST, exp->get_val());
        // printf("*****:%s %d\n", ident.c_str(), exp->get_val());
        symbol_table.insert_symbol(ident, val);
        return nullptr;
    } else {
        // array
        vec<int> dims;
        int sz = 1;
        for (auto &dim : *arr) {
            dims.push_back(dim->get_val());
            sz *= dim->get_val();
        }
        // allocate for array
        auto *ret = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        
        auto *next_array = new koopa_raw_type_kind_t();
        auto next_array_ty = new koopa_raw_type_kind_t();
        next_array->tag = KOOPA_RTT_ARRAY;
        next_array->data.array.len = dims.back();
        next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        for (int i = int(dims.size()) - 2; i >= 0; --i) {
            auto *now_array = new koopa_raw_type_kind_t();
            now_array->tag = KOOPA_RTT_ARRAY;
            now_array->data.array.len = dims[i];
            now_array->data.array.base = next_array;
            next_array = now_array;
        }
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = next_array;
        ret->ty = ty;
        char *temp_name = new char[ident.length() + 2];
        strcpy(temp_name, ("@"+ident).c_str());
        ret->name = temp_name;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        global->push_back(ret);
        symbol_table.insert_symbol(ident, SymbolValue(SymbolValueType::SVT_ARRAY, ret));
        
        // get init array value
        auto initval = (ConstInitValAST *)(exp.get());
        auto *arr_init = new vec<const void *>();
        if (initval == nullptr) {
            for (int i = 0; i < sz; ++i) arr_init->push_back(NumberAST(0).to_koopa());
        } else {
            initval->get_arr_init(arr_init, dims);
        }
        reverse(arr_init->begin(), arr_init->end());
        function<void*(int)> build_array = [&](int dim_idx) -> void* {
            auto *array = new vec<const void *>();
            auto *array_dim = new vec<int>();
            if (dim_idx == dims.size() - 1) {
                for (int i = 0; i < dims[dim_idx]; ++i) {
                    array->push_back(arr_init->back());
                    arr_init->pop_back();
                }
            } else {
                for (int i = 0; i < dims[dim_idx]; ++i)
                    array->push_back(build_array(dim_idx + 1));
            }
            for (int i = dim_idx; i < dims.size(); ++i) array_dim->push_back(dims[i]);
            auto *ret = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();        
            auto *next_array = new koopa_raw_type_kind_t();
            auto next_array_ty = new koopa_raw_type_kind_t();
            next_array->tag = KOOPA_RTT_ARRAY;
            next_array->data.array.len = array_dim->back();
            next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
            for (int i = int(array_dim->size()) - 2; i >= 0; --i) {
                auto *now_array = new koopa_raw_type_kind_t();
                now_array->tag = KOOPA_RTT_ARRAY;
                now_array->data.array.len = array_dim->at(i);
                now_array->data.array.base = next_array;
                next_array = now_array;
            }
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = next_array;
            ret->ty = ty;
            ret->name = nullptr;
            ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            ret->kind.tag = KOOPA_RVT_AGGREGATE;
            ret->kind.data.aggregate.elems = koopa_slice_new(*array, KOOPA_RSIK_VALUE);
            return ret;
        };
        ret->kind.data.global_alloc.init = (koopa_raw_value_t)build_array(0);
        return nullptr;
    }
}
void *LValAST::to_koopa() const {
    auto value = symbol_table.get_symbol(ident);
    auto *exp = new koopa_raw_value_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_INT32;
    exp->ty = ty;
    exp->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
    exp->name = nullptr;
    if (value.type == SymbolValueType::SVT_CONST) {
        exp->kind.data.integer.value = value.data.const_value;
        exp->kind.tag = KOOPA_RVT_INTEGER;
    } else if (value.type == SymbolValueType::SVT_VAR) {
        exp->kind.data.load.src = value.data.var_value;
        exp->kind.tag = KOOPA_RVT_LOAD;
        block_manager.insert_inst(exp);
    } else if (value.type == SymbolValueType::SVT_ARRAY) {
        // get pointer
        koopa_raw_value_data_t *last_array = nullptr;
        if (arr == nullptr || arr->empty()) goto ARR_EMPTY;
        for (auto &idx : *arr) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                auto temp = value.data.array_value->ty;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
                ty->data.pointer.base = temp->data.pointer.base->data.array.base;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)idx->to_koopa();
            block_manager.insert_inst(pointer);
            last_array = pointer;
        }
        ARR_EMPTY:;
        if (last_array == nullptr) {
            // index = 0
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                ty->data.pointer.base = value.data.array_value->ty->data.pointer.base;
                ty->data.pointer.base = ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).to_koopa();
            block_manager.insert_inst(pointer);
            last_array = pointer;
            return pointer;
        }
        if (last_array->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                ty->data.pointer.base = value.data.array_value->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).to_koopa();
            block_manager.insert_inst(pointer);
            return pointer;
        } else {
            // get value
            exp->kind.tag = KOOPA_RVT_LOAD;
            exp->kind.data.get_elem_ptr.src = last_array;
            block_manager.insert_inst(exp);
        }
    } else if (value.type == SymbolValueType::SVT_POINTER) {
        // get pointer
        auto *load = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = value.data.pointer_value->ty->data.pointer.base->tag;
        ty->data = value.data.pointer_value->ty->data.pointer.base->data;
        load->ty = ty;
        load->name = nullptr;
        load->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        load->kind.tag = KOOPA_RVT_LOAD;
        load->kind.data.load.src = value.data.pointer_value;
        block_manager.insert_inst(load);
        // getptr
        auto *pointer = new koopa_raw_value_data_t();
        ty = new koopa_raw_type_kind_t();
        koopa_raw_value_data_t *last_array = nullptr;

        if (arr == nullptr || arr->empty()) goto ARR_EMPTY_2;

        last_array = pointer;
        ty->tag = load->ty->tag;
        ty->data = load->ty->data;
        pointer->ty = ty;
        pointer->name = nullptr;
        pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        pointer->kind.tag = KOOPA_RVT_GET_PTR;
        pointer->kind.data.get_ptr.index = (koopa_raw_value_t)(arr->at(0))->to_koopa();
        pointer->kind.data.get_ptr.src = load;
        block_manager.insert_inst(pointer);
        arr->erase(arr->begin());
        for (auto &idx : *arr) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                ty->data.pointer.base = value.data.array_value->ty->data.pointer.base;
                ty->data.pointer.base = ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)idx->to_koopa();
            block_manager.insert_inst(pointer);
            last_array = pointer;
        }
        ARR_EMPTY_2:;
        if (last_array == nullptr) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                pointer->ty = load->ty;
                pointer->kind.data.get_elem_ptr.src = load;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).to_koopa();
            block_manager.insert_inst(pointer);
            last_array = pointer;
            return pointer;
        }
        if (last_array->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                ty->data.pointer.base = value.data.array_value->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).to_koopa();
            block_manager.insert_inst(pointer);
            return pointer;
        } else {
            // get value
            exp->kind.tag = KOOPA_RVT_LOAD;
            exp->kind.data.get_elem_ptr.src = last_array;
            block_manager.insert_inst(exp);
        }
    }
    return exp;
}
void *LValAST::to_koopa_leftvalue() const {
    auto value = symbol_table.get_symbol(ident);
    if (value.type == SymbolValueType::SVT_VAR)
        return (void *)symbol_table.get_symbol(ident).data.var_value;
    if (value.type == SymbolValueType::SVT_ARRAY) {
        koopa_raw_value_data_t *last_array = nullptr;
        for (auto &idx : *arr) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                ty->data.pointer.base = value.data.array_value->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)idx->to_koopa();
            block_manager.insert_inst(pointer);
            last_array = pointer;
        }
        return last_array;
    }
    if (value.type == SymbolValueType::SVT_POINTER) {
        // get pointer
        auto *load = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = value.data.pointer_value->ty->data.pointer.base->tag;
        ty->data = value.data.pointer_value->ty->data.pointer.base->data;
        load->ty = ty;
        load->name = nullptr;
        load->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        load->kind.tag = KOOPA_RVT_LOAD;
        load->kind.data.load.src = value.data.pointer_value;
        block_manager.insert_inst(load);

        // getptr
        auto *pointer = new koopa_raw_value_data_t();
        ty = new koopa_raw_type_kind_t();
        ty->tag = load->ty->tag;
        ty->data = load->ty->data;
        pointer->ty = ty;
        pointer->name = nullptr;
        pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        pointer->kind.tag = KOOPA_RVT_GET_PTR;
        pointer->kind.data.get_ptr.index = (koopa_raw_value_t)(arr->at(0))->to_koopa();
        pointer->kind.data.get_ptr.src = load;
        block_manager.insert_inst(pointer);
        arr->erase(arr->begin());
        koopa_raw_value_data_t *last_array = pointer;
        for (auto &idx : *arr) {
            auto *pointer = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_POINTER;
            if (last_array != nullptr) {
                ty->data.pointer.base = last_array->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = last_array;
            } else {
                ty->data.pointer.base = value.data.array_value->ty->data.pointer.base->data.array.base;
                pointer->ty = ty;
                pointer->kind.data.get_elem_ptr.src = value.data.array_value;
            }
            pointer->name = nullptr;
            pointer->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            pointer->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            pointer->kind.data.get_elem_ptr.index = (koopa_raw_value_t)idx->to_koopa();
            block_manager.insert_inst(pointer);
            last_array = pointer;
        }
        return last_array;
    }
}
void *ConstInitValAST::to_koopa() const {
    assert(exp != nullptr);
    return exp->to_koopa();
}
void *ConstInitValAST::to_koopa(int index) const {
    if (arr == nullptr || index >= arr->size()) return NumberAST(0).to_koopa();
    return arr->at(index)->to_koopa();
}
void ConstInitValAST::get_arr_init(vec<const void *> *arr_init, vec<int> dims) const {
    function<int(vec<int> dims)> proc = [&](vec<int> dims) -> int {            
        int sz = 1;
        for (auto &dim : dims) sz *= dim;
        if (arr != nullptr) {
            for (auto &init : *arr) {
                auto initval = (ConstInitValAST *)(init.get());
                if (initval->arr != nullptr) {
                    int pres = arr_init->size() / dims.back();
                    int cnt = 1;
                    for (int i = (int)dims.size() - 2; i >= 0; --i) {
                        if (pres % dims[i] != 0) break;
                        pres /= dims[i];
                        cnt += 1;
                    }
                    vec<int> nxt_dims;
                    for (int i = (int)dims.size() - cnt > 1 ? dims.size() - cnt : 1; i < dims.size(); ++i)
                        nxt_dims.push_back(dims[i]);
                    initval->get_arr_init(arr_init, nxt_dims);
                } else {
                    arr_init->push_back(NumberAST(initval->exp->get_val()).to_koopa());
                }
            }
        } else {
            for (int i = 0; i < sz; ++i)
                arr_init->push_back(NumberAST(0).to_koopa());
        }
        while (arr_init->size() == 0 || arr_init->size() % sz != 0) arr_init->push_back(NumberAST(0).to_koopa());
        return sz;
    };
    int tot = proc(dims);
    while (arr_init->size() < tot) arr_init->push_back(NumberAST(0).to_koopa());
}
void InitValAST::get_arr_init(vec<const void *> *arr_init, vec<int> dims) const {
    function<int(vec<int> dims)> proc = [&](vec<int> dims) -> int {
        int sz = 1;
        for (auto &dim : dims) sz *= dim;
        if (arr != nullptr) {
            for (auto &init : *arr) {
                auto initval = (InitValAST *)(init.get());
                if (initval->arr != nullptr) {
                    int pres = arr_init->size() / dims.back();
                    int cnt = 1;
                    for (int i = (int)dims.size() - 2; i >= 0; --i) {
                        if (pres % dims[i] != 0) break;
                        pres /= dims[i];
                        cnt += 1;
                    }
                    vec<int> nxt_dims;
                    for (int i = (int)dims.size() - cnt > 1 ? dims.size() - cnt : 1; i < dims.size(); ++i)
                        nxt_dims.push_back(dims[i]);
                    initval->get_arr_init(arr_init, nxt_dims);
                } else {
                    arr_init->push_back(NumberAST(initval->exp->get_val()).to_koopa());
                }
            }
        } else {
            for (int i = 0; i < sz; ++i)
                arr_init->push_back(NumberAST(0).to_koopa());
        }
        while (arr_init->size() == 0 || arr_init->size() % sz != 0) arr_init->push_back(NumberAST(0).to_koopa());
        return sz;
    };
    int tot = proc(dims);
    while (arr_init->size() < tot) arr_init->push_back(NumberAST(0).to_koopa());
}
void *VarDeclAST::to_koopa() const {
    auto type = koopa_raw_type_t(btype->to_koopa());
    for (auto &def : *arr) def->to_koopa(type);
    return nullptr;
}
void *VarDeclAST::to_koopa(vec<const void *> *global) const {
    auto type = koopa_raw_type_t(btype->to_koopa());
    for (auto &def : *arr) def->to_koopa(type, global);
    return nullptr;
}
void *VarDefAST::to_koopa(koopa_raw_type_t type) const {
    if (kind == VAR) {
        // alloc i32
        auto *ret = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = type; //INT32
        char *name = new char[ident.size() + 2];
        strcpy(name, ("@"+ident).c_str());
        name[ident.size() + 1] = '\0';
        ret->ty = ty;
        ret->name = name;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.insert_inst(ret);
        symbol_table.insert_symbol(ident, SymbolValue(SymbolValueType::SVT_VAR, ret));
        if (exp == nullptr) return nullptr;
        // store
        auto *store = new koopa_raw_value_data_t();
        ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_UNIT;
        store->ty = ty;
        store->name = nullptr;
        store->kind.data.store.dest = (koopa_raw_value_t)ret;
        // cout << "ret : " << ret << endl;
        store->kind.data.store.value = (koopa_raw_value_t)exp->to_koopa();
        store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        store->kind.tag = KOOPA_RVT_STORE;
        block_manager.insert_inst(store);
    } else {
        // array
        vec<int> dims;
        int sz = 1;
        for (auto &dim : *arr) {
            dims.push_back(dim->get_val());
            sz *= dim->get_val();
        }
        // allocate for array
        auto *ret = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        
        auto *next_array = new koopa_raw_type_kind_t();
        auto next_array_ty = new koopa_raw_type_kind_t();
        next_array->tag = KOOPA_RTT_ARRAY;
        next_array->data.array.len = dims.back();
        next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        for (int i = int(dims.size()) - 2; i >= 0; --i) {
            auto *now_array = new koopa_raw_type_kind_t();
            now_array->tag = KOOPA_RTT_ARRAY;
            now_array->data.array.len = dims[i];
            now_array->data.array.base = next_array;
            next_array = now_array;
        }
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = next_array;
        ret->ty = ty;
        char *temp_name = new char[ident.length() + 2];
        strcpy(temp_name, ("@"+ident).c_str());
        ret->name = temp_name;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.insert_inst(ret);
        symbol_table.insert_symbol(ident, SymbolValue(SymbolValueType::SVT_ARRAY, ret));

        // get init array value
        auto initval = (InitValAST *)(exp.get());
        auto *arr_init = new vec<const void *>();
        if (initval == nullptr) {
            for (int i = 0; i < sz; ++i) arr_init->push_back(NumberAST(0).to_koopa());
        } else {
            initval->get_arr_init(arr_init, dims);
        }
        // init
        vec<koopa_raw_value_t> ptr_stack;
        ptr_stack.push_back(ret); // 原数组
        for (int i = 0; i < sz; ++i) {
            int sz_of_j = sz, now = i, idx = 0;
            for (int j = 0; j < dims.size(); ++j) {
                sz_of_j /= dims[j];
                idx = now / sz_of_j;
                now %= sz_of_j;
                if (j + 1 < ptr_stack.size()) {
                    if (ptr_stack[j + 1]->kind.data.get_elem_ptr.index->kind.data.integer.value == idx) continue;
                    else while (j + 1 < ptr_stack.size()) ptr_stack.pop_back();
                }
                // get index pointer 
                auto *index = new koopa_raw_value_data_t();
                ty = new koopa_raw_type_kind_t();
                ty->tag = KOOPA_RTT_INT32;
                index->ty = ty;
                index->name = nullptr;
                index->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
                index->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                index->kind.data.get_elem_ptr.src = ptr_stack[j];
                index->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(idx).to_koopa();
                block_manager.insert_inst(index);
                ptr_stack.push_back(index);
            }
            // store
            auto *store = new koopa_raw_value_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_UNIT;
            store->ty = ty;
            store->name = nullptr;
            store->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            store->kind.tag = KOOPA_RVT_STORE;
            // auto initval = (InitValAST *)(exp.get());
            // if (initval == nullptr || initval->arr == nullptr || initval->arr->size() <= i)
            //     store->kind.data.store.value = (koopa_raw_value_t)NumberAST(0).to_koopa();
            // else
            //     store->kind.data.store.value = (koopa_raw_value_t)NumberAST(initval->arr->at(i)->get_val()).to_koopa();
            store->kind.data.store.value = (koopa_raw_value_t)arr_init->at(i);
            store->kind.data.store.dest = (koopa_raw_value_t)ptr_stack[dims.size()];
            block_manager.insert_inst(store);
        }
        return nullptr;
    }
    return nullptr;
}
void *VarDefAST::to_koopa(koopa_raw_type_t type, vec<const void *> *global) const {
    if (kind == VAR) {
        // alloc i32
        auto *ret = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = type; //INT32
        char *name = new char[ident.size() + 2];
        strcpy(name, ("@"+ident).c_str());
        name[ident.size() + 1] = '\0';
        ret->ty = ty;
        ret->name = name;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        if (exp == nullptr) {
            ret->kind.data.global_alloc.init = (koopa_raw_value_t)NumberAST(0).to_koopa();
        } else {
            ret->kind.data.global_alloc.init = (koopa_raw_value_t)exp->to_koopa();
        }
        global->push_back(ret);
        symbol_table.insert_symbol(ident, SymbolValue(SymbolValueType::SVT_VAR, ret));
    } else {
        // array
        vec<int> dims;
        int sz = 1;
        for (auto &dim : *arr) {
            dims.push_back(dim->get_val());
            sz *= dim->get_val();
        }
        // allocate for array
        auto *ret = new koopa_raw_value_data_t();
        auto ty = new koopa_raw_type_kind_t();
        
        auto *next_array = new koopa_raw_type_kind_t();
        auto next_array_ty = new koopa_raw_type_kind_t();
        next_array->tag = KOOPA_RTT_ARRAY;
        next_array->data.array.len = dims.back();
        next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
        for (int i = int(dims.size()) - 2; i >= 0; --i) {
            auto *now_array = new koopa_raw_type_kind_t();
            now_array->tag = KOOPA_RTT_ARRAY;
            now_array->data.array.len = dims[i];
            now_array->data.array.base = next_array;
            next_array = now_array;
        }
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = next_array;
        ret->ty = ty;
        char *temp_name = new char[ident.length() + 2];
        strcpy(temp_name, ("@"+ident).c_str());
        ret->name = temp_name;
        ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        global->push_back(ret);
        symbol_table.insert_symbol(ident, SymbolValue(SymbolValueType::SVT_ARRAY, ret));
        
        // get init array value
        auto initval = (InitValAST *)(exp.get());
        auto *arr_init = new vec<const void *>();
        if (initval == nullptr) {
            // for (int i = 0; i < sz; ++i) arr_init->push_back(NumberAST(0).to_koopa());
        } else {
            initval->get_arr_init(arr_init, dims);
        }
        reverse(arr_init->begin(), arr_init->end());
        auto array_ty = new koopa_raw_type_kind_t();
        function<void*(int)> build_array = [&](int dim_idx) -> void* {
            auto *array = new vec<const void *>();
            auto *array_dim = new vec<int>();
            if (dim_idx == dims.size() - 1) {
                for (int i = 0; i < dims[dim_idx]; ++i) {
                    if (arr_init->empty()) break;
                    array->push_back(arr_init->back());
                    arr_init->pop_back();
                }
            } else {
                for (int i = 0; i < dims[dim_idx]; ++i)
                    array->push_back(build_array(dim_idx + 1));
            }
            for (int i = dim_idx; i < dims.size(); ++i) array_dim->push_back(dims[i]);
            auto *ret = new koopa_raw_value_data_t();
            auto ty = new koopa_raw_type_kind_t();        
            auto *next_array = new koopa_raw_type_kind_t();
            auto next_array_ty = new koopa_raw_type_kind_t();
            next_array->tag = KOOPA_RTT_ARRAY;
            next_array->data.array.len = array_dim->back();
            next_array->data.array.base = (next_array_ty->tag = KOOPA_RTT_INT32, next_array_ty);
            for (int i = int(array_dim->size()) - 2; i >= 0; --i) {
                auto *now_array = new koopa_raw_type_kind_t();
                now_array->tag = KOOPA_RTT_ARRAY;
                now_array->data.array.len = array_dim->at(i);
                now_array->data.array.base = next_array;
                next_array = now_array;
            }
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = next_array;
            ret->ty = ty;
            array_ty = next_array;
            ret->name = nullptr;
            ret->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            ret->kind.tag = KOOPA_RVT_AGGREGATE;
            ret->kind.data.aggregate.elems = koopa_slice_new(*array, KOOPA_RSIK_VALUE);
            return ret;
        };
        if (arr_init->size())
            ret->kind.data.global_alloc.init = (koopa_raw_value_t)build_array(0);
        else {
            build_array(0);
            auto *zeroinit = new koopa_raw_value_data_t();
            zeroinit->ty = array_ty;
            zeroinit->name = nullptr;
            zeroinit->used_by = koopa_slice_new(KOOPA_RSIK_VALUE);
            zeroinit->kind.tag = KOOPA_RVT_ZERO_INIT;
            ret->kind.data.global_alloc.init = zeroinit;
        }
        return nullptr;
    }
    return nullptr;
}
void *InitValAST::to_koopa() const {
    return exp->to_koopa();
}

// ==================================================
// 构造函数实现
// ==================================================
NumberAST::NumberAST(int val) : val(val) {}