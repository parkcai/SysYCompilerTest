#pragma once
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <vector>
using namespace std;
class BaseAST; 
class CompUnitAST;
class DeclAST;
class ConstDeclAST;
class ConstDefAST;
class ConstDefsAST;
class ConstInitValAST;
class VarDeclAST;
class VarDefsAST;
class VarDefAST;
class InitValAST;

class FuncDefAST;
class FuncFParamAST;
class FuncFParamsAST;
class FuncRParamsAST;
//class FuncTypeAST;

class BlockAST;
class BlockItemAST;
class BlockItemsAST;
class StmtAST;

class ExpAST;
class LValAST;
class PrimaryExpAST;
class UnaryExpAST;
class AddExpAST;
class MulExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;
class ConstExpAST;

struct valinfo{
  int cst;
  int value;
  int init;
  std::string name;
  std::vector<int> shape;
};

struct whileinfo{
  std::string entry;
  std::string body;
  std::string end;
};

struct funcinfo{
  std::string name;
  std::string functype;
};

extern int Koopa_Index;
extern int Val_Index;
extern int Block_Index;
extern int is_ret;
extern int global_libfunc;
extern std::vector<std::unordered_map<std::string, struct valinfo>>Koopa_blocklist;
extern std::vector<struct whileinfo>while_stack;
extern std::unordered_map<std::string, struct funcinfo>func_list;




// 所有 AST 的基类
class BaseAST {
 public:
  struct ret {
    std::string strret;
    int intret;
  };
  virtual ~BaseAST() = default;
  virtual void Dump() const = 0;
  virtual int cal_val() const = 0;
  virtual ret ToIR(std::ostream& out) const = 0;
};


class FuncFParamAST : public BaseAST{
  public:
    int catagory;
    std::string btype;
    std::string ident;
    std::vector<unique_ptr<BaseAST>> constexps;
  void Dump() const override {

  }
  int cal_val() const override{
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;

    return ret_;
  }
  void printparam(std::ostream& out){
    if (catagory == 1)
    {out<<ident<<": i32";}
    else{
      std::string ret="i32";
      for(auto & exp: constexps){
        ret = "[" + ret + ", "+ to_string((exp->ToIR(out)).intret)+"]";
      }
      out<<ident<<": *"<<ret;
    }
  }

};
class FuncFParamsAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<FuncFParamAST> funcfparam;
    std::unique_ptr<FuncFParamsAST> funcfparams;
    std::vector<std::unique_ptr<FuncFParamAST>> paramlist;
  void Dump() const override {

  }
  int cal_val() const override{
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;

    return ret_;
  }
  void get_paramlist(){
    if (catagory == 1){
      paramlist.emplace_back(std::move(funcfparam));
    }
    else if (catagory == 2){
      if ((funcfparams->paramlist).empty())
        funcfparams->get_paramlist();
      paramlist.emplace_back(std::move(funcfparam));
      for (int i = 0; i<funcfparams->paramlist.size();i++ ) {
          paramlist.emplace_back(std::move(funcfparams->paramlist[i]));  // Assuming BaseAST has a copy constructor
      }

    }
  }
};
// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  int catagory;
  std::string func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<FuncFParamsAST> funcfparams;

  void Dump() const override {

  }
  int cal_val() const override{
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    std::string funty="";
    if (func_type=="int"){
      funty=": i32";
    }
    struct funcinfo func_info;
    func_info.name=ident;
    func_info.functype=func_type;
    func_list.insert(make_pair(ident,func_info));
    if (catagory == 1){
      out << "fun "<< ident <<"()"<<funty<<" {\n";
      out << "%entry:\n";
      
    }
    else if (catagory == 2){
      funcfparams->get_paramlist();
      out << "fun "<< ident <<"(";
      auto &paramlist=funcfparams->paramlist;
      for (int i=0; i< paramlist.size()-1;i++){
        paramlist[i]->printparam(out);
        out<<", ";
      }
      paramlist[paramlist.size()-1]->printparam(out);
      out<<")"<<funty<<" {\n";
      out << "%entry:\n";
    }
    is_ret =0;
    std::unordered_map<std::string,struct valinfo> func_val_list;
    Koopa_blocklist.push_back(func_val_list);
    if (catagory == 2){
      auto &paramlist=funcfparams->paramlist;
      for (int i =0;i< paramlist.size();i++){
        if(paramlist[i]->catagory == 1)
        {struct valinfo val_info;
        val_info.cst = 0;
        val_info.name = paramlist[i]->ident+"_"+to_string(Val_Index);
        Val_Index+=1;
        Koopa_blocklist[Koopa_blocklist.size()-1].insert(make_pair(paramlist[i]->ident,val_info));
        out<<"\t"<<val_info.name<<" = alloc i32\n";
        out<<"\tstore "<<paramlist[i]->ident<<", "<<val_info.name<<"\n";}
        else{
          std::vector<int> shape;
          shape.push_back(-1);
          for (auto &exp:paramlist[i]->constexps){
            shape.push_back((exp->ToIR(out)).intret);
          }
          struct valinfo val_info;
          val_info.cst = 2;
          val_info.name = paramlist[i]->ident+"_"+to_string(Val_Index);
          Val_Index+=1;
          val_info.shape = shape;
          Koopa_blocklist[Koopa_blocklist.size()-1].insert(make_pair(paramlist[i]->ident,val_info));
          std::vector<int> left = std::vector<int>(shape.begin()+1,shape.end());
          std::string ret = "i32";
          for (int i = left.size()-1;i>=0;i--){
            ret = "[" +ret+", "+to_string(left[i]) + "]";
          }
          out<<"\t"<<val_info.name<<" = alloc *"<<ret<<"\n";
          out<<"\tstore "<<paramlist[i]->ident<<", "<<val_info.name<<"\n";
        }
      }
    }
    block->ToIR(out);
    if(!is_ret){
      if (func_type=="int")
      {out<<"\tret 0\n";}
      else if (func_type=="void"){
        out <<"\tret\n";
      }
    }
    out << "}\n\n";
    Koopa_blocklist.pop_back();
    Koopa_blocklist.shrink_to_fit();
    return ret_;
  }
};





class FuncRParamsAST :public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<FuncRParamsAST> funcrparams;
    std::vector<std::unique_ptr<BaseAST>> explist;
  void Dump() const override {

  }
  int cal_val() const override{
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;

    return ret_;
  }
  void get_explist(){
    if(catagory == 1) {
      explist.emplace_back(std::move(exp));
    }
    else if (catagory == 2){
      if((funcrparams->explist).empty()) funcrparams->get_explist();
      explist.emplace_back(std::move(exp));
      for (int i =0;i<funcrparams->explist.size();i++) {
          explist.emplace_back(std::move(funcrparams->explist[i]));  // Assuming BaseAST has a copy constructor
      }
    }
  }

};

class BlockAST : public BaseAST{
    public:
        int catagory;
        std::unique_ptr<BaseAST> blockitems;
    void Dump() const override {

    }
    int cal_val() const override{
    return 0;
  }
    ret ToIR(std::ostream& out) const override {
        struct ret ret_;
        if (catagory ==1){
        std::unordered_map<std::string,struct valinfo> val_list;
        Koopa_blocklist.push_back(val_list);
        blockitems->ToIR(out);
        Koopa_blocklist.pop_back();
        Koopa_blocklist.shrink_to_fit();
        }
        return ret_;
    }
};



class BlockItemsAST : public BaseAST{
  public:
  int catagory;
  std::unique_ptr<BaseAST> blockitem;
  std::unique_ptr<BaseAST> blockitems;
  void Dump() const override {

    }
  int cal_val() const override{
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory==1){
      blockitem->ToIR(out);
    }
    else if (catagory ==2){
      blockitem->ToIR(out);
      blockitems->ToIR(out);
    }
    return ret_;
  }
};

class LValAST : public BaseAST{
  public:
    int catagory;
    std::string ident;
    int number;
    std::vector<unique_ptr<BaseAST>> exps;
  void Dump() const override {

    }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    return ret_;
  }
  int cal_val() const override{
    for(int j=Koopa_blocklist.size()-1;j>=0;j--){
      auto i = Koopa_blocklist[j].find(ident);
      if(i==Koopa_blocklist[j].end()){
        continue;
      }
      else{
        return i->second.value;

      }
    }
    return -1;
  }
  std::string my_getptr(std::ostream& out,const std::string &name, const std::vector<std::string>& reg_index){
    if(reg_index.size() == 1){
        std::string tmp = "%"+std::to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp<<" = getelemptr "<<name<<" , "<<reg_index[0]<<"\n";
        return tmp;
    } else {
        std::string tmp = "%"+std::to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp<<" = getelemptr "<<name<<" , "<<reg_index[0]<<"\n";
        return my_getptr(out,tmp,std::vector<std::string>(reg_index.begin() + 1, reg_index.end()));
    }
}
  ret ToIR2(std::ostream& out, int flag)  {
    struct ret ret_;
    ret_.intret =-1;
    if (catagory == 1)
    {for(int j=Koopa_blocklist.size()-1;j>=0;j--){
      auto i = Koopa_blocklist[j].find(ident);
      if(i==Koopa_blocklist[j].end()){
        continue;
      }
      if (i->second.shape.size() == 0) //not an array
        {if(i->second.cst==1){//常量
          number=i->second.value;
          ret_.strret = std::to_string(number);
          break;
        }
        else{//变量
          if(!flag){
            std::string tmp="%"+std::to_string(Koopa_Index+1);
            Koopa_Index+=1;
            out<<"\t"+tmp+" = load "+i->second.name+"\n";
            ret_.strret = tmp;
            break;
          }
          else{
            ret_.strret = i->second.name;
            break;
          }
        }}
      else{
        std::string tmp = "%"+std::to_string(Koopa_Index+1);
        Koopa_Index+=1;
        std::string name;
        std::vector<int> shape;
        for(int j=Koopa_blocklist.size()-1;j>=0;j--){
          auto i = Koopa_blocklist[j].find(ident);
          if(i!=Koopa_blocklist[j].end()){
            name = i->second.name;
            shape=i->second.shape;
            break;
          }
        }
        if (shape[0]==-1){
          out<<"\t"<<tmp<<" = load "<<name<<"\n";
          ret_.strret = tmp;
        }
        else {
          out<<"\t"<<tmp<<" = getelemptr "<<name<<" , 0\n";
          ret_.strret = tmp;
        }
      }
        
    }
    }
    else{
      std::vector<std::string> reg_index;
      std::vector<int> shape;
      std::string name;
      for(auto &exp : exps){
        reg_index.push_back((exp->ToIR(out)).strret);
      }
      for (int j = Koopa_blocklist.size()-1;j>=0;j--){
        auto i = Koopa_blocklist[j].find(ident);
        if(i == Koopa_blocklist[j].end()){
          continue;
        }
        else{
          name=i->second.name;
          shape=i->second.shape;
        }
        break;
      }
      std::string tmp;
      if(shape.size()!=0 && shape[0] == -1){
        std::vector<int> slen(shape.begin()+1,shape.end());
        std::string tmp1 = "%"+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp1<<" = load "<<name<<"\n";
        std::string tmp2 = "%"+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp2<<" = getptr "<<tmp1<<" , "<<reg_index[0]<<"\n";
        if(reg_index.size()>1){
          tmp = my_getptr(out,tmp2,std::vector<std::string>(reg_index.begin()+1,reg_index.end()));
        }
        else{
          tmp=tmp2;
        }
      }
      else{
        tmp = my_getptr(out,name, reg_index);
      }
      if (reg_index.size()<shape.size()){
        std::string tmp1 = "%" + to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp1<<" = getelemptr "<<tmp<<" , 0\n";
        ret_.strret = tmp1;
      }
      else if(flag){
        ret_.strret = tmp;
      }
      else {
        std::string tmp2 = "%"+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp2<<" = load "<<tmp<<"\n";
        ret_.strret = tmp2;
      }
    }
    return ret_;
  }
};

class StmtAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<LValAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> ifstmt;
    std::unique_ptr<BaseAST> elsestmt;
    void Dump() const override {

    }
    int cal_val() const override{
    return 0;
  }
    ret ToIR(std::ostream& out) const override {
      struct ret ret_;
      if (is_ret){
        return ret_;
      }
      if (catagory==2){
        struct ret tmp =exp->ToIR(out);
        out << "\tret " << tmp.strret <<"\n";
        is_ret=1;
      }
      else if (catagory==1){
        struct ret tmp =exp->ToIR(out);
        struct ret tmp0 =lval->ToIR2(out,1);
        out << "\tstore " <<tmp.strret<<", "<<tmp0.strret<<"\n";
      }
      else if (catagory == 3){
        block->ToIR(out);
      }
      else if (catagory == 4){
        out <<"\tret\n";
        is_ret=1;
      }
      else if (catagory ==5){
        exp->ToIR(out);
      }
      else if (catagory == 6){

      }
      else if (catagory == 7){
        std::string thenblock="%then_"+to_string(Block_Index);
        std::string elseblock="%else_"+to_string(Block_Index);
        std::string endblock="%end_"+to_string(Block_Index);
        Block_Index+=1;
        struct ret tmp = exp->ToIR(out);
        
        out<<"\tbr "<<tmp.strret<<", "<<thenblock<<", "<<elseblock<<"\n";
        out<<thenblock<<":\n";
        is_ret=0;
        ifstmt->ToIR(out);
        if(!is_ret){
          out<<"\tjump "<<endblock<<"\n";
        }
        out<<elseblock<<":\n";
        is_ret=0;
        elsestmt->ToIR(out);
        if(!is_ret){
          out<<"\tjump "<<endblock<<"\n";
        }
        is_ret=0;
        out<<endblock<<":\n";
      }
      else if (catagory == 8){
        std::string thenblock="%then_"+to_string(Block_Index);
        std::string endblock="%end_"+to_string(Block_Index);
        Block_Index+=1;
        struct ret tmp = exp->ToIR(out);
        
        out<<"\tbr "<<tmp.strret<<", "<<thenblock<<", "<<endblock<<"\n";
        out<<thenblock<<":\n";
        is_ret=0;
        ifstmt->ToIR(out);
        if(!is_ret){
          out<<"\tjump "<<endblock<<"\n";
        }
        is_ret=0;
        out<<endblock<<":\n";
      }
      else if (catagory == 9){
        std::string whileentry="%while_entry_"+to_string(Block_Index);
        std::string whilebody="%while_body_"+to_string(Block_Index);
        std::string whileend="%end_"+to_string(Block_Index);
        Block_Index+=1;
        struct whileinfo while_;
        while_.entry=whileentry;
        while_.body=whilebody;
        while_.end=whileend;
        while_stack.push_back(while_);
        out << "\tjump "<<whileentry<<"\n";
        out<< whileentry<<":\n";
        is_ret = 0;
        struct ret tmp = exp->ToIR(out);
        out<<"\tbr "<<tmp.strret<<", "<<whilebody<<", "<<whileend<<"\n";
        out<<whilebody<<":\n";
        is_ret = 0;
        std::unordered_map<std::string, struct valinfo> block_val_list;
        Koopa_blocklist.push_back(block_val_list);
        stmt->ToIR(out);
        if(!is_ret){
          out<<"\tjump "<<whileentry<<"\n";
        }
        out<<whileend<<":\n";
        Koopa_blocklist.pop_back();
        Koopa_blocklist.shrink_to_fit();
        while_stack.pop_back();
        while_stack.shrink_to_fit();
        is_ret=0;



      }
      else if (catagory == 10){
        struct whileinfo while_now = while_stack[while_stack.size()-1];
        out<<"\tjump "<<while_now.end<<"\n";
        is_ret = 1;
      }
      else if (catagory == 11){
        struct whileinfo while_now = while_stack[while_stack.size()-1];
        out<<"\tjump "<<while_now.entry<<"\n";
        is_ret = 1;
      }
      return ret_;
    }
};



class ExpAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> lorexp;
    std::vector<unique_ptr<BaseAST>> exps;
  void Dump() const override {
        std::cout << "ExpAST { ";
        lorexp->Dump();
        std::cout << " }";
    }
  int cal_val() const override{
    return lorexp->cal_val();
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    struct ret tmp = lorexp->ToIR(out);
    ret_.strret=tmp.strret;
    ret_.intret=tmp.intret;
    return ret_;
    }
};

class PrimaryExpAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> exp;
    int number;
    int catagory;
    std::unique_ptr<LValAST> lval;
  void Dump() const override {
        std::cout << "PrimaryExpAST { ";
        if (catagory == 1){
          exp->Dump();
        }
        else if (catagory == 2){
          std::cout << number ;
        }

        std::cout << " }";
    }
  int cal_val() const override{
    if (catagory == 1){
      return exp->cal_val();
    }
    else if (catagory == 2){
      return number;
    }
    else if (catagory == 3){
      return lval->cal_val();
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp=exp->ToIR(out);
      ret_.strret = tmp.strret;
      ret_.intret=tmp.intret;
    }
    else if (catagory == 2){
      ret_.strret = to_string(number);
      ret_.intret = number;
    }
    else if (catagory == 3){
      struct ret tmp=lval->ToIR2(out, 0);
      ret_.strret = (tmp).strret;
      ret_.intret=(tmp).intret;
    }
    return ret_;
  }
  
};

class UnaryExpAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> primaryexp;
    char unaryop;
    std::unique_ptr<BaseAST> unaryexp;
    std::string ident;
    std::unique_ptr<FuncRParamsAST> funcrparams;
  void Dump() const override {
        std::cout << "UnaryExpAST { ";
        if (catagory == 1){
          primaryexp->Dump();
        }
        else if (catagory == 2){
          std::cout << unaryop<<", " ;
          unaryexp->Dump();
        }

        std::cout << " }";
    }
  int cal_val() const override{
    if (catagory == 1){
      return primaryexp->cal_val();
    }
    else if (catagory ==2){
      int tmp0 = unaryexp->cal_val();
      if(unaryop == '+') {return tmp0;}
      else if (unaryop == '-'){
        return -tmp0;
      }
      else if (unaryop == '!'){
        return !tmp0;
      }
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp = (primaryexp->ToIR(out));
      ret_.strret = tmp.strret;
      ret_.intret=tmp.intret;
    }
    else if (catagory == 2){
      struct ret tmp0 = unaryexp->ToIR(out);
      if (unaryop == '+'){
        ret_.strret=tmp0.strret;
        ret_.intret=tmp0.intret;
      }
      else if (unaryop == '-'){

          out<<std::string("\t%")+to_string(Koopa_Index+1)+" = sub 0, "+tmp0.strret+"\n";
          Koopa_Index+=1;
          ret_.strret= std::string("%")+to_string(Koopa_Index);
          ret_.intret=-tmp0.intret;
      }
      else if (unaryop == '!'){

          out<<std::string("\t%")+to_string(Koopa_Index+1)+" = eq "+tmp0.strret+", 0\n";
          Koopa_Index+=1;
          ret_.strret= std::string("%")+to_string(Koopa_Index);
          ret_.intret=!tmp0.intret;

      }
    }
    else if (catagory == 3){
      auto func_info=func_list.find(ident);
      std::string tmp;
      if(!func_info->second.functype.compare("int")){
        tmp="%"+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp<<" = call "<<ident<<"()\n";
      }
      else {
        out<<"\tcall "<<ident<<"()\n";
      }
      ret_.strret=tmp;
    }
    else if (catagory == 4){
      auto func_info=func_list.find(ident);
      std::string tmp;
      std::vector<std::string> param_values;
      if((funcrparams->explist).empty()) funcrparams->get_explist();
      auto &paramlist=funcrparams->explist;
      for (int j=0;j<paramlist.size();j++){
        param_values.push_back((paramlist[j]->ToIR(out)).strret);
      }
      if(!func_info->second.functype.compare("int")){
        tmp="%"+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp<<" = call "<<ident<<"(";
        //out<<endl;
        for (int i = 0;i<param_values.size()-1;i++){
          out<<param_values[i]<<", ";
          //out<<endl;
        }
        out<<param_values[param_values.size()-1]<<")\n";
      }
      else {
        out<<"\tcall "<<ident<<"(";
        for (int i = 0;i<param_values.size()-1;i++){
          out<<param_values[i]<<", ";
        }
        out<<param_values[param_values.size()-1]<<")\n";
      }
      ret_.strret=tmp;

    }
    return ret_;
  }
};

class MulExpAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> unaryexp;
    std::unique_ptr<BaseAST> mulexp;
    char op;
  void Dump() const override {
    
  }
  int cal_val() const override{
    if (catagory == 1){
      return unaryexp->cal_val();
    }
    else if (catagory  ==2 ){
      int tmp0=mulexp->cal_val();
      int tmp1=unaryexp->cal_val();
      if (op=='*'){
        return tmp0*tmp1;
      }
      else if (op=='/'){
        return tmp0/tmp1;
      }
      else if (op=='%'){
        return tmp0%tmp1;
      }
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp_ = (unaryexp->ToIR(out));
      ret_.strret=tmp_.strret;
      ret_.intret=tmp_.intret;
    }
    else if (catagory == 2){
      struct ret left = mulexp->ToIR(out);
      struct ret right = unaryexp->ToIR(out);
      if (op == '*'){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = mul "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret= std::string("%")+to_string(Koopa_Index);
        ret_.intret= left.intret*right.intret;
      }
      else if (op == '/'){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = div "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret= std::string("%")+to_string(Koopa_Index);
        ret_.intret= left.intret/right.intret;
      }
      else if (op == '%'){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = mod "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret= std::string("%")+to_string(Koopa_Index);
        ret_.intret= left.intret%right.intret;
      }
    }
    return ret_;
  }
};

class AddExpAST : public BaseAST{
  public:
    int  catagory;
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> mulexp;
    char op;

  void Dump() const override {
    
  }
  int cal_val() const override{
    if (catagory == 1){
      return mulexp->cal_val();
    }
    else if (catagory  ==2 ){
      int tmp0=addexp->cal_val();
      int tmp1=mulexp->cal_val();
      if (op=='+'){
        return tmp0+tmp1;
      }
      else if (op=='-'){
        return tmp0-tmp1;
      }
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp_=(mulexp->ToIR(out));
      ret_.strret = tmp_.strret;
      ret_.intret = tmp_.intret;
    }
    else if (catagory == 2){
      struct ret left = addexp->ToIR(out);
      struct ret right = mulexp->ToIR(out);
      if (op == '+'){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = add "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret= std::string("%")+to_string(Koopa_Index);
        ret_.intret = left.intret+right.intret;
      }
      else if (op == '-'){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = sub "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret =  std::string("%")+to_string(Koopa_Index);
        ret_.intret = left.intret-right.intret;
      }
    }
    return ret_;
  }
};

class RelExpAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> relexp;
    std::string op;
  
  void Dump() const override {
    
  }
  int cal_val() const override{
    if (catagory == 1){
      return addexp->cal_val();
    }
    else if (catagory  ==2 ){
      int tmp0=relexp->cal_val();
      int tmp1=addexp->cal_val();
      if (op=="<"){
        return tmp0<tmp1;
      }
      else if (op==">"){
        return tmp0>tmp1;
      }
      else if (op==">="){
        return tmp0>=tmp1;
      }
      else if (op=="<="){
        return tmp0<=tmp1;
      }
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp_=(addexp->ToIR(out));
      ret_.strret=tmp_.strret;
      ret_.intret=tmp_.intret;
    }
    else if (catagory == 2){
      struct ret left = relexp->ToIR(out);
      struct ret right = addexp->ToIR(out);
      if (op == "<"){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = lt "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret =  std::string("%")+to_string(Koopa_Index);
        ret_.intret = (left.intret<right.intret);
      }
      else if (op == ">"){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = gt "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret = std::string("%")+to_string(Koopa_Index);
        ret_.intret = (left.intret>right.intret);
      }
      else if (op == "<="){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = le "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret = std::string("%")+to_string(Koopa_Index);
        ret_.intret = (left.intret<=right.intret);
      }
      else if (op == ">="){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = ge "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret = std::string("%")+to_string(Koopa_Index);
        ret_.intret = (left.intret>=right.intret);
      }
    }
    return ret_;
  }
};

class EqExpAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> relexp;
    std::unique_ptr<BaseAST> eqexp;
    std::string op;
  
  void Dump() const override {
    
  }
  int cal_val() const override{
    if (catagory == 1){
      return relexp->cal_val();
    }
    else if (catagory  ==2 ){
      int tmp0=eqexp->cal_val();
      int tmp1=relexp->cal_val();
      if (op=="=="){
        return tmp0==tmp1;
      }
      else if (op=="!="){
        return tmp0!=tmp1;
      }
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp_ =(relexp->ToIR(out));
      ret_.strret= tmp_.strret;
      ret_.intret = tmp_.intret;
    }
    else if (catagory == 2){
      struct ret left = eqexp->ToIR(out);
      struct ret right = relexp->ToIR(out);
      if (op == "=="){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = eq "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret = std::string("%")+to_string(Koopa_Index);
        ret_.intret = (left.intret==right.intret);
      }
      else if (op == "!="){
        out<<std::string("\t%")+to_string(Koopa_Index+1)+" = ne "+left.strret+ ", "+right.strret+"\n";
        Koopa_Index+=1;
        ret_.strret =  std::string("%")+to_string(Koopa_Index);
        ret_.intret = (left.intret!=right.intret);
      }
    }
    return ret_;
  }
};

class LAndExpAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> eqexp;
    std::string op;
  
  void Dump() const override {
    
  }
  int cal_val() const override{
    if (catagory == 1){
      return eqexp->cal_val();
    }
    else if (catagory  ==2 ){
      int tmp0=landexp->cal_val();
      int tmp1=eqexp->cal_val();
      if (op=="&&"){
        return tmp0&&tmp1;
      }

    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp_= (eqexp->ToIR(out));
      ret_.strret = tmp_.strret;
      ret_.intret = tmp_.intret;
    }
    else if (catagory == 2){
      
      
      if (op == "&&"){
        struct ret left = landexp->ToIR(out);
        std::string thenblock="%then_"+to_string(Block_Index);
        std::string endblock="%end_"+to_string(Block_Index);
        Block_Index+=1;
        std::string retn="@midret_"+to_string(Val_Index);
        Val_Index+=1;
        out<<"\t"<<retn<<" = alloc i32\n";
        out<<"\tstore 0, "<<retn<<"\n";
        out<<"\tbr "+left.strret<<", "<<thenblock<<", "<<endblock<<"\n";
        is_ret =0;
        out<<thenblock<<":\n";
        struct ret right = eqexp->ToIR(out);
        std::string tmp1=std::string("%")+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp1+" = ne "+right.strret+ ", 0\n";
        out<<"\tstore "<<tmp1<<", "<<retn<<"\n";
        out<<"\tjump "<<endblock<<"\n";
        is_ret = 0;
        out<< endblock<<":\n";
        std::string tmp0=std::string("%")+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp0<<" = load "<<retn<<"\n";
        
        ret_.strret =  tmp0;
        ret_.intret = (left.intret && right.intret);
      }
    }
    return ret_;
  }
};

class LOrExpAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> lorexp;
    std::string op;
  
  void Dump() const override {
    
  }
  int cal_val() const override{
    if (catagory == 1){
      return landexp->cal_val();
    }
    else if (catagory  ==2 ){
      int tmp0=lorexp->cal_val();
      int tmp1=landexp->cal_val();
      if (op=="||"){
        return tmp0||tmp1;
      }
    }
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    if (catagory == 1){
      struct ret tmp_=(landexp->ToIR(out));
      ret_.strret = tmp_.strret;
      ret_.intret = tmp_.intret;
    }
    else if (catagory == 2){

      if (op == "||"){


        struct ret left = lorexp->ToIR(out);
        std::string thenblock="%then_"+to_string(Block_Index);
        std::string endblock="%end_"+to_string(Block_Index);
        Block_Index+=1;
        std::string retn="@midret_"+to_string(Val_Index);
        Val_Index+=1;
        out<<"\t"<<retn<<" = alloc i32\n";
        out<<"\tstore 1, "<<retn<<"\n";
        out<<"\tbr "+left.strret<<", "<<endblock<<", "<<thenblock<<"\n";
        is_ret =0;
        out<<thenblock<<":\n";
        struct ret right = landexp->ToIR(out);
        std::string tmp1=std::string("%")+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp1+" = ne "+right.strret+ ", 0\n";
        out<<"\tstore "<<tmp1<<", "<<retn<<"\n";
        out<<"\tjump "<<endblock<<"\n";
        is_ret = 0;
        out<< endblock<<":\n";
        std::string tmp0=std::string("%")+to_string(Koopa_Index+1);
        Koopa_Index+=1;
        out<<"\t"<<tmp0<<" = load "<<retn<<"\n";
        
        ret_.strret =  tmp0;
        ret_.intret = (left.intret && right.intret);
      }
    }
    return ret_;
  }
};

class ConstExpAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> exp;
  void Dump() const override {
    std::cout<<"constexp: "<<exp->cal_val();
  }
  int cal_val() const override{

    return exp->cal_val();
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    ret_.intret=exp->cal_val();
    return ret_;
  }
};

class InitValAST : public BaseAST{
  public :
  int catagory;
  std::unique_ptr<BaseAST> exp;
  std::vector<unique_ptr<InitValAST>> initvals;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return exp->cal_val();
  }
  ret ToIR(std::ostream& out) const override{
    return exp->ToIR(out);
  }
  void ToIR2(std::ostream& out, std::string *initval, const std::vector<int> &shape, int global){
    int n = shape.size();
    //out<<"in"<<endl;
    std::vector<int> width(n);
    width[n-1]=shape[n-1];
    for(int i =n-2; i>=0; i--){
      width[i]=width[i+1]*shape[i];
    }
    int i = 0;
    //out<<"hh "<<n<<endl;
    for(auto &init_val : initvals){
      //out<<init_val->catagory<<endl;
      if (init_val->catagory == 1){
        //out<<"hhhhh"<<endl;
        if(global)
        initval[i++] = to_string(init_val->cal_val());
        else initval[i++] = (init_val->ToIR(out)).strret;
      }
      else {
        int j = 1;
        if (i!=0){
          j = n-1;
          for(;j>=0;--j){
            if (i%width[j]!=0) break;
          }
          j+=1;
        }
        init_val->ToIR2(out, initval+i, std::vector<int>(shape.begin()+j,shape.end()), global);
        i+=width[j];
      }
      if (i>=width[0]) break;
    }

  }
};

class ConstInitValAST : public BaseAST{
  public:
  int catagory;
  std::unique_ptr<BaseAST> constexp;
  std::vector<unique_ptr<ConstInitValAST>> constinitvals;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return constexp->cal_val();
  }
  ret ToIR(std::ostream& out) const override{
    return constexp->ToIR(out);
  }

  void ToIR2(std::ostream& out, std::string *initval, const std::vector<int> &shape){
    int n = shape.size();
    std::vector<int> width(n);
    width[n-1]=shape[n-1];
    for(int i =n-2; i>=0; i--){
      width[i]=width[i+1]*shape[i];
    }
    int i = 0;
    for(auto &init_val : constinitvals){
      if (init_val->catagory == 1){
        initval[i++] = to_string((init_val->ToIR(out)).intret);
        //out<<initval[i-1]<<endl;
      }
      else {
        int j = 1;
        if (i!=0){
          j = n-1;
          for(;j>=0;--j){
            if (i%width[j]!=0) break;
          }
          j+=1;
        }
        init_val->ToIR2(out, initval+i, std::vector<int>(shape.begin()+j,shape.end()));
        i+=width[j];
      }
      if (i>=width[0]) break;
    }

  }
};
class VarDefAST : public BaseAST{
  public:
  int catagory;
  std::string ident;
  std::unique_ptr<InitValAST> initval;
  std::vector<unique_ptr<BaseAST>> constexps;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    int block_index=Koopa_blocklist.size()-1;
    if (catagory==1){
      struct valinfo val;
      val.init=0;
      val.cst=0;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
      Koopa_blocklist[block_index].insert(make_pair(ident,val));
      out<<"    "<<val.name<<" = alloc i32\n";
    }
    else if (catagory==2){
      struct valinfo val;
      val.init=1;
      val.cst=0;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
      Koopa_blocklist[block_index].insert(make_pair(ident,val));
      struct ret tmp0=initval->ToIR(out);
      out<<"\t"<<val.name<<" = alloc i32\n";
      out<<"\tstore "<<tmp0.strret<<", "<<val.name<<"\n";
    }

    return ret_;
  }
  std::string my_getInitList(std::string *initval, const std::vector<int> &shape){
    std::string ret = "{";
    if(shape.size() == 1){
        int n = shape[0];
        ret += initval[0];
        for(int i = 1; i < n; i++){
          ret += ", " + initval[i];
        }
    } 
    else{
      int n = shape[0], width = 1;
      std::vector<int> sublen(shape.begin() + 1, shape.end());
      for(auto iter = shape.end() - 1; iter != shape.begin(); --iter)width *= *iter;
        ret += my_getInitList(initval, sublen);
        for(int i = 1; i < n; ++i){
          ret += ", " + my_getInitList(initval + width * i, sublen);
        }
      }
      ret += "}";
      return ret;
    }
  void my_Initarray(std::ostream& out,std::string name, std::string *initval, const std::vector<int> &shape){    
    int n = shape[0];
    if(shape.size() == 1){
        for(int i = 0; i < n; ++i){
            std::string tmp = "%"+std::to_string(Koopa_Index+1);
            Koopa_Index+=1;
            out<<"\t"<<tmp<<" = getelemptr "<<name<<" , "<<i<<"\n";
            out<<"\tstore "<<initval[i]<<" , "<<tmp<<"\n";

        }
    } else {
        std::vector<int> sublen(shape.begin() + 1, shape.end());
        int width = 1;
        for(auto l : sublen)  width *= l;
        for(int i = 0; i < n; ++i){
            std::string tmp = "%"+std::to_string(Koopa_Index+1);
            Koopa_Index+=1;
            out<<"\t"<<tmp<<" = getelemptr "<<name<<" , "<<i<<"\n";
            my_Initarray(out,tmp, initval + i * width, sublen);
        }
    }
}
  void ToIR2(std::ostream& out, int global) {
    int block_index=Koopa_blocklist.size()-1;
    if (catagory==1){
      struct valinfo val;
      val.init=0;
      val.cst=0;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
      Koopa_blocklist[block_index].insert(make_pair(ident,val));
      if(global){out<<"global "<<val.name<<" = alloc i32, zeroinit\n";}
      else {out<<"    "<<val.name<<" = alloc i32\n";}
    }
    else if (catagory==2){
      struct valinfo val;
      val.init=1;
      val.cst=0;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
      Koopa_blocklist[block_index].insert(make_pair(ident,val));
      struct ret tmp0=initval->ToIR(out);
      if(global){out<<"global "<<val.name<<" = alloc i32 ,"<<initval->cal_val()<<"\n";}
      else{
        out<<"\t"<<val.name<<" = alloc i32\n";
        out<<"\tstore "<<tmp0.strret<<", "<<val.name<<"\n";
      }
    }
    else{
      //out<<"eee"<<endl;
      struct valinfo val;
      val.cst=2;
      val.init=1;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
      std::vector<int> shape;
      int all_size=1;
      //out<<"eee"<<endl;
      //out<<constexps.size()<<endl;
      for (int i=0;i<constexps.size();i++){
        shape.push_back((constexps[i]->ToIR(out)).intret);
        all_size*=shape[i];
      }
      //out<<"ee"<<endl;
      val.shape=shape;
      int block_index=Koopa_blocklist.size()-1;
      Koopa_blocklist[block_index].insert(make_pair(ident,val));
      std::string type="i32";
      for (int i=shape.size()-1;i>=0;i--){
        type = "[" + type + ", " + to_string(shape[i]) + "]";
      }
      std::string *initvec=new std::string[all_size];
      for (int i= 0; i<all_size;i++){
        initvec[i] = "0";
      }
      if (global){
        if (catagory == 4){
          if (initval->catagory == 3){
            out<<"global "<<val.name<<" = alloc "<<type<<" , zeroinit\n";
          }
          else{
            initval->ToIR2(out, initvec, shape, global);
            out<<"global "<<val.name<<" = alloc "<<type<<" , "<<my_getInitList(initvec,shape)<<"\n";
          }
          
        }
        else if (catagory == 3){
          out<<"global "<<val.name<<" = alloc "<<type<<" , zeroinit\n";
        }
        
      }

      else {
        out << "\t"<<val.name<<" = alloc "<<type<<"\n";
        if(catagory==3){
          return;
        }
        initval->ToIR2(out, initvec, shape, 0);
        my_Initarray(out, val.name, initvec, shape);
      }
    }
  }
};

class VarDefsAST : public BaseAST{
  public:
  int catagory;
  std::unique_ptr<VarDefAST> vardef;
  std::unique_ptr<VarDefsAST> vardefs;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    if (catagory==1){
      vardef->ToIR2(out,0);
    }
    else if (catagory==2){
      vardef->ToIR2(out,0);
      vardefs->ToIR(out);
    }

    return ret_;
  }
  void ToIR2(std::ostream& out, int global){
    if (catagory==1){
      vardef->ToIR2(out,global);
    }
    else if (catagory==2){
      vardef->ToIR2(out,global);
      vardefs->ToIR2(out,global);
    }
  }
};
class VarDeclAST : public BaseAST{
  public:
  std::unique_ptr<VarDefsAST> vardefs;
  std::string btype;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    return vardefs->ToIR(out);
  }

  void ToIR2(std::ostream& out, int global){
    vardefs->ToIR2(out,global);
  }
};
class ConstDefAST : public BaseAST{
  public:
  int catagory;
  std::unique_ptr<ConstInitValAST> constinitval;
  std::vector<unique_ptr<BaseAST>> constexps;
  std::string ident;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    if (catagory == 1)
    {struct valinfo val;
    val.cst=1;
    val.init=1;
    val.name=ident+"_"+to_string(Val_Index);
    Val_Index+=1;
    constinitval->ToIR(out);
    val.value= constinitval->cal_val();
    int block_index=Koopa_blocklist.size()-1;
    Koopa_blocklist[block_index].insert(make_pair(ident,val));}
    else if (catagory == 2){
      struct valinfo val;
      val.cst=1;
      val.init=1;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
    }
    return ret_;
  }
  std::string my_getInitList(std::string *initval, const std::vector<int> &shape){
    std::string ret = "{";
    if(shape.size() == 1){
        int n = shape[0];
        ret += initval[0];
        for(int i = 1; i < n; i++){
          ret += ", " + initval[i];
        }
    } 
    else{
      int n = shape[0], width = 1;
      std::vector<int> sublen(shape.begin() + 1, shape.end());
      for(auto iter = shape.end() - 1; iter != shape.begin(); --iter)width *= *iter;
        ret += my_getInitList(initval, sublen);
        for(int i = 1; i < n; ++i){
          ret += ", " + my_getInitList(initval + width * i, sublen);
        }
      }
      ret += "}";
      return ret;
    }
  void my_Initarray(std::ostream& out,std::string name, std::string *initval, const std::vector<int> &shape){    
    int n = shape[0];
    if(shape.size() == 1){
        for(int i = 0; i < n; ++i){
            std::string tmp = "%"+std::to_string(Koopa_Index+1);
            Koopa_Index+=1;
            out<<"\t"<<tmp<<" = getelemptr "<<name<<" , "<<i<<"\n";
            out<<"\tstore "<<initval[i]<<" , "<<tmp<<"\n";

        }
    } else {
        std::vector<int> sublen(shape.begin() + 1, shape.end());
        int width = 1;
        for(auto l : sublen)  width *= l;
        for(int i = 0; i < n; ++i){
            std::string tmp = "%"+std::to_string(Koopa_Index+1);
            Koopa_Index+=1;
            out<<"\t"<<tmp<<" = getelemptr "<<name<<" , "<<i<<"\n";
            my_Initarray(out,tmp, initval + i * width, sublen);
        }
    }
  }
  void ToIR2(std::ostream& out, int global){
    //out<<"ho"<<endl;
    if (catagory == 1)
    {struct valinfo val;
    val.cst=1;
    val.init=1;
    val.name=ident+"_"+to_string(Val_Index);
    Val_Index+=1;
    constinitval->ToIR(out);
    val.value= constinitval->cal_val();
    int block_index=Koopa_blocklist.size()-1;
    Koopa_blocklist[block_index].insert(make_pair(ident,val));}
    else if (catagory == 2){
      struct valinfo val;
      val.cst=1;
      val.init=1;
      val.name=ident+"_"+to_string(Val_Index);
      Val_Index+=1;
      std::vector<int> shape;
      int all_size=1;
      //out<<"h"<<endl;
      for (int i=0;i<constexps.size();i++){
        shape.push_back((constexps[i]->ToIR(out)).intret);
        all_size*=shape[i];
      }
      val.shape=shape;
      int block_index=Koopa_blocklist.size()-1;
      Koopa_blocklist[block_index].insert(make_pair(ident,val));
      std::string type="i32";
      for (int i=shape.size()-1;i>=0;i--){
        type = "[" + type + ", " + to_string(shape[i]) + "]";
      }
      std::string *initvec=new std::string[all_size];
      for (int i= 0; i<all_size;i++){
        initvec[i] = "0";
      }
      //out<<"hhh"<<endl;
      constinitval->ToIR2(out,initvec,shape);
      //out<<"eee"<<endl;
      if (global){
        out << "global "<<val.name<<" = alloc "<<type<<" , "<<my_getInitList(initvec,shape)<<"\n";
        //out<<endl;
      }
      else {
        out << "\t"<<val.name<<" = alloc "<<type<<"\n";
        my_Initarray(out, val.name, initvec, shape);
      }
    }
    return;
  }
};
class ConstDefsAST : public BaseAST{
  public:
  int catagory;
  std::unique_ptr<ConstDefAST> constdef;
  std::unique_ptr<ConstDefsAST> constdefs;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    if (catagory==1){
      constdef->ToIR2(out,0);
    }
    else if (catagory==2){
      constdef->ToIR2(out,0);
      constdefs->ToIR(out);
    }

    return ret_;
  }
  void ToIR2(std::ostream& out ,int global){
    if (catagory==1){
      constdef->ToIR2(out,global);
    }
    else if (catagory==2){
      constdef->ToIR2(out,global);
      constdefs->ToIR2(out,global);
    }
  }

};
class ConstDeclAST : public BaseAST{
  public:
    std::string btype;
    std::unique_ptr<ConstDefsAST> constdefs;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    constdefs->ToIR(out);
    return ret_;
  }
  void ToIR2(std::ostream& out, int global){
    constdefs->ToIR2(out,global);
  }
};
class DeclAST : public BaseAST {
  public:
    int catagory;
    std::unique_ptr<ConstDeclAST> constdecl;
    std::unique_ptr<VarDeclAST> vardecl;
  void Dump() const override {
    
  }
  int cal_val() const override{

    return 0;
  }
  ret ToIR(std::ostream& out) const override{
    struct ret ret_;
    if (catagory==1){
      constdecl->ToIR(out);
    }
    else if (catagory==2){
      vardecl->ToIR(out);
    }
    return ret_;
  }
  void ToIR2(std::ostream& out,int global){
    if (catagory==1){
      constdecl->ToIR2(out,global);
    }
    else if (catagory==2){
      vardecl->ToIR2(out,global);
    }
  }
};
// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::vector<std::unique_ptr<BaseAST>> funcdefs;
  std::vector<std::unique_ptr<DeclAST>> decls;
  //int catagory;
  //std::unique_ptr<BaseAST> func_def;
  //std::unique_ptr<BaseAST> compunit;
  //std::unique_ptr<DeclAST> decl;
  //std::unique_ptr<BaseAST> declorfuncdef;
  void Dump() const override {
    std::cout << "CompUnitAST {} ";

  }
  int cal_val() const override{
    return 0;
  }
  ret ToIR(std::ostream& out) const override {
    struct ret ret_;
    std::unordered_map<std::string,struct valinfo> global_val_list;
    Koopa_blocklist.push_back(global_val_list);
    //out<<decls.size()<<endl;
    for (int i=0; i<decls.size();i++){
      //out<<"oh\n"<<endl;
      decls[i]->ToIR2(out,1);
    }
    if (!global_libfunc){
      out<<"decl @getint(): i32\n"
      <<"decl @getch(): i32\n"
      <<"decl @getarray(*i32): i32\n"
      <<"decl @putint(i32)\n"
      <<"decl @putch(i32)\n"
      <<"decl @putarray(i32, *i32)\n"
      <<"decl @starttime()\n"
      <<"decl @stoptime()\n\n";
      struct funcinfo func_info;
      func_info.name="@getint";
      func_info.functype="int";
      func_list.insert(make_pair("@getint",func_info));
      struct funcinfo func_info2;
      func_info2.name="@getch";
      func_info2.functype="int";
      func_list.insert(make_pair("@getch",func_info2));
      struct funcinfo func_info3;
      func_info3.name="@getarray";
      func_info3.functype="int";
      func_list.insert(make_pair("@getarray",func_info3));
      struct funcinfo func_info4;
      func_info4.name="@putint";
      func_info4.functype="void";
      func_list.insert(make_pair("@putint",func_info4));
      struct funcinfo func_info5;
      func_info5.name="@putch";
      func_info5.functype="void";
      func_list.insert(make_pair("@putch",func_info5));
      struct funcinfo func_info6;
      func_info6.name="@putarray";
      func_info6.functype="void";
      func_list.insert(make_pair("@putarray",func_info6));
      struct funcinfo func_info7;
      func_info7.name="@starttime";
      func_info7.functype="void";
      func_list.insert(make_pair("@starttime",func_info7));
      struct funcinfo func_info8;
      func_info8.name="@stoptime";
      func_info8.functype="void";
      func_list.insert(make_pair("@stoptime",func_info8));
      global_libfunc=1;
    }
    for (int i =0;i<funcdefs.size();i++){
      funcdefs[i]->ToIR(out);
    }
    Koopa_blocklist.pop_back();
    Koopa_blocklist.shrink_to_fit();
    //if (catagory == 1){
    //  compunit->ToIR(out);
    //  func_def->ToIR(out);
    //}
    //else if (catagory ==2){
    //  func_def->ToIR(out);
    //}
    //else if (catagory == 3){
    //  decl->ToIR2(out,1);
    //}
    //else if (catagory == 4){
    //  decl->ToIR2(out,1);
    //}
    return ret_;
  }
};


class BlockItemAST : public BaseAST{
  public:
    int catagory;
    std::unique_ptr<DeclAST> decl;
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override {

    }
    int cal_val() const override{
    return 0;
  }
    ret ToIR(std::ostream& out) const override {
      struct ret ret_;
      if (is_ret){
        return ret_;
      }
      if (catagory==1){
        decl->ToIR(out);
      }
      else if (catagory ==2){
        stmt->ToIR(out);
      }
      return ret_;
    }

};










// ...