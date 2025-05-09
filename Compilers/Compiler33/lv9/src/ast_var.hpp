#pragma once

#include "ast_exp.hpp"
#include "ast.hpp"

void get_arr_elem(int pos, const std::vector<int> &arr_sizes, const std::string &arr_name);

//  Decl ::= ConstDecl VarDecl
class DeclAST : public BaseAST {
public:
    past_t const_decl, var_decl;
    void Dump() const override;
    void DumpIR() const override;
};

//  ConstDecl ::= "const" BType ConstDef {"," ConstDef} ";"
class ConstDeclAST : public BaseAST {
public:
    s_t b_type;
    std::unique_ptr<std::vector<past_t> > const_def_list;
    void Dump() const override;
    void DumpIR() const override;
};

//  VarDecl ::= BType VarDef {"," VarDef} ";"
class VarDeclAST : public BaseAST {
public:
    s_t b_type;
    std::unique_ptr<std::vector<past_t> > var_def_list;
    void Dump() const override;
    void DumpIR() const override;
};

//  ConstDef ::= IDENT "=" ConstInitVal
class ConstDefAST : public BaseAST {
public:
    s_t ident;
    past_t const_init_val;
    std::unique_ptr<std::vector<past_t> > arr_exp_list;
    void Dump() const override;
    void DumpIR() const override;
};

//  VarDef ::= IDENT | IDENT "=" InitVal; 
class VarDefAST : public BaseAST {
public:
    s_t ident;
    past_t init_val;
    std::unique_ptr<std::vector<past_t> > arr_exp_list;
    void Dump() const override;
    void DumpIR() const override;
};

//  ConstInitVal ::= ConstExp 
class ConstInitValAST : public BaseAST {
public:
    past_t const_exp;
    std::unique_ptr<std::vector<past_t> > const_init_val_list;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
    void Aggregate(const std::vector<int> &arr_sizes) const;
    std::vector<int> all_elem(const std::vector<int> &arr_sizes) const;
};

//  InitVAl ::= Exp
class InitValAST : public BaseAST {
public:
    past_t exp;
    std::unique_ptr<std::vector<past_t> > init_val_list;
    void Dump() const override;
    void DumpIR() const override;
    int Eval() const override;
    void Aggregate(const std::vector<int> &arr_sizes) const;
    std::vector<BaseAST*> all_elem(const std::vector<int> &arr_sizes) const;
};
