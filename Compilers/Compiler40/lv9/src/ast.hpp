#pragma once
#include <iostream>
#include <fstream>
#include <koopa.h>
#include <variant>
#include <cassert>
#include <stack>
#include <vector>
#include <unordered_map>
#include <list>

class ConstSymbol{
  public:
  std::string val;
  ConstSymbol(std::string val) : val(val) {}
  ConstSymbol() : val("") {}
};
class VarSymbol{
  public:
  std::string val;
  std::string alloc;
  bool is_ptr;
  std::vector<int> size_list;
  VarSymbol(std::string val, std::string alloc) : val(val), alloc(alloc), is_ptr(false), size_list() {}
  VarSymbol(std::string val, std::string alloc, std::vector<int>size_list) : val(val), alloc(alloc), is_ptr(false), size_list(size_list) {}
  VarSymbol(std::string val, std::string alloc, bool is_ptr, std::vector<int> size_list) : val(val), alloc(alloc), is_ptr(is_ptr), size_list(size_list) {}
  VarSymbol() : val(""), alloc(""), size_list() {}
};

class loop_element{
  public:
  std::string entry_block;
  std::string end_block;
  loop_element(std::string entry_block, std::string end_block) : entry_block(entry_block), end_block(end_block) {}
};

class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncFParamAST;
class FuncTypeAST;
class FuncCallAST;
class BlockAST;
class StmtAST;
class BranchAST;
class LoopAST;
class ExpAST;
class UnaryExpAST;
class PrimaryExpAST;
class AddExpAST;
class MulExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;
class BTypeAST;
class LValAST;
class ConstInitValAST;
class InitValAST;
class ConstDeclAST;
class ConstDefAST;
class VarDeclAST;
class VarDefAST;
extern int nxt;
extern std::unordered_map<std::string, int> symbol_name_map;
extern std::unordered_map<std::string, int> block_name_map;
extern std::stack<int> return_stack; // for if
extern std::stack<loop_element> loop_stack; // for loop

class block_element{
public:
  std::unordered_map<std::string, std::variant<ConstSymbol, VarSymbol> > symbol_map;
  block_element(): symbol_map(){}
};
extern std::list<block_element> block_list;

class param_element{
  public:
  std::string name;
  std::string type;
  std::vector<int> size_list;
  param_element(std::string name, std::string type) : name(name), type(type), size_list() {}
  param_element(std::string name, std::string type, std::vector<int> size_list) : name(name), type(type), size_list(size_list) {}
  param_element(): name(""), type(""), size_list() {}
};

class func_info{
  public:
  std::string name;
  std::string ret_type;
  std::vector<param_element> param_list;
  func_info(std::string name, std::string ret_type) : name(name), ret_type(ret_type), param_list() {}
  func_info() : name(""), ret_type(""), param_list() {}
};
extern func_info * cur_func;
extern bool is_param;
extern std::unordered_map<std::string, func_info> func_map;
extern std::string get_koopa_array_type(std::vector<int> size_list, int index);
extern std::string get_koopa_ptr_array_type(std::vector<int> size_list, int index);
extern std::vector<std::string> get_koopa_aggregate(ConstInitValAST & init, std::vector<int> size_list, int index, int total_size, std::ostream &of);
extern std::vector<std::string> get_koopa_aggregate(InitValAST & init, std::vector<int> size_list, int index, int total_size, std::ostream &of);
extern std::string output_koopa_aggregate(std::vector<int> size_list, int dim, int idx, std::vector<std::string> array);
extern std::string gen_init_koopa_code(std::vector<int> size_list, int dim, int idx, std::vector<std::string> array, std::string base_ptr);


// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
  virtual void to_koopa_string(std::ostream &) = 0;
};

class ExpAST : public BaseAST {
  public:
  std::string val;
  int res_id;
  bool is_const;
  ExpAST() : val(""), res_id(-1), is_const(false){}
  virtual void to_koopa_string(std::ostream &of) override {
    
  }
};


// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > global_def_list;
  CompUnitAST() : global_def_list(nullptr) {
    std::cout << "CompUnitAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "CompUnitAST { ";
    for(auto &def : *global_def_list){
      def->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream & of) override {
    block_list.push_back(block_element());
    of << "decl @getint(): i32\ndecl @getch(): i32\ndecl @getarray(*i32): i32\ndecl @putint(i32)\ndecl @putch(i32)\ndecl @putarray(i32, *i32)\ndecl @starttime()\ndecl @stoptime()" << std::endl;
    func_map["getint"] = func_info("getint", "int");
    func_map["getch"] = func_info("getch", "int");
    func_map["getarray"] = func_info("getarray", "int");
    func_map["putint"] = func_info("putint", "void");
    func_map["putch"] = func_info("putch", "void");
    func_map["putarray"] = func_info("putarray", "void");
    func_map["starttime"] = func_info("starttime", "void");
    func_map["stoptime"] = func_info("stoptime", "void");
    std::cout << 1 << std::endl;
    for(auto &def : *global_def_list){
      def->to_koopa_string(of);
    }
  }
  // void * to_koopa_raw() override{
  //   koopa_raw_program_t * program = new koopa_raw_program_t;
  //   program->funcs.kind = KOOPA_RSIK_FUNCTION;
  //   program->funcs.len = 1;
  //   program->funcs.buffer = new const void * [1];
  //   program->funcs.buffer[0] = func_def->to_koopa_raw();
  //   // program->values.kind = KOOPA_RSIK_VALUE;
  //   // program->values.len = 0;
  //   // program->values.buffer = nullptr;
  //   return program;
  // }
};


class BlockAST : public BaseAST {
 public:
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > block_item_list;
  std::string block_name;
  bool is_entry_block;
  BlockAST(): block_item_list(nullptr), block_name(""), is_entry_block(false) {
    std::cout << "BlockAST constructed" << std::endl;
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "BlockAST to_koopa_string" << std::endl;
    block_list.push_back(block_element());
    if(is_entry_block){
      // put params into symbol_map
      for(auto &param : cur_func->param_list){
        if(param.type == "i32"){
          of << "  %" << param.name << "_param" << " = alloc i32" << std::endl;
          of << "  store " << "@" << param.name << ", %" << param.name << "_param" << std::endl;
          block_list.back().symbol_map[param.name] = VarSymbol("", "%" + param.name + "_param");  
        }
        else{
          if(param.size_list.size()) of << "  %" << param.name << "_param" << " = alloc *" << get_koopa_ptr_array_type(param.size_list, 0) << std::endl;
          else of << "  %" << param.name << "_param" << " = alloc *i32" << std::endl;
          of << "  store " << "@" << param.name << ", %" << param.name << "_param" << std::endl;
          block_list.back().symbol_map[param.name] = VarSymbol("", "%" + param.name + "_param", true, param.size_list);
        }
      }
      // output symbol_map
      for(auto &symbol : block_list.back().symbol_map){
        std::cout << symbol.first << " " << std::get<VarSymbol>(symbol.second).alloc << std::endl;
      }
    }
    if(block_item_list){
      for(auto &item : *block_item_list){
        item->to_koopa_string(of);
      }
    }
    block_list.pop_back();
  }
  void Dump() const override {
    std::cout << "BlockAST { ";
    if(block_item_list){
      for(auto &item : *block_item_list){
        item->Dump();
      }
    }
    std::cout << " }";
  }
};

class FuncTypeAST : public BaseAST {
 public:
  std::string type;
  FuncTypeAST() : type("") {
    std::cout << "FuncTypeAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "FuncTypeAST { " << type << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    if(type == "int"){
      of << ": i32";
    }
  }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > param_list;
  std::unique_ptr<BaseAST> block;
  FuncDefAST() : func_type(nullptr), ident(""), param_list(nullptr), block(nullptr) {
    std::cout << "FuncDefAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    if(param_list){
      for(auto &param : *param_list){
        param->Dump();
      }
    }
    block->Dump();
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "FuncDefAST to_koopa_string" << std::endl;
    while(return_stack.size()){
      return_stack.pop();
    }
    return_stack.push(0);
    func_info finfo = func_info(ident, dynamic_cast<FuncTypeAST*>(func_type.get())->type);
    cur_func = &finfo;
    of << "fun @" << ident << "(";
    if(param_list){
      for(auto &param : *param_list){
        param->to_koopa_string(of);
        if (&param != &param_list->back()) {
          of << ", ";
        }
      }
    }
    func_map[ident] = finfo;
    of << ")";
    func_type->to_koopa_string(of);
    of << " {\n";
    of << "%" << "entry:\n";
    dynamic_cast<BlockAST*>(block.get())->is_entry_block = true;

    block->to_koopa_string(of);
    if(return_stack.top() == 0){
      if(dynamic_cast<FuncTypeAST*>(func_type.get())->type == "int") of << "  ret 0\n";
      else of << "  ret\n";
      return_stack.top() = 1;   
    }
    return_stack.pop();
    of << "}\n";
  }
};

class FuncFParamAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> btype;
  std::string ident;
  std::vector<int> size_list;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > sizeexp_list;
  FuncFParamAST() : btype(nullptr), ident(""), size_list(), sizeexp_list(nullptr) {
    std::cout << "FuncFParamAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "FuncFParamAST { ";
    btype->Dump();
    std::cout << ", " << ident << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "FuncFParamAST to_koopa_string" << std::endl;
    
    of << "@" << ident << ": ";
    if(sizeexp_list){
      for(auto &sizeexp : *sizeexp_list){
        sizeexp->to_koopa_string(of);
        size_list.push_back(std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val));
      }
      cur_func->param_list.push_back({ident, "*i32", size_list});
      if(size_list.size()){
        of << "*" << get_koopa_ptr_array_type(size_list, 0);
      }
      else{
        of << "*i32";
      }
    }
    else{
      cur_func->param_list.push_back({ident, "i32"});
      of << "i32";
    }
    
  }
};


class FuncCallAST : public ExpAST {
  public:
  std::string ident;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > param_list;
  FuncCallAST() : ident(""), param_list(nullptr) {
    std::cout << "FuncCallAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "FuncCallAST { " << ident << ", ";
    if(param_list){
      for(auto &param : *param_list){
        param->Dump();
      }
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "FuncCallAST to_koopa_string" << std::endl;
    
    if(param_list){
      for(auto &param : *param_list){
        dynamic_cast<ExpAST*>(param.get())->is_const = is_const;
        param->to_koopa_string(of);
      }
    }

    if(func_map[ident].ret_type == "int"){
      res_id = nxt;
      of << "  %" << std::to_string(nxt++) << " = call @" << ident << "(";
      if(param_list){
        for(auto &param : *param_list){
          if(dynamic_cast<ExpAST*>(param.get())->res_id == -1){
            of << dynamic_cast<ExpAST*>(param.get())->val;
          }
          else{
            of << "%" << dynamic_cast<ExpAST*>(param.get())->res_id;
          }
          if (&param != &param_list->back()) {
            of << ", ";
          }
        }
      }
      of << ")\n";
    }
    else{
      of << "  call @" << ident << "(";
      if(param_list){
        for(auto &param : *param_list){
          if(dynamic_cast<ExpAST*>(param.get())->res_id == -1){
            of << dynamic_cast<ExpAST*>(param.get())->val;
          }
          else{
            of << "%" << dynamic_cast<ExpAST*>(param.get())->res_id;
          }
          if (&param != &param_list->back()) {
            of << ", ";
          }
        }
      }
      of << ")\n";
    }
  }
};

class LValAST : public ExpAST {
  public:
  std::string ident;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > exp_list;
  std::string ptr;
  LValAST() : ident(""), exp_list(nullptr), ptr("") {
    std::cout << "LValAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "LValAST { " << ident << ", ";
    if(exp_list){
      for(auto &exp : *exp_list){
        exp->Dump();
      }
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    if(exp_list){
      for(auto &exp : *exp_list){
        exp->to_koopa_string(of);
      }
      auto cur_block = block_list.end();
      cur_block--;
      while(cur_block->symbol_map.find(ident) == cur_block->symbol_map.end()){
        cur_block--;
      }
      std::string base_ptr = "";      
      int exp_len = exp_list->size();
      int array_size_len = 0;
      bool is_ptr = false;
      std::visit( 
        [&is_ptr, &base_ptr, &array_size_len](auto && args){
          using T = std::decay_t<decltype(args)>;
          if constexpr (std::is_same_v<T, VarSymbol>){
            // TODO if not ConstEval
            // get res id when load from alloc
            if(!base_ptr.size()) base_ptr = args.alloc;
            is_ptr = args.is_ptr;
            array_size_len = args.size_list.size();
          }
        }, (cur_block->symbol_map)[ident]);
      if(is_ptr) array_size_len++;
      // get base ptr arraylen explen
      for(auto &exp : *exp_list){
        if(is_ptr){
          of << "  %" << std::to_string(nxt++) << " = load " << base_ptr << std::endl;
          base_ptr = "%" + std::to_string(nxt-1);
          if(dynamic_cast<ExpAST*>(exp.get())->res_id == -1){
            // PrimaryExp is immediate
            of << "  %" << std::to_string(nxt++) << " = getptr " << base_ptr << ", " << dynamic_cast<ExpAST*>(exp.get())->val << std::endl;
          }
          else{
            of << "  %" << std::to_string(nxt++) << " = getptr " << base_ptr << ", %" << dynamic_cast<ExpAST*>(exp.get())->res_id << std::endl;
          }
          is_ptr = false;
        }
        else{
          if(dynamic_cast<ExpAST*>(exp.get())->res_id == -1){
            // PrimaryExp is immediate
            of << "  %" << std::to_string(nxt++) << " = getelemptr " << base_ptr << ", " << dynamic_cast<ExpAST*>(exp.get())->val << std::endl;
          }
          else{
            of << "  %" << std::to_string(nxt++) << " = getelemptr " << base_ptr << ", %" << dynamic_cast<ExpAST*>(exp.get())->res_id << std::endl;
          }
        }
        res_id = nxt-1;
        base_ptr = "%" + std::to_string(nxt-1);
        // TODO if don't have alloc
      }
      ptr = base_ptr;
      if(exp_len < array_size_len){ // if explen < arraylen + is_func_param then passed as param
        is_param = true;
      }
    }
    else{
      auto cur_block = block_list.end();
      cur_block--;
      while(cur_block->symbol_map.find(ident) == cur_block->symbol_map.end()){
        cur_block--;
      }
      bool is_ptr = false;
      std::visit(
        [this, &is_ptr](auto && args){
          using T = std::decay_t<decltype(args)>;
          if constexpr (std::is_same_v<T, ConstSymbol>){
            val = args.val;
          }
          else if constexpr (std::is_same_v<T, VarSymbol>){
            // TODO if not ConstEval
            // get res id when load from alloc              
            ptr = args.alloc;
            val = args.val;
            is_ptr = args.size_list.size(); // if have size, then passed as param
            // TODO if don't have alloc
          }
          else{
            assert(false);
          }
        }, (cur_block->symbol_map)[ident]);
      if (is_ptr)
      {
        is_param = true;
      }
    }
  }
};
class UnaryExpAST : public ExpAST {
  public:
  std::unique_ptr<BaseAST> primary_exp;
  std::unique_ptr<BaseAST> unary_exp;
  std::unique_ptr<BaseAST> func_call;
  std::string unary_op;
  UnaryExpAST() : primary_exp(nullptr), unary_exp(nullptr), func_call(nullptr), unary_op("") {
    std::cout << "UnaryExpAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "UnaryExpAST { ";
    if(primary_exp){
      primary_exp->Dump();
    }
    else if(func_call){
      func_call->Dump();
    }
    else{
      std::cout << unary_op << ", ";
      unary_exp->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "UnaryExpAST to_koopa_string" << std::endl;
    if(primary_exp){
      // UnaryExp : PrimaryExp
      dynamic_cast<ExpAST*>(primary_exp.get())->is_const = is_const;
      primary_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(primary_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(primary_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(primary_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(primary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(primary_exp.get())->val));
    }
    else if(func_call){
      func_call->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(func_call.get())->res_id;
    }
    else{
      // UnaryExp : UnaryOp UnaryExp
      dynamic_cast<ExpAST*>(unary_exp.get())->is_const = is_const;
      unary_exp->to_koopa_string(of);
      switch (unary_op[0])
      {
        case '+':
        // do nothing!
        res_id = dynamic_cast<ExpAST*>(unary_exp.get())->res_id;
        // std::cout << dynamic_cast<ExpAST*>(unary_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(unary_exp.get())->val.size() << "TO DO" << std::endl;
        if(dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val));
        break;
        case '-':
        
        if (dynamic_cast<ExpAST*>(unary_exp.get())->res_id == -1){
          // PrimaryExp is immediate
          // of << "  %" << std::to_string(nxt++) << " = sub 0, " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
        }
        else{
          res_id = nxt;
          of << "  %" << std::to_string(nxt++) << " = sub 0, %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
        }
        // std::cout << dynamic_cast<ExpAST*>(unary_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(unary_exp.get())->val.size() << "TO DO" << std::endl;
        if(dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(-std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val));
        break;
        case '!':
        
        if (dynamic_cast<ExpAST*>(unary_exp.get())->res_id == -1){
          // PrimaryExp is immediate
          // of << "  %" << std::to_string(nxt++) << " = eq "<< dynamic_cast<ExpAST*>(unary_exp.get())->val << ", 0" << std::endl;
        }
        else{
          res_id = nxt;
          of << "  %" << std::to_string(nxt++) << " = eq %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << ", 0" << std::endl;
        }
        // std::cout << dynamic_cast<ExpAST*>(unary_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(unary_exp.get())->val.size() << "TO DO" << std::endl;
        if(dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val) == 0);
        break;
        default:
        break;
      }
    }
    std::cout << "UnaryExpAST to_koopa_string end" << std::endl;
  }
};

class PrimaryExpAST : public ExpAST {
  public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> lval;
  PrimaryExpAST() : exp(nullptr), lval(nullptr) {
    std::cout << "PrimaryExpAST constructed" << std::endl;
  }
  void Dump() const override { 
    std::cout << "PrimaryExpAST { ";
    if(exp){
      exp->Dump();
    }
    else if(lval){
      lval->Dump();
    }
    else{
      std::cout << val;
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "PrimaryExpAST to_koopa_string" << std::endl;
    if(exp){
      // PrimaryExp : ( Exp )
      dynamic_cast<ExpAST*>(exp.get())->is_const = is_const;
      exp->to_koopa_string(of);
      val = dynamic_cast<ExpAST*>(exp.get())->val;
      res_id = dynamic_cast<ExpAST*>(exp.get())->res_id;
    }
    else if(lval){
      // PrimaryExp : lval
      // res_id = nxt;
      // of << "  %" << std::to_string(nxt++) << " = load " << lval << std::endl;
      lval->to_koopa_string(of);
      if(is_param){
        if(dynamic_cast<LValAST*>(lval.get())->ptr.size()){
          res_id = nxt;
          of << "  %" << std::to_string(nxt++) << " = getelemptr " << dynamic_cast<LValAST*>(lval.get())->ptr << ", 0" << std::endl;
        }
        else{
          assert(false);
        }
        is_param = false;
      }
      else{
        if(dynamic_cast<LValAST*>(lval.get())->ptr.size()){
          res_id = nxt;
          of << "  %" << std::to_string(nxt++) << " = load " << dynamic_cast<LValAST*>(lval.get())->ptr << std::endl;
        }
        else{
          val = dynamic_cast<LValAST*>(lval.get())->val;
        }
      }
    }
    else{
      // PrimaryExp : num
      res_id = -1;
    }
    std::cout << val << "len: " << val.size() << "TO DO" << std::endl;
    std::cout << "PrimaryExpAST to_koopa_string end" << std::endl;
  }
};

class AddExpAST : public ExpAST {
  public:
  std::unique_ptr<BaseAST> mul_exp;
  std::unique_ptr<BaseAST> add_exp;
  std::string add_op;
  AddExpAST() : mul_exp(nullptr), add_exp(nullptr), add_op("") {
    std::cout << "AddExpAST constructed" << std::endl;
  }
  void Dump() const override {
    if (add_op.size()){
      std::cout << "AddExpAST { ";
      add_exp->Dump();
      std::cout << ", " << add_op << ", ";
      mul_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "AddExpAST { ";
      mul_exp->Dump();
      std::cout << " }";
    }
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "AddExpAST to_koopa_string" << std::endl;
    if(add_op.size()){
      // AddExp : AddExp + MulExp
      dynamic_cast<ExpAST*>(add_exp.get())->is_const = is_const;
      dynamic_cast<ExpAST*>(mul_exp.get())->is_const = is_const;
      add_exp->to_koopa_string(of);
      mul_exp->to_koopa_string(of);
      
      if (dynamic_cast<ExpAST*>(add_exp.get())->res_id == -1){
         // AddExp is immediate
        if (dynamic_cast<ExpAST*>(mul_exp.get())->res_id == -1){
          // MulExp is immediate
          // if (add_op == "+"){
          //   of << "  %" << std::to_string(nxt++) << " = add " << dynamic_cast<ExpAST*>(add_exp.get())->val << ", " << dynamic_cast<ExpAST*>(mul_exp.get())->val << std::endl;
          // }
          // else if(add_op == "-"){
          //   of << "  %" << std::to_string(nxt++) << " = sub " << dynamic_cast<ExpAST*>(add_exp.get())->val << ", " << dynamic_cast<ExpAST*>(mul_exp.get())->val << std::endl;
          // }
        }
        else{
          res_id = nxt;
          if (add_op == "+"){
            of << "  %" << std::to_string(nxt++) << " = add " << dynamic_cast<ExpAST*>(add_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << std::endl;
          }
          else if(add_op == "-"){
            of << "  %" << std::to_string(nxt++) << " = sub " << dynamic_cast<ExpAST*>(add_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << std::endl;
          }
        }
      }
      else{
        // AddExp is not immediate
        res_id = nxt;
        if (dynamic_cast<ExpAST*>(mul_exp.get())->res_id == -1){
          // MulExp is immediate
          if (add_op == "+"){
            of << "  %" << std::to_string(nxt++) << " = add %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(mul_exp.get())->val << std::endl;
          }
          else if(add_op == "-"){
            of << "  %" << std::to_string(nxt++) << " = sub %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(mul_exp.get())->val << std::endl;
          }
        }
        else{
          if (add_op == "+"){
            of << "  %" << std::to_string(nxt++) << " = add %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << std::endl;
          }
          else if(add_op == "-"){
            of << "  %" << std::to_string(nxt++) << " = sub %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << std::endl;
          }
        }
      }
      if (add_op == "+"){
        if(dynamic_cast<ExpAST*>(add_exp.get())->val.size() && dynamic_cast<ExpAST*>(mul_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val) + std::stoi(dynamic_cast<ExpAST*>(mul_exp.get())->val));
      }
      else if(add_op == "-"){
        if(dynamic_cast<ExpAST*>(add_exp.get())->val.size() && dynamic_cast<ExpAST*>(mul_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val) - std::stoi(dynamic_cast<ExpAST*>(mul_exp.get())->val));
      }
    }
    else{
      // AddExp : MulExp
      dynamic_cast<ExpAST*>(mul_exp.get())->is_const = is_const;
      mul_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(mul_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(mul_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(mul_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(mul_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(mul_exp.get())->val));
    }
    std::cout << "AddExpAST to_koopa_string end" << std::endl;
  }
};

class MulExpAST : public ExpAST {
  public:
  std::unique_ptr<BaseAST> unary_exp;
  std::unique_ptr<BaseAST> mul_exp;
  std::string mul_op;
  MulExpAST() : unary_exp(nullptr), mul_exp(nullptr), mul_op("") {
    std::cout << "MulExpAST constructed" << std::endl;
  }
  void Dump() const override {
    if (mul_op.size()){
      std::cout << "MulExpAST { ";
      mul_exp->Dump();
      std::cout << ", " << mul_op << ", ";
      unary_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "MulExpAST { ";
      unary_exp->Dump();
      std::cout << " }";
    }
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "MulExpAST to_koopa_string" << std::endl;
    if (mul_op.size()){
      // MulExp : MulExp * UnaryExp
      dynamic_cast<ExpAST*>(mul_exp.get())->is_const = is_const;
      dynamic_cast<ExpAST*>(unary_exp.get())->is_const = is_const;
      mul_exp->to_koopa_string(of);
      unary_exp->to_koopa_string(of);
      
      if (dynamic_cast<ExpAST*>(mul_exp.get())->res_id == -1){
        // MulExp is immediate
        if (dynamic_cast<ExpAST*>(unary_exp.get())->res_id == -1){
          // UnaryExp is immediate
          // if (mul_op == "*"){
          //   of << "  %" << std::to_string(nxt++) << " = mul " << dynamic_cast<ExpAST*>(mul_exp.get())->val << ", " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
          // }
          // else if(mul_op == "/"){
          //   of << "  %" << std::to_string(nxt++) << " = div " << dynamic_cast<ExpAST*>(mul_exp.get())->val << ", " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
          // }
          // else if(mul_op == "%"){
          //   of << "  %" << std::to_string(nxt++) << " = mod " << dynamic_cast<ExpAST*>(mul_exp.get())->val << ", " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
          // }
        }
        else{
          res_id = nxt;
          if (mul_op == "*"){
            of << "  %" << std::to_string(nxt++) << " = mul " << dynamic_cast<ExpAST*>(mul_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
          }
          else if(mul_op == "/"){
            of << "  %" << std::to_string(nxt++) << " = div " << dynamic_cast<ExpAST*>(mul_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
          }
          else if(mul_op == "%"){
            of << "  %" << std::to_string(nxt++) << " = mod " << dynamic_cast<ExpAST*>(mul_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
          }
        }
      }
      else{
        // MulExp is not immediate
        res_id = nxt;
        if (dynamic_cast<ExpAST*>(unary_exp.get())->res_id == -1){
          // UnaryExp is immediate
          if (mul_op == "*"){
            of << "  %" << std::to_string(nxt++) << " = mul %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
          }
          else if(mul_op == "/"){
            of << "  %" << std::to_string(nxt++) << " = div %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
          }
          else if(mul_op == "%"){
            of << "  %" << std::to_string(nxt++) << " = mod %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(unary_exp.get())->val << std::endl;
          }
        }
        else{
          if (mul_op == "*"){
            of << "  %" << std::to_string(nxt++) << " = mul %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
          }
          else if(mul_op == "/"){
            of << "  %" << std::to_string(nxt++) << " = div %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
          }
          else if(mul_op == "%"){
            of << "  %" << std::to_string(nxt++) << " = mod %" << dynamic_cast<ExpAST*>(mul_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(unary_exp.get())->res_id << std::endl;
          }
        }
      }
      if (mul_op == "*"){
        if(dynamic_cast<ExpAST*>(mul_exp.get())->val.size() && dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(mul_exp.get())->val) * std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val));
      }
      else if(mul_op == "/"){
        if(dynamic_cast<ExpAST*>(mul_exp.get())->val.size() && dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(mul_exp.get())->val) / std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val));
      }
      else if(mul_op == "%"){
        if(dynamic_cast<ExpAST*>(mul_exp.get())->val.size() && dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(mul_exp.get())->val) % std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val));
      }
    }
    else{
      // MulExp : UnaryExp
      dynamic_cast<ExpAST*>(unary_exp.get())->is_const = is_const;
      unary_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(unary_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(unary_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(unary_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(unary_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(unary_exp.get())->val));
    }
    std::cout << "MulExpAST to_koopa_string end" << std::endl;
  }
};

class RelExpAST: public ExpAST {
  public:
  std::unique_ptr<BaseAST> add_exp;
  std::unique_ptr<BaseAST> rel_exp;
  std::string rel_op;
  RelExpAST() : add_exp(nullptr), rel_exp(nullptr), rel_op("") {
    std::cout << "RelExpAST constructed" << std::endl;
  }
  void Dump() const override {
    if (rel_op.size()){
      std::cout << "RelExpAST { ";
      rel_exp->Dump();
      std::cout << ", " << rel_op << ", ";
      add_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "RelExpAST { ";
      add_exp->Dump();
      std::cout << " }";
    }
  }
  void to_koopa_string(std::ostream & of) override {
    std::cout << "RelExpAST to_koopa_string" << std::endl;
    if (rel_op.size()){
      // RelExp : RelExp RELOP AddExp
      dynamic_cast<ExpAST*>(rel_exp.get())->is_const = is_const;
      dynamic_cast<ExpAST*>(add_exp.get())->is_const = is_const;
      rel_exp->to_koopa_string(of);
      add_exp->to_koopa_string(of);
      if (dynamic_cast<ExpAST*>(rel_exp.get())->res_id == -1){
        // RelExp is immediate
        if (dynamic_cast<ExpAST*>(add_exp.get())->res_id == -1){
          // AddExp is immediate
          // if (rel_op == "<"){
          //   of << "  %" << std::to_string(nxt++) << " = lt " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          // }
          // else if(rel_op == ">"){
          //   of << "  %" << std::to_string(nxt++) << " = gt " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          // }
          // else if(rel_op == "<="){
          //   of << "  %" << std::to_string(nxt++) << " = le " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          // }
          // else if(rel_op == ">="){
          //   of << "  %" << std::to_string(nxt++) << " = ge " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          // }
        }
        else{
          res_id = nxt;
          if (rel_op == "<"){
            of << "  %" << std::to_string(nxt++) << " = lt " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
          else if(rel_op == ">"){
            of << "  %" << std::to_string(nxt++) << " = gt " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
          else if(rel_op == "<="){
            of << "  %" << std::to_string(nxt++) << " = le " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
          else if(rel_op == ">="){
            of << "  %" << std::to_string(nxt++) << " = ge " << dynamic_cast<ExpAST*>(rel_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
        }
      }
      else {
        res_id = nxt;
        // RelExp is not immediate
        if (dynamic_cast<ExpAST*>(add_exp.get())->res_id == -1){
          // AddExp is immediate
          if (rel_op == "<"){
            of << "  %" << std::to_string(nxt++) << " = lt %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          }
          else if(rel_op == ">"){
            of << "  %" << std::to_string(nxt++) << " = gt %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          }
          else if(rel_op == "<="){
            of << "  %" << std::to_string(nxt++) << " = le %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          }
          else if(rel_op == ">="){
            of << "  %" << std::to_string(nxt++) << " = ge %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(add_exp.get())->val << std::endl;
          }
        }
        else{
          if (rel_op == "<"){
            of << "  %" << std::to_string(nxt++) << " = lt %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
          else if(rel_op == ">"){
            of << "  %" << std::to_string(nxt++) << " = gt %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
          else if(rel_op == "<="){
            of << "  %" << std::to_string(nxt++) << " = le %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
          else if(rel_op == ">="){
            of << "  %" << std::to_string(nxt++) << " = ge %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(add_exp.get())->res_id << std::endl;
          }
        }
      }
      if(rel_op == "<"){
        if(dynamic_cast<ExpAST*>(rel_exp.get())->val.size() && dynamic_cast<ExpAST*>(add_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val) < std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val));
      }
      else if(rel_op == ">"){
        if(dynamic_cast<ExpAST*>(rel_exp.get())->val.size() && dynamic_cast<ExpAST*>(add_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val) > std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val));
      }
      else if(rel_op == "<="){
        if(dynamic_cast<ExpAST*>(rel_exp.get())->val.size() && dynamic_cast<ExpAST*>(add_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val) <= std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val));
      }
      else if(rel_op == ">="){
        if(dynamic_cast<ExpAST*>(rel_exp.get())->val.size() && dynamic_cast<ExpAST*>(add_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val) >= std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val));
      }
    }
    else {
      // RelExp : AddExp
      dynamic_cast<ExpAST*>(add_exp.get())->is_const = is_const;
      add_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(add_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(add_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(add_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(add_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(add_exp.get())->val));
    }
    std::cout << "RelExpAST to_koopa_string end" << std::endl;
  }
};

class EqExpAST: public ExpAST {
  public:
  std::unique_ptr<BaseAST> rel_exp;
  std::unique_ptr<BaseAST> eq_exp;
  std::string eq_op;
  EqExpAST() : rel_exp(nullptr), eq_exp(nullptr), eq_op("") {
    std::cout << "EqExpAST constructed" << std::endl;
  }
  void Dump() const override {
    if (eq_op.size()){
      std::cout << "EqExpAST { ";
      eq_exp->Dump();
      std::cout << ", " << eq_op << ", ";
      rel_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "EqExpAST { ";
      rel_exp->Dump();
      std::cout << " }";
    }
  }
  void to_koopa_string(std::ostream & of) override {
    std::cout << "EqExpAST to_koopa_string" << std::endl;
    if(eq_op.size()){
      // EqExp : EqExp EQOP RelExp
      dynamic_cast<ExpAST*>(eq_exp.get())->is_const = is_const;
      dynamic_cast<ExpAST*>(rel_exp.get())->is_const = is_const;
      eq_exp->to_koopa_string(of);
      rel_exp->to_koopa_string(of);
      
      if (dynamic_cast<ExpAST*>(eq_exp.get())->res_id == -1){
        // EqExp is immediate
        if (dynamic_cast<ExpAST*>(rel_exp.get())->res_id == -1){
          // RelExp is immediate
          // if (eq_op == "=="){
          //   of << "  %" << std::to_string(nxt++) << " = eq " << dynamic_cast<ExpAST*>(eq_exp.get())->val << ", " << dynamic_cast<ExpAST*>(rel_exp.get())->val << std::endl;
          // }
          // else if(eq_op == "!="){
          //   of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(eq_exp.get())->val << ", " << dynamic_cast<ExpAST*>(rel_exp.get())->val << std::endl;
          // }
        }
        else{
          if (eq_op == "=="){
            res_id = nxt;
            of << "  %" << std::to_string(nxt++) << " = eq " << dynamic_cast<ExpAST*>(eq_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << std::endl;
          }
          else if(eq_op == "!="){
            res_id = nxt;
            of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(eq_exp.get())->val << ", %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << std::endl;
          }
        }
      }
      else{
        res_id = nxt;
        // EqExp is not immediate
        if (dynamic_cast<ExpAST*>(rel_exp.get())->res_id == -1){
          // RelExp is immediate
          if (eq_op == "=="){
            of << "  %" << std::to_string(nxt++) << " = eq %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(rel_exp.get())->val << std::endl;
          }
          else if(eq_op == "!="){
            of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", " << dynamic_cast<ExpAST*>(rel_exp.get())->val << std::endl;
          }
        }
        else{
          if (eq_op == "=="){
            of << "  %" << std::to_string(nxt++) << " = eq %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << std::endl;
          }
          else if (eq_op == "!="){
            of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", %" << dynamic_cast<ExpAST*>(rel_exp.get())->res_id << std::endl;
          }
        }
      }
      if(eq_op == "=="){
        if(dynamic_cast<ExpAST*>(eq_exp.get())->val.size() && dynamic_cast<ExpAST*>(rel_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val) == std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val));
      }
      else if(eq_op == "!="){
        if(dynamic_cast<ExpAST*>(eq_exp.get())->val.size() && dynamic_cast<ExpAST*>(rel_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val) != std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val));
      }
    }
    else {
      // EqExp : RelExp
      dynamic_cast<ExpAST*>(rel_exp.get())->is_const = is_const;
      rel_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(rel_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(rel_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(rel_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(rel_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(rel_exp.get())->val));
    }
    std::cout << "EqExpAST to_koopa_string end" << std::endl;
  }
};

class LAndExpAST: public ExpAST {
  public:
  std::unique_ptr<BaseAST> eq_exp;
  std::unique_ptr<BaseAST> land_exp;
  std::string land_op;
  LAndExpAST() : eq_exp(nullptr), land_exp(nullptr), land_op("") {
    std::cout << "LAndExpAST constructed" << std::endl;
  }
  void Dump() const override {
    if (land_op.size()){
      std::cout << "LAndExpAST { ";
      land_exp->Dump();
      std::cout << ", " << land_op << ", ";
      eq_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "LAndExpAST { ";
      eq_exp->Dump();
      std::cout << " }";
    }
  }
  void to_koopa_string(std::ostream & of) override {
    std::cout << "LAndExpAST to_koopa_string" << std::endl;
    // of << "debug6" << std::endl;
    if(land_op.size()){
      // LAndExp : LAndExp LANDOP EqExp
      dynamic_cast<ExpAST*>(land_exp.get())->is_const = is_const;
      dynamic_cast<ExpAST*>(eq_exp.get())->is_const = is_const;
      val = "0";
      land_exp->to_koopa_string(of);
      // of << "debug4" << std::endl;
      if(!is_const){
        // if not const, eval by branch
        if(symbol_name_map.find("tmpand") == symbol_name_map.end()){
          symbol_name_map["tmpand"] = 0;
        }
        std::string alloc = "@tmpand" + std::to_string(symbol_name_map["tmpand"]++);
        of << "  " << alloc << " = alloc i32" << std::endl; 
        of << "  store" << " " << "0" << ", " << alloc << std::endl;
        // if(dynamic_cast<ExpAST*>(land_exp.get())->res_id == -1){
        //   // of << "debug1" << std::endl;
        //   of << "  store" << " " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "@tmpand_" << std::to_string(nxt-1) << std::endl;
        // }
        // else{
        //   // of << "debug1" << std::endl;
        //   of << "  store" << " " << "%" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "@tmpand_" << std::to_string(nxt-1) << std::endl;
        // }
        // of << "  store" << " " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "@tmpand_" << std::to_string(nxt-1) << std::endl;
        if(block_name_map.find("if") == block_name_map.end()){
          block_name_map["if"] = 0;
        }
        int tmp = block_name_map["if"]++;
        std::string br_left_block = "then_" + std::to_string(tmp);
        std::string br_right_block = "endif_" + std::to_string(tmp);
        if(dynamic_cast<ExpAST*>(land_exp.get())->res_id == -1){
          // of << "debug1" << std::endl;
          of << "  br " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "%" << br_left_block << ", " << "%" << br_right_block << std::endl;
        }
        else{
          // of << "debug1" << std::endl;
          of << "  br " << "%" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "%" << br_left_block << ", " << "%" << br_right_block << std::endl;
        }
        of << "%" << br_left_block << ":" << std::endl;
        return_stack.push(0);
        eq_exp->to_koopa_string(of);
        if(dynamic_cast<ExpAST*>(eq_exp.get())->res_id == -1){
          if(dynamic_cast<ExpAST*>(land_exp.get())->val.size() && dynamic_cast<ExpAST*>(eq_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val) && std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val));
          of << "  store" << " " << std::to_string(std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val) != 0 ) << ", " << alloc << std::endl;
        }
        else{
          of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", " << "0" << std::endl;
          if(dynamic_cast<ExpAST*>(land_exp.get())->val.size() && dynamic_cast<ExpAST*>(eq_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val) && std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val));
          of << "  store" << " " << "%" << std::to_string(nxt-1) << ", " << alloc << std::endl;
        }
        if(return_stack.top() == 0) of << "  jump " << "%" << br_right_block << std::endl;
        return_stack.pop();
        of << "%" << "endif_" << std::to_string(tmp) << ":" << std::endl;
        res_id = nxt;
        of << "  %" << std::to_string(nxt++) << " = load " << alloc << std::endl;
      }
      else{
        // if const, eval immediately
        // of << "debug2" << std::endl;
        eq_exp->to_koopa_string(of);
        if (dynamic_cast<ExpAST*>(land_exp.get())->res_id == -1){
          // LAndExp is immediate
          if (dynamic_cast<ExpAST*>(eq_exp.get())->res_id == -1){
            // EqExp is immediate
            if (land_op == "&&"){
              // of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "0" << std::endl;
              // of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(eq_exp.get())->val << ", " << "0" << std::endl;
              // of << "  %" << std::to_string(nxt) << " = and %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
            }
          }
          else{
            if (land_op == "&&"){
              of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt) << " = and %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
              res_id = nxt++;
            }
          }
        }
        else{
          // LAndExp is not immediate
          if (dynamic_cast<ExpAST*>(eq_exp.get())->res_id == -1){
            // EqExp is immediate
            if (land_op == "&&"){
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(eq_exp.get())->val << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt) << " = and %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
              res_id = nxt++;
            }
          }
          else{
            if (land_op == "&&"){
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(eq_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt) << " = and %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
              res_id = nxt++;
            }
          }
        }
        
        if (land_op == "&&"){
          if(dynamic_cast<ExpAST*>(land_exp.get())->val.size() && dynamic_cast<ExpAST*>(eq_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val) && std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val));
        }

      }
    }
    else{
      // LAndExp : EqExp
      // of << "debug5" << std::endl;
      dynamic_cast<ExpAST*>(eq_exp.get())->is_const = is_const;
      eq_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(eq_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(eq_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(eq_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(eq_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(eq_exp.get())->val));
    }
    std::cout << "LAndExpAST to_koopa_string end" << std::endl;
  }
};

class LOrExpAST: public ExpAST {
  public:
  std::unique_ptr<BaseAST> land_exp;
  std::unique_ptr<BaseAST> lor_exp;
  std::string lor_op;
  LOrExpAST() : land_exp(nullptr), lor_exp(nullptr), lor_op("") {
    std::cout << "LOrExpAST constructed" << std::endl;
  }
  void Dump() const override {
    if (lor_op.size()){
      std::cout << "LOrExpAST { ";
      lor_exp->Dump();
      std::cout << ", " << lor_op << ", ";
      land_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "LOrExpAST { ";
      land_exp->Dump();
      std::cout << " }";
    }
  }
  void to_koopa_string(std::ostream & of) override {
    std::cout << "LOrExpAST to_koopa_string" << std::endl;
    if(lor_op.size()){
      // LORExp : LORExp LOROP LAndExp
      dynamic_cast<ExpAST*>(lor_exp.get())->is_const = is_const;
      dynamic_cast<ExpAST*>(land_exp.get())->is_const = is_const;
      val = "1";
      lor_exp->to_koopa_string(of);
      if(!is_const){
        if(symbol_name_map.find("tmpor") == symbol_name_map.end()){
          symbol_name_map["tmpor"] = 0;
        }
        std::string alloc = "@tmpor" + std::to_string(symbol_name_map["tmpor"]++);
        of << "  " << alloc << " = alloc i32" << std::endl; 
        of << "  store" << " " << "1" << ", " << alloc << std::endl;
        if(block_name_map.find("if") == block_name_map.end()){
          block_name_map["if"] = 0;
        }
        int tmp = block_name_map["if"]++;
        std::string br_left_block = "then_" + std::to_string(tmp);
        std::string br_right_block = "endif_" + std::to_string(tmp);
        if(dynamic_cast<ExpAST*>(lor_exp.get())->res_id == -1){
          of << "  %" << std::to_string(nxt++) << " = eq " << dynamic_cast<ExpAST*>(lor_exp.get())->val << ", " << "0" << std::endl;
          of << "  br %" << std::to_string(nxt-1) << ", " << "%" << br_left_block << ", " << "%" << br_right_block << std::endl;
        }
        else{
          of << "  %" << std::to_string(nxt++) << " = eq %" << dynamic_cast<ExpAST*>(lor_exp.get())->res_id << ", " << "0" << std::endl;
          of << "  br " << "%" << std::to_string(nxt-1) << ", " << "%" << br_left_block << ", " << "%" << br_right_block << std::endl;
        }
        of << "%" << br_left_block << ":" << std::endl;
        return_stack.push(0);
        land_exp->to_koopa_string(of);
        if(dynamic_cast<ExpAST*>(land_exp.get())->res_id == -1){
          if(dynamic_cast<ExpAST*>(lor_exp.get())->val.size() && dynamic_cast<ExpAST*>(land_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(lor_exp.get())->val) || std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val));
          of << "  store " << std::to_string(std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val) != 0 ) << ", " << alloc << std::endl;
        }
        else{
          of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "0" << std::endl;
          if(dynamic_cast<ExpAST*>(lor_exp.get())->val.size() && dynamic_cast<ExpAST*>(land_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(lor_exp.get())->val) || std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val));
          of << "  store " << "%" << std::to_string(nxt-1) << ", " << alloc << std::endl;
        }
        if(return_stack.top() == 0) of << "  jump " << "%" << br_right_block << std::endl;
        return_stack.pop();
        of << "%" << "endif_" << std::to_string(tmp) << ":" << std::endl;
        res_id = nxt;
        of << "  %" << std::to_string(nxt++) << " = load " << alloc << std::endl;
      }
      else{
        land_exp->to_koopa_string(of);
        if (dynamic_cast<ExpAST*>(lor_exp.get())->res_id == -1){
          // LORExp is immediate
          if (dynamic_cast<ExpAST*>(land_exp.get())->res_id == -1){
            // LAndExp is immediate
            if (lor_op == "||"){

              // of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(lor_exp.get())->val << ", " << "0" << std::endl;
              // of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "0" << std::endl;
              // of << "  %" << std::to_string(nxt) << " = or %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
            }
          }
          else{
            if (lor_op == "||"){
              of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(lor_exp.get())->val << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt) << " = or %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
              res_id = nxt++;
            }
          }
        }
        else{
          // LORExp is not immediate
          if (dynamic_cast<ExpAST*>(land_exp.get())->res_id == -1){
            // LAndExp is immediate
            if (lor_op == "||"){
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(lor_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt++) << " = ne " << dynamic_cast<ExpAST*>(land_exp.get())->val << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt) << " = or %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
              res_id = nxt++;
            }
          }
          else{
            if (lor_op == "||"){
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(lor_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt++) << " = ne %" << dynamic_cast<ExpAST*>(land_exp.get())->res_id << ", " << "0" << std::endl;
              of << "  %" << std::to_string(nxt) << " = or %" << std::to_string(nxt-2) << ", %" << std::to_string(nxt-1) << std::endl;
              res_id = nxt++;
            }
          }
        }
        if (lor_op == "||"){
          if(dynamic_cast<ExpAST*>(lor_exp.get())->val.size() && dynamic_cast<ExpAST*>(land_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(lor_exp.get())->val) || std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val));
        }

      }
    }
    else {
      // LORExp : LAndExp
      dynamic_cast<ExpAST*>(land_exp.get())->is_const = is_const;
      land_exp->to_koopa_string(of);
      res_id = dynamic_cast<ExpAST*>(land_exp.get())->res_id;
      std::cout << dynamic_cast<ExpAST*>(land_exp.get())->val << "len: " << dynamic_cast<ExpAST*>(land_exp.get())->val.size() << "TO DO" << std::endl;
      if(dynamic_cast<ExpAST*>(land_exp.get())->val.size()) val = std::to_string(std::stoi(dynamic_cast<ExpAST*>(land_exp.get())->val));
    }
    std::cout << "LOrExpAST to_koopa_string end" << std::endl;
  }
};


class StmtAST : public BaseAST {
 public:
  std::string num;
  std::unique_ptr<BaseAST> lval;
  bool is_return;
  bool is_break;
  bool is_continue;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> block;
  StmtAST() : num(""), lval(nullptr), is_return(false), is_break(false), is_continue(false), exp(nullptr), block(nullptr){
    std::cout << "StmtAST constructed" << std::endl;
  }
  void to_koopa_string(std::ostream &of) override {
    if(return_stack.top()) return;
    std::cout << "StmtAST to_koopa_string" << std::endl;
    if(is_return){
      if(exp){
        exp->to_koopa_string(of);
        if(dynamic_cast<ExpAST*>(exp.get())->res_id == -1){
          of << "  ret "<< dynamic_cast<ExpAST*>(exp.get())->val << std::endl;
        }
        else{
          of << "  ret %" << dynamic_cast<ExpAST*>(exp.get())->res_id << std::endl;
        }
      }
      else{
        of << "  ret" << std::endl;
      }
      return_stack.top() = 1;
      return;
    }
    else if(lval){
      // lval stmt
      exp->to_koopa_string(of);
      lval->to_koopa_string(of);
      // auto cur_block = block_list.end();
      // cur_block--;
      // while(cur_block->symbol_map.find(lval) == cur_block->symbol_map.end()){
      //   cur_block--;
      // }
      // std::visit([this](auto &&arg){
      //   using T = std::decay_t<decltype(arg)>;
      //   if constexpr (std::is_same_v<T, ConstSymbol>){
      //     assert(false);
      //   }
      //   else if constexpr (std::is_same_v<T, VarSymbol>){
      //     arg.val = dynamic_cast<ExpAST*>(exp.get())->val;
      //   }
      // }, (cur_block->symbol_map)[lval]);
      if(dynamic_cast<ExpAST*>(exp.get())->res_id == -1){
        of << "  store " << dynamic_cast<ExpAST*>(exp.get())->val << ", " << dynamic_cast<LValAST*>(lval.get())->ptr << std::endl;
      }
      else{
        of << "  store %" << dynamic_cast<ExpAST*>(exp.get())->res_id << ", " << dynamic_cast<LValAST*>(lval.get())->ptr << std::endl;
      }
    }
    else if(block){
      // block stmt
      block->to_koopa_string(of);
    }
    else if(is_break){
      of << "  jump " << "%" << loop_stack.top().end_block << std::endl;
      return_stack.top() = 1;
    }
    else if(is_continue){
      of << "  jump " << "%" << loop_stack.top().entry_block << std::endl;
      return_stack.top() = 1;
    }
    else{
      if(exp) exp->to_koopa_string(of);
    }
  }
  void Dump() const override {
    std::cout << "StmtAST { ";
    if(exp) exp->Dump();
    else if(block) block->Dump();
    std::cout << " }";
  };
};

class BTypeAST : public BaseAST {
  public:
  std::string btype;
  BTypeAST() : btype("") {
    std::cout << "BTypeAST constructed" << std::endl;
  }
  BTypeAST(std::string btype) : btype(btype) {
    std::cout << "BTypeAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "BTypeAST { " << btype << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    // do nothing
  }

};


class ConstInitValAST : public BaseAST {
  public:
  std::unique_ptr<BaseAST> exp;
  std::vector<int> size_list;
  bool is_global;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > const_init_val_list;
  ConstInitValAST() : exp(nullptr), size_list(), is_global(false), const_init_val_list(nullptr) {
    std::cout << "ConstInitValAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "ConstInitValAST { ";
    if(exp){
      exp->Dump();
    }
    else{
      for(auto &item : *const_init_val_list){
        item->Dump();
      }
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    if(exp){
      exp->to_koopa_string(of);
      return;
    }
    assert(false);
    // std::cout << "ConstInitValAST to_koopa_string" << std::endl;
    // if(exp){
    //   exp->to_koopa_string(of);
    //   return;
    // }
    // int len = 0;
    // for(auto &item : *const_init_val_list){
    //   item->to_koopa_string(of);
    //   len++;
    // }
    // if(is_global){
    //   if(len == 0){
    //     of << ", zeroinit" << std::endl;
    //   }
    //   else if(len < size && len){
    //     of << ", {";
    //     for(auto &item : *const_init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val << ", ";
    //     }
    //     for(int i = 0; i < size - len-1; i++){
    //       of << "0, ";
    //     }
    //     of << "0}" << std::endl;
    //   }
    //   else{
    //     of << ", {";
    //     for(auto &item : *const_init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val;
    //       if (&item != &const_init_val_list->back()) {
    //           of << ", ";
    //       }
    //     }
    //     of << "}" << std::endl;
    //   }
    // }
    // else{
    //   of << "  store ";
    //   if(len < size){
    //     of << "{";
    //     for(auto &item : *const_init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val << ", ";
    //     }
    //     for(int i = 0; i < size - len-1; i++){
    //       of << "0, ";
    //     }
    //     of << "0}, ";
    //   }
    //   else{
    //     of << "{";
    //     for(auto &item : *const_init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val;
    //       if (&item != &const_init_val_list->back()) {
    //           of << ", ";
    //       }
    //     }
    //     of << "}, ";
    //   }
    // }
  }
};



class ConstDefAST : public BaseAST {
  public:
  std::string ident;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > sizeexp_list;
  std::vector<int> size_list;
  std::unique_ptr<BaseAST> init;
  bool is_global;
  ConstDefAST() : ident(""), exp(nullptr), sizeexp_list(nullptr), size_list(), init(nullptr), is_global(false) {
    std::cout << "ConstDefAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "ConstDefAST { " << ident << ", ";
    if(sizeexp_list){
      std::cout << "array";
      for(auto &item : *sizeexp_list){
        item->Dump();
      }
    }
    else{
      exp->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "ConstDefAST to_koopa_string" << std::endl;
    if(!sizeexp_list){
      // const var
      exp->to_koopa_string(of);
      if(symbol_name_map.find(ident) == symbol_name_map.end()){
        symbol_name_map[ident] = 0;
      }
      block_list.back().symbol_map[ident] = ConstSymbol(dynamic_cast<ExpAST*>(dynamic_cast<ConstInitValAST*>(exp.get())->exp.get())->val);
    }
    else{
      // const array
      int total_size = 1;
      if(init) dynamic_cast<ConstInitValAST*>(init.get())->is_global = is_global;
      if(is_global){
        for (auto &sizeexp : *sizeexp_list){
          sizeexp->to_koopa_string(of);
          total_size *= std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val);
          size_list.push_back(std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val));
        }
        std::cout << "total size:" << total_size << std::endl;
        if(symbol_name_map.find(ident) == symbol_name_map.end()){
          symbol_name_map[ident] = 0;
        }
        dynamic_cast<ConstInitValAST*>(init.get())->size_list = size_list;
        std::string alloc = "@" + ident + std::to_string(symbol_name_map[ident]++);
        of << "global " << alloc << " = alloc "<< get_koopa_array_type(size_list, 0) << ", ";
        std::vector<std::string> init_val = get_koopa_aggregate(*dynamic_cast<ConstInitValAST*>(init.get()), size_list, 0, total_size, of);
        of << output_koopa_aggregate(size_list, 0, 0, init_val) << std::endl;
        // std::cout << "size:";
        // for (int i=0;i<size_list.size();i++){
        //   std::cout << size_list[i] << " ";
        // }
        // std::cout << std::endl;
        // std::cout << "init_val:";
        // for (int i=0;i<init_val.size();i++){
        //   std::cout << init_val[i] << " ";
        // }
        // std::cout << std::endl;
        block_list.back().symbol_map[ident] = VarSymbol("", alloc, size_list);
      }
      else{
        for (auto &sizeexp : *sizeexp_list){
          sizeexp->to_koopa_string(of);
          total_size *= std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val);
          size_list.push_back(std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val));
        }
        if(symbol_name_map.find(ident) == symbol_name_map.end()){
          symbol_name_map[ident] = 0;
        }
        dynamic_cast<ConstInitValAST*>(init.get())->size_list = size_list;
        std::string alloc = "@" + ident + std::to_string(symbol_name_map[ident]++);
        of << "  " << alloc << " = alloc "<< get_koopa_array_type(size_list, 0) << std::endl;
        std::vector<std::string> init_val = get_koopa_aggregate(*dynamic_cast<ConstInitValAST*>(init.get()), size_list, 0, total_size, of);
        of << gen_init_koopa_code(size_list, 0, 0, init_val, alloc);
        block_list.back().symbol_map[ident] = VarSymbol("", alloc, size_list);
      }
    }
  }
};

class ConstDeclAST : public BaseAST {
  public:
  std::unique_ptr<BaseAST> btype;
  // std::unique_ptr<BaseAST> const_def_list;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > const_def_list;
  bool is_global;
  ConstDeclAST() : btype(nullptr), const_def_list(nullptr), is_global(false) {
    is_global = 0;
    std::cout << "ConstDeclAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "ConstDeclAST { ";
    btype->Dump();
    std::cout << ", " ;
    // const_def_list->Dump();
    for(auto &item : *const_def_list){
      item->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "ConstDeclAST to_koopa_string" << is_global << std::endl;
    if(!is_global && return_stack.top()) return;
    for(auto &item : *const_def_list){
      dynamic_cast<ConstDefAST*>(item.get())->is_global = is_global;
      item->to_koopa_string(of);
    }
    // const_def_list->to_koopa_string(of);
  }
};

class InitValAST: public BaseAST {
  public:
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > init_val_list;
  std::vector<int> size_list;
  std::unique_ptr<BaseAST> exp;
  bool is_global;
  int size;
  InitValAST() : init_val_list(nullptr), exp(nullptr), is_global(false), size(0) {
    std::cout << "InitValAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "InitValAST { ";
    if(init_val_list) for(auto &item : *init_val_list){
      item->Dump();
    }
    else{
      exp->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "InitValAST to_koopa_string" << std::endl;
    if(exp){
      exp->to_koopa_string(of);
    }
    else{
      assert(false);
    }
    // int len = 0;
    // for(auto &item : *init_val_list){
    //   item->to_koopa_string(of);
    //   len++;
    // }
    // if(is_global){
    //   if(len == 0){
    //     of << ", zeroinit" << std::endl;
    //   }
    //   else if(len < size){
    //     of << ", {";
    //     for(auto &item : *init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val << ", ";
    //     }
    //     for(int i = 0; i < size - len-1; i++){
    //       of << "0, ";
    //     }
    //     of << "0}" << std::endl;
    //   }
    //   else{
    //     of << ", {";
    //     for(auto &item : *init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val;
    //       if (&item != &init_val_list->back()) {
    //           of << ", ";
    //       }
    //     }
    //     of << "}" << std::endl;
    //   }
    // }
    // else{
    //   of << "  store ";
    //   if(len < size){
    //     of << "{";
    //     for(auto &item : *init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val << ", ";
    //     }
    //     for(int i = 0; i < size - len-1; i++){
    //       of << "0, ";
    //     }
    //     of << "0}, ";
    //   }
    //   else{
    //     of << "{";
    //     for(auto &item : *init_val_list){
    //       of << dynamic_cast<ExpAST*>(item.get())->val;
    //       if (&item != &init_val_list->back()) {
    //           of << ", ";
    //       }
    //     }
    //     of << "}, ";
    //   }
    // }
  }
};

class VarDefAST : public BaseAST {
  public:
  std::string ident;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > sizeexp_list;
  std::vector<int> size_list;
  std::unique_ptr<BaseAST> init;
  bool is_global;
  VarDefAST() : ident(""), exp(nullptr), sizeexp_list(nullptr), init(nullptr), is_global(false) {
    std::cout << "VarDefAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "VarDefAST { " << ident << ", ";
    if(exp) exp->Dump();
    else if(sizeexp_list){
      std::cout << "array, ";
      for(auto &item : *sizeexp_list){
        item->Dump();
      }    
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "VarDefAST to_koopa_string" << std::endl;
    if(symbol_name_map.find(ident) == symbol_name_map.end()){
      symbol_name_map[ident] = 0;
    }
    if(init) dynamic_cast<InitValAST*>(init.get())->is_global = is_global;
    if(is_global){
      std::string alloc = "@" + ident + std::to_string(symbol_name_map[ident]++);
      if(sizeexp_list){
        // global array
        int total_size = 1;
        for (auto &sizeexp : *sizeexp_list){
          sizeexp->to_koopa_string(of);
          total_size *= std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val);
          size_list.push_back(std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val));
        }
        of << "global " << alloc << " = alloc " << get_koopa_array_type(size_list, 0);
        if(init){
          of << ", ";
          dynamic_cast<InitValAST*>(init.get())->size_list = size_list;
          std::vector<std::string> init_val = get_koopa_aggregate(*dynamic_cast<InitValAST*>(init.get()), size_list, 0, total_size, of);
          of << output_koopa_aggregate(size_list, 0, 0, init_val) << std::endl;
        }
        else{
          of << ", zeroinit" << std::endl;
        }
        block_list.back().symbol_map[ident] = VarSymbol("", alloc, size_list);
      }
      else{
        // global var
        of << "global " << alloc << " = alloc i32, ";
        if(exp){
          exp->to_koopa_string(of);
          block_list.back().symbol_map[ident] = VarSymbol(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->val, alloc);
          if(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->res_id == -1){
            of << dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->val << std::endl;
          }
          else{
            assert(false);
          }
        }
        else{
          of << "zeroinit" << std::endl;
          block_list.back().symbol_map[ident] = VarSymbol("0", alloc);
        }      
      }
    }
    else{
      if(sizeexp_list){
        std::string alloc = "@" + ident + std::to_string(symbol_name_map[ident]++);
        int total_size = 1;
        for (auto &sizeexp : *sizeexp_list){
          sizeexp->to_koopa_string(of);
          total_size *= std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val);
          size_list.push_back(std::stoi(dynamic_cast<ExpAST*>(sizeexp.get())->val));
        }
        of << "  " << alloc << " = alloc "<< get_koopa_array_type(size_list, 0) << std::endl;
        if(init){
          dynamic_cast<InitValAST*>(init.get())->size_list = size_list;
          std::vector<std::string> init_val = get_koopa_aggregate(*dynamic_cast<InitValAST*>(init.get()), size_list, 0, total_size, of);
          of << gen_init_koopa_code(size_list, 0, 0, init_val, alloc);
        }
        block_list.back().symbol_map[ident] = VarSymbol("", alloc, size_list);
      }
      else{
        std::string alloc = "@" + ident + std::to_string(symbol_name_map[ident]++);
        of << "  " << alloc << " = alloc i32" << std::endl; 
        if(exp) {
          exp->to_koopa_string(of);
          block_list.back().symbol_map[ident] = VarSymbol(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->val, alloc);
          if(dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->res_id == -1){
            of << "  store " << dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->val << ", " << alloc << std::endl;
          }
          else{
            of << "  store %" << dynamic_cast<ExpAST*>(dynamic_cast<InitValAST*>(exp.get())->exp.get())->res_id << ", " << alloc << std::endl;
          }
        }
        else{
          block_list.back().symbol_map[ident] = VarSymbol("", alloc);
        }
      }
    }
  }
};

class VarDeclAST : public BaseAST{
  public:
  std::unique_ptr<BaseAST> btype;
  std::unique_ptr<std::list<std::unique_ptr<BaseAST> > > var_def_list;
  bool is_global;
  VarDeclAST() : btype(nullptr), var_def_list(nullptr), is_global(false) {
    std::cout << "VarDeclAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "VarDeclAST { ";
    btype->Dump();
    std::cout << ", " ;
    for(auto &item : *var_def_list){
      item->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "VarDeclAST to_koopa_string" << is_global << std::endl;
    if(!is_global && return_stack.top()) return;
    for(auto &item : *var_def_list){
      dynamic_cast<VarDefAST*>(item.get())->is_global = is_global;
      item->to_koopa_string(of);
    }
  }
};

class BranchAST : public BaseAST {
  public:
  std::unique_ptr<BaseAST> ifexp;
  std::unique_ptr<BaseAST> ifstmt;
  std::unique_ptr<BaseAST> elsestmt;
  BranchAST() : ifexp(nullptr), ifstmt(nullptr), elsestmt(nullptr) {
    std::cout << "BranchAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "BranchAST { ";
    ifexp->Dump();
    std::cout << ", ";
    ifstmt->Dump();
    if(elsestmt){    
      std::cout << ", ";
        elsestmt->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "BranchAST to_koopa_string" << std::endl;
    ifexp->to_koopa_string(of);
    if(block_name_map.find("if") == block_name_map.end()){
      block_name_map["if"] = 0;
    }
    int tmp = block_name_map["if"]++;
    std::string br_left_block = "then_" + std::to_string(tmp);
    std::string br_right_block = elsestmt? "else_" + std::to_string(tmp) : "endif_" + std::to_string(tmp);
    if(dynamic_cast<ExpAST*>(ifexp.get())->res_id == -1){
      of << "  br " << dynamic_cast<ExpAST*>(ifexp.get())->val << ", " << "%" << br_left_block << ", " << "%" << br_right_block << std::endl;
    }
    
    else{
      // of << "debug3" << std::endl;
      of << "  br " << "%" << dynamic_cast<ExpAST*>(ifexp.get())->res_id << ", " << "%" << br_left_block << ", " << "%" << br_right_block << std::endl;
    }
    of << "%" << br_left_block << ":" << std::endl;
    return_stack.push(0);
    ifstmt->to_koopa_string(of);
    if(return_stack.top() == 0) of << "  jump " << "%" << "endif_" << std::to_string(tmp) << std::endl;
    return_stack.pop();
    if(elsestmt){
      of << "%" << br_right_block << ":" << std::endl;
      return_stack.push(0);
      elsestmt->to_koopa_string(of);
      if(return_stack.top() == 0) of << "  jump " << "%" << "endif_" << std::to_string(tmp) << std::endl;
      return_stack.pop();
    }
    of << "%" << "endif_" << std::to_string(tmp) << ":" << std::endl;
  }
};

class LoopAST :public BaseAST {
  public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;
  LoopAST() : exp(nullptr), stmt(nullptr) {
    std::cout << "LoopAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "LoopAST { ";
    exp->Dump();
    std::cout << ", ";
    stmt->Dump();
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "LoopAST to_koopa_string" << std::endl;
    if(block_name_map.find("while") == block_name_map.end()){
      block_name_map["while"] = 0;
    }
    int tmp = block_name_map["loop"]++;
    int nxt = 0;
    std::string entry_block = "while_entry_" + std::to_string(tmp); 
    std::string body_block = "while_body_" + std::to_string(tmp) + "_" + std::to_string(nxt++);
    std::string end_block = "endwhile_" + std::to_string(tmp);
    of << "  jump " << "%" << entry_block << std::endl;
    of << "%" << entry_block << ":" << std::endl;
    exp->to_koopa_string(of);
    if(dynamic_cast<ExpAST*>(exp.get())->res_id == -1){
      of << "  br " << dynamic_cast<ExpAST*>(exp.get())->val << ", " << "%" << body_block << ", " << "%" << end_block << std::endl;
    }
    else{
      of << "  br %" << dynamic_cast<ExpAST*>(exp.get())->res_id << ", " << "%" << body_block << ", " << "%" << end_block << std::endl;
    }
    of << "%" << body_block << ":" << std::endl;
    return_stack.push(0);
    loop_stack.push({entry_block, end_block});
    stmt->to_koopa_string(of);
    loop_stack.pop();
    if(return_stack.top() == 0) of << "  jump " << "%" << entry_block << std::endl;
    return_stack.pop();
    of << "%" << end_block << ":" << std::endl;
  }
};

class GlobalDefAST : public BaseAST {
  public:
  std::unique_ptr<BaseAST> func_def;
  std::unique_ptr<BaseAST> const_decl;
  std::unique_ptr<BaseAST> var_decl;
  GlobalDefAST() : func_def(nullptr), const_decl(nullptr), var_decl(nullptr) {
    std::cout << "GlobalDefAST constructed" << std::endl;
  }
  void Dump() const override {
    std::cout << "GlobalDefAST { ";
    if(func_def){
      func_def->Dump();
    }
    else if(const_decl){
      const_decl->Dump();
    }
    else{
      var_decl->Dump();
    }
    std::cout << " }";
  }
  void to_koopa_string(std::ostream &of) override {
    std::cout << "GlobalDefAST to_koopa_string" << std::endl;
    if(func_def){
      func_def->to_koopa_string(of);
    }
    else if(const_decl){
      dynamic_cast<ConstDeclAST*>(const_decl.get())->is_global = true;;
      const_decl->to_koopa_string(of);
    }
    else{
      dynamic_cast<VarDeclAST*>(var_decl.get())->is_global = true;
      var_decl->to_koopa_string(of);
    }
  }
};

// ...