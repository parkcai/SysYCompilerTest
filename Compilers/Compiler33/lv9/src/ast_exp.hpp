#pragma once

#include "ast.hpp"

//  Exp ::= LOrExp
class ExpAST : public BaseAST {
public:
    past_t l_or_exp;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  RelExpAST ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp
class RelExpAST : public BaseAST {
public:
    past_t add_exp, rel_exp;
    char rel_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  EqExp ::= RelExp | EqExp ("==" | "!=") RelExp
class EqExpAST : public BaseAST {
public:
    past_t rel_exp, eq_exp;
    char eq_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  LAndExp ::= EqExp | LAndExp "&&" EqExp
class LAndExpAST : public BaseAST {
public:
    past_t eq_exp, l_and_exp;
    char l_and_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  LOrExp ::= LAndExp | LOrExp "||" LAndExp
class LOrExpAST : public BaseAST {
public:
    past_t l_and_exp, l_or_exp;
    char l_or_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  AddExp ::= MulExp | AddExp AddOp MulExp
class AddExpAST : public BaseAST {
public:
    past_t mul_exp, add_exp;
    char add_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  MulExp ::= UnaryExp | MulExp MulOp UnaryExp
class MulExpAST : public BaseAST {
public:
    past_t mul_exp, unary_exp;
    char mul_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  UnaryExp ::= PrimaryExp | UnaryOp UnaryExp
class UnaryExpAST : public BaseAST {
public:
    past_t primary_exp, unary_exp;
    std::unique_ptr<std::vector<past_t> > func_args;
    s_t func_name;
    char unary_op;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  PrimaryExp ::= "(" Exp ")" | LVal | Number 
class PrimaryExpAST : public BaseAST {
public:
    past_t exp;
    LVal *l_val;
    int number;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};

//  ConstExp ::= Exp
class ConstExpAST : public BaseAST {
public:
    past_t exp;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
};