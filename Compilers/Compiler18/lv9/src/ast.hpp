#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "symbol_table.hpp"
#include "control_stream.hpp"
using namespace std;
// 所有 AST 的基类
static int KoopaVariableNum = 0;
static int IfBlockNum = 0;
static int WhileBlockNum = 0;
static int decl_global = 0;
static vector<int> WhileBlockChain; 

static string get_if_then_name(int num){
  return "if_then_" + to_string(num);
}
static string get_if_else_name(int num){
  return "if_else_" + to_string(num);
}
static string get_if_exit_name(int num){
  return "if_exit_" + to_string(num);
}
static string get_while_entry_name(int num){
  return "while_entry_" + to_string(num);
}
static string get_while_body_name(int num){
  return "while_body_" + to_string(num);
}
static string get_while_exit_name(int num){
  return "while_exit_" + to_string(num);
}
// 以aggregate的形式对数组进行初始化，类似 {1,2,3,4}...
static void aggregate_array(const string& ident, vector<int>* agg, int agg_pos, vector<int>* dimension_length, 
                            vector<int>* dimension_size, int dimension_pos){
  if (dimension_pos == dimension_length->size()){
    // 最后一维
    cout << agg->at(agg_pos);
  }
  else {
    cout << "{";
    int size = 1;
    if (dimension_pos != dimension_length->size() - 1){
      size = dimension_size->at(dimension_pos + 1);
    }
    for (int i = 0; i < dimension_length->at(dimension_pos); i++){
      aggregate_array(ident, agg, agg_pos + i * size, dimension_length, dimension_size, dimension_pos + 1);
      if (i != dimension_length->at(dimension_pos) - 1){
        cout << ", ";
      }
    }
    cout << "}";
  }
}
// 以store的形式对数组进行初始化，每个元素分别store
static void store_array(const string& ident, vector<int>* agg, int agg_pos, vector<int>* dimension_length, 
                            vector<int>* dimension_size, int dimension_pos){
  if (dimension_pos == dimension_length->size()){
    // 最后一维
    cout << "  store " << agg->at(agg_pos) << ", %" << KoopaVariableNum-1 << endl;
  }
  else {
    int size = 1;
    if (dimension_pos != dimension_length->size() - 1){
      size = dimension_size->at(dimension_pos + 1);
    }
    int cur_reg = KoopaVariableNum - 1;
    for (int i = 0; i < dimension_length->at(dimension_pos); i++){
      cout << "  %" << KoopaVariableNum++ << " = getelemptr ";
      if (dimension_pos == 0){
        // 第一维，直接用ident进行getelemptr
        cout << "@" << get_name(query_symbol(ident).first, ident) << ", " << i << endl;
      }
      else{
        cout << "%" << cur_reg << ", " << i << endl;
      }
      store_array(ident, agg, agg_pos + i * size, dimension_length, dimension_size, dimension_pos + 1);
    }
  }
}

// agg中都是寄存器编号，因为局部的数组初值可以是表达式，需要计算
static void store_array_withreg(const string& ident, vector<int>* agg, int agg_pos, vector<int>* dimension_length, 
                            vector<int>* dimension_size, int dimension_pos){
  if (dimension_pos == dimension_length->size()){
    cout << "  store %" << agg->at(agg_pos) << ", %" << KoopaVariableNum-1 << endl;
  }
  else {
    int size = 1;
    if (dimension_pos != dimension_length->size() - 1){
      size = dimension_size->at(dimension_pos + 1);
    }
    int cur_reg = KoopaVariableNum - 1;
    for (int i = 0; i < dimension_length->at(dimension_pos); i++){
      cout << "  %" << KoopaVariableNum++ << " = getelemptr ";
      if (dimension_pos == 0){
        // 第一维，直接用ident进行getelemptr
        cout << "@" << get_name(query_symbol(ident).first, ident) << ", " << i << endl;
      }
      else{
        cout << "%" << cur_reg << ", " << i << endl;
      }
      store_array_withreg(ident, agg, agg_pos + i * size, dimension_length, dimension_size, dimension_pos + 1);
    }
  }
}

class BaseAST {
 public:
  virtual ~BaseAST() = default;
  // 用来输出
  virtual void KoopaIR() const = 0;
  virtual int Calc() const = 0;
};

static void Lib_Func_KoopaIR(){
  cout<< "decl @getint(): i32" << endl;
  define_func_type("getint", "int");
  cout<< "decl @getch(): i32" << endl;
  define_func_type("getch", "int");
  cout<< "decl @getarray(*i32): i32" << endl;
  define_func_type("getarray", "int");
  cout<< "decl @putint(i32)" << endl;
  define_func_type("putint", "void");
  cout<< "decl @putch(i32)" << endl;
  define_func_type("putch", "void");
  cout<< "decl @putarray(i32, *i32)" << endl;
  define_func_type("putarray", "void");
  cout<< "decl @starttime()" << endl;
  define_func_type("starttime", "void");
  cout<< "decl @stoptime()" << endl;
  define_func_type("stoptime", "void");
  cout<<endl;
}

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  unique_ptr<vector<unique_ptr<BaseAST> > > comp_unit_items;
  void KoopaIR() const override{
    Lib_Func_KoopaIR();
    // 全局域
    enter_code_block();
    for (auto& comp_unit_item: *comp_unit_items){
      comp_unit_item->KoopaIR();
      cout<<endl;
    }
    exit_code_block();
  }
  int Calc()const override{ return -1; }
};

class CompUnitItemAST : public BaseAST {
 public:
  int type;
  unique_ptr<BaseAST> funcdef;
  unique_ptr<BaseAST> decl;
  void KoopaIR() const override{
    if (type == 1)
      funcdef->KoopaIR();
    else if (type == 2){
      decl_global = 1;
      decl->KoopaIR();
      decl_global = 0;
    }
  }
  int Calc()const override{ return -1; }
};

class FuncFParamAST : public BaseAST {
 public:
  int type;
  string btype;
  string ident;
  unique_ptr<vector<unique_ptr<BaseAST> > > array_indexs;
  void KoopaIR() const override{
    if (type == 1){
      cout<<" @"<<ident<<": i32";
    }
    else if (type == 2){
      cout<<" @"<<ident<<": *";
      for (int i = 0; i < array_indexs->size(); i++){
        cout << "[";
      }
      cout << "i32";
      for (int i = array_indexs->size() - 1; i >= 0; i--){
        cout << ", ";
        cout << array_indexs->at(i)->Calc() << "]";
      }
    }
    
  }
  int Calc()const override{return -1;}
  void Alloc() const {
    if (type == 1){
      cout << "  @" << get_name(get_current_block_id(), ident) << " = alloc i32" << endl;
      define_type(ident, "Var");
      insert_value(ident, 0);
      cout << "  store @" << ident << ", @" << get_name(get_current_block_id(), ident) << endl;
    }
    // 数组参数
    else if (type == 2){
      cout << "  @" << get_name(get_current_block_id(), ident) << " = alloc *";
      for (int i = 0; i < array_indexs->size(); i++) {
        cout << "[";
      }
      cout << "i32";
      for (int i = array_indexs->size() - 1; i >= 0; i--) {
        cout << ", ";
        cout << array_indexs->at(i)->Calc() << "]";
      }
      cout << endl;
      define_type(ident, "Pointer");
      insert_value(ident, array_indexs->size() + 1);
      cout << "  store @" << ident << ", @" << get_name(get_current_block_id(), ident) << endl;
    }
  }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  string func_type;  //int or void
  std::string ident;  //main
  unique_ptr<vector<unique_ptr<BaseAST> > > func_fparams;
  std::unique_ptr<BaseAST> block;
  void KoopaIR() const override{
    enter_code_block();
    std::cout << "fun @" << ident << "(";
    int first_param = 1;
    for(auto& func_fparam: *func_fparams) {
      if (first_param == 1){
        first_param = 0;
      }
      else{
        std::cout << ", ";
      }
      func_fparam->KoopaIR();
    }
    cout<<")";
    define_func_type(ident, func_type);
    if (func_type == "int"){
      cout<<": i32";
    }
    std::cout<<"{\n";
    std::cout<<"%entry:\n";
    // 新的流，尚未返回/分支/跳转
    change_return_flag(0);
    
    for(auto& func_fparam: *func_fparams) {
      dynamic_cast<FuncFParamAST*>(func_fparam.get())->Alloc();
    }

    block->KoopaIR();
    
    // 可能会有两个分支都返回了，因此后面没有ret的可能性
    if (get_return_flag() == 0){
      if (func_type == "int"){
        std::cout<<"  ret 233"<<"\n";
      }
      else if (func_type == "void"){
        std::cout<<"  ret"<<"\n";
      }
      change_return_flag(1);
    }
    std::cout<<"}\n";
    exit_code_block();
  }
  int Calc()const override{return -1; }
};

class FuncRParamAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;
  void KoopaIR() const override{
    exp->KoopaIR();
  }
  int Calc()const override{
    return exp->Calc();
  }
};

class BlockAST : public BaseAST {
    public:
        unique_ptr<vector<unique_ptr<BaseAST> > > blockitems;
        void KoopaIR()const override{
          for (auto& blockitem:*blockitems){
            blockitem->KoopaIR();
            if (get_return_flag() == 1)
              break;
          }
        }
        int Calc()const override{return -1;}
};

class BlockItemAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> decl;
    unique_ptr<BaseAST> stmt;
    void KoopaIR() const override{
      if (type == 1){
        // cout<<"here decl";
        decl->KoopaIR();
      }
      else{
        // cout<<"here stmt";
        stmt->KoopaIR();
      }
    }
    int Calc()const override{return -1;}
};

// Decl          ::= ConstDecl | VarDecl;
class DeclAST : public BaseAST {
  public:
    int type;
    unique_ptr<BaseAST> constdecl;
    unique_ptr<BaseAST> vardecl;
    void KoopaIR() const override{
      if (type == 1){
        constdecl->KoopaIR();
      }
      else if (type == 2){
        vardecl->KoopaIR();
      }
    }
    int Calc()const override{return -1;}
};

class VarDeclAST : public BaseAST {
  public:
    string btype;
    unique_ptr<vector<unique_ptr<BaseAST> > > vardefs;
    void KoopaIR() const override{
      for (auto& vardef: *vardefs){
        vardef->KoopaIR();
      }
    }
    int Calc()const override{return -1;}
};

// InitVal       ::= Exp | "{" [InitVal {"," InitVal}] "}";
class InitValAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> exp;
    unique_ptr<vector<unique_ptr<BaseAST> > > exps;
    
    // dimension_size数组应在调用前计算好并传入，内容为每一维的大小
    // 例如[2, 3, 4]表示一个三维数组，第一维大小为2*3*4，第二维大小为3*4，第三维大小为4
    // Aggregate的返回值是一个大小为dimension_size[0]的数组
    const vector<int> Aggregate(vector<int>::iterator dimension_size_begin, vector<int>::iterator dimension_size_end ) const {
      // 先把数组的初始值计算出来，得到一个合法的aggregate
      // cout << "type: " << type << endl;
      // if (type == 2)
      //   cout<< "size: " << exps->size() << endl;
      // else 
      //   cout<< "value: " << exp->Calc() << endl;
      assert(type == 2); 
      // cout << "passed assertion" << endl;
      if (decl_global == 1){
        // 全局数组变量
        vector<int> agg;
        for (auto& tmp: *exps){
          // 先进行类型转换
          auto exp = dynamic_cast<InitValAST*>(tmp.get());
          if (exp->type == 1){ 
            agg.push_back(exp->Calc());
          }
          else if (exp->type == 2){
            // 可能向下多层
            for (auto dimension_size_cur = dimension_size_begin + 1; dimension_size_cur != dimension_size_end; dimension_size_cur++){
              if (agg.size() % (*dimension_size_cur) == 0){
                auto tmp_agg = exp->Aggregate(dimension_size_cur, dimension_size_end);
                agg.insert(agg.end(), tmp_agg.begin(), tmp_agg.end());
                break;
              }
            }
          }
        }
        
        agg.insert(agg.end(), dimension_size_begin[0] - agg.size(), 0);
        return agg;
      }
      else{
        // 局部数组变量，把agg中各项都存到寄存器中
        vector<int> agg;
        for (auto& tmp: *exps){
          // 先进行类型转换
          auto exp = dynamic_cast<InitValAST*>(tmp.get());
          if (exp->type == 1){
            exp->KoopaIR();
            agg.push_back(KoopaVariableNum-1);
          }
          else if (exp->type == 2){
            // 可能向下多层
            for (auto dimension_size_cur = dimension_size_begin + 1; dimension_size_cur != dimension_size_end; dimension_size_cur++){
              if (agg.size() % (*dimension_size_cur) == 0){
                auto tmp_agg = exp->Aggregate(dimension_size_cur, dimension_size_end);
                agg.insert(agg.end(), tmp_agg.begin(), tmp_agg.end());
                break;
              }
            }
          }
        }
        if (agg.size() < dimension_size_begin[0]){
          cout << "  %" << KoopaVariableNum << " = add 0, 0" << endl;
          agg.insert(agg.end(), dimension_size_begin[0] - agg.size(), KoopaVariableNum);
          KoopaVariableNum++;
        }
        return agg;
      }
    }
    void KoopaIR() const override{
      if (type == 1){
        exp->KoopaIR();
      }
      else if (type == 2){
        for (auto& exp: *exps){
          exp->KoopaIR();
        }
      }
    } 
    int Calc() const override{
      return exp->Calc();
    }
};
// VarDef        ::= IDENT | IDENT "=" InitVal;
class VarDefAST : public BaseAST{
  public:
    int type;
    string ident;
    unique_ptr<BaseAST> init_val;
    unique_ptr<vector<unique_ptr<BaseAST> > > array_indexs;
    void KoopaIR() const override {
      if (array_indexs->empty()){
        // global
        if (decl_global == 1){
          cout << "global" << "  @" << get_name(get_current_block_id(), ident) << " = alloc i32, ";
          if (type == 1){
            cout << "zeroinit";
          }
          else {
            cout << init_val->Calc();
          }
          cout<< endl;
        }
        // local
        else {
          cout << "  @" << get_name(get_current_block_id(), ident) << " = alloc i32" << endl;
          if (type == 2) {
            init_val->KoopaIR();
            // cout<<"storing ident "<<ident<<endl;
            // 声明式，直接分配当前块id，不用查
            std::cout << "  store %" << KoopaVariableNum-1 << ", @" << get_name(get_current_block_id(), ident) << std::endl;
          }
        }
        // 变量不用算值，挂上号就行
        insert_value(ident, 0);
        define_type(ident, "Var");
      }
      else{
        // 否则是数组
        insert_value(ident, array_indexs->size());
        define_type(ident, "Array");
        if (decl_global == 1){
          cout << "global ";
        }
        else {
          cout << "  ";
        }
        cout << "@" << get_name(get_current_block_id(), ident) << " = alloc ";
        for (int i = 0; i < array_indexs->size(); i++) {
          cout << "[";
        }
        cout << "i32, ";
        auto dimension_length = new vector<int>();
        auto dimension_size = new vector<int>();
        int tot_dimension_size = 1;
        for (int i = 0; i < array_indexs->size(); i++) {
          int cur_index = array_indexs->at(i)->Calc();
          tot_dimension_size *= cur_index;
          dimension_length->push_back(cur_index); 
        }
        for (int i = 0; i < array_indexs->size(); i++) {
          dimension_size->push_back(tot_dimension_size);
          tot_dimension_size /= dimension_length->at(i);
        }
        // 倒着输出
        for (int i = array_indexs->size() - 1; i >= 0; i--){
          cout << dimension_length->at(i) << "]";
          if (i != 0){
            cout << ", ";
          }
        }
        if (decl_global == 1){
          if (type == 1){
            cout << ", zeroinit" << endl;
          }
          else {
            cout << ", ";
            vector<int> agg = (dynamic_cast<InitValAST*>(init_val.get()))->Aggregate(dimension_size->begin(), dimension_size->end());
            aggregate_array(ident, &agg, 0, dimension_length, dimension_size, 0);
            cout << endl;
          }
        }
        else {
          if (type == 1){
            cout << endl;
          }
          else {
            cout << endl;
            vector<int> agg = dynamic_cast<InitValAST*>(init_val.get())->Aggregate(dimension_size->begin(), dimension_size->end());
            store_array_withreg(ident, &agg, 0, dimension_length, dimension_size, 0);
          }
        }
      }
    }
    int Calc()const override{return -1;}
};


class ConstDeclAST : public BaseAST {
  public:
    string btype;
    unique_ptr<vector<unique_ptr<BaseAST> > > constdefs;
    void KoopaIR() const override{
      for (auto& constdef: *constdefs){
        constdef->KoopaIR();
      }
    }
    int Calc()const override{return -1;}
};


class ConstInitValAST : public BaseAST {
  public :
    int type;
    unique_ptr<BaseAST> constexp;
    unique_ptr<vector<unique_ptr<BaseAST> > > constexps;
    const vector<int> Aggregate(vector<int>::iterator dimension_size_begin, vector<int>::iterator dimension_size_end ) const {
      // 先把数组的初始值计算出来，得到一个合法的aggregate
      assert(type == 2); 
      vector<int> agg;
      for (auto& tmp: *constexps){
        // 先进行类型转换
        auto exp = dynamic_cast<ConstInitValAST*>(tmp.get());
        if (exp->type == 1){
          agg.push_back(exp->Calc());
        }
        else if (exp->type == 2){
          // 可能向下多层
          for (auto dimension_size_cur = dimension_size_begin + 1; dimension_size_cur != dimension_size_end; dimension_size_cur++){
            if (agg.size() % (*dimension_size_cur) == 0){
              auto tmp_agg = exp->Aggregate(dimension_size_cur, dimension_size_end);
              agg.insert(agg.end(), tmp_agg.begin(), tmp_agg.end());
              break;
            }
          }
        }
      }
      agg.insert(agg.end(), dimension_size_begin[0] - agg.size(), 0);
      return agg;
    }
    void KoopaIR() const override{
      if (type == 1){
        constexp->KoopaIR();
      }
      else if (type == 2){
        for (auto& constexp: *constexps){
          constexp->KoopaIR();
        }
      }
    }
    int Calc()const override{
      return constexp->Calc();
    }
};

// ConstDef ::= IDENT [array_indexs] "=" ConstInitVal;
class ConstDefAST : public BaseAST{
  public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > array_indexs;
    unique_ptr<BaseAST> const_init_val;
    void KoopaIR() const override {
      if (array_indexs->empty()){
        // 常量
        insert_value(ident, const_init_val->Calc());
        define_type(ident, "Const");
      }
      else {
        // 数组
        insert_value(ident, array_indexs->size());
        define_type(ident, "ConstArray");
        if (decl_global == 1){
          cout << "global  ";
        }
        else {
          cout << "  ";
        }
        cout << "@" << get_name(get_current_block_id(), ident) << " = alloc ";
        for (int i = 0; i < array_indexs->size(); i++) {
          cout << "[";
        }
        cout << "i32, ";
        auto dimension_length = new vector<int>();
        auto dimension_size = new vector<int>();
        int tot_dimension_size = 1;
        for (int i = 0; i < array_indexs->size(); i++) {
          int cur_index = array_indexs->at(i)->Calc();
          tot_dimension_size *= cur_index;
          dimension_length->push_back(cur_index); 
        }
        for (int i = 0; i < array_indexs->size(); i++) {
          dimension_size->push_back(tot_dimension_size);
          tot_dimension_size /= dimension_length->at(i);
        }
        // 倒着输出
        for (int i = array_indexs->size() - 1; i >= 0; i--){
          cout << dimension_length->at(i) << "]";
          if (i != 0){
            cout << ", ";
          }
        }
        vector<int> agg = dynamic_cast<ConstInitValAST*>(const_init_val.get())->Aggregate(dimension_size->begin(), dimension_size->end());
        if (decl_global == 1){
          // 全局，用aggregate
          cout << ", ";
          aggregate_array(ident, &agg, 0, dimension_length, dimension_size, 0);
          cout << endl;
        }
        else {
          // 局部，需要store
          cout << endl;
          store_array(ident, &agg, 0, dimension_length, dimension_size, 0);
        }
        delete dimension_length;
        delete dimension_size;
      }
      // const_init_val->KoopaIR();
    }
    int Calc()const override{return -1;}
};


class ConstExpAST : public BaseAST{
  public:
    unique_ptr<BaseAST> exp;
    void KoopaIR() const override {
      exp->KoopaIR();
    }
    int Calc()const override{
      return exp->Calc();
    }
};

class LValAST : public BaseAST{
  public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > array_indexs;
    void KoopaIR() const override {
      auto value = query_symbol(ident).second;
      auto type = query_symbol_type(ident);
      // const 符号就相当于一个数
      if (type == "Const"){
        cout << "  %" << KoopaVariableNum << " = add 0, ";
        cout<< value << std::endl;
        KoopaVariableNum++;
      }
      // var符号则相当于一块内存空间
      else if (type == "Var"){
        // %0 = load @x
        cout << "  %" << KoopaVariableNum << " = load @" << get_name(query_symbol(ident).first, ident) << std::endl;
        KoopaVariableNum++;
      }
      // Array符号需要进行指针运算，取回值
      else if ((type == "Array") or (type == "ConstArray")){
        // cout << "Comparing" << query_symbol(ident).second << " "<< array_indexs->size() << endl;
        for (auto& array_index: *array_indexs){
          int last_index_value = KoopaVariableNum - 1;
          array_index->KoopaIR();
          int cur_index_value = KoopaVariableNum - 1;
          cout << "  %" << KoopaVariableNum << " = getelemptr ";
          if (array_index == array_indexs->front()){
            cout << "@" << get_name(query_symbol(ident).first, ident);
          }
          else {
            cout << "%" << last_index_value;
          }
          cout << ", %" << cur_index_value << endl;
          KoopaVariableNum++;
        }
          
        if (query_symbol(ident).second == array_indexs->size()){
          cout << "  %" << KoopaVariableNum << " = load %" << KoopaVariableNum - 1 << endl;
          KoopaVariableNum++;
        }
        // 说明是函数的数组参数，要返回一个指针
        else {
          int tmp = KoopaVariableNum - 1;
          cout << "  %" << KoopaVariableNum << " = getelemptr ";
          if (array_indexs->size() == 0){
            cout << "@" << get_name(query_symbol(ident).first, ident);
          }
          else {
            cout << "%" << tmp;
          }
          // 做出一个指针
          cout << ", 0" << endl;
          KoopaVariableNum++;
        }
      }
      else if (type == "Pointer"){
        // 先加载出来参与index的运算
        cout << "  %" << KoopaVariableNum << " = load @" << get_name(query_symbol(ident).first, ident) << endl;
        KoopaVariableNum++;
        // 然后根据arrayindex，计算相对于数组基址的偏移
        for (auto& array_index: *array_indexs){
          int last_index_value = KoopaVariableNum - 1;
          array_index->KoopaIR();
          int cur_index_value = KoopaVariableNum - 1;
          cout << "  %" << KoopaVariableNum << " = ";
          if (array_index == array_indexs->front()){
            cout << "getptr ";
          }
          else {
            cout << "getelemptr ";
          }
          cout << "%" << last_index_value << ", %" << cur_index_value << endl;
          KoopaVariableNum++;
        }
        
        // 最后load
        if (query_symbol(ident).second == array_indexs->size()){
          cout << "  %" << KoopaVariableNum << " = load %" << KoopaVariableNum - 1 << endl;
          KoopaVariableNum++;
        }
        // 否则要返回指针
        else {
          int tmp = KoopaVariableNum - 1;
          if (array_indexs->size() == 0){
            cout << "  %" << KoopaVariableNum << " = getptr %" << tmp << ", 0" << endl;
          }
          else {
            cout << "  %" << KoopaVariableNum << " = getelemptr %" << tmp << ", 0" << endl;
          }
          KoopaVariableNum++;
        }
      }
    }
    int Calc()const override{
      return query_symbol(ident).second;
    }
};

// Stmt          ::=  "return" Exp ";" | LVal "=" Exp ";" | Block | Exp ";" | ";" | 
//                    "if" "(" Exp ")" Stmt [(ELSE Stmt)] 
class StmtAST : public BaseAST {
    public:
        int type;
        std::unique_ptr<BaseAST> lval;
        std::unique_ptr<BaseAST> exp;
        std::unique_ptr<BaseAST> block;
        unique_ptr<BaseAST> then_stmt;
        unique_ptr<BaseAST> else_stmt;
        unique_ptr<BaseAST> while_stmt;
        void KoopaIR()const override{
          // RETURN Exp ';'
          if (type == 1){
            exp->KoopaIR();
            std::cout<<"  ret %"<<KoopaVariableNum-1<<"\n";
            change_return_flag(1);
          }
          // LVal '=' Exp ';'
          else if (type == 2){
            exp->KoopaIR();
            // store %1, @x
            if (query_symbol_type(dynamic_cast<LValAST*>(lval.get())->ident) == "Var"){
              cout << "  store %" << KoopaVariableNum-1 << ", @";
              string tmp_ident =  dynamic_cast<LValAST*>(lval.get())->ident;
              cout << get_name(query_symbol(tmp_ident).first, tmp_ident)<< endl;
            }
            else if ((query_symbol_type(dynamic_cast<LValAST*>(lval.get())->ident) == "Array")or 
                     (query_symbol_type(dynamic_cast<LValAST*>(lval.get())->ident) == "ConstArray")) {
              int exp_value = KoopaVariableNum-1;
              // LVal是数组项，需要进行getelemptr
              for (auto& array_index: *dynamic_cast<LValAST*>(lval.get())->array_indexs){
                int last_index_value = KoopaVariableNum - 1;
                array_index->KoopaIR();
                int cur_index_value = KoopaVariableNum - 1;
                cout << "  %" << KoopaVariableNum << " = getelemptr ";
                if (array_index == dynamic_cast<LValAST*>(lval.get())->array_indexs->front()){
                  cout << "@" << get_name(query_symbol(dynamic_cast<LValAST*>(lval.get())->ident).first, dynamic_cast<LValAST*>(lval.get())->ident);
                }
                else {
                  cout << "%" << last_index_value;
                }
                cout << ", %" << cur_index_value << endl;
                KoopaVariableNum++;
              }
              cout << "  store %" << exp_value << ", %" << KoopaVariableNum - 1 << endl;
            }
            // 数组参数会产生指针
            else if (query_symbol_type(dynamic_cast<LValAST*>(lval.get())->ident) == "Pointer"){
              int exp_value = KoopaVariableNum-1;
              auto value = dynamic_cast<LValAST*>(lval.get());
              // 先加载出来参与index的运算
              cout << "  %" << KoopaVariableNum << " = load @" << get_name(query_symbol(value->ident).first, value->ident) << endl;
              KoopaVariableNum++;
              // 然后根据arrayindex，计算相对于数组基址的偏移
              for (auto& array_index: *value->array_indexs){
                int last_index_value = KoopaVariableNum - 1;
                array_index->KoopaIR();
                int cur_index_value = KoopaVariableNum - 1;
                cout << "  %" << KoopaVariableNum << " = ";
                // getelemptr 此时已经不好使了, 因为它要求指针必须是一个数组指针, 而 arr 是一个整数的指针.
                // 为了应对这种情况, 我们引入了另一种指针运算指令: getptr.
                if (array_index == value->array_indexs->front()){
                  cout << "getptr ";
                }
                else {
                  cout << "getelemptr ";
                }
                cout << "%" << last_index_value << ", %" << cur_index_value << endl;
                KoopaVariableNum++;
              }
              // 最后store
              cout << "  store %" << exp_value << ", %" << KoopaVariableNum - 1 << endl;
            }
          }
          else if (type == 3){
            enter_code_block();
            block->KoopaIR();
            exit_code_block();
          }
          else if (type == 4){
            exp->KoopaIR();
          }
          // RETURN ';'
          else if (type == 5){
            std::cout<<"  ret "<<"\n";
            change_return_flag(1);
          }
          // ';'
          else if (type == 6){
            
          }
          // if '(' Exp ')' Stmt 
          else if (type == 7){
            int current_if_block = IfBlockNum;
            IfBlockNum++;

            exp->KoopaIR();
            cout<<"  br %"<<KoopaVariableNum-1<<", %"<<get_if_then_name(current_if_block)<<", %"
                <<get_if_exit_name(current_if_block)<<endl;
            change_return_flag(1); // 当前流已有末尾

            enter_code_block();
            cout<<"%"<<get_if_then_name(current_if_block)<<":"<<endl;
            change_return_flag(0);  // 新流
            then_stmt->KoopaIR();
            if (get_return_flag() == 0){
              cout<< "  jump %"<<get_if_exit_name(current_if_block)<<endl;  // 若当前流还没末尾，就加一个跳转
              change_return_flag(1);
            }
            exit_code_block();

            cout<<"%"<<get_if_exit_name(current_if_block)<<":"<<endl;
            change_return_flag(0);
          }
          // if '(' Exp ')' Stmt ELSE Stmt
          else if (type == 8){
            
            int current_if_block = IfBlockNum;
            IfBlockNum++;
            exp->KoopaIR();
            cout<<"  br %"<<KoopaVariableNum-1<<", %"<<get_if_then_name(current_if_block) 
                <<", %"<<get_if_else_name(current_if_block)<<endl;
            change_return_flag(1);
            enter_code_block();
            cout<<"%"<<get_if_then_name(current_if_block)<<":"<<endl;
            change_return_flag(0);
            then_stmt->KoopaIR();
            if (get_return_flag() == 0){
              cout<< "  jump %"<<get_if_exit_name(current_if_block)<<endl;
              change_return_flag(1);
            }
            exit_code_block();

            enter_code_block();
            cout<<"%"<<get_if_else_name(current_if_block)<<":"<<endl;
            change_return_flag(0); // 新流
            else_stmt->KoopaIR();
            if (get_return_flag() == 0){
              cout<< "  jump %"<<get_if_exit_name(current_if_block)<<endl;
              change_return_flag(1);
            }
            exit_code_block();
            
            cout<<"%"<<get_if_exit_name(current_if_block)<<":"<<endl;
            change_return_flag(0);  // 新流
            
          }
          // while '(' Exp ')' Stmt
          else if (type == 9){
            int current_while_block = WhileBlockNum;
            WhileBlockChain.push_back(current_while_block);
            WhileBlockNum++;
            cout<<"  jump %"<<get_while_entry_name(current_while_block)<<endl;
            cout<<"%"<<get_while_entry_name(current_while_block)<<":"<<endl;
            enter_code_block();
            exp->KoopaIR();
            cout<<"  br %"<<KoopaVariableNum-1<<", %"<<get_while_body_name(current_while_block) 
                <<", %"<<get_while_exit_name(current_while_block)<<endl;
            exit_code_block();

            cout<<"%"<<get_while_body_name(current_while_block)<<":"<<endl;
            enter_code_block();
            change_return_flag(0);
            while_stmt->KoopaIR();
            if (get_return_flag() == 0){
              cout<< "  jump %"<<get_while_entry_name(current_while_block)<<endl;
              change_return_flag(1);
            }
            exit_code_block();
            WhileBlockChain.pop_back();
            cout<<"%"<<get_while_exit_name(current_while_block)<<":"<<endl;
            change_return_flag(0);
          }
          // BREAK ';' 
          else if (type == 10){
            cout<<"  jump %"<<get_while_exit_name(WhileBlockChain.back())<<endl;
            change_return_flag(1);
          }
          // CONTINUE ';'
          else if (type == 11){
            cout<<"  jump %"<<get_while_entry_name(WhileBlockChain.back())<<endl;
            change_return_flag(1);
          }
        }
        int Calc()const override{return -1;}
};

class PrimaryExpAST : public BaseAST{
  public:
    int type; // 1:(EXP), 2:Number, 3 LVal
    std::unique_ptr<BaseAST> content;
    void KoopaIR() const override{
      if (type == 1){
        content->KoopaIR();
      }
      else if (type==2){
        cout<<"  %"<<KoopaVariableNum<<" = add 0, ";
        content->KoopaIR();
        cout<<endl;
        KoopaVariableNum++;
      }
      else if (type == 3){
        content->KoopaIR();
      }
    }
    
    int Calc()const override{
      return content->Calc();
    }
};


class ExpAST : public BaseAST{
  public:
    unique_ptr<BaseAST> lorexp;
    void KoopaIR() const override{
      lorexp->KoopaIR();
    }
    
    int Calc()const override{
      return lorexp->Calc();
    }
};

//LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST: public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> landexp;
    unique_ptr<BaseAST> lorexp;
    void KoopaIR() const override{
      if (type == 1){
        landexp->KoopaIR();
      }
      else if (type == 2){
        // A||B -> (A!=0)|(B!=0)
        lorexp->KoopaIR();
        cout<<"  %"<<KoopaVariableNum<<" = ne ";
        cout<<"0, "<<"%"<<KoopaVariableNum - 1<<"\n";
        KoopaVariableNum++;
        // if (A!=0) then true else check (B!=0)
        int current_if_block = IfBlockNum;
        IfBlockNum++;
        
        cout << "  @" << "SHORT_CIRCUIT_" << current_if_block << " = alloc i32" << endl;
        cout<<"  br %"<<KoopaVariableNum-1<<", %"<<get_if_then_name(current_if_block)<<", %"
            <<get_if_else_name(current_if_block)<<endl;
        change_return_flag(1); // 当前流已有末尾

        cout<<"%"<<get_if_then_name(current_if_block)<<":"<<endl;
        change_return_flag(0);  // 新流
        cout<< "  %"<<KoopaVariableNum<<" = add 0, 1\n";
        // 在SHORT_CIRCUIT_{current_if_block}中存储短路求值结果
        cout<< "  store %"<<KoopaVariableNum<<", @SHORT_CIRCUIT_" << current_if_block << "\n";
        KoopaVariableNum++;
        cout<<"  jump %"<<get_if_exit_name(current_if_block)<<endl;
        change_return_flag(1);

        cout<<"%"<<get_if_else_name(current_if_block)<<":"<<endl;
        change_return_flag(0); // 新流
        landexp->KoopaIR();
        cout<<"  %"<<KoopaVariableNum<<" = ne "
            <<"0, "<<"%"<<KoopaVariableNum-1<<"\n";
        cout<< "  store %"<<KoopaVariableNum<<", @SHORT_CIRCUIT_" << current_if_block << "\n";
        KoopaVariableNum++;
        cout<<"  jump %"<<get_if_exit_name(current_if_block)<<endl;
        change_return_flag(1);

        cout<<"%"<<get_if_exit_name(current_if_block)<<":"<<endl;
        change_return_flag(0);
        cout<<"  %"<<KoopaVariableNum<<" = load @SHORT_CIRCUIT_"<<current_if_block<<"\n";
        KoopaVariableNum++;
      }
    }
    int Calc() const override{
      if (type == 1){
        return landexp->Calc();
      }
      else if (type == 2){
        int res1 = lorexp->Calc();
        if (res1 == 1)
          return 1;
        int res2 = lorexp->Calc();
        return res2;
      }
      assert(0);
    }

};

// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> eqexp;
    unique_ptr<BaseAST> landexp;
    void KoopaIR() const override{
      if (type == 1){
        eqexp->KoopaIR();
      }
      else if (type == 2){
        // A&&B -> (A!=0)&&(B!=0)
        landexp->KoopaIR();
        cout<<"  %"<<KoopaVariableNum<<" = ne ";
        cout<<"0, "<<"%"<<KoopaVariableNum - 1<<"\n";
        KoopaVariableNum++;
        // if (A==0) then false else check (B!=0)
        int current_if_block = IfBlockNum;
        IfBlockNum++;
        
        cout << "  @" << "SHORT_CIRCUIT_" << current_if_block << " = alloc i32" << endl;
        cout<<"  br %"<<KoopaVariableNum-1<<", %"<<get_if_else_name(current_if_block)<<", %"
            <<get_if_then_name(current_if_block)<<endl;
        change_return_flag(1); // 当前流已有末尾

        cout<<"%"<<get_if_then_name(current_if_block)<<":"<<endl;
        change_return_flag(0);  // 新流
        cout<< "  %"<<KoopaVariableNum<<" = add 0, 0\n";
        // 在SHORT_CIRCUIT_{current_if_block}中存储短路求值结果
        cout<< "  store %"<<KoopaVariableNum<<", @SHORT_CIRCUIT_" << current_if_block << "\n";
        KoopaVariableNum++;
        cout<<"  jump %"<<get_if_exit_name(current_if_block)<<endl;
        change_return_flag(1);

        cout<<"%"<<get_if_else_name(current_if_block)<<":"<<endl;
        change_return_flag(0); // 新流
        eqexp->KoopaIR();
        cout<<"  %"<<KoopaVariableNum<<" = ne "
            <<"0, "<<"%"<<KoopaVariableNum-1<<"\n";
        cout<< "  store %"<<KoopaVariableNum<<", @SHORT_CIRCUIT_" << current_if_block << "\n";
        KoopaVariableNum++;
        cout<<"  jump %"<<get_if_exit_name(current_if_block)<<endl;
        change_return_flag(1);

        cout<<"%"<<get_if_exit_name(current_if_block)<<":"<<endl;
        change_return_flag(0);
        cout<<"  %"<<KoopaVariableNum<<" = load @SHORT_CIRCUIT_"<<current_if_block<<"\n";
        KoopaVariableNum++;
      }
    }
    int Calc() const override{
      if (type == 1){
        return eqexp->Calc();
      }
      else if (type == 2){
        int res1 = eqexp->Calc();
        if (res1 == 0)
          return 0;
        int res2 = landexp->Calc();
        return res2;
      }
      assert(0);
    }
};

// EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
class EqExpAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> relexp;
    string eqop;
    unique_ptr<BaseAST> eqexp;
    void KoopaIR() const override{
      if (type == 1){
        relexp->KoopaIR();
      }
      else if (type == 2){
        eqexp->KoopaIR();
        int num1 = KoopaVariableNum-1;
        relexp->KoopaIR();
        int num2 = KoopaVariableNum-1;
        if (eqop == "=="){
          cout<<"  %"<<KoopaVariableNum<<" = eq ";
          cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
          KoopaVariableNum++;
        }
        else if (eqop == "!="){
          cout<<"  %"<<KoopaVariableNum<<" = ne ";
          cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
          KoopaVariableNum++;
        }
      }
    }
    int Calc() const override{
      if (type == 1){
        return relexp->Calc();
      }
      else if (type == 2){
        int res1 = eqexp->Calc();
        int res2 = relexp->Calc();
        if (eqop == "!="){
          return res1 != res2;
        }
        else if (eqop == "=="){
          return res1 == res2;
        }
      }
      assert(0);
    }
};


// RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
class RelExpAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> addexp;
    string relop;
    unique_ptr<BaseAST> relexp;
    void KoopaIR() const override{
      if (type == 1){
        addexp->KoopaIR();
      }
      else if (type == 2){
        relexp->KoopaIR();
        int num1 = KoopaVariableNum-1;
        addexp->KoopaIR();
        int num2 = KoopaVariableNum-1;
        if (relop == "<"){
          cout<<"  %"<<KoopaVariableNum<<" = lt ";
          cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
          KoopaVariableNum++;
        }
        else if (relop == ">"){
          cout<<"  %"<<KoopaVariableNum<<" = gt ";
          cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
          KoopaVariableNum++;

        }
        else if (relop == "<="){
          cout<<"  %"<<KoopaVariableNum<<" = le ";
          cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
          KoopaVariableNum++;

        }
        else if (relop == ">="){
          cout<<"  %"<<KoopaVariableNum<<" = ge ";
          cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
          KoopaVariableNum++;
        }
      }
    }
    
    int Calc() const override{
      if (type == 1){
        return addexp->Calc();
      }
      else if (type == 2){
        int res1 = relexp->Calc();
        int res2 = addexp->Calc();
        if (relop == "<="){
          return res1 <= res2;
        }
        else if (relop == ">="){
          return res1 >= res2;
        }
        else if (relop == "<"){
          return res1 < res2;
        }
        else if (relop == ">"){
          return res1 > res2;
        }
      }
      assert(0);
    }
};

// AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
class AddExpAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> mulexp;
    unique_ptr<BaseAST> addexp;
    char addop;
    void KoopaIR() const override{
      if (type == 1){
        mulexp->KoopaIR();
      }
      else if (type == 2){
        int num1, num2;
        switch (addop){
          case '+':
            addexp->KoopaIR();
            num1 = KoopaVariableNum-1;
            mulexp->KoopaIR();
            num2 = KoopaVariableNum-1;
            cout<<"  %"<<KoopaVariableNum<<" = add ";
            cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
            break;
          case '-':
            addexp->KoopaIR();
            num1 = KoopaVariableNum-1;
            mulexp->KoopaIR();
            num2 = KoopaVariableNum-1;
            cout<<"  %"<<KoopaVariableNum<<" = sub ";
            cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
            break;
          default:
            break;
        }
        KoopaVariableNum++;
      }
    }
    
    int Calc() const override{
      if (type == 1){
        return mulexp->Calc();
      }
      else if (type == 2){
        int res1 = addexp->Calc();
        int res2 = mulexp->Calc();
        if (addop == '+'){
          return res1 + res2;
        }
        else if (addop == '-'){
          return res1 - res2;
        }
      }
      assert(0);
    }
};


// MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class MulExpAST : public BaseAST{
  public:
    int type;
    unique_ptr<BaseAST> unaryexp;
    unique_ptr<BaseAST> mulexp;
    char mulop;

    void KoopaIR() const override{
      if (type == 1){
        unaryexp->KoopaIR();
      }
      else if (type == 2){
        int num1, num2;
        switch (mulop){
          case '*':
            mulexp->KoopaIR();
            num1 = KoopaVariableNum-1;
            unaryexp->KoopaIR();
            num2 = KoopaVariableNum-1;
            cout<<"  %"<<KoopaVariableNum<<" = mul ";
            cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
            break;
          case '/':
            mulexp->KoopaIR();
            num1 = KoopaVariableNum-1;
            unaryexp->KoopaIR();
            num2 = KoopaVariableNum-1;
            cout<<"  %"<<KoopaVariableNum<<" = div ";
            cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
            break;
          case '%':
            mulexp->KoopaIR();
            num1 = KoopaVariableNum-1;
            unaryexp->KoopaIR();
            num2 = KoopaVariableNum-1;
            cout<<"  %"<<KoopaVariableNum<<" = mod ";
            cout<<"%"<<num1<<", "<<"%"<<num2<<"\n";
            break;
          default:
            break;
        }
        KoopaVariableNum++;
      }
      
    }    
    int Calc() const override{
      if (type == 1){
        return unaryexp->Calc();
      }
      else if (type == 2){
        int res1 = mulexp->Calc();
        int res2 = unaryexp->Calc();
        if (mulop == '*'){
          return res1 * res2;
        }
        else if (mulop == '/'){
          return res1 / res2;
        }
        else if (mulop == '%'){
          return res1 % res2;
        }
      }
      assert(0);
    }
};

class UnaryExpAST : public BaseAST{
  public:
    int type; //1:Primary, 2:UnaryOp UnaryExp, 3:FuncCall
    char unaryop;
    unique_ptr<BaseAST> content;
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > func_rparams;
    void KoopaIR() const override{
      if (type == 1){
        content->KoopaIR();
      }
      else if (type == 2){
        if (unaryop == '!'){
          // %0 = eq 6, 0
          content->KoopaIR();
          cout<<"  %"<<KoopaVariableNum<< " = ";
          cout<<"eq 0, %"<<KoopaVariableNum-1<<endl;
          KoopaVariableNum ++;

        }
        else if (unaryop == '-'){
          content->KoopaIR();
          cout<<"  %"<<KoopaVariableNum<< " = ";
          cout<<"sub 0, %"<<KoopaVariableNum-1<<endl;
          KoopaVariableNum ++;
        }
        else if (unaryop == '+'){
          content->KoopaIR();
        }
      }
      // func call
      else if (type == 3){
        // 先把参数计算好
        auto vec = new std::vector<int>();
        for (auto& exp: *func_rparams){
          exp->KoopaIR();
          // 此时当前exp的结果在KoopaVariableNum-1
          vec->push_back(KoopaVariableNum-1);
        }

        if (query_func_type(ident) == "int"){
          cout<<"  %"<<KoopaVariableNum<<" = ";
          KoopaVariableNum++;
        }
        else if (query_func_type(ident) == "void"){
          cout<<"  ";
        }
        cout<<"call @"<<ident<<"(";
        int first_param = 1;
        for (auto reg: *vec){
          if (first_param == 1){
            first_param = 0;
          }
          else{
            cout<<", ";
          }
          cout<<"%"<<reg;
        }
        cout<<")\n";
      }

    }
    
    int Calc() const override{
      if (type == 1){
        return content->Calc();
      }
      else if (type == 2){
        int res = content->Calc();
        if (unaryop == '+'){
          return res;
        }
        else if (unaryop == '-'){
          return -res;
        }
        else if (unaryop == '!'){
          return !res;
        }
      }
      // func call 算不出来
      assert(0);
    }

};

class NumberAST : public BaseAST{
    public:
        int number;
        void KoopaIR()const override{
          std::cout<<number;
        }
        int Calc() const override{
          return number;
        }
};

// ...
