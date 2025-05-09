#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ast_class.h"
#include "symbol_table.h"
#include "ir_program.h"
#define _DEBUG_sym
#define _DEBUG_ir

/*********** 小组件 **************/
struct while_cell {
    std::string continue_to;
    std::string break_to;
    while_cell(std::string c, std::string b) : continue_to(c), break_to(b) {}
};
std::vector<while_cell> while_stack;

int temp_var_counter = 0;

#ifdef DEBUG_ir
std::vector<std::string> stmt_type = {"If_Then", "If_Then_Else", "Return_Exp", "Assign", "Assign_Array", "Block", "Exp", "Empty", "Return_Void", "While", "Break", "Continue"};
std::vector<std::string> exp_op = {"ADD", "SUB", "MUL", "DIV", "MOD", "LAND", "LOR", "LT", "GT", "LE", "GE", "EQ", "NE", "PRI", "POS", "NEG", "NOT"};
std::vector<std::string> primaryexp_type = {"LVal", "INT_CONST", "FuncCall"};
std::vector<std::string> Value_type = {"LVal", "ConstIdent", "VarIdent"};
#endif
/*********** 工具函数部分 **************/
inline std::vector<int> cump(vector<int> vec)
{
    // 计算每个子矩阵的总大小
    std::vector<int> ret;
    for(int i=0; i<vec.size(); i++)
        ret.push_back(0);
    int i = vec.size()-1;
    ret[i] = vec[i];
    for(i--;i>=0;i--)
    {
        ret[i] = ret[i+1] * vec[i];
    }
    ret.push_back(1);
    return ret;
}

std::vector<int> init_array_dfs(const std::vector<InitvalAST*>& init_vec, int top_dim, std::vector<int> &child_dims)
{
    std::vector<int> dfs_ret_vec;
    for(InitvalAST* init : init_vec)
    {
        if(init->exp){
            dfs_ret_vec.push_back(init->exp->calc());
        } else {
            int temp_l = dfs_ret_vec.size();
            int temp_dim_top = child_dims.size()-1;
            for(;temp_dim_top>=0; temp_dim_top--)
            {
                if (temp_l % child_dims[temp_dim_top] != 0)
                    break;
            }
            if (temp_dim_top<0) temp_dim_top=0;
            std::vector<int> new_vec = init_array_dfs(init->inits, temp_dim_top+1, child_dims);
            dfs_ret_vec.insert(dfs_ret_vec.end(), new_vec.begin(), new_vec.end());
        }
    }
    assert(dfs_ret_vec.size()<=child_dims[top_dim]);
    for(int i = dfs_ret_vec.size(); i<child_dims[top_dim]; i++)
        dfs_ret_vec.push_back(0);
    return dfs_ret_vec;
}

void init_array_str(vector<int>& dims, int dim_iter,std::vector<int>::const_iterator &val_iter, std::string &str)
{
    if((dim_iter)==dims.size()){
        str.append(std::to_string(*(val_iter++)));
    }else{
        init_array_str(dims, dim_iter+1, val_iter, str.append("{"));
        for(int i=1; i<dims[dim_iter]; i++){
            str.append(",");
            init_array_str(dims, dim_iter+1, val_iter, str);
        }
        str.append("}");
    }   
}

inline std::string get_ir_name(std::string name,const vector<int>& key)
{
    std::string ret = "@_";
    for(auto i : key) {
        ret.append(std::to_string(i));
        ret.append("_");
    }
    return ret.append(name);
}

inline std::string get_type(Symbol *sym)
{
    if (sym->type_an==SymbolType_AN::NUMBER)
        return "i32";
    else {
        std::string ret = "i32";
        for (auto it = sym->dims.rbegin(); it != sym->dims.rend(); it++)
        {
            if (*it == -1) ret = "*" + ret;
            else ret = "[" + ret + ", " + std::to_string(*it) + "]";
        }
        return ret;
    }
}

inline void manage_If_Then(std::string b, std::string b_true, std::string s_next, ir_block *if_block, ir_block *then_block)
{
    if_block->push_stmt("br " + b + ", " + b_true + ", " + s_next + "\n");
    then_block->push_stmt("jump "+s_next+"\n");
}

inline void manage_If_Else(std::string b, std::string b_true, std::string b_false, std::string s_next, ir_block *if_block, 
ir_block *then_block, ir_block *else_block) {
    if_block->push_stmt("br " + b + ", " + b_true + ", " + b_false + "\n");
    then_block->push_stmt("jump " + s_next + "\n");
    else_block->push_stmt("jump " + s_next + "\n");
}

inline void manage_While(std::string b, std::string b_true, std::string b_false, std::string s_next, ir_block *outside_block, 
ir_block *cond_block, ir_block *main_block) {
    outside_block->push_stmt("jump " + s_next + "\n");
    cond_block->push_stmt("br " + b + ", " + b_true + ", " + b_false + "\n");
    main_block->push_stmt("jump " + s_next + "\n");
}

/*********** 构造函数、析构函数部分 **************/

CompUnitAST::CompUnitAST(){class_name = "CompUnitAST";};
CompUnitAST::~CompUnitAST(){
    for (auto decl : decls) delete decl;
    for (auto func : funcs) delete func;
};
FuncDefAST::FuncDefAST(){class_name = "FuncDefAST";};
FuncDefAST::~FuncDefAST(){
    if(block) delete block;
}    
BlockAST::BlockAST(){class_name = "BlockAST";};
BlockAST::~BlockAST(){}
BlockItemAST::BlockItemAST(){class_name = "BlockItemAST";};
BlockItemAST::~BlockItemAST(){
    if(decl) delete decl;
    if(stmt) delete stmt;
}
StmtAST::StmtAST(){class_name = "StmtAST";};
StmtAST::~StmtAST(){
    if(lval) delete lval;
    if(block && type==Stmt_type::Block) delete block;
    if(stmt1) delete stmt1;
    if(stmt2) delete stmt2;
}
PrimaryExpAST::PrimaryExpAST(){class_name = "PrimaryExpAST";};
PrimaryExpAST::~PrimaryExpAST(){
    if(params) delete params;
    if(lval) delete lval;
    if(cache) delete cache;
}
ExpAST::ExpAST(){class_name = "ExpAST";};
ExpAST::~ExpAST(){
    // if(left) delete left;
    // if(right) delete right;
    // if(primary) delete primary;
    if(cache) delete cache;
}
DeclAST::DeclAST(){class_name = "DeclAST";};
DeclAST::~DeclAST(){
    for (auto def : defs) delete def;
}
DefAST::DefAST(){class_name = "DefAST";};
DefAST::~DefAST(){
    if(value) delete value;
    if(initval) delete initval;
    if(init_stmt) delete init_stmt;
}
FuncFParamsAST::FuncFParamsAST(){class_name = "FuncFParamsAST";};
FuncFParamsAST::~FuncFParamsAST(){
    for (auto param : params) delete param;
}
FuncFParamAST::FuncFParamAST(){class_name = "FuncFParamAST";};
FuncFParamAST::~FuncFParamAST(){};
FuncRParamsAST::FuncRParamsAST(){class_name = "FuncRParamsAST";};
FuncRParamsAST::~FuncRParamsAST(){
    for (auto exp : exps) delete exp;
}
ValueAST::ValueAST(){class_name = "ValueAST";};
ValueAST::~ValueAST(){
    for (auto exp : exps) delete exp;
}
InitvalAST::InitvalAST(){class_name = "InitvalAST";};
InitvalAST::~InitvalAST(){
    if(exp) delete exp;
    for (auto init : inits) delete init;
}

/*********** 计算部分 **************/
int ExpAST::calc(){
    if (cache) return *cache;
    cache = new int(0);
    int left_val, right_val;

    switch(op){
    case Exp_op::PRI:
        *cache = primary->calc();
        break;
    case Exp_op::POS:
        *cache = right->calc();
        break;
    case Exp_op::NEG:
        *cache = -right->calc();
        break;
    case Exp_op::NOT:
        *cache = !right->calc();
        break;
    default:
        left_val = left->calc();
        right_val = right->calc();
        switch(op){
        case Exp_op::ADD:
            *cache = left_val + right_val;
            break;
        case Exp_op::SUB:
            *cache = left_val - right_val;
            break;
        case Exp_op::MUL:
            *cache = left_val * right_val;
            break;
        case Exp_op::DIV:
            *cache = left_val / right_val;
            break;
        case Exp_op::MOD:
            *cache = left_val % right_val;
            break;
        case Exp_op::LAND:
            *cache = left_val && right_val;
            break;
        case Exp_op::LOR:
            *cache = left_val || right_val;
            break;
        case Exp_op::EQ:
            *cache = left_val == right_val;
            break;
        case Exp_op::NE:
            *cache = left_val != right_val;
            break;
        case Exp_op::GE:
            *cache = left_val >= right_val;
            break;
        case Exp_op::GT:
            *cache = left_val > right_val;
            break;
        case Exp_op::LE:
            *cache = left_val <= right_val;
            break;
        case Exp_op::LT:
            *cache = left_val < right_val;
            break;
        default:;
        }
    }
    if (left) delete left;
    if (right) delete right;
    if (primary) delete primary;
    return *cache;
}

int PrimaryExpAST::calc(){
    switch (type){
    case PrimaryExp_type::INT_CONST:
        return const_val;
    case PrimaryExp_type::LVal:
        if (!lval->symbol)
            lval->setSymbolTable();
        return lval->symbol->value;
    default:
        return 0;
    }
}

/*********** 符号表部分 **************/
void CompUnitAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << "_____ log of symbol table _____" << endl;
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    for (auto decl : decls)
        decl->setSymbolTable();
    decls.clear();
    for (auto func : funcs)
        if (func->func_type=="") table.void_func_name.insert(func->ident);
    for (auto func : funcs)
        func->setSymbolTable();
}

void FuncDefAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    params->setSymbolTable();
    block->setSymbolTable();
}

void BlockAST::setSymbolTable(){ // 将blockitem的信息聚合到block里面，进一步简化树
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    table.enterScope();
    for (BlockItemAST* item : items) {
        item->setSymbolTable();
        switch(item->type){
        case BlockItem_type::Decl:
            for (DefAST *def : item->decl->defs)
            {
                if (def->init_stmt != nullptr)
                {
                    def->init_stmt->setSymbolTable();
                    stmts.push_back(def->init_stmt);
                }
            }
            break;
        case BlockItem_type::Stmt:
            item->stmt->setSymbolTable();
            stmts.push_back(item->stmt);
            break;
        }
        // delete item;
    }
    table.exitScope();
}

void BlockItemAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    if (type == BlockItem_type::Decl)
        decl->setSymbolTable();
}

void StmtAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    switch(type){
    case Stmt_type::Block:
        block->setSymbolTable();
        break;
    case Stmt_type::Exp:
        exp->setSymbolTable();
        break;
    case Stmt_type::While:
        exp->setSymbolTable();
        stmt1->setSymbolTable();
        break;
    case Stmt_type::If_Then_Else:
        exp->setSymbolTable();
        stmt1->setSymbolTable();
        stmt2->setSymbolTable();
        break;
    case Stmt_type::If_Then:
        exp->setSymbolTable();
        stmt1->setSymbolTable();
        break;
    case Stmt_type::Assign:
        lval->setSymbolTable();
        exp->setSymbolTable();
        break;
    case Stmt_type::Assign_Array:
        lval->setSymbolTable();
        break;
    case Stmt_type::Return_Exp:
        exp->setSymbolTable();
        break;
    case Stmt_type::Return_Void:
    case Stmt_type::Empty:
    case Stmt_type::Break:
    case Stmt_type::Continue:
    default:
        break;
    }
}

void ExpAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif 
    switch(op){
    case Exp_op::PRI:
        primary->setSymbolTable();
        break;
    default:
        if (left!=nullptr) left->setSymbolTable();
        if (right!=nullptr) right->setSymbolTable();
    }
}

void PrimaryExpAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    switch (type)
    {
    case PrimaryExp_type::LVal:
        lval->setSymbolTable();
        break;
    case PrimaryExp_type::FuncCall:
        params->setSymbolTable();
        break;
    default:
        break;
    }
}

void DeclAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    for (auto def : defs)
        def->setSymbolTable();
}

std::vector<int> ValueAST::getArrayDim(){
    std::vector<int> dims;
    if(empty_array_init) dims.push_back(-1);
    for (ExpAST* exp : exps)
        dims.push_back(exp->calc());
    return dims;
}

std::vector<int> InitvalAST::getInitVector(vector<int> dims){
    std::vector<int> ret_vec;
    std::vector<int> child_dims = cump(dims);

    if(exp){
        ret_vec.push_back(exp->calc());
        for(int i=1; i<child_dims[0]; i++)
            ret_vec.push_back(0);
    } else {
        ret_vec = init_array_dfs(inits, 0, child_dims);
    }
    if (ret_vec.size()<child_dims[0]){
        for(int i=ret_vec.size(); i<child_dims[0]; i++)
            ret_vec.push_back(0);
    }else if(ret_vec.size()>child_dims[0]){
        for(int i=child_dims[0]; i<ret_vec.size(); i++)
            ret_vec.pop_back();
    }
    return ret_vec;
}

void DefAST::construct_init_stmt(ExpAST *exp){
    init_stmt = new StmtAST();
    init_stmt->lval = value;
    init_stmt->exp = exp;
    init_stmt->type = Stmt_type::Assign;
    return;
}

void DefAST::construct_init_stmt(vector<int> value_vec){
    init_stmt = new StmtAST();
    init_stmt->lval = value;
    init_stmt->initvalArray = value_vec;
    init_stmt->type = Stmt_type::Assign_Array;
    return;
}

void DefAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    std::vector<int> dims = value->getArrayDim();
    if (table.key.empty())
    {//全局变量
        if (dims.empty()){
            int temp_value = initval==nullptr?0:initval->exp->calc();
            SymbolType_CVP temp_type = type==Def_type::Const?SymbolType_CVP::CONSTANT:SymbolType_CVP::VARIABLE;
            table.insertNumber(value->ident, temp_type, temp_value);
        }else{
            std::vector<int> temp_array_value = initval==nullptr?vector<int>():initval->getInitVector(dims);
            SymbolType_CVP temp_type = type==Def_type::Const?SymbolType_CVP::CONSTANT:SymbolType_CVP::VARIABLE;
            table.insertArray(value->ident, temp_type, dims, temp_array_value);
        }
    }else if (type == Def_type::Const){
        // 局部常量
        if (dims.empty()){
            int temp_value = initval->exp->calc();
            table.insertNumber(value->ident, SymbolType_CVP::CONSTANT, temp_value);
        }else{
            std::vector<int> temp_array_value = initval->getInitVector(dims);
            table.insertArray(value->ident, SymbolType_CVP::CONSTANT, dims, temp_array_value);
       }
    }else{
        //局部变量
        if (dims.empty()) {
            if (initval!=nullptr) construct_init_stmt(initval->exp);
            table.insertNumber(value->ident, SymbolType_CVP::VARIABLE);
        }else{
            // for (auto exp : value->exps) delete exp;
            value->exps.clear();
            if (initval!=nullptr) construct_init_stmt(initval->getInitVector(dims));
            table.insertArray(value->ident, SymbolType_CVP::VARIABLE, dims);
        }
    }
}

void FuncFParamsAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    table.enterScope();
    for(auto param : params)
        param->setSymbolTable();
    table.dont_exitScope();
}

void FuncFParamAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    std::vector<int> dims = value->getArrayDim();
    if (dims.empty())
        table.insertNumber(value->ident, SymbolType_CVP::PRARM);
    else
        table.insertArray(value->ident, SymbolType_CVP::PRARM, dims);
    value->symbol = table.findSymbol(value->ident);
}

void FuncRParamsAST::setSymbolTable() {
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    for (auto exp : exps)
        exp->setSymbolTable();
}

void ValueAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    symbol = table.findSymbol(ident);
}

void InitvalAST::setSymbolTable(){
    #ifdef DEBUG_sym
    cout << class_name << endl;
    #endif
}

/*********** 中间代码生成部分 **************/

void CompUnitAST::dump(std::string &code)
{
    ir_program &prog = ir_program::getInstance();
    code = prog.dump();
}
void CompUnitAST::generate()
{
    #ifdef DEBUG_ir
    cout << "_____ log of ir generate _____" << endl;
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    table.reset();
    ir_program& prog = ir_program::getInstance();
    vector<int> key = {};
    LocalSymbolTable *global = table.symMap[key];
    for (auto sym : global->symbolMap)
    {
        // 在一开始应该只有全局变量的赋值，所以不用考虑符号表中的Params
        if (sym.second->type_cv==SymbolType_CVP::VARIABLE 
        && sym.second->type_an==SymbolType_AN::NUMBER) {
            prog.push_global_stmt("global " + sym.second->ir_name + " = alloc i32, " 
            + std::to_string(sym.second->value) + "\n");
        }else if(sym.second->type_an==SymbolType_AN::ARRAY && !sym.second->array_value.empty()){
            std::vector<int> ::const_iterator val_iter = sym.second->array_value.begin();
            std::string str = "global " + sym.second->ir_name + " = alloc "+ get_type(sym.second) +", ";
            init_array_str(sym.second->dims,0,val_iter,str);
            prog.push_global_stmt(str + "\n");
        }else if(sym.second->type_an==SymbolType_AN::ARRAY && sym.second->array_value.empty()){
            prog.push_global_stmt("global " + sym.second->ir_name + " = alloc "+ get_type(sym.second) +", zeroinit\n");
        }
    }
    for (auto lsymtb : table.symMap)
    {
        if (lsymtb.first.empty()) continue;
        for(auto sym : lsymtb.second->symbolMap)
        {
            if (sym.second->type_cv==SymbolType_CVP::CONSTANT 
            && sym.second->type_an==SymbolType_AN::ARRAY && !sym.second->array_value.empty()) {
                if (sym.second->array_value.empty()) prog.push_global_stmt("global " + sym.second->ir_name + " = alloc "+ get_type(sym.second) +", zeroinit\n");
                else {
                    std::vector<int> ::const_iterator val_iter = sym.second->array_value.begin();
                    std::string str = "global " + sym.second->ir_name + " = alloc "+ get_type(sym.second) +", ";
                    init_array_str(sym.second->dims,0,val_iter,str);
                    prog.push_global_stmt(str + "\n");
                }
            }
        }
    }
    for(auto func : funcs)
        func->generate();
}

void FuncDefAST::generate()
{
    #ifdef DEBUG_ir
    cout << class_name << " " << ident << endl;
    #endif
    ir_program& prog = ir_program::getInstance();
    prog.push_function(ident, params->get_params_ir(), func_type);
    
    block->generate();
    prog.cur_func->cur_block->push_stmt(func_type=="i32" ? "ret 0\n" : "ret\n");
}

void BlockAST::generate()
{
    #ifdef DEBUG_ir
    cout << class_name << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    ir_program& prog = ir_program::getInstance();
    table.enterScope();
    const LocalSymbolTable *local = table.symMap[table.key];
    
    // if (table.key.size()==1)
    
    {
        // 在每个block开头初始化好 函数参数 和 局部数组
        for (auto sym : local->symbolMap) {
            if (sym.second->type_cv==SymbolType_CVP::PRARM)
            {
                prog.cur_func->cur_block->push_stmt(
                    sym.second->ir_name + " = alloc " + get_type(sym.second) + "\n");
                prog.cur_func->cur_block->push_stmt(
                    "store " + sym.second->ir_name + "P, " + sym.second->ir_name + "\n");
            }else if (sym.second->type_cv==SymbolType_CVP::VARIABLE && sym.second->type_an==SymbolType_AN::ARRAY) {
                prog.cur_func->cur_block->push_stmt(
                    sym.second->ir_name + " = alloc " + get_type(sym.second) + "\n");
            }
        }
    }
    for (auto sym : local->symbolMap) {
        if (sym.second->type_cv==SymbolType_CVP::VARIABLE && sym.second->type_an==SymbolType_AN::NUMBER)
            prog.cur_func->cur_block->push_stmt(
                sym.second->ir_name + " = alloc i32\n");
    }
    for (auto stmt : stmts)
        stmt->generate();
    table.exitScope();
}

void StmtAST::generate()
{
    #ifdef DEBUG_ir
    cout << class_name << " type: " << stmt_type[int(type)] << endl;
    #endif
    ir_program& prog = ir_program::getInstance();
    std::string b,then_name,else_name,next_name;
    ir_block *if_block, *then_block, *else_block, *next_block;
    vector<int>::const_iterator it;
    switch (type)
    {
        case Stmt_type::If_Then:
            b = exp->get_value_ir();
            if_block = prog.cur_func->cur_block;
            then_name = prog.cur_func->push_block();
            stmt1->generate();
            then_block = prog.cur_func->cur_block;
            next_name = prog.cur_func->push_block();
            manage_If_Then(b, then_name, next_name, if_block, then_block);
            break;
        case Stmt_type::If_Then_Else:
            b = exp->get_value_ir();
            if_block = prog.cur_func->cur_block;
            then_name = prog.cur_func->push_block();
            stmt1->generate();
            then_block = prog.cur_func->cur_block;
            else_name = prog.cur_func->push_block();
            stmt2->generate();
            else_block = prog.cur_func->cur_block;
            next_name = prog.cur_func->push_block();
            manage_If_Else(b, then_name, else_name, next_name, if_block, then_block, else_block);
            break;
        case Stmt_type::While:
            if_block = prog.cur_func->cur_block;
            next_name = prog.cur_func->push_block();
            b = exp->get_value_ir();
            next_block = prog.cur_func->cur_block;  
            then_name = prog.cur_func->push_block();

            else_block = new ir_block();
            else_name = else_block->name;
            while_stack.push_back(while_cell(next_name,else_name));
            stmt1->generate();
            while_stack.pop_back();
            
            then_block = prog.cur_func->cur_block;
            manage_While(b,then_name, else_name, next_name, if_block, next_block, then_block);
            prog.cur_func->cur_block = else_block;
            prog.cur_func->blocks.push_back(else_block);
            break;
        case Stmt_type::Assign:
            b = exp->get_value_ir();
            prog.cur_func->cur_block->push_stmt("store " + b + ", " + lval->get_addr_ir() + "\n");
            break;
        case Stmt_type::Assign_Array:
            if(initvalArray.empty()) {
                prog.cur_func->cur_block->push_stmt("store zeroinit, " + lval->symbol->ir_name + "\n");
                break;
            }
            it = initvalArray.begin();
            b = "store ";
            init_array_str(lval->symbol->dims,0,it,b);
            b = b + ", " + lval->symbol->ir_name + "\n";
            prog.cur_func->cur_block->push_stmt(b);
            break;
        case Stmt_type::Block:
            block->generate();
            break;
        case Stmt_type::Exp:
            exp->get_value_ir();
            break;
        case Stmt_type::Return_Exp:
            b = exp->get_value_ir();
            prog.cur_func->cur_block->push_stmt("ret " + b + "\n");
            prog.cur_func->push_block();
            break;
        case Stmt_type::Return_Void:
            prog.cur_func->cur_block->push_stmt("ret\n");
            prog.cur_func->push_block();
            break;
        case Stmt_type::Break:
            prog.cur_func->cur_block->push_stmt("jump " + while_stack.back().break_to + "\n");
            prog.cur_func->push_block();
            break;
        case Stmt_type::Continue:
            prog.cur_func->cur_block->push_stmt("jump " + while_stack.back().continue_to + "\n");
            prog.cur_func->push_block();
            break;
        case Stmt_type::Empty:
            break;
    }

    
}
// PrimaryExpAST ExpAST ValueAST 都视作一种特殊的“值”，为抽象语义服务
std::string PrimaryExpAST::get_value_ir()
{
    #ifdef DEBUG_ir
    cout << class_name << " type: " << primaryexp_type[int(type)] << endl;
    #endif
    SymbolTable& table = SymbolTable::getInstance();
    ir_program& prog = ir_program::getInstance();
    std::string ret;
    std::string params_ret = "";
    switch (type)
    {
        case PrimaryExp_type::LVal:
            return lval->get_value_ir();
        case PrimaryExp_type::INT_CONST:
            return std::to_string(const_val);
        case PrimaryExp_type::FuncCall:
            ret = "%T" + std::to_string(temp_var_counter++);
            params_ret = params->get_params_ir();
            if (table.void_func_name.find(func_name)!=table.void_func_name.end())
                prog.cur_func->cur_block->push_stmt("call @" + func_name + "(" + params_ret + ")\n");
            else
                prog.cur_func->cur_block->push_stmt(ret + " = call @" + func_name + "(" + params_ret + ")\n");
            return ret;
    }
}

std::string ExpAST::get_value_ir()
{
    #ifdef DEBUG_ir
    cout << class_name << " type: " << exp_op[int(op)] << endl;
    #endif
    ir_program& prog = ir_program::getInstance();
    std::string t_left, t_right, temp, then_name, next_name;
    ir_block *if_block, *then_block;

    switch(op){
    case Exp_op::PRI:
        return primary->get_value_ir();
    case Exp_op::LAND:
        t_left = left->get_value_ir();
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = alloc i32\n");
        prog.cur_func->cur_block->push_stmt("store 0, %T" + std::to_string(temp_var_counter-1) + "\n");
        temp = "%T" + std::to_string(temp_var_counter-1);
        
        if_block = prog.cur_func->cur_block;
        then_name = prog.cur_func->push_block();
        t_right = right->get_value_ir();
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = ne 0, " + t_right + "\n");
        prog.cur_func->cur_block->push_stmt("store %T" + std::to_string(temp_var_counter-1) + ", " + temp + "\n");

        then_block = prog.cur_func->cur_block;
        next_name = prog.cur_func->push_block();
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = load " + temp + "\n");

        manage_If_Then(t_left, then_name, next_name, if_block, then_block);
        return "%T" + std::to_string(temp_var_counter-1);

    case Exp_op::LOR:
        t_left = left->get_value_ir();
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = eq 0, " + t_left + "\n");
        t_left = "%T" + std::to_string(temp_var_counter-1);
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = alloc i32\n");
        prog.cur_func->cur_block->push_stmt("store 1, %T" + std::to_string(temp_var_counter-1) + "\n");
        temp = "%T" + std::to_string(temp_var_counter-1);

        if_block = prog.cur_func->cur_block;
        then_name = prog.cur_func->push_block();
        t_right = right->get_value_ir();
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = ne 0, " + t_right + "\n");
        prog.cur_func->cur_block->push_stmt("store %T" + std::to_string(temp_var_counter-1) + ", " + temp + "\n");

        then_block = prog.cur_func->cur_block;
        next_name = prog.cur_func->push_block();
        prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = load " + temp + "\n");

        manage_If_Then(t_left, then_name, next_name, if_block, then_block);
        return "%T" + std::to_string(temp_var_counter-1);

    default:
        if (left) t_left = left->get_value_ir();
        if (right) t_right = right->get_value_ir();
        switch(op) {
            case Exp_op::POS:
                return t_right;
            case Exp_op::NEG:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = sub 0, " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::NOT:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = eq 0, " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::ADD:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = add " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::SUB:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = sub " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::MUL:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = mul " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::DIV:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = div " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::MOD:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = mod " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::LT:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = lt " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::GT:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = gt " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::LE:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = le " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::GE:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = ge " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::EQ:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = eq " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            case Exp_op::NE:
                prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = ne " + t_left + ", " + t_right + "\n");
                return "%T"+std::to_string(temp_var_counter-1);
            default:
                assert(false);
        }
    }
}

std::string ValueAST::get_value_ir()
{
    #ifdef DEBUG_ir
    cout << class_name << " type: " << Value_type[int(type)] << endl;
    #endif
    if (!symbol) symbol = SymbolTable::getInstance().findSymbol(ident);
    ir_program& prog = ir_program::getInstance();
    if(symbol->type_an==SymbolType_AN::NUMBER)
    {
        if (symbol->type_cv==SymbolType_CVP::CONSTANT) return std::to_string(symbol->value);
        else { //可能是var 也可能是 param
            prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + 
            " = load " + symbol->ir_name + "\n");
            return "%T"+std::to_string(temp_var_counter-1);
        }
    }else {
        std::string array;
        std::string temp;
        std::string exp_ret;
        if (symbol->type_cv==SymbolType_CVP::PRARM) {
            array = "%T"+std::to_string(temp_var_counter++);
            prog.cur_func->cur_block->push_stmt(array + " = load " + symbol->ir_name + "\n");
            if (!exps.empty()) {

                temp = "%T"+std::to_string(temp_var_counter++);
                exp_ret = exps[0]->get_value_ir();
                prog.cur_func->cur_block->push_stmt(temp + " = getptr " + array + ", " + exp_ret + "\n");
                array = temp;
                for(int i=1;i<exps.size();i++)
                {
                    temp = "%T"+std::to_string(temp_var_counter++);
                    exp_ret = exps[i]->get_value_ir();
                    prog.cur_func->cur_block->push_stmt(temp + " = getelemptr " + array + ", " + exp_ret + "\n");
                    array = temp;
                }
            } else {
                return array;
            }
        } else {
            array = symbol->ir_name;
            for (auto exp : exps) {
                temp = "%T"+std::to_string(temp_var_counter++);
                exp_ret = exp->get_value_ir();
                prog.cur_func->cur_block->push_stmt(temp + " = getelemptr " + array + ", " + exp_ret + "\n");
                array = temp;
            }
        }
        if (exps.size()==symbol->dims.size())
            prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = load " + array + "\n");
        else
            prog.cur_func->cur_block->push_stmt("%T" + std::to_string(temp_var_counter++) + " = getelemptr " + array + ", 0\n");
        return "%T"+std::to_string(temp_var_counter-1);
    }
}

std::string ValueAST::get_addr_ir()
{
    #ifdef DEBUG_ir
    cout << class_name << " type: " << Value_type[int(type)] << endl;
    #endif
    if (!symbol) symbol = SymbolTable::getInstance().findSymbol(ident);
    ir_program& prog = ir_program::getInstance();
    if(symbol->type_an==SymbolType_AN::NUMBER)
        return symbol->ir_name;

    std::string array;
    std::string temp;
    std::string exp_ret;
    if (symbol->type_cv==SymbolType_CVP::PRARM) {
        array = "%T"+std::to_string(temp_var_counter++);
        prog.cur_func->cur_block->push_stmt(array + " = load " + symbol->ir_name + "\n");
        if (!exps.empty()) {

            temp = "%T"+std::to_string(temp_var_counter++);
            exp_ret = exps[0]->get_value_ir();
            prog.cur_func->cur_block->push_stmt(temp + " = getptr " + array + ", " + exp_ret + "\n");
            array = temp;
            for(int i=1;i<exps.size();i++)
            {
                temp = "%T"+std::to_string(temp_var_counter++);
                exp_ret = exps[i]->get_value_ir();
                prog.cur_func->cur_block->push_stmt(temp + " = getelemptr " + array + ", " + exp_ret + "\n");
                array = temp;
            }
        }
    } else {
        array = symbol->ir_name;
        for (auto exp : exps) {
            temp = "%T"+std::to_string(temp_var_counter++);
            exp_ret = exp->get_value_ir();
            prog.cur_func->cur_block->push_stmt(temp + " = getelemptr " + array + ", " + exp_ret + "\n");
            array = temp;
        }
    }    
    return array;
}

std::string FuncFParamsAST::get_params_ir() {
    #ifdef DEBUG_ir
    cout << class_name << endl;
    #endif
    std::string ret = "";
    for (auto param : params)
        ret += param->value->symbol->ir_name + "P: " + get_type(param->value->symbol) + ", ";
    if (ret.empty()) return ret;
    ret.pop_back();
    ret.pop_back();
    return ret;
}

std::string FuncRParamsAST::get_params_ir()
{
    #ifdef DEBUG_ir
    cout << class_name << endl;
    #endif
    std::string ret = "";
    std::string exp_ret = "";
    for (auto exp : exps) {
        exp_ret = exp->get_value_ir();
        ret += exp_ret + ", ";
    }
    if (ret.empty()) return ret;
    ret.pop_back();
    ret.pop_back();
    return ret;
}

void BlockItemAST::generate(){}
void ExpAST::generate() {}
void PrimaryExpAST::generate() {}
void DeclAST::generate() {}
void DefAST::generate() {}
void FuncFParamsAST::generate() {}
void FuncFParamAST::generate() {}
void FuncRParamsAST::generate() {}
void ValueAST::generate() {}
void InitvalAST::generate() {}

