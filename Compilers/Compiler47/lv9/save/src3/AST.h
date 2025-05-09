#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include "../debug/debug.h"
#include <assert.h>
using std::cout, std::string, std::to_string, std::ofstream,
std::unique_ptr;
extern ofstream out;
extern ofstream debug;
extern int entry_num;

static int symbol_num = 0;

class BaseAST
{
public:
  virtual ~BaseAST() = default;
  virtual string dump() = 0;
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
    //else
      //debug << "wrong type: " << type << '\n';
    return "";
  }
};
struct BlockAST : public BaseAST
{
  string name;
  std::unique_ptr<BaseAST> stmt;
  string dump() override
  {
    if (FuncDefAST *func = dynamic_cast<FuncDefAST *>(father))
    {
      name = func->ident;
    }
    // out<<'%'<<to_string(++entry_num)<<":\n";
    // out << "\%a" + to_string(++entry_num) << ":\n";
    out << '%' << name << ":\n";
    stmt->dump();
    return "";
  }
};

struct StmtAST : public BaseAST
{
  std::unique_ptr<BaseAST>Exp;
  string dump() override
  {
    auto ret = Exp->dump();
    out << "\tret " << ret << '\n';
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
};
struct PrimaryExpAST : public BaseAST
{
  string type;
  std::unique_ptr<BaseAST> Exp;
  int number;
  string dump() override
  {
    if(type == "Exp")
    {
      return Exp->dump();
    }
    else if(type == "Number")
    {
      return to_string(number);
    }
    else if(type == "")
    {
      string symbol ="%"+to_string(symbol_num++);
      out << "\t" << symbol << " = add 0, " << symbol<<'\n';
      return symbol;
    }
    else
    LINE;
    out<<__LINE__<<": "<<type<<'\n';
    return "";
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
    if(type == "Primary")
    {
      return PrimaryExp->dump();
    }
    else if(type == "Unary")
    {
      string result = UnaryExp->dump();
      if(UnaryOp == "+")
        return result;
      string symbol ="%"+to_string(symbol_num++);
      if(UnaryOp == "-")
      {
        out << "\t" << symbol << " = sub 0, " << result<<'\n';
        return symbol;
      }
      else if(UnaryOp == "!")
      {
        out << "\t" << symbol << " = eq " << result<< ", 0"<<'\n';
        return symbol;
      }
      LINE;
      return "";
    }
    LINE;
    return "";
  }
};
class MulExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> left;
    string op;
    unique_ptr<BaseAST> right;

    string dump()override
    {
      if(op=="")
        return left->dump();
      string symbol ="%"+to_string(symbol_num++);
      auto left_ret = left->dump();
      auto right_ret = right->dump();
      if(op=="*")
        out<<"\t"<<symbol<< " = mul " << left_ret <<", " << right_ret << std::endl;
      else if(op=="/")
        out<<"\t"<<symbol<< " = div " << left_ret <<", " << right_ret << std::endl;
      else if(op=="%")
        out<<"\t"<<symbol<< " = mod " << left_ret <<", " << right_ret << std::endl;
      else
      LINE;
      return symbol;
    }
};

// AddExpAST节点类，用于表示加法表达式
class AddExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> left;
    string op;
    unique_ptr<BaseAST> right;

    string dump() override
    {
      if(op=="")
        return left->dump();
      string symbol ="%"+to_string(symbol_num++);
      auto left_ret = left->dump();
      auto right_ret = right->dump();
      if(op=="+")
        out<<"\t"<<symbol<< " = add " << left_ret <<", " << right_ret << std::endl;
      else if(op=="-")
        out<<"\t"<<symbol<< " = sub " << left_ret <<", " << right_ret << std::endl;
      else
      LINE;
      return symbol;
    }
};

// RelExpAST节点类，用于表示关系表达式
class RelExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> left;
    string op;
    unique_ptr<BaseAST> right;

    string dump() override
    {
      if(op=="")
        return left->dump();
      string symbol ="%"+to_string(symbol_num++);
      auto left_ret = left->dump();
      auto right_ret = right->dump();
      if(op=="<")
        out<<"\t"<<symbol<< " = lt " << left_ret <<", " << right_ret << std::endl;
      else if(op==">")
        out<<"\t"<<symbol<< " = gt " << left_ret <<", " << right_ret << std::endl;
      else if(op=="<=")
        out<<"\t"<<symbol<< " = le " << left_ret <<", " << right_ret << std::endl;
      else if(op==">=")
        out<<"\t"<<symbol<< " = ge " << left_ret <<", " << right_ret << std::endl;
      else
      LINE;
      return symbol;
    }
};

// EqExpAST节点类，用于表示等式表达式
class EqExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> left;
    string op;
    unique_ptr<BaseAST> right;

    string dump() override
    {
      if(op=="")
        return left->dump();
      string symbol ="%"+to_string(symbol_num++);
      auto left_ret = left->dump();
      auto right_ret = right->dump();
      if(op=="==")
        out<<"\t"<<symbol<< " = eq " << left_ret <<", " << right_ret << std::endl;
      else if(op=="!=")
        out<<"\t"<<symbol<< " = ne " << left_ret <<", " << right_ret << std::endl;
      else
      LINE;
      return symbol;
    }
};

// LAndExpAST节点类，用于表示逻辑与表达式
class LAndExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> left;
    string op;
    unique_ptr<BaseAST> right;

    string dump() override
    {
      if(op=="")
        return left->dump();
      //string symbol ="%"+to_string(symbol_num++);
      auto left_ret = left->dump();
      auto right_ret = right->dump();
      assert(op == "&&");
      string symbol_left ="%"+to_string(symbol_num++);
      out<<"\t"<<symbol_left<< " = ne " << left_ret <<", " << 0 << std::endl;
      string symbol_right ="%"+to_string(symbol_num++);
      out<<"\t"<<symbol_right<< " = ne " << right_ret <<", " << 0 << std::endl;
      string symbol ="%"+to_string(symbol_num++);
      out<<"\t"<<symbol<< " = and " << symbol_left <<", " << symbol_right << std::endl;
      return symbol;
    }
};

// LOrExpAST节点类，用于表示逻辑或表达式
class LOrExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> left;
    string op;
    unique_ptr<BaseAST> right;

    string dump() override
    {
      if(op=="")
        return left->dump();
      //string symbol ="%"+to_string(symbol_num++);
      auto left_ret = left->dump();
      auto right_ret = right->dump();
      if(op == "&&")
      {
        std::cerr<<op<<'\n';
        assert(0);
      }
      string symbol_left ="%"+to_string(symbol_num++);
      out<<"\t"<<symbol_left<< " = ne " << left_ret <<", " << 0 << std::endl;
      string symbol_right ="%"+to_string(symbol_num++);
      out<<"\t"<<symbol_right<< " = ne " << right_ret <<", " << 0 << std::endl;
      string symbol ="%"+to_string(symbol_num++);
      out<<"\t"<<symbol<< " = or " << symbol_left <<", " << symbol_right << std::endl;
      return symbol;
    }
};

