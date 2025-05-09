#pragma once

#include <sstream>
#include <string>

#include "SymbolTable.h"
               
class BaseAST {
public:
    mutable int var_idx = 0;

    virtual ~BaseAST() = default;

    virtual void GenerateIR() const = 0;

    virtual int CompValue() const = 0;
};

class CompUnitAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> decl_func_list;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class DeclAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class ConstDeclAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> const_def;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class ConstDefAST : public BaseAST {
public:
    std::string ident;
    std::vector<std::unique_ptr<BaseAST>> const_exps;
    std::unique_ptr<BaseAST> const_init_val;

    int CompValue() const override;
    void GenerateIR() const override;
};

class ConstInitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> const_exp;
    std::vector<std::unique_ptr<BaseAST>> const_init_vals;
    int CompValue() const override;
    void GenerateIR() const override {}
};

class ConstExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    int CompValue() const override;
    void GenerateIR() const override {}
};

class VarDeclAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> var_def;
    Type type;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class VarDefAST : public BaseAST {
public:
    std::string ident;
    std::vector<std::unique_ptr<BaseAST>> const_exps;
    std::unique_ptr<BaseAST> init_val;

    int CompValue() const override;
    void GenerateIR() const override;
};

class InitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::vector<std::unique_ptr<BaseAST>> init_vals;

    int CompValue() const override;
    void GenerateIR() const override;
};

class FuncDefAST : public BaseAST {
public:
    std::string func_type;
    std::string ident;
    std::vector<std::unique_ptr<BaseAST>> func_params;
    std::unique_ptr<BaseAST> block;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class FuncFParamAST : public BaseAST {
public:
    std::string type;
    std::string ident;
    std::vector<std::unique_ptr<BaseAST>> const_exps;

    mutable std::string final_type;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
    void FParamIR() const;
};

class BlockAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> block_items;
    int while_idx = 0;
    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class BlockItemAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;
    int while_idx = 0;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class StmtAST : public BaseAST {
public:
    int while_idx = 0;
    std::unique_ptr<BaseAST> matched_stmt;
    std::unique_ptr<BaseAST> open_stmt;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class MatchedStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> assign_stmt;
    std::unique_ptr<BaseAST> exp_stmt;
    std::unique_ptr<BaseAST> return_stmt;
    std::unique_ptr<BaseAST> while_stmt;
    std::unique_ptr<BaseAST> while_jump_stmt;
    int while_idx = 0;

    // if exp matched_stmt else matched_stmt
    std::unique_ptr<BaseAST> cond;
    std::unique_ptr<BaseAST> then_stmt;
    std::unique_ptr<BaseAST> else_stmt;

    int CompValue() const override;
    void GenerateIR() const override;
};

class OpenStmtAST : public BaseAST {
public:
    // if exp stmt | if exp match_stmt else open_stmt
    std::unique_ptr<BaseAST> cond;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> then_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    int while_idx = 0;

    int CompValue() const override;
    void GenerateIR() const override;
};

class AssignStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;

    int CompValue() const override;
    void GenerateIR() const override;
};

class ExpStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    int CompValue() const override;
    void GenerateIR() const override;
};

class ReturnStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    int CompValue() const override;
    void GenerateIR() const override;
};

class WhileStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> cond;
    std::unique_ptr<BaseAST> stmt;

    int CompValue() const override;
    void GenerateIR() const override;
};

class WhileJumpStmtAST : public BaseAST {
public:
    std::string type;
    int while_idx;

    int CompValue() const override { return 0; }
    void GenerateIR() const override;
};

class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> lor_exp;

    int CompValue() const override;
    void GenerateIR() const override;
};

class LOrExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> lor_exp;

    int CompValue() const override;
    void GenerateIR() const override;
};

class LAndExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    int CompValue() const override;
    void GenerateIR() const override;
};

class EqExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> eq_exp;
    std::string eq_op;

    int CompValue() const override;
    void GenerateIR() const override;
};

class RelExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string rel_op;

    int CompValue() const override;
    void GenerateIR() const override;
};

class AddExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string add_op;

    int CompValue() const override;
    void GenerateIR() const override;
};

class MulExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;
    std::string mul_op;

    int CompValue() const override;
    void GenerateIR() const override;
};

class UnaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> primary_exp;
    std::string unary_op;
    std::unique_ptr<BaseAST> unary_exp;

    std::string ident;
    std::vector<std::unique_ptr<BaseAST>> func_params;

    int CompValue() const override;
    void GenerateIR() const override;
};

class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    int number;

    int CompValue() const override;
    void GenerateIR() const override;
};

class LValAST : public BaseAST {
public:
    std::string ident;
    std::vector<std::unique_ptr<BaseAST>> exps;

    int CompValue() const override;
    void GenerateIR() const override;
};