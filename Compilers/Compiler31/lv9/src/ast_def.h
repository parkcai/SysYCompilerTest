
#ifndef AST_DEF_H
#define AST_DEF_H

#include <iostream>
#include <memory>
#include <algorithm>
#include "koopa.h"
#include "utils.h"
struct BaseAST {
    virtual ~BaseAST() = default;
    virtual void dump() const = 0;
    virtual void *to_koopa(koopa_raw_type_t type) const { return nullptr; }
    virtual void *to_koopa() const { return nullptr; }
    virtual void *to_koopa(int index) const { return nullptr; }
    virtual void *to_type() const { return nullptr; }
    virtual void *to_koopa(vec<const void *> *global) const { return nullptr; }
    virtual void *to_koopa(koopa_raw_type_t type, vec<const void *> *global) const { return nullptr; }
    virtual int get_val() const { return 0; }
    virtual void *to_koopa_leftvalue() const { return nullptr; }
    virtual str show_type() const { return "base"; }
};
using ptr = std::unique_ptr<BaseAST>;
using str = std::string;
using std::reverse;
using std::cout;
using std::unique_ptr;
using std::endl;
using std::function;
#define def(x) \
    struct x : BaseAST
#define TO_KOOPA void *to_koopa() const override
#define get_val int get_val() const override
def(CompUnitAST) {
    // vec of funcDefAST
    unique_ptr<vec<ptr>> arr;
    TO_KOOPA;
    void dump() const override {
        cout << "CompUnitAST { ";
        for (auto &inst : *arr) {
            inst->dump();
            cout << endl;
        }
        cout << " }";
    }
};
def(FuncDefAST) {
    ptr func_type;
    str ident;
    unique_ptr<vec<ptr>> arr;
    ptr block;
    TO_KOOPA;
    str show_type() const override { return "FuncDefAST"; }
    void dump() const override {
        cout << "FuncDefAST { ";
        func_type->dump();
        cout << " " << ident << " ";
        block->dump();
        cout << " }";
    }
};
def(FuncTypeAST){
    str ident;
    TO_KOOPA;
    void dump() const override {
        cout << "FuncTypeAST { " << ident << " }";
    }
};
def(FuncFParamAST) {
    ptr btype;
    str ident;
    unique_ptr<vec<ptr>> arr;
    void *to_type() const override;
    void *to_koopa(int index) const override;
    void dump() const override {
        cout << "FuncFParamAST { " << ident << " }";
    }
};
// sysY语言中的Block，非koopa IR中的Block
def(BlockAST) {
    // vec of blockItemAST
    unique_ptr<vec<ptr>> arr;
    TO_KOOPA;
    void dump() const override {
        cout << "BlockAST { ";
        for (auto &inst : *arr) {
            inst->dump();
            cout << endl;
        }
        cout << " }";
    }
};
def(StmtAST) {
    enum StmtType {
        RETURN,
        ASSIGN,
        EXP,
        EMPTY,
        BLOCK,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
    };
    StmtType kind;
    ptr stmt;
    ptr exp;
    TO_KOOPA;
    void dump() const override {
        cout << "StmtAST { ";
        if (stmt != nullptr) stmt->dump();
        if (exp != nullptr) exp->dump();
        cout << " }";
    }
};
def(NumberAST) {
    int val;
    NumberAST(int val);
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "NumberAST { " << val << " }";
    }
};
// lab2: Exp PrimaryExp UnaryExp UnaryOp
def(ExpAST) {
    ptr exp; // AddExp
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "ExpAST { ";
        exp->dump();
        cout << " }";
    }
};
def(PrimaryExpAST) {
    ptr exp; // Exp or Number or LVal
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "PrimaryExpAST { ";
        exp->dump();
        cout << " }";
    }
};
def(UnaryExpAST) {
    str op; // unaryop or "" or function_call
    ptr exp;
    unique_ptr<vec<ptr>> arr; // for function params
    TO_KOOPA;
    get_val;
    
    void dump() const override {
        cout << "UnaryExpAST { ";
        if (op != "") cout << op;
        if (exp != nullptr) exp->dump();
        cout << " }";
    }
};
def(AddExpAST) {
    /** @brief 运算符左侧exp1是AddExp, 可能为nullptr */
    ptr exp1;
    /** @brief "+", "-", or "" */
    str op;
    /** @brief 运算符右侧exp2是MulExp */
    ptr exp2;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "AddExpAST { ";
        if (exp1 != nullptr) exp1->dump();
        cout << op;
        exp2->dump();
        cout << " }";
    }
};
def(MulExpAST) {
    /** @brief 运算符左侧exp1是MulExp, 可能为nullptr */
    ptr exp1;
    /** @brief "*", "/", "%", or "" */
    str op;
    /** @brief 运算符右侧exp2是UnaryExp */
    ptr exp2;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "MulExpAST { ";
        if (exp1 != nullptr) exp1->dump();
        cout << op;
        exp2->dump();
        cout << " }";
    }
};
def(RelExpAST) {
    /** @brief 运算符左侧exp1是RelExp, 可能为nullptr */
    ptr exp1;
    /** @brief "<", ">", "<=", ">=", or "" */
    str op;
    /** @brief 运算符右侧exp2是AddExp */
    ptr exp2;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "RelExpAST { ";
        if (exp1 != nullptr) exp1->dump();
        cout << op;
        exp2->dump();
        cout << " }";
    }
};
def(EqExpAST) {
    /** @brief 运算符左侧exp1是EqExp, 可能为nullptr */
    ptr exp1;
    /** @brief "==", "!=", or "" */
    str op;
    /** @brief 运算符右侧exp2是RelExp */
    ptr exp2;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "EqExpAST { ";
        if (exp1 != nullptr) exp1->dump();
        cout << op;
        exp2->dump();
        cout << " }";
    }
};
def(LAndExpAST) {
    /** @brief 运算符左侧exp1是LAndExp, 可能为nullptr */
    ptr exp1;
    /** @brief "&&", or "" */
    str op;
    /** @brief 运算符右侧exp2是EqExp */
    ptr exp2;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "LAndExpAST { ";
        if (exp1 != nullptr) exp1->dump();
        cout << op;
        exp2->dump();
        cout << " }";
    }
};
def(LOrExpAST) {
    /** @brief 运算符左侧exp1是LOrExp, 可能为nullptr */
    ptr exp1;
    /** @brief "||", or "" */
    str op;
    /** @brief 运算符右侧exp2是LAndExp */
    ptr exp2;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "LOrExpAST { ";
        if (exp1!= nullptr) exp1->dump();
        cout << op;
        exp2->dump();
        cout << " }";
    }
};
def(ConstDeclAST) {
    ptr btype;
    // const def
    unique_ptr<vec<ptr>> arr;
    TO_KOOPA;
    void *to_koopa(vec<const void *> *global) const override;
    void dump() const override {
        cout << "ConstDeclAST { ";
        btype->dump();
        for (auto &def : *arr) {
            def->dump();
            cout << endl;
        }
        cout << " }";
    }
};
def(BTypeAST) {
    str ident;
    TO_KOOPA;
    void dump() const override {
        cout << "BType { " << ident << " }";
    }
};
def(ConstDefAST) {
    int kind = 0; // var or array
    str ident;
    // ConstInitValAST
    ptr exp;
    unique_ptr<vec<ptr>> arr; // index
    void *to_koopa(koopa_raw_type_t type) const override;
    void *to_koopa(koopa_raw_type_t type, vec<const void *> *global) const override;
    void dump() const override {
        cout << "ConstDefAST { " << ident;
        if (exp != nullptr) {
            cout << " = ";
            exp->dump();
        }
        cout << " }";
    }
};
def(LValAST) {
    str ident;
    unique_ptr<vec<ptr>> arr;
    TO_KOOPA;
    get_val;
    void *to_koopa_leftvalue() const override;
    void dump() const override {
        cout << "LValAST { " << ident;
        cout << " }";
    }
};
def(ConstInitValAST) {
    ptr exp;
    unique_ptr<vec<ptr>> arr;
    TO_KOOPA;
    get_val;
    void get_arr_init(vec<const void *> *arr_init, vec<int> dims) const;
    void *to_koopa(int index) const override;
    void dump() const override {
        cout << "ConstInitValAST { ";
        if (exp != nullptr) exp->dump();
        cout << " }";
    }
};
def(VarDeclAST) {
    ptr btype;
    // var def
    unique_ptr<vec<ptr>> arr;
    TO_KOOPA;
    void *to_koopa(vec<const void *> *global) const override;
    void dump() const override {
        cout << "VarDeclAST { ";
        btype->dump();
        for (auto &def : *arr) {
            def->dump();
            cout << endl;
        }
        cout << " }";
    }
};
def(VarDefAST) {
    enum VarDefType {
        VAR,
        ARRAY
    };
    VarDefType kind;
    str ident;
    unique_ptr<vec<ptr>> arr;
    ptr exp;
    void *to_koopa(koopa_raw_type_t type) const override;
    void *to_koopa(koopa_raw_type_t type, vec<const void *> *global) const override;
    void dump() const override {
        cout << "VarDefAST { " << ident;
        if (exp != nullptr) {
            cout << " = ";
            exp->dump();
        }
        cout << " }";
    }
};
def(InitValAST) {
    ptr exp;
    unique_ptr<vec<ptr>> arr; // { array value }
    void get_arr_init(vec<const void *> *arr_init, vec<int> dims) const;
    TO_KOOPA;
    get_val;
    void dump() const override {
        cout << "InitValAST { ";
        if (exp != nullptr) exp->dump();
        cout << " }";
    }
};
def(IfAST) {
    ptr exp;
    ptr stmt;
    TO_KOOPA;
    void dump() const override {
        cout << "IfAST { ";
        exp->dump();
        cout << " }";
        stmt->dump();
        cout << " }";
    }
};
def(WhileAST) {
    ptr exp;
    ptr stmt;
    TO_KOOPA;
    void dump() const override {
        cout << "WhileAST { ";
        exp->dump();
        cout << " }";
        stmt->dump();
        cout << " }";
    }
};
void *to_bool(koopa_raw_value_t val);
#undef def
#undef TO_KOOPA
#undef get_val

#endif