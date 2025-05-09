#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "symbol_table.hpp"

using namespace std;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void KoopaIR() const = 0;
};

// CompUnit ::= CompUnitItemList
// CompUnitItemList ::= CompUnitItem | CompUnitItemList CompUnitItem  
class CompUnitAST : public BaseAST {
 public:

  unique_ptr<vector<unique_ptr<BaseAST> > > comp_unit_item_list;

  void KoopaIR() const override;
};

// CompUnitItem ::= Decl | FuncDef
class CompUnitItemAST : public BaseAST {
 public:
  unique_ptr<BaseAST> decl;
  unique_ptr<BaseAST> func_def;
  int type;

  void KoopaIR() const override;
};



// Decl ::= ConstDecl | VarDecl
class DeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> const_decl;
  unique_ptr<BaseAST> var_decl;
  int type;

  void KoopaIR() const override;
};

// ConstDecl ::= CONST Type ConstDefList ';'
// ConstDefList ::= ConstDef | ConstDefList ',' ConstDef
class ConstDeclAST : public BaseAST {
 public:
  string Type;
  unique_ptr<vector<unique_ptr<BaseAST> > > const_def_list;

  void KoopaIR() const override;
};



// ConstDef ::= IDENT ConstIndexList '=' CosntInitVal
// ConstIndexList ::= ε | ConstIndexList '[' ConstExp ']'
class ConstDefAST : public BaseAST {
 public:
  string ident;
  unique_ptr<BaseAST> const_init_val;
  unique_ptr<vector<unique_ptr<BaseAST> > > const_index_list;

  void KoopaIR() const override;
};

// ConstInitVal ::= ConstExp | ConstArrayInitVal
// ConstArrayInitVal ::= '{' '}' | '{' ConstInitValList '}'
// ConstInitValList ::= ConstInitVal | ConstInitValList ',' ConstInitVal
class ConstInitValAST : public BaseAST {
 public:
  int type;
  unique_ptr<BaseAST> const_exp;
  unique_ptr<vector<unique_ptr<BaseAST> > > const_init_val_list;

  void KoopaIR() const override;
  int getValue() const;

  // 计算并补全初始化列表
  // 常量初始化列表中只能出现常量表达式, 返回的 vector<int> 中即为补足 0 的各项的值
  vector<int> make_aggregate(const vector<int>& mul_len, int pos) const;
};

// VarDecl ::= Type VarDefList ';'
// VarDefList ::= VarDef | VarDefList ',' VarDef
class VarDeclAST : public BaseAST {
 public:
  string Type;
  unique_ptr<vector<unique_ptr<BaseAST> > > var_def_list;

  void KoopaIR() const override;
};

// VarDef ::= IDENT ConstIndexList | IDENT ConstIndexList '=' InitVal
// ConstIndexList ::= ε | ConstIndexList '[' ConstExp ']'
class VarDefAST : public BaseAST {
 public:
  string ident;
  int type;
  unique_ptr<BaseAST> init_val;
  unique_ptr<vector<unique_ptr<BaseAST> > > const_index_list;

  void KoopaIR() const override;
};

// InitVal ::= Exp | ArrayInitVal
// ArrayInitVal ::= '{' '}' | '{' InitValList '}'
// InitValList ::= InitVal | InitValList ',' InitVal
class InitValAST : public BaseAST {
 public:
  int type;
  unique_ptr<BaseAST> exp;
  unique_ptr<vector<unique_ptr<BaseAST> > > init_val_list;

  void KoopaIR() const override;
  int getValue() const;

  vector<int> make_aggregate(const vector<int>& mul_len, int pos) const;
};


// FuncDef ::= Type Ident '(' FuncFParams ')' Block
// FuncFParams ::= ε | FuncFParamList 
// FuncFParamList ::= FuncFParam | FuncFParamList ',' FuncFParam
class FuncDefAST : public BaseAST {
 public:
  string Type;
  string ident;
  unique_ptr<BaseAST> block;
  unique_ptr<vector<unique_ptr<BaseAST> > > func_f_param_list;

  void KoopaIR() const override;
};

// FuncFParam ::= Type IDENT | Type IDENT '[' ']' ConstIndexList
class FuncFParamAST : public BaseAST {
 public:
  int type;
  string Type;
  string ident;
  unique_ptr<vector<unique_ptr<BaseAST> > > const_index_list;

  void KoopaIR() const override;
};



// Block ::= '{' BlockItemList '}'
// BlockItemList ::= ε | BlockItemList BlockItem
class BlockAST : public BaseAST {
  public:
     unique_ptr<vector<unique_ptr<BaseAST> > > block_item_list;

  void KoopaIR() const override;
};

// BlockItem ::= Decl | Stmt
class BlockItemAST : public BaseAST {
  public:
    int type;
    unique_ptr<BaseAST> decl;
    unique_ptr<BaseAST> stmt;

  void KoopaIR() const override;
};

// Stmt ::= Matched_Stmt | Open_Stmt
class StmtAST : public BaseAST {
  public:
    int type;
    unique_ptr<BaseAST> matched_stmt;
    unique_ptr<BaseAST> open_stmt;

  void KoopaIR() const override;
};

// Matched_Stmt ::= LVal '=' Exp ';' | ';' | Exp ';' | Block | "return" ';' | "return" Exp ';'
//                | IF '(' Exp ')' Matched_Stmt ELSE Matched_Stmt 
//                | WHILE '(' Exp ')' Matched_Stmt
class MatchedStmtAST : public BaseAST {
  public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    int type;
    unique_ptr<BaseAST> block;
    unique_ptr<BaseAST> matched_stmt1;
    unique_ptr<BaseAST> matched_stmt2;

  void KoopaIR() const override;
};

// Open_Stmt ::= IF '(' Exp ')' Stmt | IF '(' Exp ')' Matched_Stmt ELSE Open_Stmt
//             | WHILE '(' Exp ')' Open_Stmt 
class OpenStmtAST : public BaseAST {
  public:
    int type;
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmt;
    unique_ptr<BaseAST> matched_stmt;
    unique_ptr<BaseAST> open_stmt;

  void KoopaIR() const override;
};


// Expression Base Class
class ExpBaseAST : public BaseAST {
  public:
    virtual int getValue() const = 0;
};

// Exp ::= LOrExp
class ExpAST : public ExpBaseAST {
  public:
    unique_ptr<BaseAST> lor_exp;

  void KoopaIR() const override;
  int getValue() const override;
};

// LVal ::= IDENT IndexList
// IndexList ::= ε | IndexList '[' Exp ']'
class LValAST : public ExpBaseAST {
  public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > index_list;

  void KoopaIR() const override;
  int getValue() const override;
};

// PrimaryExp ::= "(" Exp ")" | LVal | Number
class PrimaryExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    int number;

  void KoopaIR() const override;
  int getValue() const override;
};

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp | IDENT '(' FuncRParams ')'
// FuncRParams ::= ε | FuncRParamList
// FuncRParamList ::= FuncRParam | FuncRParamList ',' FuncRParam
class UnaryExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> primary_exp;
    char unary_op;
    unique_ptr<BaseAST> unary_exp;
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > func_r_param_list;

  void KoopaIR() const override;
  int getValue() const override;
};

// 

// AddExp ::= MulExp | AddExp AddOp MulExp
class AddExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> mul_exp;
    unique_ptr<BaseAST> add_exp;
    char add_op;

  void KoopaIR() const override;
  int getValue() const override;
};

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp
class MulExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> unary_exp;
    unique_ptr<BaseAST> mul_exp;
    char mul_op;

  void KoopaIR() const override;
  int getValue() const override;
};

// RelExp ::= AddExp | RelExp RelOp AddExp
// RelOp ::= "<" | "<=" | ">" | ">=" 
class RelExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> add_exp;
    unique_ptr<BaseAST> rel_exp;
    string rel_op;

  void KoopaIR() const override;
  int getValue() const override;
};

// EqExp ::= RelExp | EqExp EqOp RelExp
// EqOp ::= "==" | "!="
class EqExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> rel_exp;
    unique_ptr<BaseAST> eq_exp;
    string eq_op;

  void KoopaIR() const override;
  int getValue() const override;
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp
class LAndExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> eq_exp;
    unique_ptr<BaseAST> land_exp;

  void KoopaIR() const override;
  int getValue() const override;
};

// LOrExp ::= LAndExp | LOrExp "||" LAndExp
class LOrExpAST : public ExpBaseAST {
  public:
    int type;
    unique_ptr<BaseAST> land_exp;
    unique_ptr<BaseAST> lor_exp;

  void KoopaIR() const override;
  int getValue() const override;
};

// ConstExp ::= Exp
class ConstExpAST : public ExpBaseAST {
  public:
    unique_ptr<BaseAST> exp;

  void KoopaIR() const override;
  int getValue() const override;
};

