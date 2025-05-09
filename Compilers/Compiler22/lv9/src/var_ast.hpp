#pragma once

#include "base_ast.hpp"

#include <vector>

class DeclAST : public BaseAST {
public:
    enum Type {CONST, VAR};
    Type type;
    std::unique_ptr<BaseAST> content;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class ConstDeclAST : public BaseAST {
public:
    std::string btype;

    // the back is actually the first statement
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > >const_defs;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};


class ConstDefAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_initval;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > opt_array_exprs;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class ConstInitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> opt_const_expr;

    // also const
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > >opt_const_initvals;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class LValAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > opt_array_exprs;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};



class ConstExprAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> expr;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};


class VarDeclAST : public BaseAST {
public:
    std::string btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > var_defs;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

class VarDefAST : public BaseAST {
public:
    enum Type{INIT, NOINI};
    std::string ident;
    Type type;
    std::unique_ptr<BaseAST> opt_initval;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > opt_array_exprs;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};


class InitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> opt_expr;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > opt_initvals;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env) override;
};

