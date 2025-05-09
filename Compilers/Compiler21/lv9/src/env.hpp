#pragma once
#include <iostream>
#include <memory>
#include <cstring>
#include <string>
#include <unordered_map>

enum SymType
{
    SYM_CONST,
    SYM_VARIABLE,
    SYM_FUNC_VOID,
    SYM_FUNC_INT,
    SYM_ARRAY_CONST,
    SYM_ARRAY_VAR,
    SYM_PTR, 
};

class SymValue{
    public:
        SymType type;
        int value;
        SymValue(SymType t, int v);
};


class Env{
    private:
        std::unordered_map<std::string, std::unique_ptr<SymValue> > table;
    protected:
        std::unique_ptr<Env> prev;
    public:
        Env(std::unique_ptr<Env> p);
        void put(std::string name, std::unique_ptr<SymValue> pvalue);
        std::unique_ptr<SymValue> get(std::string name);
};