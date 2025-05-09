// src/ast.h
#ifndef AST_H
#define AST_H

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <stdexcept>
#include "symtab.h"

using namespace std;

// Base class for all AST nodes
class BaseAST {
public:
    virtual ~BaseAST() = default;

    // Generate IR code
    virtual void GenerateIR(ostream& out) const = 0;

    // Store the IR name (e.g., %0, %1)
    mutable string ir_name;

    // Evaluate the expression (for constant folding)
    virtual EvalResult Eval() const {
        return EvalResult();
    }

    // Check if the statement will terminate (e.g., return statement)
    virtual bool IsTerminated() const {
        return false;
    }
};

// CompUnitAST
class CompUnitAST : public BaseAST {
public:
    vector<BaseAST*> comp_unit_items;
    CompUnitAST(){}
    void GenerateIR(ostream& out) const override;
};

// NumberAST
class NumberAST : public BaseAST {
public:
    int value;

    NumberAST(int v) : value(v) {}

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// TypeAST
class TypeAST : public BaseAST {
public:
    string type;

    TypeAST(const string& t) : type(t) {}

    void GenerateIR(ostream& out) const override;
};

// FuncFParamAST
class FuncFParamAST : public BaseAST {
public:
    string btype;
    string ident;
    bool is_array = false;
    vector<BaseAST*> array_dims; // 后续维度的长度

    void GenerateIR(ostream& out) const override;
};

// FuncDefAST
class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type; // FuncTypeAST
    string ident;
    vector<BaseAST*> params;       // List of parameters (FuncFParamAST*)
    unique_ptr<BaseAST> block;     // BlockAST
    FuncDefAST(){}
    ~FuncDefAST(); // Destructor to delete parameters

    void GenerateIR(ostream& out) const override;
};

// StmtAST
class StmtAST : public BaseAST {
public:
    virtual void GenerateIR(ostream& out) const override = 0;
};

// BlockAST
class BlockAST : public BaseAST {
public:
    vector<BaseAST*> block_items;

    ~BlockAST(); // Destructor to delete block items

    void GenerateIR(ostream& out) const override;
    bool IsTerminated() const override;
};

// UnaryExpAST
class UnaryExpAST : public BaseAST {
public:
    string op;
    unique_ptr<BaseAST> operand;

    UnaryExpAST(const string& _op, unique_ptr<BaseAST> _operand)
        : op(_op), operand(move(_operand)) {}

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// BinaryExpAST
class BinaryExpAST : public BaseAST {
public:
    string op;
    unique_ptr<BaseAST> lhs;
    unique_ptr<BaseAST> rhs;

    BinaryExpAST(const string& _op, unique_ptr<BaseAST> _lhs, unique_ptr<BaseAST> _rhs)
        : op(_op), lhs(move(_lhs)), rhs(move(_rhs)) {}

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// LAndExpAST
class LAndExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lhs;
    unique_ptr<BaseAST> rhs;

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// LOrExpAST
class LOrExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lhs;
    unique_ptr<BaseAST> rhs;

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// LValAST
class LValAST : public BaseAST {
public:
    string ident;
    bool is_array = false;
    vector<unique_ptr<BaseAST>> index_exps;

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// ConstDefAST
class ConstDefAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> const_init_val;
    bool is_array = false;
    vector<unique_ptr<BaseAST>> array_dimensions; // 多维数组的各个维度

    void GenerateIR(ostream& out) const override;
    EvalResult Eval() const override;
};

// ConstDeclAST
class ConstDeclAST : public BaseAST {
public:
    string btype;
    vector<ConstDefAST*> const_defs;

    ~ConstDeclAST();

    void GenerateIR(ostream& out) const override;
};

// VarDefAST
class VarDefAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> init_val;
    bool is_array = false;
    vector<unique_ptr<BaseAST>> array_dimensions; // 多维数组的各个维度

    void GenerateIR(ostream& out) const override;
};

// VarDeclAST
class VarDeclAST : public BaseAST {
public:
    string btype;
    vector<VarDefAST*> var_defs;

    ~VarDeclAST();

    void GenerateIR(ostream& out) const override;
};

// AssignStmtAST
class AssignStmtAST : public StmtAST {
public:
    unique_ptr<BaseAST> lval; // LValAST*
    unique_ptr<BaseAST> exp;

    void GenerateIR(ostream& out) const override;
};

// ReturnStmtAST
class ReturnStmtAST : public StmtAST {
public:
    unique_ptr<BaseAST> exp; // May be null for void functions

    void GenerateIR(ostream& out) const override;
    bool IsTerminated() const override;
};

// EmptyStmtAST
class EmptyStmtAST : public StmtAST {
public:
    void GenerateIR(ostream& out) const override;
};

// ExpStmtAST
class ExpStmtAST : public StmtAST {
public:
    unique_ptr<BaseAST> exp;

    ExpStmtAST(unique_ptr<BaseAST> _exp) : exp(move(_exp)) {}
    ExpStmtAST() : exp(nullptr) {}

    void GenerateIR(ostream& out) const override;
};

// IfStmtAST
class IfStmtAST : public StmtAST {
public:
    unique_ptr<BaseAST> cond;
    unique_ptr<BaseAST> then_stmt;
    unique_ptr<BaseAST> else_stmt; // Optional

    void GenerateIR(ostream& out) const override;
    bool IsTerminated() const override;
};

// WhileStmtAST
class WhileStmtAST : public StmtAST {
public:
    unique_ptr<BaseAST> cond;
    unique_ptr<BaseAST> body;

    void GenerateIR(ostream& out) const override;
    bool IsTerminated() const override;
};

// BreakStmtAST
class BreakStmtAST : public StmtAST {
public:
    void GenerateIR(ostream& out) const override;
    bool IsTerminated() const override;
};

// ContinueStmtAST
class ContinueStmtAST : public StmtAST {
public:
    void GenerateIR(ostream& out) const override;
    bool IsTerminated() const override;
};

// FuncCallAST
class FuncCallAST : public BaseAST {
public:
    string ident;
    vector<BaseAST*> params; // List of arguments

    FuncCallAST(const string& _ident) : ident(_ident) {}

    ~FuncCallAST(); // Destructor to delete parameters

    void GenerateIR(ostream& out) const override;
};

// 新增 AggregateAST 用于表示数组的初始化列表
class AggregateAST : public BaseAST {
public:
    vector<BaseAST*> elements;

    ~AggregateAST() {
        for(auto elem : elements){
            delete elem;
        }
    }

    void GenerateIR(ostream& out) const override{}
    
    EvalResult Eval() const override {
        return EvalResult();
    }
};

#endif // AST_H
