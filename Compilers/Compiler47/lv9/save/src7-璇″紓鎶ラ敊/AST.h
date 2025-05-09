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
static int block_num = 1;
static int label_num = 0;
static int if_num = 0;
static int while_num = 0;
static bool leave = false;
static vector<map<string, pair<int, string>>> symbol_tables;
static vector<pair<string,string>> while_info;
static pair<int, string> look_up_symbol_tables(std::string l_val)
{
  for (auto it = symbol_tables.rbegin(); it != symbol_tables.rend(); it++)
  {
    if (it->count(l_val))
      return it->at(l_val);
  }
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
    std::map<std::string, std::pair<int, std::string>> global_symbol_table;
    symbol_tables.push_back(global_symbol_table);
    func_def->father = (BaseAST *)this;
    func_def->dump();
    symbol_tables.pop_back();
    return to_string(__LINE__);;
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
    if(!leave)out<<"\tret 0"<<endl;
    out << "}\n";
    return to_string(__LINE__);;
  }
};

struct FuncTypeAST : public BaseAST
{
  std::string type;
  string dump() override
  {
    if (type == "int")
      out << "i32 ";
    else if (type == "void")
      out << "void ";
    return to_string(__LINE__);;
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
    // else
    // {
    //   name = "block" + to_string(block_num++);
    // }
    std::map<std::string, std::pair<int, std::string>> symbol_table;
    symbol_tables.push_back(symbol_table);
    if (name != "")
      out << '%' << name << ":\n";
    for (auto &it : block_item_list)
      it->dump();
    symbol_tables.pop_back();
    return to_string(__LINE__);;
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
  string type;
  std::unique_ptr<BaseAST> Exp;
  std::unique_ptr<BaseAST> SingleStmt;
  std::unique_ptr<BaseAST> ifStmt;
  std::unique_ptr<BaseAST> elseStmt;
  std::unique_ptr<BaseAST> whileStmt;
  string dump() override
  {
    if(leave)
      return to_string(__LINE__);
    if (type == "if")
    {
      string result = Exp->dump();
      int cnt_num = if_num++;
      string then = "%then" + to_string(cnt_num);
      // string else_ = "%else" + to_string(cnt_num);
      string end = "%end" + to_string(cnt_num);
      out << "\tbr  " << result << ", " << then << ", " << end << endl;
      
      out << then << ":" << endl;
      leave = false;
      ifStmt->dump();
      //cout<<"!!!: "<<lastStmt<<endl;
      if (!leave)
        out << "\tjump " << end << endl;
      out << end << ":" << endl;
      leave = false;
      return to_string(__LINE__);;
    }
    else if (type == "if else")
    {
      string result = Exp->dump();
      int cnt_num = if_num++;
      string then = "%then" + to_string(cnt_num);
      string else_ = "%else" + to_string(cnt_num);
      string end = "%end" + to_string(cnt_num);
      out << "\tbr  " << result << ", " << then << ", " << else_ << endl;
      
      out << then << ":" << endl;leave = false;
      string lastStmt = ifStmt->dump();
      if (lastStmt != "return"&&!leave)
        out << "\tjump " << end << endl;

      out << else_ << ":" << endl;leave = false;
      string lastStmt2 = elseStmt->dump();
      if (lastStmt2 != "return"&&!leave)
        out << "\tjump " << end << endl;

      out << end << ":" << endl;leave = false;
      return to_string(__LINE__);;
    }
    else if (type == "single")
    {
      return SingleStmt->dump();
    }
    else if (type == "while")
    {
      int cnt_num = while_num++;
      string entry = "%while_entry" + to_string(cnt_num);
      string body = "%while_body" + to_string(cnt_num);
      string end = "%while_end" + to_string(cnt_num);
      while_info.push_back({entry, end});
      out << "\tjump " << entry << endl;
      out << entry << ":" << endl;
      leave = false;
      auto result = Exp->dump();
      out << "\tbr " << result << ", " << body << ", " << end << endl;
      out << body << ":" << endl;
      whileStmt->dump();
      if (!leave)
        out << "\tjump " << entry << endl;
      out << end << ":" << endl;
      leave = false;
      while_info.pop_back();
    }
   cout<<__LINE__<<endl;
  return to_string(__LINE__);;
  }
};
struct SingleStmtAST : public BaseAST
{
  std::unique_ptr<BaseAST> Exp;
  std::unique_ptr<BaseAST> Block;
  string LVal;
  string type;
  string dump() override
  {
    if (leave)
      return to_string(__LINE__);;
    if (type == "return")
    {
      auto ret = Exp->dump();
      out << "\tret " << ret << '\n';
      leave =true;
      return "return";
    }
    else if (type == "return empty")
    {
      int ret = symbol_num++;
      out << "\t%" << ret << " = add 0, 0" << endl;
      out << "\tret " << ret << endl;
      leave =true;
      return "return";
    }
    else if (type == "empty")
    {
      // pass
    }
    else if (type == "assignment")
    {
      std::pair<int, std::string> value = look_up_symbol_tables(LVal);
      auto ret = Exp->dump();
      out << "\tstore " << ret << ", " << value.second << '\n';
      return value.second;
    }
    else if (type == "block")
    {
      Block->dump();
      return to_string(__LINE__);;
    }
    else if (type == "break")
    {
      out << "\tjump " << while_info.back().second << endl;
      leave =true;
      return to_string(__LINE__);;
    }
    else if (type == "continue")
    {
      out << "\tjump " << while_info.back().first << endl;
      leave =true;
      return to_string(__LINE__);;
    }
    return to_string(__LINE__);;
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
        out << '\t' << symbol << " = load " << value.second << endl;
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
      ;
    out << __LINE__ << ": " << type << '\n';
    return to_string(__LINE__);;
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
      ;
    }
    else
      ;
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
      };
      return to_string(__LINE__);;
    };
    return to_string(__LINE__);;
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
      ;
      return -1;
    };
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
      out << "\t" << symbol << " = mul " << left_ret << ", " << right_ret << endl;
    else if (op == "/")
      out << "\t" << symbol << " = div " << left_ret << ", " << right_ret << endl;
    else if (op == "%")
      out << "\t" << symbol << " = mod " << left_ret << ", " << right_ret << endl;
    else
      ;
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
      ;
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
      out << "\t" << symbol << " = add " << left_ret << ", " << right_ret << endl;
    else if (op == "-")
      out << "\t" << symbol << " = sub " << left_ret << ", " << right_ret << endl;
    else
      ;
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
      ;
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
      out << "\t" << symbol << " = lt " << left_ret << ", " << right_ret << endl;
    else if (op == ">")
      out << "\t" << symbol << " = gt " << left_ret << ", " << right_ret << endl;
    else if (op == "<=")
      out << "\t" << symbol << " = le " << left_ret << ", " << right_ret << endl;
    else if (op == ">=")
      out << "\t" << symbol << " = ge " << left_ret << ", " << right_ret << endl;
    else
      ;
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
      ;
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
      out << "\t" << symbol << " = eq " << left_ret << ", " << right_ret << endl;
    else if (op == "!=")
      out << "\t" << symbol << " = ne " << left_ret << ", " << right_ret << endl;
    else
      ;
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
      ;
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
    out << "\t" << symbol_left << " = ne " << left_ret << ", " << 0 << endl;
    string symbol_right = "%" + to_string(symbol_num++);
    out << "\t" << symbol_right << " = ne " << right_ret << ", " << 0 << endl;
    string symbol = "%" + to_string(symbol_num++);
    out << "\t" << symbol << " = and " << symbol_left << ", " << symbol_right << endl;
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
    auto left_ret = left->dump();
    
    string symbol = "@y" + to_string(symbol_num++);
    int cnt_num = if_num++;
    string then = "%then" + to_string(cnt_num);
    string else_ = "%else" + to_string(cnt_num);
    string end = "%end" + to_string(cnt_num);
    out << '\t' << symbol << " = alloc i32" << endl;
    out << "\tbr  " << left_ret << ", " << then << ", " << else_ << endl;
    
    out << then << ":" << endl;leave = false;
    out << "\tstore 1, " << symbol << std::endl;
    //out <<"\t"<<symbol <<" = add 0,"<<left_ret<<endl;
    out << "\tjump " << end << std::endl;

    out << else_ << ":" << endl;leave = false;
    string lastStmt = right->dump();
    string symbol2 = "%" + to_string(symbol_num++);
    out << '\t' << symbol2 << " = ne " << lastStmt
                << ", 0" << std::endl;
    out << "\tstore " <<symbol2<<", " << symbol << std::endl;
    string return_symbol = "%" + to_string(symbol_num++);
    out << '\t' << return_symbol << " = load " << symbol << std::endl;
    out << "\tjump " << end << endl;
    if (!leave)
    {
    }

    out << end << ":" << endl;leave = false;
    return return_symbol;





  //   auto right_ret = right->dump();
  //   if (op == "&&")
  //   {
  //     std::cerr << op << '\n';
  //     assert(0);
  //   }
  //   string symbol_left = "%" + to_string(symbol_num++);
  //   out << "\t" << symbol_left << " = ne " << left_ret << ", " << 0 << endl;
  //   string symbol_right = "%" + to_string(symbol_num++);
  //   out << "\t" << symbol_right << " = ne " << right_ret << ", " << 0 << endl;
  //   string symbol = "%" + to_string(symbol_num++);
  //   out << "\t" << symbol << " = or " << symbol_left << ", " << symbol_right << endl;
  //   return symbol;
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
    return to_string(__LINE__);;
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
    (*symbol_tables.rbegin())[ident] = {value, ""};
    return to_string(__LINE__);;
  }
};
struct ConstInitValAST : public BaseAST
{
  std::unique_ptr<BaseAST> ConstExp;
  string dump() override
  {
    return to_string(__LINE__);;
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
    return to_string(__LINE__);;
  }
};
struct ConstExpAST : public BaseAST
{
  std::unique_ptr<BaseAST> Exp;
  string dump() override
  {
    return to_string(__LINE__);;
  }
  virtual int dumpExp() const override
  {
    return Exp->dumpExp();
  }
};
struct VarDeclAST : public BaseAST
{
  string type;
  vector<unique_ptr<BaseAST>> VarDefList;
  string dump() override
  {
    for (auto &it : VarDefList)
      it->dump();
    return to_string(__LINE__);;
  }
};
struct VarDefAST : public BaseAST
{
  bool init;
  string ident;
  std::unique_ptr<BaseAST> InitVal;
  string dump() override
  {
    string name = '@' + ident + to_string(label_num++);
    (*symbol_tables.rbegin())[ident] = {-1, name};
    out << '\t' << name << " = alloc i32" << endl;
    if (init)
    {
      string reg = InitVal->dump();
      out << "\tstore " << reg << ", " << name << endl;
    }

    return to_string(__LINE__);;
  }
};
struct InitValAST : public BaseAST
{
  string ident;
  std::unique_ptr<BaseAST> Exp;
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
//     return to_string(__LINE__);;
//   }
// };