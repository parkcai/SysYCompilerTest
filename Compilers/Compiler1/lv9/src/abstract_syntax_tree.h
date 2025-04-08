#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include "utils.h"
#include "symbol_table.h"
#include "funcparam_note.h"
using namespace std;

extern int current_max_register;
extern int current_if_index;
extern int current_while_index;
extern int current_or_index;
extern int current_and_index;
extern int next_while_index;
extern int next_if_index;
extern int next_or_index;
extern int next_and_index;
extern int lval_called_from_rhs;
extern int block_called_from_func_def;
extern string current_func_type;
extern int may_using_func_res;
extern int is_last_element;
extern int in_main_stream;
extern int main_stream_returned;
extern int getint_overridden;
extern int getch_overridden;
extern int getarray_overridden;
extern int putint_overridden;
extern int putch_overridden;
extern int putarray_overridden;
extern int starttime_overridden;
extern int stoptime_overridden;
extern int array_total_length;
extern string array_stack_position;
extern int dealing_with_globals;
extern string currently_dealt_btype;
extern int inside_array_init;
extern map<string, vector<int>> array_dimension_note;
extern int OR_HEAD;
extern int AND_HEAD;

// 这个映射用于记录数组（对应的stack position）到底是一个数组指针（本来定义的）还是一个整数指针（传进来的）
// 默认是前者，传进来时标记为后者
extern unordered_map<string, int> is_int_ptr;

inline string get_if_then_label(){
    assert(current_if_index >= 1);
    return "%if" + to_string(current_if_index) + "_then_label";
}

inline string get_if_else_label(){
    return "%if" + to_string(current_if_index) + "_else_label";
}

inline string get_if_end_label(){
    return "%if" + to_string(current_if_index) + "_end_label";
}

inline string get_while_entry_label(){
    assert(current_while_index >= 1);
    return "%while" + to_string(current_while_index) + "_entry_label";
}

inline string get_while_body_label(){
    return "%while" + to_string(current_while_index) + "_body_label";
}

inline string get_while_end_label(){
    return "%while" + to_string(current_while_index) + "_end_label";
}

inline string get_or_skip_label(){
    assert(current_or_index >= 1);
    return "%or" + to_string(current_or_index) + "_skip_label";
}

inline string get_or_nonskip_label(){
    return "%or" + to_string(current_or_index) + "_nonskip_label";
}

inline string get_or_end_label(){
    return "%or" + to_string(current_or_index) + "_end_label";
}

inline string get_and_skip_label(){
    assert(current_and_index >= 1);
    return "%and" + to_string(current_and_index) + "_skip_label";
}

inline string get_and_nonskip_label(){
    return "%and" + to_string(current_and_index) + "_nonskip_label";
}

inline string get_and_end_label(){
    return "%and" + to_string(current_and_index) + "_end_label";
}

// 所有 AST 的基类
class BaseAST {
    public:
        virtual ~BaseAST() = default;

        virtual void Dump() const {

        }

        virtual void SaveKoopa(string& dest_str, const string save_mode) const {
            
        }

        // 将常量和其值添加到符号表中
        // 将变量添加到符号表中，其值（代表koopa_register）为-1
        virtual int preprocess_variables() {
            return 0;
        }
        virtual void koopa_allocate(){
            
        }
        virtual int get_koopa_value() const {
            return koopa_value;
        }
        virtual int get_koopa_status(){
            return koopa_status;
        }
        virtual int get_first_flow_index(){
            return 114514;
        }
        virtual int get_has_index_flag(){
            return 114514;
        }
        virtual string get_koopa_name() const {
            return "2200011363";
        }
        virtual vector<shared_ptr<BaseAST>>& get_blockitems(){
            static std::vector<std::shared_ptr<BaseAST>> items; 
            return items; 
        }
        virtual string get_ident() {
            return "2200011363";
        }
        virtual string get_unary_op(){
            return "2200011363";
        }

        virtual bool check_control_flow(){
            return false;
        }

        virtual void set_stack_position(string position) {
        
        }

        virtual string get_stack_position() {
            return "114514_1";
        }

        virtual string get_btype() const {
            return "114514_b";
        }

        virtual void SaveKoopa_alloc(string& dest_str, const string save_mode) const {
            
        }

        virtual void SaveKoopa_store(string& dest_str, const string save_mode) const {
            
        }

        virtual void SaveKoopa_load(string& dest_str, const string save_mode){
            
        }

        virtual void SaveKoopa_parainit(string& dest_str, const string save_mode) const {
            
        }

        virtual void SaveKoopa_global(string& dest_str, const string save_mode) const {
            
        }

        virtual void scan_overrides() const {
            
        }

        virtual vector<int> initval_parse(vector<int> array_shape) {
            int total_length = 1;
            for (size_t i = 0; i < array_shape.size(); i += 1) {
                total_length *= array_shape[i];
            }
            vector<int> padded_init_list;
            for (size_t i = 0; i < total_length; i += 1) {
                padded_init_list.push_back(0);
            }
            return padded_init_list;
        }

        virtual int get_constexp_flag() {
            return 1919810;
        }

        virtual int get_exp_flag() {
            return 1919810;
        }

        // koopa_status为1表示节点为立即数，koopa_value为其值
        // koopa_status为0表示节点为计算式，koopa_value为其koopa_register
        int koopa_value = 114514;
        int koopa_status = 1919810;

};

class SysYRootAST : public BaseAST {
    public:
        vector<shared_ptr<BaseAST>> comp_unit_list;

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            // 这里是lv8~9前端的入口
            // 函数参数记事簿初始化
            funcparam_note.initialize();
            // 符号表初始化
            symbol_table_list.initialize();
            symbol_table_list.expand();
            // 各类块的指标初始化
            current_if_index = 0;
            current_while_index = 0;
            current_or_index = 0;
            current_and_index = 0;
            next_if_index = 1;
            next_while_index = 1;
            next_or_index = 1;
            next_and_index = 1;
            scan_overrides();
            if (!getint_overridden) {
                save_to(dest_str, "decl @getint(): i32\n", save_mode);
                symbol_table_list.insert_variable("getint", -1);
            }
            if (!getch_overridden) {
                save_to(dest_str, "decl @getch(): i32\n", save_mode);
                symbol_table_list.insert_variable("getch", -1);
            } 
            if (!getarray_overridden) {
                save_to(dest_str, "decl @getarray(*i32): i32\n", save_mode);
                funcparam_note.insert("getarray", 0, "int array");
                symbol_table_list.insert_variable("getarray", -1);
            }
            if (!putint_overridden) {
                save_to(dest_str, "decl @putint(i32)\n", save_mode);
                funcparam_note.insert("putint", 0, "int");
                symbol_table_list.insert_variable("putint", -1);
            }
            if (!putch_overridden) {
                save_to(dest_str, "decl @putch(i32)\n", save_mode);
                funcparam_note.insert("putch", 0, "int");
                symbol_table_list.insert_variable("putch", -1);
            }
            if (!putarray_overridden) {
                save_to(dest_str, "decl @putarray(i32, *i32)\n", save_mode);
                funcparam_note.insert("putarray", 0, "int");
                funcparam_note.insert("putarray", 1, "int array");
                symbol_table_list.insert_variable("putarray", -1);
            }
            if (!starttime_overridden) {
                save_to(dest_str, "decl @starttime()\n", save_mode);
                symbol_table_list.insert_variable("starttime", -1);
            }
            if (!stoptime_overridden) {
                save_to(dest_str, "decl @stoptime()\n", save_mode);
                symbol_table_list.insert_variable("stoptime", -1);
            } 
            save_to(dest_str, "\n", save_mode);
            for (size_t i = 0; i < comp_unit_list.size(); i++){
                int backup = is_last_element;
                is_last_element = (i == comp_unit_list.size() - 1);
                comp_unit_list[i]->SaveKoopa(dest_str, save_mode);
                is_last_element = backup;
            }
            symbol_table_list.shrink();
        }

        void scan_overrides() const override {
            for (size_t i = 0; i < comp_unit_list.size(); i++){
                comp_unit_list[i]->scan_overrides();
            }
        }
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
    public:
        shared_ptr<BaseAST> func_def;
        shared_ptr<BaseAST> decl;
        int is_func_def_flag;
        int is_decl_flag;

        void Dump() const override {
            cout << "CompUnitAST {\n";
            func_def->Dump();
            cout << "}\n";
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (is_func_def_flag) {
                dealing_with_globals = 0;
                func_def->SaveKoopa(dest_str, save_mode);
            }else if (is_decl_flag) {
                dealing_with_globals = 1;
                decl->SaveKoopa_global(dest_str, save_mode);
            }else{
                assert(false);
            }
        }

        void scan_overrides() const override {
            if (is_func_def_flag) {
                func_def->scan_overrides();
            }else if (is_decl_flag) {
                decl->scan_overrides();
            }else{
                assert(false);
            }
        }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
    public:
        string func_type;
        string ident;
        shared_ptr<BaseAST> block;
        int has_params;
        vector<shared_ptr<BaseAST>> funcfparams;

        void Dump() const override {
            cout << "FuncDefAST {\n";
            cout << "ident: " << ident << "\n";
            block->Dump();
            cout << "}\n";
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            save_to(dest_str, "fun @", save_mode);
            save_to(dest_str, ident, save_mode);
            symbol_table_list.insert_variable(ident, -1);
            if (is_special_func_name(ident)) {
                save_to(dest_str, "(", save_mode);
            }else{
                save_to(dest_str, "_func_name(", save_mode);
            }
            
            if (has_params) {
                for (size_t i = 0; i < funcfparams.size(); i++) {
                    if (i) save_to(dest_str, ", ", save_mode);
                    funcfparams[i]->SaveKoopa(dest_str, save_mode);
                    funcparam_note.insert(ident, i, funcfparams[i]->get_btype());
                }
            }
            save_to(dest_str, ")", save_mode);
            if (func_type == "int") {
                save_to(dest_str, ":i32", save_mode);
            }else if (func_type == "void") {

            }else{
                assert(false);
            }
            save_to(dest_str, " {\n", save_mode);

            // 全局变量初始化
            symbol_table_list.expand();
            current_max_register = -1;
            lval_called_from_rhs = 1;
            block_called_from_func_def = 1;
            current_func_type = func_type;
            may_using_func_res = 1;
            in_main_stream = 1;
            main_stream_returned = 0;
            
            block->check_control_flow();
            save_to(dest_str, "%entry_label:\n", save_mode);
            if (has_params) {
                for (size_t i = 0; i < funcfparams.size(); i++) {
                    funcfparams[i]->koopa_allocate();
                    funcfparams[i]->SaveKoopa_parainit(dest_str, save_mode);
                }
            }
            block->koopa_allocate();
            block->SaveKoopa(dest_str, save_mode);
            if(!main_stream_returned) {
                if (func_type == "int") {
                    save_to(dest_str, "  ret 0\n", save_mode);
                }else if (func_type == "void") {
                    save_to(dest_str, "  ret\n", save_mode);
                }else{
                    assert(false);
                }
            }
            save_to(dest_str, "}\n", save_mode);
            if (!is_last_element) save_to(dest_str, "\n", save_mode);
            block_called_from_func_def = 0;
            symbol_table_list.shrink();
        }

        void scan_overrides() const override {
            if (ident == "getint") {
                getint_overridden = 1;
            }else if (ident == "getch") {
                getch_overridden = 1;
            }else if (ident == "getarray") {
                getarray_overridden = 1;
            }else if (ident == "putint") {
                putint_overridden = 1;
            }else if (ident == "putch") {
                putch_overridden = 1;
            }else if (ident == "putarray") {
                putarray_overridden = 1;
            }else if (ident == "starttime") {
                starttime_overridden = 1;
            }else if (ident == "stoptime") {
                stoptime_overridden = 1;
            }else{

            }
        }
};

class FuncFParamAST : public BaseAST {
    public:
        string ident;
        string btype;
        string stack_position;
        vector<shared_ptr<BaseAST>> indexes;
        vector<int> array_shape;

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (btype == "int") {
                save_to(dest_str, "@"+ident+"_func_param: i32", save_mode);
            }else if (btype == "int array") {
                save_to(dest_str, "@"+ident+"_func_param: *i32", save_mode);
            }else{
                assert(false);
            }
        }
        void koopa_allocate() override {
            symbol_table_list.insert_variable(ident, -1);
            stack_position = symbol_table_list.get_stack_position(ident);
            if (btype == "int array") {
                array_shape.push_back(1);
                for (size_t i = 0; i < indexes.size(); i += 1) {
                    indexes[i]->koopa_allocate();
                    assert(indexes[i]->get_koopa_status());
                    array_shape.push_back(indexes[i]->get_koopa_value());
                }
                vector<int> processed_array_shape;
                int current_mulres = 1;
                for (size_t i = array_shape.size()-1; i != -1; i -= 1) {
                    current_mulres *= (int)array_shape[i];
                    processed_array_shape.insert(processed_array_shape.begin(), current_mulres);
                }
                array_dimension_note[stack_position] = processed_array_shape;
            }
        }

        void SaveKoopa_parainit(string& dest_str, const string save_mode) const override {
            if (btype == "int") {
                save_to(dest_str, "  "+stack_position+" = alloc i32\n", save_mode);
                save_to(dest_str, "  store @"+ident+"_func_param, "+stack_position+"\n", save_mode);
            }else if (btype == "int array") {
                save_to(dest_str, "  "+stack_position+"_ptr = alloc *i32\n", save_mode);
                save_to(dest_str, "  store @"+ident+"_func_param, "+stack_position+"_ptr\n", save_mode);
                save_to(dest_str, "  "+stack_position+" = load "+stack_position+"_ptr\n", save_mode);
                is_int_ptr[stack_position] = 1;
            }else{
                assert(false);
            }
        }

        string get_btype() const override {
            return btype;
        }
        
};


class BlockAST : public BaseAST {
    public:
        vector<shared_ptr<BaseAST>> blockitems;
        size_t first_flow_index = blockitems.size() + 1;

        void Dump() const override {
            cout << "BlockAST {\n";
            for (const auto& item : blockitems) {
                item->Dump();
            }
            cout << "}\n";
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {

            for (size_t i = 0; i < blockitems.size(); i++){
                if (i > first_flow_index) break;
                int backup = block_called_from_func_def;
                block_called_from_func_def = 0;
                blockitems[i]->SaveKoopa(dest_str, save_mode);
                block_called_from_func_def = backup;
            }
        }

        bool check_control_flow() override {
            // 获取第一个转移语句的位置，之后的blockitem应被忽略
            first_flow_index = blockitems.size();
            for (size_t i = 0; i < blockitems.size(); i++){
                if (blockitems[i]->check_control_flow()){
                    first_flow_index = i;
                    return true;
                }
            }
            return false;
        }

        void koopa_allocate() override {
            if (!block_called_from_func_def) symbol_table_list.expand();
            int backup = block_called_from_func_def;
            block_called_from_func_def = 0;
            for (size_t i = 0; i < blockitems.size(); i++){
                if (i > first_flow_index) break;
                blockitems[i]->koopa_allocate();
            }
            block_called_from_func_def = backup;
            if (!block_called_from_func_def) symbol_table_list.shrink();
        }

        vector<shared_ptr<BaseAST>>& get_blockitems() override {
            return blockitems;
        }
};


class BlockItemAST : public BaseAST {
    public:
        shared_ptr<BaseAST> stmt;
        shared_ptr<BaseAST> decl;
        int is_stmt_flag;

        void Dump() const override {
            cout << "BlockAST {\n";
            if(is_stmt_flag){
                stmt->Dump();
            }else{
                decl->Dump();
            }
            cout << "}\n";
        }

        void koopa_allocate() override {
            if (is_stmt_flag) {
                stmt->koopa_allocate();
            }else{
                decl->koopa_allocate();
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (is_stmt_flag) {
                stmt->SaveKoopa(dest_str, save_mode);
            }else{
                decl->SaveKoopa(dest_str, save_mode);
            }
        }
        
        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }

        bool check_control_flow() override {
            if (is_stmt_flag) {
                return stmt->check_control_flow();
            }
            return false;
        }
};

class DeclAST : public BaseAST {
    public:
        shared_ptr<BaseAST> constdecl;
        shared_ptr<BaseAST> vardecl;
        int is_constdecl_flag;

        void Dump() const override {
            cout << "DeclAST {\n";
            if (is_constdecl_flag) {
                constdecl->Dump();
            }else{
                vardecl->Dump();
            }
            cout << "}\n";
        }

        void scan_overrides() const override {
            if (is_constdecl_flag) {
                constdecl->scan_overrides();
            }else{
                vardecl->scan_overrides();
            }
        }

        void koopa_allocate() override {
            if (is_constdecl_flag) {
                constdecl->koopa_allocate();
            }else{
                vardecl->koopa_allocate();
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (is_constdecl_flag) {
                constdecl->SaveKoopa(dest_str, save_mode);
            }else{
                vardecl->SaveKoopa(dest_str, save_mode);
            }
        }

        void SaveKoopa_global(string& dest_str, const string save_mode) const override {
            if (is_constdecl_flag) {
                constdecl->SaveKoopa_global(dest_str, save_mode);
            }else{
                vardecl->SaveKoopa_global(dest_str, save_mode);
            }

        }
        
        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class ConstDeclAST : public BaseAST {
    public:
        string btype;
        vector<shared_ptr<BaseAST>> constdefs;

        void Dump() const override {
            cout << "ConstDeclAST {\n";
            for (const auto& item : constdefs) {
                item->Dump();
            }
            cout << "}\n";
        }

        void scan_overrides() const override {
            for (const auto& item : constdefs) {
                item->scan_overrides();
            }
        }

        void koopa_allocate() override {
            for (const auto& item : constdefs) {
                item->koopa_allocate();
            }
        }
        
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }

        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            for (const auto& item : constdefs) {
                item->SaveKoopa(dest_str, save_mode);
            }
        }

        void SaveKoopa_global(string& dest_str, const string save_mode) const override {
            int backup = is_last_element;
            for (size_t i = 0; i < constdefs.size(); i += 1) {
                auto item = constdefs[i];
                item->koopa_allocate();
                is_last_element = backup && (i == constdefs.size() - 1);
                item->SaveKoopa_global(dest_str, save_mode);
            }
            is_last_element = backup;
        }
};

class ConstDefAST : public BaseAST {
    public:
        string ident;
        string stack_position;
        shared_ptr<BaseAST> constinitval;
        int has_index_flag;
        vector<shared_ptr<BaseAST>> indexes;
        vector<int> original_array_dimensions;
        int total_length;
        vector<int> padded_init_list;

        void Dump() const override {
            cout << "ConstDefAST {\n";
            cout << "ident: " << ident << endl;
            cout << "constinitval: " << endl;
            constinitval->Dump();
            cout << "}\n";
        }

        void scan_overrides() const override {
            if (ident == "getint") {
                getint_overridden = 1;
            }else if (ident == "getch") {
                getch_overridden = 1;
            }else if (ident == "getarray") {
                getarray_overridden = 1;
            }else if (ident == "putint") {
                putint_overridden = 1;
            }else if (ident == "putch") {
                putch_overridden = 1;
            }else if (ident == "putarray") {
                putarray_overridden = 1;
            }else if (ident == "starttime") {
                starttime_overridden = 1;
            }else if (ident == "stoptime") {
                stoptime_overridden = 1;
            }else{

            }
        }

        int preprocess_variables() override {
            int constant_value = constinitval->preprocess_variables();
            symbol_table_list.insert_constant(ident, constant_value);
            return 0;
        }

        void koopa_allocate() override {
            if (has_index_flag) {
                vector<int> array_dimensions;
                for (size_t i = 0; i < indexes.size(); i += 1) {
                    indexes[i]->koopa_allocate();
                    assert(indexes[i]->get_koopa_status());
                    original_array_dimensions.push_back(indexes[i]->get_koopa_value());
                }
                int current_mulres = 1;
                for (size_t i = indexes.size() - 1; i != -1; i -= 1) {
                    current_mulres *= indexes[i]->get_koopa_value();
                    array_dimensions.insert(array_dimensions.begin(), current_mulres);
                }
                symbol_table_list.insert_variable(ident, -1);
                stack_position = symbol_table_list.get_stack_position(ident);
                array_dimension_note[stack_position] = array_dimensions;
                array_total_length = current_mulres;
                total_length = current_mulres;
                constinitval->koopa_allocate();
                koopa_value = constinitval->get_koopa_value();
                koopa_status = constinitval->get_koopa_status();
                padded_init_list = constinitval->initval_parse(original_array_dimensions);
            }else{
                preprocess_variables();
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (has_index_flag) {
                save_to(dest_str, "  "+stack_position+" = alloc [i32, "+to_string(total_length)+"]\n", save_mode);
                int i = total_length - 1;
                size_t index = 0;
                while (i >= 0) {
                    if (is_int_ptr[stack_position]){
                        save_to(dest_str, "  "+forward_count(get_koopa_name(), i)+" = getptr "+stack_position+", "+to_string(index)+"\n", save_mode);
                    }else{
                        save_to(dest_str, "  "+forward_count(get_koopa_name(), i)+" = getelemptr "+stack_position+", "+to_string(index)+"\n", save_mode);
                    }
                    save_to(dest_str, "  store "+to_string(padded_init_list[(int)index])+", "+forward_count(get_koopa_name(), i)+"\n", save_mode);
                    i -= 1;
                    index += 1;
                }
            }else{

            }
        }
        
        void SaveKoopa_global(string& dest_str, const string save_mode) const override {
            if (has_index_flag) {
                save_to(dest_str, "global "+stack_position+" = alloc [i32, "+to_string(total_length)+"], ", save_mode);
                save_to(dest_str, "{", save_mode);
                size_t i = 0;
                for (; i < padded_init_list.size(); i += 1){
                    if (i) save_to(dest_str, ", ", save_mode);
                    auto item = padded_init_list[(int)i];
                    save_to(dest_str, to_string(item), save_mode);
                }
                save_to(dest_str, "}", save_mode);
                save_to(dest_str, "\n", save_mode);
                if (!is_last_element) save_to(dest_str, "\n", save_mode);
            }else{
                
            }
        }

        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class ConstInitValAST : public BaseAST {
    public:
        shared_ptr<BaseAST> constexp;
        vector<shared_ptr<BaseAST>> constinitvals;
        int is_constexp_flag;

        void Dump() const override {
            cout << "ConstInitValAST {\n";
            constexp->Dump();
            cout << "}\n";
        }

        int preprocess_variables() override {
            return constexp->preprocess_variables();
        }

        void koopa_allocate() override {
            int backup = inside_array_init;
            if (!inside_array_init) {
                if (!is_constexp_flag) {
                    for (size_t i = 0; i < constinitvals.size(); i += 1){
                        inside_array_init = 1;
                        constinitvals[i]->koopa_allocate();
                    }
                    if (!dealing_with_globals) {
                        current_max_register += array_total_length;
                        koopa_value = current_max_register;
                        koopa_status = 0;
                    }
                }else{
                    constexp->koopa_allocate();
                    koopa_value = constexp->get_koopa_value();
                    koopa_status = constexp->get_koopa_status();
                }
            }else{
                if (!is_constexp_flag) {
                    for (size_t i = 0; i < constinitvals.size(); i += 1){
                        inside_array_init = 1;
                        constinitvals[i]->koopa_allocate();
                    }
                    // koopa_value = constinitvals[0]->get_koopa_value();
                    // koopa_status = constinitvals[0]->get_koopa_status();
                }else{
                    constexp->koopa_allocate();
                    koopa_value = constexp->get_koopa_value();
                    koopa_status = constexp->get_koopa_status();
                }
            }
            inside_array_init = backup;
        }

        vector<int> initval_parse(vector<int> array_shape) override {
            vector<int> processed_array_shape, res_array;
            int current_mulres = 1;
            for (size_t i = array_shape.size()-1; i != -1; i -= 1) {
                current_mulres *= array_shape[i];
                processed_array_shape.insert(processed_array_shape.begin(), current_mulres);
            }
            int current_element_num = 0;
            if (processed_array_shape.size() == 1) {
                for (size_t i = 0; i < constinitvals.size(); i += 1) {
                    vector<int> sub_res_array;
                    if (constinitvals[i]->get_constexp_flag()) {
                        sub_res_array.push_back(constinitvals[i]->get_koopa_value());
                        current_element_num += 1;
                    }else{
                        int array_1d_cannot_occur_bracket = 0;
                        assert(array_1d_cannot_occur_bracket);
                    }
                    res_array.insert(res_array.end(), sub_res_array.begin(), sub_res_array.end());
                }
            }else{
                for (size_t i = 0; i < constinitvals.size(); i += 1) {
                    vector<int> sub_res_array;
                    if (constinitvals[i]->get_constexp_flag()) {
                        sub_res_array.push_back(constinitvals[i]->get_koopa_value());
                        current_element_num += 1;
                    }else{
                        int ndarray_bracket_matched = 0;
                        for (size_t j = 1; j < processed_array_shape.size(); j += 1) {
                            if (current_element_num % processed_array_shape[j] == 0) {
                                vector<int> sub_array_shape(array_shape.begin()+j, array_shape.end());
                                sub_res_array = constinitvals[i]->initval_parse(sub_array_shape);
                                current_element_num += (int)sub_res_array.size();
                                ndarray_bracket_matched = 1;
                                break;
                            }
                        }
                        assert(ndarray_bracket_matched);
                    }
                    res_array.insert(res_array.end(), sub_res_array.begin(), sub_res_array.end());
                }
            }
            for (int i = 0; i < current_mulres - current_element_num; i += 1) {
                res_array.push_back(0);
            }
            return res_array;
        }

        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }

        int get_constexp_flag() override {
            return is_constexp_flag;
        }
};

class ConstExpAST : public BaseAST {
    public:
        shared_ptr<BaseAST> exp;

        void Dump() const override {
            cout << "ConstExpAST {\n";
            exp->Dump();
            cout << "}\n";
        }

        int preprocess_variables() override {
            return exp->preprocess_variables();
        }

        void koopa_allocate() override {
            exp->koopa_allocate();
            assert(exp->get_koopa_status());
            koopa_status = 1;
            koopa_value = exp->get_koopa_value();
        }

        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class VarDeclAST : public BaseAST {
    public:
        string btype;
        vector<shared_ptr<BaseAST>> vardefs;

        void Dump() const override {
            cout << "VarDeclAST {\n";
            for (const auto& item : vardefs) {
                item->Dump();
            }
            cout << "}\n";
        }

        void scan_overrides() const override {
            for (const auto& item : vardefs) {
                item->scan_overrides();
            }
        }

        void koopa_allocate() override {
            for (const auto& item : vardefs) {
                item->koopa_allocate();
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            for (const auto& item : vardefs) {
                item->SaveKoopa(dest_str, save_mode);
            }
        }

        void SaveKoopa_global(string& dest_str, const string save_mode) const override {
            int backup = is_last_element;
            for (size_t i = 0; i < vardefs.size(); i += 1) {
                auto item = vardefs[i];
                item->koopa_allocate();
                is_last_element = backup && (i == vardefs.size() - 1);
                item->SaveKoopa_global(dest_str, save_mode);
            }
            is_last_element = backup;
        }
        
        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class VarDefAST : public BaseAST {
    public:
        string ident;
        string stack_position;
        shared_ptr<BaseAST> initval;
        vector<shared_ptr<BaseAST>> indexes;
        int has_index_flag;
        int initialized_flag;
        vector<int> original_array_dimensions;
        vector<int> padded_init_list;
        int total_length;

        void Dump() const override {
            cout << "VarDefAST {\n";
            cout << "ident: " << ident << endl;
            if (initialized_flag) {
                cout << "initval: " << endl;
                initval->Dump();
            }
            cout << "}\n";
        }

        void scan_overrides() const override {
            if (ident == "getint") {
                getint_overridden = 1;
            }else if (ident == "getch") {
                getch_overridden = 1;
            }else if (ident == "getarray") {
                getarray_overridden = 1;
            }else if (ident == "putint") {
                putint_overridden = 1;
            }else if (ident == "putch") {
                putch_overridden = 1;
            }else if (ident == "putarray") {
                putarray_overridden = 1;
            }else if (ident == "starttime") {
                starttime_overridden = 1;
            }else if (ident == "stoptime") {
                stoptime_overridden = 1;
            }else{

            }
        }

        int preprocess_variables() override {
            symbol_table_list.insert_variable(ident, -1);
            stack_position = symbol_table_list.get_stack_position(ident);
            return 0;
        }

        void koopa_allocate() override {
            if (has_index_flag) {
                vector<int> array_dimensions;
                for (size_t i = 0; i < indexes.size(); i += 1) {
                    indexes[i]->koopa_allocate();
                    assert(indexes[i]->get_koopa_status());
                    original_array_dimensions.push_back(indexes[i]->get_koopa_value());
                }
                int current_mulres = 1;
                for (size_t i = indexes.size() - 1; i != -1; i -= 1) {
                    current_mulres *= indexes[i]->get_koopa_value();
                    array_dimensions.insert(array_dimensions.begin(), current_mulres);
                }
                symbol_table_list.insert_variable(ident, -1);
                stack_position = symbol_table_list.get_stack_position(ident);
                array_dimension_note[stack_position] = array_dimensions;
                total_length = current_mulres;
                if (initialized_flag) {
                    array_total_length = current_mulres;
                    initval->koopa_allocate();
                    koopa_value = initval->get_koopa_value();
                    koopa_status = initval->get_koopa_status();
                    padded_init_list = initval->initval_parse(original_array_dimensions);
                }
            }else{
                if (initialized_flag) {
                    koopa_status = 0;
                    initval->koopa_allocate();
                    preprocess_variables();
                    if (initval->get_koopa_status()) {
                        current_max_register += 1;
                        assert(current_max_register >= -1);
                        koopa_value = current_max_register;
                    }else{
                        koopa_value = initval->get_koopa_value();
                    }
                    symbol_table_list.update_variable(ident, koopa_value);
                }else{
                    preprocess_variables();
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (has_index_flag) {
                save_to(dest_str, "  "+stack_position+" = alloc [i32, "+to_string(total_length)+"]\n", save_mode);
                if (initialized_flag) {
                    int i = total_length - 1;
                    size_t index = 0;
                    while (i >= 0) {
                        if (is_int_ptr[stack_position]) {
                            save_to(dest_str, "  "+forward_count(get_koopa_name(), i)+" = getptr "+stack_position+", "+to_string(index)+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+forward_count(get_koopa_name(), i)+" = getelemptr "+stack_position+", "+to_string(index)+"\n", save_mode);
                        }
                        save_to(dest_str, "  store "+to_string(padded_init_list[(int)index])+", "+forward_count(get_koopa_name(), i)+"\n", save_mode);
                        i -= 1;
                        index += 1;
                    }
                }
            }else{
                save_to(dest_str, "  "+stack_position+" = alloc i32\n", save_mode);
                if (initialized_flag) {
                    if (!initval->get_koopa_status()) {
                        initval->SaveKoopa_load(dest_str, save_mode);
                        initval->SaveKoopa(dest_str, save_mode);
                    }
                    save_to(dest_str, "  store "+initval->get_koopa_name()+", "+stack_position+"\n", save_mode);
                }
            }
        }

        void SaveKoopa_alloc(string& dest_str, const string save_mode) const override {
            save_to(dest_str, "  "+stack_position+" = alloc i32\n", save_mode);
        }

        void SaveKoopa_global(string& dest_str, const string save_mode) const override {
            if (has_index_flag) {
                save_to(dest_str, "global "+stack_position+" = alloc [i32, "+to_string(total_length)+"], ", save_mode);
                if (initialized_flag) {
                    save_to(dest_str, "{", save_mode);
                    size_t i = 0;
                    for (; i < padded_init_list.size(); i += 1){
                        if (i) save_to(dest_str, ", ", save_mode);
                        auto item = padded_init_list[i];
                        save_to(dest_str, to_string(item), save_mode);
                    }
                    save_to(dest_str, "}", save_mode);
                    save_to(dest_str, "\n", save_mode);
                }else{
                    save_to(dest_str, "zeroinit\n", save_mode);
                }
            }else{
                if (initialized_flag) {
                    assert(initval->get_koopa_status());
                    save_to(dest_str, "global "+stack_position+" = alloc i32, "+initval->get_koopa_name()+"\n", save_mode);
                }else{
                    save_to(dest_str, "global "+stack_position+" = alloc i32, zeroinit\n", save_mode);
                }
            }
            if (!is_last_element) save_to(dest_str, "\n", save_mode);
        }
        
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }

        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class InitValAST : public BaseAST {
    public:
        shared_ptr<BaseAST> exp;
        vector<shared_ptr<BaseAST>> initvals;
        int is_exp_flag;

        void Dump() const override {
            cout << "InitValAST {\n";
            exp->Dump();
            cout << "}\n";
        }

        void koopa_allocate() override {
            int backup = inside_array_init;
            if (!inside_array_init) {
                if (!is_exp_flag) {
                    for (size_t i = 0; i < initvals.size(); i += 1){
                        inside_array_init = 1;
                        initvals[i]->koopa_allocate();
                    }
                    if (!dealing_with_globals) {
                        current_max_register += array_total_length;
                        koopa_value = current_max_register;
                        koopa_status = 0;
                    }
                }else{
                    exp->koopa_allocate();
                    koopa_value = exp->get_koopa_value();
                    koopa_status = exp->get_koopa_status();
                }
            }else{
                if (!is_exp_flag) {
                    for (size_t i = 0; i < initvals.size(); i += 1){
                        inside_array_init = 1;
                        initvals[i]->koopa_allocate();
                    }
                    // koopa_value = initvals[0]->get_koopa_value();
                    // koopa_status = initvals[0]->get_koopa_status();
                }else{
                    exp->koopa_allocate();
                    koopa_value = exp->get_koopa_value();
                    koopa_status = exp->get_koopa_status();
                }
            }
            inside_array_init = backup;
        }

        vector<int> initval_parse(vector<int> array_shape) override {
            vector<int> processed_array_shape, res_array;
            int current_mulres = 1;
            for (size_t i = array_shape.size()-1; i != -1; i -= 1) {
                current_mulres *= array_shape[i];
                processed_array_shape.insert(processed_array_shape.begin(), current_mulres);
            }
            int current_element_num = 0;
            if (processed_array_shape.size() == 1) {
                for (size_t i = 0; i < initvals.size(); i += 1) {
                    vector<int> sub_res_array;
                    if (initvals[i]->get_exp_flag()) {
                        sub_res_array.push_back(initvals[i]->get_koopa_value());
                        current_element_num += 1;
                    }else{
                        int array_1d_cannot_occur_bracket = 0;
                        assert(array_1d_cannot_occur_bracket);
                    }
                    res_array.insert(res_array.end(), sub_res_array.begin(), sub_res_array.end());
                }
            }else{
                for (size_t i = 0; i < initvals.size(); i += 1) {
                    vector<int> sub_res_array;
                    if (initvals[i]->get_exp_flag()) {
                        sub_res_array.push_back(initvals[i]->get_koopa_value());
                        current_element_num += 1;
                    }else{
                        int ndarray_bracket_matched = 0;
                        for (size_t j = 1; j < processed_array_shape.size(); j += 1) {
                            if (current_element_num % processed_array_shape[j] == 0) {
                                vector<int> sub_array_shape(array_shape.begin()+j, array_shape.end());
                                sub_res_array = initvals[i]->initval_parse(sub_array_shape);
                                current_element_num += (int)sub_res_array.size();
                                ndarray_bracket_matched = 1;
                                break;
                            }
                        }
                        assert(ndarray_bracket_matched);
                    }
                    res_array.insert(res_array.end(), sub_res_array.begin(), sub_res_array.end());
                }
            }
            for (int i = 0; i < current_mulres - current_element_num; i += 1) {
                res_array.push_back(0);
            }
            return res_array;
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (!is_exp_flag) {

            }else{
                exp->SaveKoopa(dest_str, save_mode);
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            exp->SaveKoopa_load(dest_str, save_mode);
        }

        void SaveKoopa_global(string& dest_str, const string save_mode) const override {
            if (!is_exp_flag) {
                
            }else{

            }
        }

        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }

        int get_exp_flag() override {
            return is_exp_flag;
        }
};

class StmtAST : public BaseAST {
    public:
        shared_ptr<BaseAST> exp;
        shared_ptr<BaseAST> lval;
        shared_ptr<BaseAST> block;
        shared_ptr<BaseAST> if_condition;
        shared_ptr<BaseAST> stmtelse;
        shared_ptr<BaseAST> stmtnotelse;
        shared_ptr<BaseAST> while_condition;
        shared_ptr<BaseAST> while_stmt;
        int is_if_flag;
        int has_else_branch;
        int is_block_flag;
        int is_return_flag;
        int return_empty_flag;
        int is_lval_flag;
        int is_empty_flag;
        int is_while_flag;
        int is_break_flag;
        int is_continue_flag;

        // if (is_continue_flag) {

        // }else if(is_break_flag) {

        // }else if(is_while_flag) {

        // }else if (is_if_flag) {

        // }else if (is_block_flag) {
        //     ;
        // }else if(is_return_flag) {
        //     if (return_empty_flag) {
        //         ;
        //     }else{
        //         ;
        //     }
        // }else if(is_lval_flag) {
        //     ;
        // }else{
        //     if (is_empty_flag) {
        //         ;
        //     }else{
        //         ;
        //     }
        // }

        void Dump() const override {
            cout << "StmtAST {\n";
            if (is_continue_flag) {

            }else if(is_break_flag) {

            }else if(is_while_flag) {

            }else if (is_if_flag) {

            }else if(is_block_flag) {
                ;
            }else if(is_return_flag) {
                if (return_empty_flag) {
                    ;
                }else{
                    cout << "return" << endl;
                    exp->Dump();
                }
            }else if(is_lval_flag) {
                cout << "set value" << endl;
                lval->Dump();
                exp->Dump();
            }else{
                if (is_empty_flag) {
                    ;
                }else{
                    ;
                }
            }
            cout << "}\n";
        }

        void koopa_allocate() override {
            if (is_continue_flag) {

            }else if(is_break_flag) {

            }else if(is_while_flag) {
                while_condition->koopa_allocate();
                while_stmt->koopa_allocate();
            }else if (is_if_flag) {
                if (has_else_branch) {
                    if_condition->koopa_allocate();
                    stmtelse->koopa_allocate();
                }else{
                    if_condition->koopa_allocate();
                    stmtnotelse->koopa_allocate();
                }
            }else if(is_block_flag) {
                block->koopa_allocate();
            }else if(is_return_flag) {
                if (return_empty_flag) {
                    
                }else{
                    exp->koopa_allocate();
                    // 为什么StmtAST需要获得koopa name？？？
                    koopa_status = exp->get_koopa_status();
                    koopa_value = exp->get_koopa_value();
                }
            }else if(is_lval_flag) {
                if (lval->get_has_index_flag()){
                    exp->koopa_allocate();
                    lval_called_from_rhs = 0;
                    lval->koopa_allocate();
                    lval_called_from_rhs = 1;
                }else{
                    exp->koopa_allocate();
                    lval_called_from_rhs = 0;
                    lval->koopa_allocate();
                    lval_called_from_rhs = 1;
                    symbol_table_list.update_variable(lval->get_ident(), koopa_value);
                }
            }else{
                if (is_empty_flag) {
                    
                }else{
                    int backup = may_using_func_res;
                    may_using_func_res = 0;
                    exp->koopa_allocate();
                    may_using_func_res = backup;
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (is_continue_flag) {
                save_to(dest_str, "  jump " + get_while_entry_label() + "\n", save_mode);
            }else if(is_break_flag) {
                save_to(dest_str, "  jump " + get_while_end_label() + "\n", save_mode);
            }else if(is_while_flag) {
                int backup = in_main_stream;
                in_main_stream = 0;
                int previous_while_index = current_while_index;
                current_while_index = next_while_index;
                next_while_index += 1;
                save_to(dest_str, "  jump " + get_while_entry_label() + "\n", save_mode);
                save_to(dest_str, get_while_entry_label() + ":\n", save_mode);
                while_condition->SaveKoopa_load(dest_str, save_mode);
                if (!while_condition->get_koopa_status()) while_condition->SaveKoopa(dest_str, save_mode);
                save_to(dest_str,"  br "+while_condition->get_koopa_name()+", "+get_while_body_label()+", "+get_while_end_label()+"\n",save_mode);
                save_to(dest_str, get_while_body_label() + ":\n", save_mode);
                while_stmt->SaveKoopa(dest_str, save_mode);
                if (!while_stmt->check_control_flow()) {
                    save_to(dest_str, "  jump " + get_while_entry_label() + "\n", save_mode);
                }
                save_to(dest_str, get_while_end_label() + ":\n", save_mode);
                current_while_index = previous_while_index;
                in_main_stream = backup;
            }else if (is_if_flag) {
                int backup = in_main_stream;
                in_main_stream = 0;
                int previous_if_index = current_if_index;
                current_if_index = next_if_index;
                next_if_index += 1;
                if_condition->SaveKoopa_load(dest_str, save_mode);
                if (!if_condition->get_koopa_status()) if_condition->SaveKoopa(dest_str, save_mode);
                if (has_else_branch) {
                    save_to(dest_str,"  br "+if_condition->get_koopa_name()+", "+get_if_then_label()+", "+get_if_else_label()+"\n",save_mode);
                    save_to(dest_str, get_if_then_label() + ":\n", save_mode);
                    stmtelse->SaveKoopa(dest_str, save_mode);
                }else{
                    save_to(dest_str,"  br "+if_condition->get_koopa_name()+", "+get_if_then_label()+", "+get_if_end_label()+"\n",save_mode);
                    save_to(dest_str, get_if_then_label() + ":\n", save_mode);
                    stmtnotelse->SaveKoopa(dest_str, save_mode);
                }
                save_to(dest_str, get_if_end_label() + ":\n", save_mode);
                current_if_index = previous_if_index;
                in_main_stream = backup;
            }else if (is_block_flag) {
                block->SaveKoopa(dest_str, save_mode);
            }else if(is_return_flag) {
                if (in_main_stream) main_stream_returned = 1;
                if (return_empty_flag) {
                    // assert(current_func_type == "void");
                    // ？？？？？吼牛批，int函数还能只return不return 0的
                    if (current_func_type == "int") {
                        save_to(dest_str, "  ret 0\n", save_mode);
                    }else if (current_func_type == "void") {
                        save_to(dest_str, "  ret\n", save_mode);
                    }else{
                        assert(false);
                    }
                }else{
                    // 难道说void函数也可以return [Exp]？？？？
                    assert(current_func_type == "int");
                    exp->SaveKoopa_load(dest_str, save_mode);
                    if (!exp->get_koopa_status()) exp->SaveKoopa(dest_str, save_mode);
                    save_to(dest_str, "  ret " + exp->get_koopa_name() + "\n", save_mode);
                }
            }else if(is_lval_flag) {
                if (lval->get_has_index_flag()){
                    exp->SaveKoopa_load(dest_str, save_mode);
                    if (!exp->get_koopa_status()) exp->SaveKoopa(dest_str, save_mode);
                    lval->SaveKoopa_load(dest_str, save_mode);
                    lval->SaveKoopa(dest_str, save_mode);
                    save_to(dest_str, "  store "+exp->get_koopa_name()+", "+lval->get_koopa_name()+"\n", save_mode);
                }else{
                    exp->SaveKoopa_load(dest_str, save_mode);
                    if (!exp->get_koopa_status()) exp->SaveKoopa(dest_str, save_mode);
                    save_to(dest_str, "  store "+exp->get_koopa_name()+", "+lval->get_stack_position()+"\n", save_mode);
                } 
            }else{
                if (is_empty_flag) {
                    
                }else{
                    exp->SaveKoopa_load(dest_str, save_mode);
                    if (!exp->get_koopa_status()) exp->SaveKoopa(dest_str, save_mode);
                }
            }
        }
        
        int get_koopa_value() const override {
            return koopa_value;
        }
        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }

        bool check_control_flow() override {
            if (is_continue_flag) {
                return true;
            }else if(is_break_flag) {
                return true;
            }else if(is_while_flag) {
                while_stmt->check_control_flow();
                return false;
            }else if (is_if_flag) {
                if (has_else_branch) {
                    stmtelse->check_control_flow();
                }else{
                    stmtnotelse->check_control_flow();
                }
                return false;
            }else if (is_block_flag) {
                return block->check_control_flow();
            }else if(is_return_flag) {
                return true;
            }else if(is_lval_flag) {
                return false;
            }else{
                if (is_empty_flag) {
                    return false;
                }else{
                    return false;
                }
            }
        }

};

class StmtElseAST : public BaseAST {
    public:
        shared_ptr<BaseAST> then_stmt;
        shared_ptr<BaseAST> else_stmt;

        void Dump() const override {

        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            then_stmt->SaveKoopa(dest_str, save_mode);
            if (!then_stmt->check_control_flow()) save_to(dest_str, "  jump " + get_if_end_label() + "\n", save_mode);
            save_to(dest_str, get_if_else_label() + ":\n", save_mode);
            else_stmt->SaveKoopa(dest_str, save_mode);
            if (!else_stmt->check_control_flow()) save_to(dest_str, "  jump " + get_if_end_label() + "\n", save_mode);
        }

        bool check_control_flow() override {
            then_stmt->check_control_flow();
            else_stmt->check_control_flow();
            return false;
        }

        int preprocess_variables() override {
            return 0;
        }

        void koopa_allocate() override {
            then_stmt->koopa_allocate();
            else_stmt->koopa_allocate();
        }
};

class StmtNotElseAST : public BaseAST {
    public:
        shared_ptr<BaseAST> then_stmt;

        void Dump() const override {

        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            then_stmt->SaveKoopa(dest_str, save_mode);
            if (!then_stmt->check_control_flow()) save_to(dest_str, "  jump " + get_if_end_label() + "\n", save_mode);
        }

        bool check_control_flow() override {
            then_stmt->check_control_flow();
            return false;
        }

        int preprocess_variables() override {
            return 0;
        }

        void koopa_allocate() override {
            then_stmt->koopa_allocate();
        }
};

class ExpAST : public BaseAST {
    public:
        shared_ptr<BaseAST> lor_exp;
        int is_pseudo_flag;

        void Dump() const override {
            cout << "ExpAST {\n";
            lor_exp->Dump();
            cout << "}\n";
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (is_pseudo_flag) return;
            lor_exp->SaveKoopa(dest_str, save_mode);
        }

        int preprocess_variables() override {
            return lor_exp->preprocess_variables();
        }

        void koopa_allocate() override {
            if (is_pseudo_flag) {
                koopa_status = 1;
                koopa_value = 0;
                return;
            }
            lor_exp->koopa_allocate();
            koopa_status = lor_exp->get_koopa_status();
            koopa_value = lor_exp->get_koopa_value();

        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            if (is_pseudo_flag) return;
            lor_exp->SaveKoopa_load(dest_str, save_mode);
        }

        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }

        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class LOrExpAST : public BaseAST {
    public:
        int is_single_land_flag;
        shared_ptr<BaseAST> land_exp;
        shared_ptr<BaseAST> lor_exp;
        void Dump() const override {
            cout << "LOrExpAST {\n";
            if (is_single_land_flag){
                land_exp->Dump();
            }else{
                lor_exp->Dump();
                land_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            int is_single_opt_flag = is_single_land_flag;
            shared_ptr<BaseAST> opt_exp = land_exp;
            shared_ptr<BaseAST> son_exp = lor_exp;
            if (is_single_opt_flag) {
                return opt_exp->preprocess_variables();
            } else {
                return opt_exp->preprocess_variables() || son_exp->preprocess_variables();
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            int is_single_opt_flag = is_single_land_flag;
            shared_ptr<BaseAST> opt_exp = land_exp;
            shared_ptr<BaseAST> son_exp = lor_exp;
            if (is_single_opt_flag){
                opt_exp->SaveKoopa(dest_str, save_mode);
            }else{
                // 短路或的实现
                int previous_or_index = current_or_index;
                current_or_index = next_or_index;
                next_or_index += 1;
                string little_mother_koopa_name = "%" + to_string(get_koopa_value() - 2);
                string mother_koopa_name = "%" + to_string(get_koopa_value() - 1);
                string father_koopa_name = get_koopa_name();
                string opt_koopa_name = opt_exp->get_koopa_name();
                string son_koopa_name = son_exp->get_koopa_name();
                string enchant_head_name = forward_count(father_koopa_name, 3);
                if(!son_exp->get_koopa_status()) son_exp->SaveKoopa(dest_str, save_mode);
                save_to(dest_str, "  @res_"+to_string(get_koopa_value())+"_tmp = alloc i32\n", save_mode);
                save_to(dest_str, "  br "+son_koopa_name+", "+get_or_skip_label()+", "+get_or_nonskip_label()+"\n", save_mode);
                save_to(dest_str, get_or_skip_label()+":\n", save_mode);
                save_to(dest_str, "  store 1, @res_"+to_string(get_koopa_value())+"_tmp\n", save_mode);
                save_to(dest_str, "  jump "+get_or_end_label()+"\n", save_mode);
                save_to(dest_str, get_or_nonskip_label()+":\n", save_mode);
                if(!opt_exp->get_koopa_status()) opt_exp->SaveKoopa(dest_str, save_mode);
                // koopa没有直截的lor二元运算，需要自行实现逻辑或
                save_to(dest_str, "  "+enchant_head_name+" = shr 1, "+to_string(OR_HEAD)+"\n", save_mode);
                save_to(dest_str,"  "+mother_koopa_name+" = or "+son_koopa_name+", "+opt_koopa_name+"\n",save_mode);
                save_to(dest_str,"  "+little_mother_koopa_name+" = ne 0, "+mother_koopa_name+"\n",save_mode);
                save_to(dest_str, "  store "+little_mother_koopa_name+", @res_"+to_string(get_koopa_value())+"_tmp\n", save_mode);
                save_to(dest_str, "  jump "+get_or_end_label()+"\n", save_mode);
                save_to(dest_str, get_or_end_label()+":\n", save_mode);
                save_to(dest_str, "  "+father_koopa_name+" = load @res_"+to_string(get_koopa_value())+"_tmp\n", save_mode);
                current_or_index = previous_or_index;
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            int is_single_opt_flag = is_single_land_flag;
            shared_ptr<BaseAST> opt_exp = land_exp;
            shared_ptr<BaseAST> son_exp = lor_exp;
            opt_exp->SaveKoopa_load(dest_str, save_mode);
            if (!is_single_opt_flag) son_exp->SaveKoopa_load(dest_str, save_mode);
        }
 
        void koopa_allocate() override {
            int is_single_opt_flag = is_single_land_flag;
            shared_ptr<BaseAST> son_exp = lor_exp;
            shared_ptr<BaseAST> opt_exp = land_exp;
            int allocate_num = 4;
            int backup = may_using_func_res;
            if (is_single_opt_flag && !may_using_func_res){
                may_using_func_res = 0;
            }else{
                may_using_func_res = 1;
            }
            if (is_single_opt_flag) {
                opt_exp->koopa_allocate();
                koopa_status = opt_exp->get_koopa_status();
                koopa_value = opt_exp->get_koopa_value();
            }else{
                son_exp->koopa_allocate();
                opt_exp->koopa_allocate();
                if (son_exp->get_koopa_status() && opt_exp->get_koopa_status()){
                    koopa_status = 1;
                    koopa_value = son_exp->get_koopa_value() || opt_exp->get_koopa_value();
                }else{
                    koopa_status = 0;
                    current_max_register += allocate_num;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
            may_using_func_res = backup;
        }

        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class LAndExpAST : public BaseAST {
    public:
        int is_single_eq_flag;
        shared_ptr<BaseAST> eq_exp;
        shared_ptr<BaseAST> land_exp;

        void Dump() const override {
            cout << "LAndExpAST {\n";
            if (is_single_eq_flag){
                eq_exp->Dump();
            }else{
                land_exp->Dump();
                eq_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            int is_single_opt_flag = is_single_eq_flag;
            shared_ptr<BaseAST> opt_exp = eq_exp;
            shared_ptr<BaseAST> son_exp = land_exp;
            if (is_single_opt_flag) {
                return opt_exp->preprocess_variables();
            } else {
                return opt_exp->preprocess_variables() && son_exp->preprocess_variables();
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            int is_single_opt_flag = is_single_eq_flag;
            shared_ptr<BaseAST> opt_exp = eq_exp;
            shared_ptr<BaseAST> son_exp = land_exp;
            if (is_single_opt_flag){
                opt_exp->SaveKoopa(dest_str, save_mode);
            }else{
                // 短路与的实现
                int previous_and_index = current_and_index;
                current_and_index = next_and_index;
                next_and_index += 1;
                // 幽默红丸
                string father_koopa_name = get_koopa_name();
                string mother_name = forward_count(father_koopa_name, 1);
                string little_mother_name = forward_count(father_koopa_name, 2);
                string little_father_name = forward_count(father_koopa_name, 3);
                string enchant_head_name = forward_count(father_koopa_name, 4);
                string opt_koopa_name = opt_exp->get_koopa_name();
                string son_koopa_name = son_exp->get_koopa_name();
                if(!son_exp->get_koopa_status()) son_exp->SaveKoopa(dest_str, save_mode);
                save_to(dest_str, "  @res_"+to_string(get_koopa_value())+"_tmp = alloc i32\n", save_mode);
                save_to(dest_str, "  br "+son_koopa_name+", "+get_and_nonskip_label()+", "+get_and_skip_label()+"\n", save_mode);
                save_to(dest_str, get_and_skip_label()+":\n", save_mode);
                save_to(dest_str, "  store 0, @res_"+to_string(get_koopa_value())+"_tmp\n", save_mode);
                save_to(dest_str, "  jump "+get_and_end_label()+"\n", save_mode);
                save_to(dest_str, get_and_nonskip_label()+":\n", save_mode);
                if(!opt_exp->get_koopa_status()) opt_exp->SaveKoopa(dest_str, save_mode);
                // koopa没有直截的land二元运算，需要自行实现逻辑与
                save_to(dest_str, "  "+enchant_head_name+" = shr 1, "+to_string(AND_HEAD)+"\n", save_mode);
                save_to(dest_str,"  "+little_mother_name+" = ne 0, "+opt_koopa_name+"\n",save_mode);
                save_to(dest_str,"  "+mother_name+" = ne 0, "+son_koopa_name+"\n",save_mode);
                save_to(dest_str,"  "+little_father_name+" = and "+little_mother_name+", "+mother_name+"\n",save_mode);
                save_to(dest_str, "  store "+little_father_name+", @res_"+to_string(get_koopa_value())+"_tmp\n", save_mode);
                save_to(dest_str, "  jump "+get_and_end_label()+"\n", save_mode);
                save_to(dest_str, get_and_end_label()+":\n", save_mode);
                save_to(dest_str, "  "+father_koopa_name+" = load @res_"+to_string(get_koopa_value())+"_tmp\n", save_mode);
                current_and_index = previous_and_index;
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            int is_single_opt_flag = is_single_eq_flag;
            shared_ptr<BaseAST> opt_exp = eq_exp;
            shared_ptr<BaseAST> son_exp = land_exp;
            opt_exp->SaveKoopa_load(dest_str, save_mode);
            if (!is_single_opt_flag) son_exp->SaveKoopa_load(dest_str, save_mode);
        }
       
        void koopa_allocate() override {
            int is_single_opt_flag = is_single_eq_flag;
            shared_ptr<BaseAST> son_exp = land_exp;
            shared_ptr<BaseAST> opt_exp = eq_exp;
            int allocate_num = 5;
            int backup = may_using_func_res;
            if (is_single_opt_flag && !may_using_func_res){
                may_using_func_res = 0;
            }else{
                may_using_func_res = 1;
            }
            if (is_single_opt_flag) {
                opt_exp->koopa_allocate();
                koopa_status = opt_exp->get_koopa_status();
                koopa_value = opt_exp->get_koopa_value();
            }else{
                son_exp->koopa_allocate();
                opt_exp->koopa_allocate();
                if (son_exp->get_koopa_status() && opt_exp->get_koopa_status()){
                    koopa_status = 1;
                    koopa_value = son_exp->get_koopa_value() && opt_exp->get_koopa_value();
                }else{
                    koopa_status = 0;
                    current_max_register += allocate_num;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
            may_using_func_res = backup;
        }
        
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
        
};

class EqExpAST : public BaseAST {
    public:
        int is_single_rel_flag;
        string eq_operator;
        shared_ptr<BaseAST> rel_exp;
        shared_ptr<BaseAST> eq_exp;
        void Dump() const override {
            cout << "EqExpAST {\n";
            if (is_single_rel_flag){
                rel_exp->Dump();
            }else{
                eq_exp->Dump();
                cout << "eq_operator: " << eq_operator << "\n";
                rel_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            int is_single_opt_flag = is_single_rel_flag;
            shared_ptr<BaseAST> opt_exp = rel_exp;
            shared_ptr<BaseAST> son_exp = eq_exp;
            if (is_single_opt_flag) {
                return opt_exp->preprocess_variables();
            } else {
                if (eq_operator == "==") {
                    return opt_exp->preprocess_variables() == son_exp->preprocess_variables();
                }else if (eq_operator == "!=") {
                    return opt_exp->preprocess_variables() != son_exp->preprocess_variables();
                }else{
                    cout << "Unknown Eq-level operator " << eq_operator <<"!\n";
                    assert(false);
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            int is_single_opt_flag = is_single_rel_flag;
            shared_ptr<BaseAST> opt_exp = rel_exp;
            shared_ptr<BaseAST> son_exp = eq_exp;
            string koopa_operator_string;
            if (eq_operator == "=="){
                koopa_operator_string = "eq";
            }else if (eq_operator == "!="){
                koopa_operator_string = "ne";
            }else{
                // cout << "Unknown Eq-level operator " << eq_operator <<"!\n";
                // assert(false);
            }
            if (is_single_opt_flag){
                opt_exp->SaveKoopa(dest_str, save_mode);
            }else{
                if(!opt_exp->get_koopa_status()) opt_exp->SaveKoopa(dest_str, save_mode);
                if(!son_exp->get_koopa_status()) son_exp->SaveKoopa(dest_str, save_mode);
                string father_koopa_name = this->get_koopa_name();
                string opt_koopa_name = opt_exp->get_koopa_name();
                string son_koopa_name = son_exp->get_koopa_name();
                save_to(dest_str, "  "+father_koopa_name+" = "+koopa_operator_string+" "+son_koopa_name+", "+opt_koopa_name+"\n", save_mode);
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            int is_single_opt_flag = is_single_rel_flag;
            shared_ptr<BaseAST> opt_exp = rel_exp;
            shared_ptr<BaseAST> son_exp = eq_exp;
            opt_exp->SaveKoopa_load(dest_str, save_mode);
            if (!is_single_opt_flag) son_exp->SaveKoopa_load(dest_str, save_mode);
        }

        void koopa_allocate() override {
            int is_single_opt_flag = is_single_rel_flag;
            shared_ptr<BaseAST> son_exp = eq_exp;
            shared_ptr<BaseAST> opt_exp = rel_exp;
            int allocate_num = 1;
            int backup = may_using_func_res;
            if (is_single_opt_flag && !may_using_func_res){
                may_using_func_res = 0;
            }else{
                may_using_func_res = 1;
            }
            if (is_single_opt_flag) {
                opt_exp->koopa_allocate();
                koopa_status = opt_exp->get_koopa_status();
                koopa_value = opt_exp->get_koopa_value();
            }else{
                son_exp->koopa_allocate();
                opt_exp->koopa_allocate();
                if (son_exp->get_koopa_status() && opt_exp->get_koopa_status()){
                    koopa_status = 1;
                    if (eq_operator == "==") {
                        koopa_value = son_exp->get_koopa_value() == opt_exp->get_koopa_value();
                    }else if (eq_operator == "!=") {
                        koopa_value = son_exp->get_koopa_value() != opt_exp->get_koopa_value();
                    }else{
                        cout << "Unknown Eq-level operator " << eq_operator <<"!\n";
                        assert(false);
                    }
                    
                }else{
                    koopa_status = 0;
                    current_max_register += allocate_num;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
            may_using_func_res = backup;
        }
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
        
};

class RelExpAST : public BaseAST {
    public:
        int is_single_add_flag;
        shared_ptr<BaseAST> rel_exp;
        shared_ptr<BaseAST> add_exp;
        string rel_operator;
        void Dump() const override {
            cout << "RelExpAST {\n";
            if (is_single_add_flag){
                add_exp->Dump();
            }else{
                rel_exp->Dump();
                cout << "rel_operator: " << rel_operator << "\n";
                add_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            int is_single_opt_flag = is_single_add_flag;
            shared_ptr<BaseAST> opt_exp = add_exp;
            shared_ptr<BaseAST> son_exp = rel_exp;
            if (is_single_opt_flag) {
                return opt_exp->preprocess_variables();
            } else {
                if (rel_operator == "<") {
                    return son_exp->preprocess_variables() < opt_exp->preprocess_variables();
                }else if (rel_operator == ">") {
                    return son_exp->preprocess_variables() > opt_exp->preprocess_variables();
                }else if (rel_operator == "<=") {
                    return son_exp->preprocess_variables() <= opt_exp->preprocess_variables();
                }else if (rel_operator == ">=") {
                    return son_exp->preprocess_variables() >= opt_exp->preprocess_variables();
                }else{
                    cout << "Unknown Rel-level operator " << rel_operator <<"!\n";
                    assert(false);
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            int is_single_opt_flag = is_single_add_flag;
            shared_ptr<BaseAST> opt_exp = add_exp;
            shared_ptr<BaseAST> son_exp = rel_exp;
            string koopa_operator_string;
            if (rel_operator == "<"){
                koopa_operator_string = "lt";
            }else if (rel_operator == ">"){
                koopa_operator_string = "gt";
            }else if (rel_operator == "<="){
                koopa_operator_string = "le";
            }else if (rel_operator == ">="){
                koopa_operator_string = "ge";
            }else{
                // cout << "Unknown Rel-level operator " << rel_operator <<"!\n";
                // assert(false);
            }
            if (is_single_opt_flag){
                opt_exp->SaveKoopa(dest_str, save_mode);
            }else{
                if(!opt_exp->get_koopa_status()) opt_exp->SaveKoopa(dest_str, save_mode);
                if(!son_exp->get_koopa_status()) son_exp->SaveKoopa(dest_str, save_mode);
                string father_koopa_name = this->get_koopa_name();
                string opt_koopa_name = opt_exp->get_koopa_name();
                string son_koopa_name = son_exp->get_koopa_name();
                save_to(dest_str, "  "+father_koopa_name+" = "+koopa_operator_string+" "+son_koopa_name+", "+opt_koopa_name+"\n", save_mode);
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            int is_single_opt_flag = is_single_add_flag;
            shared_ptr<BaseAST> opt_exp = add_exp;
            shared_ptr<BaseAST> son_exp = rel_exp;
            opt_exp->SaveKoopa_load(dest_str, save_mode);
            if (!is_single_opt_flag) son_exp->SaveKoopa_load(dest_str, save_mode);
        }

        void koopa_allocate() override {
            int is_single_opt_flag = is_single_add_flag;
            shared_ptr<BaseAST> son_exp = rel_exp;
            shared_ptr<BaseAST> opt_exp = add_exp;
            int allocate_num = 1;
            int backup = may_using_func_res;
            if (is_single_opt_flag && !may_using_func_res){
                may_using_func_res = 0;
            }else{
                may_using_func_res = 1;
            }
            if (is_single_opt_flag) {
                opt_exp->koopa_allocate();
                koopa_status = opt_exp->get_koopa_status();
                koopa_value = opt_exp->get_koopa_value();
            }else{
                son_exp->koopa_allocate();
                opt_exp->koopa_allocate();
                if (son_exp->get_koopa_status() && opt_exp->get_koopa_status()){
                    koopa_status = 1;
                    if (rel_operator == "<") {
                        koopa_value = son_exp->get_koopa_value() < opt_exp->get_koopa_value();
                    }else if (rel_operator == ">") {
                        koopa_value = son_exp->get_koopa_value() > opt_exp->get_koopa_value();
                    }else if (rel_operator == "<=") {
                        koopa_value = son_exp->get_koopa_value() <= opt_exp->get_koopa_value();
                    }else if (rel_operator == ">=") {
                        koopa_value = son_exp->get_koopa_value() >= opt_exp->get_koopa_value();
                    }else{
                        cout << "Unknown Rel-level operator " << rel_operator <<"!\n";
                        assert(false);
                    }
                }else{
                    koopa_status = 0;
                    current_max_register += allocate_num;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
            may_using_func_res = backup;
        }
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
        
};

class AddExpAST : public BaseAST {
    public:
        int is_single_mul_flag;
        shared_ptr<BaseAST> mul_exp;
        shared_ptr<BaseAST> add_exp;
        string add_operator;
        void Dump() const override {
            cout << "AddExpAST {\n";
            if (is_single_mul_flag){
                mul_exp->Dump();
            }else{
                add_exp->Dump();
                cout << "add_operator: " << add_operator << "\n";
                mul_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            int is_single_opt_flag = is_single_mul_flag;
            shared_ptr<BaseAST> opt_exp = mul_exp;
            shared_ptr<BaseAST> son_exp = add_exp;
            if (is_single_opt_flag) {
                return opt_exp->preprocess_variables();
            } else {
                if (add_operator == "+") {
                    return opt_exp->preprocess_variables() + son_exp->preprocess_variables();
                }else if (add_operator == "-") {
                    return son_exp->preprocess_variables() - opt_exp->preprocess_variables();
                }else{
                    cout << "Unknown Add-level operator " << add_operator <<"!\n";
                    assert(false);
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            int is_single_opt_flag = is_single_mul_flag;
            shared_ptr<BaseAST> opt_exp = mul_exp;
            shared_ptr<BaseAST> son_exp = add_exp;
            string koopa_operator_string;
            if (add_operator == "+"){
                koopa_operator_string = "add";
            }else if(add_operator == "-"){
                koopa_operator_string = "sub";
            }else{
                // cout << "Unknown Add-level operator " << add_operator << "!\n";
                // assert(false);
            }
            if (is_single_opt_flag){
                opt_exp->SaveKoopa(dest_str, save_mode);
            }else{
                if(!opt_exp->get_koopa_status()) opt_exp->SaveKoopa(dest_str, save_mode);
                if(!son_exp->get_koopa_status()) son_exp->SaveKoopa(dest_str, save_mode);
                string father_koopa_name = this->get_koopa_name();
                string opt_koopa_name = opt_exp->get_koopa_name();
                string son_koopa_name = son_exp->get_koopa_name();
                save_to(dest_str, "  "+father_koopa_name+" = "+koopa_operator_string+" "+son_koopa_name+", "+opt_koopa_name+"\n", save_mode);
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            int is_single_opt_flag = is_single_mul_flag;
            shared_ptr<BaseAST> opt_exp = mul_exp;
            shared_ptr<BaseAST> son_exp = add_exp;
            opt_exp->SaveKoopa_load(dest_str, save_mode);
            if (!is_single_opt_flag) son_exp->SaveKoopa_load(dest_str, save_mode);
        }

        void koopa_allocate() override {
            int is_single_opt_flag = is_single_mul_flag;
            shared_ptr<BaseAST> son_exp = add_exp;
            shared_ptr<BaseAST> opt_exp = mul_exp;
            int allocate_num = 1;
            int backup = may_using_func_res;
            if (is_single_opt_flag && !may_using_func_res){
                may_using_func_res = 0;
            }else{
                may_using_func_res = 1;
            }
            if (is_single_opt_flag) {
                opt_exp->koopa_allocate();
                koopa_status = opt_exp->get_koopa_status();
                koopa_value = opt_exp->get_koopa_value();
            }else{
                son_exp->koopa_allocate();
                opt_exp->koopa_allocate();
                if (son_exp->get_koopa_status() && opt_exp->get_koopa_status()){
                    koopa_status = 1;
                    if (add_operator == "+") {
                        koopa_value = son_exp->get_koopa_value() + opt_exp->get_koopa_value();
                    }else if (add_operator == "-") {
                        koopa_value = son_exp->get_koopa_value() - opt_exp->get_koopa_value();
                    }else{
                        cout << "Unknown Add-level operator " << add_operator <<"!\n";
                        assert(false);
                    }
                }else{
                    koopa_status = 0;
                    current_max_register += allocate_num;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
            may_using_func_res = backup;
        }
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
        
};

class MulExpAST : public BaseAST {
    public:
        int is_single_unary_flag;
        shared_ptr<BaseAST> mul_exp;
        shared_ptr<BaseAST> unary_exp;
        string mul_operator;
        void Dump() const override {
            cout << "MulExpAST {\n";
            if (is_single_unary_flag){
                unary_exp->Dump();
            }else{
                mul_exp->Dump();
                cout << "mul_operator: " << mul_operator << "\n";
                unary_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            int is_single_opt_flag = is_single_unary_flag;
            shared_ptr<BaseAST> opt_exp = unary_exp;
            shared_ptr<BaseAST> son_exp = mul_exp;
            if (is_single_opt_flag) {
                return opt_exp->preprocess_variables();
            } else {
                if (mul_operator == "*") {
                    return opt_exp->preprocess_variables() * son_exp->preprocess_variables();
                }else if (mul_operator == "/") {
                    return son_exp->preprocess_variables() / opt_exp->preprocess_variables();
                }else if (mul_operator == "%") {
                    return son_exp->preprocess_variables() % opt_exp->preprocess_variables();
                }else{
                    cout << "Unknown Mul-level operator " << mul_operator <<"!\n";
                    assert(false);
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            int is_single_opt_flag = is_single_unary_flag;
            shared_ptr<BaseAST> opt_exp = unary_exp;
            shared_ptr<BaseAST> son_exp = mul_exp;
            string koopa_operator_string;
            if (mul_operator == "*"){
                koopa_operator_string = "mul";
            }else if (mul_operator == "/"){
                koopa_operator_string = "div";
            }else if (mul_operator == "%"){
                koopa_operator_string = "mod";
            }else{
                // cout << "Unknown Mul-level operator " << mul_operator << "!\n"; 
                // assert(false);
            }
            if (is_single_opt_flag){
                opt_exp->SaveKoopa(dest_str, save_mode);
            }else{
                if(!opt_exp->get_koopa_status()) opt_exp->SaveKoopa(dest_str, save_mode);
                if(!son_exp->get_koopa_status()) son_exp->SaveKoopa(dest_str, save_mode);
                string father_koopa_name = this->get_koopa_name();
                string opt_koopa_name = opt_exp->get_koopa_name();
                string son_koopa_name = son_exp->get_koopa_name();
                save_to(dest_str, "  "+father_koopa_name+" = "+koopa_operator_string+" "+son_koopa_name+", "+opt_koopa_name+"\n", save_mode);
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            int is_single_opt_flag = is_single_unary_flag;
            shared_ptr<BaseAST> opt_exp = unary_exp;
            shared_ptr<BaseAST> son_exp = mul_exp;
            opt_exp->SaveKoopa_load(dest_str, save_mode);
            if (!is_single_opt_flag) son_exp->SaveKoopa_load(dest_str, save_mode);
        }

        void koopa_allocate() override {
            int is_single_opt_flag = is_single_unary_flag;
            shared_ptr<BaseAST> son_exp = mul_exp;
            shared_ptr<BaseAST> opt_exp = unary_exp;
            int allocate_num = 1;
            int backup = may_using_func_res;
            if (is_single_opt_flag && !may_using_func_res){
                may_using_func_res = 0;
            }else{
                may_using_func_res = 1;
            }
            if (is_single_opt_flag) {
                opt_exp->koopa_allocate();
                koopa_status = opt_exp->get_koopa_status();
                koopa_value = opt_exp->get_koopa_value();
            }else{
                son_exp->koopa_allocate();
                opt_exp->koopa_allocate();
                if (son_exp->get_koopa_status() && opt_exp->get_koopa_status()){
                    koopa_status = 1;
                    if (mul_operator == "*") {
                        koopa_value = son_exp->get_koopa_value() * opt_exp->get_koopa_value();
                    }else if (mul_operator == "/") {
                        koopa_value = son_exp->get_koopa_value() / opt_exp->get_koopa_value();
                    }else if (mul_operator == "%") {
                        koopa_value = son_exp->get_koopa_value() % opt_exp->get_koopa_value();
                    }else{
                        cout << "Unknown Mul-level operator " << mul_operator <<"!\n";
                        assert(false);
                    }
                }else{
                    koopa_status = 0;
                    current_max_register += allocate_num;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
            may_using_func_res = backup;
        }
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class UnaryExpAST : public BaseAST {
    public:
        int is_primary_flag;
        int is_func_res_flag;
        int func_res_used;
        int has_funcrparams;
        string func_name;
        vector<shared_ptr<BaseAST>> funcrparams;
        shared_ptr<BaseAST> unary_op;
        shared_ptr<BaseAST> unary_exp;
        // 需要注意，由于语法分析阶段对+ UnaryExp的特殊处理，这里primary_exp有可能指向一个UnaryExpAST的实例
        shared_ptr<BaseAST> primary_exp; 
        void Dump() const override {
            cout << "UnaryExpAST {\n";
            if (is_primary_flag){
                primary_exp->Dump();
            }else{
                unary_op->Dump();
                unary_exp->Dump();
            }
            cout << "}\n";
        }

        int preprocess_variables() override {
            if (is_func_res_flag) {
                return 0;
            }else if (is_primary_flag) {
                return primary_exp->preprocess_variables();
            }else{
                if (unary_op->get_unary_op() == "+") {
                    return unary_exp->preprocess_variables();
                }else if (unary_op->get_unary_op() == "-") {
                    return  - unary_exp->preprocess_variables();
                }else if (unary_op->get_unary_op() == "!") {
                    return ! unary_exp->preprocess_variables();
                }else{
                    cout << "Unknown Unary operator " << unary_op->get_unary_op() <<"!\n";
                    assert(false);
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            assert(koopa_status == 0);
            if (is_func_res_flag) {
                if (has_funcrparams) {
                    string backup = currently_dealt_btype;
                    for (size_t i = 0; i < funcrparams.size(); i += 1) {
                        currently_dealt_btype = funcparam_note.check(func_name, i);
                        if (!funcrparams[i]->get_koopa_status()) funcrparams[i]->SaveKoopa(dest_str, save_mode);
                    }
                    currently_dealt_btype = backup;
                }

                if (func_res_used) {
                    if (is_special_func_name(func_name)){
                        save_to(dest_str, "  "+get_koopa_name()+" = "+"call @"+func_name+"(", save_mode);
                    }else{
                        save_to(dest_str, "  "+get_koopa_name()+" = "+"call @"+func_name+"_func_name(", save_mode);
                    }
                    
                }else{
                    if (is_special_func_name(func_name)) {
                        save_to(dest_str, "  call @"+func_name+"(", save_mode);
                    }else{
                        save_to(dest_str, "  call @"+func_name+"_func_name(", save_mode);
                    }
                }
                
                if (has_funcrparams) {
                    for (size_t i = 0; i < funcrparams.size(); i += 1) {
                        if (i) save_to(dest_str, ", ", save_mode);
                        save_to(dest_str, funcrparams[i]->get_koopa_name(), save_mode);
                    }
                }
                save_to(dest_str, ")\n", save_mode);
            }else if (is_primary_flag){
                primary_exp->SaveKoopa(dest_str, save_mode);
            }else{
                if(!unary_exp->get_koopa_status()) unary_exp->SaveKoopa(dest_str, save_mode);
                if(unary_op->get_unary_op() == "+"){
                    save_to(dest_str, "  ",save_mode);
                    save_to(dest_str,this->get_koopa_name() ,save_mode);
                    save_to(dest_str, " = ",save_mode);
                    save_to(dest_str, unary_exp->get_koopa_name(),save_mode);
                    save_to(dest_str,"\n" ,save_mode);
                }else if (unary_op->get_unary_op() == "-"){
                    save_to(dest_str, "  ",save_mode);
                    save_to(dest_str, this->get_koopa_name(),save_mode);
                    save_to(dest_str," = " ,save_mode);
                    save_to(dest_str, "sub 0, ",save_mode);
                    save_to(dest_str, unary_exp->get_koopa_name(),save_mode);
                    save_to(dest_str, "\n",save_mode);
                }else if(unary_op->get_unary_op() == "!"){
                    save_to(dest_str,"  " ,save_mode);
                    save_to(dest_str,this->get_koopa_name() ,save_mode);
                    save_to(dest_str, " = ",save_mode);
                    save_to(dest_str,"eq 0, " ,save_mode);
                    save_to(dest_str,unary_exp->get_koopa_name() ,save_mode);
                    save_to(dest_str, "\n" ,save_mode);
                }else{
                    cout << "Unknown Unary operator " << unary_op->get_unary_op() <<"!\n";
                    assert(false);
                }
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            if (is_func_res_flag) {
                if (has_funcrparams) {
                    string backup = currently_dealt_btype;
                    for (size_t i = 0; i < funcrparams.size(); i += 1) {
                        currently_dealt_btype = funcparam_note.check(func_name, i);
                        funcrparams[i]->SaveKoopa_load(dest_str, save_mode);
                    }
                    currently_dealt_btype = backup;
                }
            }else if (is_primary_flag) {
                primary_exp->SaveKoopa_load(dest_str, save_mode);
            }else{
                unary_exp->SaveKoopa_load(dest_str, save_mode);
            }
        }
   
        void koopa_allocate() override {
            if (is_func_res_flag) {
                if (has_funcrparams) {
                    for (size_t i = 0; i < funcrparams.size(); i += 1) {
                        int backup = may_using_func_res;
                        may_using_func_res = 1;
                        funcrparams[i]->koopa_allocate();
                        may_using_func_res = backup;
                    }
                }
                func_res_used = may_using_func_res;
                koopa_status = 0;
                if (func_res_used) {
                    current_max_register += 1;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }else if (is_primary_flag) {
                primary_exp->koopa_allocate();
                koopa_status = primary_exp->get_koopa_status();
                koopa_value = primary_exp->get_koopa_value();
            }else{
                unary_exp->koopa_allocate();
                if (unary_exp->get_koopa_status()){
                    koopa_status = 1;
                    if (unary_op->get_unary_op() == "+") {
                        koopa_value = unary_exp->get_koopa_value();
                    }else if (unary_op->get_unary_op() == "-") {
                        koopa_value = - unary_exp->get_koopa_value();
                    }else if (unary_op->get_unary_op() == "!") {
                        koopa_value = ! unary_exp->get_koopa_value();
                    }else{
                        cout << "Unknown Unary operator " << unary_op->get_unary_op() <<"!\n";
                        assert(false);
                    }
                }else{
                    koopa_status = 0;
                    current_max_register += 1;
                    assert(current_max_register >= -1);
                    koopa_value = current_max_register;
                }
            }
        }
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class PrimaryExpAST : public BaseAST {
    public:
        int is_single_number_flag;
        int is_lval;
        int number;
        shared_ptr<BaseAST> exp;
        shared_ptr<BaseAST> lval;
        void Dump() const override {
            cout << "PrimaryExpAST {\n";
            if (is_lval){
                lval->Dump();
            }else{
                if (is_single_number_flag){
                    cout << "number: "<<number<<"\n";
                }else{
                    exp->Dump();
                }
            }
            
            cout << "}\n";
        }
        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (is_lval) {
                if (lval->get_has_index_flag()){
                    lval->SaveKoopa(dest_str, save_mode);
                }else{

                }
            }else{
                if (is_single_number_flag) {

                }else{
                    exp->SaveKoopa(dest_str, save_mode);
                }
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            if (is_lval) {
                lval->SaveKoopa_load(dest_str, save_mode);
            }else{
                if (is_single_number_flag) {

                }else{
                    exp->SaveKoopa_load(dest_str, save_mode);
                }
            }
        }

        int preprocess_variables() override {
            if (is_single_number_flag) {
                return number;
            }else if (is_lval) {
                return lval->preprocess_variables();
            }else{
                return exp->preprocess_variables();
            }
        }

        void koopa_allocate() override {
            if (is_single_number_flag) {
                koopa_status = 1;
                koopa_value = number;
            }else if (is_lval) {
                lval->koopa_allocate();
                koopa_status = lval->get_koopa_status();
                koopa_value = lval->get_koopa_value();
            }else{
                exp->koopa_allocate();
                koopa_status = exp->get_koopa_status();
                koopa_value = exp->get_koopa_value();
            }
        }
    
        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_koopa_status() override {
            return koopa_status;
        }
        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};

class UnaryOpAST : public BaseAST {
    public:
        char unary_op;
        void Dump() const override {
            cout << "unary_op: "<< unary_op<<"\n";
        }

        string get_unary_op() override {
            return string(1, unary_op);
        }
};

class LValAST : public BaseAST {
    public:
        string ident;
        int has_index_flag;
        vector<shared_ptr<BaseAST>> indexes;
        string stack_position;
        bool is_constant = false;
        int called_from_rhs;
        int allocate_num;
        int first_time_protection;
        int forward_count_num;

        void Dump() const override {
            cout << "ident: " << ident << endl;
        }

        int preprocess_variables() override {
            if (symbol_table_list.exists(ident) && symbol_table_list.is_constant(ident)) {
                is_constant = true;
                return symbol_table_list.get_value(ident);
            }else{
                cout << "Invalid definition of constant!\n" << endl;
                assert(false);
                return 0;
            }
        }

        void koopa_allocate() override {
            if (has_index_flag) {
                stack_position = symbol_table_list.get_stack_position(ident);
                for (int i = 0; i < array_dimension_note[stack_position].size() - indexes.size(); i += 1) {
                    // 造假的expast
                    auto expast = new ExpAST();
                    expast->is_pseudo_flag = 1;
                    indexes.push_back(shared_ptr<BaseAST>(expast));
                }
                called_from_rhs = lval_called_from_rhs;
                lval_called_from_rhs = 1;
                allocate_num = 1 + called_from_rhs + (int)(indexes.size() - 1);
                for (size_t i = indexes.size()-1; i != (size_t)-1; i -= 1) {
                    indexes[i]->koopa_allocate();
                    if (!indexes[i]->get_koopa_status()) allocate_num += 1;
                }
                lval_called_from_rhs = called_from_rhs;
                koopa_status = 0;
                current_max_register += allocate_num;
                koopa_value = current_max_register;
            }else{
                if (symbol_table_list.exists(ident)) {
                    if (symbol_table_list.is_constant(ident)){
                        koopa_status = 1;
                        koopa_value = symbol_table_list.get_value(ident);
                        is_constant = true;
                    }else{
                        koopa_status = 0;
                        stack_position = symbol_table_list.get_stack_position(ident);
                        current_max_register += lval_called_from_rhs;
                        assert(current_max_register >= -1);
                        koopa_value = current_max_register;
                    }
                }else{
                    cout << "Symbol " << ident << " used before declared!" << endl;
                    // assert(false);
                }
            }
        }

        void SaveKoopa_load(string& dest_str, const string save_mode)override {
            if (has_index_flag) {
                string backup = currently_dealt_btype;
                currently_dealt_btype = "int";
                indexes[indexes.size()-1]->SaveKoopa_load(dest_str, save_mode);
                if (!indexes[indexes.size()-1]->get_koopa_status()) indexes[indexes.size()-1]->SaveKoopa(dest_str, save_mode);
                forward_count_num = allocate_num - 1;
                vector<int> array_dimensions = array_dimension_note[stack_position];
                first_time_protection = 1;
                for (size_t i = indexes.size()- 2; i != -1; i -= 1) {
                    indexes[i]->SaveKoopa_load(dest_str, save_mode);
                    if (!indexes[i]->get_koopa_status()){
                        indexes[i]->SaveKoopa(dest_str, save_mode);
                        save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = mul "+indexes[i]->get_koopa_name()+", "+to_string(array_dimensions[(int)(i+1)])+"\n", save_mode);
                        forward_count_num -= 1;
                        if (first_time_protection) {
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = add "+forward_count(get_koopa_name(),forward_count_num+1)+", "+indexes[indexes.size()-1]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = add "+forward_count(get_koopa_name(),forward_count_num+1)+", "+forward_count(get_koopa_name(),forward_count_num+1+(indexes[i]->get_koopa_status()==0))+"\n", save_mode);
                        }
                        forward_count_num -= 1;
                    }else{
                        if (first_time_protection) {
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = add "+to_string(indexes[i]->get_koopa_value()*array_dimensions[(int)(i+1)])+", "+indexes[indexes.size()-1]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = add "+to_string(indexes[i]->get_koopa_value()*array_dimensions[(int)(i+1)])+", "+forward_count(get_koopa_name(),forward_count_num+1+(indexes[i]->get_koopa_status()==0))+"\n", save_mode);
                        }
                        forward_count_num -= 1;
                    }
                    first_time_protection = 0;
                }
                if(!called_from_rhs) return;

                if (backup == "int") {
                    if (is_int_ptr[stack_position]) {
                        if (first_time_protection) {
                            assert(indexes.size() == 1);
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = getptr "+stack_position+", "+indexes[0]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = getptr "+stack_position+", "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                        }
                    }else{
                        if (first_time_protection) {
                            assert(indexes.size() == 1);
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = getelemptr "+stack_position+", "+indexes[0]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+forward_count(get_koopa_name(),forward_count_num)+" = getelemptr "+stack_position+", "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                        }
                    }
                    forward_count_num -= 1;
                    save_to(dest_str, "  "+get_koopa_name()+" = load "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                }else{
                    if (is_int_ptr[stack_position]) {
                        if (first_time_protection) {
                            assert(indexes.size() == 1);
                            save_to(dest_str, "  "+get_koopa_name()+" = getptr "+stack_position+", "+indexes[0]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+get_koopa_name()+" = getptr "+stack_position+", "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                        }
                    }else{
                        if (first_time_protection) {
                            assert(indexes.size() == 1);
                            save_to(dest_str, "  "+get_koopa_name()+" = getelemptr "+stack_position+", "+indexes[0]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+get_koopa_name()+" = getelemptr "+stack_position+", "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                        }
                    }
                    forward_count_num -= 1;
                }
                currently_dealt_btype = backup;
            }else{
                if (currently_dealt_btype == "int array"){
                    if (is_int_ptr[stack_position]) {
                        save_to(dest_str, "  "+get_koopa_name()+" = getptr "+stack_position+", 0\n", save_mode);
                    }else{
                        save_to(dest_str, "  "+get_koopa_name()+" = getelemptr "+stack_position+", 0\n", save_mode);
                    }
                    
                }else{
                    if (!is_constant) {
                        save_to(dest_str, "  "+get_koopa_name()+" = load "+stack_position+"\n", save_mode);
                    }
                }
            }
        }

        void SaveKoopa(string& dest_str, const string save_mode) const override {
            if (has_index_flag) {
                if (called_from_rhs) {

                }else{
                    if (is_int_ptr[stack_position]) {
                        if (first_time_protection) {
                            assert(indexes.size() == 1);
                            save_to(dest_str, "  "+get_koopa_name()+" = getptr "+stack_position+", "+indexes[0]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+get_koopa_name()+" = getptr "+stack_position+", "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                        }
                        
                    }else{
                        if (first_time_protection) {
                            assert(indexes.size() == 1);
                            save_to(dest_str, "  "+get_koopa_name()+" = getelemptr "+stack_position+", "+indexes[0]->get_koopa_name()+"\n", save_mode);
                        }else{
                            save_to(dest_str, "  "+get_koopa_name()+" = getelemptr "+stack_position+", "+forward_count(get_koopa_name(),forward_count_num+1)+"\n", save_mode);
                        }
                    }
                }
            }else{

            }
        }

        string get_stack_position() override {
            return stack_position;
        }

        void set_stack_position(string position) override {
            stack_position = position;
        } 

        string get_ident() override {
            return ident;
        }

        int get_koopa_value() const override {
            return koopa_value;
        }

        int get_has_index_flag() override {
            return has_index_flag;
        }

        int get_koopa_status() override {
            return koopa_status;
        }

        string get_koopa_name() const override {
            if (koopa_status){
                return to_string(koopa_value);
            }else{
                return "%" + to_string(koopa_value);
            }
        }
};



