#include "expr_ast.hpp"
#include "var_ast.hpp"

#include <cassert>


void ExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    or_expr->Dump(out, env);
    dump_message = or_expr->dump_message;
}

void UnaryExprAST::InnerPrimaryAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    primary_expr->Dump(out, env);
    dump_message = primary_expr->dump_message;
}

void UnaryExprAST::InnerUnaryAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    unary_expr->Dump(out, env);

    dump_message = getANewTmpId();
    willOutputAInst(out);
    out<<dump_message<<" = ";
    UnaryOpAST* tmp = static_cast<UnaryOpAST*> (unary_op.get());
    
    switch (tmp->op)
    {
    case UnaryOpAST::ADD:
        out <<" add 0, ";
        break;
    case UnaryOpAST::DEC:
        out <<" sub 0, ";
        break;
    case UnaryOpAST::TAN:
        out <<" eq 0, ";
        break;
    default:
        break;
    }
    out<<unary_expr->dump_message<<std::endl;    
}

void PrimaryExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    switch (type)
    {
    case EXPR:
        content->Dump(out, env);
        dump_message = content->dump_message;
        break;
    case NUMBER:

        dump_message = getANewTmpId();
        willOutputAInst(out);
        out<<dump_message<<" = ";

        out<<" add 0, ";
        content->Dump(out, env);
        out<<std::endl;
        break;
    case LVAL:
        content -> Dump(out, env);
        dump_message = getANewTmpId();
        willOutputAInst(out);
        if(isdigit(content->dump_message[0])) {
            out<<dump_message<<" = add 0, "<<content->dump_message<<std::endl;
        } else {
            auto tmp = dynamic_cast<LValAST*>(content.get());
            assert(tmp!=nullptr);
            auto ident = tmp->ident;
            auto ident_id = env->getIdForIdent(ident);
            std::string type = env->getTypeOfIdent(ident_id);
            assert(type=="shuzu"||type=="zhizhen"|| type=="i32");
            if(type=="i32") {
                out<<dump_message<<" = load "<<content->dump_message<<std::endl;
            } else {
                int should_len = env->getArraysLen(ident_id);
                int actual_len=0;
                if(tmp->opt_array_exprs!=nullptr)
                    actual_len = tmp->opt_array_exprs->size();
                assert(actual_len<=should_len);
                if(actual_len == should_len) {
                    out<<dump_message<<" = load "<<content->dump_message<<std::endl;
                } else {
                    if(type=="shuzu") {
                        out<<dump_message<<" = getelemptr "<<content->dump_message<<", 0"<<std::endl;
                    } else {
                        if(actual_len==0) {
                            out<<dump_message<<" = getptr "<<content->dump_message<<", 0"<<std::endl;
                        } else {
                            out<<dump_message<<" = getelemptr "<<content->dump_message<<", 0"<<std::endl;
                        }
                    }
                }
            }
            
        }
        
        break;
    
    default:
        break;
    }
}

void AddExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    switch (type)
    {
    case MULADD:
        opt_add_expr -> Dump(out, env);
        mul_expr -> Dump(out, env);
        dump_message = getANewTmpId();
        willOutputAInst(out);
        out<<dump_message<<" = "<<(opt_op == ADD ? " add ":" sub ")
            <<opt_add_expr -> dump_message<<", "
            << mul_expr->dump_message<<std::endl;
        
        break;
    case MUL:
        mul_expr -> Dump(out, env);
        dump_message = mul_expr->dump_message;
    default:
        break;
    }
}

void MulExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    switch (type)
    {
    case MULUNARY:
        opt_mul_expr -> Dump(out, env);
        unary_expr -> Dump(out, env);
        dump_message = getANewTmpId();
        willOutputAInst(out);
        out<<dump_message<<" = ";
        if (opt_op == MUL)
            out<<"mul";
        else if(opt_op == DIV)
            out<<"div";
        else
            out<<"mod";
        out<<" "<<opt_mul_expr -> dump_message<<", "
            << unary_expr->dump_message<<std::endl;
        
        break;
    case UNARY:
        unary_expr -> Dump(out, env);
        dump_message = unary_expr->dump_message;
    default:
        break;
    }   
}

void RelExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    if(type == ADD) {
        add_expr -> Dump(out, env);
        dump_message = add_expr ->dump_message;
    } else {
        opt_rel_expr -> Dump(out, env);
        add_expr -> Dump(out, env);
        dump_message = getANewTmpId();
        willOutputAInst(out);
        out<<dump_message<<" = ";
        switch (opt_op)
        {
        case LE:
            out<<"le";
            break;
        case GE:
            out<<"ge";
            break;
        case LT:
            out<<"lt";
            break;
        case GT:
            out<<"gt";
            break;
        
        default:
            break;
        }
        out<<" "<<opt_rel_expr->dump_message<<", "<<add_expr->dump_message<<std::endl;
    }
}

void EqExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    if(type == REL) {
        rel_expr -> Dump(out, env);
        dump_message = rel_expr ->dump_message;
    } else {
        opt_eq_expr -> Dump(out, env);
        rel_expr -> Dump(out, env);
        dump_message = getANewTmpId();
        willOutputAInst(out);
        out<<dump_message<<" = ";
        switch (opt_op)
        {
        case EQ:
            out<<"eq";
            break;
        case NE:
            out<<"ne";
            break;
        
        default:
            break;
        }
        out<<" "<<opt_eq_expr->dump_message<<", "<<rel_expr->dump_message<<std::endl;
    }
    
}

void LAndExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    if(type == EQ) {
        eq_expr -> Dump(out, env);
        dump_message = eq_expr ->dump_message;
    } else {
        opt_and_expr -> Dump(out, env);
        

        dump_message = getANewTmpId();
        auto ans = getANewTmpId();
        auto tmp1 = getANewTmpId();
        auto tmp2 = getANewTmpId();
        auto other_id = getANewTmpId();
        auto end_id = getANewTmpId();


        willOutputAInst(out);
        out<<tmp1<<" = ne 0, "<<opt_and_expr->dump_message<<std::endl;
        out<<ans<<" = alloc i32"<<std::endl;
        out<<"store "<<tmp1<<", "<<ans<<std::endl;

        willOutputAInst(out);
        out<<"br "<<tmp1<<", "<<other_id<<", "<<end_id<<std::endl;

        out<<other_id<<":"<<std::endl;
        cancelNewBasicBlock();


        eq_expr->Dump(out, env);
        willOutputAInst(out);
        out<<tmp2<<" = ne 0, "<<eq_expr->dump_message<<std::endl;
        out<<"store "<<tmp2<<", "<<ans<<std::endl;
        out<<"br 1, "<<end_id<<", "<<end_id<<std::endl;
        
        out<<end_id<<":"<<std::endl;
        cancelNewBasicBlock();

        out<<dump_message<<" = load "<<ans<<std::endl;
    }
}

void LOrExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    if(type == AND) {
        and_expr -> Dump(out, env);
        dump_message = and_expr ->dump_message;
    } else {
        opt_or_expr -> Dump(out, env);
        
        dump_message = getANewTmpId();
        auto ans = getANewTmpId();
        auto tmp1 = getANewTmpId();
        auto tmp2 = getANewTmpId();
        auto other_id = getANewTmpId();
        auto end_id = getANewTmpId();


        willOutputAInst(out);
        out<<tmp1<<" = ne 0, "<<opt_or_expr->dump_message<<std::endl;
        out<<ans<<" = alloc i32"<<std::endl;
        out<<"store "<<tmp1<<", "<<ans<<std::endl;

        willOutputAInst(out);
        out<<"br "<<tmp1<<", "<<end_id<<", "<<other_id<<std::endl;

        out<<other_id<<":"<<std::endl;
        cancelNewBasicBlock();


        and_expr->Dump(out, env);
        willOutputAInst(out);
        out<<tmp2<<" = ne 0, "<<and_expr->dump_message<<std::endl;
        out<<"store "<<tmp2<<", "<<ans<<std::endl;
        out<<"br 1, "<<end_id<<", "<<end_id<<std::endl;
        
        out<<end_id<<":"<<std::endl;
        cancelNewBasicBlock();

        out<<dump_message<<" = load "<<ans<<std::endl;
    }
}


#define isType(a, type) (dynamic_cast<type>(a)!=nullptr)
#define dc(a, type)(dynamic_cast<type>(a))

int evaluate(BaseAST* root, std::shared_ptr<Environment> env) {
    assert(root != nullptr);
    if(isType(root, ExprAST*)) {
        ExprAST* tmp = dc(root, ExprAST*);
        return evaluate((tmp->or_expr).get(), env);
    } else if(isType(root, LOrExprAST*)) {
        LOrExprAST* tmp = dc(root, LOrExprAST*);
        if(tmp->type==LOrExprAST::AND)
            return evaluate(tmp->and_expr.get(), env);
        return evaluate(tmp->opt_or_expr.get(), env) || evaluate(tmp->and_expr.get(), env);
    } else if(isType(root, LAndExprAST*)) {
        LAndExprAST* tmp = dc(root, LAndExprAST*);
        if(tmp->type == LAndExprAST::EQ)
            return evaluate(tmp->eq_expr.get(), env);
        return evaluate(tmp->opt_and_expr.get(), env) && evaluate(tmp->eq_expr.get(), env);
    } else if(isType(root, EqExprAST*)) {
        EqExprAST* tmp = dc(root, EqExprAST*);
        if(tmp->type==EqExprAST::REL)
            return evaluate(tmp->rel_expr.get(), env);
        if(tmp->opt_op == EqExprAST::EQ)
            return evaluate(tmp->opt_eq_expr.get(), env) == evaluate(tmp->rel_expr.get(), env);
        return evaluate(tmp->opt_eq_expr.get(), env) != evaluate(tmp->rel_expr.get(), env);
    } else if(isType(root, RelExprAST*)) {
        RelExprAST* tmp = dc(root, RelExprAST*);
        if(tmp->type == RelExprAST::ADD)
            return evaluate(tmp->add_expr.get(), env);
        int tmpl = evaluate(tmp->opt_rel_expr.get(), env);
        int tmpr = evaluate(tmp->add_expr.get(), env);
        if(tmp->opt_op == RelExprAST::LE)
            return tmpl <= tmpr;
        if(tmp->opt_op == RelExprAST::GE)
            return tmpl >= tmpr;
        if(tmp->opt_op == RelExprAST::LT)
            return tmpl < tmpr;
        if(tmp->opt_op == RelExprAST::GT)
            return tmpl > tmpr;
        assert(false);
    } else if(isType(root, AddExprAST*)) {
        AddExprAST* tmp = dc(root, AddExprAST*);
        if(tmp->type == AddExprAST::MUL)
            return evaluate(tmp->mul_expr.get(), env);
        int tmpl = evaluate(tmp->opt_add_expr.get(), env);
        int tmpr = evaluate(tmp->mul_expr.get(), env);
        if(tmp->opt_op == AddExprAST::ADD)
            return tmpl+tmpr;
        return tmpl-tmpr;
    } else if(isType(root, MulExprAST*)) {
        MulExprAST* tmp = dc(root, MulExprAST*);
        if(tmp->type == MulExprAST::UNARY)
            return evaluate(tmp->unary_expr.get(), env);
        int tmpl = evaluate(tmp->opt_mul_expr.get(), env);
        int tmpr = evaluate(tmp->unary_expr.get(), env);
        if(tmp->opt_op == MulExprAST::MUL)
            return tmpl*tmpr;
        else if(tmp->opt_op == MulExprAST::DIV)
            return tmpl/tmpr;
        else
            return tmpl%tmpr;
    } else if(isType(root, UnaryExprAST*)) {
        UnaryExprAST* tmptmp = dc(root, UnaryExprAST*);
        if(tmptmp->type == UnaryExprAST::UNARY) {
            UnaryExprAST::InnerUnaryAST* tmp2 = dc(tmptmp->content.get(), UnaryExprAST::InnerUnaryAST*);
            assert(tmp2!=nullptr);
            BaseAST* unary_op_tmp = tmp2->unary_op.get();
            BaseAST* unary_expr = tmp2->unary_expr.get();
            assert(isType(unary_op_tmp, UnaryOpAST*));
            UnaryOpAST* unary_op = dc(unary_op_tmp, UnaryOpAST*);
            int tmpval = evaluate(unary_expr, env);
            if(unary_op->op == UnaryOpAST::ADD)
                return tmpval;
            else if(unary_op->op == UnaryOpAST::DEC)
                return -tmpval;
            return !tmpval;
        } else if (tmptmp->type == UnaryExprAST::PRIMARY) {
             UnaryExprAST::InnerPrimaryAST* tmp2 = dc(tmptmp->content.get(), UnaryExprAST::InnerPrimaryAST*);
             assert(tmp2!=nullptr);
             return evaluate(tmp2->primary_expr.get(), env);
        }

        assert(false);

    } else if(isType(root, PrimaryExprAST*)) {
        PrimaryExprAST* tmp = dc(root, PrimaryExprAST*);
        return evaluate(tmp->content.get(), env);
    } else if(isType(root, NumberAST*)) {
        NumberAST* tmp = dc(root, NumberAST*);
        return tmp->val;
    } else if(isType(root, LValAST*)) {
        LValAST* tmp = dc(root, LValAST*);
        std::string ident = tmp->ident;
        std::string value = env->getIdForIdent(ident);
        assert(value!="");
        assert(isdigit(value[0]));
        return atoi(value.c_str());
    } else if(isType(root, ConstExprAST*)) {
        ConstExprAST* tmp = dc(root, ConstExprAST*);
        return evaluate(tmp->expr.get(), env);
    }
    else {
        assert(false);
    }
}

#undef isType
#undef dc