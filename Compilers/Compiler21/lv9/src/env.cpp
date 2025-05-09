#include "env.hpp"

SymValue::SymValue(SymType t, int v){
    type = t;
    value = v;
}

Env::Env(std::unique_ptr<Env> p){
    prev = std::move(p);
}

void Env::put(std::string name, std::unique_ptr<SymValue> pvalue){
    table.insert(std::make_pair(name, std::move(pvalue)));
}

std::unique_ptr<SymValue> Env::get(std::string name){
    auto iter = table.find(name);
    if(iter != table.end()){
        return std::move(iter->second);
    }
    else
        return nullptr;
}