#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include "../debug/debug.h"
#include <assert.h>
#include <vector>
#include <map>

using std::cout, std::string, std::to_string, std::ofstream,
    std::unique_ptr, std::vector, std::pair;
using namespace std;

extern ofstream out;
extern ofstream debug;
extern int entry_num;

static int symbol_num = 0;
static map<string, pair<int, string>> symbol_tables;

static pair<int, string> look_up_symbol_tables(std::string l_val)
{
  if (symbol_tables.count(l_val))
    return symbol_tables[l_val];
  assert(false);
}
class BaseAST
{
public:
  virtual ~BaseAST() = default;
  virtual string dump() = 0;
  virtual int dumpExp() const
  {
    assert(false);
    return -1;
  }
  BaseAST *father = NULL;
};

class CompUnitAST : public BaseAST
{
public:
  std::unique_ptr<BaseAST> func_def;

  string dump() override
  {
    func_def->father = (BaseAST *)this;
    func_def->dump();
    return "";
  }
};

class FuncDefAST : public BaseAST
{
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  string dump() override
  {
    block->father = (BaseAST *)this;

    out << "fun ";
    out << '@' << ident << "(): ";
    func_type->dump();
    out << "{\n";
    block->dump();
    out << "}\n";
    return "";
  }
};

struct FuncTypeAST : public BaseAST
{
  std::string type;
  string dump() override
  {
    if (type == "int")
      out << "i32 ";
    // else
    // debug << "wrong type: " << type << '\n';
    return "";
  }
};
struct BlockAST : public BaseAST
{
  string name;
  vector<std::unique_ptr<BaseAST>> block_item_list;
  string dump() override
  {
    if (FuncDefAST *func = dynamic_cast<FuncDefAST *>(father))
    {
      name = func->ident;
    }
    out << '%' << name << ":\n";
    for (auto &it : block_item_list)
      it->dump();
    return "";
  }
};
class BlockItemAST : public BaseAST
{
public:
  std::unique_ptr<BaseAST> content;
  string dump() override { return content->dump(); }
};
struct StmtAST : public BaseAST
{
  std::unique_ptr<BaseAST> Exp;
  string LVal;
  string type;
  string dump() override
  {
    if(type == "return")
    {
    auto ret = Exp->dump();
    out << "\tret " << ret << '\n';
    return "";
    }
    else if(type == "assignment")
    {
      std::pair<int, std::string> value = look_up_symbol_tables(LVal);
      auto ret = Exp->dump();
      //store %1, @x
      out << "\tstore "<<ret<<", "<<value.second<<'\n';
      return value.second;
    }
    LINE;
    return "";
  }
};

struct ExpAST : public BaseAST
{
  std::unique_ptr<BaseAST> LOrExp;
  string dump() override
  {
    return LOrExp->dump();
  }
  virtual int dumpExp() const override
  {
    return LOrExp->dumpExp();
  }
};
struct PrimaryExpAST : public BaseAST
{
  string type;
  std::unique_ptr<BaseAST> Exp;
  string LVal;
  int number;
  string dump() override
  {
    if (type == "Exp")
    {
      return Exp->dump();
    }
    else if (type == "Number")
    {
      return to_string(number);
    }
    else if (type == "LVal")
    {
      std::pair<int, std::string> value = look_up_symbol_tables(LVal);
      if (value.second == "")
        return std::to_string(value.first);
      else
      {
        string symbol = "%" + to_string(symbol_num++);
        out<<'\t'<<symbol << " = load "<<value.second << endl;
        return symbol;
      }
    }
    else if (type == "")
    {
      string symbol = "%" + to_string(symbol_num++);
      out << "\t" << symbol << " = add 0, " << symbol << endl;
      return symbol;
    }
    else
      LINE;
    out << __LINE__ << ": " << type << '\n';
    return "";
  }
  int dumpExp() const override
  {
    if (type == "Exp")
    {
      return Exp->dumpExp();
    }
    else if (type == "Number")
    {
      return number;
    }
    else if (type == "LVal")
    {
      std::pair<int, std::string> value = look_up_symbol_tables(LVal);
      if (value.second == "")
        return value.first;
    }
    else if (type == "")
    {
      LINE;
    }
    else
      LINE;
    return -1;
  }
};
struct UnaryExpAST : public BaseAST
{
  string type;
  std::unique_ptr<BaseAST> PrimaryExp;
  string UnaryOp;
  std::unique_ptr<BaseAST> UnaryExp;
  string dump() override
  {
    if (type == "Primary")
    {
      return PrimaryExp->dump();
    }
    else if (type == "Unary")
    {
      string result = UnaryExp->dump();
      if (UnaryOp == "+")
        return result;
      string symbol = "%" + to_string(symbol_num++);
      if (UnaryOp == "-")
      {
        out << "\t" << symbol << " = sub 0, " << result << '\n';
        return symbol;
      }
      else if (UnaryOp == "!")
      {
        out << "\t" << symbol << " = eq " << result << ", 0" << '\n';
        return symbol;
      }
      LINE;
      return "";
    }
    LINE;
    return "";
  }
  int dumpExp() const override
  {
    if (type == "Primary")
    {
      return PrimaryExp->dumpExp();
    }
    else if (type == "Unary")
    {
      int result = UnaryExp->dumpExp();
      if (UnaryOp == "+")
        return result;
      if (UnaryOp == "-")
        return -result;
      else if (UnaryOp == "!")
        return !result;
      LINE;
      return -1;
    }
    LINE;
    return -1;
  }
};
class MulExpAST : public BaseAST
{
public:
  unique_ptr<BaseAST> left;
  string op;
  unique_ptr<BaseAST> right;

  string dump() override
  {
    if (op == "")
      return left->dump();
    string symbol = "%" + to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    if (op == "*")
      out << "\t" << symbol << " = mul " << left_ret << ", " << right_ret << std::endl;
    else if (op == "/")
      out << "\t" << symbol << " = div " << left_ret << ", " << right_ret << std::endl;
    else if (op == "%")
      out << "\t" << symbol << " = mod " << left_ret << ", " << right_ret << std::endl;
    else
      LINE;
    return symbol;
  }
  virtual int dumpExp() const override
  {
    if (op == "")
      return left->dumpExp();
    auto left_ret = left->dumpExp();
    auto right_ret = right->dumpExp();
    if (op == "*")
      return left_ret * right_ret;
    else if (op == "/")
      return left_ret / right_ret;
    else if (op == "%")
      return left_ret % right_ret;
    else
      LINE;
    return -1;
  }
};

// AddExpAST节点类，用于表示加法表达式
class AddExpAST : public BaseAST
{
public:
  unique_ptr<BaseAST> left;
  string op;
  unique_ptr<BaseAST> right;

  string dump() override
  {
    if (op == "")
      return left->dump();
    string symbol = "%" + to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    if (op == "+")
      out << "\t" << symbol << " = add " << left_ret << ", " << right_ret << std::endl;
    else if (op == "-")
      out << "\t" << symbol << " = sub " << left_ret << ", " << right_ret << std::endl;
    else
      LINE;
    return symbol;
  }
  virtual int dumpExp() const override
  {
    if (op == "")
      return left->dumpExp();
    auto left_ret = left->dumpExp();
    auto right_ret = right->dumpExp();
    if (op == "+")
      return left_ret + right_ret;
    else if (op == "-")
      return left_ret - right_ret;
    else
      LINE;
    return -1;
  }
};

// RelExpAST节点类，用于表示关系表达式
class RelExpAST : public BaseAST
{
public:
  unique_ptr<BaseAST> left;
  string op;
  unique_ptr<BaseAST> right;

  string dump() override
  {
    if (op == "")
      return left->dump();
    string symbol = "%" + to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    if (op == "<")
      out << "\t" << symbol << " = lt " << left_ret << ", " << right_ret << std::endl;
    else if (op == ">")
      out << "\t" << symbol << " = gt " << left_ret << ", " << right_ret << std::endl;
    else if (op == "<=")
      out << "\t" << symbol << " = le " << left_ret << ", " << right_ret << std::endl;
    else if (op == ">=")
      out << "\t" << symbol << " = ge " << left_ret << ", " << right_ret << std::endl;
    else
      LINE;
    return symbol;
  }
  virtual int dumpExp() const override
  {
    if (op == "")
      return left->dumpExp();
    auto left_ret = left->dumpExp();
    auto right_ret = right->dumpExp();
    if (op == ">=")
      return left_ret >= right_ret;
    else if (op == "<=")
      return left_ret <= right_ret;
    else if (op == "<")
      return left_ret < right_ret;
    else if (op == ">")
      return left_ret > right_ret;
    else
      LINE;
    return -1;
  }
};

// EqExpAST节点类，用于表示等式表达式
class EqExpAST : public BaseAST
{
public:
  unique_ptr<BaseAST> left;
  string op;
  unique_ptr<BaseAST> right;

  string dump() override
  {
    if (op == "")
      return left->dump();
    string symbol = "%" + to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    if (op == "==")
      out << "\t" << symbol << " = eq " << left_ret << ", " << right_ret << std::endl;
    else if (op == "!=")
      out << "\t" << symbol << " = ne " << left_ret << ", " << right_ret << std::endl;
    else
      LINE;
    return symbol;
  }
  virtual int dumpExp() const override
  {
    if (op == "")
      return left->dumpExp();
    auto left_ret = left->dumpExp();
    auto right_ret = right->dumpExp();
    if (op == "==")
      return left_ret == right_ret;
    else if (op == "!=")
      return left_ret != right_ret;
    else
      LINE;
    return -1;
  }
};

// LAndExpAST节点类，用于表示逻辑与表达式
class LAndExpAST : public BaseAST
{
public:
  unique_ptr<BaseAST> left;
  string op;
  unique_ptr<BaseAST> right;

  string dump() override
  {
    if (op == "")
      return left->dump();
    // string symbol ="%"+to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    assert(op == "&&");
    string symbol_left = "%" + to_string(symbol_num++);
    out << "\t" << symbol_left << " = ne " << left_ret << ", " << 0 << std::endl;
    string symbol_right = "%" + to_string(symbol_num++);
    out << "\t" << symbol_right << " = ne " << right_ret << ", " << 0 << std::endl;
    string symbol = "%" + to_string(symbol_num++);
    out << "\t" << symbol << " = and " << symbol_left << ", " << symbol_right << std::endl;
    return symbol;
  }
  virtual int dumpExp() const override
  {
    if (op == "")
      return left->dumpExp();
    auto left_ret = left->dumpExp();
    if (!left_ret)
      return 0;
    auto right_ret = right->dumpExp();
    return right_ret != 0;
  }
};

// LOrExpAST节点类，用于表示逻辑或表达式
class LOrExpAST : public BaseAST
{
public:
  unique_ptr<BaseAST> left;
  string op;
  unique_ptr<BaseAST> right;

  string dump() override
  {
    if (op == "")
      return left->dump();
    // string symbol ="%"+to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    if (op == "&&")
    {
      std::cerr << op << '\n';
      assert(0);
    }
    string symbol_left = "%" + to_string(symbol_num++);
    out << "\t" << symbol_left << " = ne " << left_ret << ", " << 0 << std::endl;
    string symbol_right = "%" + to_string(symbol_num++);
    out << "\t" << symbol_right << " = ne " << right_ret << ", " << 0 << std::endl;
    string symbol = "%" + to_string(symbol_num++);
    out << "\t" << symbol << " = or " << symbol_left << ", " << symbol_right << std::endl;
    return symbol;
  }
  virtual int dumpExp() const override
  {
    if (op == "")
      return left->dumpExp();
    auto left_ret = left->dumpExp();
    if (left_ret)
      return 1;
    auto right_ret = right->dumpExp();
    return right_ret != 0;
  }
};

struct DeclAST : public BaseAST
{
  std::unique_ptr<BaseAST> ConstDecl;
  string dump() override
  {
    return ConstDecl->dump();
  }
};
struct ConstDeclAST : public BaseAST
{
  string type;
  vector<std::unique_ptr<BaseAST>> const_def_list;
  string dump() override
  {
    for (auto &it : const_def_list)
      it->dump();
    return "";
  }
};
struct ConstDefAST : public BaseAST
{
  // ConstDef      ::= IDENT "=" ConstInitVal;
  string ident;
  std::unique_ptr<BaseAST> ConstInitVal;
  string dump() override
  {
    int value = ConstInitVal->dumpExp();
    symbol_tables[ident] = {value, ""};
    return "";
  }
};
struct ConstInitValAST : public BaseAST
{
  std::unique_ptr<BaseAST> ConstExp;
  string dump() override
  {
    return "";
  }
  int dumpExp() const override
  {
    return ConstExp->dumpExp();
  }
};
struct LValAST : public BaseAST
{
  string ident;
  string dump() override
  {
    return "";
  }
};
struct ConstExpAST : public BaseAST
{
  std::unique_ptr<BaseAST> Exp;
  string dump() override
  {
    return "";
  }
  virtual int dumpExp() const override
  {
    return Exp->dumpExp();
  }
};
struct VarDeclAST : public BaseAST
{
  string type;
  vector<unique_ptr<BaseAST>>VarDefList;
  string dump() override
  {
    for(auto &it : VarDefList)
      it->dump();
    return "";
  }
};
struct VarDefAST : public BaseAST
{
  bool init;
  string ident;
  std::unique_ptr<BaseAST>InitVal;
  string dump() override
  {
    string name = '@' + ident;
    symbol_tables[ident] = {-1, name};
    out << '\t' << name << " = alloc i32" << endl;
    if (init)
    {
      string reg = InitVal->dump();
      out <<"\tstore "<<reg << ", " << name << endl;
    }
    
    return "";
  }
};
struct InitValAST : public BaseAST
{
  string ident;
  std::unique_ptr<BaseAST>Exp;
  string dump() override
  {
    return Exp->dump();
  }
  int dumpExp() const override
  {
    return Exp->dumpExp();
  }
};
// struct FuncDefAST : public BaseAST
// {
//   string type;
//   string ident;
//   vector<unique_ptr<BaseAST>>params;
//   std::unique_ptr<BaseAST>block;
//   string dump() override
//   {
//     return "";
//   }
// };