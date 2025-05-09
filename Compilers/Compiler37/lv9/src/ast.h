#pragma once

// 所有 AST 的基类
//

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include "debug.h"
#define _STATUS(pred) if(!(pred)){_DEBUG("Invalid status,return");return;}

#define CODE_RET 0
#define CODE_BREAK 1
#define CODE_CONTINUE 2
/*
// 
                        
//   _____                  __
//  /\   _`\               /\ \  
//  \ \ \L\ \  __  __    __\_\ \  
//   \ \  _ < /\ \/\ \  /\  __  \  
//    \ \ \L\ \ \ \_\ \ \ \ \L\  \  
//     \ \____/\ \____ \ \ \_____/  
//      \/___/  \/___/\ \ \/____/
//                   __\ \             
//                  /\ ___\           
//                  \/____/
//
*/


static int uuidCounter=0;
static int genUid(){
    return uuidCounter++;
}
static std::string genNextSymbol(){
    return "%t"+std::to_string(genUid());
}
static int returnCounter=0;
//simple mod still contains bug!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//a

//may fix 
//fix? may!

//计划： 简化refAST 复用 减少load store
//需要在同一个基本块
//问题挺大
//basicd on block
//block label的时候要刷新掉所有ref


static int branchCounter=0;
static int branchIfCounter=0;
static int branchWhileCounter=0;
static int globalCounter=0;
static int functionCounter=0;
class BaseAST;
class DefAST;
class BlockItemAST;
class DeclAST;
class DefAST;
class FuncDefAST;

//here is a problem in old BranchCounter ret method!


template <typename T>
class ValuedObject {
    public:
        bool calculated=false;
        T value;
        virtual T calculateStatic(std::function<T(void)> valSupplier) final{
            if(!calculated){
                value=valSupplier();
                calculated=true;
            }
            return value;
        }
};
template <typename T>
class Catcher {
    public:
    //0 ret
    //we don't use Catcher to deal with ret here,because we have already realize a ret mechanism
    //1 break
    //2 continue
    //else else
        virtual T catchOp(int code) = 0;
};
class Scope {
    public: 

        virtual DefAST* getDefByName(const std::string& definition) =0;
        virtual bool define(const std::string& definition,const DefAST& ast) =0;
        virtual void allocateAddr(const std::string & definition,const std::string & allocAddr) = 0;
        virtual std::string* getAllocateAddr(const std::string& definatino) = 0;
        //是否已经终止
        virtual bool isDeprecate() = 0;
        //终止
        virtual void doDeprecate() = 0;
        //Scope被pop出栈
        virtual void doLeave() = 0;
        virtual void addAbortCatcher(Catcher<std::string>* catcher) = 0;
        virtual std::string handleAbort(int code) = 0;
        virtual FuncDefAST* getFuncDefByName(const std::string& definition) = 0;
        virtual bool define(const std::string& definition,const FuncDefAST& func) = 0;
        Scope(){
           
        }
        virtual ~Scope(){
            
        };
};
class SimpleScope : public Scope{
    public:
        std::unordered_map<std::string,DefAST* > varSymbolMap;
        std::unordered_map<std::string,std::string> addrMap;
        virtual DefAST* getDefByName(const std::string& definition) override{
            auto it = varSymbolMap.find(definition);
            if (it != varSymbolMap.end()) {
                return it->second;  
            }
            return nullptr;
        }
        virtual bool define(const std::string& definition,const DefAST& ast) override{
            auto it =varSymbolMap.find(definition);  
            if (it != varSymbolMap.end()) {
                std::cerr << "Definition '" << definition << "' already exists.\n";
                return false;  
            }

            varSymbolMap[definition] =(DefAST*)&ast;
            return true;  
        }
         virtual void allocateAddr(const std::string & definition,const std::string & allocAddr)override {
            auto it = addrMap.find(definition);
            _PRECONDITION((it == addrMap.end()),"Definition already allocated")
            addrMap[definition]=allocAddr;
         }
        virtual std::string* getAllocateAddr(const std::string& definatino) override{
            return &addrMap[definatino];
        }
        bool deprecated=false;
         virtual bool isDeprecate() override{
            return deprecated;
         }
        //终止
        virtual void doDeprecate() override{
            deprecated=true;
            // originStd =std::cout.rdbuf();
            // std::cout.rdbuf(oss.rdbuf());
            //ignore all codeGen after deprecated
        }
        //Scope被pop出栈
        virtual void doLeave() override{
            // if(deprecated){
            //     std::cout.rdbuf(originStd);
            // }
        }
        std::vector<Catcher<std::string>* > catcherList;
        virtual void addAbortCatcher(Catcher<std::string>* catcher) override{
            catcherList.push_back(catcher);
        }
        virtual std::string handleAbort(int code) override {
            std::string val;
            for (auto i = catcherList.begin(); i !=catcherList.end(); i++)
            {
                val=(*i)->catchOp(code);
                if(!val.empty()){
                    return val;
                }
            }
            return "";
        }
        std::unordered_map<std::string,FuncDefAST* > funcSymbolMap;
        virtual FuncDefAST* getFuncDefByName(const std::string& definition) override{
            auto it = funcSymbolMap.find(definition);
            if (it != funcSymbolMap.end()) {
                return it->second;  
            }
            return nullptr;
        }
        virtual bool define(const std::string& definition,const FuncDefAST& func) override{
            auto it =funcSymbolMap.find(definition);  
            if (it != funcSymbolMap.end()) {
                std::cerr << "Definition '" << definition << "' already exists.\n";
                return false;  
            }

            funcSymbolMap[definition] =(FuncDefAST*)&func;
            return true;  
        }
};

class ScopeStack {
    public: 
        std::vector<std::unique_ptr<Scope> > stack; 
        //std::function<Scope(void)> scopeSuppiler;
        std::unordered_map<std::string,int> sameDefNameCnt;
        //
        ScopeStack(){
            stack=std::vector<std::unique_ptr<Scope> > ();
        }
        virtual int size(){
            return stack.size();
        }
        virtual void push() final {
            stack.push_back(std::unique_ptr<Scope>(new SimpleScope()));
        }
        virtual void pop() final{
            std::unique_ptr<Scope>& leave = stack.back();
            _NONNULL(leave);
            leave->doLeave();
            stack.pop_back();
            
        }
        virtual DefAST* getDefByName(const std::string& definition){
            _PRECONDITION(status(),"Invalid scope statuc")
            for(auto i=stack.rbegin();i!=stack.rend();++i){
                std::unique_ptr<Scope>& scope=*i;
                DefAST* findVal=scope->getDefByName(definition);
                if(findVal){
                    return findVal;// std::make_unique<BaseAST>(*findVal);
                }
            }
             _ERROR("NULL")
            return nullptr;
        }
        virtual FuncDefAST* getFuncDefByName(const std::string& definition){
            _PRECONDITION(status(),"Invalid scope statuc")
            for(auto i=stack.rbegin();i!=stack.rend();++i){
                std::unique_ptr<Scope>& scope=*i;
                FuncDefAST* findVal=scope->getFuncDefByName(definition);
                if(findVal){
                    
                    return findVal;// std::make_unique<BaseAST>(*findVal);
                }
            }
             _ERROR("NULL")
            return nullptr;
        }
        virtual bool defineAtTop(const std::string& definition,const DefAST& ast){
            _PRECONDITION(status(),"Invalid scope statuc")
            assert(size());
            return (*stack.rbegin())->define(definition,ast);
        }
        virtual bool defineAtTop(const std::string& definition,const FuncDefAST& ast){
            _PRECONDITION(status(),"Invalid scope statuc")
            assert(size());
            return (*stack.rbegin())->define(definition,ast);
        }
        virtual std::string* allocAtTop(const std::string& definition){
            _PRECONDITION(status(),"Invalid scope statuc")
            (*stack.rbegin())->allocateAddr(definition,genDefinitionAddr(definition));
            sameDefNameCnt[definition]++;
            return (*stack.rbegin())->getAllocateAddr(definition);
        }
        std::string genDefinitionAddr(const std::string& definition){
            int uid=sameDefNameCnt[definition];
            return std::string("@v_")+definition+((uid==0)?"":( "_"+std::to_string(uid)));
        }
        virtual std::string* allocAtTop(const std::string& definition,const std::string& customAddr){
            _PRECONDITION(status(),"Invalid scope statuc")
            sameDefNameCnt[definition]++;
            (*stack.rbegin())->allocateAddr(definition,customAddr);
            return (*stack.rbegin())->getAllocateAddr(definition);
        }

        virtual std::string* getAllocAddr(const std::string& definition){
            _PRECONDITION(status(),"Invalid scope statuc")
            for(auto i=stack.rbegin();i!=stack.rend();++i){
                std::unique_ptr<Scope>& scope=*i;
                std::string* findVal=scope->getAllocateAddr(definition);
                if(findVal){
                    return findVal;// std::make_unique<BaseAST>(*findVal);
                }
            }
            _ERROR("NULL")
            return nullptr;
        }
        //保证每个block中仅有一条控制流语句执行
        //当执行后 后续的控制流均作废
        
        // bool doScopeControlLeave(){
        //     std::unique_ptr<Scope>& scope=*stack.rbegin();
            
        //     if(scope->isDeprecate()){
        //         return false;
        //     }else{
        //         scope->doDeprecate();
        //         return true;
        //     }
        // }
        bool hasReturn =false;
        void doReturn(){
            hasReturn = true;
        }
        bool hasJump = false;
        void doJump(){
            //哦 我的上帝啊 我想,当一个jump被生成的时候,这想必应该是到下个label为止都不会有jump了
            hasJump = true;
        }
        void nextLabel(){
            //哦 我的上帝啊 瞧瞧这是什么,一个新的label
            hasJump =  false;
        }
        void nextGlobal(){
            hasJump=false;
            hasReturn=false;
        }
        //stmt should check control status before gen
        bool status(){
            return !hasReturn && !hasJump;//!((*stack.rbegin())->isDeprecate());
        }
        void registerAbortCatcher(Catcher<std::string>* catcher){
            //I can add catch-throw here
            (*(stack.rbegin()))->addAbortCatcher(catcher);
        }
        std::string handleAbortJump(int code){
            std::string val;
            for(auto i=stack.rbegin();i!=stack.rend();++i){
                val = (*i)->handleAbort(code);
                if(!val.empty()){
                    return val;
                }
            }
            //no way : write sth like int main(){break;}
            
            _LOGIC("Unhandled Abort Message:"+std::to_string(code))
        }
        void allocStatic(){
            //static content while running
        }
};

class BaseAST :public ValuedObject<int> {
    public:
        
        int uuid;
        //this method is for asking whether the val of AST can be calculated by compiler
        //only AST with Expression will override this
        
        //this method is for calculating const value, after calling this, const value should be accessed by getVarName()
        std::string className;
        virtual int calculateIntConst(ScopeStack& stack)  final{
            assert(isConstVal(stack));
            return ValuedObject<int>::calculateStatic([this,&stack](){
                return calculateInt(stack);
            });
        }
        virtual bool isConstVal(ScopeStack& stack) {
            return false;
        }
        //called for initializing Global vars
        virtual bool isGlobalVal(ScopeStack& stack){
            return false;
        }
        virtual int calculateIntGlobal(ScopeStack& stack){
            assert(isGlobalVal(stack));
            return ValuedObject<int>::calculateStatic([this,&stack](){
                return calculateInt(stack);
            });
        }
        virtual int calculateInt(ScopeStack& stack) {
            _NOT_IMPLEMENTED(typeid(*this).name());
        }
        virtual void markChange(ScopeStack& stack,std::unique_ptr<BaseAST>& from){
            _NOT_IMPLEMENTED(typeid(*this).name());
        }
    
        //this method is to get Unique id of AST
        virtual int getUniqueId() const final{
            return uuid;
        }
        //this method is to get VarName of expression which AST represent
        virtual std::string getVarName(ScopeStack& stack) {
            if(isConstVal(stack)){
                return std::to_string(calculateIntConst(stack));
            }
            return "%t"+std::to_string(getUniqueId());
        }
        BaseAST(const std::string& classT):className(classT){
            uuid=genUid();
            //_DEBUG("creating "+className);
    
        }
        virtual std::string getClass() const {
            return "BaseAST";
        }
        virtual ~BaseAST() {
            //_DEBUG(std::string("deconstruct AST ")+std::to_string(uuid))
        }
        static void genJumpCode(const std::string& label,ScopeStack& stack){
            stack.doJump();
            std::cout<<"  jump "<<label<<"\n";
        }
        static void genLabelCode(const std::string& label,ScopeStack& stack){
            stack.nextLabel();
            std::cout<<"\n"<<label<<":\n";
        }
        
        virtual void toString()  = 0;
        virtual void toKoopaString(ScopeStack& stack) =0;
};
class BlockItemAST : public BaseAST{
    public:
        virtual std::string getClass() const override{
            return "BlockItemAST";
        }
        BlockItemAST(const std::string& classT):BaseAST(classT){
            
        }
        BlockItemAST* next;
        BlockItemAST* parent;
        bool requestLabel;
        bool overrideNext=false;
        std::string overrideNextLabel;
        void setNext(BlockItemAST* nextItem){
            next=nextItem;
        }
        void requestNextOverride(const std::string& label){
            overrideNext=true;
            overrideNextLabel=label;
        }
        std::string genLabel(){
            requestLabel=true;
            return std::string("%label_")+std::to_string(getUniqueId());
        }
        std::string getNextJumpLabel(){
            //后面还有
            if(overrideNext){
                return overrideNextLabel;
            }
            if(next){
                return next->genLabel();
            }else{
                //todo 这玩意有问题 当嵌套了多重block的时候 next会被搞混 需要维护
                //只有最外层应当被定向为。。。
                //需要设置一个这个
                //upper 标记自己属于的BlockList块 
                if(!parent){
                    return "%end_part_"+std::to_string(functionCounter);
                }else{
                    return parent->getNextJumpLabel();
                }
                
            }
        }
        std::string getVarName(ScopeStack& stack) override{
            return "%t"+std::to_string(getUniqueId());
        }
        //gen label when request before
        void toBlockItem(ScopeStack& stack){
            if(requestLabel){
                //mark this
                genLabelCode(genLabel(),stack);
            }
            //move status check ,so can status refresh at genLabel()
            _STATUS(stack.status())

            toKoopaString(stack);
            //so you know what? someone quit between toKoopaString,
            //jump code should be ignored
            _STATUS(stack.status());
            //到目目前都还没跳
            if(overrideNext){
                genJumpCode(overrideNextLabel,stack);
            }else if(next&&next->requestLabel){
                //如果下一个blockItem被生成label了，上一个要自动跳过去
                genJumpCode(next->genLabel(),stack);
                //if
            }else if(!next&&!parent){//存在没有next的 但是不存在没next且没parent的
                //最上层
                //这他妈是什么修复操作
                //补全默认没有的ret
                //不是这bug怎么自己好了
                //这合理吗？？？？？？？？？？？？？？？？？
                if(branchCounter>0){
                    genJumpCode("%end_part_"+std::to_string(functionCounter),stack);
                }else{
                    std::string defaultJump=stack.handleAbortJump(CODE_RET);
                    if(defaultJump=="__void_return__"){
                        std::cout<<"  ret\n";
                    }else{
                        //你猜怎么着，void
                        std::cout<<"  ret 0\n";
                    }
                    stack.doReturn();
                }
                
            }
        }

};


class CompItemAST : public BaseAST {
    public:
        CompItemAST():BaseAST("CompItemAST"){

        }
        virtual std::string getClass() const override{
            return "CompItemAST";
        }
        std::unique_ptr<BaseAST> item;
        void toString()  override {
            _INTER("CompItem");
            std::cout << "CompItemAST { ";
            _NONNULL(item);
            item->toString();
            std::cout << " }\n";
        }
        void toKoopaString(ScopeStack& stack)  override{
            _INTER("CompItem");
            //CompItem是global定义 如function和global var
            stack.nextGlobal();
            _NONNULL(item);
            item->toKoopaString(stack);
        }
};

// CompUnit 是 BaseAST


//BlockAST
class BlockAST : public BlockItemAST{
    public:
        BlockAST():BlockItemAST("BlockAST"){

        }
            virtual std::string getClass() const override{
            return "BlockAST";
        }
        std::unique_ptr< std::vector<std::unique_ptr<BlockItemAST> > > item;
        void toString() override{
            _INTER("BlockAST");
            std::cout << "BlockAST { ";
            std::for_each(item->begin(),item->end(), [](const std::unique_ptr<BlockItemAST>& item) {
                _NONNULL(item);
                item->toString(); //
                std::cout << " ,";
            });
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("BlockAST");
            
            //enter a code block

            stack.push();
            std::for_each(item->begin(),item->end(), [this](const std::unique_ptr<BlockItemAST>& item) {
                _NONNULL(item)
                item->parent=this; //
            });
            std::for_each(item->begin(),item->end(), [&stack](const std::unique_ptr<BlockItemAST>& item) {
                _NONNULL(item)
        
                item->toBlockItem(stack); //
            });
            //quit a code block

            stack.pop();
            _DEBUG("exit Block");
        }
};


class StmtAST : public BlockItemAST{
    public:
        virtual std::string getClass() const override{
            return "StmtAST";
        }
        StmtAST(std::string op):BlockItemAST("StmtAST"),operation(op){

        }
        std::unique_ptr<BaseAST> lval;
        std::unique_ptr<BaseAST> exp;
        std::string operation;
        virtual void toString() override{
            _INTER("StmtAST")
        
            std::cout << "StmtAST { code: "+operation+", exp:";
            if(exp){
                exp->toString();
            }
            if(lval){
                std::cout<<", lval:";
                lval->toString();
            }
            std::cout << " }";
        }
    
};
class ReturnStmtAST : public StmtAST {
    public:
        virtual std::string getClass() const override{
            return "ReturnStmtAST";
        }
        ReturnStmtAST():StmtAST("return exp"){
            lval=nullptr;
            returnCounter++;
        }
        virtual void toKoopaString(ScopeStack& stack)  override{
            _INTER("ReturnStmt")
            _STATUS(stack.status())
            _NONNULL(exp)
            exp->toKoopaString(stack);
            if(branchCounter==0){
                std::string returnedLabel=stack.handleAbortJump(CODE_RET);
                if(returnedLabel=="__void_return__"){
                    std::cout << "  ret\n";
                }else{
                    std::cout << "  ret "<<exp->getVarName(stack)<<"\n";
                }
                stack.doReturn();
            }else{
                std::string returnedLabel=stack.handleAbortJump(CODE_RET);
                if(returnedLabel=="__void_return__"){
                    std::cout << "  jump %end_part_"<<functionCounter<<"\n";
                }else{
                    std::cout << "  store "<<exp->getVarName(stack)<<", "<< returnedLabel << "\n  jump %end_part_"<<functionCounter<<"\n";
                }
                stack.doJump();
            }
            
            
            
            
        }
};
class EmptyReturnStmtAST : public StmtAST {
    public: 
        virtual std::string getClass() const override{
            return "EmptyReturnStmtAST";
        }
        EmptyReturnStmtAST():StmtAST("return;"){
            returnCounter++;
        }
        virtual void toKoopaString(ScopeStack& stack) override {
            _INTER("EmptyReturnStmt")
            _STATUS(stack.status())
            std::string returnedLabel=stack.handleAbortJump(CODE_RET);
            if(returnedLabel=="__void_return__"){
                if(branchCounter==0){
                    std::cout << "  ret\n";
                    stack.doReturn();
                }else{
                    std::cout << "  jump %end_part_"<<functionCounter<<"\n";
                    stack.doJump();
                }
                
            }else{
                if(branchCounter==0){
                    std::cout << "  ret 0\n";
                    stack.doReturn();

                }else{
                    std::cout << "  store 0, "<< returnedLabel << "\n  jump %end_part_"<<functionCounter<<"\n";
                    stack.doJump();
                }
            }
            
            
            
          
        }
};
class AssignStmtAST : public StmtAST {
    public:
        virtual std::string getClass() const override{
            return "AssignStmtAST";
        }
        AssignStmtAST():StmtAST("lval=exp"){

        }
        virtual void toKoopaString(ScopeStack& stack) override{
            _INTER("AssignStmt")
            _STATUS(stack.status())
            _NONNULL(exp)
            _NONNULL(exp)
            exp->toKoopaString(stack);
            lval->markChange(stack,exp);
            //
        }
};
//doing nothing between two ;; or sth
class BlankStmtAST : public StmtAST {
    public:
        virtual std::string getClass() const override{
            return "BlankStmtAST";
        }
        BlankStmtAST():StmtAST(";"){

        }
        virtual void toKoopaString(ScopeStack& stack) override{
            _INTER("BlankStmt")
            _STATUS(stack.status())
            
            //do nothing
        }
};
class ExpressionStmtAST : public StmtAST {
    public :
        virtual std::string getClass() const override{
            return "ExpressionStmtAST";
        }
        ExpressionStmtAST():StmtAST("exp;"){

        }
        virtual void toKoopaString(ScopeStack& stack) override{
            _INTER("ExpressionStmt")
            _STATUS(stack.status())
            _NONNULL(exp)
            exp->toKoopaString(stack);
            //
            //todo: left not done
        }
};

class BranchIfStructureAST : public BlockItemAST {
    public: 
        virtual std::string getClass() const override{
            return "BranchIfStructureAST";
        }
        BranchIfStructureAST():BlockItemAST("BranchIfStructureAST"){
            //mark as have branch
            branchCounter++;
            branchIfCounter++;
        }
        std::unique_ptr<BaseAST> exp;
        std::unique_ptr<BlockItemAST> branchIf;
        std::unique_ptr<BlockItemAST> branchElse;
        void toString() override {
            _INTER("BranchIf")
            std::cout << "BranchIfStructureAST { ";
            _NONNULL(exp)
            _NONNULL(branchIf)
            exp->toString();
            branchIf->toString();
            if(branchElse){
                branchElse->toString();
            }
            std::cout << " }";
        }
        // std::string genIfLabel(ScopeStack& stack){
        //     return getVarName(stack)+ std::string("_if");
        // }
        // std::string genElseLabel(ScopeStack& stack){
        //     return getVarName(stack)+ std::string("_else");
        // }
        //left as not complete
        void toKoopaString(ScopeStack& stack) override{
             _INTER("BranchIf")
             _NONNULL(exp)
             _NONNULL(branchIf)
            exp->toKoopaString(stack);
            std::string nextLabel=getNextJumpLabel();
            std::string ifLabel=branchIf->genLabel();

            if(branchElse){
                std::string elseLabel=branchElse->genLabel();
                std::cout<<"  br "<<exp->getVarName(stack)<<", "<<ifLabel<<", "<<elseLabel<<"\n";
            }else{
                std::cout<<"  br "<<exp->getVarName(stack)<<", "<<ifLabel<<", "<<nextLabel<<"\n";
            }
            //if.next=this.next
            branchIf->requestNextOverride(nextLabel);
            branchIf->parent=this;
            stack.push();
            branchIf->toBlockItem(stack);
            stack.pop();
            if(branchElse){
                branchElse->requestNextOverride(nextLabel);
                branchElse->parent=this;
                stack.push();
                branchElse->toBlockItem(stack);
                stack.pop();
            }
        }
};
class BranchWhileStructureAST : public BlockItemAST ,public Catcher<std::string> {
    public:
        virtual std::string getClass() const override{
            return "BranchWhileStructureAST";
        }
        std::unique_ptr<BaseAST> exp;
        std::unique_ptr<BlockItemAST> loopBody;
        //we have to handle break/jp out<-when return ,in case of block scope leak, continue, common leave, loop
        BranchWhileStructureAST():BlockItemAST("BranchWhileStructureAST"){
            //mark as have branch
            branchCounter++;
            branchWhileCounter++;
            genLabel();
        }
        void toString() override {
            _INTER("BranchIf")
            std::cout << "BranchIfStructureAST { ";
            _NONNULL(exp)
            exp->toString();
            loopBody->toString();
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("BranchWhile")
            //_NOT_IMPLEMENTED("not completed yet")
             //label_xxx:
             //we request label gen when ast created
             //     expression
            _NONNULL(exp)
            _NONNULL(loopBody)
            exp->toKoopaString(stack);
            // if expression failed ,goto this.next;
            loopBody->parent=this;
            std::string nextLabel=getNextJumpLabel();
            std::string loopLabel=loopBody->genLabel();
            std::cout<<"  br "<<exp->getVarName(stack)<<", " <<loopLabel<<", "<<nextLabel<<"\n";
            loopBody->requestNextOverride(genLabel());
            stack.push();
            stack.registerAbortCatcher(this);
            loopBody->toBlockItem(stack);
            stack.pop();
            // exp->toKoopaString(stack);
            // std::string nextLabel=getNextJumpLabel();
            // std::string ifLabel=branchIf->genLabel();

            // if(branchElse){
            //     std::string elseLabel=branchElse->genLabel();
            //     std::cout<<"  br "<<exp->getVarName(stack)<<", "<<ifLabel<<", "<<elseLabel<<"\n";
            // }else{
            //     std::cout<<"  br "<<exp->getVarName(stack)<<", "<<ifLabel<<", "<<nextLabel<<"\n";
            // }
            // //if.next=this.next
            // branchIf->requestNextOverride(nextLabel);
            // branchIf->toBlockItem(stack);
            // if(branchElse){
            //     branchElse->requestNextOverride(nextLabel);
            //     branchElse->toBlockItem(stack);
            // }
        }
        std::string catchOp(int code) override{
            switch (code)
            {
                case CODE_BREAK:
                //break,jump to this.next
                    return getNextJumpLabel();
                case CODE_CONTINUE:
                //continue, jump to this.start
                    return genLabel();
                default:
                    return "";
            }
        }
};
class AbortStmtAST : public StmtAST{
    public:
        int mode;
        AbortStmtAST():StmtAST("Abort"){

        }
        void toKoopaString(ScopeStack& stack) override {
            std::string label;
            label=stack.handleAbortJump(mode);
            genJumpCode(label,stack);
        }
};
// ...

//exp
//Exp         ::= AddExp;
class ExpAST :public BaseAST{
    public:
        ExpAST():BaseAST("ExpAST"){

        }
        virtual std::string getClass() const override{
            return "ExpAST";
        }
    //should store a UnaryExpAST
        std::unique_ptr<BaseAST> addExp;
        //now this class is just a direct reference to unaryExp,so we redirect varName to unaryExp
        virtual bool isConstVal(ScopeStack& stack) override{
            _NONNULL(addExp)
            return addExp->isConstVal(stack);
        }
        virtual int calculateInt(ScopeStack& stack)  override {
            _NONNULL(addExp)
            return addExp->calculateInt(stack);
        }
        std::string getVarName(ScopeStack& stack)  override{
            _NONNULL(addExp)
            return addExp->getVarName(stack);
        }
        void toString()  override{
            _INTER("ExpAST")
            std::cout << "ExpAST { ";
            addExp->toString();
            std::cout << " }";

        }
        void toKoopaString(ScopeStack& stack) override{
             _INTER("ExpAST")
             _NONNULL(addExp)
            addExp->toKoopaString(stack);
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            _NONNULL(addExp)
            return addExp->isGlobalVal(stack);
        }
};

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST{
    public :
        UnaryExpAST():BaseAST("UnaryExpAST"){

        }
        virtual std::string getClass() const override{
            return "UnaryExpAST";
        }
    //when stored 0, should use PrimaryEXP
        //UnaryOp     ::= "+" | "-" | "!";
        char unaryOp;
        //based on unaryOp, store PrimaryEXPAST or sub UnaryExp
        std::unique_ptr<BaseAST> subExp;
        virtual bool isConstVal(ScopeStack& stack) override{
            return subExp->isConstVal(stack);
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            return subExp->isGlobalVal(stack);
        }
        virtual int calculateInt(ScopeStack& stack) override {
            _NONNULL(subExp)
            int val=subExp->calculateInt(stack);
            switch (unaryOp)
            {
                case '-':
                    /* code */
                    return -val;
                case '!':
                    return !val;
                default:
                    return val;
            }
        }
        std::string getVarName(ScopeStack& stack)  override{
            //option which do nothing
            if(unaryOp==0||unaryOp=='+'){
                return subExp->getVarName(stack);
            }else{
                return BaseAST::getVarName(stack);
            }
        
        }
        void toString() override{
            _INTER("UnaryExpAST");
            std::cout << "UnaryExpAST { op:" << unaryOp << " " ;
            subExp->toString();
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack)override{
            _INTER("UnaryExpAST");
            if(isConstVal(stack)){
                return ;
            }
            _NONNULL(subExp)
            subExp->toKoopaString(stack);
            switch (unaryOp)
            {
                case '-':
                    /* code */
                    std::cout << "  " << getVarName(stack) << " = " << "sub 0, " << subExp->getVarName(stack) <<"\n";
                    break;
                case '!':
                    std::cout << "  " << getVarName(stack) << " = " << "eq 0, " << subExp->getVarName(stack) <<"\n";
                    break;
                default:
                    break;
            }
        
        }
};

// //PrimaryExp  ::= "(" Exp ")" | Number;
// //ignore ()
// // store Num
// class PrimaryExpAST : public BaseAST{
//     public :
//     //store Exp or Number
//     //Number should be an BaseAST
//         std::unique_ptr<BaseAST> valueExp;
//         std::string getVarName() override{
//             return valueExp->getVarName();
//         }
//         void toString() override{
//             std::cout << "PrimaryExpAST { ";
//             valueExp->toString();
//             std::cout << " }";
//         }
//         void toKoopaString() override{
//             valueExp->toKoopaString();
//         }
// };
//Number      ::= INT_CONST;     
class NumberAST : public BaseAST {
    public:
        NumberAST():BaseAST("NumberAST"){

        }
    //from 0 to 2^31-1
        virtual std::string getClass() const override{
            return "NumberAST";
        }
        int intValue;
        virtual bool isConstVal(ScopeStack& stack) override{
            return true;
        }
        virtual int calculateInt(ScopeStack& stack) override{
            return intValue;
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            return false;
        }
        std::string getVarName(ScopeStack& stack)  override{
            return std::to_string(intValue);
        }
        void toString() override{
            _INTER("NumberAST")
            std::cout << "NumberAST { " << intValue << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("NumberAST")
        }
};


//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
//MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class BiOpExpAST : public BaseAST{
    public :
        BiOpExpAST ():BaseAST("BiOpExpAST "){

        }
        virtual std::string getClass() const override{
            return "BiOpExpAST";
        }
    //0 + - * / % > <
    //logic op remapping:
    // 'l' <=
    // 'g' >=
    // 'e' ==
    // 'n' !=
    
        char op;
        //nonnull
        std::unique_ptr<BaseAST> val1;
        //null if op==0
        std::unique_ptr<BaseAST> val2;
        virtual bool isConstVal(ScopeStack& stack) override{
            _NONNULL(val1)
    
            return val1->isConstVal(stack)&&(op==0||val2->isConstVal(stack));
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            _NONNULL(val1)
            return val1->isGlobalVal(stack)&&val2->isGlobalVal(stack);
        }
        virtual int calculateInt(ScopeStack& stack) override {
            _NONNULL(val1)
            
            if(op==0){
                return val1->calculateInt(stack);
            }else{_NONNULL(val2)
                int c1=val1->calculateInt(stack);
                int c2=val2->calculateInt(stack);
                switch (op)
                {
                    case '-':
                        /* code */
                        return c1-c2;
                    case '+':
                        return c1+c2;
                    case '*':
                        /* code */
                        return c1*c2;
                    case '/':
                        return c1/c2;
                    case '%':
                        return c1%c2;
                    case '<':
                        return c1<c2;
                    case '>':
                        return c1>c2;
                        //lesser and eq
                    case 'l':
                        return c1<=c2;
                        //greater and eq
                    case 'g':
                        return c1>=c2;
                        //eq
                    case 'e':
                        return c1==c2;
                        //not eq
                    case 'n':
                        return c1!=c2;
                        //and
                    default:
                        _NOT_IMPLEMENTED(std::string(1,op));
                }
            }
        }
        std::string getVarName(ScopeStack& stack)  override{
            //option which do nothing
            if(op==0){
                return val1->getVarName(stack);
            }else{
                return BaseAST::getVarName(stack);
            }
        }
        void toString()  override{
            _INTER("BiOpExpAST");
            std::cout << "BiOpExpAST { " ;
            if( op==0){
                val1->toString();
            }else{
                val1->toString();
                std::cout << ", " << op << ", ";
                val2->toString();
            }
            std::cout<< " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("BiOpExpAST");
            if(isConstVal(stack)){
                return ;
            }
            _NONNULL(val1)
            val1->toKoopaString(stack);
            if(op==0){
                return;
            }
            _NONNULL(val2)
            val2->toKoopaString(stack);
            switch (op)
            {   
                
                case '-':
                    /* code */
                    std::cout << "  " << getVarName(stack) << " = " << "sub " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                case '+':
                    std::cout << "  " << getVarName(stack) << " = " << "add " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                case '*':
                    /* code */
                    std::cout << "  " << getVarName(stack) << " = " << "mul " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                case '/':
                    std::cout << "  " << getVarName(stack) << " = " << "div " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                case '%':
                    std::cout << "  " << getVarName(stack) << " = " << "mod " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                case '<':
                    std::cout << "  " << getVarName(stack) << " = " << "lt " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                case '>':
                    std::cout << "  " << getVarName(stack) << " = " << "gt " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                    //lesser and eq
                case 'l':
                    std::cout << "  " << getVarName(stack) << " = " << "le " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                    //greater and eq
                case 'g':
                    std::cout << "  " << getVarName(stack) << " = " << "ge " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                    //eq
                case 'e':
                    std::cout << "  " << getVarName(stack) << " = " << "eq " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                    //not eq
                case 'n':
                    std::cout << "  " << getVarName(stack) << " = " << "ne " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
                    break;
                    //and
                default:
                    break;
            }
        }
};

class LOpExpAST : public BiOpExpAST{
    public:
    
        virtual std::string getClass() const override{
            return "LOpExpAST";
        }
    //
    //complex operation ,new class
    // 'a' &&
    // 'o' ||
    // 'x' xor
        virtual bool isConstVal(ScopeStack& stack) override{
            if(val1->isConstVal(stack)){
                int c1=val1->calculateIntConst(stack);
                if(op=='a'&&(!c1)){
                    //如果是&&操作并且c1为0,则返回false无需计算c2
                    return true;
                }else if(op=='o'&&c1){
                    //如果是||操作且c1为1 则返回true,无需计算c2
                    return true;
                }else if(op==0){
                    
                }else{
                   return val2->isConstVal(stack);
                }
            }
            return false;
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            return val1->isGlobalVal(stack)&&val2->isGlobalVal(stack);
        }
        virtual int calculateInt(ScopeStack& stack)  override{
            if(op==0){
                return val1->calculateInt(stack);
            }else{
                int c1=val1->calculateInt(stack);
                //int c2=val2->calculateIntConst(stack);
                switch (op)
                {
                    case 'a':
                        if(!c1){
                            return false;
                        }else{
                            int c2=val2->calculateInt(stack);
                            return 1&&c2;
                        }
                    case 'o':
                        if(c1){
                            return true;
                        }else{
                            int c2=val2->calculateInt(stack);
                            return 0||c2;
                        }
                    default:
                        _NOT_IMPLEMENTED(std::string(1,op));
                }
            }
        }
        void toString() override{
            _INTER("LOpExpAST")
            std::cout << "LOpExpAST { " ;
            if( op==0){
                val1->toString();
            }else{
                val1->toString();
                std::cout << ", " << op << ", ";
                val2->toString();
            
            }
            std::cout<< " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("LOpExpAST")
            if(isConstVal(stack)){
                return ;
            }
            switch (op)
            {
                case 'a':
                    doAndOperation(stack);
                    break;
                case 'o':
                    doOrOperation(stack);
                    break;
               // case 'x'
                default:
                    break;
            }
            // val1->toKoopaString(stack);
            
            // switch (op)
            // {
            //     case 'a':
            //          val2->toKoopaString(stack);
            //         //not good, and operation 位运算的
            //         doOperation(stack,"and");
            //         // std::cout << "  " << getVarName(stack) << " = " << "and " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
            //         //doAndOperationAfterVal1Calculated();
            //         break;
            //     case 'o':
            //          val2->toKoopaString(stack);
            //         // std::cout << "  " << getVarName(stack) << " = " << "or " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
            //         doOperation(stack,"or");
            //         //doOrOperationAfterVal1Calculated();
            //         break;
            //     // case 'x':
            //     //     val2->toKoopaString();
            //     //     std::cout << "  " << getVarName(stack) << " = " << "xor " << val1->getVarName(stack)<<", "<<val2->getVarName(stack) <<"\n";
            //     //     break;
            //     default:
            //         break;
            // }
        }
        std::string getVarName(ScopeStack& stack)override{
            if(isConstVal(stack)){
                return BiOpExpAST::getVarName(stack);
            }else{
                //
                return BiOpExpAST::getVarName(stack);
            }
        }
    protected:
        virtual std::string genShortLabel(){
            return std::string("%stpath_inr_jmp_")+std::to_string( getUniqueId());
        }
        virtual std::string genSimpleLabel(){
            return std::string("%stpath_inr_nxt_")+std::to_string( getUniqueId());
        }
        virtual std::string genShortEndLabel(){
            return std::string("%stpath_inr_sum_")+std::to_string( getUniqueId());
        }
        virtual std::string* allocInnerVar(ScopeStack& stack){
            return stack.allocAtTop("pathst_inr_var_"+std::to_string( getUniqueId()));
        }
        virtual void doAndOperation(ScopeStack& stack) final{
            //const only if val2 const val1 not 
            if(val2->isConstVal(stack)){
                val1->toKoopaString(stack);
                int c2 = val2->calculateIntConst(stack);
                if(c2){
                    //val&&1
                    std::cout << "  " << getVarName(stack) << " = ne " << val1->getVarName(stack)<<", 0"  <<"\n";
                }else{
                    std::cout << "  " << getVarName(stack) << " = ne 0, 0"  <<"\n";
                }
                
            }else{
                std::string* innerAlloc=allocInnerVar(stack);
                std::cout << "  " << *innerAlloc << " = alloc i32\n";
                std::string shortPath=genShortLabel();
                std::string commonPath=genSimpleLabel();
                std::string end=genShortEndLabel();
                val1->toKoopaString(stack);
                std::cout << "  br "<<val1->getVarName(stack)<< ", "<< commonPath<< ", " <<shortPath <<"\n";
                genLabelCode(commonPath,stack);
                val2->toKoopaString(stack);
                std::string innerVar=genNextSymbol();
                std::cout << "  " << innerVar <<" = ne " << val2->getVarName(stack) << ", 0\n";
                std::cout << "  store " << innerVar << ", " << *innerAlloc <<"\n";
            
                std::cout << "  jump " << end << "\n";
                genLabelCode(shortPath,stack);
                std::cout << "  store 0, " << *innerAlloc <<"\n";
                std::cout << "  jump " << end << "\n";
                genLabelCode(end,stack);
                std::cout << "  " << getVarName(stack) << " = load " << *innerAlloc <<"\n";
                // std::string labelLeft = genNextSymbol();
                // std::string labelRight = genNextSymbol();
            }
            
        }
        virtual void doOrOperation(ScopeStack& stack) final{
            if(val2->isConstVal(stack)){
                val1->toKoopaString(stack);
                int c2 = val2->calculateIntConst(stack);
                if(!c2){
                    // val||0
                    std::cout << "  " << getVarName(stack) << " = ne " << val1->getVarName(stack)<<", 0"  <<"\n";
                }else{
                    std::cout << "  " << getVarName(stack) << " = ne 1, 0"  <<"\n";
                }
                
            }else{
                std::string* innerAlloc=allocInnerVar(stack);
                std::cout << "  " << *innerAlloc << " = alloc i32\n";
                std::string shortPath=genShortLabel();
                std::string commonPath=genSimpleLabel();
                std::string end=genShortEndLabel();
                val1->toKoopaString(stack);
                //
                std::cout << "  br "<<val1->getVarName(stack)<< ", "<< shortPath << ", " << commonPath<<"\n";
                genLabelCode(commonPath,stack);
                val2->toKoopaString(stack);
                std::string innerVar=genNextSymbol();
                std::cout << "  " << innerVar <<" = ne " << val2->getVarName(stack) << ", 0\n";
                std::cout << "  store " << innerVar << ", " << *innerAlloc <<"\n";
            
                std::cout << "  jump " << end << "\n";
                genLabelCode(shortPath,stack);
                std::cout << "  store 1, " << *innerAlloc <<"\n";
                std::cout << "  jump " << end << "\n";
                genLabelCode(end,stack);
                std::cout << "  " << getVarName(stack) << " = load " << *innerAlloc <<"\n";
                // std::string labelLeft = genNextSymbol();
                // std::string labelRight = genNextSymbol();
            }
        }
        virtual void doOperation(ScopeStack& stack,const char* operation)  final{
            std::string labelLeft = genNextSymbol();
            std::string labelRight = genNextSymbol();
            std::cout << "  " << labelLeft << " = " << "ne " << val1->getVarName(stack)<<", 0\n";
            std::cout << "  " << labelRight << " = " << "ne " << val2->getVarName(stack)<<", 0\n";
            std::cout << "  " << getVarName(stack) << " = " << operation<<" " << labelLeft<<", "<<labelRight <<"\n";
        }
        //deprecated
        virtual void doAndOperationAfterVal1Calculated()  final{
            // std::string labelName=std::to_string(genUid());
            // std::cout<<"  br " << val1->getVarName()<<", %val2_"<<labelName<<", %shortcut_"<<labelName<<"\n";
            // std::cout<<"%shortcut_"<<labelName<<":\n  "<<getVarName()<<" = ne 0, 1\n  jump %next_"<<labelName<<"\n";
            // //how to set value???

            // std::cout<<"%val2_"<<labelName<<":\n";
            // //val2->toKoopaString();
            // std::cout<<"  "<<getVarName()<<" = ne 0, "<<val2->getVarName()<<"\n";
            // std::cout<<"%next_"<<labelName<<":\n";
        }
        //deprecated
        virtual void doOrOperationAfterVal1Calculated()  final{
        //     std::string labelName=std::to_string(genUid());
        //     std::cout<<"  br " << val1->getVarName()<<", %shortcut_"<<labelName<<", %val2_"<<labelName<<"\n";
        //     std::cout<<"%shortcut_"<<labelName<<":\n  "<<getVarName()<<" = ne 0, 0\n  jump %next_"<<labelName<<"\n";
        //     //how to set value???

        //     std::cout<<"%val2_"<<labelName<<":\n";
        //    // val2->toKoopaString();
        //     std::cout<<"  "<<getVarName()<<" = ne 0, "<<val2->getVarName()<<"\n";
        //     std::cout<<"%next_"<<labelName<<":\n";
        }
};
class ArrayInitExpAST : public BaseAST {
    //important here 
    public: 
        bool isConstVal(ScopeStack& stack) override{
            return true;
        }
        int calculateInt(ScopeStack& stack) override {
            _NOT_IMPLEMENTED("You shouldn't calculate a Array Initial value")
        }
        std::unique_ptr<std::vector< std::unique_ptr<BaseAST> > > rawInit;
        
        ArrayInitExpAST () :BaseAST("Array"){

        }
        void toString() override {
            std::cout<<"Array{}";
        }
        void toKoopaString(ScopeStack& stack) override{
            _NOT_IMPLEMENTED("Array Initialize Expression shouldn't be initialzed through this method")
        }
        void toArray(std::vector<std::pair<BaseAST*,int > >& collector,const int& totalSize,std::vector<int>& revertSizeStack){
            //implement this initial method
           // _DEBUG("Intering toArray "+std::to_string(revertSizeStack.size()))
            _NONNULL(rawInit)
            //_DEBUG("rawInit "+std::to_string(rawInit->size()))
            if(totalSize<=0)return;
            int sumOfSize=0;
            //enter this
            _PRECONDITION(!revertSizeStack.empty(),"Undefined array definition")
            int topValue= *(revertSizeStack.rbegin());
            revertSizeStack.pop_back();
            for (auto i = rawInit->begin(); i !=rawInit->end(); i++)
            {
                ArrayInitExpAST* isArray;
                if((isArray=dynamic_cast<ArrayInitExpAST*>((*i).get()))){
          
                    
                    //is a recursive array
                    //check {4} ->4 if totalSize =1; this is ok
                    //if this empty without last element,,must NOT be an array
                    _PRECONDITION(!revertSizeStack.empty(),"Undefined array nesting definition")
                    int expectSize=1;
                    std::vector<int> reverseSubArraySizeStack;
                    for (auto i = revertSizeStack.begin(); i < revertSizeStack.end(); i++)
                    {
                        if(sumOfSize% (expectSize*(*i))==0){
                            expectSize*=(*i);
                            reverseSubArraySizeStack.push_back(*i);
                        }
                        /* code */
                    }
                    sumOfSize+=expectSize;
                    isArray->toArray(collector,expectSize,reverseSubArraySizeStack);
                    //
                }else{
       
                    collector.push_back(std::make_pair((*i).get(),1));
                    sumOfSize+=1;
                }
                if(sumOfSize>=totalSize){
                    break;
                }
                /* code */
            }
            revertSizeStack.push_back(topValue);
            //we still need this to fill up blanks
   
            int blankSize = 1;
            for (auto i = revertSizeStack.begin(); i != revertSizeStack.end(); i++)
            {
                int nextBlank =(*i);
       
                if(sumOfSize>=totalSize){
                    break;
                }
                if(nextBlank==1){
                    continue;
                }

                int nextBlankSize = blankSize*nextBlank;
                int a = sumOfSize%nextBlankSize;
                
                //should be parts of 
                sumOfSize += (nextBlankSize -a);
                //补全0
                collector .push_back(std::make_pair(nullptr,(nextBlankSize-a)));
                
                blankSize = nextBlankSize;
                
                /* code */
            }

            _PRECONDITION(sumOfSize == totalSize ,"Undefined error in array definition")
            return;
            //_NOT_IMPLEMENTED("left as not complete")
            //真是抽抽又象象的设置啊
            //该方法要求一个ArrayInitExp展开填满长度为totalSize数组
            //当出现数组嵌套,考虑当前已经积累的长度 sumOfSize
            //并从sizeStack顶头开始查找长度
            //即sizeT=1; vec =empty
            //我说 我们不如直接给sizeStack反过来好了
            //rbegin()->rend() run i-> if(sumOfSize% sizeT*i==0)-> sizeT*=i vec.push_front(i); else break;
            //request a sizeT 大小的数组即 toArray(collector, 0 ,sizeT,vec);
            // request出来之后给他加上,sumOfSize+=sizeT
            //最后没满怎么办
            //sizeT=1
            //while rbegin()->end() sizeT*=i
            //sumOfSize->nearest i倍数
            //直到sumOfSize是totalSize
            //如果超了
            //直接报compile error

        }

};

class BTypeAST : public BaseAST{
    public:
        BTypeAST ():BaseAST("BTypeAST "){

        }
        virtual std::string getClass() const override{
            return "BTypeAST";
        }
        std::string type = "i32";
        //not used yet
        //std::string type;
        void toString() override{
            _INTER("BTypeAST")
            std::cout << "BTypeAST { int }";
        }
        void toKoopaString(ScopeStack& stack) override{
             _INTER("BTypeAST")
            std::cout << type;
        }
};
class DefAST : public BaseAST {
    public:
        DefAST ():BaseAST("DefAST"){

        }
        virtual std::string getClass() const override{
            return "DefAST";
        }
        virtual bool isConstVal(ScopeStack& stack) override{
            if(declaredAsArray){
                return false;
            }
            if(declaredAsConst){
                //sysY const must be computed when compiling
                _PRECONDITION(isGlobalVal(stack)|| (isInitialized()&&initVal->isConstVal(stack)),std::string("Local const variable ")+ident+" must be defined initialized and computable");
            }
            return declaredAsConst;
        }
        virtual bool isGlobalVal(ScopeStack& stack) override {
            // if(declaredAsGlobal){
            //     _PRECONDITION(!isInitialized()||(initVal->isConstVal(stack)||initVal->isGlobalVal(stack)),"Initializing Global var using Non-const and Non-global initVal");
            // }
            return declaredAsGlobal;
        }
        virtual int calculateInt(ScopeStack& stack) override {
            if(declaredAsArray){
                _ERROR("Array can not be calculate as int")
            }
            if(isInitialized()){
                return initVal->calculateInt(stack);
            }else{
                _PRECONDITION(isGlobalVal(stack),"local const without initialization! ")
                return 0;
            }
        }
        virtual inline bool isInitialized() final{
            return static_cast<bool>(initVal);
        }
        std::string ident;
        std::unique_ptr<BaseAST> initVal;
        bool declaredAsConst = false;
        bool declaredAsGlobal = false;
        bool declaredAsArray = false ; 
        bool isPointerArg = false;
        std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > arraySizeRaw;
        std::vector<int> arraySize;
        int dim;
        void toString() override{
            _INTER("DefAST")
            std::cout << "DefAST {  "<<ident<<" = ";
            if(isInitialized())
                initVal->toString();
            std::cout << " }";
        }
        std::string* allocAddr;
        inline std::string getAddr(ScopeStack& stack){
            if(!allocAddr){
                allocAddr=stack.allocAtTop(ident);
            }
            return *allocAddr;
        }
        void toKoopaString(ScopeStack& stack)  override{
            _INTER("DefAST")
            //std::cout << "i32";
            //add definition to stack;
            if(declaredAsArray){
                toArrayInit(stack);
            }else{
                if(isGlobalVal(stack)){
                    _PRECONDITION(stack.defineAtTop(ident,*this),std::string("variable ")+ident+" already defined in this scope!");
                    if(!declaredAsConst){
                        if(isInitialized()){
                            int initialValue = calculateInt(stack);
                            std::cout << "global " <<getAddr(stack) << " = alloc i32, "<< initialValue <<"\n";
                        }else{
                            std::cout << "global " <<getAddr(stack) << " = alloc i32, zeroinit\n";
                        }
                    }
                }else{
                    if(isInitialized()){
                        initVal->toKoopaString(stack);
                    }
                    _PRECONDITION(stack.defineAtTop(ident,*this),std::string("variable ")+ident+" already defined in this scope!");
                    if(!declaredAsConst){
                        //并不是常量定义
                        //需要allocate
                        //@x = allocate i32
                        std::cout << "  "<<getAddr(stack)<<" = alloc i32\n";
                        if(isInitialized()){
                            std::cout << "  store "<<initVal->getVarName(stack)<<", "<<getAddr(stack)<<"\n";
                        }
                    }
                }
            }
        }
        void genAggregate(const std::vector<int>& array,int start,int end,std::vector<int> & reverseSize){
            int value =* reverseSize.rbegin();
            reverseSize.pop_back();
            std::cout <<"{";
            //最底层
            if(reverseSize.empty()){
                for (int i = start; i < end; i++)
                {
                    if(i>start){
                        std::cout << ", ";
                    }
                    std:: cout <<array[i];
                }
                
            }else{
                int blockSize = (end-start) /value;
                for (int i = start; i < end; i+=blockSize)
                {   
                    if(i>start){
                        std::cout << ", ";
                    }
                    genAggregate(array,i,i+blockSize,reverseSize);
                    /* code */
                }
            }
            std::cout << "}";
            reverseSize.push_back(value);
        }
        void toArrayInit(ScopeStack& stack){
            //
            //先算一下大小
            int totalSize=1;
            _NONNULL(arraySizeRaw)
            for (auto i = arraySizeRaw->begin();i != arraySizeRaw->end();i++)
            {
                _PRECONDITION((*i)->isConstVal(stack),"Array Size must be const value");
                int sizeX=(*i)->calculateInt(stack);
                _PRECONDITION(sizeX>0,"What are you doing");
                totalSize*=sizeX;
                arraySize.push_back(sizeX);
                /* code */
            }
            dim=arraySize.size();
           
            //首先出场的是默认初始化玩家
            if(!isInitialized()){
                _PRECONDITION(stack.defineAtTop(ident,*this),std::string("variable ")+ident+" already defined in this scope!");
                //global @zeroed_array = alloc [i32, 2048], zeroinit
                std::string adr=getAddr(stack);
                if(declaredAsGlobal){
                    std::cout << "  global " << adr ;//" = alloc [i32, " << totalSize << "], zeroinit\n";
                }else{
                    
                    std::cout << "  " << adr;
                }
                std::cout << " = alloc ";
                for (int i = 0; i < arraySize.size(); i++)
                {
                    std::cout << "[";
                    /* code */
                }
                std::cout << "i32";
                for (auto i = arraySize.rbegin(); i != arraySize.rend(); i++)
                {
                    std::cout << ", " << (*i) << "]";
                    /* code */
                }
                if(declaredAsGlobal){
                    std::cout << ", zeroinit";
                }
                std:: cout << "\n";
                if(!declaredAsGlobal){
                    std::vector<std::string> varsFlat ;
                    for (int i=0;i < totalSize ;++i)
                    {
                        varsFlat.push_back("0");
                    }
                    
                    genInit(varsFlat,stack,adr,totalSize);
                }
            }else{
                ArrayInitExpAST* ref;

                _PRECONDITION((ref=dynamic_cast<ArrayInitExpAST*>(initVal.get())),"Array must be initialized by array init value")
                std::vector<std::pair<BaseAST*,int > > collector;
                

                std::vector<int> reverseArraySize={arraySize.rbegin(),arraySize.rend()};
                //enable parser like int[4] a={1,2,{3},4} but not {1,2,{{3}},4}
                reverseArraySize.insert(reverseArraySize.begin(),1);

                ref->toArray(collector,totalSize,reverseArraySize);
                //all subVars collected in collector;
                
                //each element should be callable
                std::string adr=getAddr(stack);
                if(declaredAsGlobal){
                    std::cout << "  global " << adr ;//" = alloc [i32, " << totalSize << "], zeroinit\n";
                }else{
                    
                    std::cout << "  " << adr;
                }
                std::cout << " = alloc ";
                for (int i = 0; i < arraySize.size(); i++)
                {
                    std::cout << "[";
                    /* code */
                }
                std::cout << "i32";
                for (auto i = arraySize.rbegin(); i != arraySize.rend(); i++)
                {
                    std::cout << ", " << (*i) << "]";
                    /* code */
                }
                if(declaredAsGlobal){
                    std::cout << ", ";
                    std::vector<int> reverseSize = {arraySize.rbegin(),arraySize.rend()};
                    std::vector<int> varsFlat;
                    for (auto i = collector.begin(); i !=collector.end(); i++)
                    {
                        int value;
                    
                        if(i->first==nullptr){
                            value = 0;
                        }else{
                            value =  i->first->calculateInt(stack);
                        }
                        for (auto it = 0; it < i->second; it++)
                        { 
                            varsFlat.push_back(value);
                        }
                    }
                    _PRECONDITION(varsFlat.size()==totalSize,"Unknown Error: var amount mismatch")
                    genAggregate(varsFlat,0,totalSize,reverseSize);
                }
                std:: cout << "\n";
                if(!declaredAsGlobal){
                    //store value in
                    std::vector<std::string> varsFlat ;
                    for (auto i = collector.begin(); i !=collector.end(); i++)
                    {
                        std::string value;
                    
                        if(i->first==nullptr){
                            value = "0";
                        }else{
                            i->first->toKoopaString(stack);
                            value =  i->first->getVarName(stack);
                        }

                        for (auto it = 0; it < i->second; it++)
                        { 
                            varsFlat.push_back(value);
                        }
                    }
                    genInit(varsFlat,stack,adr,totalSize);
                }
                _PRECONDITION(stack.defineAtTop(ident,*this),std::string("variable ")+ident+" already defined in this scope!");
            }
            
            

        }
        //gen init
        void genInit(const std::vector<std::string>& varsFlat,ScopeStack& stack,std::string& adr,int totalSize){
            
            _PRECONDITION(varsFlat.size()==totalSize,"Unknown Error: var amount mismatch")
            std::vector<std::string> multiLabels; //记录当前每一级的数组的ptr暂时标签
            std::vector<int > multiIndexs ;//记录当前的a[i1][i2][i3]
            int dim = arraySize.size(); //几维数组
            //std::vector<int > multiIndexsSizeof; //记录当前之后的子索引大小 如 [1][2][][][]后面的大小 最底层为1怕[1][2][3][4]..[k]
            //load 0 0 0 0 0
            std::string labelname = getVarName(stack) +"_ini";

            for (int i = 0; i < dim; i++)
            {
                std::string newlabelname = labelname + std::string("_0");
                multiIndexs.push_back(0) ;
    
                //like a[2][3][5]
                //push 15 5 1 in this array
                
                multiLabels.push_back(std::string(newlabelname));
                std::cout << "  " << newlabelname << " = getelemptr " << ((i== 0)?adr :labelname)<<", 0\n";
                labelname = newlabelname;
                //gen code 
            }
            //store 1, %0
            std::cout  << "  store " << varsFlat[0] << ", " << labelname <<"\n";
            for (auto i = 1; i < totalSize; i++)
            {
                multiIndexs[dim-1]++;
                //update 范围
                int index = dim -1;
                for (; index >0 ; index--)
                {
                    //出现切换
                    if(multiIndexs[index]>=arraySize[index]){
                        multiIndexs[index] = 0;
                        multiIndexs[index-1] ++;
                    }else{
                        break;
                    }
                    /* code */
                }
               
                for (; index< dim; index++)
                {
                    //需要更新这一级的label
                    std::string oldParent =  (index == 0)? adr : multiLabels[index-1];
                    std::string newLabel = ((index==0)?(getVarName(stack) +"_ini"): oldParent)  + "_" + std::to_string(multiIndexs[index]);
                    multiLabels[index] = std::string(newLabel);
                    std::cout << "  " << newLabel << " = getelemptr " << oldParent<<", "<< multiIndexs[index]<<"\n";
                }
                //trigger flag update
                std::cout  << "  store " << varsFlat[i] << ", " << multiLabels[dim-1] <<"\n";
            }
        }

};

class DeclAST : public BlockItemAST {

    public :
        DeclAST ():BlockItemAST("DeclAST"){

        }
        virtual std::string getClass() const override{
            return "DeclAST";
        }
        bool isConst=false;
        virtual bool isConstVal(ScopeStack& stack) override{
            return false;
        }
        virtual int calculateInt(ScopeStack& stack) override {
            _NOT_IMPLEMENTED("should not invoke this method on a DeclAST!");

        }
        std::unique_ptr<BaseAST> btype;
        std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > defs ;
        void markAsGlobal(){
            for (auto i = defs->begin(); i != defs->end(); i++)
            {
                dynamic_cast<DefAST*> ((*i).get())->declaredAsGlobal=true;
                /* code */
            }
            
        }
        void toString()  override{
            _INTER("DeclAST")
            std::cout << "DeclAST { const: "<<isConst<<", ";
            _NONNULL(btype)
            btype->toString();
            std::cout << " ;";
            std::for_each(defs->begin(),defs->end(), [](const std::unique_ptr<BaseAST>& item) {
                item->toString(); //
                std::cout << " ,";
            });
            std::cout << " }";

        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("DeclAST")
            //std::cout << "i32";
            if(isConst){
               
                //const def
                std::for_each(defs->begin(),defs->end(),[&stack](const std::unique_ptr<BaseAST> & item){
                    //const int array
                    item->isConstVal(stack);
                    
                    item->toKoopaString(stack);
                });
            }else{
                //left 
                std::for_each(defs->begin(),defs->end(),[&stack](const std::unique_ptr<BaseAST> & item){
                    
                    assert(!item->isConstVal(stack));
                    
                    item->toKoopaString(stack);
                });
            }
         
        }
};


//reference from a definition
//complete later by symbol table
class RefAST : public BaseAST {
    public:
        RefAST():BaseAST("RefAST"){

        }
        virtual std::string getClass() const override{
            return "RefAST";
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            initRef(stack);
            _PRECONDITION(init&&ref,std::string("undefined reference ")+refName);
            return ref->declaredAsGlobal;
        }
        virtual bool isConstVal(ScopeStack& stack) override{
            initRef(stack);
            _PRECONDITION(init&&ref,std::string("undefined reference ")+refName);
            return ref->declaredAsConst;
        }
        virtual int calculateInt(ScopeStack& stack) override {
            initRef(stack);
            _PRECONDITION(isConstVal(stack)||isGlobalVal(stack),"Non const/Non global value can't be calculated: "+refName);
            return ref->initVal->calculateInt(stack);
        }
        virtual std::string getVarName(ScopeStack& stack) override {
            initRef(stack);
            if(isConstVal(stack)){
                return ref->initVal->getVarName(stack);
            }
            else{
                return BaseAST::getVarName(stack);
            }
        }
        std::string refName;
        //nullptr
        bool init=false;
        DefAST* ref;
        void initRef(ScopeStack& stack){
            if(!init){
                init=true;
                DefAST* ptr=stack.getDefByName(refName);
                _PRECONDITION(ptr,std::string("this ident haven't been defined: ")+refName);
                ref=ptr;
            }
        }
        void toString()  override{
            _INTER("RefAST")
        
            std::cout << "RefAST {  "<<refName;
            //initVal->toString();
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
             _INTER("RefAST")
            //std::cout << "i32";
            //init ref
   
            //保证ref的目标已经找到了
            initRef(stack);

            // when doing sth like
            //int x;
            //x=x+1
            //we should load x in x=x+1 's ref at x:
            //%2= load @x

            if(!ref->declaredAsArray){

                if(!ref->declaredAsConst){
                //如果不是const,每次koopaString的时候都要load一次
                    std::cout<<"  "<<getVarName(stack)<<" = load "<<ref->getAddr(stack)<<"\n";
                }
            }else{

                //refering to an array and need a pointer     ?????????????????????????????
                if(ref->isPointerArg){
                    std::cout << "  " << getVarName(stack) << "_ptr_tmp_ld = load "<< ref->getAddr(stack)<<"\n";
                    std::cout<<"  "<<getVarName(stack) << " = getptr " << getVarName(stack) << "_ptr_tmp_ld, 0\n";
                }else{
                    std::cout<<"  "<<getVarName(stack) << " = getelemptr " << ref->getAddr(stack) <<", 0\n";
                }
            }

            

        }
        virtual void markChange(ScopeStack& stack,std::unique_ptr<BaseAST>& fromExp) override{
            _DEBUG("do setVariable");
            initRef(stack);
            _PRECONDITION(!isConstVal(stack),std::string("could not assign a value to a const value: ")+refName);
            _PRECONDITION(!ref->declaredAsArray,"Undefined operation of setting array")
            std::cout<<"  store "<<fromExp->getVarName(stack)<<", "<<ref->getAddr(stack)<<"\n";
        }
};
class ArrayIndexRefAST :public RefAST {
    public:
        virtual std::string getClass() const override{
            return "ArrayIndexRefAST";
        }
        virtual bool isGlobalVal(ScopeStack& stack) override{
            initRef(stack);
            _PRECONDITION(init&&ref,std::string("undefined reference ")+refName);
            return ref->declaredAsGlobal;
        }
        virtual bool isConstVal(ScopeStack& stack) override{
            return false;
        }
        virtual int calculateInt(ScopeStack& stack) override {
            _PRECONDITION(false,"Array is not calculatable: "+refName);
           // return ref->initVal->calculateInt(stack);
        }
        virtual std::string getVarName(ScopeStack& stack) override {
                return BaseAST::getVarName(stack);
        }
        //add a indexPath here 
        std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > indexPath;
        void initRef(ScopeStack& stack){
            //_NOT_IMPLEMENTED("left as not complete")
            if(!init){
                init=true;
                DefAST* ptr=stack.getDefByName(refName);
                
                _PRECONDITION(ptr,std::string("this ident haven't been defined: ")+refName);
                _PRECONDITION(ptr->declaredAsArray,std::string("this ident is not an array: ")+refName);
                ref=ptr;
            }
        }
        void toString()  override{
            _INTER("ArrayRefAST")
        
            std::cout << "ArrayRefAST {  "<<refName;
            //initVal->toString();
            std::cout << " }";
        }
        std::string finalPtr;
        bool initializedPtr = false;
        bool isPointerRef ;
        void initPtr(ScopeStack & stack){
            if(!initializedPtr){
                initRef(stack);
                std::string target = ref->getAddr(stack);
                std::string thisName = getVarName(stack);
                _NONNULL(indexPath)
                int dim =indexPath->size();

                //because of pointer
                _PRECONDITION(dim <= ref->dim,"Dim not match, larger than definition")
                //如果是指针引用的话 需要更改行为 不load
                isPointerRef = dim<(ref->dim);
                std::string parentPath;
                std::string nextPath;
                if(!ref->isPointerArg){
                    for (int i = 0; i < dim; i++)
                    {
                        BaseAST* indexI= (*indexPath)[i].get();
                        indexI->toKoopaString(stack);
                        parentPath = (i==0)? target: nextPath;
                        nextPath = (i!=dim-1)?( thisName+"_ref_"+std::to_string(i)):(thisName+"_ref_ptr_");
                        std::cout << "  "<< nextPath << " = getelemptr " << parentPath << ", " << indexI->getVarName(stack) << "\n";
                        /* code */
                    }
                }else{
                    std::string loadPath = thisName+"_pref_";//+std::to_string(i);
                    std::cout << "  " << loadPath << " = load " << target <<"\n";
                    for (int i = 0; i < dim; i++)
                    {
                        BaseAST* indexI= (*indexPath)[i].get();
                        indexI->toKoopaString(stack);
                        parentPath = (i==0)? loadPath: nextPath;
                        nextPath = (i!=dim-1)?( thisName+"_ref_"+std::to_string(i)):(thisName+"_ref_ptr_");
                        if(i==0){
                            std::cout << "  "<< nextPath << " = getptr " << parentPath << ", " << indexI->getVarName(stack) << "\n";
                        }else{
                            std::cout << "  "<< nextPath << " = getelemptr " << parentPath << ", " << indexI->getVarName(stack) << "\n";
                        }
                        
                        /* code */
                    }
                }
                
                finalPtr = nextPath;
                initializedPtr=true;
            }
        }
        void toKoopaString(ScopeStack& stack) override{
             _INTER("ArrayRefAST")
            //std::cout << "i32";
            //init ref
            //_DEBUG(std::string("fetch reference ")+refName);
            initPtr(stack);
            if(!isPointerRef){
                std::cout<<"  "<<getVarName(stack)<<" = load "<<finalPtr<<"\n";
            }else{
                //相当于给自己整一个ptr
                std::cout<<"  "<<getVarName(stack) << " = getelemptr " << finalPtr <<", 0\n";
            }
            
            //保证ref的目标已经找到了
            
        }
        virtual void markChange(ScopeStack& stack,std::unique_ptr<BaseAST>& fromExp) override{
            initRef(stack);
            initPtr(stack);
            _PRECONDITION(!isPointerRef,"Undefined operation to set pointer")
            _NONNULL(fromExp)
            std::cout<<"  store "<<fromExp->getVarName(stack)<<", "<<finalPtr<<"\n";
        }
};
//symbol table design:
//a stack
//push and pop when toKoopaString a BlockAST
        

class FuncParamAST : public BaseAST {
    public: 
        FuncParamAST():BaseAST("FuncParamAST"){

        }
        virtual std::string getClass() const override{
            return "FuncParamAST";
        }
        std::string ident;
        std::unique_ptr<BaseAST> type;
        void toString()  override {
            _INTER("FuncParamType");
            std::cout << "FuncParamAST { "<<ident <<",";
            type->toString();
            
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            addr=stack.genDefinitionAddr(ident);
            std::cout <<addr<<": ";
            printType(stack);
            //assume that this is a int or int* or sth
            //type->toKoopaString(stack);
        } 
        virtual void printType(ScopeStack& stack){
            std::cout << "i32" ;
        }
        
        std::unique_ptr<DefAST> paramStackDecl;
        std::string addr;
        virtual void allocParam(ScopeStack& stack) {
            _DEBUG("Alloc Simple Argument "+ident)
            paramStackDecl=std::make_unique<DefAST>();
            //be careful of segmentation fault!
            //It's like %x=@x;
            paramStackDecl->declaredAsConst=false;
            //different from simple alloc ,do maually
            paramStackDecl->ident=ident;
            stack.defineAtTop(ident,*paramStackDecl);
            
            //这里的变量永远不会重复,因为地址而且以%开头
            paramStackDecl->allocAddr=stack.allocAtTop(ident,"%_func_param_"+ident);

            std::cout << "  "<<paramStackDecl->getAddr(stack)<<" = alloc i32\n";
            std::cout << "  store " << addr<<", "<<paramStackDecl->getAddr(stack)<<"\n";
            
        }
};
class FuncParamPointerAST : public FuncParamAST{
    public:
        std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > followSize;
        void allocParam(ScopeStack& stack) override {
            _DEBUG("Alloc Pointer Argument "+ident)
            paramStackDecl=std::make_unique<DefAST>();
            //be careful of segmentation fault!
            //It's like %x=@x;
            paramStackDecl->declaredAsConst=false;
            //pretend that I am allocing an array
            paramStackDecl->declaredAsArray=true;
            //mark this as a pointer argument passed,so fetching value will use getPtr
            paramStackDecl->isPointerArg=true;
            //first [] is empty, set dim
            paramStackDecl->dim = followSize->size()+1;
            //different from simple alloc ,do maually
            paramStackDecl->ident=ident;

            stack.defineAtTop(ident,*paramStackDecl);
            
            //这里的变量永远不会重复,因为地址而且以%开头
            paramStackDecl->allocAddr=stack.allocAtTop(ident,"%_func_param_"+ident);
            //alloc as int pointer
            std::cout << "  "<<paramStackDecl->getAddr(stack)<<" = alloc ";
            printType(stack);
            std::cout << "\n";
            std::cout << "  store " << addr<<", "<<paramStackDecl->getAddr(stack)<<"\n";
            
        }
        virtual void printType(ScopeStack& stack) override{
            std::cout <<"*";
            _NONNULL(followSize)
            if(followSize->empty()){
                std::cout << "i32";
            }else{
                int dimT=followSize->size();
                for (int i = 0; i < dimT; i++)
                {
                    std::cout << "[";
                }
                std:: cout << "i32";
                for (int i = dimT-1; i >=0; --i)
                {
                    auto& indexT= (*followSize)[i];
                    int index = indexT->calculateInt(stack);
                    std::cout <<", " << index <<"]";
                }          
            }
        }
};
class FuncParamTypeAST : public BaseAST{
    public: 
        FuncParamTypeAST():BaseAST("FuncParamTypeAST"){

        }
        virtual std::string getClass() const override{
            return "FuncParamTypeAST";
        }
        std::vector<std::unique_ptr<FuncParamAST> >  paramList;
        void toString()  override {
            _INTER("FuncParamType");
            std::cout << "FuncParamTypeAST { ";
            for_each(paramList.begin(),paramList.end(),[](const std::unique_ptr<FuncParamAST>& item){
                item->toString();
            });
            
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            bool hasOneOrMore=false;
            for (auto i = paramList.begin(); i != paramList.end(); i++)
            {   
                if(hasOneOrMore){
                    std::cout << ", ";
                }
                (*i)->toKoopaString(stack);
                hasOneOrMore=true;
                /* code */
            }
            
        } 

};
class FuncTypeAST : public BaseAST{
    public:
        FuncTypeAST():BaseAST("FuncTypeAST"){

        }
        std::string type;
        virtual std::string getClass() const override{
            return "FuncTypeAST";
        }
        //not used yet
        //std::string type;
        void toString() override{
            _INTER("FuncTypeAST");
            std::cout << "FuncTypeAST { "<<type<<" }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("FuncTypeAST");
            //todo
            if(type=="int"){
                std::cout << ": i32";
            }else if(type=="array"){
                std::cout << ": *i32";
            }
            
          
        }
};
// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST,public Catcher<std::string> {
    public:
        virtual std::string getClass() const override{
            return "FuncDefAST";
        }
        FuncDefAST():BaseAST("FuncDefAST"){

        }
        bool referenced=false;
        void markReferenced(){
            referenced=true;
        }
        bool isReferenced(){
            return referenced||ident=="main";
        }
        std::unique_ptr<BaseAST> func_type;
        std::string ident;
        std::unique_ptr<FuncParamTypeAST> paramType;
        std::unique_ptr<BlockItemAST> block;
        void toString()  override {
            _INTER("FuncDefAST");
            std::cout << "FuncDefAST { ";
            func_type->toString();
            std::cout << ", " << ident << ", ";
            
            block->toString();
            std::cout << " }";
        }
        std::string getVarName(ScopeStack& stack) override{
            return "%t"+std::to_string(getUniqueId());
        }
        void genDecl(ScopeStack& stack) {
            std::cout << "decl @" << ident << "(";
            bool hasOneOrMoreParam=false;
            for (auto i = paramType->paramList.begin(); i !=paramType->paramList.end(); i++)
            {
                std::string& type = dynamic_cast<BTypeAST&>(*(*i)->type).type;
                /* code */
                if(hasOneOrMoreParam){
                    std::cout << ", ";
                }
                std::cout << type;
                hasOneOrMoreParam=true;
            }
            std::cout <<")";
            func_type->toKoopaString(stack);
            std::cout << "\n";
        }
        std::string returnedVarName;
        bool needReturnVal;
        virtual std::string catchOp(int code) override {
            switch (code)
            {
                case CODE_RET:
                    return returnedVarName;
                    

                default:
                    return "";
             
            }
        }
        void toKoopaString(ScopeStack& stack)  override{
            _INTER("FuncDefAST");
            //function itself defined in global scope
            //this is for end_part_label
            functionCounter++;
            stack.defineAtTop(ident,*this);
            stack.push();
            //argument in the function should be defined in function scope
            std::cout << "fun @" << ident << "(";
            paramType->toKoopaString(stack);
            std::cout <<")";
            func_type->toKoopaString(stack);
            std::cout << " {";
            genLabelCode("%entry",stack);
            //alloc arguments
            for (auto i = paramType->paramList.begin(); i != paramType->paramList.end(); ++i)
            {
                (*i)->allocParam(stack);
            }
            bool newReturn=false;
            if(dynamic_cast<FuncTypeAST&>(*func_type).type=="void"){
                returnedVarName= "__void_return__";
                needReturnVal=false;
            }else{
                needReturnVal=true;
                returnedVarName= "@my_ret_val";
                newReturn=true;
                //不   复用参数

                // if(paramType->paramList.empty()){
                //     newReturn=true;
                // }else{
                //     returnedVarName= *(*paramType->paramList.begin())->paramStackDecl->allocAddr;
                // }
            }
            //register return catcher in function scope
            stack.registerAbortCatcher(this);
            
            //move to here
            if(branchCounter==0){
                //no branch jump,directly do koopaString, ret when first met;
                //没有分支和跳转,指令全部线性执行,直接对着block展开,如果遇到ret block中将会直接ret，忽略后面的语句
                block->toBlockItem(stack);
                //不是哥们你return呢
                //这里需要处理ret的问题，但是我们已经在toBlockItem中解决了
                // if(!stack.hasReturn){
                //     //这玩意还是有问题 至于什么问题我们至今不得而知
                //     if(needReturnVal){
                //         std::cout << "  ret\n";
                //     }else{
                //         std::cout << "  ret\n";
                //     }
                    
                // }
                //这里应该没问题
                
            }else{
                //should do this
                //分支跳转了,需要处理更多的问题
                //创建返回值
                if(needReturnVal&&newReturn){
                    std::cout << "  "<< returnedVarName <<" = alloc i32\n";
                    std::cout << "  store 0, "<< returnedVarName <<"\n";
                }
                
                
                //?
                //so what's the label
                block->toBlockItem(stack);
                //这里需要处理ret的问题，但是我们已经在toBlockItem中解决了

                genLabelCode("%end_part_"+std::to_string(functionCounter),stack);
                if(needReturnVal){
                    std::cout <<"  "<<getVarName(stack)<<" = load "<< returnedVarName <<"\n  ret "<<getVarName(stack)<<"\n";
                }else{
                    std::cout <<"  ret\n";
                }
                
            }
            std::cout << "}\n";
            stack.pop();
        }
};



class FuncRParamAST : public BaseAST{
    public:
        FuncRParamAST():BaseAST("FuncRParam"){

        }
        virtual std::string getClass() const override{
            return "FuncRParam";
        }
        std::vector<std::unique_ptr<BaseAST> > explist;
        void toString() override{
            _INTER("FuncRParam");
            std::cout << "FuncRParam { ";
            for_each(explist.begin(),explist.end(),[](std::unique_ptr<BaseAST> & item){
                item->toString();
            });
            std::cout << " }";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("FuncRParam");
            //todo
        
            for_each(explist.begin(),explist.end(),[&stack](std::unique_ptr<BaseAST> & item){
                item->toKoopaString(stack);
            });
            _INTER("FuncRParam Exit")
          
        }
};

class FuncRefAST : public BaseAST{
    public:
        FuncRefAST():BaseAST("FuncRefAST"){

        }
        virtual std::string getClass() const override{
            return "FuncRefAST";
        }
        std::string refName;
        std::unique_ptr<FuncRParamAST> invokeParam;
        bool isConstVal(ScopeStack& stack)override{
            return false;
        }
        int calculateInt(ScopeStack& stack)override{
            _NOT_IMPLEMENTED("Non-const val FuncRef")
        }
        std::string getVarName(ScopeStack& stack) override{
            return BaseAST::getVarName(stack);
        }
        void toString() override{
             _INTER("FuncRefAST");
            std::cout << "FuncRefAST { " <<refName <<"(";
            invokeParam->toString();
            std::cout << ")";
        }
        void toKoopaString(ScopeStack& stack) override{
            _INTER("FuncRefAST")
            invokeParam->toKoopaString(stack);
            //unchecked call to FuncRef
            //should fetch function definition
            FuncDefAST* invokeTarget=stack.getFuncDefByName(refName);
            _NONNULL(invokeTarget)
            invokeTarget->markReferenced();
            FuncTypeAST* typeAst = dynamic_cast<FuncTypeAST*>( invokeTarget->func_type.get());
            _NONNULL(typeAst)
            if(typeAst->type=="void"){
                std::cout << "  call @"<< refName <<"(" ;
            }else{
                std::cout << "  " << getVarName(stack) << " = call @"<< refName <<"(" ;
            }
            bool hasOneOrMore=false;
            for (auto i = invokeParam->explist.begin(); i != invokeParam->explist.end(); i++)
            {
                if(hasOneOrMore){
                    std::cout << ",";
                }
                std::cout  << (*i)->getVarName(stack);
                hasOneOrMore=true;
            }
            std::cout << ")\n";
            //_NOT_IMPLEMENTED("not implemented")
        }
};

class CompUnitAST : public BaseAST {
    public:
        CompUnitAST():BaseAST("CompUnitAST"){
        }
        virtual std::string getClass() const override{
            return "CompUnitAST";
        }
        std::unique_ptr< std::vector<std::unique_ptr<BaseAST> > > itemlist;
        void toString()  override {
            _INTER("CompUnit");
            std::cout << "CompUnitAST { ";
            for_each(itemlist->begin(),itemlist->end(),[](std::unique_ptr<BaseAST> & item){
                item->toString();
            });
            std::cout << " }\n";
        }
        FuncDefAST* genLibFunc(const std::string& ident,const std::string& typeT){
            auto it=new FuncDefAST();
            it->ident=ident;
            auto type=new FuncTypeAST();
            type->type=typeT;
            it->func_type=std::unique_ptr<BaseAST>(type);
            it->paramType=std::make_unique<FuncParamTypeAST>();
            return it;
        }
        template < typename... Args>
        FuncDefAST* genLibFunc(const std::string& ident,const std::string& typeT,Args... argumentTypeT){
            auto pt=genLibFunc(ident,typeT);
            _DEBUG("start add argument type")
            addArgumentTypeT(pt,argumentTypeT...);
            _DEBUG("add finish")
            return pt;
        }
        template <typename... Args>
        void addArgumentTypeT(FuncDefAST* def,const std::string& argumentT,Args... argumentTypeT){
            addArgumentTypeT(def,argumentT);
      
            addArgumentTypeT(def,argumentTypeT...);
        }
        void addArgumentTypeT(FuncDefAST* def,const std::string& argumentTypeT){
            auto argument=new FuncParamAST();
            argument->ident=std::string("var")+std::to_string( def->paramType->paramList.size());
            auto btype=new BTypeAST();
            btype->type=argumentTypeT;
            argument->type=std::unique_ptr<BaseAST>(btype);
            def->paramType->paramList.push_back(std::unique_ptr<FuncParamAST>(argument));
        }
        void toKoopaString(ScopeStack& stack)  override{
            _INTER("CompUnit");
            //global scope
            stack.push();
            //add global definitions
            std::vector<std::unique_ptr<FuncDefAST> > libFunctions=std::vector<std::unique_ptr<FuncDefAST> >();
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("getint","int")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("getch","int")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("getarray","int","*i32")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("putint","void","i32")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("putch","void","i32")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("putarray","void","i32","*i32")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("starttime","void")));
            libFunctions.push_back(std::unique_ptr<FuncDefAST>(genLibFunc("stoptime","void")));
            for (auto i = libFunctions.begin(); i != libFunctions.end(); i++)
            {
                
                stack.defineAtTop((*i)->ident,*(*i));
                
                /* code */
            }
            auto originStd= std::cout.rdbuf();
            std::ostringstream oss;
            std::cout.rdbuf(oss.rdbuf());

            for_each(itemlist->begin(),itemlist->end(),[&stack](std::unique_ptr<BaseAST> & item){
                std::ostringstream oss;
                auto origin=std::cout.rdbuf();
                std::cout.rdbuf(oss.rdbuf());
                item->toKoopaString(stack);
                std::string cd=oss.str();
                oss.str("");
                std::cout.rdbuf(origin);
                if(!cd.empty()){
                    //输出为空时直接跳过
                    if(globalCounter>0){
                        //维护全局部件之间的换行
                        std::cout<<"\n";
                    }
                    std::cout<<cd;
                    globalCounter++;
                }
            });
            std::string code=oss.str();
            oss.str("");
            std::cout.rdbuf(originStd);
            bool hasOneOrMoreRef=false;
            for (auto i = libFunctions.begin(); i != libFunctions.end(); i++)
            {
                if((*i)->isReferenced()){
                    (*i)->genDecl(stack);
                    hasOneOrMoreRef=true;
                }
                /* code */
            }
            if(hasOneOrMoreRef)
                std::cout<<"\n";
            std::cout<<code;
            stack.pop();
        }

};