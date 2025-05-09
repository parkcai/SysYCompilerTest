#include "if_ast.hpp"


// manually assign basic block
void IfStmtAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    expr -> Dump(out, env);
    auto then_id = getANewTmpId();
    auto else_id = getANewTmpId();
    auto dec_id  = getANewTmpId();
    auto end_id  = getANewTmpId();
    
    willOutputAInst(out);
    out<<"br 1, "<<dec_id<<", "<<dec_id<<std::endl;

    out<<then_id<<":"<<std::endl;
    cancelNewBasicBlock();

    then->Dump(out, env);
    willOutputAInst(out);
    out<<"br 1, "<<end_id<<", "<<end_id<<std::endl;

    out<<else_id<<":"<<std::endl;
    cancelNewBasicBlock();
    if (type==IFELSE) {
        opt_else->Dump(out, env);
    }
    willOutputAInst(out);
    out<<"br 1, "<<end_id<<", "<<end_id<<std::endl;
    
    out<<dec_id<<":"<<std::endl;
    cancelNewBasicBlock();
    out<<"br "<<expr->dump_message<<", "<<then_id<<", "<<else_id<<std::endl;

    out<<end_id<<":"<<std::endl;
    cancelNewBasicBlock();
}