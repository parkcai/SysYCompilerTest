#pragma once
#include<iostream>
#include <memory>
#include <map>
#include <string>
#include <unordered_map>
#include <optional>
#include <cassert>
#include "koopa_use.hpp"
#include "symbol_list.hpp"
#include "block_maintainer.hpp"
#include "loop_maintainer.hpp"

using namespace std;

enum InstType
{
    ConstDecl,
    Decl,
    ArrayDecl,
    ConstArrayDecl,
    Stmt,
    Branch,
    While,
    Break,
    Continue
};
char *new_char_arr(string str);

// BRS AST AYES
class BaseAST {
    public:
        bool is_const = false;
        static SymbolList symbol_list;
        static BlockMaintainer block_maintainer;
        static LoopMaintainer loop_maintainer;
         //构造一个临时变量，比较exp是否为0
        
        virtual ~BaseAST() = default;
        virtual void show()const{
        };
        virtual void *build_koopa_values() const
        {
            cerr << "Not Implement build_koopa_values" << endl;
            assert(false);
            return nullptr;
        }
        // 用于表达式AST求值
        virtual int CalcValue() const
        {
            cerr << "Not Implement build_koopa_values" << endl;
            assert(false);
            return 0;
        }
        virtual void *koopa_leftvalue() const
        {
            return nullptr;
        }

};

typedef vector<pair<InstType, unique_ptr<BaseAST>>> InstSet;

class NumberAST : public BaseAST {
    public:
        string number;
        int val;
        NumberAST(int _val) { val = _val; }
        void show()const override{
            cout<<"NumberAST { ";
            cout<<number;
            cout<<" } ";
        }
        void *build_koopa_values() const override
        {
            koopa_raw_value_data *res = new koopa_raw_value_data();
            res->ty = make_simple_type(KOOPA_RTT_INT32);
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_INTEGER;
            res->kind.data.integer.value = val;
            return res;
        }
        int CalcValue() const override
        {
            return val;
        }
};




class LValAST : public BaseAST
{
    enum ValType
    {
        Num,
        Array
    };
    public:
        ValType type;
        std::string name;
        std::vector<std::unique_ptr<BaseAST>> idx;
        LValAST(const char *_name) : name(_name) 
        {
            type = Num;
        }
        LValAST(const char *_name, std::vector<BaseAST*> &_idx) : name(_name)
        {
            type = Array;
            for(auto &i : _idx)
                idx.emplace_back(i);
        } 
        //返回左值
        void *koopa_leftvalue() const override
        {
           if(type == Array)
            {
                koopa_raw_value_data *get;
                koopa_raw_value_t src = (koopa_raw_value_t)symbol_list.GetSymbol(name).number;
                if(src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
                {
                    koopa_raw_value_t src = (koopa_raw_value_t)symbol_list.GetSymbol(name).number;
                    koopa_raw_value_data *load0 = new koopa_raw_value_data();
                    load0->ty = src->ty->data.pointer.base;
                    load0->name = nullptr;
                    load0->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                    load0->kind.tag = KOOPA_RVT_LOAD;
                    load0->kind.data.load.src = src;
                    block_maintainer.AddInst(load0);

                    bool first = true;
                    src = load0;
                    for(auto &i : idx)
                    {
                        get = new koopa_raw_value_data();
                        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                        if(first)
                        {
                            get->ty = src->ty;
                            get->name = nullptr;
                            get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                            get->kind.tag = KOOPA_RVT_GET_PTR;
                            get->kind.data.get_ptr.src = src;
                            get->kind.data.get_ptr.index = (koopa_raw_value_t)i->build_koopa_values();
                            first = false;
                        }
                        else
                        {
                            ty->tag = KOOPA_RTT_POINTER;
                            ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                            get->ty = ty;
                            get->name = nullptr;
                            get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                            get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                            get->kind.data.get_elem_ptr.src = src;
                            get->kind.data.get_elem_ptr.index = (koopa_raw_value_t)i->build_koopa_values();
                        }
                        block_maintainer.AddInst(get);
                        src = get;
                    }
                }
                else
                {
                    for(auto &i : idx)
                    {
                        get = new koopa_raw_value_data();
                        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                        ty->tag = KOOPA_RTT_POINTER;
                        ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                        get->ty = ty;
                        get->name = nullptr;
                        get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                        get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                        get->kind.data.get_elem_ptr.src = src;
                        get->kind.data.get_elem_ptr.index = (koopa_raw_value_t)i->build_koopa_values();
                        block_maintainer.AddInst(get);
                        src = get;
                    }
                }
                return get;
            }
            else if (type == Num)
            {
                return (void *)symbol_list.GetSymbol(name).number;
            }
            return nullptr;
        }
        void *build_koopa_values() const override
        {
            koopa_raw_value_data *res = new koopa_raw_value_data();
            auto var = symbol_list.GetSymbol(name);
            if (var.type == LValSymbol::Const)
                return (void *)var.number;
            else if (var.type == LValSymbol::Var)
            {
                res->ty = make_simple_type(KOOPA_RTT_INT32);
                res->name = nullptr;
                res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                res->kind.tag = KOOPA_RVT_LOAD;
                res->kind.data.load.src = (koopa_raw_value_t)var.number;
                block_maintainer.AddInst(res);
            }
            else if (var.type == LValSymbol::Array)
            {
                bool need_load = false;
                koopa_raw_value_data *get;
                koopa_raw_value_data *src = (koopa_raw_value_data*)var.number;
                if(idx.empty())
                {
                    get = new koopa_raw_value_data();
                    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                    ty->tag = KOOPA_RTT_POINTER;
                    ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                    get->ty = ty;
                    get->name = nullptr;
                    get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                    get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                    get->kind.data.get_elem_ptr.src = src;
                    get->kind.data.get_elem_ptr.index = make_koopa_interger(0);
                    block_maintainer.AddInst(get);
                }
                else
                {
                    for(auto &i : idx)
                    {
                        get = new koopa_raw_value_data();
                        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                        ty->tag = KOOPA_RTT_POINTER;
                        ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                        get->ty = ty;
                        get->name = nullptr;
                        get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                        get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                        get->kind.data.get_elem_ptr.src = src;
                        get->kind.data.get_elem_ptr.index = (koopa_raw_value_t)i->build_koopa_values();
                        block_maintainer.AddInst(get);
                        src = get;
                        if(ty->data.pointer.base->tag == KOOPA_RTT_INT32)
                            need_load = true;
                    }
                }
                if(need_load)
                {
                    res->ty = make_simple_type(KOOPA_RTT_INT32);
                    res->name = nullptr;
                    res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                    res->kind.tag = KOOPA_RVT_LOAD;
                    res->kind.data.load.src = get;
                    block_maintainer.AddInst(res);
                }
                else if(src->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
                {
                    res = new koopa_raw_value_data();
                    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                    ty->tag = KOOPA_RTT_POINTER;
                    ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                    res->ty = ty;
                    res->name = nullptr;
                    res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                    res->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                    res->kind.data.get_elem_ptr.src = src;
                    res->kind.data.get_elem_ptr.index = make_koopa_interger(0);
                    block_maintainer.AddInst(res);
                }
                else
                    res = src;
            }
            else if (var.type == LValSymbol::Pointer)
            {
                koopa_raw_value_data *src = (koopa_raw_value_data*)var.number;
                koopa_raw_value_data *load0 = new koopa_raw_value_data();
                load0->ty = src->ty->data.pointer.base;
                load0->name = nullptr;
                load0->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                load0->kind.tag = KOOPA_RVT_LOAD;
                load0->kind.data.load.src = src;
                block_maintainer.AddInst(load0);

                bool need_load = false;
                bool first = true;
                koopa_raw_value_data *get;
                src = load0;
                for(auto &i : idx)
                {
                    get = new koopa_raw_value_data();
                    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                    if(first)
                    {
                        get->ty = src->ty;
                        get->name = nullptr;
                        get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                        get->kind.tag = KOOPA_RVT_GET_PTR;
                        get->kind.data.get_ptr.src = src;
                        get->kind.data.get_ptr.index = (koopa_raw_value_t)i->build_koopa_values();
                        first = false;
                    }
                    else
                    {
                        ty->tag = KOOPA_RTT_POINTER;
                        ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                        get->ty = ty;
                        get->name = nullptr;
                        get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                        get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                        get->kind.data.get_elem_ptr.src = src;
                        get->kind.data.get_elem_ptr.index = (koopa_raw_value_t)i->build_koopa_values();
                    }
                    block_maintainer.AddInst(get);
                    src = get;
                    if(get->ty->data.pointer.base->tag == KOOPA_RTT_INT32)
                        need_load = true;
                }
                if(need_load)
                {
                    res->ty = make_simple_type(KOOPA_RTT_INT32);
                    res->name = nullptr;
                    res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                    res->kind.tag = KOOPA_RVT_LOAD;
                    res->kind.data.load.src = get;
                    block_maintainer.AddInst(res);
                }
                else if(src->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
                {
                    res = new koopa_raw_value_data();
                    koopa_raw_type_kind *ty = new koopa_raw_type_kind();
                    ty->tag = KOOPA_RTT_POINTER;
                    ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
                    res->ty = ty;
                    res->name = nullptr;
                    res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                    res->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
                    res->kind.data.get_elem_ptr.src = src;
                    res->kind.data.get_elem_ptr.index = make_koopa_interger(0);
                    block_maintainer.AddInst(res);
                }
                else
                    res = src;
            }
                return res;
        }

         int CalcValue() const override
        {
            auto var = symbol_list.GetSymbol(name);
            //assert(var.type == LValSymbol::Const);
            return ((koopa_raw_value_t)var.number)->kind.data.integer.value;
        }
};


class ConstDefAST : public BaseAST
{
public:
    std::string name;
    std::unique_ptr<BaseAST> exp;

    ConstDefAST(const char *_name, std::unique_ptr<BaseAST> &_exp)
        : name(_name)
    {
        exp = std::move(_exp);
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_simple_type(KOOPA_RTT_INT32);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_INTEGER;
        res->kind.data.integer.value = exp->CalcValue();
        //符号表
        symbol_list.AddSymbol(name, LValSymbol(LValSymbol::Const, res));
        return res;
    }
};


class VarDefAST : public BaseAST
{
public:
    std::string name;
    std::unique_ptr<BaseAST> exp;

    VarDefAST(const char *_name)
        : name(_name)
    {
        exp = nullptr;
    }

    VarDefAST(const char *_name, std::unique_ptr<BaseAST> &_exp)
        : name(_name)
    {
        exp = std::move(_exp);
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_int_pointer_type();
        res->name = new_char_arr("@" + name);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_ALLOC;
        block_maintainer.AddInst(res);
        //变量要加入符号表
        symbol_list.AddSymbol(name, LValSymbol(LValSymbol::Var, res));

        //has value，store it
        if (exp)
        {
            koopa_raw_value_data *store = new koopa_raw_value_data();
            store->ty = make_simple_type(KOOPA_RTT_UNIT);
            store->name = nullptr;
            store->used_by = empty_koopa_rs();
            store->kind.tag = KOOPA_RVT_STORE;
            store->kind.data.store.dest = res;
            store->kind.data.store.value = (koopa_raw_value_t)exp->build_koopa_values();
            block_maintainer.AddInst(store);
        }

        return res;
    }
};

class BlockAST : public BaseAST {
    public:
        InstSet insts;
        BlockAST(){}
        //收集这个block的所有指令
        BlockAST(InstSet &_insts)
        {
            for (auto &inst : _insts)
                insts.push_back(make_pair(inst.first, move(inst.second)));
        }
        void show()const override{
            cout<<"BlockAST { ";
            cout<<" } ";
        }
        static void add_InstSet(const InstSet &insts)
        {
            symbol_list.NewEnv();
            for (const auto &inst : insts)
                inst.second->build_koopa_values();
            symbol_list.DeleteEnv();
        }

        void build_koopa_values_no_env() const
        {
            for (const auto &inst: insts)
                //cout<<inst.first<<endl;
                inst.second->build_koopa_values();
        }

        void *build_koopa_values() const override
        {
            add_InstSet(insts);
            return nullptr;
        }
};

class FuncFParamAST : public BaseAST
{
public:
    enum ParamType
    {
        Int,
        Array
    } type;
    std::string name;
    int index;
    std::vector<std::unique_ptr<BaseAST>> sz_exp;

    FuncFParamAST(ParamType _type, const char *_name, int _index) : type(_type), name(_name), index(_index) {}
    FuncFParamAST(ParamType _type, const char *_name, int _index, std::vector<BaseAST*> &_sz_Exp) : type(_type), name(_name), index(_index)
    {
        for(auto e : _sz_Exp)
            sz_exp.emplace_back(e);
    }

    void *get_koopa_type() const
    {
        if(type == Array)
        {
            if(sz_exp.empty())
                return make_int_pointer_type();
            else
            {
                std::vector<int> sz;
                for(auto &e : sz_exp)
                    sz.push_back(e->CalcValue());
                koopa_raw_type_kind *ty = make_array_type(sz);
                koopa_raw_type_kind *tty = new koopa_raw_type_kind();
                tty->tag = KOOPA_RTT_POINTER;
                tty->data.pointer.base = ty;
                return tty;
            }
        }
        else if (type == Int)
            return make_simple_type(KOOPA_RTT_INT32);
        return nullptr;
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = (koopa_raw_type_kind*)get_koopa_type();
        res->name = new_char_arr("@" + name);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
        res->kind.data.func_arg_ref.index = index;
        return res;
    }
};

class FuncDefAST : public BaseAST {
    public:
        string func_type;
        string ident;
        unique_ptr<BlockAST> block;
        std::vector<std::unique_ptr<FuncFParamAST>> fparams;


        FuncDefAST(std::vector<BaseAST*> &_fparams){
            for(BaseAST* fp : _fparams)
                fparams.emplace_back(dynamic_cast<FuncFParamAST*>(fp));
        }
        FuncDefAST(){
            fparams.clear();
        }
        void show()const override{
            cout<<"FuncDefAST { ";
            cout<< func_type;
            cout<<", "<<ident<<", ";
            block->show();
            cout<<" } ";
        }
        void *build_koopa_values() const override
        {   
            koopa_raw_function_data_t *res = new koopa_raw_function_data_t();
            symbol_list.AddSymbol(ident, LValSymbol(LValSymbol::Function, res));
            
            koopa_raw_type_kind_t *ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            vector<const void*> par;
            //参数的类型
            for(auto &fp : fparams)
                par.push_back(fp->get_koopa_type());
            ty->data.function.params = make_koopa_rs_from_vector(par, KOOPA_RSIK_TYPE);
            
            if (func_type == "int"){
                ty->data.function.ret = (const struct koopa_raw_type_kind *) make_simple_type(KOOPA_RTT_INT32);
            }
            else if(func_type == "void")
                ty->data.function.ret = (const struct koopa_raw_type_kind *) make_simple_type(KOOPA_RTT_UNIT);
            else ty->data.function.ret = nullptr; // not implement
            res->ty = ty;
            res->name = new_char_arr("@" + ident);

            //函数参数的值
            par.clear();
            for(auto &fp : fparams)
                par.push_back(fp->build_koopa_values());
            res->params = make_koopa_rs_from_vector(par, KOOPA_RSIK_VALUE);

            vector<const void *> blocks;
            block_maintainer.SetBasicBlockBuf(&blocks);

            koopa_raw_basic_block_data_t *entry_block = new koopa_raw_basic_block_data_t();
            entry_block->name = new_char_arr("%entry_" + ident);
            entry_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            entry_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

            symbol_list.NewEnv();
            block_maintainer.AddNewBasicBlock(entry_block);
            block_maintainer.SetCurrentFunction(res);
            
            
             for(size_t i = 0; i < fparams.size(); i++)
            {
                auto &fp = fparams[i];
                koopa_raw_value_data *allo = AllocType("@" + fp->name, ((koopa_raw_value_t)par[i])->ty);
                if(allo->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
                    symbol_list.AddSymbol(fp->name, LValSymbol(LValSymbol::Pointer, allo));
                else
                    symbol_list.AddSymbol(fp->name, LValSymbol(LValSymbol::Var, allo));
                block_maintainer.AddInst(allo);
                koopa_raw_value_data *sto = new koopa_raw_value_data();
                sto->ty = make_simple_type(KOOPA_RTT_UNIT);
                sto->name = nullptr;
                sto->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                sto->kind.tag = KOOPA_RVT_STORE;
                sto->kind.data.store.dest = allo;
                sto->kind.data.store.value = (koopa_raw_value_t)par[i];
                block_maintainer.AddInst(sto);
            }


            block->build_koopa_values_no_env();
            symbol_list.DeleteEnv();
            block_maintainer.FinishCurrentBlock();

            res->bbs = make_koopa_rs_from_vector(blocks, KOOPA_RSIK_BASIC_BLOCK);
            return res;
            }
};


class InitValAST : public BaseAST
{
    enum ValType{
        Exp,
        Array
    };

    std::vector<koopa_raw_value_t> cache;

public:
    ValType type;
    std::unique_ptr<BaseAST> exp;
    std::vector<std::unique_ptr<InitValAST>> arr_list;

    InitValAST(std::unique_ptr<BaseAST> &_exp)
    {
        type = Exp;
        exp = std::move(_exp);
    }
    InitValAST(std::vector<BaseAST*> &_arr_list)
    {
        type = Array;
        for(auto t : _arr_list)
            arr_list.emplace_back(dynamic_cast<InitValAST*>(t));
    }
    //make int koopa 存到cache上
    void sub_preprocess(std::vector<int> &pro, int align_pos, std::vector<koopa_raw_value_t> &buf)
    {
        int target_size = buf.size() + pro[align_pos];
        for(size_t i = 0; i < arr_list.size(); i++)
        {
            auto &t = arr_list[i];
            if(t->type == Exp)
            {
                if(is_const)
                    buf.push_back(make_koopa_interger(t->exp->CalcValue()));
                else
                    buf.push_back((koopa_raw_value_t)t->exp->build_koopa_values());
            }
            else
            {
                int new_align_pos = align_pos + 1;
                while(cache.size() % pro[new_align_pos] != 0)
                    new_align_pos ++;
                arr_list[i]->sub_preprocess(pro, new_align_pos, buf);
            }
        }
        while(buf.size() < target_size)
            buf.push_back(make_koopa_interger(0));
    }

    void preprocess(const std::vector<int> &sz)
    {
        std::vector<int> pro(sz.size() + 1);
        pro[sz.size()] = 1;
        for(int i = sz.size() - 1; i >= 0; i--)
            pro[i] = pro[i + 1] * sz[i];
        sub_preprocess(pro, 0, cache);
    }
    //获取第idx初始化值的koopa IR
    koopa_raw_value_t At(int idx)
    {
        if(type == Array)
            return cache[idx];
        else if(type == Exp)
            return (koopa_raw_value_t)exp->build_koopa_values();
        return nullptr;
    }
    //根据int重新构造koopa
    koopa_raw_value_t sub_make_aggerate(const std::vector<int> &sz, std::vector<int> &pro, int align_pos, std::vector<koopa_raw_value_t> &buf, int st_pos)
    {
        if(pro[align_pos] == 1)
            return buf[st_pos];
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_array_type(sz, align_pos);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_AGGREGATE;

        std::vector<const void*> elems;
        for(int i = 0; i < sz[align_pos]; i++)
            elems.push_back(sub_make_aggerate(sz, pro, align_pos + 1, buf, st_pos + pro[align_pos + 1] * i));
        res->kind.data.aggregate.elems = make_koopa_rs_from_vector(elems, KOOPA_RSIK_VALUE);
        return res;
    }

    koopa_raw_value_t make_aggerate(const std::vector<int> &sz)
    {
        std::vector<int> pro(sz.size() + 1);
        pro[sz.size()] = 1;
        for(int i = sz.size() - 1; i >= 0; i--)
            pro[i] = pro[i + 1] * sz[i];
        return sub_make_aggerate(sz, pro, 0, cache, 0);
    }
};

class ArrayDefAST : public BaseAST
{
public:
    std::string name;
    std::vector<std::unique_ptr<BaseAST>> sz_exp;
    std::unique_ptr<InitValAST> init_val;

    ArrayDefAST(const char *_name, std::vector<BaseAST*> &_exp) : name(_name)
    {
        for(auto &e : _exp)
            sz_exp.emplace_back(e);
        init_val = nullptr;
    }
    ArrayDefAST(const char *_name, std::vector<BaseAST*> &_exp, std::unique_ptr<BaseAST> &_init_val) : name(_name)
    {
        for(auto &e : _exp)
            sz_exp.emplace_back(e);
        init_val = std::unique_ptr<InitValAST>(dynamic_cast<InitValAST*>(_init_val.release()));
    }

    koopa_raw_value_data *get_index(int i, std::vector<int> &pro, koopa_raw_value_data *src, int cur_pos = 0) const
    {
        if(cur_pos >= pro.size())
            return src;
        koopa_raw_value_data *get = new koopa_raw_value_data();
        koopa_raw_type_kind *ty = new koopa_raw_type_kind();
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;
        get->ty = ty;
        get->name = nullptr;
        get->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        get->kind.tag = KOOPA_RVT_GET_ELEM_PTR;
        get->kind.data.get_elem_ptr.src = src;
        get->kind.data.get_elem_ptr.index = make_koopa_interger(i / pro[cur_pos]);
        block_maintainer.AddInst(get);
        return get_index(i % pro[cur_pos], pro, get, cur_pos + 1);
    }

    void *build_koopa_values() const override
    {
        int total_size = 1;
        std::vector<int> sz;
        for(auto &e : sz_exp)
        {
            int tmp = e->CalcValue();
            total_size *= tmp;
            sz.push_back(tmp);
        }
        
        koopa_raw_value_data *res = new koopa_raw_value_data();
        koopa_raw_type_kind *ty = make_array_type(sz);
        koopa_raw_type_kind *tty = new koopa_raw_type_kind();
        tty->tag = KOOPA_RTT_POINTER;
        tty->data.pointer.base = ty;
        res->ty = tty;
        res->name = new_char_arr("@" + name);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_ALLOC;
        block_maintainer.AddInst(res);
        symbol_list.AddSymbol(name, LValSymbol(LValSymbol::Array, res));

        if(init_val)
        {
            init_val->preprocess(sz);
            std::vector<int> pro(sz.size());
            pro[sz.size() - 1] = 1;
            for(int i = sz.size() - 2; i >= 0; i--)
                pro[i] = pro[i + 1] * sz[i + 1];
            for(int i = 0; i < total_size; i++)
            {
                koopa_raw_value_data *get = get_index(i, pro, res);

                koopa_raw_value_data *st = new koopa_raw_value_data();
                st->ty = make_simple_type(KOOPA_RTT_UNIT);
                st->name = nullptr;
                st->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                st->kind.tag = KOOPA_RVT_STORE;
                st->kind.data.store.dest = get;
                st->kind.data.store.value = init_val->At(i);
                block_maintainer.AddInst(st);
            }
        }
        return res;
    }
};

class GlobalVarDefAST : public BaseAST
{
public:
    std::string name;
    std::unique_ptr<BaseAST> exp;

    GlobalVarDefAST(std::unique_ptr<BaseAST> &vardef_ast)
    {
        VarDefAST *var = dynamic_cast<VarDefAST*>(vardef_ast.release());
        name = var->name;
        exp = std::move(var->exp);
        delete var;
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_int_pointer_type();
        res->name = new_char_arr("@" + name);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        if(exp)
            res->kind.data.global_alloc.init = make_koopa_interger(exp->CalcValue());//(koopa_raw_value_data*)exp->build_koopa_values();
        else
            res->kind.data.global_alloc.init = ZeroInit();
        block_maintainer.AddInst(res);
        symbol_list.AddSymbol(name, LValSymbol(LValSymbol::Var, res));
        return res;
    }
};



class GlobalArrayDefAST : public BaseAST
{
public:
    std::string name;
    std::vector<std::unique_ptr<BaseAST>> sz_exp;
    std::unique_ptr<InitValAST> init_val;

    GlobalArrayDefAST(std::unique_ptr<BaseAST> &arraydef_ast)
    {
        ArrayDefAST *arraydef = dynamic_cast<ArrayDefAST*>(arraydef_ast.release());
        name = arraydef->name;
        sz_exp = std::move(arraydef->sz_exp);
        init_val = std::move(arraydef->init_val);
    }

    void *build_koopa_values() const override
    {
        std::vector<int> sz;
        for(auto &e : sz_exp)
            sz.push_back(e->CalcValue());
        koopa_raw_value_data *res = new koopa_raw_value_data();
        koopa_raw_type_kind *ty = make_array_type(sz);
        koopa_raw_type_kind *tty = new koopa_raw_type_kind();
        tty->tag = KOOPA_RTT_POINTER;
        tty->data.pointer.base = ty;
        res->ty = tty;
        res->name = new_char_arr("@" + name);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        if(init_val)
        {
            init_val->is_const = true;
            init_val->preprocess(sz);
            res->kind.data.global_alloc.init = init_val->make_aggerate(sz);
        }
        else
            res->kind.data.global_alloc.init = ZeroInit(ty);
        symbol_list.AddSymbol(name, LValSymbol(LValSymbol::Array, res));

        return res;
    }
};



class CompUnitAST : public BaseAST {
    public:
        vector<unique_ptr<BaseAST>> func_list;
        InstSet value_list;

        CompUnitAST(vector<BaseAST*> &_func_list, InstSet &_value_list){
            for(BaseAST* func : _func_list)
                func_list.push_back(unique_ptr<BaseAST>(func));
            cout<<_value_list.size()<<endl;
            for(auto &pa : _value_list)
            {
                if(pa.first == Decl)
                    value_list.push_back(make_pair(pa.first, std::unique_ptr<BaseAST>(new GlobalVarDefAST(pa.second))));
                else if(pa.first == ArrayDecl)
                    value_list.push_back(make_pair(pa.first, std::unique_ptr<BaseAST>(new GlobalArrayDefAST(pa.second))));
                else
                    value_list.push_back(make_pair(pa.first, std::move(pa.second)));
            }
        }
        void show()const override{
            cout<<"CompUnitAST { ";
            for (auto &func_def : func_list)
                func_def->show();
            cout<<" } ";
        }
        koopa_raw_program_t to_koopa_raw_program() const
        {
            symbol_list.NewEnv();

            vector<const void *> values;
            vector<const void *> funcs;
            
            //先把系统函数装进去
            koopa_raw_function_data_t *func = new koopa_raw_function_data_t();
            koopa_raw_type_kind_t *ty = new koopa_raw_type_kind_t();
            std::vector<const void *> fparams;

            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_INT32);
            func->ty = ty;
            func->name = "@getint";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("getint", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_INT32);
            func->ty = ty;
            func->name = "@getch";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("getch", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            fparams.clear();
            fparams.push_back(make_int_pointer_type());
            ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_INT32);
            func->ty = ty;
            func->name = "@getarray";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("getarray", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            fparams.clear();
            fparams.push_back(make_simple_type(KOOPA_RTT_INT32));
            ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_UNIT);
            func->ty = ty;
            func->name = "@putint";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("putint", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            fparams.clear();
            fparams.push_back(make_simple_type(KOOPA_RTT_INT32));
            ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_UNIT);
            func->ty = ty;
            func->name = "@putch";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("putch", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            fparams.clear();
            fparams.push_back(make_simple_type(KOOPA_RTT_INT32));
            fparams.push_back(make_int_pointer_type());
            ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_UNIT);
            func->ty = ty;
            func->name = "@putarray";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("putarray", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_UNIT);
            func->ty = ty;
            func->name = "@starttime";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("starttime", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            func = new koopa_raw_function_data_t();
            ty = new koopa_raw_type_kind_t();
            ty->tag = KOOPA_RTT_FUNCTION;
            ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
            ty->data.function.ret = make_simple_type(KOOPA_RTT_UNIT);
            func->ty = ty;
            func->name = "@stoptime";
            func->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
            func->bbs = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
            symbol_list.AddSymbol("stoptime", LValSymbol(LValSymbol::Function, func));
            funcs.push_back(func);

            for(auto &pa : value_list)
            {
                assert(pa.first == ConstDecl || pa.first == Decl || pa.first == ArrayDecl);
                if(pa.first == ConstDecl)
                    pa.second->build_koopa_values();
                else
                    values.push_back(pa.second->build_koopa_values());
            }

            for(auto &func_ast : func_list)
                funcs.push_back(func_ast->build_koopa_values());
            symbol_list.DeleteEnv();
            koopa_raw_program_t res;
            res.values = make_koopa_rs_from_vector(values, KOOPA_RSIK_VALUE);
            res.funcs = make_koopa_rs_from_vector(funcs, KOOPA_RSIK_FUNCTION);
            return res;
        }
};
class ReturnAST : public BaseAST {
    public:
        unique_ptr<BaseAST> ret_num;
        ReturnAST()
        {
            ret_num = nullptr;
        }
        ReturnAST(unique_ptr<BaseAST> &_ret_num)
        {
            ret_num = move(_ret_num);
        }
        void show() const override{
            cout<<"ReturnAST  ";
            if(ret_num){
                ret_num->show();
            }
        }
        void *build_koopa_values() const override
        {
            koopa_raw_value_data *res = new koopa_raw_value_data();
            res->ty = make_simple_type(KOOPA_RTT_UNIT);
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_RETURN;
            if(ret_num){
                cout<<endl;
                res->kind.data.ret.value = (const koopa_raw_value_data *)ret_num->build_koopa_values();
            }
            else{
                res->kind.data.ret.value = nullptr;
            }
            block_maintainer.AddInst(res);
            return res;
        }
};


class AssignmentAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    AssignmentAST(std::unique_ptr<BaseAST> &_lval, std::unique_ptr<BaseAST> &_exp)
    {
        lval = std::move(_lval);
        exp = std::move(_exp);
    }
    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_simple_type(KOOPA_RTT_UNIT);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_STORE;
        res->kind.data.store.value = (koopa_raw_value_t)exp->build_koopa_values();
        res->kind.data.store.dest = (koopa_raw_value_t)lval->koopa_leftvalue();
        block_maintainer.AddInst(res);
        return nullptr;
    }
};


class BranchAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    InstSet true_instset;
    InstSet false_instset;
    //初始化，复制指令集
    BranchAST(std::unique_ptr<BaseAST> &_exp, InstSet &_true_insts)
    {
        for (auto &inst : _true_insts)
            true_instset.push_back(std::make_pair(inst.first, std::move(inst.second)));
        exp = std::move(_exp);
    }
    BranchAST(std::unique_ptr<BaseAST> &_exp, InstSet &_true_insts, InstSet &_false_insts)
    {
        for (auto &inst : _true_insts)
            true_instset.push_back(std::make_pair(inst.first, std::move(inst.second)));
        for (auto &inst : _false_insts)
            false_instset.push_back(std::make_pair(inst.first, std::move(inst.second)));
        exp = std::move(_exp);
    }
    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_simple_type(KOOPA_RTT_UNIT);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_BRANCH;
        res->kind.data.branch.cond = (koopa_raw_value_t)exp->build_koopa_values();
        koopa_raw_basic_block_data_t *true_block = new koopa_raw_basic_block_data_t();
        koopa_raw_basic_block_data_t *false_block = new koopa_raw_basic_block_data_t();
        koopa_raw_basic_block_data_t *end_block = new koopa_raw_basic_block_data_t();
        res->kind.data.branch.true_bb = true_block;
        res->kind.data.branch.false_bb = false_block;
        res->kind.data.branch.true_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.data.branch.false_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddInst(res);

        true_block->name = new_char_arr("%true");
        true_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
        true_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddNewBasicBlock(true_block);
        BlockAST::add_InstSet(this->true_instset);
        block_maintainer.AddInst(JumpInst(end_block));
        
        false_block->name = new_char_arr("%false");
        false_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
        false_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddNewBasicBlock(false_block);
        std::vector<const void *> false_insts;
        BlockAST::add_InstSet(this->false_instset);
        block_maintainer.AddInst(JumpInst(end_block));

        end_block->name = new_char_arr("%end");
        end_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
        end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddNewBasicBlock(end_block);
        
        return res;
    }
};


class WhileAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    InstSet body_insts;
    WhileAST(std::unique_ptr<BaseAST> &_exp, InstSet &_body_insts)
    {
        for (auto &inst : _body_insts)
            body_insts.push_back(std::make_pair(inst.first, std::move(inst.second)));
        exp = std::move(_exp);
    }
    void *build_koopa_values() const override
    {
        koopa_raw_basic_block_data_t *while_entry = new koopa_raw_basic_block_data_t();
        koopa_raw_basic_block_data_t *while_body = new koopa_raw_basic_block_data_t();
        koopa_raw_basic_block_data_t *end_block = new koopa_raw_basic_block_data_t();
        loop_maintainer.AddLoop(while_entry, while_body, end_block);

        block_maintainer.AddInst(JumpInst(while_entry));

        while_entry->name = new_char_arr("%while_entry");
        while_entry->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
        while_entry->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddNewBasicBlock(while_entry);

        koopa_raw_value_data *br = new koopa_raw_value_data();
        br->ty = make_simple_type(KOOPA_RTT_UNIT);
        br->name = nullptr;
        br->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        br->kind.tag = KOOPA_RVT_BRANCH;
        br->kind.data.branch.cond = (koopa_raw_value_t)exp->build_koopa_values();
        br->kind.data.branch.true_bb = while_body;
        br->kind.data.branch.false_bb = end_block;
        br->kind.data.branch.true_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
        br->kind.data.branch.false_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddInst(br);

        while_body->name = new_char_arr("%while_body");
        while_body->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
        while_body->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddNewBasicBlock(while_body);
        BlockAST::add_InstSet(this->body_insts);
        block_maintainer.AddInst(JumpInst(while_entry));

        end_block->name = new_char_arr("%end");
        end_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
        end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        block_maintainer.AddNewBasicBlock(end_block);
        
        loop_maintainer.PopLoop();
        return nullptr;
    }
};

class BreakAST : public BaseAST
{
public:
    void *build_koopa_values() const override
    {
        block_maintainer.AddInst(JumpInst(loop_maintainer.GetLoop().end_block));
        return nullptr;
    }
};

class ContinueAST : public BaseAST
{
public:
    void *build_koopa_values() const override
    {
        block_maintainer.AddInst(JumpInst(loop_maintainer.GetLoop().while_entry));
        return nullptr;
    }
};

///










class PrimaryExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> nextExp; // Exp or Number
    PrimaryExpAST(std::unique_ptr<BaseAST> &_nextExp)
    {
        nextExp = std::move(_nextExp);
        cout<<"number"<<endl;
    }
    void *build_koopa_values() const override
    {
        return nextExp->build_koopa_values();
    }
    int CalcValue() const override
    {
        return nextExp->CalcValue();
    }
};

class UnaryExpAST : public BaseAST
{
public:
    enum
    {
        Primary,
        Op,
        Function
    } type;
    string op;
    unique_ptr<BaseAST> nextExp; // PrimaryExp or UnaryExp
    vector<BaseAST*> funcRParams;

    UnaryExpAST(unique_ptr<BaseAST> &_primary_exp)
    {
        type = Primary;
        nextExp = move(_primary_exp);
    }
    UnaryExpAST(const char *_op, unique_ptr<BaseAST> &_unary_exp)
    {
        type = Op;
        op = string(_op);
        nextExp = move(_unary_exp);
    }
    UnaryExpAST(const char *_ident, vector<BaseAST*> &rparams) : op(_ident), funcRParams(rparams)
    {
        type = Function;
    }

    void *build_koopa_values() const override
    {
        NumberAST zero(0);
        koopa_raw_value_data *res = nullptr;
        koopa_raw_function_data_t *func = nullptr;
        vector<const void *> rpa;
        switch (type)
        {
        case Primary:
            res = (koopa_raw_value_data *)nextExp->build_koopa_values();
            break;
        case Function:
            func = (koopa_raw_function_data_t *)symbol_list.GetSymbol(op).number;
            for(auto rp : funcRParams)
                rpa.push_back(rp->build_koopa_values());
            res = new koopa_raw_value_data();
            res->ty = func->ty->data.function.ret;
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_CALL;
            res->kind.data.call.callee = func;
            res->kind.data.call.args = make_koopa_rs_from_vector(rpa, KOOPA_RSIK_VALUE);
            block_maintainer.AddInst(res);
            break;

        case Op:
            if (op == "+")
            {
                res = (koopa_raw_value_data *)nextExp->build_koopa_values();
                break;
            }
            res = new koopa_raw_value_data();
            res->ty = make_simple_type(KOOPA_RTT_INT32);
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto &binary = res->kind.data.binary;
            if (op == "-")
                binary.op = KOOPA_RBO_SUB;
            else if (op == "!")
                binary.op = KOOPA_RBO_EQ;
            binary.lhs = (koopa_raw_value_t)zero.build_koopa_values();
            binary.rhs = (koopa_raw_value_t)nextExp->build_koopa_values();
            block_maintainer.AddInst(res);
            break;
        }
        return res;
    }
    int CalcValue() const override
    {
        if (type == Primary)
            return nextExp->CalcValue();
        int res = 0;
        if (op == "+")
            res = nextExp->CalcValue();
        else if (op == "-")
            res = -nextExp->CalcValue();
        else if (op == "!")
            res = !nextExp->CalcValue();
        return res;
    }
};



class MulExpAST : public BaseAST
{
public:
    enum
    {
        Primary,
        Op
    } type;
    std::string op;
    std::unique_ptr<BaseAST> leftExp; // may be primary
    std::unique_ptr<BaseAST> rightExp;

    MulExpAST(std::unique_ptr<BaseAST> &_primary_exp)
    {
        type = Primary;
        leftExp = std::move(_primary_exp);
    }
    MulExpAST(std::unique_ptr<BaseAST> &_left_exp, const char *_op, std::unique_ptr<BaseAST> &_right_exp)
    {
        type = Op;
        leftExp = std::move(_left_exp);
        op = std::string(_op);
        rightExp = std::move(_right_exp);
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = nullptr;
        switch (type)
        {
        case Primary:
            res = (koopa_raw_value_data *)leftExp->build_koopa_values();
            break;
        case Op:
            res = new koopa_raw_value_data();
            res->ty = make_simple_type(KOOPA_RTT_INT32);
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto &binary = res->kind.data.binary;
            if (op == "*")
                binary.op = KOOPA_RBO_MUL;
            else if (op == "/")
                binary.op = KOOPA_RBO_DIV;
            else if (op == "%")
                binary.op = KOOPA_RBO_MOD;
            binary.lhs = (koopa_raw_value_t)leftExp->build_koopa_values();
            binary.rhs = (koopa_raw_value_t)rightExp->build_koopa_values();
            block_maintainer.AddInst(res);
            break;
        }
        return res;
    }
    int CalcValue() const override
    {
        if (type == Primary)
            return leftExp->CalcValue();
        int res = 0;
        if (op == "*")
            res = leftExp->CalcValue() * rightExp->CalcValue();
        else if (op == "/")
            res = leftExp->CalcValue() / rightExp->CalcValue();
        else if (op == "%")
            res = leftExp->CalcValue() % rightExp->CalcValue();
        return res;
    }
};

class AddExpAST : public BaseAST
{
public:
    enum
    {
        Primary,
        Op
    } type;
    std::string op;
    std::unique_ptr<BaseAST> leftExp; // may be primary
    std::unique_ptr<BaseAST> rightExp;

    AddExpAST(std::unique_ptr<BaseAST> &_primary_exp)
    {
        type = Primary;
        leftExp = std::move(_primary_exp);
    }
    AddExpAST(std::unique_ptr<BaseAST> &_left_exp, const char *_op, std::unique_ptr<BaseAST> &_right_exp)
    {
        type = Op;
        leftExp = std::move(_left_exp);
        op = std::string(_op);
        rightExp = std::move(_right_exp);
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = nullptr;
        switch (type)
        {
        case Primary:
            res = (koopa_raw_value_data *)leftExp->build_koopa_values();
            break;
        case Op:
            res = new koopa_raw_value_data();
            res->ty = make_simple_type(KOOPA_RTT_INT32);
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto &binary = res->kind.data.binary;
            if (op == "+")
                binary.op = KOOPA_RBO_ADD;
            else if (op == "-")
                binary.op = KOOPA_RBO_SUB;
            binary.lhs = (koopa_raw_value_t)leftExp->build_koopa_values();
            binary.rhs = (koopa_raw_value_t)rightExp->build_koopa_values();
            block_maintainer.AddInst(res);
            break;
        }
        return res;
    }
    int CalcValue() const override
    {
        if (type == Primary)
            return leftExp->CalcValue();
        int res = 0;
        if (op == "+")
            res = leftExp->CalcValue() + rightExp->CalcValue();
        else if (op == "-")
            res = leftExp->CalcValue() - rightExp->CalcValue();
        return res;
    }
};




class RelExpAST : public BaseAST
{
public:
    enum
    {
        Primary,
        Op
    } type;
    std::string op;
    std::unique_ptr<BaseAST> leftExp; // may be primary
    std::unique_ptr<BaseAST> rightExp;

    RelExpAST(std::unique_ptr<BaseAST> &_primary_exp)
    {
        type = Primary;
        leftExp = std::move(_primary_exp);
    }
    RelExpAST(std::unique_ptr<BaseAST> &_left_exp, const char *_op, std::unique_ptr<BaseAST> &_right_exp)
    {
        type = Op;
        leftExp = std::move(_left_exp);
        op = std::string(_op);
        rightExp = std::move(_right_exp);
    }

    void *build_koopa_values() const override
    {
        koopa_raw_value_data *res = nullptr;
        switch (type)
        {
        case Primary:
            res = (koopa_raw_value_data *)leftExp->build_koopa_values();
            break;
        case Op:
            res = new koopa_raw_value_data();
            res->ty = make_simple_type(KOOPA_RTT_INT32);
            res->name = nullptr;
            res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto &binary = res->kind.data.binary;
            if (op == "<")
                binary.op = KOOPA_RBO_LT;
            else if (op == "<=")
                binary.op = KOOPA_RBO_LE;
            else if (op == ">")
                binary.op = KOOPA_RBO_GT;
            else if (op == ">=")
                binary.op = KOOPA_RBO_GE;
            binary.lhs = (koopa_raw_value_t)leftExp->build_koopa_values();
            binary.rhs = (koopa_raw_value_t)rightExp->build_koopa_values();
            block_maintainer.AddInst(res);
            break;
        }
        return res;
    }
    int CalcValue() const override
    {
        if (type == Primary)
            return leftExp->CalcValue();
        int res = 0;
        if (op == "<")
            res = leftExp->CalcValue() < rightExp->CalcValue();
        else if (op == "<=")
            res = leftExp->CalcValue() <= rightExp->CalcValue();
        else if (op == ">")
            res = leftExp->CalcValue() > rightExp->CalcValue();
        else if (op == ">=")
            res = leftExp->CalcValue() >= rightExp->CalcValue();
        return res;
    }
};



class EqExpAST : public BaseAST
{
    public:
        enum
        {
            Primary,
            Op
        } type;
        string op;
        unique_ptr<BaseAST> leftExp; 
        unique_ptr<BaseAST> rightExp;
        
        EqExpAST(unique_ptr<BaseAST> &_primary_exp)
        {
            type = Primary;
            leftExp = move(_primary_exp);
        }
        EqExpAST(unique_ptr<BaseAST> &_left_exp, const char *_op, unique_ptr<BaseAST> &_right_exp)
        {
            type = Op;
            leftExp = move(_left_exp);
            op = string(_op);
            rightExp = move(_right_exp);
        }

        void *build_koopa_values() const override
        {
            koopa_raw_value_data *res = nullptr;
            switch (type){

            case Primary:
                res = (koopa_raw_value_data *)leftExp->build_koopa_values();
                break;
            case Op:
                res = new koopa_raw_value_data();
                res->ty = make_simple_type(KOOPA_RTT_INT32);
                res->name = nullptr;
                res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                res->kind.tag = KOOPA_RVT_BINARY;
                auto &binary = res->kind.data.binary;
                if (op == "==")
                    binary.op = KOOPA_RBO_EQ;
                else if (op == "!=")
                    binary.op = KOOPA_RBO_NOT_EQ;
                binary.lhs = (koopa_raw_value_t)leftExp->build_koopa_values();
                binary.rhs = (koopa_raw_value_t)rightExp->build_koopa_values();
                block_maintainer.AddInst(res);
                break;
            }
            return res;
        }
        int CalcValue() const override
        {
            if (type == Primary)
                return leftExp->CalcValue();
            int res = 0;
            if (op == "==")
                res = leftExp->CalcValue() == rightExp->CalcValue();
            else if (op == "!=")
                res = leftExp->CalcValue() != rightExp->CalcValue();
            return res;
        }
};





class LAndExpAST : public BaseAST{
    koopa_raw_value_data *build_not_eq_koopa(koopa_raw_value_t exp) const
    {
        NumberAST zero(0);
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_simple_type(KOOPA_RTT_INT32);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_BINARY;
        auto &binary = res->kind.data.binary;
        binary.op = KOOPA_RBO_NOT_EQ;
        binary.lhs = exp;
        binary.rhs = (koopa_raw_value_t)zero.build_koopa_values();
        block_maintainer.AddInst(res);
        return res;
    }
    public:
        enum
        {
            Primary,
            Op
        } type;
        string op;
        unique_ptr<BaseAST> leftExp; // may be primary
        unique_ptr<BaseAST> rightExp;

        LAndExpAST(unique_ptr<BaseAST> &_primary_exp)
        {
            type = Primary;
            leftExp = move(_primary_exp);
        }
        LAndExpAST(unique_ptr<BaseAST> &_left_exp, const char *_op, unique_ptr<BaseAST> &_right_exp)
        {
            type = Op;
            leftExp = move(_left_exp);
            op = string(_op);
            rightExp = move(_right_exp);
        }

        void *build_koopa_values() const override{

            koopa_raw_value_data *res = nullptr;
            switch (type)
            {
            case Primary:
                res = (koopa_raw_value_data *)leftExp->build_koopa_values();
                break;
            case Op:
                unique_ptr<NumberAST> zero(new NumberAST(0));
                koopa_raw_value_data *temp_var = new koopa_raw_value_data();
                temp_var->ty = make_int_pointer_type();
                temp_var->name = new_char_arr("%temp");
                temp_var->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                temp_var->kind.tag = KOOPA_RVT_ALLOC;
                block_maintainer.AddInst(temp_var);
                //%temp = alloc i32  
                //store 0, %temp 
                koopa_raw_value_data *temp_store = new koopa_raw_value_data();
                temp_store->ty = make_simple_type(KOOPA_RTT_UNIT);
                temp_store->name = nullptr;
                temp_store->used_by = empty_koopa_rs();
                temp_store->kind.tag = KOOPA_RVT_STORE;
                temp_store->kind.data.store.dest = temp_var;
                temp_store->kind.data.store.value = (koopa_raw_value_t)zero->build_koopa_values();
                block_maintainer.AddInst(temp_store);

                //br eq &leftExp 0, @true, @end 
                koopa_raw_value_data *br = new koopa_raw_value_data();
                br->ty = make_simple_type(KOOPA_RTT_UNIT);
                br->name = nullptr;
                br->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                br->kind.tag = KOOPA_RVT_BRANCH;
                br->kind.data.branch.cond = build_not_eq_koopa((koopa_raw_value_t)leftExp->build_koopa_values());
                koopa_raw_basic_block_data_t *true_block = new koopa_raw_basic_block_data_t();
                koopa_raw_basic_block_data_t *end_block = new koopa_raw_basic_block_data_t();
                br->kind.data.branch.true_bb = true_block;
                br->kind.data.branch.false_bb = end_block;
                br->kind.data.branch.true_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
                br->kind.data.branch.false_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
                block_maintainer.AddInst(br);
                //这是true的基本快,存放true的指令
                true_block->name = new_char_arr("%true");
                true_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
                true_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                block_maintainer.AddNewBasicBlock(true_block);
                
                //store ne %rightExp, 0 %temp
                //jump @end
                koopa_raw_value_data *b_store = new koopa_raw_value_data();
                b_store->ty = make_simple_type(KOOPA_RTT_UNIT);
                b_store->name = nullptr;
                b_store->used_by = empty_koopa_rs();
                b_store->kind.tag = KOOPA_RVT_STORE;
                b_store->kind.data.store.dest = temp_var;
                b_store->kind.data.store.value = build_not_eq_koopa((koopa_raw_value_t)rightExp->build_koopa_values());
                block_maintainer.AddInst(b_store);
                block_maintainer.AddInst(JumpInst(end_block));

                //这是end的基本快,存放true的指令
                end_block->name = new_char_arr("%end");
                end_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
                end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                block_maintainer.AddNewBasicBlock(end_block);

                //%res = load %temp
                res = new koopa_raw_value_data();
                res->ty = make_simple_type(KOOPA_RTT_INT32);
                res->name = nullptr;
                res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                res->kind.tag = KOOPA_RVT_LOAD;
                res->kind.data.load.src = temp_var;
                block_maintainer.AddInst(res);

                break;
            }
            return res;
        }
        int CalcValue() const override
        {
            if (type == Primary)
                return leftExp->CalcValue();
            int res = 0;
            if (op == "&&")
                res = leftExp->CalcValue() && rightExp->CalcValue();
            return res;
        }
};



class LOrExpAST : public BaseAST
{

    koopa_raw_value_data *build_eq_koopa(koopa_raw_value_t exp) const
    {
        NumberAST zero(0);
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_simple_type(KOOPA_RTT_INT32);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_BINARY;
        auto &binary = res->kind.data.binary;
        binary.op = KOOPA_RBO_EQ;
        binary.lhs = exp;
        binary.rhs = (koopa_raw_value_t)zero.build_koopa_values();
        block_maintainer.AddInst(res);
        return res;
    }

    koopa_raw_value_data *build_not_eq_koopa(koopa_raw_value_t exp) const
    {
        NumberAST zero(0);
        koopa_raw_value_data *res = new koopa_raw_value_data();
        res->ty = make_simple_type(KOOPA_RTT_INT32);
        res->name = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_BINARY;
        auto &binary = res->kind.data.binary;
        binary.op = KOOPA_RBO_NOT_EQ;
        binary.lhs = exp;
        binary.rhs = (koopa_raw_value_t)zero.build_koopa_values();
        block_maintainer.AddInst(res);
        return res;
    }
    public:
        enum
        {
            Primary,
            Op
        } type;
        string op;
        unique_ptr<BaseAST> leftExp; // may be primary
        unique_ptr<BaseAST> rightExp;

        LOrExpAST(unique_ptr<BaseAST> &_primary_exp)
        {
            type = Primary;
            leftExp = move(_primary_exp);
        }
        LOrExpAST(unique_ptr<BaseAST> &_left_exp, const char *_op, unique_ptr<BaseAST> &_right_exp)
        {
            type = Op;
            leftExp = move(_left_exp);
            op = string(_op);
            rightExp = move(_right_exp);
        }
        void show() const override {
            if(type == Primary){
                cout<<"LorExp";
                cout<<" { ";
                leftExp->show();
                cout <<" } ";
            }
            else{
                cout<<"LorExp";
                cout<<" { ";
                leftExp->show();
                cout<<" || " ;
                rightExp->show() ;
                cout<<" } ";

            }
        }
        void *build_koopa_values() const override
        {
            koopa_raw_value_data *res = nullptr;
            switch (type)
            {
            case Primary:
                res = (koopa_raw_value_data *)leftExp->build_koopa_values();
                break;
            case Op:
                unique_ptr<NumberAST> zero(new NumberAST(0));
                unique_ptr<NumberAST> one(new NumberAST(1));
                koopa_raw_value_data *temp_var = new koopa_raw_value_data();
                temp_var->ty = make_int_pointer_type();
                temp_var->name = new_char_arr("%temp");
                temp_var->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                temp_var->kind.tag = KOOPA_RVT_ALLOC;
                block_maintainer.AddInst(temp_var);

                //%temp = alloc i32  
                //store 1, %temp 
                koopa_raw_value_data *temp_store = new koopa_raw_value_data();
                temp_store->ty = make_simple_type(KOOPA_RTT_UNIT);
                temp_store->name = nullptr;
                temp_store->used_by = empty_koopa_rs();
                temp_store->kind.tag = KOOPA_RVT_STORE;
                temp_store->kind.data.store.dest = temp_var;
                temp_store->kind.data.store.value = (koopa_raw_value_t)one->build_koopa_values();
                block_maintainer.AddInst(temp_store);

                //br eq &leftExp 0, @true, @end 
                koopa_raw_value_data *br = new koopa_raw_value_data();
                br->ty = make_simple_type(KOOPA_RTT_UNIT);
                br->name = nullptr;
                br->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                br->kind.tag = KOOPA_RVT_BRANCH;
                br->kind.data.branch.cond = build_eq_koopa((koopa_raw_value_t)leftExp->build_koopa_values());
                koopa_raw_basic_block_data_t *true_block = new koopa_raw_basic_block_data_t();
                koopa_raw_basic_block_data_t *end_block = new koopa_raw_basic_block_data_t();
                br->kind.data.branch.true_bb = true_block;
                br->kind.data.branch.false_bb = end_block;
                br->kind.data.branch.true_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
                br->kind.data.branch.false_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
                block_maintainer.AddInst(br);

                //这是true的基本快,存放true的指令
                true_block->name = new_char_arr("%true");
                true_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
                true_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                block_maintainer.AddNewBasicBlock(true_block);

                //store ne %rightExp, 0 %temp
                //jump @end
                koopa_raw_value_data *b_store = new koopa_raw_value_data();
                b_store->ty = make_simple_type(KOOPA_RTT_UNIT);
                b_store->name = nullptr;
                b_store->used_by = empty_koopa_rs();
                b_store->kind.tag = KOOPA_RVT_STORE;
                b_store->kind.data.store.dest = temp_var;
                b_store->kind.data.store.value = build_not_eq_koopa((koopa_raw_value_t)rightExp->build_koopa_values());
                block_maintainer.AddInst(b_store);
                block_maintainer.AddInst(JumpInst(end_block));

                //存放分支结束后的指令
                end_block->name = new_char_arr("%end");
                end_block->params = empty_koopa_rs(KOOPA_RSIK_VALUE);
                end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                block_maintainer.AddNewBasicBlock(end_block);

                //%res = load %temp
                res = new koopa_raw_value_data();
                res->ty = make_simple_type(KOOPA_RTT_INT32);
                res->name = nullptr;
                res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
                res->kind.tag = KOOPA_RVT_LOAD;
                res->kind.data.load.src = temp_var;
                block_maintainer.AddInst(res);

                break;
            }
            return res;
        }
        int CalcValue() const override
        {
            if (type == Primary)
                return leftExp->CalcValue();
            int res = 0;
            if (op == "||")
                res = leftExp->CalcValue() || rightExp->CalcValue();
            return res;
        }
};







///
class ExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> unaryExp;

    ExpAST(unique_ptr<BaseAST> &_unaryExp)
    {
        unaryExp = move(_unaryExp);
    }

    void *build_koopa_values() const override
    {
        return unaryExp->build_koopa_values();
    }

    int CalcValue() const override
    {
        return unaryExp->CalcValue();
    }
};

//...