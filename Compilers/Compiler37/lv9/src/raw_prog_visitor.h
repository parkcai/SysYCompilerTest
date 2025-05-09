#pragma once

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <functional>
#include "koopa.h"

class AbstractKoopaRawProgramProcessor{
    public:
    //program pre & post method
        virtual void startProgram(const koopa_raw_program_t& function)=0;
        virtual void endProgram(const koopa_raw_program_t& function)=0;
    //stackFrame control method
        //重定向code生成流,开始栈记录
        virtual void startFunction(const koopa_raw_function_t& function) = 0;
        //different from ret command
        //ret command will only do register filling
        //终止函数 生成栈指针维护指令
        virtual void endFunction(const koopa_raw_function_t& function) = 0;
    //block pre & post method ,用于后面记录整块与处理跳转 比如前面start是否要生成label，endBlock可以用于收集块内的code
        virtual void startBlock(const koopa_raw_basic_block_t &bb)  = 0;
        virtual void endBlock(const koopa_raw_basic_block_t& bb) = 0;
    //process different command method
        //virtual void process(const koopa_raw_function_t& func)  =0;
        
        //virtual void process(const koopa_raw_value_t & value) const =0;
        //return 指令
        virtual void process(const koopa_raw_return_t &ret)  = 0;
        //integer 指令 尝试将一个整数载入
        virtual void process(const koopa_raw_integer_t &integer,const std::string& reg)  = 0;
        //load指令
        virtual void process(const koopa_raw_load_t & load,const koopa_raw_value_t& value) = 0;
        //store指令
        virtual void process(const koopa_raw_store_t & store)  = 0;
        //binary operation 指令
        virtual void process(const koopa_raw_binary_t& binary,const koopa_raw_value_t& value) =0;
        //allocate 指令
        virtual void allocate(const koopa_raw_value_t& alloc)=0;
        virtual void process(const koopa_raw_branch_t& branch) = 0;
        virtual void process(const koopa_raw_jump_t& jump) = 0;
        virtual void process(const koopa_raw_call_t& call,const koopa_raw_value_t& value) = 0;
        virtual void process(const koopa_raw_global_alloc_t& global,const koopa_raw_value_t& value) = 0;
        virtual void process(const koopa_raw_get_elem_ptr_t& getElem,const koopa_raw_value_t& value) = 0;
        virtual void process(const koopa_raw_get_ptr_t& getPtr,const koopa_raw_value_t& value) = 0;

    //manager of commands 
        void processCommand(const koopa_raw_value_t &value)
        {
            // 根据指令类型判断后续需要如何访问

            const auto &kind = value->kind;
            switch (kind.tag)
            {
            case KOOPA_RVT_RETURN:
                // 访问 return 指令
                process(kind.data.ret);
                break;
            case KOOPA_RVT_INTEGER:
                // 访问 integer 指令
                //process(kind.data.integer);
                _LOGIC("Instruction Only loading register can only be issued within other instructions");
                break;
             case KOOPA_RVT_ALLOC:
                allocate(value);
                break;
            case KOOPA_RVT_LOAD:
                process(kind.data.load, value);
                break;
            case KOOPA_RVT_STORE:
                process(kind.data.store);
                break;
            case KOOPA_RVT_BINARY:
                process(kind.data.binary, value);
                break;
            case KOOPA_RVT_BRANCH:
                process(kind.data.branch);
                break;
            case KOOPA_RVT_JUMP:
                process(kind.data.jump);
                break;
            case KOOPA_RVT_CALL:
                process(kind.data.call,value);
                break;
            case KOOPA_RVT_GLOBAL_ALLOC:
                process(kind.data.global_alloc,value);
                break;
            case KOOPA_RVT_GET_ELEM_PTR:
                process(kind.data.get_elem_ptr,value);
                break;
            case KOOPA_RVT_GET_PTR:
                process(kind.data.get_ptr,value);
                break;
            default:
                // 其他类型暂时遇不到
                _NOT_IMPLEMENTED(std::string("kind.tag: ")+std::to_string( static_cast<int>(kind.tag)));
            }
        } 
        virtual void processArgLd(const koopa_raw_func_arg_ref_t&value ,const std::string& reg) = 0;
        virtual void processArgLdAddr(const koopa_raw_func_arg_ref_t&value ,const std::string& reg) = 0;
        //li or lw
        //value的值载入寄存器
        virtual void processLwReg(const koopa_raw_value_t& value,const std::string & reg) = 0 ;
        virtual void processLwRegAddr(const koopa_raw_value_t& value,const std::string & reg)=0;
        void processRegload(const koopa_raw_value_t& value,const std::string& reg){
            const auto &kind = value->kind;
            switch (kind.tag)
            {
                case KOOPA_RVT_INTEGER:
                //processLiReg load immediate to reg
                    process(value->kind.data.integer,reg);
                    break;
                case KOOPA_RVT_FUNC_ARG_REF:
                    processArgLd(value->kind.data.func_arg_ref,reg);
                    break;
                case KOOPA_RVT_GLOBAL_ALLOC:
                    processGlLw(value,reg);
                    break;
                default:
                    processLwReg(value,reg);
            }
        }
        //value的地址载入寄存器
        void processRegLoadAddr(const koopa_raw_value_t& value,const std::string& reg) {
            const auto& kind  = value->kind;
            switch (kind.tag)
            {
                case KOOPA_RVT_INTEGER:
                    _LOGIC("get ptr of a const value")
                    break;
                case KOOPA_RVT_FUNC_ARG_REF:
                    processArgLdAddr(value->kind.data.func_arg_ref,reg);
                    break;
                case KOOPA_RVT_GLOBAL_ALLOC:
                    processGlLwAddr(value,reg);
                    break;
                default:
                    processLwRegAddr(value,reg);
            }
        }
        virtual void processGlLw(const koopa_raw_value_t& value,const std::string& reg) final {
             std::cout << "  la t6, " << (value->name+1) << std::endl;
            std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
        }
         virtual void processGlLwAddr(const koopa_raw_value_t& value,const std::string& reg) final{
            std::cout << "  la " << reg << ", " << (value->name+1) << std::endl;
        }
        virtual void processGlSw(const koopa_raw_value_t& value,const std::string& reg,bool isPointer) final{
            std::cout << "  la t6, " << (value->name+1) << std::endl;
            if(isPointer){
                std::cout << "  lw t6, 0(t6)" << std::endl;
            }
            std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
        }
        virtual void processSwReg(const koopa_raw_value_t& value,const std::string& reg,bool isPointer) = 0;
        void processRegSave(const koopa_raw_value_t& value,const std::string& reg,bool isValueAPointer){
            const auto &kind = value->kind;
            switch (kind.tag)
            {
                // case KOOPA_RVT_FUNC_ARG_REF:
                //     processArgLd(value->kind.data.func_arg_ref,reg);
                //     break;
                case KOOPA_RVT_GLOBAL_ALLOC:
                    processGlSw(value,reg,isValueAPointer);
                    break;
                default:
                    processSwReg(value,reg,isValueAPointer);
            }
        }
        //sw
        
        //generate BiOp command with register
        void processBiOp(const koopa_raw_binary_t& binary,const std::string& result,const std::string& arg1,const std::string& arg2){
            if(binary.op == KOOPA_RBO_NOT_EQ) {
                std::cout << "  xor "<<result<<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
                std::cout << "  snez "<<result<<", "<<result<<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_EQ) {
                std::cout << "  xor "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
                std::cout << "  seqz "<< result <<", "<< result <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_GT) {
                std::cout << "  sgt "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_LT) {
                std::cout << "  slt "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_GE) {
                std::cout << "  slt "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
                std::cout << "  xori "<< result <<", "<< result <<", 1" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_LE) {
                std::cout << "  sgt "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
                std::cout << "  xori "<< result <<", "<< result <<", 1" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_ADD) {
                std::cout << "  add "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_SUB) {
                std::cout << "  sub "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_MUL) {
                std::cout << "  mul "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_DIV) {
                std::cout << "  div "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_MOD) {
                std::cout << "  rem "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_AND) {
                std::cout << "  and "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_OR) {
                std::cout << "  or "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_XOR) {
                std::cout << "  xor "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_SHL) {
                std::cout << "  sll "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_SHR) {
                std::cout << "  srl "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
            else if(binary.op == KOOPA_RBO_SAR) {
                std::cout << "  sra "<< result <<", "<< arg1 <<", "<< arg2 <<"" << std::endl;
            }
        }
        
};

class KoopaRawProgramVisiter{
    protected:
        AbstractKoopaRawProgramProcessor *processor;
    public:
        KoopaRawProgramVisiter(AbstractKoopaRawProgramProcessor* processor):processor(processor){

        }
        ~KoopaRawProgramVisiter() = default;
        void visit(const koopa_raw_program_t &program){
            processor->startProgram(program);
            visitSlice(program.values);
            visitSlice(program.funcs);
            processor->endProgram(program);
        }
        void visitSlice(const koopa_raw_slice_t& slice ){
            for (size_t i = 0; i < slice.len; ++i)
            {
                auto ptr = slice.buffer[i];
                // 根据 slice 的 kind 决定将 ptr 视作何种元素
                switch (slice.kind)
                {
                case KOOPA_RSIK_FUNCTION:
                    // 访问函数
                    visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                    break;
                case KOOPA_RSIK_BASIC_BLOCK:
                    // 访问基本块
                    visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                    break;
                case KOOPA_RSIK_VALUE:
                    // 访问指令
                    visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                    break;
                case KOOPA_RSIK_TYPE:
                    _NOT_IMPLEMENTED("Type kind not implemented")
                    break;
                default:
                    // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                    _ERROR("slice kind :Unknown")
                    
                }
            }
        }
        // 访问函数
        void visit(const koopa_raw_function_t &func)
        {
            // 执行一些其他的必要操作
            // ...
            // 访问所有基本块
            //重定向code生成流,开始栈记录
            processor->startFunction(func);
            visitSlice(func->bbs);
            //终止函数 生成栈指针维护指令
            processor->endFunction(func);
        }

        // 访问基本块
        void visit(const koopa_raw_basic_block_t &bb)
        {
            // 执行一些其他的必要操作
            // ...
            // 访问所有指令
            processor->startBlock(bb);

            visitSlice(bb->insts);
            processor->endBlock(bb);
        }


        // 访问指令
        void visit(const koopa_raw_value_t &value)
        {
            processor->processCommand(value);
        }
        // void visit(const koopa_raw_return_t& ret){
        //     processor->process(ret);

        // }
        // //访问int
        // void visit(const koopa_raw_integer_t& integer){
        //     processor->process(integer);
        // }

        // //
        // // void visit(const koopa_raw){

        // // }
        // //访问load指令
        // void visit(const koopa_raw_load_t& load,const koopa_raw_value_t& value){
        //     processor->process(load,value);
        // }
        // // 访问 store 指令
        // void visit(const koopa_raw_store_t &store) {
        //     processor->process(store);
        // }
        // //访问binary
        // void visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
        //     processor->process(binary,value);
        // }

};