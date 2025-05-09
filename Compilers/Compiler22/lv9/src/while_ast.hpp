#pragma once

#include "base_ast.hpp"

// can use environment to transfer id of beginning and end
class WhileStmtAST : public BaseAST{
public:
    std::unique_ptr<BaseAST> expr;
    std::unique_ptr<BaseAST> body;
    void Dump(std::ostream &out, std::shared_ptr<Environment> env)override;
};
