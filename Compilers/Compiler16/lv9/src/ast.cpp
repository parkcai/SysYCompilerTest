// src/ast.cpp
#include "ast.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <functional>
#include <queue>
#include <cmath>

using namespace std;

int temp_var_id = 0;      // 全局临时变量计数器
int label_if_id = 0;        // 全局if-else标签计数器
int label_while_id = 0;     // 全局while标签计数器
int label_land_id = 0;      // 全局短路与标签计数器
int label_lor_id = 0;       // 全局短路或标签计数器

// 全局循环上下文栈: <continue_label, break_label>
vector<pair<string, string>> loop_stack;

bool func_enter_flag=false;

// 获取下一个临时变量名
string AllocTempVar() {
    return "%" + to_string(temp_var_id++);
}

void ResetTempVar() {
    temp_var_id = 0;
}

vector<BaseAST*> FlattenAndFillInitVal(AggregateAST* agg, const vector<int>& offsets, int begin, int end) {
    vector<BaseAST*> flat_list;
    for(auto& elem : agg->elements){
        AggregateAST* sub_agg = dynamic_cast<AggregateAST*>(elem);
        if(sub_agg){
            for(int k = begin + 1; k < end; ++k){
                if (flat_list.size() % offsets[k] == 0) {
                    auto sub_flat_list = FlattenAndFillInitVal(sub_agg, offsets, k, end);
                    flat_list.insert(flat_list.end(), sub_flat_list.begin(), sub_flat_list.end());
                    break;
                }
            }
        }
        else{
            flat_list.push_back(elem);
        }
    }
    int fill_size = offsets[begin] - flat_list.size();
    flat_list.insert(flat_list.end(), fill_size, new NumberAST(0));
    return flat_list;
}


vector<BaseAST*> ComputeArraryInitList(AggregateAST* agg, const vector<int>& dimensions){
    vector<int> offsets;
    for(int i = dimensions.size() - 1; i >= 0; --i){
        if(i == dimensions.size() - 1){
            offsets.push_back(dimensions[i]);
        }
        else{
            int offset = offsets.back() * dimensions[i];
            offsets.push_back(offset);
        }
    }
    reverse(offsets.begin(), offsets.end());
    vector<BaseAST*> flat_init = FlattenAndFillInitVal(agg, offsets, 0, offsets.size());

    return flat_init;;
}

// Helper function to compute multi-dimensional indices from flat index
vector<int> GetMultiDimIndices(int index, const vector<int>& dimensions) {
    vector<int> indices(dimensions.size(), 0);
    int remaining = index;
    for(int d = dimensions.size()-1; d >=0; --d){
        indices[d] = remaining % dimensions[d];
        remaining /= dimensions[d];
    }
    return indices;
}

string GetTypeString(const vector<int>& dimensions, bool is_func_param=false){
    string type_str;
    int idx = 0;
    if(is_func_param){
        type_str += "*";
        idx = 1;
    }
    for (int i = idx; i < dimensions.size(); ++i) {
        type_str += "[";
    }
    type_str += "i32";
    for (int i = dimensions.size()-1; i >= idx; --i) {
        type_str += ", " + to_string(dimensions[i]) + "]";
    }
    return type_str;
}

string BuildNestedAggregate(const vector<BaseAST*>& flat_init, const vector<int>& dimensions, int dim, int& idx) {
    if (dim == dimensions.size() - 1) { // Last dimension
        string agg = "{";
        for(int i = 0; i < dimensions[dim]; ++i) {
            if(i > 0) agg += ", ";

            BaseAST* expr = flat_init[idx];
            EvalResult res = expr->Eval();
            if(res.success){
                agg += to_string(res.value);
            }
            else{
                agg += expr->ir_name;
            }
            idx++;
            
        }
        agg += "}";
        return agg;
    }
    else { // Higher dimensions
        string agg = "{";
        for(int i = 0; i < dimensions[dim]; ++i) {
            if(i > 0) agg += ", ";
            agg += BuildNestedAggregate(flat_init, dimensions, dim +1, idx);
        }
        agg += "}";
        return agg;
    }
}

// Implement CompUnitAST
void CompUnitAST::GenerateIR(ostream& out) const {
    // 首先插入SysY标准库函数信息
    InitSysYLibFuncs();

    // 输出库函数的decl声明
    out << "decl @getint(): i32\n";
    out << "decl @getch(): i32\n";
    out << "decl @getarray(*i32): i32\n";
    out << "decl @putint(i32)\n";
    out << "decl @putch(i32)\n";
    out << "decl @putarray(i32, *i32)\n";
    out << "decl @starttime()\n";
    out << "decl @stoptime()\n";

    for (const auto& item : comp_unit_items) {
        item->GenerateIR(out);
    }
}
// Implement NumberAST
void NumberAST::GenerateIR(ostream& out) const {
    this->ir_name = to_string(value); // 立即数直接使用数值
}

EvalResult NumberAST::Eval() const {
    return EvalResult(value);
}

// Implement FuncDefAST
FuncDefAST::~FuncDefAST() {
    for (auto param : params) {
        delete param;
    }
}

void FuncDefAST::GenerateIR(ostream& out) const {
    // Function name
    string func_name = ident;
    if (!func_name.empty() && func_name[0] == '@') {
        func_name = func_name.substr(1);
    }

    // Function return type
    TypeAST* func_type_ptr = dynamic_cast<TypeAST*>(func_type.get());
    string ret_type = func_type_ptr->type;

    // Insert function into global symbol table
    vector<string> param_types;
    for (auto param : params) {
        FuncFParamAST* func_param_ptr = dynamic_cast<FuncFParamAST*>(param);
        vector<int> dimensions;
        for(auto dim_ast : func_param_ptr->array_dims){
            EvalResult dim_eval = dim_ast->Eval();
            if(!dim_eval.success) {
                throw runtime_error("Dimension eval failed.");
            }
            dimensions.push_back(dim_eval.value);
        }

        if (func_param_ptr->is_array) {
            // 构建数组参数的类型字符串，例如 "*i32" 或 "*[i32,10]"
            string type = GetTypeString(dimensions, true);
            param_types.push_back(type);
        } else {
            if(func_param_ptr->btype == "int"){
                param_types.push_back("i32");
            }
            // else{
            //     throw runtime_error("Invalid function parameter type: " + func_param_ptr->btype);
            // }
            
        }
    }
    InsertFunc(ident, param_types, ret_type);

    // Generate function signature
    out << "fun @" << func_name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        FuncFParamAST* param = dynamic_cast<FuncFParamAST*>(params[i]);
        if (i > 0) out << ", ";
        
        // Determine parameter type
        string param_type = param_types[i];
        out << "%" << param->ident << ": " << param_type;
    }
    if (ret_type == "int") {
        out << "): i32 {\n";
    } else {
        out << ") {\n";
    }

    // Entry label
    out << "%entry:\n";

    EnterScope();
    ResetTempVar();
    func_enter_flag = true;

    // Allocate space for parameters and store them
    for (int i = 0; i < params.size(); ++i) {
        FuncFParamAST* param = dynamic_cast<FuncFParamAST*>(params[i]);
        if (!param) {
            throw runtime_error("Invalid function parameter type.");
        }

        // Determine parameter type
        string param_type = param_types[i];

        // Allocate space for parameter
        string addr = "@SYMTAB_LEVEL_" + to_string(GetSymtabID()) + "_" + param->ident;
        out << "  " << addr << " = alloc " << param_type << "\n";

        // Store the parameter into allocated space
        out << "  store %" << param->ident << ", " << addr << "\n";

        // Insert into symbol table with correct type
        if (param->is_array) {
            // For array parameters, insert as pointers
            InsertVar(param->ident, addr, true, param->array_dims.size());
        } else {
            InsertVar(param->ident, addr);
        }
    }

    block->GenerateIR(out);

    if (!block->IsTerminated()) {
        if (ret_type == "int") {
            out << "  ret 0\n";
        } else if (ret_type == "void") {
            out << "  ret\n";
        }
    }

    out << "}\n";
}

void FuncCallAST::GenerateIR(ostream& out) const {
    // Generate code for each parameter
    vector<string> param_ir_names;
    for (auto param : params) {
        param->GenerateIR(out);
        param_ir_names.push_back(param->ir_name);
    }
    
    // Generate call instruction
    string func_name = ident;
    if (!func_name.empty() && func_name[0] == '@') {
        func_name = func_name.substr(1);
    }

    // Lookup function in symbol table to get return type
    SymbolEntry* func_entry = LookupFunc(ident);
    if (!func_entry) {
        throw runtime_error("Undefined function: " + ident);
    }

    if (func_entry->ret_type == "void") {
        out << "  call @" << func_name << "(";
        for (size_t i = 0; i < param_ir_names.size(); ++i) {
            if (i > 0) out << ", ";
            out << param_ir_names[i];
        }
        out << ")\n";
    } else {
        string temp = AllocTempVar();
        out << "  " << temp << " = call @" << func_name << "(";
        for (size_t i = 0; i < param_ir_names.size(); ++i) {
            if (i > 0) out << ", ";
            out << param_ir_names[i];
        }
        out << ")\n";
        this->ir_name = temp;
    }
}

void TypeAST::GenerateIR(ostream& out) const {
    return;
}

void FuncFParamAST::GenerateIR(ostream& out) const {
    // Function parameters are handled in FuncDefAST; no IR generation needed here
}

// Implement BlockAST
BlockAST::~BlockAST() {
    for (auto item : block_items) {
        delete item;
    }
}

void BlockAST::GenerateIR(ostream& out) const {
    if(!func_enter_flag){
        EnterScope();
    } else {
        func_enter_flag = false;
    }
    bool terminated = false;
    for (const auto &item : block_items) {
        item->GenerateIR(out);
        if (item->IsTerminated()) {
            terminated = true;
        }
        if (terminated) {
            break;
        }
    }
    ExitScope();
}

bool BlockAST::IsTerminated() const {
    for (const auto &item : block_items) {
        if (item->IsTerminated()) {
            return true;
        }
    }
    return false;
}

// Implement UnaryExpAST
void UnaryExpAST::GenerateIR(ostream& out) const {
    EvalResult operand_eval = operand->Eval();

    if (operand_eval.success) {
        // 操作数是常量，可以在编译期计算
        EvalResult result = this->Eval();
        this->ir_name = to_string(result.value);
    } else {
        // 操作数不是常量，需要生成 IR 指令
        operand->GenerateIR(out);
        string operand_ir = operand->ir_name;

        if (op == "+") {
            // +x 等价于 x
            this->ir_name = operand_ir;
        } else if (op == "-") {
            string temp = AllocTempVar();
            out << "  " << temp << " = sub 0, " << operand_ir << "\n";
            this->ir_name = temp;
        } else if (op == "!") {
            string temp = AllocTempVar();
            out << "  " << temp << " = eq " << operand_ir << ", 0\n";
            this->ir_name = temp;
        } 
        else {
            throw runtime_error("Unsupported unary operator: " + op);
        }
    }
}

EvalResult UnaryExpAST::Eval() const {
    EvalResult operand_eval = operand->Eval();
    if (!operand_eval.success) {
        return EvalResult();
    }

    if (op == "+") {
        return EvalResult(operand_eval.value);
    } else if (op == "-") {
        return EvalResult(-operand_eval.value);
    } else if (op == "!") {
        return EvalResult(!operand_eval.value);
    } else {
        return EvalResult();
    }
}

// Implement BinaryExpAST
void BinaryExpAST::GenerateIR(ostream& out) const {
    EvalResult lhs_eval = lhs->Eval();
    EvalResult rhs_eval = rhs->Eval();

    if (lhs_eval.success && rhs_eval.success) {
        // 两个操作数都是常量，可以在编译期计算
        EvalResult result = this->Eval();
        this->ir_name = to_string(result.value);
    } else {
        // 至少有一个操作数不是常量，需要生成 IR 指令
        lhs->GenerateIR(out);
        string lhs_ir = lhs->ir_name;

        rhs->GenerateIR(out);
        string rhs_ir = rhs->ir_name;

        string temp = AllocTempVar();

        // 生成对应的二元操作指令
        if (op == "+") {
            out << "  " << temp << " = add " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "-") {
            out << "  " << temp << " = sub " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "*") {
            out << "  " << temp << " = mul " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "/") {
            out << "  " << temp << " = div " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "%") {
            out << "  " << temp << " = mod " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "<") {
            out << "  " << temp << " = lt " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == ">") {
            out << "  " << temp << " = gt " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "<=") {
            out << "  " << temp << " = le " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == ">=") {
            out << "  " << temp << " = ge " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "==") {
            out << "  " << temp << " = eq " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "!=") {
            out << "  " << temp << " = ne " << lhs_ir << ", " << rhs_ir << "\n";
        } else if (op == "&&") {
            // 逻辑与，需要特殊处理
            string temp1 = AllocTempVar();
            string temp2 = AllocTempVar();
            out << "  " << temp1 << " = ne " << lhs_ir << ", 0\n";
            out << "  " << temp2 << " = ne " << rhs_ir << ", 0\n";
            out << "  " << temp << " = and " << temp1 << ", " << temp2 << "\n";
        } else if (op == "||") {
            // 逻辑或，需要特殊处理
            string temp1 = AllocTempVar();
            string temp2 = AllocTempVar();
            out << "  " << temp1 << " = ne " << lhs_ir << ", 0\n";
            out << "  " << temp2 << " = ne " << rhs_ir << ", 0\n";
            out << "  " << temp << " = or " << temp1 << ", " << temp2 << "\n";
        } 
        else {
            throw runtime_error("Unsupported binary operator: " + op);
        }

        this->ir_name = temp;
    }
}

EvalResult BinaryExpAST::Eval() const {
    EvalResult lhs_eval = lhs->Eval();
    EvalResult rhs_eval = rhs->Eval();

    if (!lhs_eval.success || !rhs_eval.success) {
        return EvalResult();
    }

    int left = lhs_eval.value;
    int right = rhs_eval.value;
    int result = 0;

    if (op == "+") {
        result = left + right;
    }
    else if (op == "-") {
        result = left - right;
    }
    else if (op == "*") {
        result = left * right;
    }
    else if (op == "/") {
        if (right == 0) {
            throw runtime_error("Division by zero");
        }
        result = left / right;
    }
    else if (op == "%") {
        if (right == 0) {
            throw runtime_error("Modulo by zero");
        }
        result = left % right;
    }
    else if (op == "<") {
        result = left < right;
    }
    else if (op == ">") {
        result = left > right;
    }
    else if (op == "<=") {
        result = left <= right;
    }
    else if (op == ">=") {
        result = left >= right;
    }
    else if (op == "==") {
        result = left == right;
    }
    else if (op == "!=") {
        result = left != right;
    }
    else if (op == "&&") {
        result = left && right;
    }
    else if (op == "||") {
        result = left || right;
    }
    else {
        throw runtime_error("Unsupported binary operator: " + op);
    }

    return EvalResult(result);
}

void LAndExpAST::GenerateIR(ostream& out) const {
    string lhs_true_label = "%LAND_LHS_TRUE_" + to_string(label_land_id);
    string lhs_false_label = "%LAND_LHS_FALSE_" + to_string(label_land_id);
    string end_label = "%LAND_END_" + to_string(label_land_id);
    string result_var = "@SYMTAB_LEVEL_" + to_string(GetSymtabID()) + "_LANDREULT_" + to_string(label_land_id);
    label_land_id++;
    out << "  " << result_var << " = alloc i32\n";

    // 计算左操作数
    lhs->GenerateIR(out);
    string lhs_ir = lhs->ir_name;

    // 判断左操作数是否为假（0）
    out << "  br " << lhs_ir << ", " << lhs_true_label << ", " << lhs_false_label << "\n";

    // 左操作数为真，继续计算右操作数
    out << lhs_true_label << ":\n";
    rhs->GenerateIR(out);
    string rhs_ir = rhs->ir_name;
    string rhs_ir_logic = AllocTempVar();
    out << "  " << rhs_ir_logic << " = ne " << rhs_ir << ", 0\n";
    out << "  store " << rhs_ir_logic << ", " << result_var << "\n";
    out << "  jump " << end_label << "\n";

    // 左操作数为假，结果为假
    out << lhs_false_label << ":\n";
    out << "  store 0, " << result_var << "\n";
    out << "  jump " << end_label << "\n";

    // 结束标签
    out << end_label << ":\n";
    string temp = AllocTempVar();
    out << "  " << temp << " = load " << result_var << "\n";
    this->ir_name = temp;
}

EvalResult LAndExpAST::Eval() const {
    EvalResult lhs_eval = lhs->Eval();
    if (!lhs_eval.success) {
        return EvalResult();
    }
    if (lhs_eval.value == 0) {
        return EvalResult(0);
    }
    EvalResult rhs_eval = rhs->Eval();
    if (!rhs_eval.success) {
        return EvalResult();
    }
    return EvalResult(rhs_eval.value != 0 ? 1 : 0);
}

void LOrExpAST::GenerateIR(ostream& out) const {
    string lhs_true_label = "%LOR_LHS_TRUE_" + to_string(label_lor_id);
    string lhs_false_label = "%LOR_LHS_False_" + to_string(label_lor_id);
    string end_label = "%LOR_END_" + to_string(label_lor_id);
    string result_var = "@SYMTAB_LEVEL_" + to_string(GetSymtabID()) + "_LORREULT_" + to_string(label_lor_id);
    label_lor_id++;
    out << "  " << result_var << " = alloc i32\n";

    // 计算左操作数
    lhs->GenerateIR(out);
    string lhs_ir = lhs->ir_name;

    // 判断左操作数是否为真（非0）
    out << "  br " << lhs_ir << ", " << lhs_true_label << ", " << lhs_false_label << "\n";

    // 左操作数为假，计算右操作数
    out << lhs_false_label << ":\n";
    rhs->GenerateIR(out);
    string rhs_ir = rhs->ir_name;
    string rhs_ir_logic = AllocTempVar();
    out << "  " << rhs_ir_logic << " = ne " << rhs_ir << ", 0\n";
    out << "  store " << rhs_ir_logic << ", " << result_var << "\n";
    out << "  jump " << end_label << "\n";

    // 左操作数为真，结果为真
    out << lhs_true_label << ":\n";
    out << "  store 1, " << result_var << "\n";
    out << "  jump " << end_label << "\n";

    // 结束标签
    out << end_label << ":\n";
    string temp = AllocTempVar();
    out << "  " << temp << " = load " << result_var << "\n";
    this->ir_name = temp;
}

EvalResult LOrExpAST::Eval() const {
    EvalResult lhs_eval = lhs->Eval();
    if (!lhs_eval.success) {
        return EvalResult();
    }
    if (lhs_eval.value != 0) {
        return EvalResult(1);
    }
    EvalResult rhs_eval = rhs->Eval();
    if (!rhs_eval.success) {
        return EvalResult();
    }
    return EvalResult(rhs_eval.value != 0 ? 1 : 0);
}

void LValAST::GenerateIR(ostream& out) const {
    SymbolEntry* entry = LookupSymbol(ident);

    if (!entry) {
        throw runtime_error("Undefined identifier: " + ident);
    }

    if (entry->type == SymbolType::CONST) {
        // 常量，直接使用值
        ir_name = to_string(entry->const_value);
    } 
    else if(entry->type == SymbolType::VAR){
        string temp = AllocTempVar();
        out << "  " << temp << " = load " << entry->var_addr << "\n";
        ir_name = temp;
    } 
    else if (entry->type == SymbolType::ARRAY || entry->type == SymbolType::CONST_ARRAY) {
        string current_ptr = entry->var_addr;

        for (size_t i = 0; i < index_exps.size(); ++i) {
            index_exps[i]->GenerateIR(out);
            string index_ir = index_exps[i]->ir_name;
            string ptr_temp = AllocTempVar();
            out << "  " << ptr_temp << " = getelemptr " << current_ptr << ", " << index_ir << "\n";
            current_ptr = ptr_temp;
        }

        if(index_exps.size() == entry->array_dimensions.size()){
            string value_temp = AllocTempVar();
            out << "  " << value_temp << " = load " << current_ptr << "\n";
            ir_name = value_temp;   
        }
        else{
            string ptr_temp = AllocTempVar();
            out << "  " << ptr_temp << " = getelemptr " << current_ptr << ", " << 0 << "\n";
            ir_name = ptr_temp;
        }
    }
    else if (entry->type == SymbolType::PTR){
        string current_ptr = AllocTempVar();
        out << "  " << current_ptr << " = load " << entry->var_addr << "\n";

        for (size_t i = 0; i < index_exps.size(); ++i) {
            index_exps[i]->GenerateIR(out);
            string index_ir = index_exps[i]->ir_name;
            string ptr_temp = AllocTempVar();
            if(i == 0){
                out << "  " << ptr_temp << " = getptr " << current_ptr << ", " << index_ir << "\n";
            }
            else{
                out << "  " << ptr_temp << " = getelemptr " << current_ptr << ", " << index_ir << "\n";
            }
            current_ptr = ptr_temp;
        }

        if(index_exps.size() == entry->ptr_dim){
            string value_temp = AllocTempVar();
            out << "  " << value_temp << " = load " << current_ptr << "\n";
            ir_name = value_temp;   
        }
        else{
            string ptr_temp = AllocTempVar();
            if(index_exps.size() == 0){
                out << "  " << ptr_temp << " = getptr " << current_ptr << ", " << 0 << "\n";
            }
            else{
                out << "  " << ptr_temp << " = getelemptr " << current_ptr << ", " << 0 << "\n";
            }
            ir_name = ptr_temp;
        }
    }
    else {
        throw runtime_error("Unknown symbol type" + PrintSymbolType(entry->type));
    }
}

EvalResult LValAST::Eval() const {
    if(is_array){
        return EvalResult();
    }

    // 查找符号
    SymbolEntry* entry = LookupSymbol(ident);

    if(!entry){
        throw runtime_error("Undefined symbol" + ident);
    }

    if (entry->type == SymbolType::CONST) {
        return EvalResult(entry->const_value);
    } else {
        return EvalResult();
    }
}

void ConstDefAST::GenerateIR(ostream& out) const {
    if (is_array) {
        // 处理多维数组
        vector<int> dimensions;
        for (const auto& dim_ast : array_dimensions) {
            EvalResult dim_res = dim_ast->Eval();
            if (!dim_res.success) {
                throw runtime_error("Array dimension is not a constant for: " + ident);
            }
            dimensions.push_back(dim_res.value);
        }

        bool is_global = (GetSymtabLevel() == 0);
        // 计算数组总元素数量
        int total_elements = 1;
        for(auto dim : dimensions) {
            total_elements *= dim;
        }

        // 构建类型字符串，如 [[i32,2],2]
        string type_str = GetTypeString(dimensions);

        if (is_global) {
            // 全局数组初始化使用 aggregate 或 zeroinit
            string addr = "@GLOBAL_CONST_ARRAY_" + ident;

            InsertGlobalArray(ident, addr, dimensions, true);

            if (const_init_val) {
                AggregateAST* agg = dynamic_cast<AggregateAST*>(const_init_val.get());
                vector<BaseAST*> flat_init = ComputeArraryInitList(agg, dimensions);
                
                if(flat_init.size() != total_elements){
                    throw runtime_error("Invalid flat_init");
                }

                // 生成 aggregate 初始化字符串
                int idx = 0;
                string agg_str = BuildNestedAggregate(flat_init, dimensions, 0, idx);
                out << "global " << addr << " = alloc " << type_str << ", " << agg_str << "\n";

            }
            else {
                // 未初始化数组，使用 zeroinit
                out << "global " << addr << " = alloc " << type_str << ", zeroinit\n";
            }

        }
        else {
            // 局部数组初始化
            string addr = "@SYMTAB_LEVEL_" + to_string(GetSymtabID()) + "_CONST_ARRAY_" + ident;

            InsertArray(ident, addr, dimensions, true);

            // Allocate the array
            out << "  " << addr << " = alloc " << type_str << "\n";

            if (const_init_val) {
                // 使用嵌套的初始化列表
                AggregateAST* agg = dynamic_cast<AggregateAST*>(const_init_val.get());
                vector<BaseAST*> flat_init = ComputeArraryInitList(agg, dimensions);

                if(flat_init.size() != total_elements){
                    throw runtime_error("Invalid flat_init");
                }   

                vector<string> level_ptrs(dimensions.size(), "");
                vector<int> level_idx(dimensions.size(), -1);

                // Initialize the top-level pointer
                level_ptrs[0] = addr; // -1 represents the base address

                for(int i = 0; i < total_elements; ++i) {
                    // 计算多维索引
                    vector<int> indices = GetMultiDimIndices(i, dimensions);

                    string current_ptr = addr;
                    for(size_t d = 0; d < dimensions.size() - 1; ++d) {
                        int idx = indices[d];
                        if(level_idx[d] != idx) {
                            string temp_ptr = AllocTempVar();
                            out << "  " << temp_ptr << " = getelemptr " << current_ptr << ", " << idx << "\n";
                            level_ptrs[d] = temp_ptr;
                            level_idx[d] = idx;
                        }
                        current_ptr = level_ptrs[d];
                    }

                    // Now, handle the last dimension
                    int last_dim = dimensions.size() - 1;
                    int last_idx = indices[last_dim];
                    string final_ptr = AllocTempVar();
                    out << "  " << final_ptr << " = getelemptr " << current_ptr << ", " << last_idx << "\n";

                    // 生成 store 指令
                    BaseAST* expr = flat_init[i];
                    expr->GenerateIR(out);
                    EvalResult res = expr->Eval();
                    if(res.success){
                        out << "  store " << res.value << ", " << final_ptr << "\n";
                    }
                    else{
                        out << "  store " << expr->ir_name << ", " << final_ptr << "\n";
                    }
                }

            }
        } 
    }
    else {
        // 处理标量常量
        EvalResult init_eval = const_init_val->Eval();
        if (!init_eval.success) {
            throw runtime_error("Cannot initialize constant with non-constant expression: " + ident);
        }
        InsertConst(ident, init_eval.value);
    }
}

EvalResult ConstDefAST::Eval() const {
    if (is_array) {
        return EvalResult();
    }
    EvalResult init_eval = const_init_val->Eval();
    if (!init_eval.success) {
        return EvalResult();
    }
    return EvalResult(init_eval.value);
}

// Implement ConstDeclAST
ConstDeclAST::~ConstDeclAST() {
    for (auto def : const_defs) {
        delete def;
    }
}

void ConstDeclAST::GenerateIR(ostream& out) const {
    for (const auto &def : const_defs) {
        def->GenerateIR(out);
    }
}

void VarDefAST::GenerateIR(ostream& out) const {
    bool is_global = (GetSymtabLevel() == 0);
    
    if (is_array) {
        // 处理多维数组
        vector<int> dimensions;
        for (const auto& dim_ast : array_dimensions) {
            EvalResult dim_res = dim_ast->Eval();
            if (!dim_res.success) {
                throw runtime_error("Array dimension is not a constant for: " + ident);
            }
            dimensions.push_back(dim_res.value);
        }

        // 计算数组总元素数量
        int total_elements = 1;
        for(auto dim : dimensions) {
            total_elements *= dim;
        }

        // 构建类型字符串，如 [[i32,2],2]
        string type_str = GetTypeString(dimensions);

        if (is_global) {
            // 全局数组初始化使用 aggregate 或 zeroinit
            string addr = "@GLOBAL_ARRAY_" + ident;

            InsertGlobalArray(ident, addr, dimensions, false);

            if (init_val) {
                // 使用嵌套的初始化列表
                AggregateAST* agg = dynamic_cast<AggregateAST*>(init_val.get());
                vector<BaseAST*> flat_init = ComputeArraryInitList(agg, dimensions);

                if(flat_init.size() != total_elements){
                    throw runtime_error("Invalid flat_init");
                }

                // 生成 aggregate 初始化字符串
                int idx = 0;
                string agg_str = BuildNestedAggregate(flat_init, dimensions, 0, idx);
                out << "global " << addr << " = alloc " << type_str << ", " << agg_str << "\n";

            }
            else {
                // 未初始化数组，使用 zeroinit
                out << "global " << addr << " = alloc " << type_str << ", zeroinit\n";
            }

        }
        else {
            // 局部数组初始化
            string addr = "@SYMTAB_LEVEL_" + to_string(GetSymtabID()) + "_ARRAY_" + ident;

            InsertArray(ident, addr, dimensions, false);

            // Allocate the array
            out << "  " << addr << " = alloc " << type_str << "\n";
            
            if (init_val) {
                // 使用嵌套的初始化列表
                AggregateAST* agg = dynamic_cast<AggregateAST*>(init_val.get());
                vector<BaseAST*> flat_init = ComputeArraryInitList(agg, dimensions);

                if(flat_init.size() != total_elements){
                    throw runtime_error("Invalid flat_init");
                }

                vector<string> level_ptrs(dimensions.size(), "");
                vector<int> level_idx(dimensions.size(), -1);

                // Initialize the top-level pointer
                level_ptrs[0] = addr; // -1 represents the base address

                for(int i = 0; i < total_elements; ++i) {
                    // 计算多维索引
                    vector<int> indices = GetMultiDimIndices(i, dimensions);

                    string current_ptr = addr;
                    for(size_t d = 0; d < dimensions.size() - 1; ++d) {
                        int idx = indices[d];
                        if(level_idx[d] != idx) {
                            // 需要生成 getelemptr 指令
                            string temp_ptr = AllocTempVar();
                            out << "  " << temp_ptr << " = getelemptr " << current_ptr << ", " << idx << "\n";
                            level_ptrs[d] = temp_ptr;
                            level_idx[d] = idx;
                        }
                        current_ptr = level_ptrs[d];
                    }

                    // Now, handle the last dimension
                    int last_dim = dimensions.size() - 1;
                    int last_idx = indices[last_dim];
                    string final_ptr = AllocTempVar();
                    out << "  " << final_ptr << " = getelemptr " << current_ptr << ", " << last_idx << "\n";

                    // 生成 store 指令
                    BaseAST* expr = flat_init[i];
                    expr->GenerateIR(out);
                    EvalResult res = expr->Eval();
                    if(res.success){
                        out << "  store " << res.value << ", " << final_ptr << "\n";
                    }
                    else{
                        out << "  store " << expr->ir_name << ", " << final_ptr << "\n";
                    }
                }
            }
        } 
    }
    else {
        if(is_global){
            string addr = "@GLOBAL_"+ ident;
            InsertVar(ident, addr);

            out << "global " << addr << " = alloc i32, ";
            if(init_val){
                init_val->GenerateIR(out);
                out << init_val->ir_name << "\n";
            } else {
                out << "zeroinit\n";
            }

        }else{
            string addr = "@SYMTAB_LEVEL_" + to_string(GetSymtabID()) + "_" + ident; // 防止命名冲突
            InsertVar(ident, addr);
            
            out << "  " << addr << " = alloc i32\n";
            if (init_val) {
                init_val->GenerateIR(out);
                out << "  store " << init_val->ir_name << ", " << addr << "\n";
            }
        }
    }
}

// Implement VarDeclAST
VarDeclAST::~VarDeclAST() {
    for (auto def : var_defs) {
        delete def;
    }
}

void VarDeclAST::GenerateIR(ostream& out) const {
    for (const auto& def : var_defs) {
        def->GenerateIR(out);
    }
}

void AssignStmtAST::GenerateIR(ostream& out) const {
    // 获取 LValAST 的指针
    LValAST* lval_ptr = dynamic_cast<LValAST*>(lval.get());
    if (!lval_ptr) {
        throw runtime_error("Left side of assignment is not a variable");
    }
    
    SymbolEntry* entry = LookupSymbol(lval_ptr->ident);
    if (!entry) {
        throw runtime_error("Undefined variable: " + lval_ptr->ident);
    }

    // 生成赋值表达式的 IR
    EvalResult exp_eval = exp->Eval();
    string exp_ir;
    if(!exp_eval.success){
        exp->GenerateIR(out);
        exp_ir = exp->ir_name;
    }
    else{
        exp_ir = to_string(exp_eval.value);
    }
    
    if(entry->type == SymbolType::VAR){
        out << "  store " << exp_ir << ", " << entry->var_addr << "\n";

    }
    else if(entry->type == SymbolType::ARRAY){
        string current_ptr = entry->var_addr;

        for (size_t i = 0; i < lval_ptr->index_exps.size(); ++i) {
            lval_ptr->index_exps[i]->GenerateIR(out);
            string index_ir = lval_ptr->index_exps[i]->ir_name;
            string ptr_temp = AllocTempVar();
            out << "  " << ptr_temp << " = getelemptr " << current_ptr << ", " << index_ir << "\n";
            current_ptr = ptr_temp;
        }

        out << "  store " << exp_ir << ", " << current_ptr << "\n";
    }
    else if(entry->type == SymbolType::PTR){
        string current_ptr = AllocTempVar();
        out << "  " << current_ptr << " = load " << entry->var_addr << "\n";
        
        for (size_t i = 0; i < lval_ptr->index_exps.size(); ++i) {
            lval_ptr->index_exps[i]->GenerateIR(out);
            string index_ir = lval_ptr->index_exps[i]->ir_name;
            string ptr_temp = AllocTempVar();
            if(i == 0){
                out << "  " << ptr_temp << " = getptr " << current_ptr << ", " << index_ir << "\n";
            }
            else{
                out << "  " << ptr_temp << " = getelemptr " << current_ptr << ", " << index_ir << "\n";
            }
            current_ptr = ptr_temp;
        }

        out << "  store " << exp_ir << ", " << current_ptr << "\n";
    }
    else{
        throw runtime_error("AssignStmtAST: Unsupported SymbolType");
    }
}


// Implement ReturnStmtAST
void ReturnStmtAST::GenerateIR(ostream& out) const {
    if(exp){
        EvalResult exp_eval = exp->Eval();
        if (exp_eval.success) {
            out << "  ret " << exp_eval.value << "\n";
        } else {
            exp->GenerateIR(out);
            out << "  ret " << exp->ir_name << "\n";
        }
    } else {
        out << "  ret\n";
    }
}

bool ReturnStmtAST::IsTerminated() const {
    return true;
}

void EmptyStmtAST::GenerateIR(ostream& out) const {
    // 空语句，无需生成 IR
}

void ExpStmtAST::GenerateIR(ostream& out) const {
    exp->GenerateIR(out);
    // 生成的值被丢弃，不需要存储
}

// ast.cpp

void IfStmtAST::GenerateIR(ostream& out) const {
    // 生成条件表达式的 IR
    cond->GenerateIR(out);
    string cond_ir = cond->ir_name;

    // 创建基本块标签
    string then_label = "%LABEL_THEN_" + to_string(label_if_id);
    string else_label = else_stmt ? ("%LABEL_ELSE_" + to_string(label_if_id)) : ("%LABEL_END_" + to_string(label_if_id));
    string end_label = "%LABEL_END_" + to_string(label_if_id);
    label_if_id++;

    // 生成条件跳转指令
    out << "  br " << cond_ir << ", " << then_label << ", " << else_label << "\n";

    // 生成 then 分支
    out << then_label << ":\n";
    then_stmt->GenerateIR(out);
    if (!then_stmt->IsTerminated()) {
        out << "  jump " << end_label << "\n";
    }

    // 生成 else 分支（如果有）
    if (else_stmt) {
        out << else_label << ":\n";
        else_stmt->GenerateIR(out);
        if (!else_stmt->IsTerminated()) {
            out << "  jump " << end_label << "\n";
        }
    }

    // 如果整个 if 语句不终止，则生成 end 标签
    if (!this->IsTerminated()) {
        out << end_label << ":\n";
    }
}

bool IfStmtAST::IsTerminated() const {
    if (else_stmt) {
        // 如果 then 和 else 分支都终止，则整个 if 语句终止
        return then_stmt->IsTerminated() && else_stmt->IsTerminated();
    }
    return false; // 如果没有 else 分支，或有一个分支不终止，则认为 if 语句不终止
}

void WhileStmtAST::GenerateIR(ostream& out) const {
    string cond_label = "%WHILE_COND_" + to_string(label_while_id);
    string body_label = "%WHILE_BODY_" + to_string(label_while_id);
    string end_label = "%WHILE_END_" + to_string(label_while_id);
    label_while_id++;
    loop_stack.emplace_back(cond_label, end_label);

    out << "  jump " << cond_label << "\n";
    out << cond_label << ":\n";
    cond->GenerateIR(out);
    string cond_ir = cond->ir_name;
    out << "  br " << cond_ir << ", " << body_label << ", " << end_label << "\n";

    out << body_label << ":\n";
    body->GenerateIR(out);

    if (!body->IsTerminated()) {
        out << "  jump " << cond_label << "\n";
    }

    out << end_label << ":\n";
    loop_stack.pop_back();
}

bool WhileStmtAST::IsTerminated() const {
    return false;
}

// Implement BreakStmtAST
void BreakStmtAST::GenerateIR(ostream& out) const {
    if (loop_stack.empty()) {
        throw runtime_error("Error: 'break' statement not within a loop");
    }
    string break_label = loop_stack.back().second;
    out << "  jump " << break_label << "\n";
}

bool BreakStmtAST::IsTerminated() const {
    return true; // 'break' 导致控制流转移，但是转移后仍在原Block内，因此应置为false
}

// Implement ContinueStmtAST
void ContinueStmtAST::GenerateIR(ostream& out) const {
    if (loop_stack.empty()) {
        throw runtime_error("Error: 'continue' statement not within a loop");
    }
    string continue_label = loop_stack.back().first;
    out << "  jump " << continue_label << "\n";
}

bool ContinueStmtAST::IsTerminated() const {
    return true; // 'continue' 导致控制流转移，但是转移后仍在原Block内，因此应置为false
}

FuncCallAST::~FuncCallAST() {
    for (auto param : params) {
        delete param;
    }
}
