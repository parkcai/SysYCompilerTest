#pragma once

#include "base_ast.hpp"

class IfStmtAST : public BaseAST{
public:
    enum Type{ONLYIF, IFELSE};
    Type type;
    std::unique_ptr<BaseAST> expr;
    std::unique_ptr<BaseAST> then;
    std::unique_ptr<BaseAST> opt_else;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override;
};