#pragma once

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "utils.h"
using namespace std;

static SymbolVector symbol_vector;
static BlockVector block_vector;
static LoopVector loop_vector;

class BaseAST {
public:
  virtual ~BaseAST() = default; 
  virtual void *to_left_value() const { return nullptr; }
  virtual void *to_koopa() const { return nullptr; }
  virtual void *to_koopa(int index) const { return nullptr; }
  virtual void *to_koopa(vector<const void *> &global_var) const {
    return nullptr;
  }
  virtual void *to_koopa(koopa_raw_basic_block_t end_block) const { return nullptr; }
  virtual void *to_koopa(koopa_raw_type_t type) const { return nullptr; }

  virtual void *to_koopa(vector<const void *> &func,vector<const void *> &value) const {
    return nullptr;
  }
  virtual void *to_koopa(vector<const void *> &global_var,koopa_raw_type_t type) const {
    return nullptr;
  }
  virtual void *to_koopa(vector<const void *> &init_list, vector<size_t> size_vec, int level) const {
    return nullptr;
  }
  virtual int cal_value() const { assert(false); }
};

class CompUnitAST : public BaseAST {
public:
  unique_ptr<vector<unique_ptr<BaseAST>>> def_vec;
  CompUnitAST(unique_ptr<vector<unique_ptr<BaseAST>>> &def_vec);
  void load_lib_func(vector<const void *> &lib_func_vec) const;
  void* to_koopa() const override;
};

class DefAST : public BaseAST {
public:
  enum DefType {FuncDef, ConstDef, VarDef};
  DefType type;
  unique_ptr<BaseAST> def;
  DefAST(unique_ptr<BaseAST> &def, DefType type);
  void *to_koopa(vector<const void *> &func,vector<const void *> &value) const override;
};

class FuncDefAST : public BaseAST {
public:
  unique_ptr<BaseAST> func_type;
  string ident;
  unique_ptr<BaseAST> block;
  unique_ptr<vector<unique_ptr<BaseAST>>> param_vec;

  FuncDefAST(unique_ptr<BaseAST> &func_type, const char *ident,
             unique_ptr<vector<unique_ptr<BaseAST>>> &param_vec,
             unique_ptr<BaseAST> &block);

  void *to_koopa() const override;
};

class GlobalConstDefAST : public BaseAST {
public:
  unique_ptr<BaseAST> const_type;
  unique_ptr<vector<unique_ptr<BaseAST>>> ConstDef_vec;
  GlobalConstDefAST(
      unique_ptr<BaseAST> &const_type,
      unique_ptr<vector<unique_ptr<BaseAST>>> &ConstDef_vec);
  void *to_koopa() const override;
};

class GlobalVarDefAST : public BaseAST {
public:
  unique_ptr<BaseAST> var_type;
  unique_ptr<vector<unique_ptr<BaseAST>>> VarDef_vec;
  GlobalVarDefAST(
      unique_ptr<BaseAST> &var_type,
      unique_ptr<vector<unique_ptr<BaseAST>>> &VarDef_vec);
  void *to_koopa(vector<const void *> &global_var) const override;
};

class FuncFParamAST : public BaseAST {
public:
  enum FuncFParamType { Array, Var };
  unique_ptr<BaseAST> param_type;
  string ident;
  FuncFParamType type;
  unique_ptr<vector<unique_ptr<BaseAST>>> index_array;
  FuncFParamAST(unique_ptr<BaseAST> &param_type, const char *ident, FuncFParamType type);
  FuncFParamAST(
      unique_ptr<BaseAST> &param_type,
      unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
      const char *ident, FuncFParamType type);
  void *to_koopa() const override;
  void *to_koopa(int index) const override;
};

class BlockAST : public BaseAST {
 public:
  enum {Item, Empty} type; 
  //unique_ptr<BaseAST> stmt;

  //BlockAST(unique_ptr<BaseAST>& stmt);

  //string to_string() const override;

  
  unique_ptr<vector<unique_ptr<BaseAST>>> blockitem_vec;
  
  BlockAST();

  BlockAST(
      unique_ptr<vector<unique_ptr<BaseAST>>> &blockitem_vec);

  void *to_koopa() const override;  
};

class StmtAST : public BaseAST {
public:
  enum StmtType{ Exp, Assign, Block, Return, Empty, If, While, Break, Continue};
  StmtType type;
  unique_ptr<BaseAST> exp;
  unique_ptr<BaseAST> stmt;
  StmtAST(StmtType type);
  StmtAST(unique_ptr<BaseAST> &exp, StmtType type);
  StmtAST(unique_ptr<BaseAST> &stmt, unique_ptr<BaseAST> &exp, StmtType type);
  void* to_koopa() const override;
};

class IfAST : public BaseAST {
public:
  unique_ptr<BaseAST> exp;
  unique_ptr<BaseAST> stmt;
  IfAST(unique_ptr<BaseAST> &exp, unique_ptr<BaseAST> &stmt);
  void* to_koopa() const override;
};

class ConstDeclAST : public BaseAST {
public:
  unique_ptr<BaseAST> const_type;
  unique_ptr<vector<unique_ptr<BaseAST>>> ConstDef_vec;
  ConstDeclAST(
      unique_ptr<BaseAST> &const_type,
      unique_ptr<vector<unique_ptr<BaseAST>>> &ConstDef_vec);
  void* to_koopa() const override;
  void *to_koopa(vector<const void *> &global_var) const override;
};

class TypeAST : public BaseAST {
public:
  string type;
  TypeAST(const char *type);
  void *to_koopa() const override;
};

class ConstDefAST : public BaseAST {
public:
  enum { Var, Array } type;
  string ident;
  unique_ptr<BaseAST> exp;
  unique_ptr<vector<unique_ptr<BaseAST>>> index_array;
  ConstDefAST(const char *ident, unique_ptr<BaseAST> &init_array);
  ConstDefAST(
      const char *ident,
      unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
      unique_ptr<BaseAST> &init_array);
  void *to_koopa(koopa_raw_type_t type) const override;
  void *to_koopa(vector<const void *> &global_var,
                 koopa_raw_type_t type) const override;
};

class VarDeclAST : public BaseAST {
public:
  unique_ptr<BaseAST> var_type;
  unique_ptr<vector<unique_ptr<BaseAST>>> VarDef_vec;
  VarDeclAST(unique_ptr<BaseAST> &var_type,
             unique_ptr<vector<unique_ptr<BaseAST>>> &VarDef_vec);
  void *to_koopa() const override;
  void *to_koopa(vector<const void *> &global_var) const override;
};

class VarDefAST : public BaseAST {
public:
  enum VarDefType { Exp, Array };
  VarDefType type;
  string ident;
  unique_ptr<BaseAST> exp;
  unique_ptr<vector<unique_ptr<BaseAST>>> index_array;
  VarDefAST(const char *ident, unique_ptr<BaseAST> &exp, VarDefType type);
  VarDefAST(const char *ident,
            unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
            VarDefType type);
  VarDefAST(const char *ident, VarDefType type);
  VarDefAST(const char *ident,
            unique_ptr<vector<unique_ptr<BaseAST>>> &index_array,
            unique_ptr<BaseAST> &exp, VarDefType type);
  void *to_koopa(koopa_raw_type_t type) const override;
  void *to_koopa(vector<const void *> &global_var,
                 koopa_raw_type_t type) const override;
};

class InitValAST : public BaseAST {
public:
  enum { Exp, InitList, Empty } type;
  unique_ptr<BaseAST> exp;
  unique_ptr<vector<unique_ptr<BaseAST>>> initlist_vec;
  InitValAST();
  InitValAST(unique_ptr<BaseAST> &exp);
  InitValAST(
      unique_ptr<vector<unique_ptr<BaseAST>>> &initlist_vec);
  void *to_koopa(vector<const void *> &init_vec, vector<size_t> size_vec, int level) const override;
  void *to_koopa() const override;
  int cal_value() const override;
  void preprocess(vector<const void *> &init_vec,
                  vector<size_t> size_vec);
};
class LValAST : public BaseAST {
public:
  string ident;
  unique_ptr<vector<unique_ptr<BaseAST>>> index_array;
  LValAST(const char *ident);
  LValAST(const char *ident,
          unique_ptr<vector<unique_ptr<BaseAST>>> &index_array);
  void *to_left_value() const override;
  void *to_koopa() const override;
  int cal_value() const override;
};

class ExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> add_exp;
  ExpAST(unique_ptr<BaseAST>& add_exp);
  void* to_koopa() const override;
  int cal_value() const override;
};

class PrimaryExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;
  PrimaryExpAST(unique_ptr<BaseAST>& exp);
  void* to_koopa() const override;
  int cal_value() const override;
};

class UnaryExpAST : public BaseAST {
 public:
  //enum { Exp, Op } type;
  //LV8
  enum { Exp, Op, Call } type;
  string op;
  unique_ptr<BaseAST> exp;
  unique_ptr<vector<unique_ptr<BaseAST>>> args;
  UnaryExpAST(unique_ptr<BaseAST>& exp);
  UnaryExpAST(const char* op, unique_ptr<BaseAST>& exp);
  UnaryExpAST(const char *op,
              unique_ptr<vector<unique_ptr<BaseAST>>> &args);
  void* to_koopa() const override;
  int cal_value() const override;
};

class AddExpAST : public BaseAST {
 public:
  enum { Exp, Op } type;
  string op;
  unique_ptr<BaseAST> add_exp;
  unique_ptr<BaseAST> mul_exp;
  AddExpAST(unique_ptr<BaseAST>& add_exp);
  AddExpAST(const char* op, unique_ptr<BaseAST>& add_exp,
            unique_ptr<BaseAST>& mul_exp);
  void* to_koopa() const override;
  int cal_value() const override;
};

class MulExpAST : public BaseAST {
 public:
  enum { Exp, Op } type;
  string op;
  unique_ptr<BaseAST> mul_exp;
  unique_ptr<BaseAST> unary_exp;
  MulExpAST(unique_ptr<BaseAST>& unary_exp);
  MulExpAST(const char* op, unique_ptr<BaseAST>& mul_exp,
            unique_ptr<BaseAST>& unary_exp);
  void* to_koopa() const override;
  int cal_value() const override;
};

class RelExpAST : public BaseAST {
 public:
  enum { Exp, Op } type;
  string op;
  unique_ptr<BaseAST> rel_exp;
  unique_ptr<BaseAST> add_exp;
  RelExpAST(unique_ptr<BaseAST>& add_exp);
  RelExpAST(const char* op, unique_ptr<BaseAST>& rel_exp,
            unique_ptr<BaseAST>& add_exp);
  void* to_koopa() const override;
  int cal_value() const override;
};

class EqExpAST : public BaseAST {
 public:
  enum { Exp, Op } type;
  string op;
  unique_ptr<BaseAST> eq_exp;
  unique_ptr<BaseAST> rel_exp;
  EqExpAST(unique_ptr<BaseAST>& rel_exp);
  EqExpAST(const char* op, unique_ptr<BaseAST>& eq_exp,
           unique_ptr<BaseAST>& rel_exp);
  void* to_koopa() const override;
  int cal_value() const override;
};

class LAndExpAST : public BaseAST {
 public:
  enum { Exp, Op } type;
  string op;
  unique_ptr<BaseAST> and_exp;
  unique_ptr<BaseAST> eq_exp;
  LAndExpAST(unique_ptr<BaseAST>& eq_exp);
  LAndExpAST(const char* op, unique_ptr<BaseAST>& and_exp,
             unique_ptr<BaseAST>& eq_exp);
  void* make_bool( const unique_ptr<BaseAST>& exp) const;
  void* to_koopa() const override;
  int cal_value() const override;
};

class LOrExpAST : public BaseAST {
 public:
  enum { Exp, Op } type;
  string op;
  unique_ptr<BaseAST> or_exp;
  unique_ptr<BaseAST> and_exp;
  LOrExpAST(unique_ptr<BaseAST>& and_exp);
  LOrExpAST(const char* op, unique_ptr<BaseAST>& or_exp,
            unique_ptr<BaseAST>& and_exp);
  void* make_bool( const unique_ptr<BaseAST>& exp) const;
  void* to_koopa() const override;
  int cal_value() const override;
};

class NumberAST : public BaseAST {
 public:
  int val;

  NumberAST(int val);

  //string to_string() const override;
  void* to_koopa() const override;
  int cal_value() const override;
};
