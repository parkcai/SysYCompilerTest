// AST.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "koopa.h"
#include "utils.h"

static SymbolList symbol_list;
static BlockManager block_manager;
static LoopManager loop_manager;

// 基类声明
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  
  virtual void Dump() const = 0;
  virtual void *toKoopaIR() const { return nullptr; };
  virtual void *toKoopaIR(std::vector<const void *> &stmts) const { return nullptr; };
  virtual void *toKoopaIR(std::vector<const void *> &stmts, koopa_raw_type_t type) const { return nullptr; };
  virtual void *toKoopaIR(koopa_raw_type_t type) const { return nullptr; };
  virtual void *toKoopaIR(int i) const { return nullptr; };
  virtual void *toKoopaIR(std::vector<const void *> &functions, std::vector<const void *> &values) const { return nullptr; };
  virtual void *toKoopaIR(koopa_raw_type_t type, std::vector<const void *> &values) const { return nullptr; };
  virtual void *toKoopaIR(std::vector<const void *> &init_val_vec, std::vector<int> dim_size) const { return nullptr; };
  virtual int calculate() const { return 0; };
  virtual void *getKoopaIR() const { return nullptr; };
};

// CompUnitAST 派生类
class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> def_array;

  void Dump() const override;
  void *toKoopaIR() const override;
  void DeclineLibFunc(std::vector<const void *> &functions) const;
};

class DefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;
  std::unique_ptr<BaseAST> decl;
  std::string def_type;

  void Dump() const override;
  void *toKoopaIR(std::vector<const void *> &functions, std::vector<const void *> &values) const override;
};

// FuncDefAST 派生类
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_fparams;
  std::unique_ptr<BaseAST> block;

  void Dump() const override;
  void *toKoopaIR() const override;
};

class FuncFParamAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> type;
  std::string ident;
  bool is_array;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> array;

  void Dump() const override;
  void *toKoopaIR() const override;
  void *toKoopaIR(int i) const override;
};

// BlockAST 派生类
class BlockAST : public BaseAST {
 public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> block_item_array;

  void Dump() const override;
  void *toKoopaIR() const override;
  void *toKoopaIR(std::vector<const void *> &stmts) const override;
};

class BlockItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> stmt;
  std::string block_item_type;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
};


class DeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_decl;
  std::unique_ptr<BaseAST> var_decl;
  std::string decl_type;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  void *toKoopaIR(std::vector<const void *> &values) const override;
};

class ConstDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> const_def_array;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  void *toKoopaIR(std::vector<const void *> &values) const override;
};

class VarDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> var_def_array;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  void *toKoopaIR(std::vector<const void *> &values) const override;
};

class BTypeAST : public BaseAST {
 public:
  std::string type;

  void Dump() const override;
  void *toKoopaIR() const override;
};

class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> array;
  std::unique_ptr<BaseAST> const_init_val;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR(koopa_raw_type_t type) const override;
  void *toKoopaIR(koopa_raw_type_t type, std::vector<const void *> &values) const override;
};

class VarDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> array;
  std::unique_ptr<BaseAST> var_init_val;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts, koopa_raw_type_t type) const override;
  void *toKoopaIR(koopa_raw_type_t type) const override;
  void *toKoopaIR(koopa_raw_type_t type, std::vector<const void *> &values) const override;
};

class ConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;

};

class InitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> init_val_array;
  enum { EXP, ARRAY, ZERO } type;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  void *toKoopaIR(std::vector<const void *> &init_val_vec, std::vector<int> dim_size) const override;
  int calculate() const override;
  void restructure(std::vector<int> dim_size, std::vector<const void *> &init_val_vec) const;
};

// StmtAST 派生类
class StmtAST : public BaseAST {
 public:
  std::string stmt_type;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  std::unique_ptr<BaseAST> while_stmt;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
};

class IfAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
};

class ConstExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lor_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class LOrExpAST : public BaseAST {
 public:
  std::string op;
  std::unique_ptr<BaseAST> lor_exp;
  std::unique_ptr<BaseAST> land_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  //void *toBoolKoopaIR(const std::unique_ptr<BaseAST> &exp, std::vector<const void *> &stmts) const;
  void *toBoolKoopaIR(const std::unique_ptr<BaseAST> &exp) const;
  int calculate() const override;
};

class LAndExpAST : public BaseAST {
 public:
  std::string op;
  std::unique_ptr<BaseAST> land_exp;
  std::unique_ptr<BaseAST> eq_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  //void *toBoolKoopaIR(const std::unique_ptr<BaseAST> &exp, std::vector<const void *> &stmts) const;
  void *toKoopaIR() const override;
  void *toBoolKoopaIR(const std::unique_ptr<BaseAST> &exp) const;
  int calculate() const override;
};

class EqExpAST : public BaseAST {
 public:
  std::string op;
  std::unique_ptr<BaseAST> eq_exp;
  std::unique_ptr<BaseAST> rel_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class RelExpAST : public BaseAST {
 public:
  std::string op;
  std::unique_ptr<BaseAST> rel_exp;
  std::unique_ptr<BaseAST> add_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class AddExpAST : public BaseAST {
 public:
  std::string op;
  std::unique_ptr<BaseAST> add_exp;
  std::unique_ptr<BaseAST> mul_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class MulExpAST : public BaseAST {
 public:
  std::string op;
  std::unique_ptr<BaseAST> mul_exp;
  std::unique_ptr<BaseAST> unary_exp;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class UnaryExpAST : public BaseAST {
 public:
  std::string tag;
  std::string unary_op;
  std::unique_ptr<BaseAST> unary_exp;
  std::unique_ptr<BaseAST> primary_exp;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_rparams;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

class PrimaryExpAST : public BaseAST {
 public:
  std::string tag;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> number;
  std::unique_ptr<BaseAST> lval;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

// NumberAST 派生类
class NumberAST : public BaseAST {
 public:
  int int_const;

  NumberAST(){};
  NumberAST(int num);
  void Dump() const override;
  void *toKoopaIR() const override;
  int calculate() const override;
};

// LValAST 派生类
class LValAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> array;

  void Dump() const override;
  //void *toKoopaIR(std::vector<const void *> &stmts) const override;
  void *toKoopaIR() const override;
  int calculate() const override;
  void *getKoopaIR() const override;
};