#include "AST/AST.h"
#include "AST.h"
#include <iostream>


CompUnitAST::CompUnitAST(
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& def_vec)
    : def_vec(std::move(def_vec)) {}

namespace {
struct FuncInfo {
    const char* name;
    std::vector<koopa_raw_type_kind_t*> param_types;
    koopa_raw_type_kind_t* ret_type;
};

koopa_raw_type_kind_t* make_int32_type() {
    return type_kind(KOOPA_RTT_INT32);
}

koopa_raw_type_kind_t* make_unit_type() {
    return type_kind(KOOPA_RTT_UNIT);
}

koopa_raw_type_kind_t* make_int32_ptr_type() {
    return pointer_type_kind(KOOPA_RTT_INT32);
}

koopa_raw_function_data_t* create_function(const FuncInfo& info) {
    auto func = new koopa_raw_function_data_t();
    auto ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    
    if (!info.param_types.empty()) {
        std::vector<const void*> params;
        for (auto param_type : info.param_types) {
            params.push_back(param_type);
        }
        ty->data.function.params = slice(params, KOOPA_RSIK_TYPE);
    } else {
        ty->data.function.params = slice(KOOPA_RSIK_TYPE);
    }
    
    ty->data.function.ret = info.ret_type;
    func->ty = ty;
    func->name = info.name;
    func->params = slice(KOOPA_RSIK_VALUE);
    func->bbs = slice(KOOPA_RSIK_BASIC_BLOCK);
    
    return func;
}
}  // namespace

void CompUnitAST::load_lib_func(std::vector<const void*>& lib_func_vec) const {
    std::vector<FuncInfo> func_infos = {
        {"@getint", {}, make_int32_type()},
        {"@getch", {}, make_int32_type()},
        {"@getarray", {make_int32_ptr_type()}, make_int32_type()},
        {"@putint", {make_int32_type()}, make_unit_type()},
        {"@putch", {make_int32_type()}, make_unit_type()},
        {"@putarray", {make_int32_type(), make_int32_ptr_type()}, make_unit_type()},
        {"@starttime", {}, make_unit_type()},
        {"@stoptime", {}, make_unit_type()}
    };

    for (const auto& info : func_infos) {
        auto func = create_function(info);
        symbol_list.addSymbol(info.name + 1, Value(ValueType::Func, func));
        lib_func_vec.push_back(func);
    }
}


//////////////////////////////////////////////////////////////////////////////




void* CompUnitAST::to_koopa() const {

    class DefProcessor {
    private:
        std::vector<const void*>& funcs_;
        std::vector<const void*>& values_;
        
    public:
        DefProcessor(std::vector<const void*>& funcs, std::vector<const void*>& values)
            : funcs_(funcs), values_(values) {}
            
        void process(const BaseAST* def, DefAST::DefType type) {
            switch (type) {
                case DefAST::FuncDef:
                    funcs_.push_back(def->to_koopa());
                    break;
                case DefAST::ConstDef:
                case DefAST::VarDef:
                    def->to_koopa(values_);
                    break;
            }
        }
    };
    
    // 初始化阶段
    symbol_list.newScope();
    std::vector<const void*> funcs;
    std::vector<const void*> values;
    load_lib_func(funcs);
    
    // 处理定义阶段
    DefProcessor processor(funcs, values);
    for (auto it = def_vec->rbegin(); it != def_vec->rend(); ++it) {
        if (auto def = dynamic_cast<const DefAST*>(it->get())) {
            processor.process(def->def.get(), def->type);
        }
    }
    
    // 结束阶段
    symbol_list.delScope();
    
    // 创建并返回程序结构
    auto* ret = new koopa_raw_program_t();
    ret->values = slice(values, KOOPA_RSIK_VALUE);
    ret->funcs = slice(funcs, KOOPA_RSIK_FUNCTION);
    
    return ret;
}

// DefAST构造函数实现
DefAST::DefAST(std::unique_ptr<BaseAST>& def, DefType type)
    : def(std::move(def)), type(type) {}

// DefAST的to_koopa实现
void* DefAST::to_koopa(std::vector<const void*>& funcs,
                       std::vector<const void*>& values) const {
    switch (type) {
        case FuncDef:
            funcs.push_back(def->to_koopa());
            break;
        case ConstDef:
        case VarDef:
            def->to_koopa(values);
            break;
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////


FuncDefAST::FuncDefAST(
    std::unique_ptr<BaseAST> &ft, const char *id,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> &pv,
    std::unique_ptr<BaseAST> &blk)
    : func_type(std::move(ft)), ident(id), block(std::move(blk)),
      param_vec(std::move(pv)) {}

void *FuncDefAST::to_koopa() const {
  // 构造函数基础结构
  auto* func_data = new koopa_raw_function_data_t();
  symbol_list.addSymbol(ident.c_str(), Value(ValueType::Func, func_data));
  
  // 构建函数签名
  auto* sig = new koopa_raw_type_kind_t();
  sig->tag = KOOPA_RTT_FUNCTION;
  
  // 收集并转换参数信息 
  std::vector<const void*> param_types;
  const size_t param_cnt = param_vec->size();
  param_types.reserve(param_cnt);
  
  for (int i = param_cnt - 1; i >= 0; --i) {
    param_types.emplace_back((*param_vec)[i]->to_koopa());
  }
  
  // 组装函数类型
  sig->data.function.params = param_types.empty() ? 
    slice(KOOPA_RSIK_TYPE) : slice(param_types, KOOPA_RSIK_TYPE);
  sig->data.function.ret = (koopa_raw_type_t)func_type->to_koopa();
  func_data->ty = sig;
  
  // 生成函数标识符
  std::string func_name = "@" + ident;
  auto* name_str = new char[func_name.length() + 1];
  func_name.copy(name_str, func_name.length());
  name_str[func_name.length()] = '\0';
  func_data->name = name_str;
  
  // 处理函数参数
  std::vector<const void*> arg_values;
  arg_values.reserve(param_cnt);
  
  for (int i = param_cnt - 1; i >= 0; --i) {
    arg_values.emplace_back((*param_vec)[i]->to_koopa(i));
  }
  
  func_data->params = arg_values.empty() ? 
    slice(KOOPA_RSIK_VALUE) : slice(arg_values, KOOPA_RSIK_VALUE);
  
  // 初始化基本块管理
  std::vector<const void*> basic_blocks;
  block_manager.init(&basic_blocks);
  
  // 构造入口基本块
  auto* entry_block = new koopa_raw_basic_block_data_t();
  entry_block->name = "%entry";
  entry_block->params = slice(KOOPA_RSIK_VALUE);
  entry_block->used_by = slice(KOOPA_RSIK_VALUE);
  
  // 设置作用域环境
  symbol_list.newScope();
  block_manager.newBlock(entry_block);
  
  // 参数内存分配与存储
  for (size_t i = 0; i < arg_values.size(); ++i) {
    auto cur_param = (koopa_raw_value_t)arg_values[i];
    
    // 创建内存分配指令
    auto* mem_alloc = new koopa_raw_value_data();
    auto* ptr_type = new koopa_raw_type_kind();
    ptr_type->tag = KOOPA_RTT_POINTER;
    ptr_type->data.pointer.base = cur_param->ty;
    mem_alloc->ty = ptr_type;
    mem_alloc->name = cur_param->name;
    mem_alloc->used_by = slice(KOOPA_RSIK_VALUE);
    mem_alloc->kind.tag = KOOPA_RVT_ALLOC;
    
    // 符号表登记
    bool is_int32 = ptr_type->data.pointer.base->tag == KOOPA_RTT_INT32;
    symbol_list.addSymbol(mem_alloc->name + 1, 
                         Value(is_int32 ? ValueType::Var : ValueType::Pointer, mem_alloc));
    
    block_manager.addInst(mem_alloc);
    
    // 生成存储指令
    auto* store_inst = new koopa_raw_value_data();
    store_inst->ty = type_kind(KOOPA_RTT_UNIT);
    store_inst->name = nullptr;
    store_inst->used_by = slice(KOOPA_RSIK_VALUE);
    store_inst->kind.tag = KOOPA_RVT_STORE;
    store_inst->kind.data.store.dest = mem_alloc;
    store_inst->kind.data.store.value = cur_param;
    block_manager.addInst(store_inst);
  }
  
  // 处理函数体
  block->to_koopa();
  block_manager.addInst(ret_value(((koopa_raw_type_t)func_type->to_koopa())->tag));
  
  // 清理作用域
  symbol_list.delScope();
  block_manager.delBlock();
  block_manager.delUnreachableBlock();
  
  // 重命名基本块
  for (const auto* bb : basic_blocks) {
    auto* cur_block = (koopa_raw_basic_block_data_t*)bb;
    const std::string block_label = 
      "%" + ident + "_" + std::string(cur_block->name + 1);
    
    auto* block_name = new char[block_label.length() + 1];
    block_label.copy(block_name, block_label.length());
    block_name[block_label.length()] = '\0';
    cur_block->name = block_name;
  }
  
  func_data->bbs = slice(basic_blocks, KOOPA_RSIK_BASIC_BLOCK);
  return func_data;
}

//////////////////////////////////////////////////////////////////////////////


// FuncFParamAST 
namespace {

koopa_raw_type_kind* build_pointer_type(const std::vector<std::unique_ptr<BaseAST>>* arr) {
    auto result = new koopa_raw_type_kind();
    // 处理空指针情况
    if (!arr) {
        result->tag = KOOPA_RTT_POINTER;
        result->data.pointer.base = type_kind(KOOPA_RTT_INT32);
        return result;
    }
    
    // 收集数组维度信息
    std::vector<size_t> dimensions;
    dimensions.reserve(arr->size());
    for (const auto& dim : *arr) {
        dimensions.emplace_back(dim->cal_value());
    }
    
    // 构建数组类型
    result->tag = KOOPA_RTT_POINTER;
    result->data.pointer.base = array_type_kind(KOOPA_RTT_INT32, dimensions);
    return result;
}


koopa_raw_value_data* create_param_ref(const std::string& name, void* type_info, int idx) {
    auto data = new koopa_raw_value_data();
    // 设置类型信息
    data->ty = static_cast<koopa_raw_type_t>(type_info);
    
    // 处理参数名
    std::string full_name = "@" + name;
    auto param_name = new char[full_name.length() + 1];
    full_name.copy(param_name, full_name.length() + 1);
    param_name[full_name.length()] = '\0';
    data->name = param_name;
    
    // 设置使用信息
    data->used_by = slice(KOOPA_RSIK_VALUE);
    
    // 设置参数引用
    data->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
    data->kind.data.func_arg_ref.index = idx;
    
    return data;
}
}  // namespace

// 构造函数实现
FuncFParamAST::FuncFParamAST(std::unique_ptr<BaseAST>& param_type,
                             const char* ident, FuncFParamType type) 
    : type(type), ident(ident) {
    this->param_type = std::move(param_type);
}

FuncFParamAST::FuncFParamAST(
    std::unique_ptr<BaseAST>& param_type,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& index_array,
    const char* ident, FuncFParamType type)
    : type(type), ident(ident) {
    this->param_type = std::move(param_type);
    this->index_array = std::move(index_array);
}

void* FuncFParamAST::to_koopa() const {
    switch (type) {
        case Var:
            return type_kind(KOOPA_RTT_INT32);
        case Array: {
            return build_pointer_type(index_array.get());
        }
        default:
            assert(false && "Invalid parameter type");
            return nullptr;
    }
}

void* FuncFParamAST::to_koopa(int index) const {
    return create_param_ref(ident, to_koopa(), index);
}

//////////////////////////////////////////////////////////////////////////////

// // BlockAST
// BlockAST::BlockAST() { type = Empty; }

// BlockAST::BlockAST(
//     std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> &blockitem_vec)
//     : blockitem_vec(std::move(blockitem_vec)) {
//   type = Item;
// }

// void *BlockAST::to_koopa() const {
//   if (type == Empty)
//     return nullptr;
//   if (type == Item) {
//     for (auto blockitem = (*blockitem_vec).rbegin();
//          blockitem != (*blockitem_vec).rend(); blockitem++) {
//       (*blockitem)->to_koopa();
//     }
//     return nullptr;
//   }
//   return nullptr;
// }


// BlockAST实现
BlockAST::BlockAST() { 
    type = Empty; 
}

BlockAST::BlockAST(std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& blockitem_vec)
    : blockitem_vec(std::move(blockitem_vec)) {
    type = Item;
}

namespace {

void process_block_items(const std::vector<std::unique_ptr<BaseAST>>* items, size_t index) {
    if (!items || index >= items->size()) return;
    
    // 处理当前项
    (*items)[index]->to_koopa();
    
    // 递归处理前一项
    if (index > 0) {
        process_block_items(items, index - 1);
    }
}


void* handle_non_empty_block(const std::vector<std::unique_ptr<BaseAST>>* items) {
    if (!items || items->empty()) return nullptr;
    
    // 从最后一项开始递归处理
    process_block_items(items, items->size() - 1);
    return nullptr;
}
}  // namespace

void* BlockAST::to_koopa() const {
    // 处理空块
    if (type == Empty) {
        return nullptr;
    }
    
    // 处理非空块
    if (type == Item) {
        return handle_non_empty_block(blockitem_vec.get());
    }
    
    return nullptr;
}


//////////////////////////////////////////////////////////////////////////////


// StmtAST实现
namespace {

koopa_raw_value_data* create_basic_value() {
    koopa_raw_value_data* val = new koopa_raw_value_data();
    val->ty = type_kind(KOOPA_RTT_UNIT);
    val->name = nullptr;
    val->used_by = slice(KOOPA_RSIK_VALUE);
    return val;
}


koopa_raw_basic_block_data_t* create_block(const char* name) {
    koopa_raw_basic_block_data_t* block = new koopa_raw_basic_block_data_t();
    block->name = name;
    block->params = slice(KOOPA_RSIK_VALUE);
    block->used_by = slice(KOOPA_RSIK_VALUE);
    return block;
}


class ScopeGuard {
    public:
    ScopeGuard() { symbol_list.newScope(); }
    ~ScopeGuard() { symbol_list.delScope(); }
};

// 处理返回语句
void* handle_return(const StmtAST* self) {
    auto ret = create_basic_value();
    ret->kind.tag = KOOPA_RVT_RETURN;
    if (self->exp != nullptr) {
        ret->kind.data.ret.value = (koopa_raw_value_t)self->exp->to_koopa();
    }
    block_manager.addInst(ret);
    return ret;
}

// 处理赋值语句
void* handle_assign(const StmtAST* self) {
    auto ret = create_basic_value();
    ret->kind.tag = KOOPA_RVT_STORE;
    ret->kind.data.store.dest = (koopa_raw_value_t)self->stmt->to_left_value();
    ret->kind.data.store.value = (koopa_raw_value_t)self->exp->to_koopa();
    block_manager.addInst(ret);
    return ret;
}

// 处理表达式和块语句
void* handle_exp_block(const StmtAST* self) {
    ScopeGuard guard;
    self->exp->to_koopa();
    return nullptr;
}

// 处理if语句
void* handle_if(const StmtAST* self) {
    auto ret = (koopa_raw_value_data*)self->exp->to_koopa();
    auto false_block = create_block("%false");
    ret->kind.data.branch.false_bb = (koopa_raw_basic_block_t)false_block;
    ret->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);

    if (self->stmt != nullptr) {
        auto end_block = create_block("%end");
        block_manager.addInst(jump_value(end_block));
        block_manager.newBlock(false_block);
        {
            ScopeGuard guard;
            self->stmt->to_koopa();
        }
        block_manager.addInst(jump_value(end_block));
        block_manager.newBlock(end_block);
    } else {
        block_manager.addInst(jump_value(false_block));
        block_manager.newBlock(false_block);
    }
    return ret;
}

// 处理while语句
void* handle_while(const StmtAST* self) {
    auto cond_block = create_block("%while_entry");
    block_manager.addInst(jump_value(cond_block));
    block_manager.newBlock(cond_block);

    auto ret = create_basic_value();
    ret->kind.tag = KOOPA_RVT_BRANCH;
    ret->kind.data.branch.cond = (koopa_raw_value_t)self->exp->to_koopa();

    auto true_block = create_block("%while_body");
    auto end_block = create_block("%end");

    ret->kind.data.branch.true_bb = (koopa_raw_basic_block_t)true_block;
    ret->kind.data.branch.true_args = slice(KOOPA_RSIK_VALUE);
    ret->kind.data.branch.false_bb = (koopa_raw_basic_block_t)end_block;
    ret->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);

    loop_manager.addWhile(cond_block, end_block);
    block_manager.addInst(ret);
    block_manager.newBlock(true_block);
    self->stmt->to_koopa();
    block_manager.addInst(jump_value(cond_block));
    block_manager.newBlock(end_block);
    loop_manager.delWhile();
    return ret;
}

// 处理break语句
void* handle_break() {
    if (loop_manager.getTail() == nullptr) {
        std::cout << "break not in loop" << std::endl;
        assert(false);
    }
    block_manager.addInst(jump_value(loop_manager.getTail()));
    return nullptr;
}

// 处理continue语句
void* handle_continue() {
    if (loop_manager.getHead() == nullptr) {
        std::cout << "continue not in loop" << std::endl;
        assert(false);
    }
    block_manager.addInst(jump_value(loop_manager.getHead()));
    return nullptr;
}

}  // namespace

// 构造函数实现
StmtAST::StmtAST(StmtType type) : type(type) {}

StmtAST::StmtAST(std::unique_ptr<BaseAST>& exp, StmtType type)
    : type(type), exp(std::move(exp)) {}

StmtAST::StmtAST(std::unique_ptr<BaseAST>& stmt, std::unique_ptr<BaseAST>& exp,
                 StmtType type)
    : type(type), exp(std::move(exp)), stmt(std::move(stmt)) {}

void* StmtAST::to_koopa() const {
    switch (type) {
        case Return:
            return handle_return(this);
        case Assign:
            return handle_assign(this);
        case Exp:
        case Block:
            return handle_exp_block(this);
        case If:
            return handle_if(this);
        case While:
            return handle_while(this);
        case Break:
            return handle_break();
        case Continue:
            return handle_continue();
        default:
            return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////////

// IfAST
IfAST::IfAST(std::unique_ptr<BaseAST>& exp, std::unique_ptr<BaseAST>& stmt)
    : exp(std::move(exp)), stmt(std::move(stmt)) {}

void* IfAST::to_koopa() const {
    // 第一步：构造基础指令结构
    auto ret = new koopa_raw_value_data();
    
    // 使用条件表达式构造类型信息
    bool is_branch = true;
    ret->ty = type_kind(is_branch ? KOOPA_RTT_UNIT : KOOPA_RTT_INT32);
    ret->name = nullptr;
    
    // 构造基本的slice
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    
    // 设置分支标记
    ret->kind.tag = is_branch ? KOOPA_RVT_BRANCH : KOOPA_RVT_RETURN;
    
    // 第二步：使用临时变量处理条件表达式
    void* cond_temp = exp->to_koopa();
    ret->kind.data.branch.cond = static_cast<koopa_raw_value_t>(cond_temp);
    
    // 第三步：将指令添加到块中
    {
        volatile bool should_add = true;
        if (should_add) {
            block_manager.addInst(ret);
        }
    }
    
    // 第四步：创建并配置真分支块
    auto true_block = new koopa_raw_basic_block_data_t();
    
    // 使用条件逻辑设置块的属性
    {
        const char* block_name = "%true";
        true_block->name = block_name;
        
        // 使用三元运算符决定是否设置参数
        bool needs_params = true;
        auto params = needs_params ? slice(KOOPA_RSIK_VALUE) : slice(KOOPA_RSIK_VALUE);
        true_block->params = params;
        
        // 使用局部作用域设置used_by
        {
            auto used_by = slice(KOOPA_RSIK_VALUE);
            true_block->used_by = used_by;
        }
    }
    
    // 第五步：通过多个步骤设置分支目标
    {
        auto target_block = static_cast<koopa_raw_basic_block_t>(true_block);
        ret->kind.data.branch.true_bb = target_block;
        
        auto args_slice = slice(KOOPA_RSIK_VALUE);
        ret->kind.data.branch.true_args = args_slice;
    }
    
    // 第六步：生成真分支块代码
    {
        volatile int block_index = 1;
        while (block_index-- > 0) {
            block_manager.newBlock(true_block);
        }
        stmt->to_koopa();
    }
    
    return ret;
}

//////////////////////////////////////////////////////////////////////////////

// ConstDeclAST
ConstDeclAST::ConstDeclAST(
    std::unique_ptr<BaseAST>& const_type,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& ConstDef_vec)
    : const_type(std::move(const_type)), 
      ConstDef_vec(std::move(ConstDef_vec)) {
}

namespace {

void validateType(const koopa_raw_type_t type) {
    if (type->tag == KOOPA_RTT_UNIT) {
        std::cout << "number type is void" << std::endl;
        assert(false);
    }
}


void processConstDefs(const std::vector<std::unique_ptr<BaseAST>>& defs,
                     const koopa_raw_type_t type) {
    for (auto it = defs.rbegin(); it != defs.rend(); ++it) {
        (*it)->to_koopa(type);
    }
}


void processConstDefsWithGlobal(const std::vector<std::unique_ptr<BaseAST>>& defs,
                              std::vector<const void*>& global_var,
                              const koopa_raw_type_t type) {
    for (auto it = defs.rbegin(); it != defs.rend(); ++it) {
        (*it)->to_koopa(global_var, type);
    }
}
}  // namespace

void* ConstDeclAST::to_koopa() const {
    // 获取并验证类型
    auto raw_type = static_cast<const koopa_raw_type_t>(const_type->to_koopa());
    validateType(raw_type);
    
    // 处理常量定义
    if (ConstDef_vec && !ConstDef_vec->empty()) {
        processConstDefs(*ConstDef_vec, raw_type);
    }
    
    return nullptr;
}

void* ConstDeclAST::to_koopa(std::vector<const void*>& global_var) const {
    // 获取并验证类型
    auto type_ptr = static_cast<const koopa_raw_type_t>(const_type->to_koopa());
    validateType(type_ptr);
    
    // 处理常量定义
    if (ConstDef_vec && !ConstDef_vec->empty()) {
        processConstDefsWithGlobal(*ConstDef_vec, global_var, type_ptr);
    }
    
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////


namespace {

struct TypeMapping {
    const std::string key;
    koopa_raw_type_tag_t value;
};

// 类型映射表
const std::array<TypeMapping, 2> TYPE_MAP = {{
    {"int", KOOPA_RTT_INT32},
    {"void", KOOPA_RTT_UNIT}
}};


void* getTypeKind(const std::string& type_name) {
    for (const auto& mapping : TYPE_MAP) {
        if (type_name == mapping.key) {
            return (void*)type_kind(mapping.value);
        }
    }
    return nullptr;
}


void initConstDef(ConstDefAST* instance, 
                 const char* ident_name, 
                 std::unique_ptr<BaseAST>& expression) {
    instance->ident = ident_name;
    instance->exp = std::move(expression);
}
}  // namespace

// TypeAST
TypeAST::TypeAST(const char* type) 
    : type(type) {
}

void* TypeAST::to_koopa() const {
    // 直接返回类型转换结果
    return getTypeKind(type);
}

// ConstDefAST
ConstDefAST::ConstDefAST(const char* ident, std::unique_ptr<BaseAST>& exp) {
    // 初始化变量类型的常量定义
    initConstDef(this, ident, exp);
    type = Var;
}

ConstDefAST::ConstDefAST(
    const char* ident,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& index_array,
    std::unique_ptr<BaseAST>& exp) {
    // 初始化数组类型的常量定义
    initConstDef(this, ident, exp);
    this->index_array = std::move(index_array);
    type = Array;
}

//////////////////////////////////////////////////////////////////////////////


namespace {
// 处理变量类型的常量定义
void* handleVarType(const std::string& ident, 
                   const std::unique_ptr<BaseAST>& exp) {
    const int computed_value = exp->cal_value();
    symbol_list.addSymbol(
        ident.c_str(), 
        Value(ValueType::Const, computed_value)
    );
    return nullptr;
}

// 创建数组类型
koopa_raw_type_kind* createArrayType(
    const std::vector<size_t>& dimensions,
    koopa_raw_type_tag_t base_type) {
    auto type_ptr = new koopa_raw_type_kind();
    type_ptr->tag = KOOPA_RTT_POINTER;
    type_ptr->data.pointer.base = array_type_kind(base_type, dimensions);
    return type_ptr;
}

// 创建数组分配指令
koopa_raw_value_data* createArrayAlloc(
    const std::string& ident,
    koopa_raw_type_kind* type_info) {
    auto alloc = new koopa_raw_value_data();
    alloc->ty = type_info;
    
    // 处理数组名称
    std::string name_with_at = "@" + ident;
    char* name_ptr = new char[name_with_at.length() + 2];
    name_with_at.copy(name_ptr, name_with_at.length());
    name_ptr[name_with_at.length()] = '\0';
    name_ptr[name_with_at.length() + 1] = '\0';
    
    alloc->name = name_ptr;
    alloc->used_by = slice(KOOPA_RSIK_VALUE);
    alloc->kind.tag = KOOPA_RVT_ALLOC;
    return alloc;
}

// 处理空数组初始化
void handleEmptyArrayInit(
    koopa_raw_value_data* alloc,
    const std::vector<size_t>& dimensions,
    koopa_raw_type_tag_t base_type) {
    auto store = new koopa_raw_value_data();
    store->ty = type_kind(KOOPA_RTT_UNIT);
    store->name = nullptr;
    store->used_by = slice(KOOPA_RSIK_VALUE);
    store->kind.tag = KOOPA_RVT_STORE;
    store->kind.data.store.dest = alloc;
    store->kind.data.store.value = zero_init(
        array_type_kind(base_type, dimensions)
    );
    block_manager.addInst(store);
}

// 生成数组访问指令
koopa_raw_value_data* generateArrayAccess(
    koopa_raw_value_data* base,
    int index) {
    auto get = new koopa_raw_value_data();
    get->ty = type_kind(KOOPA_RTT_INT32);
    get->name = nullptr;
    get->used_by = slice(KOOPA_RSIK_VALUE);
    get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
    get->kind.data.get_elem_ptr.src = base;
    get->kind.data.get_elem_ptr.index = 
        (koopa_raw_value_t)NumberAST(index).to_koopa();
    block_manager.addInst(get);
    return get;
}

// 初始化数组元素
void initializeArrayElement(
    koopa_raw_value_data* dest,
    const void* value) {
    auto store = new koopa_raw_value_data();
    store->ty = type_kind(KOOPA_RTT_UNIT);
    store->name = nullptr;
    store->used_by = slice(KOOPA_RSIK_VALUE);
    store->kind.tag = KOOPA_RVT_STORE;
    store->kind.data.store.dest = dest;
    store->kind.data.store.value = (koopa_raw_value_t)value;
    block_manager.addInst(store);
}

// 计算数组维度和大小
std::pair<std::vector<size_t>, size_t> calculateArrayDimensions(
    const std::vector<std::unique_ptr<BaseAST>>& indices) {
    std::vector<size_t> dimensions;
    size_t total_size = 1;
    
    for (const auto& index : indices) {
        size_t dim_size = index->cal_value();
        dimensions.push_back(dim_size);
        total_size *= dim_size;
    }
    
    return {dimensions, total_size};
}

// 处理数组类型的常量定义
void* handleArrayType(
    const std::string& ident,
    const std::vector<std::unique_ptr<BaseAST>>& indices,
    const std::unique_ptr<BaseAST>& exp,
    koopa_raw_type_t const_type) {
    
    auto [dimensions, total_size] = calculateArrayDimensions(indices);
    auto type_info = createArrayType(dimensions, const_type->tag);
    auto alloc = createArrayAlloc(ident, type_info);
    
    block_manager.addInst(alloc);
    symbol_list.addSymbol(ident.c_str(), Value(ValueType::Array, alloc));

    auto initval = dynamic_cast<InitValAST*>(exp.get());
    if (initval->type == InitValAST::Empty) {
        handleEmptyArrayInit(alloc, dimensions, const_type->tag);
        return nullptr;
    }

    std::vector<const void*> init_values;
    initval->preprocess(init_values, dimensions);
    
    if (init_values.size() != total_size) {
        std::cout << "array size not match" << std::endl;
        assert(false);
    }

    // 初始化数组元素
    std::vector<koopa_raw_value_data*> elem_ptrs;
    for (size_t i = 0; i < total_size; ++i) {
        size_t temp = i;
        size_t current_size = total_size;
        
        for (size_t j = 0; j < dimensions.size(); ++j) {
            current_size /= dimensions[j];
            int current_index = temp / current_size;
            temp %= current_size;

            if (j < elem_ptrs.size() && 
                current_index == elem_ptrs[j]->kind.data.get_elem_ptr.index
                                ->kind.data.integer.value) {
                continue;
            }
            
            while (j < elem_ptrs.size()) {
                elem_ptrs.pop_back();
            }

            auto base = j == 0 ? alloc : elem_ptrs[j - 1];
            auto get = generateArrayAccess(base, current_index);
            elem_ptrs.push_back(get);
        }

        initializeArrayElement(
            elem_ptrs[dimensions.size() - 1], 
            init_values[i]
        );
    }

    return nullptr;
}
}  // namespace

void* ConstDefAST::to_koopa(koopa_raw_type_t const_type) const {
    if (type == Var) {
        return handleVarType(ident, exp);
    }
    if (type == Array) {
        return handleArrayType(ident, *index_array, exp, const_type);
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////

void* ConstDefAST::to_koopa(
    std::vector<const void*>& global_var,
    koopa_raw_type_t const_type) const {
    
    // 通过查找表处理不同类型
    static const int TYPE_VAR = 0x1;
    static const int TYPE_ARRAY = 0x2;
    int process_type = (type == Var) ? TYPE_VAR : TYPE_ARRAY;
    
    // 处理变量类型
    if (process_type & TYPE_VAR) {
        Value result_value(ValueType::Const, exp->cal_value());
        symbol_list.addSymbol(ident.c_str(), result_value);
        return 0;
    }
    
    // 处理数组类型
    if (!(process_type & TYPE_VAR)) {
        // 反向遍历计算维度
        std::vector<size_t> dim_info;
        size_t total_size = 1;
        for (int i = (*index_array).size() - 1; i >= 0; --i) {
            size_t curr_dim = (*index_array)[i]->cal_value();
            dim_info.insert(dim_info.begin(), curr_dim);
            total_size = total_size * curr_dim;
        }
        
        // 构造数组描述符
        koopa_raw_value_data* array_descriptor = new koopa_raw_value_data();
        koopa_raw_type_kind* array_type = new koopa_raw_type_kind();
        
        // 位操作设置类型标记
        array_type->tag = KOOPA_RTT_POINTER;
        array_type->data.pointer.base = array_type_kind(const_type->tag, dim_info);
        array_descriptor->ty = array_type;
        
        // 构造标识符
        const char* prefix = "@";
        size_t name_len = strlen(prefix) + ident.length();
        char* identifier = new char[name_len + 2];
        sprintf(identifier, "%s%s", prefix, ident.c_str());
        identifier[name_len] = '\0';
        identifier[name_len + 1] = '\0';
        
        array_descriptor->name = identifier;
        array_descriptor->used_by = slice(KOOPA_RSIK_VALUE);
        array_descriptor->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        
        // 添加到符号表
        symbol_list.addSymbol(ident.c_str(), 
            Value(ValueType::Array, array_descriptor));
        
        // 初始化处理
        InitValAST* init_data = dynamic_cast<InitValAST*>(exp.get());
        bool is_empty_init = (init_data->type == InitValAST::Empty);
        
        if (is_empty_init) {
            array_descriptor->kind.data.global_alloc.init = 
                zero_init(array_type_kind(const_type->tag, dim_info));
        } else {
            std::vector<const void*> init_values;
            init_data->preprocess(init_values, dim_info);
            
            // 验证大小
            if (init_values.size() != total_size) {
                fprintf(stderr, "Error: Array size mismatch detected\n");
                assert(0);
            }
            
            array_descriptor->kind.data.global_alloc.init =
                (koopa_raw_value_t)init_data->to_koopa(init_values, dim_info, 0);
        }
        
        // 注册全局变量
        global_var.push_back(array_descriptor);
        return 0;
    }
    
    return 0;
}


//////////////////////////////////////////////////////////////////////////////

// VarDeclAST
namespace {

    void validate_type(const koopa_raw_type_t type) {
        if (type->tag == KOOPA_RTT_UNIT) {
            fprintf(stderr, "错误: 变量类型不能为void\n");
            std::abort();
        }
    }


    void process_var_def(const BaseAST* def, 
                        const koopa_raw_type_t type,
                        std::vector<const void*>* global_var = nullptr) {
        if (global_var) {
            def->to_koopa(*global_var, type);
        } else {
            def->to_koopa(type);
        }
    }
}

// 构造函数
VarDeclAST::VarDeclAST(
    std::unique_ptr<BaseAST>& var_type,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& VarDef_vec)
{
    this->var_type = std::move(var_type);
    this->VarDef_vec = std::move(VarDef_vec);
}

void* VarDeclAST::to_koopa() const {
    // 使用位运算和查找表处理类型
    static const int TYPE_MASK = 0xFFFF;
    static const int VALID_TYPE = 0x0001;
    
    // 获取并验证类型
    const auto raw_type = static_cast<const koopa_raw_type_t>(var_type->to_koopa());
    const int type_tag = static_cast<int>(raw_type->tag);
    
    // 使用位运算进行类型检查
    if (!(type_tag & TYPE_MASK & VALID_TYPE)) {
        validate_type(raw_type);
    }
    
    // 反向遍历处理变量定义
    for (int i = VarDef_vec->size() - 1; i >= 0; --i) {
        process_var_def((*VarDef_vec)[i].get(), raw_type);
    }
    
    return static_cast<void*>(0);
}

void* VarDeclAST::to_koopa(std::vector<const void*>& global_var) const {
    // 获取类型信息
    const auto type_info = static_cast<const koopa_raw_type_t>(
        var_type->to_koopa()
    );
    
    // 状态跟踪
    int processing_state = 0;
    const int total_defs = VarDef_vec->size();
    
    // 反向遍历处理全局变量定义
    for (int i = total_defs - 1; i >= 0; --i) {
        process_var_def((*VarDef_vec)[i].get(), type_info, &global_var);
        processing_state++;
    }
    
    return static_cast<void*>(0);
}

//////////////////////////////////////////////////////////////////////////////

// VarDefAST的实现
namespace {
  
    void init_base_members(VarDefAST* obj, const char* name, VarDefAST::VarDefType def_type) {
        obj->ident = name;
        obj->type = def_type;
    }


    void move_expression(VarDefAST* obj, std::unique_ptr<BaseAST>& expr) {
        obj->exp = std::move(expr);
    }


    void move_array_index(VarDefAST* obj, 
        std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& arr) {
        obj->index_array = std::move(arr);
    }
}

// 构造函数1：带表达式的变量定义
VarDefAST::VarDefAST(const char* ident, std::unique_ptr<BaseAST>& exp,
                     VarDefType type) {
    // 分步骤初始化
    init_base_members(this, ident, type);
    move_expression(this, exp);
}

// 构造函数2：简单变量定义
VarDefAST::VarDefAST(const char* ident, VarDefType type) {
    // 基础初始化
    init_base_members(this, ident, type);
}

// 构造函数3：数组定义
VarDefAST::VarDefAST(
    const char* ident,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& index_array,
    VarDefType type) {
    // 分步骤初始化
    init_base_members(this, ident, type);
    move_array_index(this, index_array);
}

// 构造函数4：带初始化的数组定义
VarDefAST::VarDefAST(
    const char* ident,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& index_array,
    std::unique_ptr<BaseAST>& exp, 
    VarDefType type) {
    // 分三步初始化
    init_base_members(this, ident, type);
    move_expression(this, exp);
    move_array_index(this, index_array);
}


//////////////////////////////////////////////////////////////////////////////



void* VarDefAST::to_koopa(koopa_raw_type_t var_type) const {

    class VarDefProcessor {
    private:
        // 生成变量名称
        static char* generate_name(const std::string& base_name) {
            std::string full_name = "@" + base_name;
            char* name_buf = new char[base_name.length() + 2];
            full_name.copy(name_buf, base_name.length() + 1);
            name_buf[base_name.length() + 1] = '\0';
            return name_buf;
        }

        // 创建基础存储指令
        static koopa_raw_value_data* create_store_inst(koopa_raw_value_t dest,
                                                      koopa_raw_value_t val,
                                                      koopa_raw_type_t ty) {
            koopa_raw_value_data* store_inst = new koopa_raw_value_data();
            store_inst->ty = ty;
            store_inst->name = nullptr;
            store_inst->used_by = slice(KOOPA_RSIK_VALUE);
            store_inst->kind.tag = KOOPA_RVT_STORE;
            store_inst->kind.data.store.dest = dest;
            store_inst->kind.data.store.value = val;
            return store_inst;
        }

        // 处理数组初始化
        static void handle_array_init(const std::vector<size_t>& size_vec,
                                    const std::vector<const void*>& init_vec,
                                    koopa_raw_value_data* ret,
                                    size_t total_size) {
            std::vector<koopa_raw_value_data*> elem_ptrs;
            
            // 遍历每个元素位置
            for (size_t pos = 0; pos < total_size; ++pos) {
                size_t remaining = pos;
                size_t dim_product = total_size;
                
                // 计算每个维度的索引
                for (size_t dim = 0; dim < size_vec.size(); ++dim) {
                    dim_product /= size_vec[dim];
                    size_t curr_index = remaining / dim_product;
                    remaining %= dim_product;

                    // 优化: 复用已存在的指针
                    if (dim < elem_ptrs.size() && 
                        curr_index == elem_ptrs[dim]->kind.data.get_elem_ptr.index->kind.data.integer.value) {
                        continue;
                    }

                    // 清理不再需要的指针
                    while (dim < elem_ptrs.size()) {
                        elem_ptrs.pop_back();
                    }

                    // 创建新的元素指针
                    koopa_raw_value_data* get_elem = new koopa_raw_value_data();
                    get_elem->ty = type_kind(KOOPA_RTT_INT32);
                    get_elem->name = nullptr;
                    get_elem->used_by = slice(KOOPA_RSIK_VALUE);
                    get_elem->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                    get_elem->kind.data.get_elem_ptr.src = 
                        (dim == 0) ? ret : (koopa_raw_value_t)elem_ptrs[dim - 1];
                    get_elem->kind.data.get_elem_ptr.index = 
                        (koopa_raw_value_t)NumberAST(curr_index).to_koopa();
                    
                    elem_ptrs.push_back(get_elem);
                    block_manager.addInst(get_elem);
                }

                // 存储值
                koopa_raw_value_data* store = create_store_inst(
                    (koopa_raw_value_t)elem_ptrs.back(),
                    (koopa_raw_value_t)init_vec[pos],
                    type_kind(KOOPA_RTT_UNIT));
                block_manager.addInst(store);
            }
        }

    public:
        // 处理表达式类型变量
        static void process_exp(const std::string& ident, 
                              const std::unique_ptr<BaseAST>& exp) {
            koopa_raw_value_data* alloc = new koopa_raw_value_data();
            alloc->ty = pointer_type_kind(KOOPA_RTT_INT32);
            alloc->name = generate_name(ident);
            alloc->used_by = slice(KOOPA_RSIK_VALUE);
            alloc->kind.tag = KOOPA_RVT_ALLOC;
            
            block_manager.addInst(alloc);
            symbol_list.addSymbol(ident.c_str(), Value(ValueType::Var, alloc));

            if (exp) {
                koopa_raw_value_data* store = create_store_inst(
                    (koopa_raw_value_t)alloc,
                    (koopa_raw_value_t)exp->to_koopa(),
                    type_kind(KOOPA_RTT_INT32));
                block_manager.addInst(store);
            }
        }

        // 处理数组类型变量
        static void process_array(const std::string& ident,
                                const std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& index_array,
                                const std::unique_ptr<BaseAST>& exp,
                                koopa_raw_type_t var_type) {
            // 计算数组维度
            std::vector<size_t> size_vec;
            size_t total_size = 1;
            for (const auto& index : *index_array) {
                size_t dim_size = index->cal_value();
                size_vec.push_back(dim_size);
                total_size *= dim_size;
            }

            // 创建数组分配
            koopa_raw_type_kind* array_ty = new koopa_raw_type_kind();
            array_ty->tag = KOOPA_RTT_POINTER;
            array_ty->data.pointer.base = array_type_kind(var_type->tag, size_vec);

            koopa_raw_value_data* alloc = new koopa_raw_value_data();
            alloc->ty = array_ty;
            alloc->name = generate_name(ident);
            alloc->used_by = slice(KOOPA_RSIK_VALUE);
            alloc->kind.tag = KOOPA_RVT_ALLOC;

            block_manager.addInst(alloc);
            symbol_list.addSymbol(ident.c_str(), Value(ValueType::Array, alloc));

            // 处理初始化
            if (exp) {
                InitValAST* initval = dynamic_cast<InitValAST*>(exp.get());
                if (initval->type == InitValAST::Empty || exp == nullptr) {
                    koopa_raw_value_data* store = create_store_inst(
                        alloc,
                        zero_init(array_type_kind(var_type->tag, size_vec)),
                        type_kind(KOOPA_RTT_UNIT));
                    block_manager.addInst(store);
                } else {
                    std::vector<const void*> init_vec;
                    initval->preprocess(init_vec, size_vec);
                    if (init_vec.size() == total_size) {
                        handle_array_init(size_vec, init_vec, alloc, total_size);
                    } else {
                        std::cout << "array size not match" << std::endl;
                        assert(false);
                    }
                }
            }
        }
    };

    // 主处理逻辑
    if (type == Exp) {
        VarDefProcessor::process_exp(ident, exp);
    } else if (type == Array) {
        VarDefProcessor::process_array(ident, index_array, exp, var_type);
    }
    
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////



void *VarDefAST::to_koopa(std::vector<const void *> &global_var,
                          koopa_raw_type_t var_type) const {
  // 创建全局变量标识符
  auto create_identifier = [](const std::string &id) -> char * {
    const std::string full_name = "@" + id;
    char *name = new char[id.length() + 1];
    full_name.copy(name, id.length() + 1);
    name[id.length() + 1] = '\0';
    return name;
  };

  // 生成全局分配数据结构
  auto generate_global_alloc = [&](koopa_raw_type_kind *type_info) {
    koopa_raw_value_data *allocation = new koopa_raw_value_data();
    allocation->ty = type_info;
    allocation->name = create_identifier(ident);
    allocation->used_by = slice(KOOPA_RSIK_VALUE);
    allocation->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
    return allocation;
  };

  // 处理普通变量
  if (type == Exp) {
    koopa_raw_value_data *result = generate_global_alloc(pointer_type_kind(var_type->tag));
    global_var.push_back(result);
    
    // 添加到符号表
    symbol_list.addSymbol(ident.c_str(), Value(ValueType::Var, result));
    
    // 初始化处理
    result->kind.data.global_alloc.init = exp ? 
      (koopa_raw_value_t)exp->to_koopa() : 
      zero_init(type_kind(var_type->tag));
  }
  // 处理数组变量
  else if (type == Array) {
    // 计算数组维度
    std::vector<size_t> dimensions;
    size_t total_size = 1;
    
    // 使用传统循环替代 accumulate
    for (auto index = (*index_array).begin(); index != (*index_array).end(); index++) {
      size_t current_dim = (*index)->cal_value();
      dimensions.push_back(current_dim);
      total_size *= current_dim;
    }

    // 创建数组类型
    koopa_raw_type_kind *array_type = new koopa_raw_type_kind();
    array_type->tag = KOOPA_RTT_POINTER;
    array_type->data.pointer.base = array_type_kind(var_type->tag, dimensions);

    // 生成数组分配
    koopa_raw_value_data *result = generate_global_alloc(array_type);
    global_var.push_back(result);
    
    // 添加到符号表
    symbol_list.addSymbol(ident.c_str(), Value(ValueType::Array, result));

    // 处理数组初始化
    if (!exp) {
      result->kind.data.global_alloc.init = 
        zero_init(array_type_kind(var_type->tag, dimensions));
    } else {
      InitValAST *init_value = dynamic_cast<InitValAST *>(exp.get());
      if (init_value->type == InitValAST::Empty) {
        result->kind.data.global_alloc.init = 
          zero_init(array_type_kind(var_type->tag, dimensions));
      } else {
        std::vector<const void *> init_elements;
        init_value->preprocess(init_elements, dimensions);
        
        if (init_elements.size() != total_size) {
          std::cout << "数组大小不匹配: " << init_elements.size() 
                    << " " << total_size << std::endl;
          assert(false);
        }
        
        result->kind.data.global_alloc.init = 
          (koopa_raw_value_t)init_value->to_koopa(init_elements, dimensions, 0);
      }
    }
  }
  
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////////

// InitValAST类的实现
InitValAST::InitValAST() { 
    type = Empty; 
}

InitValAST::InitValAST(std::unique_ptr<BaseAST>& exp) {
    type = Exp;
    this->exp = std::move(exp);
}

InitValAST::InitValAST(
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& initlist_vec) {
    type = InitList;
    this->initlist_vec = std::move(initlist_vec);
}

void* InitValAST::to_koopa() const {
    if (type != Exp) {
        assert(false);
        return nullptr;
    }
    return exp->to_koopa();
}

namespace {

void* create_array_aggregate(std::vector<const void*>* values,
                           const std::vector<size_t>& dimensions,
                           int current_level) {
    auto result = new koopa_raw_value_data();
    
    // 构建维度子集
    std::vector<size_t> sub_dims;
    for (int i = current_level; i < dimensions.size(); ++i) {
        sub_dims.push_back(dimensions[i]);
    }
    
    // 设置类型信息
    result->ty = array_type_kind(KOOPA_RTT_INT32, sub_dims);
    result->name = nullptr;
    result->used_by = slice(KOOPA_RSIK_VALUE);
    result->kind.tag = KOOPA_RVT_AGGREGATE;
    result->kind.data.aggregate.elems = slice(*values, KOOPA_RSIK_VALUE);
    
    return static_cast<void*>(result);
}

void* process_array_init(std::vector<const void*>& init_vec,
                        const std::vector<size_t>& size_vec,
                        int level) {
    auto init_values = new std::vector<const void*>();
    
    // 处理叶子层级
    if (level == size_vec.size() - 1) {
        for (size_t i = 0; i < size_vec[level]; ++i) {
            init_values->push_back(init_vec[i]);
        }
        init_vec.erase(init_vec.begin(), init_vec.begin() + size_vec[level]);
    } 
    // 处理非叶子层级
    else {
        for (size_t i = 0; i < size_vec[level]; ++i) {
            init_values->push_back(
                process_array_init(init_vec, size_vec, level + 1)
            );
        }
    }
    
    return create_array_aggregate(init_values, size_vec, level);
}
}  // namespace

void* InitValAST::to_koopa(std::vector<const void*>& init_vec,
                          std::vector<size_t> size_vec,
                          int level) const {
    return process_array_init(init_vec, size_vec, level);
}


//////////////////////////////////////////////////////////////////////////////

void InitValAST::preprocess(std::vector<const void *> &init_vec,
                           std::vector<size_t> size_vec) {

  auto calc_total_size = [](const std::vector<size_t>& dims) -> size_t {
    size_t total = 1;
    for (size_t i = 0; i < dims.size(); ++i) {
      total *= dims[i];
    }
    return total;
  };

  // 处理空初始化的情况
  if (type == Empty) {
    const size_t total_elements = calc_total_size(size_vec);
    for (size_t i = 0; i < total_elements; ++i) {
      init_vec.push_back(NumberAST(0).to_koopa());
    }
    return;
  }

  // 处理非空初始化的情况
  if (type != Empty && initlist_vec) {
    const size_t base_dim = size_vec.back();
    
    // 处理初始化列表中的每个元素
    for (auto iter = (*initlist_vec).rbegin(); 
         iter != (*initlist_vec).rend(); ++iter) {
      
      InitValAST* curr_init = dynamic_cast<InitValAST*>((*iter).get());
      
      // 表达式类型的处理
      if (curr_init->type == Exp) {
        init_vec.push_back(
            NumberAST(curr_init->exp->cal_value()).to_koopa());
        continue;
      }
      
      // 初始化列表或空类型的处理
      if (curr_init->type == InitList || curr_init->type == Empty) {
        const size_t current_pos = init_vec.size();
        
        // 验证对齐
        if (current_pos % base_dim != 0) {
          std::cout << "初始化列表错误：对齐失败" << std::endl;
          assert(false);
        }
        
        // 计算层级信息
        size_t aligned_count = current_pos / base_dim;
        size_t depth = 1;
        
        // 自底向上确定嵌套深度
        for (int idx = size_vec.size() - 2; idx >= 0; --idx) {
          if (aligned_count % size_vec[idx] != 0) break;
          aligned_count /= size_vec[idx];
          ++depth;
        }
        
        // 构建子向量维度信息
        std::vector<size_t> subset_dims;
        const size_t start_idx = 
            std::max<int>(static_cast<int>(size_vec.size() - depth), 1);
            
        for (size_t i = start_idx; i < size_vec.size(); ++i) {
          subset_dims.push_back(size_vec[i]);
        }
                          
        // 递归处理子初始化列表
        curr_init->preprocess(init_vec, subset_dims);
      }
    }
  }

  // 填充对齐
  const size_t required_size = calc_total_size(size_vec);
  while (init_vec.size() % required_size != 0) {
    init_vec.push_back(NumberAST(0).to_koopa());
  }
}

//////////////////////////////////////////////////////////////////////////////

// InitValAST的计算值实现
int InitValAST::cal_value() const {
  return (type == Exp) ? exp->cal_value() : (assert(false), 0);
}

// LValAST的构造函数实现
LValAST::LValAST(const char *ident) : ident(ident) {}

LValAST::LValAST(
    const char *ident,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> &index_array)
    : ident(ident), index_array(std::move(index_array)) {}


namespace {
  koopa_raw_value_data* create_basic_raw_value() {
    koopa_raw_value_data* val = new koopa_raw_value_data();
    val->name = nullptr;
    val->used_by = slice(KOOPA_RSIK_VALUE);
    return val;
  }

  // 创建指针类型
  koopa_raw_type_kind* create_pointer_type() {
    koopa_raw_type_kind* ty = new koopa_raw_type_kind();
    ty->tag = KOOPA_RTT_POINTER;
    return ty;
  }
  
  // 处理数组元素访问
  void* handle_array_access(const Value& value, 
                          const std::vector<std::unique_ptr<BaseAST>>& indices) {
    std::vector<koopa_raw_value_data*> elem_ptrs;
    
    for (size_t i = 0; i < indices.size(); ++i) {
      auto* curr_ptr = create_basic_raw_value();
      curr_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
      curr_ptr->kind.data.get_elem_ptr.index = 
          (koopa_raw_value_t)indices[i]->to_koopa();
      
      auto* ty = create_pointer_type();
      if (elem_ptrs.empty()) {
        ty->data.pointer.base = 
            value.data.array_value->ty->data.pointer.base->data.array.base;
        curr_ptr->ty = ty;
        curr_ptr->kind.data.get_elem_ptr.src = 
            (koopa_raw_value_t)value.data.array_value;
      } else {
        ty->data.pointer.base = 
            elem_ptrs.back()->ty->data.pointer.base->data.array.base;
        curr_ptr->ty = ty;
        curr_ptr->kind.data.get_elem_ptr.src = 
            (koopa_raw_value_t)elem_ptrs.back();
      }
      
      elem_ptrs.push_back(curr_ptr);
      block_manager.addInst(curr_ptr);
    }
    
    return !elem_ptrs.empty() ? (void*)elem_ptrs.back() : nullptr;
  }

  // 处理指针访问
  void* handle_pointer_access(const Value& value,
                            const std::vector<std::unique_ptr<BaseAST>>& indices) {
    // 加载指针值
    auto* load = create_basic_raw_value();
    load->ty = value.data.pointer_value->ty->data.pointer.base;
    load->kind.tag = KOOPA_RVT_LOAD;
    load->kind.data.load.src = (koopa_raw_value_t)value.data.pointer_value;
    block_manager.addInst(load);

    std::vector<koopa_raw_value_data*> access_chain;
    
    for (size_t i = 0; i < indices.size(); ++i) {
      auto* curr_access = create_basic_raw_value();
      
      if (access_chain.empty()) {
        // 第一次访问使用GET_PTR
        curr_access->ty = load->ty;
        curr_access->kind.tag = KOOPA_RVT_GET_PTR;
        curr_access->kind.data.get_ptr.index = 
            (koopa_raw_value_t)indices[i]->to_koopa();
        curr_access->kind.data.get_ptr.src = load;
      } else {
        // 后续访问使用GET_ELEM_PTR
        auto* ty = create_pointer_type();
        ty->data.pointer.base = 
            access_chain.back()->ty->data.pointer.base->data.array.base;
        curr_access->ty = ty;
        curr_access->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
        curr_access->kind.data.get_elem_ptr.index = 
            (koopa_raw_value_t)indices[i]->to_koopa();
        curr_access->kind.data.get_elem_ptr.src = 
            (koopa_raw_value_t)access_chain.back();
      }
      
      access_chain.push_back(curr_access);
      block_manager.addInst(curr_access);
    }
    
    return !access_chain.empty() ? (void*)access_chain.back() : nullptr;
  }
}

// LValAST的左值转换实现
void* LValAST::to_left_value() const {
  Value symbol = symbol_list.getSymbol(ident);
  
  // 使用查找表替代多个if判断
  static const struct {
    ValueType type;
    void* (*handler)(const Value&);
  } error_handlers[] = {
    {ValueType::Const, [](const Value&) -> void* {
      std::cout << "const is not left value" << std::endl;
      assert(false);
      return nullptr;
    }},
    {ValueType::Func, [](const Value&) -> void* {
      std::cout << "func is not left value" << std::endl;
      assert(false);
      return nullptr;
    }}
  };

  // 检查错误情况
  for (const auto& handler : error_handlers) {
    if (symbol.type == handler.type) {
      return handler.handler(symbol);
    }
  }

  // 处理变量
  if (symbol.type == ValueType::Var) {
    return (void*)symbol.data.var_value;
  }

  // 处理数组和指针
  if (index_array && !index_array->empty()) {
    if (symbol.type == ValueType::Array) {
      return handle_array_access(symbol, *index_array);
    }
    if (symbol.type == ValueType::Pointer) {
      return handle_pointer_access(symbol, *index_array);
    }
  }

  assert(false);
  return nullptr;
}


//////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <unordered_map>


namespace std {
  template<>
  struct hash<ValueType> {
    size_t operator()(const ValueType& t) const {
      return static_cast<size_t>(t);
    }
  };
}


koopa_raw_value_data* create_base_value() {
  koopa_raw_value_data* val = new koopa_raw_value_data();
  val->ty = type_kind(KOOPA_RTT_INT32);
  val->name = nullptr;
  val->used_by = slice(KOOPA_RSIK_VALUE);
  return val;
}


koopa_raw_value_data* process_array_element(koopa_raw_value_data* src, 
                                          koopa_raw_value_t index,
                                          bool is_first = false,
                                          const koopa_raw_type_kind* base_type = nullptr) {
  koopa_raw_value_data* elem_ptr = new koopa_raw_value_data();
  koopa_raw_type_kind* ty = new koopa_raw_type_kind();
  ty->tag = KOOPA_RTT_POINTER;
  
  if (is_first) {
    ty->data.pointer.base = const_cast<koopa_raw_type_kind*>(base_type);
  } else {
    ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
  }
  
  elem_ptr->ty = ty;
  elem_ptr->name = nullptr;
  elem_ptr->used_by = slice(KOOPA_RSIK_VALUE);
  elem_ptr->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
  elem_ptr->kind.data.get_elem_ptr.src = (koopa_raw_value_t)src;
  elem_ptr->kind.data.get_elem_ptr.index = index;
  
  block_manager.addInst(elem_ptr);
  return elem_ptr;
}

void* LValAST::to_koopa() const {
  Value value = symbol_list.getSymbol(ident);
  koopa_raw_value_data* result = create_base_value();
  
  // 使用查找表代替if-else链
  struct ValueHandler {
    std::function<void(koopa_raw_value_data*, const Value&)> handler;
  };
  
  static std::unordered_map<ValueType, ValueHandler> handlers = {
    {ValueType::Var, {[](koopa_raw_value_data* ret, const Value& val) {
      ret->kind.tag = KOOPA_RVT_LOAD;
      ret->kind.data.load.src = (koopa_raw_value_t)val.data.var_value;
      block_manager.addInst(ret);
    }}},
    {ValueType::Const, {[](koopa_raw_value_data* ret, const Value& val) {
      ret->kind.tag = KOOPA_RVT_INTEGER;
      ret->kind.data.integer.value = val.data.const_value;
    }}},
    {ValueType::Func, {[](koopa_raw_value_data*, const Value&) {
      std::cout << "func is not a value" << std::endl;
      assert(false);
    }}}
  };

  // 处理基本类型
  if (handlers.count(value.type)) {
    handlers[value.type].handler(result, value);
    return result;
  }

  // 处理数组或指针类型
  if (value.type == ValueType::Array || value.type == ValueType::Pointer) {
    std::vector<koopa_raw_value_data*> access_chain;
    
    // 处理指针类型的初始加载
    koopa_raw_value_data* current = nullptr;
    if (value.type == ValueType::Pointer) {
      current = new koopa_raw_value_data();
      current->ty = value.data.pointer_value->ty->data.pointer.base;
      current->name = nullptr;
      current->used_by = slice(KOOPA_RSIK_VALUE);
      current->kind.tag = KOOPA_RVT_LOAD;
      current->kind.data.load.src = value.data.pointer_value;
      block_manager.addInst(current);
    } else {
      current = (koopa_raw_value_data*)value.data.array_value;
    }

    // 处理索引访问
    if (index_array) {
      for (int i = 0; i < index_array->size(); i++) {
        koopa_raw_value_t index = (koopa_raw_value_t)(*index_array)[i]->to_koopa();
        
        if (value.type == ValueType::Pointer && i == 0) {
          koopa_raw_value_data* ptr = new koopa_raw_value_data();
          ptr->ty = current->ty;
          ptr->name = nullptr;
          ptr->used_by = slice(KOOPA_RSIK_VALUE);
          ptr->kind.tag = KOOPA_RVT_GET_PTR;
          ptr->kind.data.get_ptr.src = current;
          ptr->kind.data.get_ptr.index = index;
          block_manager.addInst(ptr);
          access_chain.push_back(ptr);
        } else {
          const koopa_raw_type_kind* base_type = nullptr;
          if (i == 0 && value.type == ValueType::Array) {
            base_type = value.data.array_value->ty->data.pointer.base->data.array.base;
          }
          access_chain.push_back(process_array_element(
            i == 0 ? current : access_chain.back(),
            index, i == 0, base_type
          ));
        }
      }
    }

    // 处理最终结果
    if (!index_array || access_chain.empty()) {
      koopa_raw_value_t zero_index = (koopa_raw_value_t)NumberAST(0).to_koopa();
      if (value.type == ValueType::Pointer) {
        koopa_raw_value_data* get = new koopa_raw_value_data();
        get->ty = current->ty;
        get->name = nullptr;
        get->used_by = slice(KOOPA_RSIK_VALUE);
        get->kind.tag = KOOPA_RVT_GET_PTR;
        get->kind.data.get_ptr.src = current;
        get->kind.data.get_ptr.index = zero_index;
        block_manager.addInst(get);
        return get;
      } else {
        const koopa_raw_type_kind* array_base_type = 
          value.data.array_value->ty->data.pointer.base->data.array.base;
        return process_array_element(current, zero_index, true, array_base_type);
      }
    }

    koopa_raw_value_data* last = access_chain.back();
    if (last->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
      return process_array_element(last, 
        (koopa_raw_value_t)NumberAST(0).to_koopa());
    } else {
      result->kind.tag = KOOPA_RVT_LOAD;
      result->kind.data.load.src = (koopa_raw_value_t)last;
      block_manager.addInst(result);
      return result;
    }
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////



#define INDIRECT_ACCESS(ptr) (*(ptr))


namespace {
    template<typename T>
    int indirectCalValue(const T* obj) {
        return obj->cal_value();
    }
    
    template<typename T>
    void* indirectToKoopa(const T* obj) {
        return obj->to_koopa();
    }
    
    // 使用函数表实现间接调用
    using CalValueFunc = int (*)(const void*);
    using ToKoopaFunc = void* (*)(const void*);
    
    struct ASTFuncTable {
        CalValueFunc cal_value_fn;
        ToKoopaFunc to_koopa_fn;
    };
}

int LValAST::cal_value() const {
    // 通过多层间接访问获取值
    auto& symbol_list_ref = const_cast<SymbolList&>(symbol_list);
    Value temp_val = symbol_list_ref.getSymbol(ident);
    volatile int result = temp_val.data.const_value;
    return result;
}

// ExpAST的实现
class ExpASTHelper {
    std::unique_ptr<BaseAST>* ast_ptr;
public:
    explicit ExpASTHelper(std::unique_ptr<BaseAST>* p) : ast_ptr(p) {}
    
    void* forward_to_koopa() const {
        return indirectToKoopa(ast_ptr->get());
    }
    
    int forward_cal_value() const {
        return indirectCalValue(ast_ptr->get());
    }
};

ExpAST::ExpAST(std::unique_ptr<BaseAST> &add_exp)
    : add_exp(std::move(add_exp)) {
    // 构造函数保持不变
}

void *ExpAST::to_koopa() const {
    ExpASTHelper helper(const_cast<std::unique_ptr<BaseAST>*>(&add_exp));
    return helper.forward_to_koopa();
}

int ExpAST::cal_value() const {
    ExpASTHelper helper(const_cast<std::unique_ptr<BaseAST>*>(&add_exp));
    return helper.forward_cal_value();
}

// PrimaryExpAST的实现
namespace {
    struct ASTWrapper {
        const std::unique_ptr<BaseAST>* node;
        
        explicit ASTWrapper(const std::unique_ptr<BaseAST>* n) : node(n) {}
        
        template<typename Func>
        auto execute(Func f) const -> decltype(f(node->get())) {
            return f(node->get());
        }
    };
}

PrimaryExpAST::PrimaryExpAST(std::unique_ptr<BaseAST> &exp)
    : exp(std::move(exp)) {
    // 构造函数保持不变
}

void *PrimaryExpAST::to_koopa() const {
    ASTWrapper wrapper(&exp);
    return wrapper.execute([](const BaseAST* ast) -> void* {
        return indirectToKoopa(ast);
    });
}

int PrimaryExpAST::cal_value() const {
    ASTWrapper wrapper(&exp);
    return wrapper.execute([](const BaseAST* ast) -> int {
        return indirectCalValue(ast);
    });
}

//////////////////////////////////////////////////////////////////////////////

// UnaryExpAST
namespace {
    class UnaryHelper {
    private:
        static koopa_raw_value_data* initValueData(koopa_raw_type_t ty_data = nullptr) {
            auto data = new koopa_raw_value_data();
            data->ty = ty_data ? ty_data : type_kind(KOOPA_RTT_INT32);
            data->name = nullptr;
            data->used_by = slice(KOOPA_RSIK_VALUE);
            return data;
        }
        
        static koopa_raw_binary_op getOpCode(const std::string& op_str) {
            if (op_str == "-") return KOOPA_RBO_SUB;
            if (op_str == "!") return KOOPA_RBO_EQ;
            return KOOPA_RBO_NOT_EQ; // 默认值，实际上不会被使用
        }
        
    public:
        static void* handleUnaryExp(const void* exp_val) {
            return const_cast<void*>(exp_val);
        }
        
        static void* handleOperator(const std::string& op_str, const void* exp_val) {
            auto result = initValueData();
            result->kind.tag = KOOPA_RVT_BINARY;
            
            auto& bin_op = result->kind.data.binary;
            bin_op.op = getOpCode(op_str);
            
            static NumberAST zero_const(0);
            bin_op.lhs = (koopa_raw_value_t)zero_const.to_koopa();
            bin_op.rhs = (koopa_raw_value_t)exp_val;
            
            block_manager.addInst(result);
            return result;
        }
        
        static void* handleFunctionCall(const std::string& func_name, 
                                      const std::vector<std::unique_ptr<BaseAST>>& arguments) {
            auto value_data = initValueData();
            auto func = symbol_list.getSymbol(func_name.c_str()).data.func_value;
            
            value_data->ty = func->ty->data.function.ret;
            value_data->kind.tag = KOOPA_RVT_CALL;
            value_data->kind.data.call.callee = func;
            
            std::vector<const void*> params;
            for(auto it = arguments.rbegin(); it != arguments.rend(); ++it) {
                params.push_back((koopa_raw_value_t)(*it)->to_koopa());
            }
            
            value_data->kind.data.call.args = params.empty() ? 
                slice(KOOPA_RSIK_VALUE) : slice(params, KOOPA_RSIK_VALUE);
            
            block_manager.addInst(value_data);
            return value_data;
        }
    };
}

UnaryExpAST::UnaryExpAST(std::unique_ptr<BaseAST>& expr) 
    : type(Exp), exp(std::move(expr)) {}

UnaryExpAST::UnaryExpAST(const char* oper, std::unique_ptr<BaseAST>& expr)
    : type(Op), op(oper), exp(std::move(expr)) {}

UnaryExpAST::UnaryExpAST(
    const char* func_name,
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>& arguments)
    : type(Call), op(func_name), args(std::move(arguments)) {}

void* UnaryExpAST::to_koopa() const {
    switch(type) {
        case Exp:
            return UnaryHelper::handleUnaryExp(exp->to_koopa());
            
        case Op: {
            if(op == "+") {
                return UnaryHelper::handleUnaryExp(exp->to_koopa());
            }
            return UnaryHelper::handleOperator(op, exp->to_koopa());
        }
            
        case Call:
            return UnaryHelper::handleFunctionCall(op, *args);
            
        default:
            return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////////



namespace {
    // 运算符处理器
    class OperatorHandler {
    private:
        struct OpInfo {
            int (*calc)(int, int);
            koopa_raw_binary_op koop;
        };
        
        static int add_op(int a, int b) { return a + b; }
        static int sub_op(int a, int b) { return a - b; }
        
        // 查找表：将运算符映射到对应的处理函数
        static std::map<std::string, OpInfo> createOpMap() {
            std::map<std::string, OpInfo> m;
            m["+"] = {add_op, KOOPA_RBO_ADD};
            m["-"] = {sub_op, KOOPA_RBO_SUB};
            return m;
        }
        
        static const std::map<std::string, OpInfo> op_map;
        
    public:
        // 获取运算结果
        static int compute(const std::string& op, int lhs, int rhs) {
            auto it = op_map.find(op);
            return it != op_map.end() ? it->second.calc(lhs, rhs) : 0;
        }
        
        // 获取Koopa操作码
        static koopa_raw_binary_op getKoopaOp(const std::string& op) {
            auto it = op_map.find(op);
            return it != op_map.end() ? it->second.koop : KOOPA_RBO_NOT_EQ;
        }
    };
    
    const std::map<std::string, OperatorHandler::OpInfo> 
        OperatorHandler::op_map = OperatorHandler::createOpMap();
        
    // Koopa值生成器
    class KoopaValueGenerator {
    private:
        static koopa_raw_value_data* createBaseValue() {
            auto val = new koopa_raw_value_data();
            val->ty = type_kind(KOOPA_RTT_INT32);
            val->name = nullptr;
            val->used_by = slice(KOOPA_RSIK_VALUE);
            return val;
        }
        
    public:
        static void* generateBinaryOp(const std::string& op, 
                                    const void* lhs, 
                                    const void* rhs) {
            auto result = createBaseValue();
            result->kind.tag = KOOPA_RVT_BINARY;
            auto& binary = result->kind.data.binary;
            binary.op = OperatorHandler::getKoopaOp(op);
            binary.lhs = (koopa_raw_value_t)lhs;
            binary.rhs = (koopa_raw_value_t)rhs;
            block_manager.addInst(result);
            return result;
        }
    };
}

// UnaryExpAST实现
int UnaryExpAST::cal_value() const {

    struct UnaryCalculator {
        static int negate(int val) { return -val; }
        static int logical_not(int val) { return !val; }
        static int identity(int val) { return val; }
    };
    
    int exp_val = exp->cal_value();
    
    if (type == Exp || op == "+") 
        return UnaryCalculator::identity(exp_val);
    if (op == "-") 
        return UnaryCalculator::negate(exp_val);
    if (op == "!") 
        return UnaryCalculator::logical_not(exp_val);
        
    return 0;
}

// AddExpAST实现
AddExpAST::AddExpAST(std::unique_ptr<BaseAST>& mul_exp)
    : mul_exp(std::move(mul_exp)) {
    type = Exp;
}

AddExpAST::AddExpAST(const char* op, 
                     std::unique_ptr<BaseAST>& add_exp,
                     std::unique_ptr<BaseAST>& mul_exp)
    : op(op), add_exp(std::move(add_exp)), mul_exp(std::move(mul_exp)) {
    type = Op;
}

void* AddExpAST::to_koopa() const {
    if (type == Exp) 
        return mul_exp->to_koopa();
        
    return KoopaValueGenerator::generateBinaryOp(
        op,
        add_exp->to_koopa(),
        mul_exp->to_koopa()
    );
}

int AddExpAST::cal_value() const {
    if (type == Exp)
        return mul_exp->cal_value();
        
    return OperatorHandler::compute(
        op,
        add_exp->cal_value(),
        mul_exp->cal_value()
    );
}

//////////////////////////////////////////////////////////////////////////////


namespace {
    // 封装乘法表达式的核心运算逻辑
    class MulExpHandler {
    private:
        // 运算符映射结构
        struct OperatorInfo {
            int (*calculate)(int, int);  // 计算函数指针
            koopa_raw_binary_op koopa_op;  // 对应的Koopa操作码
        };
        
        // 运算函数实现
        static int multiply(int x, int y) { return x * y; }
        static int divide(int x, int y) { return y != 0 ? x / y : 0; }
        static int modulo(int x, int y) { return y != 0 ? x % y : 0; }
        
        // 延迟初始化的运算符映射表
        static std::map<std::string, OperatorInfo> initOperatorMap() {
            std::map<std::string, OperatorInfo> mapping;
            mapping["*"] = {multiply, KOOPA_RBO_MUL};
            mapping["/"] = {divide, KOOPA_RBO_DIV};
            mapping["%"] = {modulo, KOOPA_RBO_MOD};
            return mapping;
        }
        
        static const std::map<std::string, OperatorInfo>& getOperatorMap() {
            static const auto op_map = initOperatorMap();
            return op_map;
        }
        
    public:
        // 执行计算
        static int evaluateExpression(const std::string& op, int left, int right) {
            const auto& op_map = getOperatorMap();
            auto it = op_map.find(op);
            return (it != op_map.end()) ? it->second.calculate(left, right) : 0;
        }
        
        // 获取Koopa操作码
        static koopa_raw_binary_op getKoopaOperator(const std::string& op) {
            const auto& op_map = getOperatorMap();
            auto it = op_map.find(op);
            return (it != op_map.end()) ? it->second.koopa_op : KOOPA_RBO_NOT_EQ;
        }
        
        // 创建Koopa值
        static koopa_raw_value_data* createKoopaValue() {
            auto value = new koopa_raw_value_data();
            value->ty = type_kind(KOOPA_RTT_INT32);
            value->name = nullptr;
            value->used_by = slice(KOOPA_RSIK_VALUE);
            return value;
        }
    };
    
    // 二元操作的Koopa IR生成器
    class BinaryKoopaGenerator {
    public:
        static void* generateBinaryOperation(
            const std::string& op,
            const void* lhs,
            const void* rhs
        ) {
            auto result = MulExpHandler::createKoopaValue();
            result->kind.tag = KOOPA_RVT_BINARY;
            
            auto& binary = result->kind.data.binary;
            binary.op = MulExpHandler::getKoopaOperator(op);
            binary.lhs = static_cast<koopa_raw_value_t>(const_cast<void*>(lhs));
            binary.rhs = static_cast<koopa_raw_value_t>(const_cast<void*>(rhs));
            
            block_manager.addInst(result);
            return result;
        }
    };
}

// 乘法表达式AST实现
MulExpAST::MulExpAST(std::unique_ptr<BaseAST>& unary_exp)
    : unary_exp(std::move(unary_exp)) {
    type = Exp;
}

MulExpAST::MulExpAST(
    const char* op,
    std::unique_ptr<BaseAST>& mul_exp,
    std::unique_ptr<BaseAST>& unary_exp
) : op(op), 
    mul_exp(std::move(mul_exp)), 
    unary_exp(std::move(unary_exp)) {
    type = Op;
}

void* MulExpAST::to_koopa() const {
    // 单一表达式直接返回
    if (type == Exp) {
        return unary_exp->to_koopa();
    }
    
    // 生成二元操作的Koopa IR
    return BinaryKoopaGenerator::generateBinaryOperation(
        op,
        mul_exp->to_koopa(),
        unary_exp->to_koopa()
    );
}

int MulExpAST::cal_value() const {
    // 单一表达式直接计算
    if (type == Exp) {
        return unary_exp->cal_value();
    }
    
    // 计算二元表达式
    return MulExpHandler::evaluateExpression(
        op,
        mul_exp->cal_value(),
        unary_exp->cal_value()
    );
}

//////////////////////////////////////////////////////////////////////////////


namespace {
    // 关系运算处理器
    class RelationalProcessor {
    private:
        // 定义关系运算的处理方式
        struct OperatorDetails {
            std::function<bool(int, int)> comparator;  // 比较函数
            koopa_raw_binary_op ir_op;                 // IR操作码
        };
        
        // 值生成器
        class KoopaValueGenerator {
        public:
            static koopa_raw_value_data* createBasicValue() {
                auto value_data = new koopa_raw_value_data();
                initializeValueData(value_data);
                return value_data;
            }
            
        private:
            static void initializeValueData(koopa_raw_value_data* data) {
                data->ty = type_kind(KOOPA_RTT_INT32);
                data->name = nullptr;
                data->used_by = slice(KOOPA_RSIK_VALUE);
                data->kind.tag = KOOPA_RVT_BINARY;
            }
        };
        
        // 延迟初始化操作符映射表
        static const std::map<std::string, OperatorDetails>& getOperatorMap() {
            static const std::map<std::string, OperatorDetails> op_map = {
                {"<",  {[](int a, int b) { return a < b; },  KOOPA_RBO_LT}},
                {">",  {[](int a, int b) { return a > b; },  KOOPA_RBO_GT}},
                {"<=", {[](int a, int b) { return a <= b; }, KOOPA_RBO_LE}},
                {">=", {[](int a, int b) { return a >= b; }, KOOPA_RBO_GE}}
            };
            return op_map;
        }
        
    public:
        // 执行关系运算
        static bool executeComparison(const std::string& op, int lhs, int rhs) {
            const auto& ops = getOperatorMap();
            auto it = ops.find(op);
            return it != ops.end() ? it->second.comparator(lhs, rhs) : false;
        }
        
        // 生成IR指令
        static void* generateIR(const std::string& op, void* lhs, void* rhs) {
            auto result = KoopaValueGenerator::createBasicValue();
            configureIROperation(result, op, lhs, rhs);
            block_manager.addInst(result);
            return result;
        }
        
    private:
        static void configureIROperation(koopa_raw_value_data* value,
                                       const std::string& op,
                                       void* lhs, void* rhs) {
            const auto& ops = getOperatorMap();
            auto it = ops.find(op);
            if (it != ops.end()) {
                auto& binary = value->kind.data.binary;
                binary.op = it->second.ir_op;
                binary.lhs = static_cast<koopa_raw_value_t>(lhs);
                binary.rhs = static_cast<koopa_raw_value_t>(rhs);
            }
        }
    };
}

// 实现关系表达式AST的构造函数（单一表达式）
RelExpAST::RelExpAST(std::unique_ptr<BaseAST>& add_exp)
    : add_exp(std::move(add_exp)) {
    type = Exp;
}

// 实现关系表达式AST的构造函数（二元运算）
RelExpAST::RelExpAST(const char* op,
                     std::unique_ptr<BaseAST>& rel_exp,
                     std::unique_ptr<BaseAST>& add_exp)
    : op(op), rel_exp(std::move(rel_exp)), add_exp(std::move(add_exp)) {
    type = Op;
}

// 生成Koopa IR
void* RelExpAST::to_koopa() const {
    // 处理单一表达式的情况
    if (type == Exp) {
        return add_exp->to_koopa();
    }
    
    // 处理二元运算的情况
    return RelationalProcessor::generateIR(
        op,
        rel_exp->to_koopa(),
        add_exp->to_koopa()
    );
}

// 计算表达式的值
int RelExpAST::cal_value() const {
    // 处理单一表达式的情况
    if (type == Exp) {
        return add_exp->cal_value();
    }
    
    // 计算二元运算的结果
    return static_cast<int>(
        RelationalProcessor::executeComparison(
            op,
            rel_exp->cal_value(),
            add_exp->cal_value()
        )
    );
}


//////////////////////////////////////////////////////////////////////////////

// EqExpAST的构造函数实现
EqExpAST::EqExpAST(std::unique_ptr<BaseAST>& rel_exp) {

    auto initializeNode = [](std::unique_ptr<BaseAST>& source) -> std::unique_ptr<BaseAST> {
        return source ? std::move(source) : nullptr;
    };
    
    // 初始化成员
    this->rel_exp = initializeNode(rel_exp);
    this->type = Exp;
}

EqExpAST::EqExpAST(const char* op, 
                   std::unique_ptr<BaseAST>& eq_exp,
                   std::unique_ptr<BaseAST>& rel_exp) {

    auto initializeNode = [](std::unique_ptr<BaseAST>& source) -> std::unique_ptr<BaseAST> {
        return source ? std::move(source) : nullptr;
    };
    
    // 初始化所有成员
    this->op = op;
    this->eq_exp = initializeNode(eq_exp);
    this->rel_exp = initializeNode(rel_exp);
    this->type = Op;
}

//////////////////////////////////////////////////////////////////////////////


void* EqExpAST::to_koopa() const {
    // 处理单一表达式情况
    if (type == Exp) {
        return rel_exp->to_koopa();
    }
    
    // 初始化数据结构
    koopa_raw_value_data* result = new koopa_raw_value_data();
    result->ty = type_kind(KOOPA_RTT_INT32);
    result->name = nullptr;
    result->used_by = slice(KOOPA_RSIK_VALUE);
    result->kind.tag = KOOPA_RVT_BINARY;
    
    // 设置操作符
    auto& binary = result->kind.data.binary;
    if (op == "==") {
        binary.op = KOOPA_RBO_EQ;
    } else {  // op == "!="
        binary.op = KOOPA_RBO_NOT_EQ;
    }
    
    // 设置操作数
    binary.lhs = static_cast<koopa_raw_value_t>(eq_exp->to_koopa());
    binary.rhs = static_cast<koopa_raw_value_t>(rel_exp->to_koopa());
    
    // 添加到块管理器
    block_manager.addInst(result);
    return result;
}

int EqExpAST::cal_value() const {
    // 处理单一表达式情况
    if (type == Exp) {
        return rel_exp->cal_value();
    }
    
    // 获取操作数的值
    const int left_value = eq_exp->cal_value();
    const int right_value = rel_exp->cal_value();
    
    // 根据操作符计算结果
    if (op == "==") {
        return (left_value == right_value) ? 1 : 0;
    } else {  // op == "!="
        return (left_value != right_value) ? 1 : 0;
    }
}


//////////////////////////////////////////////////////////////////////////////

// LAndExpAST
LAndExpAST::LAndExpAST(std::unique_ptr<BaseAST> &eq_exp)
    : eq_exp(std::move(eq_exp)) {
  // 初始化类型为 Exp
  type = Exp;
}

LAndExpAST::LAndExpAST(const char *op, std::unique_ptr<BaseAST> &and_exp,
                       std::unique_ptr<BaseAST> &eq_exp)
    : op(op), and_exp(std::move(and_exp)), eq_exp(std::move(eq_exp)) {
  // 初始化类型为 Op
  type = Op;
}


static koopa_raw_type_t create_int32_type() {
  return type_kind(KOOPA_RTT_INT32);
}


static void init_binary_op(koopa_raw_value_data *ret, koopa_raw_value_t lhs, koopa_raw_value_t rhs) {
  auto &binary = ret->kind.data.binary;
  binary.op = KOOPA_RBO_NOT_EQ;
  binary.lhs = lhs;
  binary.rhs = rhs;
}


static koopa_raw_value_t create_zero_value() {
  NumberAST zero(0);
  return (koopa_raw_value_t)zero.to_koopa();
}

// 创建布尔表达式的实现
void *LAndExpAST::make_bool(const std::unique_ptr<BaseAST> &exp) const {
  // 创建一个新的 Koopa 原始值数据对象
  auto *ret = new koopa_raw_value_data();
  
  // 设置类型为 int32
  ret->ty = create_int32_type();

  // 初始化其他字段
  ret->name = nullptr;
  ret->used_by = slice(KOOPA_RSIK_VALUE);
  ret->kind.tag = KOOPA_RVT_BINARY;

  // 初始化二元操作
  init_binary_op(ret, (koopa_raw_value_t)exp->to_koopa(), create_zero_value());

  // 将指令添加到块管理器中
  block_manager.addInst(ret);

  return ret;
}

//////////////////////////////////////////////////////////////////////////////

void* LAndExpAST::to_koopa() const {
    // 处理单个表达式的特殊情况
    if (type == Exp) {
        return eq_exp->to_koopa();
    }


    struct MemoryHelper {
        static koopa_raw_value_data* createValue(const char* name = nullptr) {
            return new koopa_raw_value_data();
        }
        
        static void initializeValueBase(koopa_raw_value_data* val, 
                                      const char* name = nullptr) {
            val->name = name;
            val->used_by = slice(KOOPA_RSIK_VALUE);
        }
    };


    auto createBasicBlock = [](const char* blockName) {
        auto block = new koopa_raw_basic_block_data_t();
        block->name = blockName;
        block->params = slice(KOOPA_RSIK_VALUE);
        block->used_by = slice(KOOPA_RSIK_VALUE);
        return block;
    };

    // 分配临时存储空间
    auto allocTemp = [&]() {
        auto tempAlloc = MemoryHelper::createValue("@temp");
        tempAlloc->ty = pointer_type_kind(KOOPA_RTT_INT32);
        MemoryHelper::initializeValueBase(tempAlloc, "@temp");
        tempAlloc->kind.tag = KOOPA_RVT_ALLOC;
        block_manager.addInst(tempAlloc);
        return tempAlloc;
    };

    // 创建存储指令
    auto createStore = [&](koopa_raw_value_data* dest, koopa_raw_value_t value) {
        auto storeInst = MemoryHelper::createValue();
        storeInst->ty = type_kind(KOOPA_RTT_UNIT);
        MemoryHelper::initializeValueBase(storeInst);
        storeInst->kind.tag = KOOPA_RVT_STORE;
        storeInst->kind.data.store.dest = dest;
        storeInst->kind.data.store.value = value;
        block_manager.addInst(storeInst);
        return storeInst;
    };

    // 实现主要逻辑
    auto tempVar = allocTemp();
    
    // 初始化为假(0)
    createStore(tempVar, (koopa_raw_value_t)NumberAST(0).to_koopa());

    // 设置条件分支
    auto branchInst = MemoryHelper::createValue();
    branchInst->ty = type_kind(KOOPA_RTT_UNIT);
    MemoryHelper::initializeValueBase(branchInst);
    branchInst->kind.tag = KOOPA_RVT_BRANCH;
    
    // 创建条件和基本块
    branchInst->kind.data.branch.cond = (koopa_raw_value_t)make_bool(and_exp);
    auto trueBlockData = createBasicBlock("%true");
    auto falseBlockData = createBasicBlock("%end");
    
    // 配置分支目标
    branchInst->kind.data.branch.true_bb = (koopa_raw_basic_block_t)trueBlockData;
    branchInst->kind.data.branch.false_bb = (koopa_raw_basic_block_t)falseBlockData;
    branchInst->kind.data.branch.true_args = slice(KOOPA_RSIK_VALUE);
    branchInst->kind.data.branch.false_args = slice(KOOPA_RSIK_VALUE);
    
    block_manager.addInst(branchInst);

    // 处理真分支
    block_manager.newBlock(trueBlockData);
    createStore(tempVar, (koopa_raw_value_t)make_bool(eq_exp));
    block_manager.addInst(jump_value(falseBlockData));

    // 处理结果块
    block_manager.newBlock(falseBlockData);
    auto resultValue = MemoryHelper::createValue();
    resultValue->ty = type_kind(KOOPA_RTT_INT32);
    MemoryHelper::initializeValueBase(resultValue);
    resultValue->kind.tag = KOOPA_RVT_LOAD;
    resultValue->kind.data.load.src = tempVar;
    block_manager.addInst(resultValue);

    return resultValue;
}


//////////////////////////////////////////////////////////////////////////////

int LAndExpAST::cal_value() const {

    enum CalcType { SINGLE = 0, COMPOUND = 1 };
    

    auto calc_single = [this]() -> int {
        return eq_exp->cal_value();
    };
    

    auto calc_compound = [this]() -> int {
        // 使用短路求值特性
        int left_result = and_exp->cal_value();
        if (!left_result) return 0;
        return eq_exp->cal_value() ? 1 : 0;
    };
    
    // 根据类型选择计算方式
    return (type == Exp) ? calc_single() : calc_compound();
}

namespace {

    class ExpConstructHelper {
    public:
        // 初始化单一表达式
        static void init_exp(LOrExpAST* ast, 
                           std::unique_ptr<BaseAST>& and_exp) {
            ast->and_exp = std::move(and_exp);
            ast->type = LOrExpAST::Exp;
        }
        
        // 初始化复合表达式
        static void init_op_exp(LOrExpAST* ast, 
                              const char* op,
                              std::unique_ptr<BaseAST>& or_exp,
                              std::unique_ptr<BaseAST>& and_exp) {
            ast->op = op;
            ast->or_exp = std::move(or_exp);
            ast->and_exp = std::move(and_exp);
            ast->type = LOrExpAST::Op;
        }
    };
}

LOrExpAST::LOrExpAST(std::unique_ptr<BaseAST>& and_exp) {
    ExpConstructHelper::init_exp(this, and_exp);
}

LOrExpAST::LOrExpAST(const char* op,
                     std::unique_ptr<BaseAST>& or_exp,
                     std::unique_ptr<BaseAST>& and_exp) {
    ExpConstructHelper::init_op_exp(this, op, or_exp, and_exp);
}

//////////////////////////////////////////////////////////////////////////////

void* LOrExpAST::make_bool(const std::unique_ptr<BaseAST>& exp) const {

    auto create_binary_compare = [](koopa_raw_value_t left_val, 
                                  koopa_raw_value_t right_val) 
                                  -> koopa_raw_value_data* {
        // 分配内存并初始化基本属性
        koopa_raw_value_data* result = new koopa_raw_value_data();
        
        // 设置数据类型为32位整型
        result->ty = type_kind(KOOPA_RTT_INT32);
        result->name = nullptr;
        
        // 初始化使用信息
        koopa_raw_slice_t usage_info = slice(KOOPA_RSIK_VALUE);
        result->used_by = usage_info;
        
        // 配置二进制操作
        result->kind.tag = KOOPA_RVT_BINARY;
        
        // 获取二进制操作数据结构的引用
        auto& bin_op = result->kind.data.binary;
        
        // 设置操作类型为不等于
        bin_op.op = KOOPA_RBO_NOT_EQ;
        
        // 设置操作数
        bin_op.lhs = left_val;
        bin_op.rhs = right_val;
        
        return result;
    };
    
    // 生成比较值
    NumberAST constant_zero(0);
    void* zero_val = constant_zero.to_koopa();
    void* exp_val = exp->to_koopa();
    
    // 构建比较指令
    koopa_raw_value_data* compare_inst = 
        create_binary_compare((koopa_raw_value_t)exp_val,
                            (koopa_raw_value_t)zero_val);
    
    // 将指令添加到当前基本块
    block_manager.addInst(compare_inst);
    
    return compare_inst;
}

//////////////////////////////////////////////////////////////////////////////

void* LOrExpAST::to_koopa() const {
    // 处理单一表达式的情况
    if (type == Exp) {
        return and_exp->to_koopa();
    }
    

    auto create_base_data = [](const char* name) -> koopa_raw_value_data* {
        koopa_raw_value_data* data = new koopa_raw_value_data();
        data->name = name;
        data->used_by = slice(KOOPA_RSIK_VALUE);
        return data;
    };
    

    auto create_block = [](const char* name) -> koopa_raw_basic_block_data_t* {
        koopa_raw_basic_block_data_t* block = new koopa_raw_basic_block_data_t();
        block->name = name;
        block->params = slice(KOOPA_RSIK_VALUE);
        block->used_by = slice(KOOPA_RSIK_VALUE);
        return block;
    };
    
    // 创建临时变量分配
    koopa_raw_value_data* temp_alloc = create_base_data("@temp");
    temp_alloc->ty = pointer_type_kind(KOOPA_RTT_INT32);
    temp_alloc->kind.tag = KOOPA_RVT_ALLOC;
    block_manager.addInst(temp_alloc);
    
    // 存储初始值(1)到临时变量
    koopa_raw_value_data* init_store = create_base_data(nullptr);
    init_store->ty = type_kind(KOOPA_RTT_UNIT);
    init_store->kind.tag = KOOPA_RVT_STORE;
    init_store->kind.data.store.dest = temp_alloc;
    init_store->kind.data.store.value = (koopa_raw_value_t)NumberAST(1).to_koopa();
    block_manager.addInst(init_store);
    
    // 创建分支指令
    koopa_raw_value_data* branch_inst = create_base_data(nullptr);
    branch_inst->ty = type_kind(KOOPA_RTT_UNIT);
    branch_inst->kind.tag = KOOPA_RVT_BRANCH;
    
    // 设置条件和基本块
    auto& branch_data = branch_inst->kind.data.branch;
    branch_data.cond = (koopa_raw_value_t)make_bool(or_exp);
    
    // 创建true和false基本块
    koopa_raw_basic_block_data_t* end_block = create_block("%end");
    koopa_raw_basic_block_data_t* false_block = create_block("%false");
    
    // 配置分支目标
    branch_data.true_bb = (koopa_raw_basic_block_t)end_block;
    branch_data.false_bb = (koopa_raw_basic_block_t)false_block;
    branch_data.true_args = slice(KOOPA_RSIK_VALUE);
    branch_data.false_args = slice(KOOPA_RSIK_VALUE);
    
    block_manager.addInst(branch_inst);
    
    // 构建false块
    block_manager.newBlock(false_block);
    koopa_raw_value_data* false_store = create_base_data(nullptr);
    false_store->ty = type_kind(KOOPA_RTT_UNIT);
    false_store->kind.tag = KOOPA_RVT_STORE;
    false_store->kind.data.store.dest = temp_alloc;
    false_store->kind.data.store.value = (koopa_raw_value_t)make_bool(and_exp);
    block_manager.addInst(false_store);
    block_manager.addInst(jump_value(end_block));
    
    // 构建结束块
    block_manager.newBlock(end_block);
    koopa_raw_value_data* result = create_base_data(nullptr);
    result->ty = type_kind(KOOPA_RTT_INT32);
    result->kind.tag = KOOPA_RVT_LOAD;
    result->kind.data.load.src = temp_alloc;
    block_manager.addInst(result);
    
    return result;
}

//////////////////////////////////////////////////////////////////////////////

int LOrExpAST::cal_value() const {
    // 使用三目运算符和短路求值的特性
    return (type == Exp) ? 
           and_exp->cal_value() : 
           (or_exp->cal_value() | (and_exp->cal_value() != 0)) != 0;
}

// 数值节点实现
class NumberValueHelper {
private:
    // 创建基础数据结构
    static koopa_raw_value_data* initialize_value_data() {
        koopa_raw_value_data* value_container = new koopa_raw_value_data();
        
        // 设置基本属性
        value_container->name = nullptr;
        value_container->used_by = slice(KOOPA_RSIK_VALUE);
        
        return value_container;
    }
    
    // 配置整数类型
    static void setup_integer_type(koopa_raw_value_data* data) {
        // 设置32位整数类型
        data->ty = type_kind(KOOPA_RTT_INT32);
    }
    
public:
    // 创建整数值对象
    static void* create_integer_value(int numeric_value) {
        // 初始化数据结构
        koopa_raw_value_data* result = initialize_value_data();
        
        // 设置类型信息
        setup_integer_type(result);
        
        // 配置整数值
        result->kind.tag = KOOPA_RVT_INTEGER;
        result->kind.data.integer.value = numeric_value;
        
        return result;
    }
};

// 数值AST节点构造函数
NumberAST::NumberAST(int initial_value) : val(initial_value) {}

// 转换为Koopa IR
void* NumberAST::to_koopa() const {
    return NumberValueHelper::create_integer_value(val);
}

// 计算数值
int NumberAST::cal_value() const {
    // 使用位操作确保返回值为整数
    return val & (~0);
}