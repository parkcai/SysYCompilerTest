#pragma once

#include<string>
#include<memory>
#include<iostream>
using std::cout, std::string;

class BaseAST {
 public:
  virtual ~BaseAST() = default;
  virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump() const override {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }
};

struct FuncTypeAST: public BaseAST{
    std::string type;
    void Dump() const override {
        cout << "FuncTypeAST { ";
        cout << type;
        cout << " }";
    }
};
struct BlockAST : public BaseAST
{
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override {
        cout << "BlockAST { ";
        stmt->Dump();
        cout << " }";
    } 
};

struct StmtAST : public BaseAST
{
    string number;
    void Dump() const override {
        cout << "StmtAST { ";
        cout << number;
        cout << " }";
    } 
};
