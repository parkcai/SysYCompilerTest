#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <variant>
#include <stack>
#include <cstring>
#include <set>

using namespace std;

static int val_num=0;
static int ifelse_num=0;
static bool is_return=false;
static int entry_num=1;
static int while_num=0;
static int break_num=0;
static int continue_num=0;
static stack<int> current_while;
static bool current_func_int=false;
static bool in_param=false;

static int get_list_size(int total_size,vector<int>& dims,int prev_size){
    int size=1;
    vector<int> total(dims.size(),1);
    for(int i=0;i<dims.size();i++){
        size*=dims[i];
        total[i]=size;
    }
    size=prev_size;
    if(size!=0){
        for(int i=dims.size()-1;i>=0;i--){
            if(size<total[i])
                continue;
            else{
                size%=total[i];
                if(size==0){
                    size=total[i];
                    break;
                }   
            }
        }
    }
    else{
        int i=0;
        for(i=0;i<dims.size();i++){
            if(total[i]==total_size){
                break;
            }
        }
        size=total_size/dims[i];
    }
    return size;
}

class SymbolTable{
public:
    SymbolTable* parent=nullptr;
    map<std::string,std::variant<int,std::string>> table;
    map<std::string,int> var_table; //存变量值
    map<string,int> is_ptr;
    map<string,int> is_array;
    std::vector<SymbolTable*> children;
    
    ~SymbolTable(){
        for (auto& child:children){
            delete child;
        }
    }

    void add_ptr(const std::string& name,int len){
        is_ptr[name]=len;
    }

    bool is_ptr_var(const std::string& name){
        if(is_ptr.find(name)!=is_ptr.end()&&table.find(name)!=table.end())
            return true;
        else if(table.find(name)!=table.end())
            return false;
        else if(parent!=nullptr)
            return parent->is_ptr_var(name);
        else
            return false;
    }

    int get_ptr_len(const std::string& name){
        if(is_ptr.find(name)!=is_ptr.end()&&table.find(name)!=table.end())
            return is_ptr[name];
        else if(table.find(name)!=table.end())
            assert(false);
        else if(parent!=nullptr)
            return parent->get_ptr_len(name);
        else
            assert(false);
    }

    void add_array(const std::string& name,int len){
        is_array[name]=len;
    }

    int get_array_len(const std::string& name){
        if(is_array.find(name)!=is_array.end()&&table.find(name)!=table.end())
            return is_array[name];
        else if(table.find(name)!=table.end())
            assert(false);
        else if(parent!=nullptr)
            return parent->get_array_len(name);
        else
            assert(false);
    }

    bool is_array_var(const std::string& name){
        if(is_array.find(name)!=is_array.end()&&table.find(name)!=table.end())
            return true;
        else if(table.find(name)!=table.end())
            return false;
        else if(parent!=nullptr)
            return parent->is_array_var(name);
        else
            return false;
    }

    void Insert(const std::string& name,variant<int,string> value){
        table[name]=value;
    }

    bool isExist(const std::string& name){
        if (table.find(name)!=table.end())
            return true;
        else if(parent!=nullptr)
            return parent->isExist(name);
        else
            return false;
    }

    variant<int,string> query(const std::string& name){
        if (table.find(name)!=table.end())
            return table[name];
        else if (parent!=nullptr)
            return parent->query(name);
        else
            assert(false);
    }

    int get_val(const std::string& name){
        if (table.find(name)!=table.end())
            return var_table[name];
        else if (parent!=nullptr)
            return parent->get_val(name);
        else
            assert(false);
    }

    void set_val(const std::string& name,int value){
        if(table.find(name)!=table.end()){
            var_table[name]=value;
        }
        else if(parent!=nullptr){
            parent->set_val(name,value);
        }
        else{
            assert(false);
        }
    }

    SymbolTable* AddChild(){
        SymbolTable* child=new SymbolTable();
        child->parent=this;
        children.push_back(child);
        return child;
    }

    void RemoveChild(SymbolTable* child){
        for (auto it=children.begin();it!=children.end();++it){
            if (*it==child){
                children.erase(it);
                delete child;
                return;
            }
        }
    }
};

static SymbolTable global_table;
static map<string,int> var_count;

class BaseAST {
public:
    mutable SymbolTable* symbol_table;
    virtual ~BaseAST() {};
    virtual void Dump() const =0;
    virtual std::string GenerateIR(string& s) const=0;
    virtual int compute_exp() const { return 0; }
    virtual void set_symbol_table(SymbolTable* table){}
    virtual string get_ident() const { return ""; }
    virtual string get_type() const { return ""; }
    virtual string get_name() const { return ""; }
    virtual void set_global(){return;}
    virtual int compute_array_init_val(int* array_val,int total_size,vector<int>& dims,int start){return 0;}
    virtual int compute_array_init_string(string* array_val,int total_size,vector<int>& dims,int start,string& s){return 0;}
    virtual int compute_glob_array_init_string(string* array_val,int total_size,vector<int>& dims,int start,string& s){return 0;}
};

enum class FuncType{Int,Void};
struct Func{
    string name;
    FuncType type;
    vector<string> params;
    BaseAST* func_def;

    Func(string n, FuncType t, vector<string> p, BaseAST* f)
        : name(n), type(t), params(p), func_def(f) {}

    Func(){}

    Func& operator =(Func f){
        name=f.name;
        type=f.type;
        params=f.params;
        func_def=f.func_def;
        return *this;
    }
};

static map<string,Func> func_table;

class CompUnitAST:public BaseAST{
public:
    // std::unique_ptr<BaseAST> func_def;
    vector<std::unique_ptr<BaseAST>> func_defs;
    vector<std::unique_ptr<BaseAST>> decls;

    void Dump() const override{
        std::cout<<"CompUnitAST { ";
        for(const auto& val:decls){
            val->Dump();
            std::cout<<", ";
        }
        for(const auto& f_def:func_defs){
            f_def->Dump();
            std::cout<<", ";
        }
        // func_def->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for(const auto& decl:decls){
            decl->set_symbol_table(symbol_table);
        }
        for(const auto& f_def:func_defs){
            f_def->set_symbol_table(symbol_table->AddChild());
        }
        // func_def->set_symbol_table(symbol_table->AddChild());
    }

    std::string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        s+="decl @getint(): i32\n";
        s+="decl @getch(): i32\n";
        s+="decl @getarray(*i32): i32\n";
        s+="decl @putint(i32)\n";
        s+="decl @putch(i32)\n";
        s+="decl @putarray(i32, *i32)\n";
        s+="decl @starttime()\n";
        s+="decl @stoptime()\n\n";
        func_table["getint"]=Func("getint",FuncType::Int,{},nullptr);
        func_table["getch"]=Func("getch",FuncType::Int,{},nullptr);
        func_table["getarray"]=Func("getarray",FuncType::Int,{"*i32"},nullptr);
        func_table["putint"]=Func("putint",FuncType::Void,{"i32"},nullptr);
        func_table["putch"]=Func("putch",FuncType::Void,{"i32"},nullptr);
        func_table["putarray"]=Func("putarray",FuncType::Void,{"i32","*i32"},nullptr);
        func_table["starttime"]=Func("starttime",FuncType::Void,{},nullptr);
        func_table["stoptime"]=Func("stoptime",FuncType::Void,{},nullptr);
        for(const auto& decl:decls){
            decl->set_global();
            decl->GenerateIR(s);
        }
        if(decls.size()!=0){
            s+="\n";
        }
        for(const auto& f_def:func_defs){
            if(f_def->get_type()=="int"){
                current_func_int=true;
            }
            else{
                current_func_int=false;
            }
            f_def->GenerateIR(s);
            s+="\n";
        }
        // func_def->GenerateIR(s);
        return "";
    }
};

class FuncFParamsAST:public BaseAST{
public:
    vector<std::unique_ptr<BaseAST>> FuncFParams;

    void Dump() const override{
        std::cout<<"FuncFParamsAST { ";
        for (const auto& f_param:FuncFParams){
            f_param->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& f_param:FuncFParams){
            f_param->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        for (const auto& f_param:FuncFParams){
            f_param->GenerateIR(s);
            s+=", ";
        }
        s.pop_back();
        s.pop_back();
        return "";
    }
};

class FuncFParamAST:public BaseAST{
public:
    enum class Kind{Int,Array};
    Kind kind;
    mutable std::string btype;
    std::string ident;
    mutable std::string name;
    vector<unique_ptr<BaseAST>> constexps;

    string get_ident() const override{
        return ident;
    }

    string get_type() const override{
        return btype;
    }

    string get_name() const override{
        return name;
    }

    void Dump() const override{
        std::cout<<"FuncFParamAST { "<<btype<<", "<<ident<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& constexp:constexps){
            constexp->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        name="@"+ident;
        if(var_count.find(ident)==var_count.end()){
            var_count[ident]=1;
            name+="_"+to_string(var_count[ident]);
        }
        else{
            var_count[ident]++;
            name+="_"+to_string(var_count[ident]);
        }
        s+=name;
        if(kind==Kind::Int)
            s+=": i32";
        else if(kind==Kind::Array){
            string array_type="i32";
            if(constexps.size()!=0){
                for(int j=constexps.size()-1;j>=0;j--){
                    array_type='['+array_type+", "+to_string(constexps[j]->compute_exp())+']';
                }
            }
            btype='*'+array_type;
            s+=": "+btype;
        }
        return "";
    }
};

class FuncRParamsAST:public BaseAST{
public:
    vector<std::unique_ptr<BaseAST>> exps;
    mutable vector<string> exp_names;

    void Dump() const override{
        std::cout<<"FuncRParamsAST { ";
        for (const auto& exp:exps){
            exp->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& exp:exps){
            exp->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string exp_name;
        for (const auto& exp:exps){
            exp_name=exp->GenerateIR(s);
            exp_names.push_back(exp_name);
        }
        return "";
    }
};

class FuncDefAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> functype;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    bool has_params;
    std::unique_ptr<BaseAST> funcfparams;

    string get_type() const override{
        return functype->get_type();
    }

    int compute_exp() const override{
        return 0;
    }

    void Dump() const override{
        std::cout<<"FuncDefAST { ";
        functype->Dump();
        std::cout<<", "<<ident<<", ";
        block->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        block->set_symbol_table(symbol_table->AddChild());
        if(has_params){
            funcfparams->set_symbol_table(block->symbol_table);
        }
    }

    std::string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        if(func_table.find(ident)!=func_table.end()){
            assert(false);
        }
        if(functype->get_type()=="int"){
            vector<string> params;
            if(has_params){
                auto f_params=(FuncFParamsAST*)funcfparams.get();
                for(const auto& f_param:f_params->FuncFParams){
                    params.push_back(f_param->get_ident());
                }
            }
            func_table[ident]=Func(ident,FuncType::Int,params,(BaseAST*)this);
        }
        else if(functype->get_type()=="void"){
            vector<string> params;
            if(has_params){
                auto f_params=(FuncFParamsAST*)funcfparams.get();
                for(const auto& f_param:f_params->FuncFParams){
                    params.push_back(f_param->get_ident());
                }
            }
            func_table[ident]=Func(ident,FuncType::Void,params,(BaseAST*)this);
        }
        s+="fun @"+ident+"(";
        if(has_params){
            funcfparams->GenerateIR(s);
        }
        s+=")";
        functype->GenerateIR(s);
        s+="{\n";
        s+="%entry:\n";
        if(has_params){
            auto f_params=(FuncFParamsAST*)funcfparams.get();
            for(const auto& f_param:f_params->FuncFParams){
                string name="%"+f_param->get_ident();
                if(var_count.find(f_param->get_ident())!=var_count.end()){
                    var_count[f_param->get_ident()]++;
                    name+="_"+to_string(var_count[f_param->get_ident()]);
                }
                else{
                    var_count[f_param->get_ident()]=1;
                    name+="_1";
                }
                if(f_param->get_type()=="int"){
                    symbol_table->Insert(f_param->get_ident(),name);
                    s+="  "+name+" = alloc i32\n";
                    s+="  store "+f_param->get_name()+", "+name+"\n";
                }
                else{
                    symbol_table->Insert(f_param->get_ident(),name);
                    auto f_p=(FuncFParamAST*)f_param.get();
                    symbol_table->add_ptr(f_param->get_ident(),f_p->constexps.size()+1);
                    s+="  "+name+" = alloc "+f_param->get_type()+"\n";
                    s+="  store "+f_param->get_name()+", "+name+'\n';
                }
            }
        }
        block->GenerateIR(s);
        if(!is_return){
            if(functype->get_type()=="int"){
                s+="  ret 0\n";
            }
            else if(functype->get_type()=="void"){
                s+="  ret\n";
            }
        }
        s+="}\n";
        symbol_table->RemoveChild(block->symbol_table);
        if(is_return){
            is_return=false;
        }
        val_num=0;
        return "";
    }
};

class FuncTypeAST:public BaseAST{
public:
    std::string type;

    void Dump() const override{
        std::cout<<"FuncTypeAST { "<<type<<" }";
    }

    string get_type() const override{
        return type;
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        if (type=="int")
            s+=": i32 ";
        return "";
    }
};

class BlockAST:public BaseAST{
public:
    std::vector<std::unique_ptr<BaseAST>> blockitem;

    void Dump() const override{
        std::cout<<"BlockAST { ";
        for (const auto& b_item:blockitem){
            b_item->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& b_item:blockitem){
            b_item->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";

        for (const auto& b_item:blockitem){
            b_item->GenerateIR(s);
        }
        return "";
    }
};

class BlockItemAST:public BaseAST{
public:
    enum class Kind{Decl,Stmt};
    Kind kind;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override{
        std::cout<<"BlockItemAST { ";
        switch (kind){
            case Kind::Decl:
                decl->Dump();
                break;
            case Kind::Stmt:
                stmt->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Decl:
                decl->set_symbol_table(symbol_table);
                break;
            case Kind::Stmt:
                stmt->set_symbol_table(symbol_table);
                break;
        }
    }

    string GenerateIR(string& s) const override{
        if(is_return){
            return "";
        }
        switch (kind){
            case Kind::Decl:
                decl->GenerateIR(s);
                return "";
            case Kind::Stmt:
                stmt->GenerateIR(s);
                return "";
        }
        return "";
    }
};

class StmtAST:public BaseAST{
public:
    enum class Kind{Matched,Open};
    Kind kind;
    std::unique_ptr<BaseAST> matched_stmt;
    std::unique_ptr<BaseAST> open_stmt;

    void Dump() const override{
        std::cout<<"StmtAST { ";
        switch (kind){
            case Kind::Matched:
                matched_stmt->Dump();
                break;
            case Kind::Open:
                open_stmt->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Matched:
                matched_stmt->set_symbol_table(symbol_table);
                break;
            case Kind::Open:
                open_stmt->set_symbol_table(symbol_table);
                break;
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        switch (kind){
            case Kind::Matched:
                matched_stmt->GenerateIR(s);
                break;
            case Kind::Open:
                open_stmt->GenerateIR(s);
                break;
        }
        return "";
    }
};

class OpenStmtAST:public BaseAST{
public:
    enum class Kind{If,IfElse,While};
    Kind kind;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override{
        std::cout<<"OpenStmtAST { ";
        switch (kind){
            case Kind::If:
                std::cout<<"if ";
                exp->Dump();
                std::cout<<" ";
                if_stmt->Dump();
                std::cout<<" }";
                break;
            case Kind::IfElse:
                std::cout<<"if ";
                exp->Dump();
                std::cout<<" ";
                if_stmt->Dump();
                std::cout<<"else ";
                else_stmt->Dump();
                std::cout<<" }";
                break;
            case Kind::While:
                std::cout<<"while ";
                exp->Dump();
                std::cout<<" ";
                while_stmt->Dump();
                std::cout<<" }";
                break;
        }
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        if(exp!=nullptr)
            exp->set_symbol_table(symbol_table);
        if(kind==Kind::If||kind==Kind::IfElse)
            if_stmt->set_symbol_table(symbol_table);
        if(kind==Kind::IfElse)
            else_stmt->set_symbol_table(symbol_table);
        if(kind==Kind::While){
            while_stmt->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s)const override{
        if(kind==Kind::IfElse){
            string value;
            string then_label;
            string else_label;
            string end_label;
            bool stmt1_return=false;
            bool stmt2_return=false;
            value=exp->GenerateIR(s);
            then_label="%lb_then_"+to_string(ifelse_num);
            else_label="%lb_else_"+to_string(ifelse_num);
            end_label="%lb_end_"+to_string(ifelse_num);
            ifelse_num++;
            s+="  br "+value+", "+then_label+", "+else_label+'\n';
            s+=then_label+":\n";
            if_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+end_label+'\n';
            }
            else{
                is_return=false;
                stmt1_return=true;
            }
            s+=else_label+":\n";
            else_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+end_label+'\n';
            }
            else{
                is_return=false;
                stmt2_return=true;
            }
            if(stmt1_return&&stmt2_return){
                is_return=true;
            }
            if(!stmt1_return||!stmt2_return)
                s+=end_label+":\n";
        }
        else if(kind==Kind::If){
            string value;
            string then_label;
            string end_label;
            value=exp->GenerateIR(s);
            then_label="%lb_then_"+to_string(ifelse_num);
            end_label="%lb_end_"+to_string(ifelse_num);
            ifelse_num++;
            s+="  br "+value+", "+then_label+", "+end_label+'\n';
            s+=then_label+":\n";
            if_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+end_label+'\n';
            }
            else{
                is_return=false;
            }
            s+=end_label+":\n";
        }
        else if(kind==Kind::While){
            string while_entry;
            string while_body;
            string end_label;
            current_while.push(while_num);
            while_entry="%lb_while_entry_"+to_string(while_num);
            while_body="%lb_while_body_"+to_string(while_num);
            end_label="%lb_while_end_"+to_string(while_num);
            while_num++;
            s+="  jump "+while_entry+'\n';
            s+=while_entry+":\n";
            string value=exp->GenerateIR(s);
            s+="  br "+value+", "+while_body+", "+end_label+'\n';
            s+=while_body+":\n";
            while_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+while_entry+'\n';
            }
            else{
                is_return=false;
            }
            s+=end_label+":\n";
            current_while.pop();
        }
        return "";
    }
};

class LValAST:public BaseAST{
public:
    enum class Kind{Int,Array};
    Kind kind;
    std::string ident;
    vector<std::unique_ptr<BaseAST>> exps;

    void Dump() const override{
        std::cout<<"LValAST { "<<ident<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& exp:exps){
            exp->set_symbol_table(symbol_table);
        }
    }

    string get_ident() const override{
        return ident;
    }

    int compute_exp() const override{
        if(symbol_table->isExist(ident)){
            std::variant<int,string> value=symbol_table->query(ident);
            if(std::holds_alternative<int>(value)){
                return get<int>(value);
            }
            else if(std::holds_alternative<string>(value)){
                // return symbol_table->var_table[ident];
                assert(false);
                return symbol_table->get_val(ident);
            }
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        if(symbol_table->isExist(ident)){
            variant<int,string> value=symbol_table->query(ident);
            if (std::holds_alternative<int>(value)){
                return to_string(get<int>(value));
            }
            else if (std::holds_alternative<string>(value)){
                if(kind==Kind::Int){
                    if(in_param&&symbol_table->is_array_var(ident)){
                        string current_val="%"+to_string(val_num++);
                        s+="  "+current_val+" = getelemptr "+get<string>(value)+", 0\n";
                        return current_val;
                    }
                    string current_val="%"+to_string(val_num++);
                    s+="  "+current_val+" = load "+get<string>(value)+'\n';
                    return current_val;
                }
                else if(kind==Kind::Array){
                    bool is_ptr=symbol_table->is_ptr_var(ident);
                    string name=get<string>(value);
                    string current_val;
                    string next_val;
                    if(in_param){
                        if(is_ptr){
                            current_val="%"+to_string(val_num++);
                            s+="  "+current_val+" = load "+name+'\n';
                            name=current_val;
                        }
                        for(int i=0;i<exps.size();i++){
                            current_val=exps[i]->GenerateIR(s);
                            next_val="%"+to_string(val_num++);
                            if(i==0&&is_ptr){
                                s+="  "+next_val+" = getptr "+name+", "+current_val+'\n';
                            }
                            else{
                                s+="  "+next_val+" = getelemptr "+name+", "+current_val+'\n';
                            }
                            name=next_val;
                        }
                        next_val="%"+to_string(val_num++);
                        if((symbol_table->is_array_var(ident)&&exps.size()==symbol_table->get_array_len(ident))||(symbol_table->is_ptr_var(ident)&&exps.size()==symbol_table->get_ptr_len(ident))){
                            s+="  "+next_val+" = load "+name+'\n';
                        }
                        else{
                            s+="  "+next_val+" = getelemptr "+name+", 0\n";
                        }
                        // s+="  "+next_val+" = getelemptr "+name+", 0\n";
                        return next_val;
                    }
                    if(is_ptr){
                        current_val="%"+to_string(val_num++);
                        s+="  "+current_val+" = load "+name+'\n';
                        name=current_val;
                    }
                    for(int i=0;i<exps.size();i++){
                        current_val=exps[i]->GenerateIR(s);
                        next_val="%"+to_string(val_num++);
                        if(i==0&&is_ptr){
                            s+="  "+next_val+" = getptr "+name+", "+current_val+'\n';
                        }
                        else{
                            s+="  "+next_val+" = getelemptr "+name+", "+current_val+'\n';
                        }
                        name=next_val;
                    }
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = load "+name+'\n';
                    return next_val;
                }
            }
        }
       assert(false);
    }
};

class MatchedStmtAST:public BaseAST{
public:
    enum class Kind{Return,Assign,Exp,Block,IfElse,While,Break,Continue};
    Kind kind;
    std::string ret;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override{
        std::cout<<"MatchedStmtAST { ";
        switch (kind){
            case Kind::Return:
                std::cout<<"return ";
                if(exp!=nullptr)
                    exp->Dump();
                break;
            case Kind::Assign:
                lval->Dump();
                std::cout<<"=";
                exp->Dump();
                break;
            case Kind::Exp:
                if(exp!=nullptr)
                    exp->Dump();
                break;
            case Kind::Block:
                block->Dump();
                break;
            case Kind::IfElse:
                std::cout<<"if ";
                if_stmt->Dump();
                std::cout<<"else ";
                else_stmt->Dump();
                break;
            case Kind::While:
                std::cout<<"while ";
                exp->Dump();
                while_stmt->Dump();
                break;
            case Kind::Break:
                std::cout<<"break"<<endl;
                break;
            case Kind::Continue:
                std::cout<<"continue"<<endl;
                break;
        }
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Return:
                if(exp!=nullptr)
                    exp->set_symbol_table(symbol_table);
                break;
            case Kind::Assign:
                lval->set_symbol_table(symbol_table);
                exp->set_symbol_table(symbol_table);
                break;
            case Kind::Block:
                block->set_symbol_table(symbol_table->AddChild());
                break;
            case Kind::Exp:
                if(exp!=nullptr)
                    exp->set_symbol_table(symbol_table);
                break;
            case Kind::IfElse:
                exp->set_symbol_table(symbol_table);
                if_stmt->set_symbol_table(symbol_table);
                else_stmt->set_symbol_table(symbol_table);
                break;
            case Kind::While:
                exp->set_symbol_table(symbol_table);
                while_stmt->set_symbol_table(symbol_table);
                break;
            default:
                break;
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string value;
        string lval_name;
        string current_val;
        string next_val;
        string then_label;
        string else_label;
        string end_label;
        string while_entry;
        string while_body;
        int find_current_while;
        bool stmt1_return=false;
        bool stmt2_return=false;
        LValAST* LVal;
        // if(exp!=nullptr)
        //     value=exp->GenerateIR(s);
        switch (kind){
            case Kind::Return:   
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                s+="  ret ";
                if(exp!=nullptr)
                    s+=value;
                if(exp==nullptr&&current_func_int)
                    s+="0";
                s+='\n';
                is_return=true;
                return "";
            case Kind::Assign:
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                LVal=(LValAST*)lval.get();
                lval_name=get<string>(symbol_table->query(lval->get_ident()));
                if(LVal->kind==LValAST::Kind::Int){
                    s+="  store "+value+", "+lval_name+'\n';
                }
                else if(LVal->kind==LValAST::Kind::Array){
                    bool is_ptr=symbol_table->is_ptr_var(lval->get_ident());
                    if(is_ptr){
                        current_val="%"+to_string(val_num++);
                        s+="  "+current_val+" = load "+lval_name+'\n';
                        lval_name=current_val;
                    }
                    for(int i=0;i<LVal->exps.size();i++){
                        current_val=LVal->exps[i]->GenerateIR(s);
                        next_val="%"+to_string(val_num++);
                        if(i==0&&is_ptr){
                            s+="  "+next_val+" = getptr "+lval_name+", "+current_val+'\n';
                        }
                        else{
                            s+="  "+next_val+" = getelemptr "+lval_name+", "+current_val+'\n';
                        }
                        lval_name=next_val;
                    }
                    s+="  store "+value+", "+lval_name+'\n';
                }
                // symbol_table->var_table[lval->get_ident()]=exp->compute_exp();
                // symbol_table->set_val(lval->get_ident(),exp->compute_exp());
                return "";
            case Kind::Block:
                block->GenerateIR(s);
                symbol_table->RemoveChild(block->symbol_table);
                return "";
            case Kind::Exp:
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                return value;
            case Kind::IfElse:
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
            //试图用全局的is_return来判断是否有return语句，但是在ifelse语句中，if和else都有可能有return语句
                then_label="%lb_then_"+to_string(ifelse_num);
                else_label="%lb_else_"+to_string(ifelse_num);
                end_label="%lb_end_"+to_string(ifelse_num);
                ifelse_num++;
                s+="  br "+value+", "+then_label+", "+else_label+'\n';
                s+=then_label+":\n";
                if_stmt->GenerateIR(s);
                if(!is_return){
                    s+="  jump "+end_label+'\n';
                }
                else{
                    is_return=false;
                    stmt1_return=true;
                }
                s+=else_label+":\n";
                else_stmt->GenerateIR(s);
                if(!is_return){
                    s+="  jump "+end_label+'\n';
                }
                else{
                    is_return=false;
                    stmt2_return=true;
                }
                if(stmt1_return&&stmt2_return){
                    is_return=true;
                }
                if(!stmt1_return||!stmt2_return){
                    s+=end_label+":\n";
                }
                return "";
            case Kind::While:
                current_while.push(while_num);
                while_entry="%lb_while_entry_"+to_string(while_num);
                while_body="%lb_while_body_"+to_string(while_num);
                end_label="%lb_while_end_"+to_string(while_num);
                while_num++;
                s+="  jump "+while_entry+'\n';
                s+=while_entry+":\n";
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                s+="  br "+value+", "+while_body+", "+end_label+'\n';
                s+=while_body+":\n";
                while_stmt->GenerateIR(s);
                if(!is_return){
                    s+="  jump "+while_entry+'\n';
                }
                else{
                    is_return=false;
                }
                s+=end_label+":\n";
                current_while.pop();
                return "";
            case Kind::Break:
            // 先查看break在哪层循环中
                if(current_while.empty()){
                    assert(false);
                }
                find_current_while=current_while.top();
                s+="  jump %lb_while_end_"+to_string(find_current_while)+'\n';
                //再生成一个标签，处理后面的while中语句
                s+="%lb_while_body_"+to_string(find_current_while)+"_break_"+to_string(break_num)+":\n";
                break_num++;
                return "";
            case Kind::Continue:
                if(current_while.empty()){
                    assert(false);
                }
                find_current_while=current_while.top();
                s+="  jump %lb_while_entry_"+to_string(find_current_while)+'\n';
                s+="%lb_while_body_"+to_string(find_current_while)+"_continue_"+to_string(continue_num)+":\n";
                continue_num++;
                return "";
            default:
                return "";
        }
    }
};

class NumberAST:public BaseAST{
public:
    int int_const;

    void Dump() const override{
        std::cout<<int_const;
    }

    int compute_exp() const override{
        return int_const;
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        return to_string(int_const);
    }
};

class ExpAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> orexp;

    void Dump() const override{
        std::cout<<"ExpAST { ";
        orexp->Dump();
        std::cout<<" }";
    }

    int compute_exp() const override{
        return orexp->compute_exp();
        assert(false);
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        orexp->set_symbol_table(symbol_table);
    }

    //生成IR时，返回目前变量的名字
    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string value;
        value=orexp->GenerateIR(s);
        return value;
    }
};

class PrimaryExpAST:public BaseAST{
public:
    enum class Kind{Number,Exp,LVal};
    Kind kind;
    std::unique_ptr<BaseAST> number;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;

    void Dump() const override{
        std::cout<<"PrimaryExpAST { ";
        switch (kind){
            case Kind::Number:
                number->Dump();
                break;
            case Kind::Exp:
                exp->Dump();
                break;
            case Kind::LVal:
                lval->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Exp:
                exp->set_symbol_table(symbol_table);
                break;
            case Kind::LVal:
                lval->set_symbol_table(symbol_table);
                break;
            default:
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Number:
                return number->compute_exp();
            case Kind::Exp:
                return exp->compute_exp();
            case Kind::LVal:
                return lval->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Number:
                current_val=number->GenerateIR(s);
                break;
            case Kind::Exp:
                current_val=exp->GenerateIR(s);
                break;
            case Kind::LVal:
                current_val=lval->GenerateIR(s);
                break;
        }
        return current_val;
    }
};

class UnaryExpAST:public BaseAST{
public:
    enum class Kind{Primary,Unary,FunCall};
    Kind kind;
    std::unique_ptr<BaseAST> primaryexp;
    std::string unaryop;
    std::unique_ptr<BaseAST> unaryexp;
    std::string fun_name;
    std::unique_ptr<BaseAST> funcrparams;
    bool has_params;

    void Dump() const override{
        std::cout<<"UnaryExpAST { ";
        switch (kind){
            case Kind::Primary:
                primaryexp->Dump();
                break;
            case Kind::Unary:
                std::cout<<unaryop;
                unaryexp->Dump();
                break;
            case Kind::FunCall:
                std::cout<<"call"<<fun_name;
                if(has_params){
                    funcrparams->Dump();
                }
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Primary:
                primaryexp->set_symbol_table(symbol_table);
                break;
            case Kind::Unary:
                unaryexp->set_symbol_table(symbol_table);
                break;
            case Kind::FunCall:
                if(has_params){
                    funcrparams->set_symbol_table(symbol_table);
                }
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Primary:
                return primaryexp->compute_exp();
            case Kind::Unary:
                if (unaryop=="-")
                    return -(unaryexp->compute_exp());
                else if (unaryop=="!")
                    return !(unaryexp->compute_exp());
                else if (unaryop=="+")
                    return unaryexp->compute_exp();
            case Kind::FunCall:
                break;
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Primary:
                current_val=primaryexp->GenerateIR(s);
                next_val=current_val;
                break;
            case Kind::Unary:
                current_val=unaryexp->GenerateIR(s);
                if (unaryop=="-"){
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = sub 0, "+current_val+'\n';
                }
                else if(unaryop=="!"){
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = eq "+current_val+", 0\n";
                }    
                else if(unaryop=="+"){
                    next_val=current_val;
                }
                break;
            case Kind::FunCall:
                if(func_table[fun_name].type==FuncType::Int){
                    if(has_params){
                        in_param=true;
                        funcrparams->GenerateIR(s);
                        in_param=false;
                    }
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = call @"+fun_name+"(";
                    if(has_params){
                        auto r_params=(FuncRParamsAST*)funcrparams.get();
                        for(const auto& r_param:r_params->exp_names){
                            s+=r_param+", ";
                        }
                        s.pop_back();
                        s.pop_back();
                    }
                    s+=")\n";
                    return next_val;
                }
                else if(func_table[fun_name].type==FuncType::Void){
                    if(has_params){
                        in_param=true;
                        funcrparams->GenerateIR(s);
                        in_param=false;
                    }
                    s+="  call @"+fun_name+"(";
                    if(has_params){
                        auto r_params=(FuncRParamsAST*)funcrparams.get();
                        for(const auto& r_param:r_params->exp_names){
                            s+=r_param+", ";
                        }
                        s.pop_back();
                        s.pop_back();
                    }
                    s+=")\n";
                    return "";
                }
                break;
        }
        return next_val;
    }
};

class MulExpAST:public BaseAST{
public:
    enum class Kind{Unary,Mult};
    Kind kind;
    std::unique_ptr<BaseAST> unaryexp;
    std::string op;
    std::unique_ptr<BaseAST> mulexp;

    void Dump() const override{
        std::cout<<"MulExpAST { ";
        switch (kind){
            case Kind::Unary:
                unaryexp->Dump();
                break;
            case Kind::Mult:
                mulexp->Dump();
                std::cout<<op;
                unaryexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Unary:
                unaryexp->set_symbol_table(symbol_table);
                break;
            case Kind::Mult:
                mulexp->set_symbol_table(symbol_table);
                unaryexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Unary:
                return unaryexp->compute_exp();
            case Kind::Mult:
                if (op=="*")
                    return mulexp->compute_exp()*unaryexp->compute_exp();
                else if (op=="/")
                    return mulexp->compute_exp()/unaryexp->compute_exp();
                else if (op=="%")
                    return mulexp->compute_exp()%unaryexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Unary:
                current_val_2=unaryexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Mult:
                current_val_1=mulexp->GenerateIR(s);
                current_val_2=unaryexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                switch (op[0]){
                    case '*':
                        s+="  "+next_val+" = mul "+current_val_1+", "+current_val_2+"\n";
                        break;
                    case '/':
                        s+="  "+next_val+" = div "+current_val_1+", "+current_val_2+"\n";
                        break;
                    case '%':
                        s+="  "+next_val+" = mod "+current_val_1+", "+current_val_2+"\n";
                        break;
                }
        }
        return next_val;
    }
};

class AddExpAST:public BaseAST{
public:
    enum class Kind{Mul,Add};
    Kind kind;
    std::unique_ptr<BaseAST> mulexp;
    std::string op;
    std::unique_ptr<BaseAST> addexp;

    void Dump() const override{
        std::cout<<"AddExpAST { ";
        switch (kind){
            case Kind::Mul:
                mulexp->Dump();
                break;
            case Kind::Add:
                addexp->Dump();
                std::cout<<op;
                mulexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Mul:
                mulexp->set_symbol_table(symbol_table);
                break;
            case Kind::Add:
                addexp->set_symbol_table(symbol_table);
                mulexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Mul:
                return mulexp->compute_exp();
            case Kind::Add:
                if (op=="+")
                    return addexp->compute_exp()+mulexp->compute_exp();
                else if (op=="-")
                    return addexp->compute_exp()-mulexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Mul:
                current_val_2=mulexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Add:
                current_val_1=addexp->GenerateIR(s);
                current_val_2=mulexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                switch (op[0]){
                    case '+':
                        s+="  "+next_val+" = add "+current_val_1+", "+current_val_2+"\n";
                        break;
                    case '-':
                        s+="  "+next_val+" = sub "+current_val_1+", "+current_val_2+"\n";
                        break;
                }
        }
        return next_val;
    }
};

class RelExpAST:public BaseAST{
public:
    enum class Kind{Add,Rel};
    Kind kind;
    std::unique_ptr<BaseAST> addexp;
    std::string op;
    std::unique_ptr<BaseAST> relexp;

    void Dump() const override{
        std::cout<<"RelExpAST { ";
        switch (kind){
            case Kind::Add:
                addexp->Dump();
                break;
            case Kind::Rel:
                relexp->Dump();
                std::cout<<op;
                addexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Add:
                addexp->set_symbol_table(symbol_table);
                break;
            case Kind::Rel:
                relexp->set_symbol_table(symbol_table);
                addexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Add:
                return addexp->compute_exp();
            case Kind::Rel:
                if (op=="<")
                    return relexp->compute_exp()<addexp->compute_exp();
                else if (op==">")
                    return relexp->compute_exp()>addexp->compute_exp();
                else if (op=="<=")
                    return relexp->compute_exp()<=addexp->compute_exp();
                else if (op==">=")
                    return relexp->compute_exp()>=addexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Add:
                current_val_2=addexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Rel:
                current_val_1=relexp->GenerateIR(s);
                current_val_2=addexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                if (op=="<"){
                    s+="  "+next_val+" = lt "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op==">"){
                    s+="  "+next_val+" = gt "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op=="<="){
                    s+="  "+next_val+" = le "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op==">="){
                    s+="  "+next_val+" = ge "+current_val_1+", "+current_val_2+"\n";
                }
                break;
        }
        return next_val;
    }
};

class EqExpAST:public BaseAST{
public:
    enum class Kind{Rel,Eq};
    Kind kind;
    std::unique_ptr<BaseAST> relexp;
    std::string op;
    std::unique_ptr<BaseAST> eqexp;

    void Dump() const override{
        std::cout<<"EqExpAST { ";
        switch (kind){
            case Kind::Rel:
                relexp->Dump();
                break;
            case Kind::Eq:
                eqexp->Dump();
                std::cout<<op;
                relexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Rel:
                relexp->set_symbol_table(symbol_table);
                break;
            case Kind::Eq:
                eqexp->set_symbol_table(symbol_table);
                relexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Rel:
                return relexp->compute_exp();
            case Kind::Eq:
                if (op=="==")
                    return eqexp->compute_exp()==relexp->compute_exp();
                else if (op=="!=")
                    return eqexp->compute_exp()!=relexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s)const override{
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Rel:
                current_val_2=relexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Eq:
                current_val_1=eqexp->GenerateIR(s);
                current_val_2=relexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                if (op=="=="){
                    s+="  "+next_val+" = eq "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op=="!="){
                    s+="  "+next_val+" = ne "+current_val_1+", "+current_val_2+"\n";
                }
                break;
        }
        return next_val;
    }
};

class LAndExpAST:public BaseAST{
public:
    enum class Kind{Eq,And};
    Kind kind;
    std::unique_ptr<BaseAST> eqexp;
    std::unique_ptr<BaseAST> landexp;

    void Dump() const override{
        std::cout<<"LAndExpAST { ";
        switch (kind){
            case Kind::Eq:
                eqexp->Dump();
                break;
            case Kind::And:
                landexp->Dump();
                std::cout<<"&&";
                eqexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Eq:
                eqexp->set_symbol_table(symbol_table);
                break;
            case Kind::And:
                landexp->set_symbol_table(symbol_table);
                eqexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Eq:
                return eqexp->compute_exp();
            case Kind::And:
                return landexp->compute_exp()&&eqexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        string tmp_val;
        switch (kind){
            case Kind::Eq:
                current_val_2=eqexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::And:
                current_val_1=landexp->GenerateIR(s);
                string then_label="%lb_then_"+to_string(ifelse_num);
                string else_label="%lb_else_"+to_string(ifelse_num);
                string end_label="%lb_end_"+to_string(ifelse_num);
                ifelse_num++;
                tmp_val="%"+to_string(val_num++);
                s+="  "+tmp_val+" = alloc i32\n";
                s+="  br "+current_val_1+", "+then_label+", "+else_label+'\n';
                s+=then_label+":\n";
                string tmp_val_1="%"+to_string(val_num++);
                s+="  "+tmp_val_1+" = ne "+current_val_1+", 0\n";
                current_val_2=eqexp->GenerateIR(s);
                string tmp_val_2="%"+to_string(val_num++);
                s+="  "+tmp_val_2+" = ne "+current_val_2+", 0\n";
                next_val="%"+to_string(val_num++);
                s+="  "+next_val+" = and "+tmp_val_1+", "+tmp_val_2+"\n";
                s+="  store "+next_val+", "+tmp_val+'\n';
                s+="  jump "+end_label+'\n';
                s+=else_label+":\n";
                s+="  store 0, "+tmp_val+'\n';
                s+="  jump "+end_label+'\n';
                s+=end_label+":\n";
                next_val="%"+to_string(val_num++);
                s+="  "+next_val+" = load "+tmp_val+'\n';
                // current_val_1=landexp->GenerateIR(s);
                // tmp_val=landexp->compute_exp();
                // if(tmp_val==0){
                //     return current_val_1;
                // }
                // string tmp_val_1="%"+to_string(val_num++);
                // s+="  "+tmp_val_1+" = ne "+current_val_1+", 0\n";
                // current_val_2=eqexp->GenerateIR(s);
                // string tmp_val_2="%"+to_string(val_num++);
                // s+="  "+tmp_val_2+" = ne "+current_val_2+", 0\n";
                // next_val="%"+to_string(val_num++);
                // s+="  "+next_val+" = and "+tmp_val_1+", "+tmp_val_2+"\n";
                // s+="  "+next_val+" = ne "+current_val_2+", 0\n";
                break;
        }
        return next_val;
    }
};

class LOrExpAST:public BaseAST{
public:
    enum class Kind{And,Or};
    Kind kind;
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> lorexp;

    void Dump() const override{
        std::cout<<"LOrExpAST { ";
        switch (kind){
            case Kind::And:
                landexp->Dump();
                break;
            case Kind::Or:
                lorexp->Dump();
                std::cout<<"||";
                landexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::And:
                landexp->set_symbol_table(symbol_table);
                break;
            case Kind::Or:
                lorexp->set_symbol_table(symbol_table);
                landexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::And:
                return landexp->compute_exp();
            case Kind::Or:
                return lorexp->compute_exp()||landexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        string tmp_val;
        switch (kind){
            case Kind::And:
                current_val_2=landexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Or:
                current_val_1=lorexp->GenerateIR(s);
                string then_label="%lb_then_"+to_string(ifelse_num);
                string else_label="%lb_else_"+to_string(ifelse_num);
                string end_label="%lb_end_"+to_string(ifelse_num);
                ifelse_num++;
                tmp_val="%"+to_string(val_num++);
                s+="  "+tmp_val+" = alloc i32\n";
                s+="  br "+current_val_1+", "+then_label+", "+else_label+'\n';
                s+=then_label+":\n";
                s+="  store 1, "+tmp_val+'\n';
                s+="  jump "+end_label+'\n';
                s+=else_label+":\n";
                current_val_2=landexp->GenerateIR(s);
                string tmp_val_1="%"+to_string(val_num++);
                s+="  "+tmp_val_1+" = ne "+current_val_2+", 0\n";
                s+="  store "+tmp_val_1+", "+tmp_val+'\n';
                s+="  jump "+end_label+'\n';
                s+=end_label+":\n";
                next_val="%"+to_string(val_num++);
                s+="  "+next_val+" = load "+tmp_val+'\n';
                // current_val_1=lorexp->GenerateIR(s);
                // tmp_val=lorexp->compute_exp();
                // if(tmp_val!=0){
                //     next_val="%"+to_string(val_num++);
                //     s+="  "+next_val+" = ne "+current_val_1+", 0\n";
                //     return next_val;
                // }
                // current_val_2=landexp->GenerateIR(s);
                // string tmp_val_1="%"+to_string(val_num++);
                // s+="  "+tmp_val_1+" = or "+current_val_1+", "+current_val_2+"\n";
                // next_val="%"+to_string(val_num++);
                // s+="  "+next_val+" = ne "+tmp_val_1+", 0\n";
                break;
        }
        return next_val;
    }
};

class DeclAST:public BaseAST{
public:
    enum class Kind{Const,Var};
    Kind kind;
    std::unique_ptr<BaseAST> constdecl;
    std::unique_ptr<BaseAST> vardecl;
    bool is_global=false;

    void set_global() override{
        is_global=true;
    }

    void Dump() const override{
        std::cout<<"DeclAST { ";
        switch (kind){
            case Kind::Const:
                constdecl->Dump();
                break;
            case Kind::Var:
                vardecl->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Const:
                constdecl->set_symbol_table(symbol_table);
                break;
            case Kind::Var:
                vardecl->set_symbol_table(symbol_table);
                break;
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        switch (kind){
            case Kind::Const:
                if(is_global)
                    constdecl->set_global();
                constdecl->GenerateIR(s);
                break;
            case Kind::Var:
                if(is_global)
                    vardecl->set_global();
                vardecl->GenerateIR(s);
                break;
        }
        return "";
    }
};

class ConstDeclAST:public BaseAST{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> constdef;
    bool is_global=false;

    void set_global() override{
        is_global=true;
    }

    void Dump() const override{
        std::cout<<"ConstDeclAST { "<<btype;
        for (const auto& c_def:constdef){
            c_def->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& c_def:constdef){
            c_def->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        for (const auto& c_def:constdef){
            if(is_global)
                c_def->set_global();
            c_def->GenerateIR(s);
        }
        return "";
    }
};

class ConstInitValAST:public BaseAST{
public:
    enum class Kind {Int,Array,EmptyArray};
    Kind kind;
    std::unique_ptr<BaseAST> constexp;
    vector<std::unique_ptr<BaseAST>> constInitVals;
    bool is_global=false;

    void Dump() const override{
        std::cout<<"ConstInitValAST { ";
        constexp->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        if(constexp!=nullptr)
            constexp->set_symbol_table(symbol_table);
        for(auto& constInitval:constInitVals){
            constInitval->set_symbol_table(symbol_table);
        }
    }

    int compute_exp() const override{
        return constexp->compute_exp();
    }

    void set_global() override{
        is_global=true;
    }

    int compute_array_init_val(int* init_array,int total_size,vector<int>& dims,int start) override{
        if(kind==Kind::Int){
            init_array[start]=constexp->compute_exp();
            return start+1;
        }
        else if(kind==Kind::EmptyArray){
            return start+total_size;
        }
        else if(kind==Kind::Array){
            int next_start=start;
            for(auto& constInitval:constInitVals){
                auto init_val=(ConstInitValAST*)constInitval.get();
                if(init_val->kind==Kind::Int){
                    next_start=constInitval->compute_array_init_val(init_array,1,dims,next_start);
                }
                else if(init_val->kind==Kind::EmptyArray){
                    int new_size=get_list_size(total_size,dims,next_start-start);
                    next_start=constInitval->compute_array_init_val(init_array,new_size,dims,next_start);
                }
                else if(init_val->kind==Kind::Array){
                    int new_size=get_list_size(total_size,dims,next_start-start);
                    // next_start+=(new_size-next_start%new_size);
                    next_start=constInitval->compute_array_init_val(init_array,new_size,dims,next_start);
                }
            }
            return start+total_size;
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        constexp->GenerateIR(s);
        return "";
    }
};

class ConstDefAST:public BaseAST{
public:
    enum class Kind{Int,Array};
    Kind kind;
    std::string ident;
    vector<std::unique_ptr<BaseAST>> constexps;
    std::unique_ptr<BaseAST> constinitval;
    bool is_global=false;

    void Dump() const override{
        std::cout<<"ConstDefAST { "<<ident<<", ";
        constinitval->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for(auto& constexp:constexps){
            constexp->set_symbol_table(symbol_table);
        }
        constinitval->set_symbol_table(symbol_table);
    }

    int compute_exp() const override{
        return constinitval->compute_exp();
    }

    void set_global() override{
        is_global=true;
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        if(kind==Kind::Int){
            symbol_table->Insert(ident,constinitval->compute_exp());
        }
        else if(kind==Kind::Array){
            string name="@"+ident;
            if(var_count.find(ident)==var_count.end()){
                var_count[ident]=1;
                name+="_"+to_string(var_count[ident]);
            }
            else{
                var_count[ident]++;
                name+="_"+to_string(var_count[ident]);
            }
            symbol_table->Insert(ident,name);
            symbol_table->add_array(ident,constexps.size());
            string array_type="i32";
            vector<int> dims;
            int dim;
            int total_size=1;
            for (int i=constexps.size()-1;i>=0;i--){
                dim=constexps[i]->compute_exp();
                array_type='['+array_type+", "+to_string(dim)+']';
                dims.push_back(dim);
                total_size*=dim;
            }
            int* init_array=new int[total_size];
            memset(init_array,0,total_size*sizeof(int));
            constinitval->compute_array_init_val(init_array,total_size,dims,0);
            if(is_global){
                s+="global ";
            }
            else{
                s+="  ";
            }
            s+=name+" = alloc "+array_type;
            if(is_global){
                constinitval->set_global();
                s+=", ";
                auto init_val=(ConstInitValAST*)constinitval.get();
                if(init_val->kind==ConstInitValAST::Kind::EmptyArray){
                    s+="zeroinit\n";
                }
                else if(init_val->kind==ConstInitValAST::Kind::Array){
                    string initlist;
                    vector<int> indexs(dims.size(),0);
                    int current_size=1;
                    for(int i=0;i<dims.size();i++){
                        current_size*=dims[i];
                        indexs[i]=current_size;
                    }
                    for(int i=0;i<total_size;i++){
                        for(int j=dims.size()-1;j>=0;j--){
                            if(i%indexs[j]==0){
                                initlist+="{";
                            }
                        }
                        initlist+=to_string(init_array[i])+", ";
                        for(int j=0;j<dims.size();j++){
                            if((i+1)%indexs[j]==0){
                                initlist.pop_back();
                                initlist.pop_back();
                                initlist+="}, ";
                            }
                        }
                    }
                    initlist.pop_back();
                    initlist.pop_back();
                    s+=initlist+'\n';
                }
            }
            else{
                s+='\n';
                string val_name;
                string prev_name;
                vector<int> indexs(dims.size(),0);
                for(int i=0;i<total_size;i++){
                    prev_name=name;
                    for(int j=dims.size()-1;j>=0;j--){
                        val_name="%"+to_string(val_num++);
                        s+="  "+val_name+" = getelemptr "+prev_name+", "+to_string(indexs[j])+'\n';
                        prev_name=val_name;
                    }
                    s+="  store "+to_string(init_array[i])+", "+prev_name+'\n';
                    indexs[0]++;
                    for(int j=0;j<dims.size()-1;j++){
                        if(indexs[j]==dims[j]){
                            indexs[j]=0;
                            indexs[j+1]++;
                        }
                    }
                }
            }
            delete[] init_array;
        }
        return "";
    }
};

class ConstExpAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override{
        std::cout<<"ConstExpAST { ";
        exp->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        exp->set_symbol_table(symbol_table);
    }

    int compute_exp() const override{
        return exp->compute_exp();
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        exp->GenerateIR(s);
        return "";
    }
};

class VarDeclAST:public BaseAST{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> vardef;
    bool is_global=false;

    void set_global() override{
        is_global=true;
    }

    void Dump() const override{
        std::cout<<"VarDeclAST { "<<btype;
        for (const auto& v_def:vardef){
            v_def->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& v_def:vardef){
            v_def->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        for (const auto& v_def:vardef){
            if(is_global)
                v_def->set_global();
            v_def->GenerateIR(s);
        }
        return "";
    }
};

class InitValAST:public BaseAST{
public:
    enum class Kind{Exp,Array,EmptyArray};
    Kind kind;
    std::unique_ptr<BaseAST> exp;
    std::vector<std::unique_ptr<BaseAST>> initvals;
    bool is_global=false;

    void set_global() override{
        is_global=true;
    }

    void Dump() const override{
        std::cout<<"InitValAST { ";
        exp->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        if(exp!=nullptr)
            exp->set_symbol_table(symbol_table);
        for(auto& initval:initvals){
            initval->set_symbol_table(symbol_table);
        }
    }

    int compute_exp() const override{
        return exp->compute_exp();
    }

    int compute_glob_array_init_string(string* init_array,int total_size,vector<int>& dims,int start,string& s) override{
        if(kind==Kind::Exp){
            init_array[start]=to_string(exp->compute_exp());
            return start+1;
        }
        else if(kind==Kind::EmptyArray){
            return start+total_size;
        }
        else if(kind==Kind::Array){
            int next_start=start;
            for(auto& initval:initvals){
                auto init_val=(InitValAST*)initval.get();
                if(init_val->kind==Kind::Exp){
                    next_start=init_val->compute_glob_array_init_string(init_array,1,dims,next_start,s);
                }
                else if(init_val->kind==Kind::EmptyArray){
                    int new_size=get_list_size(total_size,dims,next_start-start);
                    next_start=init_val->compute_glob_array_init_string(init_array,new_size,dims,next_start,s);
                }
                else if(init_val->kind==Kind::Array){
                    int new_size=get_list_size(total_size,dims,next_start-start);
                    // next_start+=(new_size-next_start%new_size);
                    next_start=init_val->compute_glob_array_init_string(init_array,new_size,dims,next_start,s);
                }
            }
            return start+total_size;
        }
        assert(false);
    }

    int compute_array_init_string(string* init_array,int total_size,vector<int>& dims,int start,string& s) override{
        if(kind==Kind::Exp){
            init_array[start]=exp->GenerateIR(s);
            return start+1;
        }
        else if(kind==Kind::EmptyArray){
            return start+total_size;
        }
        else if(kind==Kind::Array){
            int next_start=start;
            for(auto& initval:initvals){
                auto init_val=(InitValAST*)initval.get();
                if(init_val->kind==Kind::Exp){
                    next_start=init_val->compute_array_init_string(init_array,1,dims,next_start,s);
                }
                else if(init_val->kind==Kind::EmptyArray){
                    int new_size=get_list_size(total_size,dims,next_start-start);
                    next_start=init_val->compute_array_init_string(init_array,new_size,dims,next_start,s);
                }
                else if(init_val->kind==Kind::Array){
                    int new_size=get_list_size(total_size,dims,next_start-start);
                    // next_start+=(new_size-next_start%new_size);
                    next_start=init_val->compute_array_init_string(init_array,new_size,dims,next_start,s);
                }
            }
            return start+total_size;
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        return exp->GenerateIR(s);;
    }
};

class VarDefAST:public BaseAST{
public:
    enum class Kind{Int_Ident,Array_Ident,Int_Init,Array_Init};
    Kind kind;
    std::string ident;
    vector<std::unique_ptr<BaseAST>> constexps;
    std::unique_ptr<BaseAST> initval;
    bool is_global=false;

    void set_global() override{
        is_global=true;
    }

    void Dump() const override{
        std::cout<<"VarDefAST { ";
        switch (kind){
            case Kind::Int_Ident:
                std::cout<<ident;
                break;
            case Kind::Int_Init:
                std::cout<<ident<<", ";
                initval->Dump();
                break;
            default:
                break;
        }
        std::cout<<" }";
    }

    int compute_exp() const override{
        int value=0;
        if (kind==Kind::Int_Init){
            value=initval->compute_exp();
            symbol_table->var_table[ident]=value;
            return value;
        }
        else{
            assert(false);
        }
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for(auto& constexp:constexps){
            constexp->set_symbol_table(symbol_table);
        }
        if (kind==Kind::Int_Init||kind==Kind::Array_Init)
            initval->set_symbol_table(symbol_table);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string name="@"+ident;
        if(var_count.find(ident)==var_count.end()){
            var_count[ident]=1;
            name+="_"+to_string(var_count[ident]);
        }
        else{
            var_count[ident]++;
            name+="_"+to_string(var_count[ident]);
        }
        symbol_table->Insert(ident,name);
        if(kind==Kind::Int_Ident||kind==Kind::Int_Init){
            if(is_global){
                s+="global "+name+" = alloc i32";
                if(kind==Kind::Int_Ident){
                    s+=", zeroinit\n";
                    //symbol_table->var_table[ident]=0;
                }
                else if(kind==Kind::Int_Init){
                    int v=initval->compute_exp();
                    symbol_table->var_table[ident]=v;
                    s+=", "+to_string(v)+'\n';
                }
            }
            else{
                s+="  "+name+" = alloc i32\n";
                if (kind==Kind::Int_Init){
                // symbol_table->var_table[ident]=initval->compute_exp();
                // cout<<"插入初始值"<<symbol_table->var_table[ident]<<endl;
                    string value=initval->GenerateIR(s);
                    s+="  store "+value+", "+name+'\n';
                }
            }
        }
        else if(kind==Kind::Array_Ident||kind==Kind::Array_Init){
            symbol_table->add_array(ident,constexps.size());
            string array_type="i32";
            vector<int> dims;
            int dim;
            int total_size=1;
            for (int i=constexps.size()-1;i>=0;i--){
                dim=constexps[i]->compute_exp();
                array_type='['+array_type+", "+to_string(dim)+']';
                dims.push_back(dim);
                total_size*=dim;
            }
            if(kind==Kind::Array_Ident){
                if(is_global){
                    s+="global "+name+" = alloc "+array_type+", zeroinit\n";
                }
                else{
                    s+="  "+name+" = alloc "+array_type+'\n';
                }
            }
            else if(kind==Kind::Array_Init){
                string* init_array=new string[total_size];
                for(int i=0;i<total_size;i++){
                    init_array[i]="0";
                }
                if(is_global){
                    s+="global ";
                }
                else{
                    s+="  ";
                }
                if(is_global){
                    initval->compute_glob_array_init_string(init_array,total_size,dims,0,s);
                }
                else{
                    initval->compute_array_init_string(init_array,total_size,dims,0,s);
                }
                s+=name+" = alloc "+array_type;
                if(is_global){
                    initval->set_global();
                    s+=", ";
                    auto init_val=(InitValAST*)initval.get();
                    if(init_val->kind==InitValAST::Kind::EmptyArray){
                        s+="zeroinit\n";
                    }
                    else if(init_val->kind==InitValAST::Kind::Array){
                        string initlist;
                        vector<int> indexs(dims.size(),0);
                        int current_size=1;
                        for(int i=0;i<dims.size();i++){
                            current_size*=dims[i];
                            indexs[i]=current_size;
                        }
                        for(int i=0;i<total_size;i++){
                            for(int j=dims.size()-1;j>=0;j--){
                                if(i%indexs[j]==0){
                                    initlist+="{";
                                }
                            }
                            initlist+=init_array[i]+", ";
                            for(int j=0;j<dims.size();j++){
                                if((i+1)%indexs[j]==0){
                                    initlist.pop_back();
                                    initlist.pop_back();
                                    initlist+="}, ";
                                }
                            }
                        }
                        initlist.pop_back();
                        initlist.pop_back();
                        s+=initlist+'\n';
                    }
                }
                else{
                    s+='\n';
                    string val_name;
                    string prev_name;
                    vector<int> indexs(dims.size(),0);
                    for(int i=0;i<total_size;i++){
                        prev_name=name;
                        for(int j=dims.size()-1;j>=0;j--){
                            val_name="%"+to_string(val_num++);
                            s+="  "+val_name+" = getelemptr "+prev_name+", "+to_string(indexs[j])+'\n';
                            prev_name=val_name;
                        }
                        s+="  store "+init_array[i]+", "+prev_name+'\n';
                        indexs[0]++;
                        for(int j=0;j<dims.size()-1;j++){
                            if(indexs[j]==dims[j]){
                                indexs[j]=0;
                                indexs[j+1]++;
                            }
                        }
                    }
                }
                delete[] init_array;
            }
        }
        return "";
    }
};
