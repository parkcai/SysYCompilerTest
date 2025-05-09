#include "base_ast.hpp"
#include "expr_ast.hpp"

#include <cassert>


std::map<std::string, std::string> ident_to_koopa;

unsigned BaseAST::tmp_sym_cnt = 0;
static bool flag = false;
void registerNewBasicBlock() {
    flag = true;
}

void cancelNewBasicBlock() {
    flag = false;
}

void willOutputAInst(std::ostream &out) {
    if (flag) {
        flag = false;
        out<<getANewTmpId()<<":"<<std::endl;
    }
}

std::string getANewTmpId() {
    return "%"+std::to_string(BaseAST::tmp_sym_cnt++);
}


std::string Environment::WHILE_END_ID="while_end_id", Environment::WHILE_DEC_ID="while_dec_id",
    Environment::INIT_SIZE = "init_size", Environment::CUR_ARRAY_PTR = "cur_array_ptr";

bool Environment::isRoot() {
    return parent == nullptr;
}

std::string Environment::getIdForIdent(std::string ident) {
    if(ident_to_koopa.find(ident)!=ident_to_koopa.end()) {
        return ident_to_koopa[ident];
    }
    if(isRoot()) return "";
    return parent -> getIdForIdent(ident);
}

std::string Environment::getIdOfFunction(std::string key) {
    auto env = this;
    while(!env->isRoot()) {
        assert(env->parent != nullptr);
        env = (env->parent).get();
    }
    return env->getIdForIdent(key);
}

bool Environment::allocIdForIdent(std::string ident) {
    if(ident_to_koopa.find(ident)!=ident_to_koopa.end()) {
        return false;
    }
    ident_to_koopa[ident] = getANewTmpId();
    return true;
}

std::string Environment::getDevInfo(std::string key) {
    if(dev_info.find(key)!=dev_info.end())
        return dev_info[key];
    
    if(isRoot()) return "";
    return parent->getDevInfo(key);
}

std::string Environment::getTypeOfFunction(std::string id) {
    if(type_of_function.find(id)!=type_of_function.end())
        return type_of_function[id];
    
    if(isRoot()) return "";
    return parent->getTypeOfFunction(id);
}

// return false if key already exist in current env
bool Environment::trySetDevInfo(std::string key, std::string value) {
    if(dev_info.find(key)!=dev_info.end())
        return false;
    dev_info[key]=value;
    return true;
}

void Environment::forceSetDevInfo(std::string key, std::string value){
    dev_info[key]=value;
}

void Environment::forceSetTypeOfFunction(std::string id, std::string value){
    type_of_function[id]=value;
}

void Environment::setTypeOfIdent(std::string id, std::string type) {
    assert(type=="shuzu" || type == "zhizhen");
    type_of_ident[id]=type;
}

std::string Environment::getTypeOfIdent(std::string id) {
    if(type_of_ident.find(id)!=type_of_ident.end()) {
        return type_of_ident[id];
    }
    if(isRoot()) return "i32";
    return parent->getTypeOfIdent(id);
}

void Environment::setArraysLen(std::string id, int len) {
    arrayslen[id]=len;
}

int Environment::getArraysLen(std::string id) {
    if(arrayslen.find(id)!=arrayslen.end()) {
        return arrayslen[id];
    }
    if(isRoot()) assert(false);
    return parent->getArraysLen(id);
}


void CompUnitAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    env->ident_to_koopa["main"]="@main";

    // built in functions
    // decl @getint(): i32
    // decl @getch(): i32
    // decl @getarray(*i32): i32
    // decl @putint(i32)
    // decl @putch(i32)
    // decl @putarray(i32, *i32)
    // decl @starttime()
    // decl @stoptime()

    out<<"decl @getint(): i32"<<std::endl;
    out<<"decl @getch(): i32"<<std::endl;
    out<<"decl @getarray(*i32): i32"<<std::endl;
    out<<"decl @putint(i32)"<<std::endl;
    out<<"decl @putch(i32)"<<std::endl;
    out<<"decl @putarray(i32, *i32)"<<std::endl;
    out<<"decl @starttime()"<<std::endl;
    out<<"decl @stoptime()"<<std::endl;
    

    env->ident_to_koopa["getint"]= "@getint";
    env->ident_to_koopa["getch"]= "@getch";
    env->ident_to_koopa["getarray"]= "@getarray";
    env->ident_to_koopa["putint"]= "@putint";
    env->ident_to_koopa["putch"]= "@putch";
    env->ident_to_koopa["putarray"]= "@putarray";
    env->ident_to_koopa["starttime"]= "@starttime";
    env->ident_to_koopa["stoptime"]= "@stoptime";

    env->forceSetTypeOfFunction("@getint", "int");
    env->forceSetTypeOfFunction("@getch", "int");
    env->forceSetTypeOfFunction("@getarray", "int");
    env->forceSetTypeOfFunction("@putint", "void");
    env->forceSetTypeOfFunction("@putch", "void");
    env->forceSetTypeOfFunction("@putarray", "void");
    env->forceSetTypeOfFunction("@starttime", "void");
    env->forceSetTypeOfFunction("@stoptime", "void");

    for(int i = global_units->size();i>0;i--) {
        (*global_units)[i-1]->Dump(out, env);
    }
}

void GlobalUnitAST::Dump(std::ostream &out, std::shared_ptr<Environment> env){
    content->Dump(out, env);
}


void FuncDefAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)  {
    
    if (*ident == "main") {
        
    } else {
        assert(env->allocIdForIdent(*ident));
    }
    
    

    auto new_env = std::make_shared<Environment>();
    new_env -> parent = env;


    out<<"fun "<<env->getIdForIdent(*ident)<<"(";
    

    func_params->Dump(out, new_env);
    out<<")";


    func_type->Dump(out, new_env);

    env->forceSetTypeOfFunction(env->getIdForIdent(*ident), func_type->dump_message);


    out<<" {"<<std::endl;
    registerNewBasicBlock();
    
    func_params->Dump(out, new_env);

    block->Dump(out, new_env);
    willOutputAInst(out);
    out<<"ret"<<(func_type->dump_message=="int"?" 0":"")<<std::endl;
    cancelNewBasicBlock();
    out<<"}"<<std::endl;

}

void FuncTypeAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)  {
    if (*val == "int") out<<": i32";
    dump_message = *val;
}

void FuncDefManyParamsAST:: Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    assert(dump_times<=1);
    if(func_def_params==nullptr) return;
    if(dump_times==0) {
        dump_times++;
        for(int i = func_def_params->size();i>0;i--) {
            if(i<func_def_params->size()) {
                out<<", ";
            }
            (*func_def_params)[i-1]->Dump(out, env);
        }
    } else{
        for(int i = func_def_params->size();i>0;i--)
            (*func_def_params)[i-1]->Dump(out, env);
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

void FuncDefOneParamAST:: Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    assert(dump_times<=1);
    assert(btype == "int");
    if(dump_times==0) {
        dump_times++;
        dev_first_name = getANewTmpId();
        if(opt_array_exprs==nullptr)
            out<<dev_first_name<<": i32";
        else {
            out<<dev_first_name<<": *";
            printArrayType(out, env, opt_array_exprs.get());
        }
    } else {
        dump_times++;
        if(opt_array_exprs==nullptr) {
            assert(env->allocIdForIdent(ident));
            willOutputAInst(out);
            out<<env->getIdForIdent(ident)<<" = alloc i32"<<std::endl;
            out<<"store "<<dev_first_name<<", "<<env->getIdForIdent(ident)<<std::endl;            
        } else {
            assert(env->allocIdForIdent(ident));
            auto ident_id = env->getIdForIdent(ident);
            willOutputAInst(out);
            out<<ident_id<<" = getptr "<<dev_first_name<<", 0"<<std::endl;
            env->setArraysLen(ident_id, opt_array_exprs->size()+1);
            env->setTypeOfIdent(ident_id, "zhizhen");
        }

    }
}

void FuncCallAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    if(func_call_params!=nullptr) {
        for (int i = func_call_params->size();i>0;i--) {
            (*func_call_params)[i-1]->Dump(out, env);
        }
    }
    auto func_id = env->getIdOfFunction(ident);
    auto func_type = env->getTypeOfFunction(func_id);
    assert(func_type=="int"||func_type=="void");
    willOutputAInst(out);
    if(func_type == "int") {
        
        dump_message = getANewTmpId();
        out<<dump_message<<" = ";
    }
    out<<"call "<<func_id<<"(";
    if(func_call_params!=nullptr) {
        for (int i = func_call_params->size();i>0;i--) {
            if(i<func_call_params->size())
                out<<", ";
            out<<(*func_call_params)[i-1]->dump_message;
        }
    }
    out<<")"<<std::endl;

}

void FuncCallOneParamAST::Dump(std::ostream &out, std::shared_ptr<Environment> env) {
    expr->Dump(out, env);
    dump_message = expr->dump_message;
}

void BlockAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)  {

    std::shared_ptr<Environment> new_env = std::make_shared<Environment>();
    new_env -> parent = env;

    for (int i = block_items->size();i>0;i--) {

        (*block_items)[i-1]->Dump(out, new_env);
    }
}


void StmtAST::Dump(std::ostream &out,std::shared_ptr<Environment> env) {
        
    if (type == RETURN) {
        if(content!=nullptr)
            content -> Dump(out, env);
        willOutputAInst(out);
        if(content!=nullptr)
            out<<"ret "<<content->dump_message<<std::endl;
        else
            out<<"ret"<<std::endl;
        registerNewBasicBlock();
    } else if(type==ASSIGN){
        content -> Dump(out, env);
        opt_lval -> Dump(out, env);
        willOutputAInst(out);
        out<<"store "<<content->dump_message<<", "<<opt_lval->dump_message<<std::endl;
    } else if(type==BLOCK) {
        content->Dump(out, env);
    } else if(type==EXPR) {
        content->Dump(out, env);
    } else if(type==IF) {
        content->Dump(out, env);
    } else if(type==WHILE) {
        content->Dump(out, env);
    } else if(type==BREAK) {
        auto while_end_id = env->getDevInfo(Environment::WHILE_END_ID);
        assert(while_end_id!="");
        willOutputAInst(out);
        out<<"br 1, "<<while_end_id<<", "<<while_end_id<<std::endl;
        registerNewBasicBlock();
    } else if(type==CONTINUE) {
        auto while_dec_id = env->getDevInfo(Environment::WHILE_DEC_ID);
        assert(while_dec_id!="");
        willOutputAInst(out);
        out<<"br 1, "<<while_dec_id<<", "<<while_dec_id<<std::endl;
        registerNewBasicBlock();
    }
}

void BlockItemAST::Dump(std::ostream &out, std::shared_ptr<Environment> env)
{
    
    content->Dump(out, env);
}
