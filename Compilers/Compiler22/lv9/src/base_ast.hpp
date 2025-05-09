#pragma once
#include <iostream>
#include <memory>
#include <map>
#include <vector>

void registerNewBasicBlock();
void cancelNewBasicBlock();
void willOutputAInst(std::ostream &out);




std::string getANewTmpId();

class Environment{
public:
    std::shared_ptr<Environment> parent;
    bool isRoot();
    std::map<std::string, std::string> ident_to_koopa;
    std::map<std::string, std::string> dev_info;
    std::map<std::string, std::string> type_of_function;

    // zhizhen, shuzu, default int32
    std::map<std::string, std::string> type_of_ident;

    // []的数量
    std::map<std::string, int>arrayslen; 

    std::string getTypeOfIdent(std::string id);
    int getArraysLen(std::string id);

    // type: zhizhen, shuzu default int32, after
    void setTypeOfIdent(std::string id, std::string type);

    void setArraysLen(std::string id, int len);

    // return "" if not find
    std::string getIdForIdent(std::string ident);

    // return false if failed
    bool allocIdForIdent(std::string ident);

    // return "" if not find
    std::string getDevInfo(std::string key);

    // return false if key already exist in current env
    bool trySetDevInfo(std::string key, std::string value);

    void forceSetDevInfo(std::string key, std::string value);

    // return "" if not found
    std::string getTypeOfFunction(std::string id);

    std::string getIdOfFunction(std::string key);

    void forceSetTypeOfFunction(std::string key, std::string value);


    static std::string WHILE_END_ID, WHILE_DEC_ID;
    static std::string INIT_SIZE, CUR_ARRAY_PTR;

};

// 所有 AST 的基类
class BaseAST {
public:
    // after dump, keep some message based on diff AST
    std::string dump_message;
    static unsigned tmp_sym_cnt;
    virtual ~BaseAST() = default;
    virtual void Dump(std::ostream &out, std::shared_ptr<Environment> env) = 0;
    
};


// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    // back is actually first
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > global_units;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};


// function or global variable;
class GlobalUnitAST : public BaseAST {
public:
    enum Type {DECL, FUNCDEF};
    Type type;
    std::unique_ptr<BaseAST> content;

    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::unique_ptr<std::string> ident;
    std::unique_ptr<BaseAST> func_params;
    std::unique_ptr<BaseAST> block;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
    
};

class FuncTypeAST : public BaseAST {
public:

    // int or void
    std::unique_ptr<std::string> val;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
    
};

class FuncDefManyParamsAST : public BaseAST {
public:
    unsigned dump_times = 0;
    // back is actually first
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > func_def_params;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class FuncDefOneParamAST : public BaseAST {
public:
    unsigned dump_times = 0;
    std::string dev_first_name;

    // maybe not null but with size == 0
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > opt_array_exprs;
    std::string ident;
    std::string btype;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};


class FuncCallAST : public BaseAST {
public:
    std::string ident;
    // back is actually first
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > func_call_params;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class FuncCallOneParamAST : public BaseAST {

public:
    std::unique_ptr<BaseAST> expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;

};



class BlockAST : public BaseAST {
public:
    
    // the back is actually the first
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > block_items;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
    
};

class BlockItemAST : public BaseAST {
public:
    enum Type {DECL, STMT};
    Type type;
    std::unique_ptr<BaseAST> content;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class StmtAST : public BaseAST {
public:
    enum Type{ASSIGN, RETURN, BLOCK, EXPR, EMPTY, IF, WHILE,
                BREAK, CONTINUE};
    Type type;

    // expr or block
    std::unique_ptr<BaseAST> content;

    std::unique_ptr<BaseAST> opt_lval;

    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
    
};



// ...
