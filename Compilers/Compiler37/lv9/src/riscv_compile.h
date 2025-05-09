#pragma once

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <functional>
#include <algorithm>
#include "koopa.h"
#include "raw_prog_visitor.h"
#include "debug.h"


// struct Info{
//     virtual bool isMultiDim() = 0;
//     virtual bool isPointer() = 0;
//     ~ Info
// };
//reference to a value object
// a[][][][]
// struct ValueInfo :public Info{
//     public:
//         ValueInfo(const koopa_raw_value_t& ref):valueRef(ref){
//             auto base =ref->ty->data.pointer.base;
//             totalSize = 1;
//             totalDim = 0;
//             while (base->tag == KOOPA_RTT_ARRAY)
//             {
//                 arraySize .push_back(base->data.array.len);
//                 totalSize *=base->data.array.len;
//                 totalDim +=1;
//                 base = base->data.array.base;
//             }
//         }
//         bool isMultiDim() override{
//             return totalDim >0;
//         }
//         bool isPointer() override {
//             return false;
//         }
//         const koopa_raw_value_t& valueRef;
//         int totalDim;
//         int totalSize;
//         std::vector<int> arraySize;
// };
//reference to a pointer object
// **** a[][][][]
#define TYPE_ALLOC 0
#define TYPE_LOAD 1
#define TYPE_GETPTR 2


#define ALLOC_AS_INT 1
#define ALLOC_AS_ARRAY 2
#define ALLOC_AS_ARRAY_PTR 3

struct PointerInfo{
    public:
        int type;
        int allocType;
        //约定 如果是*(int[])就在arraySize末尾填1 
        PointerInfo(const koopa_raw_value_t& ref,int allocT):valueRef(ref){
            auto base = ref->ty->data.pointer.base;
            pointer = 0;
            totalSize = 1;
            totalDim = 0;
            allocType = allocT;
            //因为这里也是新的临时变量用的, 当为false时 不计算全部默认为int
            if(allocType==ALLOC_AS_ARRAY||allocType==ALLOC_AS_ARRAY_PTR){
                if(allocType == ALLOC_AS_ARRAY_PTR){
                    pointer = 1;
                    _PRECONDITION(base->tag ==KOOPA_RTT_POINTER,"Type Mismatch for ALLOC_ARRAY_PTR")
                }
                while(base ->tag ==KOOPA_RTT_POINTER){
                    base = base->data.pointer.base;
                }
                while (base->tag == KOOPA_RTT_ARRAY)
                {
                    arraySize .push_back(base->data.array.len);
                    totalSize *=base->data.array.len;
                    totalDim +=1;
                    base = base->data.array.base;
                    if(!(base&& base->tag)){
                        break;
                    }
                }
                arraySize.push_back(1);
                if(allocType == ALLOC_AS_ARRAY){
                    _PRECONDITION(totalDim>0,"TypeMismatch for ALLOC_ARRAY");
                }
            }
            
            
            type =TYPE_ALLOC;
            toString();
        }
        //as pointer
        //给ref创建一个pointer的身份,是base下一级的
        PointerInfo(const PointerInfo& base,const koopa_raw_value_t& ref):valueRef(ref){
            //继承目标的属性 用于计算
            //这指的是分配的空间是否存储 ref的地址 还是存储ref本身
            //分配的空间是记录在栈注册表中的
            //当我们获取value的时候需要检查 如果记录的是地址 需要。。。
            pointer = 1;
            totalSize = 1;
            totalDim = 0;
            for (auto i = 1; i < base.totalDim; i++)
            {
                totalDim += 1;
                int indexT = base.arraySize[i];
                arraySize.push_back(indexT);
                totalSize *= indexT;
                /* code */
            }
            //继承一下base的标识符
            allocType = base.allocType;
            type = TYPE_GETPTR;
            toString();
        }
        //as copy
        PointerInfo(const koopa_raw_value_t& ref,const PointerInfo& origin):valueRef(ref){
            pointer = origin.pointer;
            totalDim = origin.totalDim;
            totalSize = origin.totalSize ;
            arraySize = origin.arraySize;
           // loadInfo = origin.isDataDirectlySavedOrAPointerReference();
            type = TYPE_LOAD;
            allocType = origin.allocType;
            toString();
        }
        bool isMultiDim()const  {
            return totalDim >0;
        }
        // bool isPointer()const  {
        //     return pointer>0;
        // }
   
        bool isDataDirectlySavedOrAPointerReference()const{
            if(type==TYPE_ALLOC){
                //alloc出来的 直接通过info就可以找到data所在的位置
                return true;
            }else if(type==TYPE_GETPTR){
                //getptr出来的 肯定是pointer reference
                return false;
            }else if(type==TYPE_LOAD){
                //从某个不知道是啥玩意的地方load出来的
                //应该和之前的一样
                return pointer == 0;
            }
            _NOT_IMPLEMENTED("No more mod")
        }
        void toString() {
            _DEBUG("PointerInfo")
            _DEBUG(std::string("Ref name: ")+valueRef->name)
            _DEBUG("Dim: "+ std::to_string(totalDim))
            _DEBUG("Pointer: "+ std::to_string(pointer))
            _DEBUG("Size: "+ std::to_string( totalSize ) )
            _DEBUG("type :" +std::to_string(type))
            _DEBUG("allocType :"+std::to_string(allocType))
        }
        int pointer;
        const koopa_raw_value_t valueRef;
        int totalDim;
        int totalSize;
        std::vector<int> arraySize;
        int wordSizeOf()const {
            //*(int[]) 仍旧是4
            return (pointer>0)?1:totalSize;
        }
        //cal size of pointer
        // T -> sizeof(T*);
        int pointerSizeOf()const {
            if(type==TYPE_ALLOC){
                switch (allocType)
                {
                    case ALLOC_AS_ARRAY:
                        return totalSize/arraySize[0];

                    /* code */
                    case ALLOC_AS_ARRAY_PTR:
                        return totalSize;
                    default:
                        _ERROR("Unsupported type ALLOC_AS_INT for pointerSizeOf")
                }
            }else if(type == TYPE_GETPTR){
                switch (allocType)
                {
                    case ALLOC_AS_ARRAY:
                        return totalSize/arraySize[0];

                    /* code */
                    case ALLOC_AS_ARRAY_PTR:
                        return totalSize;
                    default:
                        _ERROR("Unsupported type ALLOC_AS_INT for pointerSizeOf")
                }
            }else if(type ==TYPE_LOAD){
                switch (allocType)
                {
                    case ALLOC_AS_ARRAY:
                        return totalSize/arraySize[0];

                    /* code */
                    case ALLOC_AS_ARRAY_PTR:
                        return totalSize;
                    default:
                        _ERROR("Unsupported type ALLOC_AS_INT for pointerSizeOf")
                }
            }else{
                _ERROR("Unknown type?")
            }
        }
};
struct AddrInfo {
    public:
        virtual bool isOnStack() = 0;
        //virtual std::string 
};
class FuncInfo{
    private:
        int stackTop = 0;
        int maxStackSize = 0 ;
    public:
        std::string name;
        int paramSize;
        std::unordered_map<koopa_raw_value_t, int> stackFrameAddrMap;
        //std::unordered_map<koopa_raw_value_t, ValueInfo > valueInfo;
        //孩子们 我们合体了 
        // *** a [][][]
        //当 ***没了就是数组,
        std::unordered_map<koopa_raw_value_t, std::unique_ptr<PointerInfo> > pValueInfo;
        std::vector<std::string>  calledFunc;
        bool complete = false ; 
        bool withEndPart=false;
        //is valid only with withEndPart=false;
        koopa_raw_basic_block_t blockWithRetOptional;
        //offset will be alloc from sp+stackSize to sp
        //alloc a value at stack
        //that means I have a 1 ,我把这个1 放到了sp+4中 记录这个valueT为sp+4
        //访问时候只要lw 4(sp)就行
        int* allocateOnStack(const koopa_raw_value_t& valueT,int complex){
           //"0(sp)" "4(sp)"
           _DEBUG(std::string("Allocate as value on stack")+valueT->name)
            auto& ref = (pValueInfo[valueT]=std::make_unique<PointerInfo>(valueT,complex));
            expandStack(4*ref->wordSizeOf());
            auto& offset= stackFrameAddrMap[valueT] = stackTop;
            //int [] [][][][][][][]][][][][][]
            //如果是指针 只用存4大小的
            
            _DEBUG("Allocate success")
            return &offset;
        }
        //I have a 1 它的地址是 sp+114(可能在别的栈帧里) 我希望分配sp+4存储sp+114 然后获取的时候 我希望直接获取到sp+114
        //那么 %s = load %i 是什么
        //store和load需要继承属性!
        int* allocateAsPointerOnStack(const koopa_raw_value_t& valueT,const PointerInfo& base){
            _DEBUG("Allocate as ptr on stack")
           _DEBUG(std::string("name ")+valueT->name)

            //auto& ref=
            (pValueInfo[valueT]=std::make_unique<PointerInfo>(base,valueT));
      
           // _PRECONDITION(,"What?")
            _DEBUG("Allocate finish")
            getVPAInfo(valueT)->toString();
            expandStack(4);
            auto& offset= stackFrameAddrMap[valueT] = stackTop;
            //int [] [][][][][][][]][][][][][]
            //如果是指针 只用存4大小的
            
            return &offset;
        }
        int* allocateAsCopyOnStack(const koopa_raw_value_t& valueT,const PointerInfo& origin){
            //继承origin的标识符信息
           // auto& ref = 
           _DEBUG("Allocate as copt on stack")
           _DEBUG(std::string("name ")+valueT->name)
           _PRECONDITION(pValueInfo.find(valueT)==pValueInfo.end(),"What???")
            (pValueInfo[valueT]=std::make_unique<PointerInfo>(valueT,origin));
            _DEBUG("?")
            getVPAInfo(valueT)->toString();
            expandStack(4);
            auto& offset= stackFrameAddrMap[valueT] = stackTop;
            //非指针多维度 是数组
            _PRECONDITION( !(origin.isMultiDim()&&origin.pointer==0),"We don't support copying an array directly here,you should use getptr to copy array pointer")
            //哈哈哈哈
            //*(int[])
            //int [] [][][][][][][]][][][][][]
            //不能是array只能是这个
            // @var = *alloc[4][5]//这是指针 占地4
            //  @var = alloc 这是int 占地4
            // %byd = load @var 这是载入值 没问题 载入的值应当和原定义拥有相同属性
            
            return &offset;
        }
        
        void expandStack(int size){
            stackTop+=size;
            maxStackSize = std::max(stackTop,maxStackSize);
        }
        void preExpandStack(int size){
            //to leave enough space for placing sth
            maxStackSize = std::max(stackTop+size,maxStackSize);
        }
        int getMaxStackSize(){

            return (maxStackSize+15)& ~15;
        }
        int* getAddrOnStack(const koopa_raw_value_t& valueT){
            auto it = stackFrameAddrMap.find(valueT);
            if (it != stackFrameAddrMap.end()) {
                return &(it->second);  
            }
            return nullptr;
        }
        void doFunctionCall(const std::string& value){
            calledFunc.push_back(value);
        }
        // ValueInfo& getVal =ueInfo(const koopa_raw_value_t& value){
        //     _PRECONDITION(valueInfo.find(value)!=valueInfo.end(),"value info not found!")
        //     return valueInfo[value];
        // }
        PointerInfo* getVPAInfo(const koopa_raw_value_t& pointer){
           // _PRECONDITION(,"value info not found! "+pointer->name)
            auto ret = pValueInfo.find(pointer);
            PointerInfo* val ;
            if(ret != pValueInfo.end()){
                val = ret ->second.get();
            }else{
                val = nullptr;
            }
            //should be all ok
            return val;
        }

};
class InfoPreReadKoopaProgramProcessor: public AbstractKoopaRawProgramProcessor{
    public:
        static InfoPreReadKoopaProgramProcessor *INSTANCE ;
        //simple plan
        //when doing function call ,these should be in a stack
        //we just put a simple example here
        std::unordered_map<std::string,std::unique_ptr<FuncInfo> > funcDefs;
        FuncInfo* top = nullptr;
        std::unordered_map<koopa_raw_value_t,std::unique_ptr<PointerInfo> > globalInfo;
        //怎么处理双return的编译错误
        //we will do " save and load" ,without register first
        PointerInfo* getVPAInfo(const koopa_raw_value_t& value){
            PointerInfo* p;
            p = top -> getVPAInfo(value);
            if(p){
                return p;
            }else{
                return globalInfo[value].get();
            }
        }
        std::ostringstream buf;
        std::streambuf* originStdGlobal;
        //should be 16x
        void startProgram(const koopa_raw_program_t &program)  override{
            _INTER("program start")
            originStdGlobal=std::cout.rdbuf();
            std::cout.rdbuf(buf.rdbuf());
            //maybe we will move some code into this later
        }
        virtual void endProgram(const koopa_raw_program_t& function)override{
            _INTER("program end")
            std::cout.rdbuf(originStdGlobal);
        }
        

        
        void startFunction(const koopa_raw_function_t &func) override{
             _INTER("Function start")
            auto funcInfo = new FuncInfo();
            funcInfo->name = func->name;
            funcInfo->paramSize = func->params.len;
            _DEBUG("Func Argument Amount"+std::to_string( funcInfo->paramSize))

            _DEBUG(std::string("Func Name")+func->name)
            auto& ptr = funcDefs[func->name]=std::unique_ptr<FuncInfo>(funcInfo);
            top = ptr.get();
        }
        virtual void endFunction(const koopa_raw_function_t& function)override{
            _INTER("Function end")
            //stop writting to top
            //calculate stackTop;
            top -> complete = true;
            top = nullptr;
        
        }
        
        void startBlock(const koopa_raw_basic_block_t &bb)  override{
            _INTER("basicBlock start")
            withReturn =false;
    
        }
        bool withReturn;
        virtual void endBlock(const koopa_raw_basic_block_t& bb)override{
            _INTER("basicBlock end")
            //mark the ret position
            if(withReturn){
                
                top->blockWithRetOptional = bb;
            }
            if(!std::strncmp(bb->name+1, "end_part",8)){
                top->withEndPart = true;
            }
        }

        //simple policy: 所有运行用的寄存器都用a0 a0
        //under this policy, return only need to load ret.val
        void process(const koopa_raw_return_t &ret)  override{
            _INTER("return")
            withReturn = true;
            if(ret.value){
                //ret value not void
                processRegload(ret.value,"a0");
            }
            
            //no need to store something,because this is going to return!
        }
        void process(const koopa_raw_integer_t &integer,const std::string& reg)  override{
             _INTER("integer")
            //std::cout <<"  li "<< reg << "," << integer.value <<"\n";
        }
        virtual void process(const koopa_raw_load_t & load,const koopa_raw_value_t& value) override{
            _INTER("load")
            // processRegload(load.src,"t0");
            // processRegSave(value,"t0");
            _DEBUG(load.src->name)
            PointerInfo* valueInfo =getVPAInfo(load.src);
            _DEBUG(load.src->name)
            _DEBUG("source")
            valueInfo->toString();
            _DEBUG(value->name)
            //创建信息拷贝
            top->allocateAsCopyOnStack(value,*valueInfo);
        }
        virtual void process(const koopa_raw_store_t & store) override{
            _INTER("store")
           // processRegload(store.value,"t0");

            //processRegSave(store.dest,"t0");
            //*********函数参数不在getVPA里面*************
            // 不是哥们 你创建dest的时候应当已经有info了
           // PointerInfo& valueInfo = top->getVPAInfo(store.value);
            //创建信息拷贝
           // top->allocateAsCopyOnStack(store.dest,valueInfo);
        }
        virtual void process(const koopa_raw_binary_t& binary,const koopa_raw_value_t& value) override{
            _INTER("binary")
            // processRegload(binary.lhs,"t0");
            // processRegload(binary.rhs,"t1");
            // processBiOp(binary,"t0","t0","t1");
            //创建结果值 当前还不支持除了int之外的结果
            top->allocateOnStack(value,ALLOC_AS_INT);
           // processRegSave(value,"t0");
        }
        virtual void allocate(const koopa_raw_value_t& alloc)override{
            //  std::cerr<<"debuging alloc\n";
            _INTER("alloc")
            auto& tag=alloc->ty->data.pointer.base->tag;
            _DEBUG("type "+std::to_string(tag))
            if(tag==KOOPA_RTT_INT32 ){
                //new ValueInfo in alocate
                top->allocateOnStack(alloc,ALLOC_AS_INT);
            }else if(tag ==KOOPA_RTT_ARRAY){
                top->allocateOnStack(alloc,ALLOC_AS_ARRAY);
            }
            else if(tag == KOOPA_RTT_POINTER){
                top->allocateOnStack(alloc,ALLOC_AS_ARRAY_PTR);
            }
            
            //use value->name to find variable name
            
            // // loc[value] = std::to_string(stack_frame_used) + "(sp)";
            // // stack_frame_used += 4;
            // _ERROR("abort")
        }
        virtual void process(const koopa_raw_branch_t& branch) override{
            _INTER("branch")
           // processRegload(branch.cond,"t0");
            // std::cout << "  bnez t0, " << branch.true_bb->name+1 <<"\n";
            // std::cout << "  j " << branch.false_bb->name+1 << "\n";
            // _DEBUG(branch.true_bb->name)
            // _DEBUG(branch.false_bb->name)
            //branch instruction
        }
        virtual void process(const koopa_raw_jump_t& jump) override{
            _INTER("jump")
           // std::cout << "  j " << jump.target->name+1 << "\n";
        }
        virtual void process(const koopa_raw_call_t& call,const koopa_raw_value_t& value) override{
            _INTER("call");
            for (size_t i = 0; i < call.args.len; ++i) {
                //auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
                // if (i < 8) {
                //     processRegload(arg ,"a"+std::to_string(i));
                // }
                // else {
                //     processRegload(arg, "t0");
                //     // std::cout << "  li t6, " << (i - 8) * 4 << std::endl;
                //     // std::cout << "  add t6, t6, sp" << std::endl;
                //     // std::cout << "  sw t0, 0(t6)" << std::endl;
                // }
            }
            if( call.args.len>8){
                top -> preExpandStack(4*(call.args.len-8));
            }
           // std::cout << "  call " << call.callee->name+1 << std::endl;
            if(value->ty->tag != KOOPA_RTT_UNIT) {
                //processRegSave(value,"a0");
                //创建一个返回值 需要allocate 应当为int
                top->allocateOnStack(value,ALLOC_AS_INT);
            }
            top->doFunctionCall(call.callee->name);
        }
        //global
        virtual void process(const koopa_raw_global_alloc_t& global,const koopa_raw_value_t& value) override {
            // nextGlobal();
            // std::cout << "  .data" << std::endl;
            // std::cout << "  .globl " << value->name+1 << std::endl;
            // std::cout << value->name+1 << ":" << std::endl;
            // if (global.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
               
            //     std::cout << "  .zero 4" << std::endl;
            // }
            // else if (global.init->kind.tag == KOOPA_RVT_INTEGER) {
                
            //     std::cout << "  .word " << global.init->kind.data.integer.value << std::endl;
            // }
            // endGlobal();
            //generate global variable Info
            _INTER("global alloc")
            auto& tag=value->ty->data.pointer.base->tag;
            _DEBUG("type "+std::to_string(tag))
            int typeT ;
            if(tag==KOOPA_RTT_INT32 ){
                //new ValueInfo in alocate
                typeT = ALLOC_AS_INT;
            }else if(tag ==KOOPA_RTT_ARRAY){
                typeT = ALLOC_AS_ARRAY;
            }
            else if(tag == KOOPA_RTT_POINTER){
                typeT = ALLOC_AS_ARRAY_PTR;
            }else{
                _ERROR("Unknown global type")
            }
            globalInfo.insert(std::make_pair(value,std::make_unique<PointerInfo>(value,typeT)));
        }
        virtual void process(const koopa_raw_get_elem_ptr_t& getPtr,const koopa_raw_value_t& value) override{
            //getElem
            _DEBUG("getElemptr")
            doPointerIssue(getPtr.src,getPtr.index,value);

        }   
        void doPointerIssue(const koopa_raw_value_t& source,const koopa_raw_value_t& index,const koopa_raw_value_t& value){
            //推导value的类型
            _INTER("do pointer issue")
            auto* base = getVPAInfo(source);
            top->allocateAsPointerOnStack(value,*base);
        }
        virtual void process(const koopa_raw_get_ptr_t& getPtr,const koopa_raw_value_t& value) override {
            _DEBUG("getptr")
           doPointerIssue(getPtr.src,getPtr.index,value);
     
        }
        //using 
        virtual void processLwReg(const koopa_raw_value_t& value,const std::string & reg) override{
            //should have a getOrAlloc but, this is a read instruction,so it must be already on the stack
        }
        virtual void processLwRegAddr(const koopa_raw_value_t& value,const std::string & reg) override{
             //should have a getOrAlloc but, this is a read instruction,so it must be already on the stack
        }
        virtual void processSwReg(const koopa_raw_value_t& value,const std::string& reg,bool byd)override{
           // auto addr=
           
            //getOrAllocate(value);
            // if(*addr<2048){
            //     std::cout << "  sw " << reg << ", " << *addr<<"(sp)" << std::endl;
            // }else{
            //     std::cout << "  li t6, " << *addr << std::endl;
            //     std::cout << "  add t6, t6, sp" << std::endl;
            //     std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
            // }
            
        }
        
        void processSpMove(int delta){
            // if(delta==0)return;
            // int abs=(delta<0)?-delta:delta;
            // if(abs<=2040){
            //     std::cout << "  addi sp, sp, "<<delta<<"\n";
            // }else{
            //     std::cout << "  li t6, " << delta << std::endl;
            //     std::cout << "  add sp, sp, t6" << std::endl;

            // }
            
        }
        void processArgLd(const koopa_raw_func_arg_ref_t&value ,const std::string& reg) override{

        }
        virtual void processArgLdAddr(const koopa_raw_func_arg_ref_t&value ,const std::string& reg) override{

        }
        
        
};

class SimpleKoopaRawProgramProcessor:public AbstractKoopaRawProgramProcessor{
    public:
        SimpleKoopaRawProgramProcessor(InfoPreReadKoopaProgramProcessor* info) : preReadInfo(info->funcDefs),globalInfo(info->globalInfo){
            
        }
        std::unordered_map<std::string,std::unique_ptr<FuncInfo> >& preReadInfo;
        std::unordered_map<koopa_raw_value_t,std::unique_ptr<PointerInfo> >& globalInfo;
        FuncInfo* top;
        //simple plan
        //when doing function call ,these should be in a stack

        // std::ostringstream oss;
        // std::streambuf* originStd;
        bool hasmarkedRetPosition;
        std::string codeBeforeRet;
        int globalObjectCnt = 0;
        PointerInfo* getVPAInfo(const koopa_raw_value_t& value){
            PointerInfo* p;
            p = top -> getVPAInfo(value);
            if(p){
                return p;
            }else{
                return globalInfo[value].get();
            }
        }
        //怎么处理双return的编译错误
        //we will do " save and load" ,without register first
        int* allocateOnStack(const koopa_raw_value_t& valueT){
           //"0(sp)" "4(sp)"
           //use preRead Info
            return top->getAddrOnStack(valueT);
        }
        int* getOrAllocate(const koopa_raw_value_t& valueT){
            //use preRead Info
            return getAddrOnStack(valueT);
        }
        int* getAddrOnStack(const koopa_raw_value_t& valueT){
            //use preRead Info
            return top->getAddrOnStack(valueT);
        }
        //should be 16x
        inline int getStackFrameSize(){
            //use preRead Info
            //预留4个 如果有函数调用,四个位置用给ra
            return top->getMaxStackSize()+ ((top->calledFunc.empty())?0:4) ;
        }
        inline int getAllocStackFrameSize(){
            //能被申请的空间大小,已经排除ra的位置了
            return top->getMaxStackSize();
        }
        // void redirectStdForStackBody(){
        //     originStd =std::cout.rdbuf();
        //     std::cout.rdbuf(oss.rdbuf());
            
        // }
        // std::string collectCodeWhenRet(){
        //     std::string ret=oss.str();
        //     //清除缓冲区
        //     oss.str("");
        //     std::cout.rdbuf(originStd);
        //     return ret;
        // }
        std::vector<std::string> globalCodes;

        void startProgram(const koopa_raw_program_t &program)  override{
            _INTER("program start")
        
            //maybe we will move some code into this later
            
            
        }
        virtual void endProgram(const koopa_raw_program_t& function)override{
            _INTER("program end")
            bool oneOrMoreGlobal=false;
            for (auto i = globalCodes.begin(); i != globalCodes.end(); i++)
            {
                /* code */
                if((*i).empty()){
                    continue;
                }
                if(oneOrMoreGlobal){
                    std::cout << "\n";
                }
                
                std::cout << (*i);
                oneOrMoreGlobal = true;
            }
            //todo assemble function codes here
        }
        std::ostringstream currentGlobal;
        std::streambuf* originStdGlobal;
        void nextGlobal(){
            originStdGlobal = std::cout.rdbuf();
            std::cout.rdbuf(currentGlobal.rdbuf());
        }
        void endGlobal(){
            globalCodes.push_back(currentGlobal.str());
            currentGlobal.str("");
            std::cout.rdbuf(originStdGlobal);

        }
        void startFunction(const koopa_raw_function_t &func) override{
             _INTER("Function start")
             
            nextGlobal();
            hasmarkedRetPosition = false;
            codeBeforeRet = "";
            //跳过函数声明
            if(func->bbs.len==0){
                return;
            }
            std::cout << "  .text\n"//todo move this to startProgram
                      << "  .globl "
                      << (func->name)+1
                      << "\n"
                      << (func->name)+1 << ":\n";
            //需要重定向输出流 截取从process到ret的全部指令
            top = preReadInfo[func->name].get();
            if(!(top->calledFunc.empty())){
                //save ra
                processRegSwImm12(-4,"ra",false);
            }
            int stackSize = getStackFrameSize();
            
            //可以根据是否要保存ra来改变stackSize
            processSpMove(-stackSize);
           // redirectStdForStackBody();
            // enable Info
            
            _PRECONDITION(top,"Function Info Not Recorded!?");

            //需要维护传参
            
        }
        virtual void endFunction(const koopa_raw_function_t& function)override{
            _INTER("Function end")
            //跳过函数声明
            if(function->bbs.len!=0){
                // auto codeBlock=collectCodeWhenRet();
                // int stackSize=getStackFrameSize();
                // if(hasmarkedRetPosition){
                //     //should at least met one time end_part
                //     std::cout <<codeBeforeRet;
                //     processSpMove(stackSize);
                //     if(!(top->calledFunc.empty())){
                //         processRegLdImm12(-4,"ra");
                //     }
                //     std::cout << "  ret\n";
                //     std::cout << codeBlock ;
                // }else{
                //     std::cout << codeBlock ;
                //     processSpMove(stackSize);
                //     if(!(top->calledFunc.empty())){
                //         processRegLdImm12(-4,"ra");
                //     }
                //     std::cout << "  ret\n";
                // }
                
            }
            //disable Info 
            top = nullptr;
            endGlobal();
        }


        void startBlock(const koopa_raw_basic_block_t &bb)  override{
            _INTER("basicBlock start")
            //label gen here
            if(std::strncmp(bb->name+1, "entry", 5)){
                //first label %entry ignored
                std::cout <<"\n"<< bb->name+1 << ":\n" ;
            }
    
        }
        virtual void endBlock(const koopa_raw_basic_block_t& bb)override{
            _INTER("basicBlock end")
            //mark the ret position
            //we add end part label with suffix
        
            // if( (top->withEndPart&& !std::strncmp(bb->name+1, "end_part",8))|| (!top->withEndPart&& bb==top->blockWithRetOptional)){
            //     //ret is in this block!

            //     //mark! this place is for ret
            //     //collect code before

            //     // still contains problem when in simple mod without end_part_label
            //     hasmarkedRetPosition=true;
            //     //先切断 缓存从开头到ret的code
            //     codeBeforeRet=collectCodeWhenRet();
            //     //继续收集 直到全部跑完，
            //     redirectStdForStackBody();
                
            // }
        }

        //simple policy: 所有运行用的寄存器都用a0 a0
        //under this policy, return only need to load ret.val
        void process(const koopa_raw_return_t &ret)  override{
            _INTER("return")
            if(ret.value){
                processRegload(ret.value,"a0");
            }
            //use preRead info 
            processSpMove(getStackFrameSize());
            if(!(top->calledFunc.empty())){
                processRegLdImm12(-4,"ra");
            }
            std::cout << "  ret\n";
            
            //no need to store something,because this is going to return!
        }
        void process(const koopa_raw_integer_t &integer,const std::string& reg)  override{
             _INTER("integer")
            std::cout <<"  li "<< reg << ", " << integer.value <<"\n";
        }
        virtual void process(const koopa_raw_load_t & load,const koopa_raw_value_t& value) override{
            _INTER("load")
            processRegload(load.src,"t0");
            
            PointerInfo* loadInfo =getVPAInfo(load.src);
            _DEBUG(std::string("source name ")+load.src->name)
            loadInfo->toString();
            if(!loadInfo->isDataDirectlySavedOrAPointerReference()){
        
                //从指针变量中载入数据
                //force let value pointer = 0;
                std::cout << "  lw t0, 0(t0)" << std::endl;
            }
            // a = load p
            // a = load *p
            //save result to a
            //a is not a pointer
            processRegSave(value,"t0",false);
        }
        // void processRegSave(const koopa_raw_value_t& value,const  std::string& reg){
        //     PointerInfo* info = getVPAInfo(value);
        //     AbstractKoopaRawProgramProcessor::processRegSave(value,reg,info->isPointer());
        // }
        virtual void process(const koopa_raw_store_t & store) override{
            _INTER("store")
            
            processRegload(store.value,"t0");
            //
            PointerInfo* info = getVPAInfo(store.dest);
            //if some one save store 1 *p;
            //then when declare *p ,*p is pointer
            //should
            // load *p into t0 and store to 0(t0)
            processRegSave(store.dest,"t0",!info->isDataDirectlySavedOrAPointerReference());
            
           
        }
        virtual void process(const koopa_raw_binary_t& binary,const koopa_raw_value_t& value) override{
            _INTER("binary")
            processRegload(binary.lhs,"t0");
            processRegload(binary.rhs,"t1");
            processBiOp(binary,"t0","t0","t1");
            
            processRegSave(value,"t0",false);
        }
        virtual void allocate(const koopa_raw_value_t& alloc)override{
            //  std::cerr<<"debuging alloc\n";
            _INTER("alloc")
            allocateOnStack(alloc);
            //use value->name to find variable name
            
            // // loc[value] = std::to_string(stack_frame_used) + "(sp)";
            // // stack_frame_used += 4;
            // _ERROR("abort")
        }
        virtual void process(const koopa_raw_branch_t& branch) override{
            _INTER("branch")
            processRegload(branch.cond,"t0");
            std::cout << "  bnez t0, " << branch.true_bb->name+1 <<"\n";
            std::cout << "  j " << branch.false_bb->name+1 << "\n";

            //branch instruction
        }
        virtual void process(const koopa_raw_jump_t& jump) override{
            _INTER("jump")
            std::cout << "  j " << jump.target->name+1 << "\n";
        }
         virtual void process(const koopa_raw_call_t& call,const koopa_raw_value_t& value) override{
            _INTER("call");
            for (size_t i = 0; i < call.args.len; ++i) {
                auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
                if (i < 8) {
                    processRegload(arg ,"a"+std::to_string(i));
                }
                else {
                    processRegload(arg, "t0");
                    std::cout << "  li t6, " << (i - 8) * 4 << std::endl;
                    std::cout << "  add t6, t6, sp" << std::endl;
                    std::cout << "  sw t0, 0(t6)" << std::endl;
                }
            }
            std::cout << "  call " << call.callee->name+1 << std::endl;
            if(value->ty->tag != KOOPA_RTT_UNIT) {
                processRegSave(value,"a0",false);
            }
        }
        void dfs_aggregate(const koopa_raw_value_t& value) {
            //可以优化！word太多
            if(value->kind.tag == KOOPA_RVT_INTEGER) {
                std::cout << "  .word " << value->kind.data.integer.value << std::endl;
            }
            else if(value->kind.tag == KOOPA_RVT_AGGREGATE) {
                const auto& agg = value->kind.data.aggregate;
                for(int i = 0; i < agg.elems.len; i++) {
                    dfs_aggregate(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
                }
            }
        }
        virtual void process(const koopa_raw_global_alloc_t& global,const koopa_raw_value_t& value) override {
            nextGlobal();
            std::cout << "  .data" << std::endl;
            std::cout << "  .globl " << value->name+1 << std::endl;
            std::cout << value->name+1 << ":" << std::endl;
            if (global.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
                auto base = value->ty->data.pointer.base;
                if (base->tag == KOOPA_RTT_INT32){
                    std::cout << "  .zero 4" << std::endl;
                }
                else if (base->tag == KOOPA_RTT_ARRAY) {
                    int zeromem = 4;
                    while (base->tag ==KOOPA_RTT_ARRAY)
                    {
                        zeromem *= base->data.array.len;
                        base = base->data.array.base;
                    }
                    std::cout << "  .zero " << zeromem << std::endl;
                }
                //std::cout << "  .zero 4" << std::endl;
            }
            else if (global.init->kind.tag == KOOPA_RVT_INTEGER) {
                
                std::cout << "  .word " << global.init->kind.data.integer.value << std::endl;
            }else if (global.init->kind.tag == KOOPA_RVT_AGGREGATE)
            {
                _DEBUG("AGGREGATE")
                dfs_aggregate(global.init);
                /* code */
            }
            
            endGlobal();
        }

        virtual void process(const koopa_raw_get_elem_ptr_t& getElem,const koopa_raw_value_t& value) override{
            //getElem
            doPointerIssue(getElem.src,getElem.index,value);
        }   
        void doPointerIssue(const koopa_raw_value_t& source,const koopa_raw_value_t& index,const koopa_raw_value_t& value){
            _INTER("Pointer issue")
            PointerInfo* info = getVPAInfo(source);
            //为什么是pointer呢
            //因为这里使用array载入的时候也是用这个方法
            //但是array load Addr出来就是我们要的
            //pointer是存的地址
            info->toString();
            if(!info->isDataDirectlySavedOrAPointerReference()){
                //指针取值
                processRegload(source,"t0");
                //std::cout << "  lw t0, 0(t0)" << std::endl;
                //看底下
                //假设地址为(sp+12) 存储的值为sp+8
                //现在t0的值为sp+12
                //这里将sp+12的值载入 因此 最终t0应当为sp+8;
            }else{
                 processRegLoadAddr(source,"t0");
            }
            int gap = info->pointerSizeOf();
            processRegload(index,"t1");
            //从指针地址高到低申请地址
            std::cout << "  li t6, "<< gap*(4) << std::endl;
            std::cout << "  mul t6, t6, t1" << std::endl;
            std::cout << "  add t0, t0, t6" << std::endl;
            //把新地址的值存到新建地址
            // 例如 save (sp+8) into (sp+12)
            processRegSave(value,"t0",false);
        }
        virtual void process(const koopa_raw_get_ptr_t& getPtr,const koopa_raw_value_t& value) override{
           //getPtr
            doPointerIssue(getPtr.src,getPtr.index,value);
            //source should be a value
        
        }
        
        void processRegLdImm12(int offset,const std::string& reg){
            if(offset<2048&&offset>-2048){
                std::cout << "  lw " << reg << ", " << offset<<"(sp)" << std::endl;
            }else{
                std::cout << "  li t6, " << offset << std::endl;
                std::cout << "  add t6, t6, sp" << std::endl;
                std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
            }
        }
        void processRegLdImm12Addr(int offset,const std::string& reg){
             if(offset>2048){
                std::cout << "  li " << reg << ", " << offset << std::endl;
                std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
            }else{
                std::cout << "  addi " <<reg << ", sp, " << offset << std::endl;
            }
        }
        void processRegSwImm12(int offset,const std::string& reg,bool isPointer){
            if(offset<2048&&offset>-2048){
                if(isPointer){
                    std::cout << "  lw " << "t6" <<", " << offset  <<"(sp)" <<std::endl;
                    std::cout << "  sw " << reg << ", 0(" << "t6" << ")" << std::endl;
                }else{
                    std::cout << "  sw " << reg << ", " << offset<<"(sp)" << std::endl;
                }
                
            }else{
                std::cout << "  li t6, " << offset << std::endl;
                std::cout << "  add t6, t6, sp" << std::endl;
                if(isPointer){
                    std:: cout << "  lw t6, 0(t6)" << std::endl;
                }
                std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
            }
        }
       

        //我们这边真实分配的时候不仅仅采用上面preRead的模式,我们变量从大向小分配,参数从小向大分配(call)
        //上面的preRead只是一个粗略的计算
        virtual void processLwReg(const koopa_raw_value_t& value,const std::string & reg) override{
            auto addr=getAddrOnStack(value);
            _PRECONDITION(static_cast<bool>(addr),"value not declared on stack!");
            //std::cout << "  lw " << reg << ", " << *addr << std::endl;
            processRegLdImm12(getAllocStackFrameSize()-*addr,reg);
        }
         virtual void processLwRegAddr(const koopa_raw_value_t& value,const std::string & reg)override{
            auto addr = getAddrOnStack(value);
            _PRECONDITION(static_cast<bool>(addr),"value not declared on stack!");
            //addr is offset to sp
           processRegLdImm12Addr(getAllocStackFrameSize()-*addr,reg);
         }
        virtual void processSwReg(const koopa_raw_value_t& value,const std::string& reg,bool isPointer)override{
            auto addr=getOrAllocate(value);
            processRegSwImm12(getAllocStackFrameSize()-*addr,reg,isPointer);
        }
        virtual void processArgLdAddr(const koopa_raw_func_arg_ref_t&value ,const std::string& reg) override{
            //传参都是参数 
            //就算传数组/指针也是传指针的值 需要load 而不是ldAddr
            //应该没有这个操作
            _NOT_IMPLEMENTED("should not called loadPtr to Func Argument")
        }
        virtual void processArgLd(const koopa_raw_func_arg_ref_t&value ,const std::string& reg) override{
            //left as not complete
            if(value.index<8){
                std::cout << "  mv " << reg << ", a" << value.index << std::endl;
            }else{
                //这里需要算上ra的位置
                processRegLdImm12(( getStackFrameSize() + (value.index - 8) * 4),reg);
            }
        }
        
        
        
        void processSpMove(int delta) {
            if(delta==0)return;
            int abs=(delta<0)?-delta:delta;
            if(abs<=2040){
                std::cout << "  addi sp, sp, "<<delta<<"\n";
            }else{
                std::cout << "  li t6, " << delta << std::endl;
                std::cout << "  add sp, sp, t6" << std::endl;

            }
            
        }
        
        
};



InfoPreReadKoopaProgramProcessor* InfoPreReadKoopaProgramProcessor::INSTANCE = new InfoPreReadKoopaProgramProcessor();