// AST.cpp
#include "ast.h"

// CompUnitAST 类的实现
void CompUnitAST::Dump() const {
    std::cout << "CompUnitAST { ";
    for(auto &def : *def_array) {
        def->Dump();
    }
    std::cout << " }\n";
}

void CompUnitAST::DeclineLibFunc(std::vector<const void *> &functions) const{
    koopa_raw_function_data_t *func;
    std::vector<const void *> params;

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "getint");
    func->ty = make_func_ty(nullptr, make_ty(KOOPA_RTT_INT32));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("getint", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "getch");
    func->ty = make_func_ty(nullptr, make_ty(KOOPA_RTT_INT32));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("getch", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "getarray");
    params.push_back(make_pointer_ty(KOOPA_RTT_INT32));
    func->ty = make_func_ty(&params, make_ty(KOOPA_RTT_INT32));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("getarray", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "putint");
    params.clear();
    params.push_back(make_ty(KOOPA_RTT_INT32));
    func->ty = make_func_ty(&params, make_ty(KOOPA_RTT_UNIT));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("putint", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "putch");
    params.clear();
    params.push_back(make_ty(KOOPA_RTT_INT32));
    func->ty = make_func_ty(&params, make_ty(KOOPA_RTT_UNIT));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("putch", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "putarray");
    params.clear();
    params.push_back(make_ty(KOOPA_RTT_INT32));
    params.push_back(make_pointer_ty(KOOPA_RTT_INT32));
    func->ty = make_func_ty(&params, make_ty(KOOPA_RTT_UNIT));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("putarray", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "starttime");
    func->ty = make_func_ty(nullptr, make_ty(KOOPA_RTT_UNIT));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("starttime", Value(ValueType::Func, func));
    functions.push_back(func);

    func = new koopa_raw_function_data_t;
    func->name = add_prefix("@", "stoptime");
    func->ty = make_func_ty(nullptr, make_ty(KOOPA_RTT_UNIT));
    func->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    func->bbs = make_slice(nullptr, KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("stoptime", Value(ValueType::Func, func));
    functions.push_back(func);

    return;
}

void *CompUnitAST::toKoopaIR() const {
    symbol_list.newScope();
    std::vector<const void *> functions;
    std::vector<const void *> values;
    DeclineLibFunc(functions);
    for(auto &def : *def_array) {
        def->toKoopaIR(functions, values);
    }
    symbol_list.deleteScope();
    koopa_raw_program_t *ir = new koopa_raw_program_t;
    ir->funcs = make_slice(&functions, KOOPA_RSIK_FUNCTION);
    ir->values = make_slice(&values, KOOPA_RSIK_VALUE);
    return ir;
}

void DefAST::Dump() const {
    std::cout << "DefAST { ";
    if(def_type == "FUNC") {
        func_def->Dump();
    }
    else if(def_type == "DECL") {
        decl->Dump();
    }
    std::cout << " }";
}

void *DefAST::toKoopaIR(std::vector<const void *> &functions, std::vector<const void *> &values) const {
    if(def_type == "FUNC") {
        functions.push_back(func_def->toKoopaIR());
    }
    else if(def_type == "DECL") {
        decl->toKoopaIR(values);
    }
    return nullptr;
}

// FuncDefAST 类的实现
void FuncDefAST::Dump() const {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}

void *FuncDefAST::toKoopaIR() const {
    koopa_raw_function_data_t *ir = new koopa_raw_function_data_t;
    symbol_list.addSymbol(ident.c_str(), Value(ValueType::Func, ir));
    ir->name = add_prefix("@", ident.c_str());
    std::vector<const void *> params_type;
    for(auto &param : *func_fparams) {
        params_type.push_back(static_cast<FuncFParamAST*>(param.get())->toKoopaIR());
    }
    if(params_type.size() > 0) {
        ir->ty = make_func_ty(&params_type, (const koopa_raw_type_kind *)func_type->toKoopaIR());
    }
    else {
        ir->ty = make_func_ty(nullptr, (const koopa_raw_type_kind *)func_type->toKoopaIR());
    }
    std::vector<const void *> params;
    for(int i = 0; i < func_fparams->size(); i++) {
        params.push_back(static_cast<FuncFParamAST*>(func_fparams->at(i).get())->toKoopaIR(i));
    }
    if(params.size() > 0) {
        ir->params = make_slice(&params, KOOPA_RSIK_VALUE);
    }
    else {
        ir->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    }

    std::vector<const void *> blocks;
    block_manager.init(&blocks);
    koopa_raw_basic_block_data_t *entry = new koopa_raw_basic_block_data_t();
    entry->name = add_prefix("%", "entry");
    entry->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    entry->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    block_manager.newBlock(entry);
    symbol_list.newScope();
    for(int i = 0; i < params.size(); i++) {
        koopa_raw_value_data_t *alloc_local = new koopa_raw_value_data_t;
        alloc_local->name = ((koopa_raw_value_t)params[i])->name;
        alloc_local->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        //alloc_local->ty = make_pointer_ty(KOOPA_RTT_INT32);
        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = ((koopa_raw_value_t)params[i])->ty;
        alloc_local->ty = ty;
        alloc_local->kind.tag = KOOPA_RVT_ALLOC;
        if(ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
            symbol_list.addSymbol(std::string(remove_prefix("@", alloc_local->name)), Value(ValueType::Var, alloc_local));
        }
        else {
            symbol_list.addSymbol(std::string(remove_prefix("@", alloc_local->name)), Value(ValueType::Pointer, alloc_local));
        }
        block_manager.addInst(alloc_local);
        koopa_raw_value_data_t *store_local = new koopa_raw_value_data_t;
        store_local->name = nullptr;
        store_local->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        store_local->ty = make_ty(KOOPA_RTT_UNIT);
        store_local->kind.tag = KOOPA_RVT_STORE;
        store_local->kind.data.store.dest = (koopa_raw_value_t)alloc_local;
        store_local->kind.data.store.value = (koopa_raw_value_t)params[i];
        block_manager.addInst(store_local);
    }
    block->toKoopaIR();
    if(block_manager.willBlockReturn())
    {
        koopa_raw_value_data_t *ret = new koopa_raw_value_data_t;
        ret->name = nullptr;
        ret->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ret->ty = make_ty(KOOPA_RTT_UNIT);
        ret->kind.tag = KOOPA_RVT_RETURN;
        if(static_cast<BTypeAST*>(func_type.get())->type == "INT") {
            ret->kind.data.ret.value = (koopa_raw_value_data_t *)NumberAST(0).toKoopaIR();
        }
        else {
            ret->kind.data.ret.value = nullptr;
        }
        block_manager.addInst(ret);
    }
    symbol_list.deleteScope();
    block_manager.popBuffer();
    for(int i=0;i<blocks.size();i++) {
        koopa_raw_basic_block_data_t *bb = (koopa_raw_basic_block_data_t *)blocks[i];
        bb->name = add_prefix(add_prefix("%", ident.c_str()), add_prefix("_", remove_prefix("%", bb->name)));
    }
    ir->bbs = make_slice(&blocks, KOOPA_RSIK_BASIC_BLOCK);
    return ir;

}

void FuncFParamAST::Dump() const {
    std::cout << "FuncFParamAST { ";
    type->Dump();
    std::cout << ", " << ident << " }";
}

void *FuncFParamAST::toKoopaIR() const {//return type
    if(!is_array)
    {
        return type->toKoopaIR();
    }
    else
    {
        koopa_raw_type_kind *ty = new koopa_raw_type_kind();

        if(array==nullptr)
        {
            ty = make_pointer_ty(KOOPA_RTT_INT32);
        }
        else
        {
            std::vector<int> dim_size;
            for(auto &dim : *array)
            {
                dim_size.push_back(dim->calculate());
            }
            ty = make_array_pointer_ty(KOOPA_RTT_INT32, dim_size);
        }
        return ty;
    }
}

void *FuncFParamAST::toKoopaIR(int i) const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = add_prefix("@", ident.c_str());
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    //ir->ty = (koopa_raw_type_t)type->toKoopaIR();
    ir->ty = (koopa_raw_type_t)toKoopaIR();
    ir->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
    ir->kind.data.func_arg_ref.index = i;
    return ir;
}

// BlockAST 类的实现
void BlockAST::Dump() const {
    std::cout << "BlockAST { ";
    if(block_item_array) {
        for(auto &block_item : *block_item_array) {
            block_item->Dump();
        }
    }
    std::cout << " }";
}

void *BlockAST::toKoopaIR() const {
    if(block_item_array) {
        for(auto &block_item : *block_item_array) {
            block_item->toKoopaIR();
        }
    }
    return nullptr;
}

void *BlockAST::toKoopaIR(std::vector<const void *> &stmts) const {
    koopa_raw_basic_block_data_t *ir = new koopa_raw_basic_block_data_t;
    if(block_item_array) {
        for(auto &block_item : *block_item_array) {
            block_item->toKoopaIR(stmts);
            if (stmts.size() > 0) {
                auto inst = (koopa_raw_value_t)stmts.back();
                if (inst->kind.tag == KOOPA_RVT_RETURN) {
                    break;
                }
            }
        }
        ir->insts = make_slice(&stmts, KOOPA_RSIK_VALUE);
    }
    else {
        ir->insts = make_slice(nullptr, KOOPA_RSIK_VALUE);
    }
    ir->name = add_prefix("%", "entry");
    ir->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->insts = make_slice(&stmts, KOOPA_RSIK_VALUE);
    return ir;
}


// BlockItemAST 类的实现
void BlockItemAST::Dump() const {
    std::cout << "BlockItemAST { ";
    if(block_item_type == "DECL") {
        decl->Dump();
    }
    else if(block_item_type == "STMT") {
        stmt->Dump();
    }
    std::cout << " }";
}

void *BlockItemAST::toKoopaIR() const {
    if(block_item_type == "DECL") {
        return decl->toKoopaIR();
    }
    else if(block_item_type == "STMT") {
        return stmt->toKoopaIR();
    }
    return nullptr;
}

void DeclAST::Dump() const {
    std::cout << "DeclAST { ";
    if(decl_type == "CONST") {
        const_decl->Dump();
    }
    else if(decl_type == "VAR") {
        var_decl->Dump();
    }
    std::cout << " }";
}

void *DeclAST::toKoopaIR() const {
    if(decl_type == "CONST") {
        return const_decl->toKoopaIR();
    }
    else if(decl_type == "VAR") {
        return var_decl->toKoopaIR();
    }
    return nullptr;
}

void *DeclAST::toKoopaIR(std::vector<const void *> &values) const {
    if(decl_type == "CONST") {
        return const_decl->toKoopaIR(values);
    }
    else if(decl_type == "VAR") {
        return var_decl->toKoopaIR(values);
    }
    return nullptr;
}

void ConstDeclAST::Dump() const {
    std::cout << "ConstDeclAST { ";
    type->Dump();
    for(auto &const_def : *const_def_array) {
        const_def->Dump();
    }
    std::cout << " }";
}

void *ConstDeclAST::toKoopaIR() const {
    koopa_raw_type_t const_type = (const koopa_raw_type_t)type->toKoopaIR();
    for(auto &const_def : *const_def_array) {
        const_def->toKoopaIR(const_type);
    }
    return nullptr;
}

void *ConstDeclAST::toKoopaIR(std::vector<const void *> &values) const {
    koopa_raw_type_t const_type = (const koopa_raw_type_t)type->toKoopaIR();
    for(auto &const_def : *const_def_array) {
        const_def->toKoopaIR(const_type, values);
    }
    return nullptr;
}

void VarDeclAST::Dump() const {
    std::cout << "VarDeclAST { ";
    type->Dump();
    for(auto &var_def : *var_def_array) {
        var_def->Dump();
    }
    std::cout << " }";
}

void *VarDeclAST::toKoopaIR() const {
    koopa_raw_type_t var_type = (const koopa_raw_type_t)type->toKoopaIR();
    for(auto &var_def : *var_def_array) {
        var_def->toKoopaIR(var_type);
    }
    return nullptr;
}

void *VarDeclAST::toKoopaIR(std::vector<const void *> &values) const {
    koopa_raw_type_t var_type = (const koopa_raw_type_t)type->toKoopaIR();
    for(auto &var_def : *var_def_array) {
        var_def->toKoopaIR(var_type, values);
    }
    return nullptr;
}

void BTypeAST::Dump() const {
    std::cout << "BTypeAST { " << type << " }";
}

void *BTypeAST::toKoopaIR() const {
    if(type == "INT") {
        koopa_raw_type_kind_t *ir = new koopa_raw_type_kind_t;
        ir->tag = KOOPA_RTT_INT32;
        return ir;
    }
    else if(type == "VOID") {
        koopa_raw_type_kind_t *ir = new koopa_raw_type_kind_t;
        ir->tag = KOOPA_RTT_UNIT;
        return ir;
    }
    return nullptr;
}

void ConstDefAST::Dump() const {
    std::cout << "ConstDefAST { ";
    std::cout << ident << ", ";
    if(const_init_val) {
        const_init_val->Dump();
    }
    std::cout << " }";
}

void *ConstDefAST::toKoopaIR(koopa_raw_type_t type) const {
    if(array==nullptr)
    {
        int val = const_init_val->calculate();
        Value value(ValueType::Const, val);
        symbol_list.addSymbol(ident, value);
        return nullptr;
    }
    else
    {
        std::vector<int> dim_size;
        int total_size = 1;
        for(int i=0;i<array->size();i++)
        {
            int size = array->at(i)->calculate();
            dim_size.push_back(size);
            total_size *= size;
        }
        koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
        ir->name = add_prefix("@", ident.c_str());
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_array_pointer_ty(type->tag, dim_size);
        ir->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.addInst(ir);

        Value value(ValueType::Array, ir);
        symbol_list.addSymbol(ident, value);

        InitValAST *init_val = dynamic_cast<InitValAST*>(const_init_val.get());
        if(init_val->type == InitValAST::ZERO)
        {
            koopa_raw_value_data *store = new koopa_raw_value_data();
            store->name = nullptr;
            store->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            store->ty = make_ty(KOOPA_RTT_UNIT);
            store->kind.tag = KOOPA_RVT_STORE;
            store->kind.data.store.dest = (koopa_raw_value_t)ir;
            koopa_raw_value_data *zero = new koopa_raw_value_data();
            zero->name = nullptr;
            zero->ty = make_array_ty(type->tag, dim_size);
            zero->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            zero->kind.tag = KOOPA_RVT_ZERO_INIT;
            store->kind.data.store.value = (koopa_raw_value_t)zero;
            block_manager.addInst(store);
        }
        else
        {
            std::vector<const void *> init_val_vec;
            init_val->restructure(dim_size, init_val_vec);
            assert(init_val_vec.size() == total_size);
            std::vector<koopa_raw_value_data *> get_dim_base;
            for(int i=0;i<total_size;i++)
            {
                int residual = i;
                int elem_size = total_size;
                for(int j=0;j<dim_size.size();j++)
                {
                    elem_size /= dim_size[j];
                    int idx = residual / elem_size;
                    residual = residual % elem_size;

                    if(j < get_dim_base.size())
                    {
                        if(idx == get_dim_base[j]->kind.data.get_elem_ptr.index->kind.data.integer.value)
                            continue;
                        else
                            while(j < get_dim_base.size())
                                get_dim_base.pop_back();
                    }
                    koopa_raw_value_data *src = (j == 0)? ir : get_dim_base[j-1];
                    koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
                    get_elem_ptr->name = nullptr;
                    get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                    ty->tag = KOOPA_RTT_POINTER;
                    ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                    get_elem_ptr->ty = ty;
                    get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                    get_elem_ptr->kind.data.get_elem_ptr.src = src;
                    get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(idx).toKoopaIR();
                    block_manager.addInst(get_elem_ptr);
                    get_dim_base.push_back(get_elem_ptr);
                }
                koopa_raw_value_data *store = new koopa_raw_value_data();
                store->name = nullptr;
                store->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                store->ty = make_ty(KOOPA_RTT_UNIT);
                store->kind.tag = KOOPA_RVT_STORE;
                store->kind.data.store.dest = (koopa_raw_value_t)get_dim_base[dim_size.size() - 1];
                store->kind.data.store.value = (koopa_raw_value_t)init_val_vec[i];
                block_manager.addInst(store);
            }
        }
        return nullptr;
    }
}

void *ConstDefAST::toKoopaIR(koopa_raw_type_t type, std::vector<const void *> &values) const {
    if(array==nullptr)
    {
        int val = const_init_val->calculate();
        Value value(ValueType::Const, val);
        symbol_list.addSymbol(ident, value);
        return nullptr;
    }
    else
    {
        std::vector<int> dim_size;
        int total_size = 1;
        for(int i=0;i<array->size();i++)
        {
            int size = array->at(i)->calculate();
            dim_size.push_back(size);
            total_size *= size;
        }
        koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
        ir->name = add_prefix("@", ident.c_str());
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_array_pointer_ty(type->tag, dim_size);
        ir->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        values.push_back(ir);

        Value value(ValueType::Array, ir);
        symbol_list.addSymbol(ident, value);

        InitValAST *init_val = dynamic_cast<InitValAST*>(const_init_val.get());
        if(init_val->type == InitValAST::ZERO)
        {
            koopa_raw_value_data *zero = new koopa_raw_value_data();
            zero->name = nullptr;
            zero->ty = make_array_ty(type->tag, dim_size);
            zero->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            zero->kind.tag = KOOPA_RVT_ZERO_INIT;
            ir->kind.data.global_alloc.init = (koopa_raw_value_t)zero;
        }
        else
        {
            std::vector<const void *> init_val_vec;
            init_val->restructure(dim_size, init_val_vec);
            assert(init_val_vec.size() == total_size);
            ir->kind.data.global_alloc.init = (koopa_raw_value_t)init_val->toKoopaIR(init_val_vec, dim_size);
        }
        return nullptr;
    }
}

void VarDefAST::Dump() const {
    std::cout << "VarDefAST { ";
    std::cout << ident << ", ";
    if(var_init_val) {
        var_init_val->Dump();
    }
    std::cout << " }";
}


void *VarDefAST::toKoopaIR(koopa_raw_type_t type) const {
    if(array == nullptr)
    {
        koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
        ir->name = add_prefix("@", ident.c_str());
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_pointer_ty(KOOPA_RTT_INT32);
        ir->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.addInst(ir);
        Value value(ValueType::Var, ir);
        symbol_list.addSymbol(ident, value);
        if(var_init_val!=nullptr) {
            koopa_raw_value_data_t *ir2 = new koopa_raw_value_data_t;
            ir2->name = nullptr;
            ir2->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            ir2->ty = make_ty(KOOPA_RTT_INT32);
            ir2->kind.tag = KOOPA_RVT_STORE;
            ir2->kind.data.store.dest = (koopa_raw_value_t)ir;
            ir2->kind.data.store.value = (koopa_raw_value_t)(var_init_val->toKoopaIR());
            block_manager.addInst(ir2);
        }
        return nullptr;
    }
    else
    {
        std::vector<int> dim_size;
        int total_size = 1;
        for(int i=0;i<array->size();i++)
        {
            int size = array->at(i)->calculate();
            dim_size.push_back(size);
            total_size *= size;
        }
        koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
        ir->name = add_prefix("@", ident.c_str());
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_array_pointer_ty(type->tag, dim_size);

        ir->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.addInst(ir);

        Value value(ValueType::Array, ir);
        symbol_list.addSymbol(ident, value);

        if(var_init_val != nullptr)
        {
            InitValAST *init_val = dynamic_cast<InitValAST*>(var_init_val.get());
            if(init_val->type == InitValAST::ZERO)
            {
                koopa_raw_value_data *store = new koopa_raw_value_data();
                store->name = nullptr;
                store->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                store->ty = make_ty(KOOPA_RTT_UNIT);
                store->kind.tag = KOOPA_RVT_STORE;
                store->kind.data.store.dest = (koopa_raw_value_t)ir;
                koopa_raw_value_data *zero = new koopa_raw_value_data();
                zero->name = nullptr;
                zero->ty = make_array_ty(type->tag, dim_size);
                zero->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                zero->kind.tag = KOOPA_RVT_ZERO_INIT;
                store->kind.data.store.value = (koopa_raw_value_t)zero;
                block_manager.addInst(store);
            }
            else
            {
                std::vector<const void *> init_val_vec;
                init_val->restructure(dim_size, init_val_vec);
                assert(total_size == init_val_vec.size());
                std::vector<koopa_raw_value_data *> get_dim_base;
                for(int i=0;i<total_size;i++)
                {
                    int residual = i;
                    int elem_size = total_size;
                    for(int j=0;j<dim_size.size();j++)
                    {
                        elem_size /= dim_size[j];
                        int idx = residual / elem_size;
                        residual = residual % elem_size;

                        if(j < get_dim_base.size())
                        {
                            if(idx == get_dim_base[j]->kind.data.get_elem_ptr.index->kind.data.integer.value)
                                continue;
                            else
                                while(j < get_dim_base.size())
                                    get_dim_base.pop_back();
                        }
                        koopa_raw_value_data *src = (j == 0) ? ir : get_dim_base[j-1];
                        koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
                        get_elem_ptr->name = nullptr;
                        get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                        ty->tag = KOOPA_RTT_POINTER;
                        ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                        get_elem_ptr->ty = ty;
                        get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                        get_elem_ptr->kind.data.get_elem_ptr.src = src;
                        get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(idx).toKoopaIR();
                        block_manager.addInst(get_elem_ptr);
                        get_dim_base.push_back(get_elem_ptr);
                    }
                    koopa_raw_value_data *store = new koopa_raw_value_data();
                    store->name = nullptr;
                    store->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                    store->ty = make_ty(KOOPA_RTT_UNIT);
                    store->kind.tag = KOOPA_RVT_STORE;
                    store->kind.data.store.dest = (koopa_raw_value_t)get_dim_base[dim_size.size() - 1];
                    store->kind.data.store.value = (koopa_raw_value_t)init_val_vec[i];
                    block_manager.addInst(store);
                }
            }
        }
        else
        {
        }
        return nullptr;
    }
    
}

void *VarDefAST::toKoopaIR(koopa_raw_type_t type, std::vector<const void *> &values) const {
    if(array == nullptr)
    {
        koopa_raw_value_data *ir = new koopa_raw_value_data;
        ir->name = add_prefix("@", ident.c_str());
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_pointer_ty(type->tag);
        ir->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        if(var_init_val!=nullptr) {
            ir->kind.data.global_alloc.init = (koopa_raw_value_t)(var_init_val->toKoopaIR());
        }
        else {
            koopa_raw_value_data_t *zero = new koopa_raw_value_data_t;
            zero->name = nullptr;
            zero->ty = make_ty(KOOPA_RTT_INT32);
            zero->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            zero->kind.tag = KOOPA_RVT_ZERO_INIT;
            ir->kind.data.global_alloc.init = (koopa_raw_value_t)zero;
        }
        values.push_back(ir);
        symbol_list.addSymbol(ident, Value(ValueType::Var, ir));
        return nullptr;
    }
    else
    {
        std::vector<int> dim_size;
        int total_size = 1;
        for(int i=0;i<array->size();i++)
        {
            int size = array->at(i)->calculate();
            dim_size.push_back(size);
            total_size *= size;
        }
        koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
        ir->name = add_prefix("@", ident.c_str());
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_array_pointer_ty(type->tag, dim_size);
        ir->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        values.push_back(ir);

        Value value(ValueType::Array, ir);
        symbol_list.addSymbol(ident, value);

        if(var_init_val != nullptr)
        {
            InitValAST *init_val = dynamic_cast<InitValAST*>(var_init_val.get());
            if(init_val->type == InitValAST::ZERO)
            {
                koopa_raw_value_data *zero = new koopa_raw_value_data();
                zero->name = nullptr;
                zero->ty = make_array_ty(type->tag, dim_size);
                zero->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                zero->kind.tag = KOOPA_RVT_ZERO_INIT;
                ir->kind.data.global_alloc.init = (koopa_raw_value_t)zero;
            }
            else
            {
                std::vector<const void *> init_val_vec;
                init_val->restructure(dim_size, init_val_vec);
                assert(total_size == init_val_vec.size());
                ir->kind.data.global_alloc.init = (koopa_raw_value_t)init_val->toKoopaIR(init_val_vec, dim_size);
            }
        }
        else
        {
            koopa_raw_value_data *zero = new koopa_raw_value_data();
            zero->name = nullptr;
            zero->ty = make_array_ty(type->tag, dim_size);
            zero->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            zero->kind.tag = KOOPA_RVT_ZERO_INIT;
            ir->kind.data.global_alloc.init = (koopa_raw_value_t)zero;
        }
        return nullptr;
    }
    
}

void ConstInitValAST::Dump() const {
    std::cout << "ConstInitValAST { ";
    const_exp->Dump();
    std::cout << " }";
}

void *ConstInitValAST::toKoopaIR() const {
    return const_exp->toKoopaIR();
}

int ConstInitValAST::calculate() const {
    return const_exp->calculate();
}

void InitValAST::Dump() const {
    std::cout << "InitValAST { ";
    exp->Dump();
    std::cout << " }";
}

void *InitValAST::toKoopaIR() const {
    if(type == EXP)
        return exp->toKoopaIR();
    assert(false);
    return nullptr;
}

void *InitValAST::toKoopaIR(std::vector<const void *> &init_val_vec, std::vector<int> dim_size) const 
{
    std::vector<const void *> *aggregate_vec = new std::vector<const void *>();
    if(dim_size.size() == 1)
    {
        for(int i=0;i<dim_size[0];i++)
        {
            aggregate_vec->push_back(init_val_vec[0]);
            init_val_vec.erase(init_val_vec.begin());
        }
    }
    else
    {
        for(int i=0;i<dim_size[0];i++)
        {
            std::vector<int> sub_dim_size;
            for(int j=1;j<dim_size.size();j++)
            {
                sub_dim_size.push_back(dim_size[j]);
            }
            aggregate_vec->push_back(toKoopaIR(init_val_vec, sub_dim_size));
        }
    }
    koopa_raw_value_data *ir = new koopa_raw_value_data();
    ir->ty = make_array_ty(KOOPA_RTT_INT32, dim_size);
    ir->name = nullptr;
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->kind.tag = KOOPA_RVT_AGGREGATE;
    ir->kind.data.aggregate.elems = make_slice(aggregate_vec, KOOPA_RSIK_VALUE);
    return ir;
}

void InitValAST::restructure(std::vector<int> dim_size, std::vector<const void *> &init_val_vec) const
{
    int total_size = 1;
    for(int i=0;i<dim_size.size();i++)
    {
        total_size *= dim_size[i];
    }
    if(type == ZERO)
    {
        for(int i=0;i<total_size;i++)
        {
            init_val_vec.push_back(NumberAST(0).toKoopaIR());
            //std::cout<<0<<' ';
        }
    }
    else
    {
        int len_n = dim_size.back();
        for(auto i = init_val_array->begin(); i != init_val_array->end(); i++)
        {
            InitValAST *init_val = dynamic_cast<InitValAST*>(i->get());
            if(init_val->type == EXP)
            {
                init_val_vec.push_back(NumberAST(init_val->calculate()).toKoopaIR());
                //std::cout<<init_val->calculate()<<' ';
            }
            else
            {
                int cur_size = init_val_vec.size();
                assert(len_n!=0);
                assert(cur_size % len_n == 0);
                int align_size = cur_size / len_n;
                int dim_count = 1;
                for(int i = dim_size.size() - 2; i >= 1; i--)
                {
                    assert(dim_size[i]!=0);
                    if(align_size % dim_size[i] != 0)
                        break;
                    dim_count++;
                    align_size /= dim_size[i];
                }
                std::vector<int> sub_dim_size;
                for(int i = dim_size.size() - dim_count; i < dim_size.size(); i++)
                {
                    sub_dim_size.push_back(dim_size[i]);
                }
                init_val->restructure(sub_dim_size, init_val_vec);
            }
        }
    }   
    assert(total_size != 0);
    while(init_val_vec.size() % total_size != 0)
    {
        init_val_vec.push_back(NumberAST(0).toKoopaIR());
        //std::cout<<0<<' ';
    }
}

int InitValAST::calculate() const {
    return exp->calculate();
}

// StmtAST 类的实现
void StmtAST::Dump() const {
    std::cout << "StmtAST { "<< stmt_type << ", ";
    if(stmt_type == "RETURN") {
        if(exp) {
            exp->Dump();
        }
    }
    else if(stmt_type == "ASSIGN") {
        lval->Dump();
        std::cout << " = ";
        exp->Dump();
    }
    else if(stmt_type == "EXP") {
        exp->Dump();
    }
    else if(stmt_type == "BLOCK") {
        block->Dump();
    }
    else if(stmt_type == "IFELSE") {
        if_stmt->Dump();
        else_stmt->Dump();
    }
    else if(stmt_type == "IF") {
        if_stmt->Dump();
    }
    else if(stmt_type == "LOOP") {
        exp->Dump();
        while_stmt->Dump();
    }
    std::cout << " }";
}

void *StmtAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = nullptr;
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->ty = make_ty(KOOPA_RTT_UNIT);
    if(stmt_type == "RETURN") {
        ir->kind.tag = KOOPA_RVT_RETURN;
        if(exp) {
            ir->kind.data.ret.value = (koopa_raw_value_data_t *)exp->toKoopaIR();
        }
        else {
            ir->kind.data.ret.value = nullptr;
        }
        block_manager.addInst(ir);
    }
    else if(stmt_type == "ASSIGN") {
        ir->kind.tag = KOOPA_RVT_STORE;
        ir->kind.data.store.dest = (koopa_raw_value_t)lval->getKoopaIR();
        ir->kind.data.store.value = (koopa_raw_value_t)exp->toKoopaIR();
        block_manager.addInst(ir);
    }
    else if(stmt_type == "EXP") {
        return exp->toKoopaIR();
    }
    else if(stmt_type == "BLOCK") {
        symbol_list.newScope();
        block->toKoopaIR();
        symbol_list.deleteScope();
    }
    else if(stmt_type == "BRANCH")
    {
        ir = (koopa_raw_value_data *)if_stmt->toKoopaIR();
        bool ifblock_might_comeback = block_manager.willBlockReturn();
        bool elseblock_might_comeback = true;

        koopa_raw_basic_block_data_t *else_block = new koopa_raw_basic_block_data_t;
        else_block->name = add_prefix("%", "else");
        else_block->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        else_block->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);

        koopa_raw_basic_block_data_t *end_block = new koopa_raw_basic_block_data_t;
        end_block->name = add_prefix("%", "end");
        end_block->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        end_block->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);

        ir->kind.data.branch.false_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        if(else_stmt != nullptr)
        {
            ir->kind.data.branch.false_bb = (koopa_raw_basic_block_t)else_block;
            ir->kind.data.branch.false_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
            if(ifblock_might_comeback)
            {
                block_manager.addInst(jumpInst((koopa_raw_basic_block_t)end_block));
            }
            block_manager.newBlock(else_block);
            symbol_list.newScope();
            else_stmt->toKoopaIR();
            symbol_list.deleteScope();
            elseblock_might_comeback = block_manager.willBlockReturn();
        }
        else
        {
            ir->kind.data.branch.false_bb = (koopa_raw_basic_block_t)end_block;
        }
        if(elseblock_might_comeback)
        {
            block_manager.addInst(jumpInst((koopa_raw_basic_block_t)end_block));
        }
        if(ifblock_might_comeback || elseblock_might_comeback)
        {
            block_manager.newBlock(end_block);
        }
    }
    else if(stmt_type == "LOOP") {
        koopa_raw_basic_block_data_t *while_entry = new koopa_raw_basic_block_data_t;
        while_entry->name = add_prefix("%", "while_entry");
        while_entry->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        while_entry->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        block_manager.addInst(jumpInst((koopa_raw_basic_block_t)while_entry));
        block_manager.newBlock(while_entry);
        koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
        ir->name = nullptr;
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->ty = make_ty(KOOPA_RTT_UNIT);
        ir->kind.tag = KOOPA_RVT_BRANCH;
        ir->kind.data.branch.cond = (koopa_raw_value_t)exp->toKoopaIR();
        koopa_raw_basic_block_data_t *while_body = new koopa_raw_basic_block_data_t;
        while_body->name = add_prefix("%", "while_body");
        while_body->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        while_body->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.data.branch.true_bb = (koopa_raw_basic_block_t)while_body;
        ir->kind.data.branch.true_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        koopa_raw_basic_block_data_t *while_end = new koopa_raw_basic_block_data_t;
        while_end->name = add_prefix("%", "while_end");
        while_end->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        while_end->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.data.branch.false_bb = (koopa_raw_basic_block_t)while_end;
        ir->kind.data.branch.false_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        loop_manager.pushLoop(while_entry, while_end);
        block_manager.addInst(ir);
        block_manager.newBlock(while_body);
        while_stmt->toKoopaIR();
        block_manager.addInst(jumpInst((koopa_raw_basic_block_t)while_entry));
        block_manager.newBlock(while_end);
        loop_manager.popLoop();
    }
    else if(stmt_type == "BREAK") {
        assert(loop_manager.getLoop().loop_end != nullptr);
        block_manager.addInst(jumpInst(loop_manager.getLoop().loop_end));
    }
    else if(stmt_type == "CONTINUE") {
        assert(loop_manager.getLoop().loop_entry != nullptr);
        block_manager.addInst(jumpInst(loop_manager.getLoop().loop_entry));
    }
    else if(stmt_type == "EMPTY") {
        return nullptr;
    }
    return ir;
}

void IfAST::Dump() const {
    std::cout << "IfAST { ";
    exp->Dump();
    std::cout << ", ";
    stmt->Dump();
    std::cout << " }";
}

void *IfAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = nullptr;
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->ty = make_ty(KOOPA_RTT_UNIT);
    ir->kind.tag = KOOPA_RVT_BRANCH;
    ir->kind.data.branch.cond = (koopa_raw_value_t)exp->toKoopaIR();
    block_manager.addInst(ir);
    koopa_raw_basic_block_data_t *if_block = new koopa_raw_basic_block_data_t;
    if_block->name = add_prefix("%", "then");
    if_block->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
    if_block->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->kind.data.branch.true_bb = (koopa_raw_basic_block_t)if_block;
    ir->kind.data.branch.true_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
    block_manager.newBlock(if_block);
    symbol_list.newScope();
    stmt->toKoopaIR();
    symbol_list.deleteScope();
    return ir;
}

void ConstExpAST::Dump() const {
    std::cout << "ConstExpAST { ";
    exp->Dump();
    std::cout << " }";
}

void *ConstExpAST::toKoopaIR() const {
    return exp->toKoopaIR();
}

int ConstExpAST::calculate() const {
    return exp->calculate();
}

void *ExpAST::toKoopaIR() const {
    return lor_exp->toKoopaIR();
}

int ExpAST::calculate() const {
    return lor_exp->calculate();
}

void ExpAST::Dump() const {
    std::cout << "ExpAST { ";
    lor_exp->Dump();
    std::cout << " }";
}

// LOrExpAST 类的实现
void LOrExpAST::Dump() const {
    std::cout << "LOrExpAST { ";
    if(op == "||") {
        lor_exp->Dump();
        std::cout << " || ";
        land_exp->Dump();
    }
    else if(op == " ") {
        land_exp->Dump();
    }
    std::cout << " }";
}

void *LOrExpAST::toKoopaIR() const {
    if(op == "||") {
        koopa_raw_value_data *temp_alloc = new koopa_raw_value_data;
        temp_alloc->name = add_prefix("@", "temp");
        temp_alloc->ty = make_pointer_ty(KOOPA_RTT_INT32);
        temp_alloc->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_alloc->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.addInst(temp_alloc);
        koopa_raw_value_data *temp_store = new koopa_raw_value_data;
        temp_store->name = nullptr;
        temp_store->ty = make_ty(KOOPA_RTT_INT32);
        temp_store->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_store->kind.tag = KOOPA_RVT_STORE;
        temp_store->kind.data.store.dest = (koopa_raw_value_t)temp_alloc;
        temp_store->kind.data.store.value = (koopa_raw_value_t)NumberAST(1).toKoopaIR();
        block_manager.addInst(temp_store);
        koopa_raw_value_data *temp_branch = new koopa_raw_value_data;
        temp_branch->name = nullptr;
        temp_branch->ty = make_ty(KOOPA_RTT_UNIT);
        temp_branch->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_branch->kind.tag = KOOPA_RVT_BRANCH;
        temp_branch->kind.data.branch.cond = (koopa_raw_value_t)toBoolKoopaIR(lor_exp);
        koopa_raw_basic_block_data_t *temp_then = new koopa_raw_basic_block_data_t;
        temp_then->name = add_prefix("%", "then");
        temp_then->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_then->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_branch->kind.data.branch.true_bb = (koopa_raw_basic_block_t)temp_then;
        temp_branch->kind.data.branch.true_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        koopa_raw_basic_block_data_t *temp_end = new koopa_raw_basic_block_data_t;
        temp_end->name = add_prefix("%", "end");
        temp_end->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_end->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_branch->kind.data.branch.false_bb = (koopa_raw_basic_block_t)temp_end;
        temp_branch->kind.data.branch.false_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        block_manager.addInst(temp_branch);
        block_manager.newBlock(temp_end);
        koopa_raw_value_data *temp_store2 = new koopa_raw_value_data;
        temp_store2->name = nullptr;
        temp_store2->ty = make_ty(KOOPA_RTT_INT32);
        temp_store2->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_store2->kind.tag = KOOPA_RVT_STORE;
        temp_store2->kind.data.store.dest = (koopa_raw_value_t)temp_alloc;
        temp_store2->kind.data.store.value = (koopa_raw_value_t)toBoolKoopaIR(land_exp);
        block_manager.addInst(temp_store2);
        block_manager.addInst(jumpInst((koopa_raw_basic_block_t)temp_then));
        block_manager.newBlock(temp_then);
        koopa_raw_value_data *ret = new koopa_raw_value_data;
        ret->name = nullptr;
        ret->ty = make_ty(KOOPA_RTT_INT32);
        ret->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_LOAD;
        ret->kind.data.load.src = (koopa_raw_value_t)temp_alloc;
        block_manager.addInst(ret);
        return ret;
    }
    else if(op == " ") {
        return land_exp->toKoopaIR();
    }
    return nullptr;
}

int LOrExpAST::calculate() const {
    if (op == "||") {
        return lor_exp->calculate() || land_exp->calculate();
    }
    else if (op == " ") {
        return land_exp->calculate();
    }
    return 0;
}

void *LOrExpAST::toBoolKoopaIR(const std::unique_ptr<BaseAST> &exp) const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = nullptr;
    ir->ty = make_ty(KOOPA_RTT_INT32);
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->kind.tag = KOOPA_RVT_BINARY;
    ir->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
    NumberAST zero(0);
    ir->kind.data.binary.lhs = (koopa_raw_value_t)zero.toKoopaIR();
    ir->kind.data.binary.rhs = (koopa_raw_value_t)exp->toKoopaIR();
    block_manager.addInst(ir);
    return ir;
}

// LAndExpAST 类的实现
void LAndExpAST::Dump() const {
    std::cout << "LAndExpAST { ";
    if(op == "&&") {
        land_exp->Dump();
        std::cout << " && ";
        eq_exp->Dump();
    }
    else if(op == " ") {
        eq_exp->Dump();
    }
    std::cout << " }";
}

void *LAndExpAST::toKoopaIR() const {
    if(op == "&&") {
        koopa_raw_value_data *temp_alloc = new koopa_raw_value_data;
        temp_alloc->name = add_prefix("@", "temp");
        temp_alloc->ty = make_pointer_ty(KOOPA_RTT_INT32);
        temp_alloc->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_alloc->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.addInst(temp_alloc);
        koopa_raw_value_data *temp_store = new koopa_raw_value_data;
        temp_store->name = nullptr;
        temp_store->ty = make_ty(KOOPA_RTT_INT32);
        temp_store->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_store->kind.tag = KOOPA_RVT_STORE;
        temp_store->kind.data.store.dest = (koopa_raw_value_t)temp_alloc;
        temp_store->kind.data.store.value = (koopa_raw_value_t)NumberAST(0).toKoopaIR();
        block_manager.addInst(temp_store);
        koopa_raw_value_data *temp_branch = new koopa_raw_value_data;
        temp_branch->name = nullptr;
        temp_branch->ty = make_ty(KOOPA_RTT_UNIT);
        temp_branch->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_branch->kind.tag = KOOPA_RVT_BRANCH;
        temp_branch->kind.data.branch.cond = (koopa_raw_value_t)toBoolKoopaIR(land_exp);
        koopa_raw_basic_block_data_t *temp_then = new koopa_raw_basic_block_data_t;
        temp_then->name = add_prefix("%", "then");
        temp_then->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_then->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_branch->kind.data.branch.true_bb = (koopa_raw_basic_block_t)temp_then;
        temp_branch->kind.data.branch.true_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        koopa_raw_basic_block_data_t *temp_end = new koopa_raw_basic_block_data_t;
        temp_end->name = add_prefix("%", "end");
        temp_end->params = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_end->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_branch->kind.data.branch.false_bb = (koopa_raw_basic_block_t)temp_end;
        temp_branch->kind.data.branch.false_args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        block_manager.addInst(temp_branch);
        block_manager.newBlock(temp_then);
        koopa_raw_value_data *temp_store2 = new koopa_raw_value_data;
        temp_store2->name = nullptr;
        temp_store2->ty = make_ty(KOOPA_RTT_INT32);
        temp_store2->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        temp_store2->kind.tag = KOOPA_RVT_STORE;
        temp_store2->kind.data.store.dest = (koopa_raw_value_t)temp_alloc;
        temp_store2->kind.data.store.value = (koopa_raw_value_t)toBoolKoopaIR(eq_exp);
        block_manager.addInst(temp_store2);
        block_manager.addInst(jumpInst((koopa_raw_basic_block_t)temp_end));
        block_manager.newBlock(temp_end);
        koopa_raw_value_data *ret = new koopa_raw_value_data;
        ret->name = nullptr;
        ret->ty = make_ty(KOOPA_RTT_INT32);
        ret->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_LOAD;
        ret->kind.data.load.src = (koopa_raw_value_t)temp_alloc;
        block_manager.addInst(ret);
        return ret;
    }
    else if(op == " ") {
        return eq_exp->toKoopaIR();
    }
    return nullptr;
}

int LAndExpAST::calculate() const {
    if (op == "&&") {
        return land_exp->calculate() && eq_exp->calculate();
    }
    else if (op == " ") {
        return eq_exp->calculate();
    }
    return 0;
}

void *LAndExpAST::toBoolKoopaIR(const std::unique_ptr<BaseAST> &exp) const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = nullptr;
    ir->ty = make_ty(KOOPA_RTT_INT32);
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->kind.tag = KOOPA_RVT_BINARY;
    ir->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
    NumberAST zero(0);
    ir->kind.data.binary.lhs = (koopa_raw_value_t)zero.toKoopaIR();
    ir->kind.data.binary.rhs = (koopa_raw_value_t)exp->toKoopaIR();
    block_manager.addInst(ir);
    return ir;
}

// EqExpAST 类的实现
void EqExpAST::Dump() const {
    std::cout << "EqExpAST { ";
    if(op == "==" || op == "!=") {
        eq_exp->Dump();
        std::cout << " " << op << " ";
        rel_exp->Dump();
    }
    else if(op == " ") {
        rel_exp->Dump();
    }
    std::cout << " }";
}

void *EqExpAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    if(op != " ") {
        ir->name = nullptr;
        ir->ty = make_ty(KOOPA_RTT_INT32);
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.tag = KOOPA_RVT_BINARY;
        if(op == "==") {
            ir->kind.data.binary.op = KOOPA_RBO_EQ;
        }
        else if(op == "!=") {
            ir->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
        }
        ir->kind.data.binary.lhs = (koopa_raw_value_t)eq_exp->toKoopaIR();
        ir->kind.data.binary.rhs = (koopa_raw_value_t)rel_exp->toKoopaIR();
        block_manager.addInst(ir);
        return ir;
    }
    else if(op == " ") {
        return rel_exp->toKoopaIR();
    }
    return nullptr;
}

int EqExpAST::calculate() const {
    if(op == "==") {
        return eq_exp->calculate() == rel_exp->calculate();
    }
    else if(op == "!=") {
        return eq_exp->calculate() != rel_exp->calculate();
    }
    else if(op == " ") {
        return rel_exp->calculate();
    }
    return 0;
}

// RelExpAST 类的实现
void RelExpAST::Dump() const {
    std::cout << "RelExpAST { ";
    if(op == "<" || op == ">" || op == "<=" || op == ">=") {
        rel_exp->Dump();
        std::cout << " " << op << " ";
        add_exp->Dump();
    }
    else if(op == " ") {
        add_exp->Dump();
    }
    std::cout << " }";
}

void *RelExpAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    if(op != " ") {
        ir->name = nullptr;
        ir->ty = make_ty(KOOPA_RTT_INT32);
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.tag = KOOPA_RVT_BINARY;
        if(op == "<") {
            ir->kind.data.binary.op = KOOPA_RBO_LT;
        }
        else if(op == ">") {
            ir->kind.data.binary.op = KOOPA_RBO_GT;
        }
        else if(op == "<=") {
            ir->kind.data.binary.op = KOOPA_RBO_LE;
        }
        else if(op == ">=") {
            ir->kind.data.binary.op = KOOPA_RBO_GE;
        }
        ir->kind.data.binary.lhs = (koopa_raw_value_t)rel_exp->toKoopaIR();
        ir->kind.data.binary.rhs = (koopa_raw_value_t)add_exp->toKoopaIR();
        block_manager.addInst(ir);
        return ir;
    }
    else if(op == " ") {
        return add_exp->toKoopaIR();
    }
    return nullptr;
}

int RelExpAST::calculate() const {
    if(op == "<") {
        return rel_exp->calculate() < add_exp->calculate();
    }
    else if(op == ">") {
        return rel_exp->calculate() > add_exp->calculate();
    }
    else if(op == "<=") {
        return rel_exp->calculate() <= add_exp->calculate();
    }
    else if(op == ">=") {
        return rel_exp->calculate() >= add_exp->calculate();
    }
    else if(op == " ") {
        return add_exp->calculate();
    }
    return 0;
}

// AddExpAST 类的实现
void AddExpAST::Dump() const {
    std::cout << "AddExpAST { ";
    if(op == "+" || op == "-") {
        add_exp->Dump();
        std::cout << " " << op << " ";
        mul_exp->Dump();
    }
    else if(op == " ") {
        mul_exp->Dump();
    }
    std::cout << " }";
}

void *AddExpAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    if(op != " ") {
        ir->name = nullptr;
        ir->ty = make_ty(KOOPA_RTT_INT32);
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.tag = KOOPA_RVT_BINARY;
        if(op == "+") {
            ir->kind.data.binary.op = KOOPA_RBO_ADD;
        }
        else if(op == "-") {
            ir->kind.data.binary.op = KOOPA_RBO_SUB;
        }
        ir->kind.data.binary.lhs = (koopa_raw_value_t)add_exp->toKoopaIR();
        ir->kind.data.binary.rhs = (koopa_raw_value_t)mul_exp->toKoopaIR();
        block_manager.addInst(ir);
        return ir;
    }
    else if(op == " ") {
        return mul_exp->toKoopaIR();
    }
    return nullptr;
}

int AddExpAST::calculate() const {
    if(op == "+") {
        return add_exp->calculate() + mul_exp->calculate();
    }
    else if(op == "-") {
        return add_exp->calculate() - mul_exp->calculate();
    }
    else if(op == " ") {
        return mul_exp->calculate();
    }
    return 0;
}

// MulExpAST 类的实现
void MulExpAST::Dump() const {
    std::cout << "MulExpAST { ";
    if(op == "*" || op == "/" || op == "%") {
        mul_exp->Dump();
        std::cout << " " << op << " ";
        unary_exp->Dump();
    }
    else if(op == " ") {
        unary_exp->Dump();
    }
    std::cout << " }";
}


void *MulExpAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    if(op != " ") {
        ir->name = nullptr;
        ir->ty = make_ty(KOOPA_RTT_INT32);
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.tag = KOOPA_RVT_BINARY;
        if(op == "*") {
            ir->kind.data.binary.op = KOOPA_RBO_MUL;
        }
        else if(op == "/") {
            ir->kind.data.binary.op = KOOPA_RBO_DIV;
        }
        else if(op == "%") {
            ir->kind.data.binary.op = KOOPA_RBO_MOD;
        }
        ir->kind.data.binary.lhs = (koopa_raw_value_t)mul_exp->toKoopaIR();
        ir->kind.data.binary.rhs = (koopa_raw_value_t)unary_exp->toKoopaIR();
        block_manager.addInst(ir);
        return ir;
    }
    else if(op == " ") {
        return unary_exp->toKoopaIR();
    }
    return nullptr;
}

int MulExpAST::calculate() const {
    if(op == "*") {
        return mul_exp->calculate() * unary_exp->calculate();
    }
    else if(op == "/") {
        return mul_exp->calculate() / unary_exp->calculate();
    }
    else if(op == "%") {
        return mul_exp->calculate() % unary_exp->calculate();
    }
    else if(op == " ") {
        return unary_exp->calculate();
    }
    return 0;
}

void *UnaryExpAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    if(tag == "OPEXP") {
        ir->name = nullptr;  
        ir->ty = make_ty(KOOPA_RTT_INT32);
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.tag = KOOPA_RVT_BINARY;
        if(unary_op == "-") {
            ir->kind.data.binary.op = KOOPA_RBO_SUB;
            NumberAST zero(0);
            ir->kind.data.binary.lhs = (koopa_raw_value_t)zero.toKoopaIR();
        }
        else if(unary_op == "+") {
            return unary_exp->toKoopaIR();
        }
        else if(unary_op == "!") {
            ir->kind.data.binary.op = KOOPA_RBO_EQ;
            NumberAST zero(0);
            ir->kind.data.binary.lhs = (koopa_raw_value_t)zero.toKoopaIR();
        }
        ir->kind.data.binary.rhs = (koopa_raw_value_t)unary_exp->toKoopaIR();
        block_manager.addInst(ir);
        return ir;
    }
    else if(tag == "PEXP") {
        return primary_exp->toKoopaIR();
    }
    else if(tag == "FCALL") {
        koopa_raw_function_t func = symbol_list.getSymbol(ident).data.func_value;
        ir->name = nullptr;
        ir->ty = func->ty->data.function.ret;
        ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        ir->kind.tag = KOOPA_RVT_CALL;
        ir->kind.data.call.callee = func;
        std::vector<const void *> args;
        for(auto &arg : *func_rparams) {
            args.push_back(arg->toKoopaIR());
        }
        if(args.size() > 0) {
            ir->kind.data.call.args = make_slice(&args, KOOPA_RSIK_VALUE);
        }
        else {
            ir->kind.data.call.args = make_slice(nullptr, KOOPA_RSIK_VALUE);
        }
        block_manager.addInst(ir);
        return ir;
    }
    return nullptr;
}

int UnaryExpAST::calculate() const {
    if(tag == "OPEXP") {
        if(unary_op == "-") {
            return -unary_exp->calculate();
        }
        else if(unary_op == "+") {
            return unary_exp->calculate();
        }
        else if(unary_op == "!") {
            return !unary_exp->calculate();
        }
    }
    else if(tag == "PEXP") {
        return primary_exp->calculate();
    }
    return 0;
}

void UnaryExpAST::Dump() const {
    std::cout << "UnaryExpAST { ";
    if(tag == "OPEXP") {
        std::cout << unary_op << " , ";
        unary_exp->Dump();
    }
    else if(tag == "PEXP") {
        primary_exp->Dump();
    }
    std::cout << " }";
}

void *PrimaryExpAST::toKoopaIR() const {
    if(tag == "EXP") {
        return exp->toKoopaIR();
    }
    else if(tag == "NUM") {
        return number->toKoopaIR();
    }
    else if(tag == "LVAL") {
        return lval->toKoopaIR();
    }
    return nullptr;
}

int PrimaryExpAST::calculate() const {
    if(tag == "EXP") {
        return exp->calculate();
    }
    else if(tag == "NUM") {
        return number->calculate();
    }
    else if(tag == "LVAL") {
        return lval->calculate();
    }
    return 0;
}

void PrimaryExpAST::Dump() const {
    std::cout << "PrimartExpAST { ";
    if(tag == "EXP") {
        exp->Dump();
    }
    else if(tag == "NUM") {
        number->Dump();
    }
    else if(tag == "LVAL") {
        lval->Dump();
    }
    std::cout << " }";
}

void *LValAST::toKoopaIR() const {
    Value value = symbol_list.getSymbol(ident);
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = nullptr;
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->ty = make_ty(KOOPA_RTT_INT32);
    if(value.type == ValueType::Var) {
        ir->kind.tag = KOOPA_RVT_LOAD;
        ir->kind.data.load.src = (koopa_raw_value_t)value.data.var_value;
        block_manager.addInst(ir);
    }
    else if(value.type == ValueType::Const) {
        ir->kind.tag = KOOPA_RVT_INTEGER;
        ir->kind.data.integer.value = value.data.const_value;
    }
    else if(value.type == ValueType::Array)
    {
        koopa_raw_value_t last = value.data.array_value;
        if(array != nullptr)
        {
            for(int i=0;i<array->size();i++)
            {
                koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
                get_elem_ptr->name = nullptr;
                get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                koopa_raw_type_kind *ty = new koopa_raw_type_kind;
                ty->tag = KOOPA_RTT_POINTER;
                ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
                get_elem_ptr->ty = ty;
                get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)array->at(i)->toKoopaIR();
                get_elem_ptr->kind.data.get_elem_ptr.src = last;
                block_manager.addInst(get_elem_ptr);
                last = get_elem_ptr;   
            }
        }
        if(array == nullptr)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            koopa_raw_type_kind *ty = new koopa_raw_type_kind;
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
            get_elem_ptr->ty = ty;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).toKoopaIR();
            get_elem_ptr->kind.data.get_elem_ptr.src = (koopa_raw_value_t)value.data.array_value;
            block_manager.addInst(get_elem_ptr);
            ir = get_elem_ptr;
        }
        else if(last->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            koopa_raw_type_kind *ty = new koopa_raw_type_kind;
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
            get_elem_ptr->ty = ty;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).toKoopaIR();
            get_elem_ptr->kind.data.get_elem_ptr.src = last;
            block_manager.addInst(get_elem_ptr);
            ir = get_elem_ptr;
        }
        else
        {
            ir->kind.tag = KOOPA_RVT_LOAD;
            ir->kind.data.load.src = last;
            block_manager.addInst(ir);
        }
    }
    else if(value.type == ValueType::Pointer)
    {
        koopa_raw_value_data *load = new koopa_raw_value_data();
        load->name = nullptr;
        load->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        load->kind.tag = KOOPA_RVT_LOAD;
        load->ty = value.data.pointer_value->ty->data.pointer.base;
        load->kind.data.load.src = (koopa_raw_value_t)value.data.pointer_value;
        block_manager.addInst(load);
        koopa_raw_value_t last = value.data.pointer_value;

        if(array!=nullptr)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_PTR;
            get_elem_ptr->ty = load->ty;
            get_elem_ptr->kind.data.get_ptr.src = load;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)array->at(0)->toKoopaIR();
            block_manager.addInst(get_elem_ptr);

            last = get_elem_ptr;

            for(int i=1;i<array->size();i++)
            {
                koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
                get_elem_ptr->name = nullptr;
                get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
                get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                koopa_raw_type_kind *ty = new koopa_raw_type_kind;
                ty->tag = KOOPA_RTT_POINTER;
                ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
                get_elem_ptr->ty = ty;
                get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)array->at(i)->toKoopaIR();
                get_elem_ptr->kind.data.get_elem_ptr.src = last;
                block_manager.addInst(get_elem_ptr);
                last = get_elem_ptr;         
            }
        }
        if(array == nullptr)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_PTR;
            get_elem_ptr->ty = load->ty;
            get_elem_ptr->kind.data.get_ptr.src = load;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).toKoopaIR();
            block_manager.addInst(get_elem_ptr);
            ir = get_elem_ptr;
        }
        else if(last->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            koopa_raw_type_kind *ty = new koopa_raw_type_kind;
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
            get_elem_ptr->ty = ty;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)NumberAST(0).toKoopaIR();
            get_elem_ptr->kind.data.get_elem_ptr.src = last;
            block_manager.addInst(get_elem_ptr);
            ir = get_elem_ptr;
        }
        else
        {
            ir->kind.tag = KOOPA_RVT_LOAD;
            ir->kind.data.load.src = (koopa_raw_value_t)last;
            block_manager.addInst(ir);
        }
    }
    return ir;
}

int LValAST::calculate() const {
    return symbol_list.getSymbol(ident).data.const_value;
}

void LValAST::Dump() const {
    std::cout << "LValAST { " << ident << " }";
}

void *LValAST::getKoopaIR() const {
    Value value = symbol_list.getSymbol(ident);
    if(value.type == ValueType::Var) {
        return (void *)value.data.var_value;
    }
    else if(value.type == ValueType::Array)
    {
        koopa_raw_value_t last = value.data.array_value;
        for(int i=0;i<array->size();i++)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            koopa_raw_type_kind *ty = new koopa_raw_type_kind;
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
            get_elem_ptr->ty = ty;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)array->at(i)->toKoopaIR();
            get_elem_ptr->kind.data.get_elem_ptr.src = last;
            block_manager.addInst(get_elem_ptr);
            last = get_elem_ptr;         
        }
        return (void*)last;
    }
    else if(value.type == ValueType::Pointer)
    {
        koopa_raw_value_t last = value.data.pointer_value;

        koopa_raw_value_data *load = new koopa_raw_value_data();
        load->name = nullptr;
        load->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        load->kind.tag = KOOPA_RVT_LOAD;
        load->ty = last->ty->data.pointer.base;
        load->kind.data.load.src = (koopa_raw_value_t)last;
        block_manager.addInst(load);


        koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
        get_elem_ptr->name = nullptr;
        get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
        get_elem_ptr->kind.tag = KOOPA_RVT_GET_PTR;
        get_elem_ptr->ty = load->ty;
        get_elem_ptr->kind.data.get_ptr.src = load;
        get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)array->at(0)->toKoopaIR();
        block_manager.addInst(get_elem_ptr);

        last = get_elem_ptr;

        for(int i=1;i<array->size();i++)
        {
            koopa_raw_value_data *get_elem_ptr = new koopa_raw_value_data();
            get_elem_ptr->name = nullptr;
            get_elem_ptr->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
            get_elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
            koopa_raw_type_kind *ty = new koopa_raw_type_kind;
            ty->tag = KOOPA_RTT_POINTER;
            ty->data.pointer.base = last->ty->data.pointer.base->data.array.base;
            get_elem_ptr->ty = ty;
            get_elem_ptr->kind.data.get_elem_ptr.index = (koopa_raw_value_t)array->at(i)->toKoopaIR();
            get_elem_ptr->kind.data.get_elem_ptr.src = last;
            block_manager.addInst(get_elem_ptr);
            last = get_elem_ptr;         
        }
        return (void *)last;
    }

    return nullptr;
}

// NumberAST 类的实现
NumberAST::NumberAST(int int_const) : int_const(int_const) {}

void NumberAST::Dump() const {
    std::cout << "NumberAST { " << int_const << " }";
}

void *NumberAST::toKoopaIR() const {
    koopa_raw_value_data_t *ir = new koopa_raw_value_data_t;
    ir->name = nullptr;
    ir->ty = make_ty(KOOPA_RTT_INT32);
    ir->used_by = make_slice(nullptr, KOOPA_RSIK_VALUE);
    ir->kind.tag = KOOPA_RVT_INTEGER;
    ir->kind.data.integer.value = int_const;
    return ir;
}

int NumberAST::calculate() const {
    return int_const;
}