#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <fstream>
using std::cout, std::string, std::to_string, std::ofstream;
extern ofstream out;
extern ofstream debug;
extern int entry_num;
class BaseAST
{
public:
  virtual ~BaseAST() = default;
  virtual void Dump() const = 0;
  virtual void dump() = 0;
  BaseAST* father = NULL;
};

class CompUnitAST : public BaseAST
{
public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override
  {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
  }
  void dump() override
  {
    func_def->father = (BaseAST*)this;
    func_def->dump();
  }
};

class FuncDefAST : public BaseAST
{
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump() const override
  {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }
  void dump() override
  {
    block->father = (BaseAST*)this;

    out << "fun ";
    out << '@' << ident << "(): ";
    func_type->dump();
    out << "{\n";
    block->dump();
    out << "}\n";
  }
};

struct FuncTypeAST : public BaseAST
{
  std::string type;
  void Dump() const override
  {
    cout << "FuncTypeAST { ";
    cout << type;
    cout << " }";
  }
  void dump() override
  {
    if (type == "int")
      out << "i32 ";
    else
      debug << "wrong type: " << type << '\n';
  }
};
struct BlockAST : public BaseAST
{
  string name;
  std::unique_ptr<BaseAST> stmt;
  void Dump() const override
  {
    cout << "BlockAST { ";
    stmt->Dump();
    cout << " }";
  }
  void dump() override
  {
    if(FuncDefAST* func = dynamic_cast<FuncDefAST*>(father))
    {
      name = func->ident;
    }
    // out<<'%'<<to_string(++entry_num)<<":\n";
    //out << "\%a" + to_string(++entry_num) << ":\n";
    out<<'%'<<name <<":\n";
    stmt->dump();
  }
};

struct StmtAST : public BaseAST
{
  string number;
  void Dump() const override
  {
    cout << "StmtAST { ";
    cout << number;
    cout << " }";
  }
  void dump() override
  {
    out << "  ret " << number << '\n';
  }
};
