#pragma once

#include "base_ast.hpp"

int evaluate(BaseAST* root, std::shared_ptr<Environment> env);

class ExprAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> or_expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class UnaryExprAST : public BaseAST {
public:
    class InnerUnaryAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> unary_op;
        std::unique_ptr<BaseAST> unary_expr;
        void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
    };

    class InnerPrimaryAST: public BaseAST {
    public:
        std::unique_ptr<BaseAST> primary_expr;
        void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
    };
    enum Type {UNARY, PRIMARY, FUNC};
    Type type;
    std::unique_ptr<BaseAST> content;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override {
        content->Dump(out, env);
        dump_message = content->dump_message;
    }
};

class UnaryOpAST : public BaseAST {
public:
    enum Op {ADD, DEC, TAN};
    Op op;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override {}
};

class PrimaryExprAST : public BaseAST {
public:
    enum Type {EXPR, NUMBER, LVAL};
    std::unique_ptr<BaseAST> content;
    Type type;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override;
};

class NumberAST : public BaseAST {
public:
    int val;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override { out<<val; }
};

class AddExprAST : public BaseAST {
public:
    enum Type {MULADD, MUL};
    enum Op {ADD, DEC};

    Type type;

    // optional
    Op opt_op;

    // optional
    std::unique_ptr<BaseAST> opt_add_expr;

    std::unique_ptr<BaseAST> mul_expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override;
};

class MulExprAST : public BaseAST {
public:
    enum Type {UNARY, MULUNARY};
    enum Op {MUL, DIV, MOD};

    Type type;

    // optional
    Op opt_op;

    // optional
    std::unique_ptr<BaseAST> opt_mul_expr;

    std::unique_ptr<BaseAST> unary_expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override;
};

class RelExprAST : public BaseAST {
public:
    enum Type {ADD, RELADD};
    enum Op {LE, GE, LT, GT};

    Type type;
    
    // optional
    Op opt_op;

    // optional
    std::unique_ptr<BaseAST>opt_rel_expr;
    
    std::unique_ptr<BaseAST> add_expr;

    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};


class EqExprAST : public BaseAST {
public:
    enum Type {REL, EQREL};
    enum Op {EQ, NE};

    Type type;

    Op opt_op;

    std::unique_ptr<BaseAST> opt_eq_expr;

    std::unique_ptr<BaseAST> rel_expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class LAndExprAST : public BaseAST {
public:
    enum Type {EQ, ANDEQ};
    
    Type type;

    std::unique_ptr<BaseAST> opt_and_expr;
    std::unique_ptr<BaseAST> eq_expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class LOrExprAST : public BaseAST {
public:
    enum Type {AND, ORAND};

    Type type;

    std::unique_ptr<BaseAST> opt_or_expr;
    std::unique_ptr<BaseAST> and_expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;

};
