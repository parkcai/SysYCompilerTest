#include "while_ast.hpp"

#include <cassert>

void WhileStmtAST::Dump(std::ostream & out, std::shared_ptr<Environment> env)
{
    auto then_id = getANewTmpId();
    auto else_id = getANewTmpId();
    auto dec_id  = getANewTmpId();
    auto end_id  = getANewTmpId();

    std::string prev_dec_id = env->getDevInfo(Environment::WHILE_DEC_ID);
    std::string prev_end_id = env->getDevInfo(Environment::WHILE_END_ID);

    env->forceSetDevInfo(Environment::WHILE_DEC_ID, dec_id);
    env->forceSetDevInfo(Environment::WHILE_END_ID, end_id);
    
    willOutputAInst(out);
    out<<"br 1, "<<dec_id<<", "<<dec_id<<std::endl;

    out<<then_id<<":"<<std::endl;
    cancelNewBasicBlock();

    body->Dump(out, env);
    willOutputAInst(out);
    out<<"br 1, "<<dec_id<<", "<<dec_id<<std::endl;

    out<<else_id<<":"<<std::endl;
    cancelNewBasicBlock();
    willOutputAInst(out);
    out<<"br 1, "<<end_id<<", "<<end_id<<std::endl;
    
    out<<dec_id<<":"<<std::endl;
    cancelNewBasicBlock();
    expr->Dump(out, env);
    willOutputAInst(out);
    out<<"br "<<expr->dump_message<<", "<<then_id<<", "<<else_id<<std::endl;

    out<<end_id<<":"<<std::endl;
    cancelNewBasicBlock();
    assert((prev_dec_id!="")==(prev_end_id!=""));
    if(prev_dec_id != "") {
        env->forceSetDevInfo(Environment::WHILE_DEC_ID, prev_dec_id);
        env->forceSetDevInfo(Environment::WHILE_END_ID, prev_end_id);
    }
}
