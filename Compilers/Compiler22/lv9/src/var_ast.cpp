#include "var_ast.hpp"

#include "expr_ast.hpp"

#include <cassert>

// #define DEBUG

#ifdef DEBUG
#define dbg_puts(a) puts(a)
#else
#define dbg_puts(a)
#endif

void DeclAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    content->Dump(out, env);
}

void ConstDeclAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    for(int i = const_defs->size();i>0;i--) {
        (*const_defs)[i-1]->Dump(out, env);
    }
}

static void printArrayType(std::ostream &out, std::shared_ptr<Environment> env, std::vector<std::unique_ptr<BaseAST> > * v) {
    assert(v!=nullptr);
    for(int i = v->size();i>0;i--)
        out<<"[";
    out<<"i32";
    for(int i = 0;i<v->size();i++) {
        out<<", ";
        out<<evaluate((*v)[i].get(), env);
        out<<"]";
    }
}

BaseAST* getFirstExprRecur(BaseAST* root, std::shared_ptr<Environment> env) {
    assert(root!=nullptr);
    {
        auto tmp = dynamic_cast<ExprAST*>(root);
        if(tmp!=nullptr)
            return root;
    }
    {
        auto tmp = dynamic_cast<ConstExprAST*>(root);
        if(tmp!=nullptr)
            return tmp->expr.get();
    }
    {
        auto tmp = dynamic_cast<InitValAST*>(root);
        if(tmp!=nullptr) {
            assert((tmp->opt_expr==nullptr)!=(tmp->opt_initvals==nullptr));
            if(tmp->opt_expr!=nullptr)
                return getFirstExprRecur(tmp->opt_expr.get(), env);
            assert(tmp->opt_initvals->size()>0);
            return getFirstExprRecur((*(tmp->opt_initvals))[tmp->opt_initvals->size()-1].get(), env);
        }
    }
    {
        auto tmp = dynamic_cast<ConstInitValAST*>(root);
        if(tmp!=nullptr) {
            assert((tmp->opt_const_expr==nullptr)!=(tmp->opt_const_initvals==nullptr));
            if(tmp->opt_const_expr!=nullptr)
                return getFirstExprRecur(tmp->opt_const_expr.get(), env);
            assert(tmp->opt_const_initvals->size()>0);
            return getFirstExprRecur((*(tmp->opt_const_initvals))[tmp->opt_const_initvals->size()-1].get(), env);
        }
    }

    assert(false);
    return nullptr;
}

std::string getArraySizesEmb(std::shared_ptr<Environment> env,std::vector<std::unique_ptr<BaseAST> > * v) {
    assert(v!=nullptr);
    std::string ans="";
    for(int i = v->size();i>0;i--) {
        ans = ans + std::to_string(evaluate((*v)[i-1].get(), env));
        ans = ans + ";";
    }
    return ans;
}


// warning this may be wrong
void parseArraySizesEmb(std::string emb, int &head, std::string &res) {
    assert(emb!="");
    int pos = emb.find(';');
    assert(pos!=emb.npos);
    std::string digit = emb.substr(0, pos);
    res = emb.substr(pos+1);
    head = atoi(digit.c_str());
}

void dumpConstZeroInit(std::ostream& out, std::string init_size) {
    if(init_size=="") {
        out<<"0";
    }
    int head;
    std::string next_init_size;
    parseArraySizesEmb(init_size, head, next_init_size);
    out<<"{ ";
    for(int i = 0;i<head;i++) {
        if(i>0)
            out<<", ";
        dumpConstZeroInit(out, next_init_size);
    }
    out<<" }";
}


void ConstDefAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    // assert(env->allocIdForIdent(ident));
    // dump_message = env->getIdForIdent(ident);
    // assert(dump_message!="");
    // const_initval -> Dump(out, env);
    // willOutputAInst(out);
    // out<<dump_message<<" = alloc i32"<<std::endl;
    // out<<"store "<<const_initval->dump_message<<", "<<dump_message<<std::endl;

    assert(env->ident_to_koopa.find(ident) == env->ident_to_koopa.end());
    if(opt_array_exprs==nullptr) {
        auto tmp = dynamic_cast<ConstInitValAST*>(const_initval.get());
        assert(tmp->opt_const_expr!=nullptr);
        tmp->opt_const_expr->Dump(out, env);
        dump_message = tmp->opt_const_expr->dump_message;
        env->ident_to_koopa[ident] = dump_message;
    } else {
        assert(env->allocIdForIdent(ident));
        dump_message = env->getIdForIdent(ident);
        env->setTypeOfIdent(dump_message, "shuzu");
        env->setArraysLen(dump_message, opt_array_exprs->size());
        willOutputAInst(out);
        if(env->isRoot()) {
            out<<"global ";
            out<<dump_message<<" = alloc";
            printArrayType(out, env, opt_array_exprs.get());
            out<<", ";
            env->forceSetDevInfo(Environment::INIT_SIZE, getArraySizesEmb(env, opt_array_exprs.get()));
            const_initval->Dump(out, env);
            out<<std::endl;
        } else {
            out<<dump_message<<" = alloc";
            printArrayType(out, env, opt_array_exprs.get());
            out<<std::endl;
            out<<"store ";
            env->forceSetDevInfo(Environment::INIT_SIZE, getArraySizesEmb(env, opt_array_exprs.get()));
            const_initval->Dump(out, env);
            out<<", "<<dump_message<<std::endl;
        }
            
        
    }
}

// head-1 is frist value, head == 0 over
void formatGlobalConstInit(std::ostream &out, std::shared_ptr<Environment> env, std::string init_size,
    std::vector<std::unique_ptr<BaseAST> > * v, int & head) {
    if(init_size=="") {
        if(head>0) {
            BaseAST* expr = getFirstExprRecur((*v)[head-1].get(), env);
            out<<evaluate(expr, env);
            head--;
        }
        else out<<0;
        return;
    }
    out<<"{";
    int cur_size;
    std::string next_init_size;
    parseArraySizesEmb(init_size, cur_size, next_init_size);

    for(int i = 0;i<cur_size;i++) {
        if(i>0)
            out<<",";
        if(head == 0) {
            formatGlobalConstInit(out, env, next_init_size, v, head);
            continue;
        }
        auto tmp = dynamic_cast<ConstInitValAST*>((*v)[head-1].get());
        assert(tmp!=nullptr);
        if(tmp->opt_const_initvals==nullptr) {
            formatGlobalConstInit(out, env, next_init_size, v, head);
            continue;
        }
        int new_head = tmp->opt_const_initvals->size();
        formatGlobalConstInit(out, env, next_init_size, tmp->opt_const_initvals.get(), new_head);
        head--;
    }
    out<<"}";
}

void formatGlobalVarInit(std::ostream &out, std::shared_ptr<Environment> env, std::string init_size,
    std::vector<std::unique_ptr<BaseAST> > * v, int & head) {
    if(init_size=="") {
        if(head>0) {
            BaseAST* expr = getFirstExprRecur((*v)[head-1].get(), env);
            out<<evaluate(expr, env);
            head--;
        }
        else out<<0;
        return;
    }
    out<<"{";
    int cur_size;
    std::string next_init_size;
    parseArraySizesEmb(init_size, cur_size, next_init_size);
    for(int i = 0;i<cur_size;i++) {
        if(i>0)
            out<<",";
        if(head == 0) {
            formatGlobalVarInit(out, env, next_init_size, v, head);
            continue;
        }
        auto tmp = dynamic_cast<InitValAST*>((*v)[head-1].get());
        assert(tmp!=nullptr);
        if(tmp->opt_initvals==nullptr) {
            formatGlobalVarInit(out, env, next_init_size, v, head);
            continue;
        }
        int new_head = tmp->opt_initvals->size();
        formatGlobalVarInit(out, env, next_init_size, tmp->opt_initvals.get(), new_head);
        head--;
    }
    out<<"}";
}



void ConstInitValAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    assert((opt_const_expr==nullptr)!=(opt_const_initvals==nullptr));
    if(opt_const_expr!=nullptr) {
        opt_const_expr->Dump(out, env);
        dump_message = opt_const_expr->dump_message;
        out<<dump_message;
    } else {
        std::string cur_init_size = env->getDevInfo(Environment::INIT_SIZE);
        int head = opt_const_initvals->size();
        formatGlobalConstInit(out, env, cur_init_size, opt_const_initvals.get(), head);
    }
    
}

void LValAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    
    std::string ident_id = env->getIdForIdent(ident);
    // may be wrong
    assert(ident_id!="");

    if(opt_array_exprs==nullptr) {
        dump_message = ident_id;
        return;
    }

    std::string type = env->getTypeOfIdent(ident_id);
    assert(type=="shuzu"||type=="zhizhen");
    bool isZhizhen = (type=="zhizhen");

    auto last_ptr = ident_id;
    for(int i = opt_array_exprs->size();i>0;i--) {
        (*opt_array_exprs)[i-1]->Dump(out, env);
        willOutputAInst(out);
        auto ptr = getANewTmpId();
        if(i==opt_array_exprs->size() && isZhizhen)
            out<<ptr <<" = getptr "<<last_ptr<<", "<<(*opt_array_exprs)[i-1]->dump_message<<std::endl;
        else
            out<<ptr <<" = getelemptr "<<last_ptr<<", "<<(*opt_array_exprs)[i-1]->dump_message<<std::endl;
        last_ptr = ptr;
    }
    dump_message=last_ptr;
}

void ConstExprAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    // expr->Dump(out, env);
    // dump_message = expr->dump_message;
    int ans = evaluate(expr.get(), env);
    dump_message = std::to_string(ans);
}


void VarDeclAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    for (int i = var_defs->size();i>0;i--) {
        (*var_defs)[i-1]->Dump(out, env);
    }
}

void VarDefAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    assert(env->allocIdForIdent(ident));
    dump_message = env->getIdForIdent(ident);

    if(!env->isRoot()) {
        // 局部变量

        if(opt_array_exprs==nullptr) {
            willOutputAInst(out);
            out<<dump_message<<" = alloc i32"<<std::endl;
            if(type==INIT) {
                willOutputAInst(out);
                env->forceSetDevInfo(Environment::CUR_ARRAY_PTR, dump_message);
                opt_initval->Dump(out, env);
            } else {
                // willOutputAInst(out);
                // out << "store "<<0<<", "<<dump_message<<std::endl;
            }
        } else {
            env->setArraysLen(dump_message, opt_array_exprs->size());
            env->setTypeOfIdent(dump_message, "shuzu");
            willOutputAInst(out);
            out<<dump_message<<" = alloc ";
            printArrayType(out, env, opt_array_exprs.get());
            out<<std::endl;
            if(type == INIT) {
                env->forceSetDevInfo(Environment::INIT_SIZE, getArraySizesEmb(env, opt_array_exprs.get()));
                env->forceSetDevInfo(Environment::CUR_ARRAY_PTR, dump_message);
                opt_initval->Dump(out, env);
            }
        }
        
    } else {
        // 全局变量

        if(opt_array_exprs == nullptr) {
            out<<"global "<<dump_message<<" = alloc i32, ";
            if(type==INIT) {
                InitValAST* tmp = dynamic_cast<InitValAST*>(opt_initval.get());
                assert(tmp!=nullptr);
                int val = evaluate(tmp->opt_expr.get(), env);
                out<<val;
            } else {
                out<<"zeroinit";
            }
            out<<std::endl;
        }
        else {
            env->setArraysLen(dump_message, opt_array_exprs->size());
            env->setTypeOfIdent(dump_message, "shuzu");
            willOutputAInst(out);
            out<<"global "<<dump_message<<" = alloc ";
            printArrayType(out, env, opt_array_exprs.get());
            if(type == INIT) {
                out<<", ";
                env->forceSetDevInfo(Environment::INIT_SIZE, getArraySizesEmb(env, opt_array_exprs.get()));
                opt_initval->Dump(out, env);
            } else {
                out<<", ";
                out<<"zeroinit";
            }
            out<<std::endl;            
        }
        
    }
}

void formatLocalVarInit(std::ostream &out, std::shared_ptr<Environment> env, std::string init_size,
    std::vector<std::unique_ptr<BaseAST> > * v, int & head, std::string cur_ptr) {
    if(init_size=="") {
        if(head>0) {
            BaseAST* expr = getFirstExprRecur((*v)[head-1].get(), env);
            
            expr->Dump(out, env);
            head--;
            willOutputAInst(out);
            out<<"store "<<expr->dump_message<<", "<<cur_ptr<<std::endl;
        } else {
            willOutputAInst(out);
            out<<"store 0, "<<cur_ptr<<std::endl;
        }
        return;
    }
    int cur_size;
    std::string next_init_size;
    parseArraySizesEmb(init_size, cur_size, next_init_size);

    for(int i = 0;i<cur_size;i++) {
        auto next_ptr = getANewTmpId();
        willOutputAInst(out);
        out<<next_ptr<<" = getelemptr "<<cur_ptr<<", "<<i<<std::endl;

        if(head == 0) {
            formatLocalVarInit(out, env, next_init_size, v, head, next_ptr);
            continue;
        }

        auto tmp = dynamic_cast<InitValAST*>((*v)[head-1].get());
        assert(tmp!=nullptr);
        if(tmp->opt_initvals==nullptr) {
            formatLocalVarInit(out, env, next_init_size, v, head, next_ptr);
            continue;
        }
        int new_head = tmp->opt_initvals->size();
        formatLocalVarInit(out, env, next_init_size, tmp->opt_initvals.get(), new_head, next_ptr);
        head--;
    }
}

void InitValAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {

    assert((opt_expr==nullptr)!=(opt_initvals==nullptr));

    if(env->isRoot()) {

        if(opt_expr!=nullptr) {
            out<<evaluate(opt_expr.get(), env);
        } else {
            std::string cur_init_size = env->getDevInfo(Environment::INIT_SIZE);
            int head = opt_initvals->size();
            formatGlobalVarInit(out, env, cur_init_size, opt_initvals.get(), head);
            
        }
    } else {
        std::string cur_ptr = env->getDevInfo(Environment::CUR_ARRAY_PTR);
        if(opt_expr!=nullptr) {
            opt_expr->Dump(out, env);
            willOutputAInst(out);
            out<<"store "<<opt_expr->dump_message<<", "<<cur_ptr<<std::endl;
        } else {
            std::string cur_init_size = env->getDevInfo(Environment::INIT_SIZE);
            int head = opt_initvals->size();
            
            formatLocalVarInit(out, env, cur_init_size, opt_initvals.get(), head, cur_ptr);
        }
    }
}