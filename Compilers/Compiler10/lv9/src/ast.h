#pragma once
#include <string>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <assert.h>
#include <fstream>
#include <vector>
using namespace std;
#define PRIMARYEXP_TYPE_EXP 1
#define PRIMARYEXP_TYPE_NUMBER 2
#define PRIMARYEXP_TYPE_LVAL 3
#define UNARYEXP_TYPE_PRIMARY 1
#define UNARYEXP_TYPE_OP_UNARY 2
#define UNARYEXP_TYPE_FUNC 3
#define MULEXP_TYPE_UNARY 1
#define MULEXP_TYPE_COMPLEX 2
#define ADDEXP_TYPE_MUL 1
#define ADDEXP_TYPE_COMPLEX 2
#define RELEXP_TYPE_ADD 1
#define RELEXP_TYPE_COMPLEX 2
#define EQEXP_TYPE_REL 1
#define EQEXP_TYPE_COMPLEX 2
#define LANDEXP_TYPE_EQ 1
#define LANDEXP_TYPE_COMPLEX 2
#define LOREXP_TYPE_LAND 1
#define LOREXP_TYPE_COMPLEX 2
#define BLOCKITEM_TYPE_STMT 1
#define BLOCKITEM_TYPE_DECL 2
#define INITVAL_TYPE_EXP 1
#define INITVAL_TYPE_LIST 2
#define INITEXP_TYPE_EXP 1
#define INITEXP_TYPE_ZERO 2
#define STMT_TYPE_EXP 1
#define STMT_TYPE_RET 2
#define DTYPE_INT 1
#define GE 'a'
#define LE 'b'
#define EQ 'c'
#define NE 'd'
#define GT 'e'
#define LT 'f'

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

static int tmp_id = 0;
static unordered_map<int, int> inst_num_map;
static int bb_transed_flag = 0;
static int global_ret_id = 0;
enum class VarType {
    CONST,
    VAR,
    ARR
};
struct var_info{
    VarType type;
    int val;
};
static int global_var_id = 0;
static int global_if_id = 0;
static int global_while_id = 0;
static int global_expout_id = 0;
static int global_break_id = 0;
static int global_continue_id = 0;
static int global_expend_id = 0;
static unordered_map<string, vector<int> > array_size_map;
class Envstack{
    vector<unordered_map<string, pair<var_info, string> > > symbol_table;
    public:
    void addblock(){
        symbol_table.push_back(unordered_map<string, pair<var_info, string> >());
    }
    void delblock(){
        symbol_table.pop_back();
    }
    void addvar(string name, var_info info){
        //printf("addvar %s\n", name.c_str());
        unordered_map<string, pair<var_info, string>>& now_symbol_table = symbol_table[symbol_table.size()-1];
        assert(now_symbol_table.count(name) == 0);
        now_symbol_table[name] = {info, name + string_format("_%d", ++global_var_id)};
    }
    var_info getvar(string name){
        //printf("getvar %s\n", name.c_str());
        for(int i = symbol_table.size()-1; i >= 0; i--){
            if(symbol_table[i].count(name) > 0){
                return symbol_table[i][name].first;
            }
        }
        assert(false);
    }
    pair<var_info, string> getvar_withalias(string name){
        //printf("getvar %s\n", name.c_str());
        for(int i = symbol_table.size()-1; i >= 0; i--){
            if(symbol_table[i].count(name) > 0){
                return symbol_table[i][name];
            }
        }
        assert(false);
    }
    int depth(){
        return symbol_table.size();
    }
};
static Envstack envstack;
static vector<string> shortcut_var_stack;
#define IS_INST_NUM(id) (inst_num_map.count(id) > 0)
#define GET_ID_STR(id) (IS_INST_NUM(id) ? string_format("%d", inst_num_map[id]).c_str() : string_format("%%%d", id).c_str())

// 所有 AST 的基类
class BaseAST {
    public:
        virtual ~BaseAST() = default;
        virtual unique_ptr<string> GetIR(){
            return NULL;
        }
        virtual unique_ptr<string> GetIR(int &retid){
            return NULL;
        }
        virtual unique_ptr<string> GetIR(int &retid,const string &endblock){
            return NULL;
        }
        virtual unique_ptr<string> GetIR(int &retid,const string &trueblock, const string &falseblock){
            return NULL;
        }
        virtual int getval(){
            return 0;
        }
};

// class CompUnitAST : public BaseAST {
//     public:
//         // 用智能指针管理对象
//         unique_ptr<string> compunitir;
//         virtual unique_ptr<string> GetIR() override {
//             return unique_ptr<string>(new string(compunitir->c_str()));
//         }
// };

class FuncFParamsAST : public BaseAST {
    public:
        vector<BaseAST*> fparams;
};

class FuncFParamAST : public BaseAST {
    public:
        string ident;
        string type;
};

class FuncTypeAST : public BaseAST {
    public:
        std::string type;
};

class FuncAST: public BaseAST{
    // may check type here, not implemented
    public:
        FuncTypeAST* functype;
        FuncFParamsAST* funcfparams;
        FuncAST(){
            functype = new FuncTypeAST();
            funcfparams = new FuncFParamsAST();
        }
        FuncAST(string type, vector<string> fparams){
            functype = new FuncTypeAST();
            functype->type = type;
            funcfparams = new FuncFParamsAST();
            for(auto &fparam : fparams){
                auto fparam_ast = new FuncFParamAST();
                fparam_ast->ident = fparam;
                fparam_ast->type = "I don't know";
                funcfparams->fparams.push_back(fparam_ast);
            }
        }
};

class GlobalFuncTable{
    public:
        unordered_map<string, pair<unique_ptr<FuncAST>, string> > func_table;
        GlobalFuncTable(){
            func_table = unordered_map<string, pair<unique_ptr<FuncAST>, string> >();
            auto getint_func_ast = make_unique<FuncAST>("int", vector<string>());
            addfunc("getint", move(getint_func_ast));
            auto getch_func_ast = make_unique<FuncAST>("int", vector<string>());
            addfunc("getch", move(getch_func_ast));
            auto getarray_func_ast = make_unique<FuncAST>("int", vector<string>{"a"});
            addfunc("getarray", move(getarray_func_ast));
            auto putint_func_ast = make_unique<FuncAST>("void", vector<string>{"a"});
            addfunc("putint", move(putint_func_ast));
            auto putch_func_ast = make_unique<FuncAST>("void", vector<string>{"a"});
            addfunc("putch", move(putch_func_ast));
            auto putarray_func_ast = make_unique<FuncAST>("void", vector<string>{"a", "b"});
            addfunc("putarray", move(putarray_func_ast));
            auto starttime_func_ast = make_unique<FuncAST>("void", vector<string>());
            addfunc("starttime", move(starttime_func_ast));
            auto stoptime_func_ast = make_unique<FuncAST>("void", vector<string>());
            addfunc("stoptime", move(stoptime_func_ast));
        }
        void addfunc(string name, unique_ptr<FuncAST> func){
            auto alias = name+string_format("_%d", func->funcfparams->fparams.size());
            assert(func_table.count(alias) == 0);
            func_table[alias] = {move(func), name};
        }
        unique_ptr<FuncAST>& getfunc(string name, int params_num){
            auto alias = name+string_format("_%d", params_num);
            assert(func_table.count(alias) > 0);
            return func_table[alias].first;
        }
};

static GlobalFuncTable global_func_table;

class BlockAST : public BaseAST {
    public:
        unique_ptr<string> blockir;

        virtual unique_ptr<string> GetIR() override {
            return unique_ptr<string>(new string(blockir->c_str()));
        }
};

class StmtAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> exp;
        int type;
        virtual unique_ptr<string> GetIR() override {
            int retid = 0;
            unique_ptr<string> retstr = exp->GetIR(retid);
            if(type == STMT_TYPE_EXP){
                return retstr;
            }
            if(IS_INST_NUM(retid)){
                int new_id = ++tmp_id;
                retstr->append( string_format("  %%%d = add 0, %s\n", new_id, GET_ID_STR(retid)));
                retid = new_id;
            }
            retstr->append( string_format("  ret %%%d\n", retid));
            return retstr;
        }
};

class LAndExpAST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> eqExp;
        std::unique_ptr<BaseAST> landExp;
        int type;
        virtual unique_ptr<string> GetIR(int &retid, const string& endblock) override {
            if (type == LANDEXP_TYPE_EQ) {
                unique_ptr<string> retstr = eqExp->GetIR(retid);
                //retstr->append( string_format("  jump %s\n", endblock.c_str()));
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                retid = ++tmp_id;
                string exp_trueblock = "%landexp_true_" + string_format("%d", ++global_expout_id);
                string exp_falseblock = "%landexp_false_" + string_format("%d", global_expout_id);
                unique_ptr<string> lhsstr = landExp->GetIR(lhs_id, exp_trueblock, exp_falseblock);
                lhsstr->append( string_format("  br %s, %s, %s\n", GET_ID_STR(lhs_id), exp_trueblock.c_str(), exp_falseblock.c_str()));
                lhsstr->append( string_format("%s:\n", exp_falseblock.c_str()));
                lhsstr->append( string_format("  store 0, @%s\n", shortcut_var_stack.back().c_str()));
                lhsstr->append( string_format("  jump %s\n", endblock.c_str()));
                lhsstr->append( string_format("%s:\n", exp_trueblock.c_str()));
                unique_ptr<string> rhsstr = eqExp->GetIR(rhs_id);
                lhsstr->append(rhsstr->c_str());
                lhsstr->append( string_format("  %s = ne %s, 0\n", GET_ID_STR(retid), GET_ID_STR(rhs_id)));
                lhsstr->append( string_format("  store %s, @%s\n", GET_ID_STR(retid), shortcut_var_stack.back().c_str()));
                return lhsstr;
            }
        }
        virtual unique_ptr<string> GetIR(int &retid, const string &trueblock, const string &falseblock) override {
            if (type == LANDEXP_TYPE_EQ) {
                unique_ptr<string> retstr = eqExp->GetIR(retid);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                string exp_trueblock = "%landexp_true_" + string_format("%d", ++global_expout_id);
                unique_ptr<string> lhsstr = landExp->GetIR(lhs_id, exp_trueblock, falseblock);
                lhsstr->append( string_format("  br %s, %s, %s\n", GET_ID_STR(lhs_id), exp_trueblock.c_str(), falseblock.c_str()));
                lhsstr->append( string_format("%s:\n", exp_trueblock.c_str()));
                unique_ptr<string> rhsstr = eqExp->GetIR(rhs_id);
                lhsstr->append(rhsstr->c_str());
                lhsstr->append( string_format("  %s = ne %s, 0\n", GET_ID_STR(tmp_id+1), GET_ID_STR(rhs_id)));
                retid = tmp_id+1; tmp_id += 1;
                return lhsstr;
            }
        }
        virtual int getval() override {
            if(type == LANDEXP_TYPE_EQ){
                return eqExp->getval();
            } else {
                return landExp->getval() && eqExp->getval();
            }
        }
};

class LOrExpAST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> landExp;
        std::unique_ptr<BaseAST> lorExp;
        int type;
        virtual unique_ptr<string> GetIR(int &retid, const string &endblock) override {
            if (type == LOREXP_TYPE_LAND) {
                unique_ptr<string> retstr = landExp->GetIR(retid, endblock);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                retid = ++tmp_id;
                string exp_falseblock = "%lorexp_false_" + string_format("%d", ++global_expout_id);
                string exp_trueblock = "%lorexp_true_" + string_format("%d", global_expout_id);
                unique_ptr<string> lhsstr = lorExp->GetIR(lhs_id, exp_trueblock, exp_falseblock);
                lhsstr->append( string_format("  br %s, %s, %s\n", GET_ID_STR(lhs_id), exp_trueblock.c_str(), exp_falseblock.c_str()));
                lhsstr->append( string_format("%s:\n", exp_trueblock.c_str()));
                lhsstr->append( string_format("  store 1, @%s\n", shortcut_var_stack.back().c_str()));
                lhsstr->append( string_format("  jump %s\n", endblock.c_str()));
                lhsstr->append( string_format("%s:\n", exp_falseblock.c_str()));
                unique_ptr<string> rhsstr = landExp->GetIR(rhs_id, endblock);
                lhsstr->append(rhsstr->c_str());
                lhsstr->append( string_format("  %s = ne %s, 0\n", GET_ID_STR(retid), GET_ID_STR(rhs_id)));
                lhsstr->append( string_format("  store %s, @%s\n", GET_ID_STR(retid), shortcut_var_stack.back().c_str()));
                return lhsstr;
            }
        }
        virtual unique_ptr<string> GetIR(int &retid, const string &trueblock, const string &falseblock) override {
            if (type == LOREXP_TYPE_LAND) {
                unique_ptr<string> retstr = landExp->GetIR(retid, trueblock, falseblock);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                string exp_falseblock = "%lorexp_false_" + string_format("%d", ++global_expout_id);
                unique_ptr<string> lhsstr = lorExp->GetIR(lhs_id, trueblock, exp_falseblock);
                lhsstr->append( string_format("  br %s, %s, %s\n", GET_ID_STR(lhs_id), trueblock.c_str(), exp_falseblock.c_str()));
                lhsstr->append( string_format("%s:\n", exp_falseblock.c_str()));
                unique_ptr<string> rhsstr = landExp->GetIR(rhs_id, trueblock, falseblock);
                lhsstr->append(rhsstr->c_str());
                lhsstr->append( string_format("  %s = ne %s, 0\n", GET_ID_STR(tmp_id+1), GET_ID_STR(rhs_id)));
                retid = tmp_id+1; tmp_id += 1;
                return lhsstr;
            }
        }
        virtual int getval() override {
            if(type == LOREXP_TYPE_LAND){
                return landExp->getval();
            } else {
                return landExp->getval() || lorExp->getval();
            }
        }
};



class ExpAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> lorExp;
        virtual unique_ptr<string> GetIR(int &retid) override {
            string endblock = string_format("%%expend_%d", ++global_expend_id);
            if(!(dynamic_cast<LOrExpAST *>(lorExp.get())->type == LOREXP_TYPE_LAND &&  dynamic_cast<LAndExpAST* >((dynamic_cast<LOrExpAST *>(lorExp.get())->landExp).get())->type == LANDEXP_TYPE_EQ)){
                shortcut_var_stack.push_back(string_format("endvalue_%d", global_expend_id));
                unique_ptr<string> retstr = make_unique<string>(string_format("  @%s = alloc i32\n", shortcut_var_stack.back().c_str()));
                retstr->append(lorExp->GetIR(retid, endblock)->c_str());
                retstr->append( string_format("  jump %s\n", endblock.c_str()));
                retstr->append(string_format("%s:\n", endblock.c_str()));
                retid = ++tmp_id;
                retstr->append( string_format("  %%%d = load @%s\n", retid, shortcut_var_stack.back().c_str()));
                shortcut_var_stack.pop_back();
                return retstr;
            }else{

                return lorExp->GetIR(retid, endblock);
            }
        }
        virtual unique_ptr<string> GetIR(int &retid, const string &trueblock, const string &falseblock) override {
            unique_ptr<string> retstr = lorExp->GetIR(retid, trueblock, falseblock);
            return retstr;
        }
        virtual int getval() override {
            return lorExp->getval();
        }
};

class FuncRParamsAST : public BaseAST {
    public:
        vector<std::unique_ptr<BaseAST> > rparams;
};

class UnaryExpAST : public BaseAST {
    public:
        char op;
        std::unique_ptr<BaseAST> primaryExp;
        std::unique_ptr<BaseAST> unaryExp;
        std::unique_ptr<BaseAST> funcrparams;
        string ident;
        int type;
        virtual unique_ptr<string> GetIR(int &retid) override {
            if (type == UNARYEXP_TYPE_PRIMARY) {
                unique_ptr<string> retstr = primaryExp->GetIR(retid);
                return retstr;
            } else if(type == UNARYEXP_TYPE_OP_UNARY){
                unique_ptr<string> retstr = unaryExp->GetIR(retid);
                int new_id = ++tmp_id;
                if(op == '!'){
                    retstr->append( string_format("  %s = eq %s, 0\n", GET_ID_STR(new_id), GET_ID_STR(retid)));
                    retid = new_id;
                } else if(op == '-'){
                    retstr->append( string_format("  %s= sub 0, %s\n", GET_ID_STR(new_id), GET_ID_STR(retid)));
                    retid = new_id;
                } else if(op == '+'){
                    tmp_id--;
                }
                return retstr;
            }else{
                vector<int> func_call_params;
                unique_ptr<string> retstr = make_unique<string>("");
                for(auto& rparam: dynamic_cast<FuncRParamsAST *>(funcrparams.get())->rparams){
                    int exp_retid = 0;
                    retstr->append(dynamic_cast<ExpAST *>(rparam.get())->GetIR(exp_retid)->c_str());
                    func_call_params.push_back(exp_retid);
                }
                unique_ptr<FuncAST>& func = global_func_table.getfunc(ident, func_call_params.size());
                if(func->functype->type == "int"){
                    int new_id = ++tmp_id;
                    retstr->append( string_format("  %%%d = call @%s(", new_id, ident.c_str()));
                    for(auto& param: func_call_params){
                        retstr->append(string_format("%s", GET_ID_STR(param)));
                        if(param != func_call_params.back()){
                            retstr->append(", ");
                        }
                    }
                    retstr->append(")\n");
                    retid = new_id;
                } else {
                    retstr->append( string_format("  call @%s(", ident.c_str()));
                    for(auto& param: func_call_params){
                        retstr->append(string_format("%s", GET_ID_STR(param)));
                        if(param != func_call_params.back()){
                            retstr->append(", ");
                        }
                    }
                    retstr->append(")\n");
                    retid = -1; // void function, should not be used
                }
                return retstr;
            }
        }
        virtual int getval() override {
            if(type == UNARYEXP_TYPE_PRIMARY){
                return primaryExp->getval();
            } else {
                if(op == '!'){
                    return !unaryExp->getval();
                } else if(op == '-'){
                    return -unaryExp->getval();
                } else if(op == '+'){
                    return unaryExp->getval();
                } else { assert (false); }
            }
        }
};

static inline int calc_tot_size(vector<int> size){
    int ret = 1;
    for(auto& s: size){
        ret *= s;
    }
    return ret;
}

class LValAST : public BaseAST {
    public:
        std::string ident;
        vector<ExpAST*> exps;
        virtual unique_ptr<string> GetIR(int &retid) override {
            unique_ptr<string> retstr = make_unique<string>("");
            string alias = envstack.getvar_withalias(ident).second;
            vector<int> array_size = array_size_map[alias];
            int ptr_id = 0;
            if(exps.size() == 0){
                if(array_size[0] < 0){
                    ptr_id = ++tmp_id;
                    retstr->append( string_format("  %s = getptr @%s, 0\n", GET_ID_STR(ptr_id), alias.c_str()));
                }else{
                    ptr_id = ++tmp_id;
                    retstr->append( string_format("  %s = getptr @%s, 0\n", GET_ID_STR(ptr_id), alias.c_str()));
                }
                retid = ptr_id;
                return retstr;
            }
            if(array_size[0] < 0){
                int new_id = ++tmp_id;
                retstr->append( string_format("  %s = load @%s\n", GET_ID_STR(new_id), alias.c_str()));
                int first_exp_id = 0;
                retstr->append(exps[0]->GetIR(first_exp_id)->c_str());
                ptr_id = ++tmp_id;
                retstr->append( string_format("  %s = getptr %s, %s\n", GET_ID_STR(ptr_id), GET_ID_STR(new_id), GET_ID_STR(first_exp_id)));
            }else{
                ptr_id = ++tmp_id;
                int first_exp_id = 0;
                retstr->append(exps[0]->GetIR(first_exp_id)->c_str());
                retstr->append( string_format("  %s = getelemptr @%s, %s\n", GET_ID_STR(ptr_id), alias.c_str(), GET_ID_STR(first_exp_id)));
            }
            for(int i = 1;i < exps.size(); i++){
                int new_id = ++tmp_id;
                retstr->append(exps[i]->GetIR(new_id)->c_str());
                int ptr_id2 = ++tmp_id;
                retstr->append( string_format("  %s = getelemptr %s, %s\n", GET_ID_STR(ptr_id2), GET_ID_STR(ptr_id), GET_ID_STR(new_id)));
                ptr_id = ptr_id2;
            }
            retid = ptr_id;
            return retstr;
            // if(exps.size() == 0){
            //     assert(false);
            // } else{
                
                //printf("alias %s\n", alias.c_str());
                
                //printf("array_size_0: %d\n", array_size[0]);
                // if(array_size[0] > 0){
                //     assert(array_size.size() == exps.size());
                //     int new_id = ++tmp_id;
                //     inst_num_map[new_id] = 0;
                //     int presize = 1, total_size = calc_tot_size(array_size);
                //     //printf("array_size %d\n", total_size);
                //     for(int i = 0; i < exps.size(); i++){
                //         int exp_retid = 0;
                //         presize *= array_size[i];
                //         retstr->append(exps[i]->GetIR(exp_retid)->c_str());
                //         //printf("exp %d getedir", i);
                //         int new_id2 = ++tmp_id;
                //         if(!IS_INST_NUM(exp_retid)){
                //             retstr->append( string_format("  %s = mul %s, %d\n", GET_ID_STR(new_id2), GET_ID_STR(exp_retid), total_size / presize));
                //         }else{
                //             inst_num_map[new_id2] = inst_num_map[exp_retid] * total_size / presize;
                //         }
                //         int new_id3 = ++tmp_id;
                //         if(IS_INST_NUM(new_id) && IS_INST_NUM(new_id2)){
                //             inst_num_map[new_id3] = inst_num_map[new_id] + inst_num_map[new_id2];
                //         }else{
                //             retstr->append( string_format("  %s = add %s, %s\n", GET_ID_STR(new_id3), GET_ID_STR(new_id), GET_ID_STR(new_id2)));
                //         }
                //         new_id = new_id3;
                //     }
                //     int ptr_id = ++tmp_id;
                //     retstr->append( string_format("  %s = getelemptr @%s, %s\n", GET_ID_STR(ptr_id), alias.c_str(), GET_ID_STR(new_id)));
                //     retid = ptr_id;
                // }else{
                //     //assert(array_size.size() == exps.size());
                //     int new_id = ++tmp_id;
                //     int presize = 1, total_size = -calc_tot_size(array_size);
                //     retstr->append( string_format("  %s = load @%s\n", GET_ID_STR(new_id), alias.c_str()));
                //     int first_exp_id = 0;
                //     retstr->append(exps[0]->GetIR(first_exp_id)->c_str());
                //     int base_ptr_id = ++tmp_id;
                //     retstr->append( string_format("  %s = getptr %s, %s\n", GET_ID_STR(base_ptr_id), GET_ID_STR(new_id), GET_ID_STR(first_exp_id)));
                //     new_id = ++tmp_id;
                //     inst_num_map[new_id] = 0;
                //     for(int i = 1; i < exps.size(); i++){
                //         int exp_retid = 0;
                //         presize *= array_size[i];
                //         retstr->append(exps[i]->GetIR(exp_retid)->c_str());
                //         int new_id2 = ++tmp_id;
                //         if(!IS_INST_NUM(exp_retid)){
                //             retstr->append( string_format("  %s = mul %s, %d\n", GET_ID_STR(new_id2), GET_ID_STR(exp_retid), total_size / presize));
                //         }else{
                //             inst_num_map[new_id2] = inst_num_map[exp_retid] * total_size / presize;
                //         }
                //         int new_id3 = ++tmp_id;
                //         if(IS_INST_NUM(new_id) && IS_INST_NUM(new_id2)){
                //             inst_num_map[new_id3] = inst_num_map[new_id] + inst_num_map[new_id2];
                //         }else{
                //             retstr->append( string_format("  %s = add %s, %s\n", GET_ID_STR(new_id3), GET_ID_STR(new_id), GET_ID_STR(new_id2)));
                //         }
                //         new_id = new_id3;
                //     }
                //     if(exps.size() == 1){
                //         retid = base_ptr_id;
                //     }else{
                //         int ptr_id = ++tmp_id;
                //         retstr->append( string_format("  %s = getelemptr %s, %s\n", GET_ID_STR(ptr_id), GET_ID_STR(base_ptr_id), GET_ID_STR(new_id)));
                //         retid = ptr_id;
                //     }
                // }
            // }
            return retstr;
        }
};

class PrimaryExpAST : public BaseAST {
    public:
        int type;
        int number;
        std::unique_ptr<BaseAST> exp;
        std::unique_ptr<BaseAST> lval;
        virtual unique_ptr<string> GetIR(int &retid) override {
            if (type == PRIMARYEXP_TYPE_EXP) {
                unique_ptr<string> retstr = exp->GetIR(retid);
                return retstr;
            } else if (type == PRIMARYEXP_TYPE_NUMBER) {
                unique_ptr<string> retstr = make_unique<string>();
                int new_id = ++tmp_id;
                // retstr->append( string_format("  %%%d = add 0, %d\n", new_id, number));
                inst_num_map[new_id] = number;
                retid = new_id;
                return retstr;
            }else {
                unique_ptr<string> retstr = make_unique<string>();
                int new_id = ++tmp_id;
                auto symbol_info = envstack.getvar_withalias(dynamic_cast<LValAST *>(lval.get())->ident);
                auto varinfo = symbol_info.first;
                // retstr->append( string_format("  %%%d = add 0, %d\n", new_id, number));
                if(varinfo.type == VarType::CONST){
                    inst_num_map[new_id] = varinfo.val;
                }else if(varinfo.type == VarType::VAR){
                    retstr->append( string_format("  %%%d = load @%s\n", new_id, symbol_info.second.c_str()));
                }else{
                    if( array_size_map[symbol_info.second].size() != dynamic_cast<LValAST *>(lval.get())->exps.size() && !(dynamic_cast<LValAST *>(lval.get())->exps.size() == 0 && array_size_map[symbol_info.second][0] < 0)){
                        int lvalid = 0;
                        retstr->append(lval->GetIR(lvalid)->c_str());
                        retstr->append( string_format("  %s = getelemptr %s, 0\n", GET_ID_STR(new_id), GET_ID_STR(lvalid)));
                    }else{
                        int lvalid = 0;
                        retstr->append(lval->GetIR(lvalid)->c_str());
                        retstr->append( string_format("  %s = load %s\n", GET_ID_STR(new_id), GET_ID_STR(lvalid)));    
                    }             
                }
                retid = new_id;
                return retstr;
            }
        }
        virtual int getval() override {
            if(type == PRIMARYEXP_TYPE_EXP){
                return exp->getval();
            } else if(type == PRIMARYEXP_TYPE_NUMBER){
                return number;
            } else {
                auto symbol_info = envstack.getvar(dynamic_cast<LValAST *>(lval.get())->ident);
                // assert(symbol_info.type == VarType::CONST);
                return symbol_info.val;
            }
        }
};

class MulExpAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> unaryExp;
        std::unique_ptr<BaseAST> mulExp;
        int type;
        char op;
        virtual unique_ptr<string> GetIR(int &retid) override {
            if (type == MULEXP_TYPE_UNARY) {
                unique_ptr<string> retstr = unaryExp->GetIR(retid);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                unique_ptr<string> lhsstr = mulExp->GetIR(lhs_id);
                unique_ptr<string> rhsstr = unaryExp->GetIR(rhs_id);
                retid = ++tmp_id;
                lhsstr->append(rhsstr->c_str());
                if(op == '/'){
                    lhsstr->append( string_format("  %s = div %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == '*'){
                    lhsstr->append( string_format("  %s = mul %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == '%'){
                    lhsstr->append( string_format("  %s = mod %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                }
                return lhsstr;
            }
        }
        virtual int getval() override {
            if(type == MULEXP_TYPE_UNARY){
                return unaryExp->getval();
            } else {
                if(op == '/'){
                    return mulExp->getval() / unaryExp->getval();
                } else if(op == '*'){
                    return mulExp->getval() * unaryExp->getval();
                } else if(op == '%'){
                    return mulExp->getval() % unaryExp->getval();
                } else { assert (false); }
            }
        }
};

class AddExpAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> mulExp;
        std::unique_ptr<BaseAST> addExp;
        int type;
        char op;
        virtual unique_ptr<string> GetIR(int &retid) override {
            if (type == ADDEXP_TYPE_MUL) {
                unique_ptr<string> retstr = mulExp->GetIR(retid);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                unique_ptr<string> lhsstr = addExp->GetIR(lhs_id);
                unique_ptr<string> rhsstr = mulExp->GetIR(rhs_id);
                retid = ++tmp_id;
                lhsstr->append(rhsstr->c_str());
                if(op == '+'){
                    lhsstr->append( string_format("  %s = add %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == '-'){
                    lhsstr->append( string_format("  %s = sub %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                }
                return lhsstr;
            }
        }
        virtual int getval() override {
            if(type == ADDEXP_TYPE_MUL){
                return mulExp->getval();
            } else {
                if(op == '+'){
                    return addExp->getval() + mulExp->getval();
                } else if(op == '-'){
                    return addExp->getval() - mulExp->getval();
                } else { assert (false); }
            }
        }
};

class RelExpAST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> addExp;
        std::unique_ptr<BaseAST> relExp;
        int type;
        char op;
        virtual unique_ptr<string> GetIR(int &retid) override {
            if (type == RELEXP_TYPE_ADD) {
                unique_ptr<string> retstr = addExp->GetIR(retid);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                unique_ptr<string> lhsstr = relExp->GetIR(lhs_id);
                unique_ptr<string> rhsstr = addExp->GetIR(rhs_id);
                retid = ++tmp_id;
                lhsstr->append(rhsstr->c_str());
                if(op == '<'){
                    lhsstr->append( string_format("  %s = lt %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == '>'){
                    lhsstr->append( string_format("  %s = gt %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == GE){
                    lhsstr->append( string_format("  %s = ge %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == LE){
                    lhsstr->append( string_format("  %s = le %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                }
                return lhsstr;
            }
        }
        virtual int getval() override {
            if(type == RELEXP_TYPE_ADD){
                return addExp->getval();
            } else {
                if(op == '<'){
                    return relExp->getval() < addExp->getval();
                } else if(op == '>'){
                    return relExp->getval() > addExp->getval();
                } else if(op == GE){
                    return relExp->getval() >= addExp->getval();
                } else if(op == LE){
                    return relExp->getval() <= addExp->getval();
                } else { assert (false); }
            }
        }
};

class EqExpAST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> relExp;
        std::unique_ptr<BaseAST> eqExp;
        int type;
        char op;
        virtual unique_ptr<string> GetIR(int &retid) override {
            if (type == EQEXP_TYPE_REL) {
                unique_ptr<string> retstr = relExp->GetIR(retid);
                return retstr;
            }else{
                int rhs_id = 0, lhs_id = 0;
                unique_ptr<string> lhsstr = eqExp->GetIR(lhs_id);
                unique_ptr<string> rhsstr = relExp->GetIR(rhs_id);
                retid = ++tmp_id;
                lhsstr->append(rhsstr->c_str());
                if(op == EQ){
                    lhsstr->append( string_format("  %s = eq %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                } else if(op == NE){
                    lhsstr->append( string_format("  %s = ne %s, %s\n", GET_ID_STR(retid), GET_ID_STR(lhs_id), GET_ID_STR(rhs_id)));
                }
                return lhsstr;
            }
        }
        virtual int getval() override {
            if(type == EQEXP_TYPE_REL){
                return relExp->getval();
            } else {
                if(op == EQ){
                    return eqExp->getval() == relExp->getval();
                } else if(op == NE){
                    return eqExp->getval() != relExp->getval();
                } else { assert (false); }
            }
        }
};


class IfexpAST : public BaseAST{
    public:
        int if_id;
        IfexpAST(int id):if_id(id){}
};

class WhileexpAST : public BaseAST{
    public:
        int while_id;
        WhileexpAST(int id):while_id(id){}
};

class InitvalAST : public BaseAST{
    public:
        int type;
        unique_ptr<BaseAST> exp;
        unique_ptr<vector<BaseAST*> > initval_list;
};

static int zero_inst_num = -1;

class InitexpAST: public BaseAST{
    public:
        int type;
        int idx;
        unique_ptr<BaseAST> exp;
        virtual unique_ptr<string> GetIR(int& retid) override {
            if(type == INITEXP_TYPE_EXP){
                return exp->GetIR(retid);
            } else{
                if(zero_inst_num == -1){
                    zero_inst_num = ++tmp_id;
                    inst_num_map[zero_inst_num] = 0;
                }
                retid = zero_inst_num;
                return make_unique<string>("");
            }
        }
        virtual int getval() override {
            if(type == INITEXP_TYPE_EXP){
                return exp->getval();
            } else {
                return 0;
            }
        }
        InitexpAST(int type, unique_ptr<BaseAST> exp, int idx):type(type), idx(idx), exp(move(exp)){}
};

static inline vector<BaseAST*> calc_initval(InitvalAST* initval, vector<int> size, int preidx = 0){
    vector<BaseAST*> ret;
    int total_size = calc_tot_size(size);
    if(initval->type == INITVAL_TYPE_EXP){
        auto retast = new InitexpAST(INITEXP_TYPE_EXP, move(initval->exp), preidx);
        ret.push_back(retast);
    } else {
        int filled_size = 0;
        for(auto& initval_ast: *initval->initval_list){
            auto nowast = dynamic_cast<InitvalAST*>(initval_ast);
            if(nowast->type == INITVAL_TYPE_EXP){
                auto retast = new InitexpAST(INITEXP_TYPE_EXP, move(nowast->exp), preidx + filled_size);
                ret.push_back(retast);
                filled_size += 1;
            } else {
                assert(filled_size % size.back() == 0);
                int pre_size = 1;
                vector<int> new_size;
                for(int i=0;i<size.size();i++){
                    pre_size *= size[i];
                    if(filled_size % (total_size / pre_size) == 0){
                        for(int j = i+1; j < size.size(); j++){
                            new_size.push_back(size[j]);
                        }
                        break;
                    }
                }
                auto now_inits = calc_initval(nowast, new_size, preidx + filled_size);
                for(auto& now_init: now_inits){
                    ret.push_back(now_init);
                }
                filled_size += calc_tot_size(new_size);
            }
            //printf("filled_size: %d, tot_size: %d\n", filled_size, total_size);
        }
        assert(filled_size <= total_size);
    }
    return ret;
}

class VarAssignAST: public BaseAST {
    public:
        std::unique_ptr<BaseAST> lval;
        std::unique_ptr<BaseAST> exp;
        virtual unique_ptr<string> GetIR() override {
            unique_ptr<string> retstr = make_unique<string>("");
            auto varinfo = envstack.getvar_withalias(dynamic_cast<LValAST*>(lval.get())->ident);
            if(varinfo.first.type == VarType::ARR){
                int indexid = 0, expid = 0;
                retstr->append(lval->GetIR(indexid)->c_str());
                retstr->append(exp->GetIR(expid)->c_str());
                retstr->append( string_format("  store %s, %s\n", GET_ID_STR(expid), GET_ID_STR(indexid)));
            }else{
                int retid = 0;
                retstr->append(exp->GetIR(retid)->c_str());
                retstr->append(string_format("  store %s, @%s\n", GET_ID_STR(retid), varinfo.second.c_str()));
            }
            return retstr;
        }
};