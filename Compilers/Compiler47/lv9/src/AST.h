#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <fstream>
// #include "../debug/debug.h"
#include <assert.h>
#include <vector>
#include <map>

// using out, std::string, std::to_string, std::ofstream,
//     std::unique_ptr, std::vector, std::pair;
using namespace std;
static void log(string x)
{
  std::ofstream outFile("log.txt", ios::app);
  outFile << x << endl;
  ;
}
static void log(int x)
{
  std::ofstream outFile("log.txt", ios::app);
  outFile << x << endl;
  ;
}
#define bug(x) log(to_string(__LINE__) + ": " + to_string(x))
extern ofstream out;
extern ofstream debug;
extern int entry_num;

static int symbol_num = 0;
static int block_num = 1;
static int label_num = 0;
static int if_num = 0;
static int while_num = 0;
static bool leave = false;
static bool global_var = true;
static int ptr_num = 0;
static string func_ret;
static vector<map<string, pair<int, string>>> symbol_tables;
static vector<pair<string, string>> while_info;
static map<string, string> param_map, ret_type;
static map<string, string> var_type;
static map<string, int> array_siz;

static std::map<std::string, std::vector<std::string>> function_param_idents;
static std::map<std::string, std::vector<std::string>> function_param_names;
static std::map<std::string, std::vector<std::string>> function_param_types;

static int len[1000]; // for array
static int dim = 0;
static vector<int> list_size;
static int *base_ptr;
static string *base_string_ptr;
struct BaseAST;         // forward declaration
struct ConstInitValAST; // forward declaration
static string list_name;
static string params_init;

static void Output(int *ptr, int level, int total_size = 0);
static void Output(string *ptr, int level, int total_size);
static int get_size(int dim)
{
  int ans = 1;
  for (int i = 0; i < dim; i++)
    ans *= len[i];
  return ans;
}
static int get_level(int pos, int dim)
{
  for (int i = 0; i < dim; i++)
  {
    if (pos % len[i] != 0)
      return i;
    pos /= len[i];
  }
  return dim - 1;
}
static string get_ptr()
{
  return "%ptr" + to_string(ptr_num++);
}
static pair<int, string> look_up_symbol_tables(std::string l_val)
{
  for (auto it = symbol_tables.rbegin(); it != symbol_tables.rend(); it++)
  {
    if (it->count(l_val))
      return it->at(l_val);
  }
  assert(false);
}
static inline void zero_dump(int level)
{
  if (level == 0)
    return out << "0", void();
  bool first = true;
  out << "{";
  for (int i = 0; i < len[level - 1]; i++)
  {
    if (first)
      first = false;
    else
      out << ", ";
    zero_dump(level - 1);
  }
  out << "}";
}

struct BaseAST
{
public:
  virtual ~BaseAST() = default;
  virtual string dump() = 0;
  virtual int dumpExp() const
  {
    assert(false);
    log(0); // let g++ happy
    log("0");
    return -1;
  }
  virtual void dumpList()
  {
    assert(false);
    return;
  }
  virtual void dumpList2(int *ptr, int dim)
  {
    assert(false);
    return;
  }
  virtual void dumpList3(string *ptr, int dim)
  {
    assert(false);
    return;
  }
  void *get_pointer()
  {
    return this;
  }
  BaseAST *father = NULL;
  int level = -1;
  string type;
};

class CompUnitAST : public BaseAST
{
public:
  vector<unique_ptr<BaseAST>> func_def_list;
  vector<unique_ptr<BaseAST>> decl_list;

  string dump() override
  {
    out << "decl @getint(): i32" << std::endl;
    out << "decl @getch(): i32" << std::endl;
    out << "decl @getarray(*i32): i32" << std::endl;
    out << "decl @putint(i32)" << std::endl;
    out << "decl @putch(i32)" << std::endl;
    out << "decl @putarray(i32, *i32)" << std::endl;
    out << "decl @starttime()" << std::endl;
    out << "decl @stoptime()" << std::endl
        << std::endl;
    ret_type["getint"] = "int";
    ret_type["getch"] = "int";
    ret_type["getarray"] = "int";
    ret_type["putint"] = "void";
    ret_type["putch"] = "void";
    ret_type["putarray"] = "void";
    ret_type["starttime"] = "void";
    ret_type["stoptime"] = "void";
    std::map<std::string, std::pair<int, std::string>> global_symbol_table;
    symbol_tables.push_back(global_symbol_table);
    for (auto &decl : decl_list)
    {
      decl->dump();
    }
    global_var = false;
    for (auto &func : func_def_list)
    {
      func->father = (BaseAST *)this;
      func->dump();
    }
    symbol_tables.pop_back();
    return to_string(__LINE__);
    ;
  }
};

class FuncDefAST : public BaseAST
{
public:
  string func_type;
  std::string ident;
  vector<std::unique_ptr<BaseAST>> params;
  std::unique_ptr<BaseAST> block;
  string dump() override
  {
    block->father = (BaseAST *)this;

    string name = "@" + ident;
    out << "fun ";
    out << name << "(";
    param_map.clear();
    params_init.clear();
    bool first = true;
    for (auto &it : params)
    {
      if (first)
        first = false;
      else
        out << ',';
      params_init += it->dump();
    }
    out << ")";
    if (func_type == "int")
      out << ": i32", ret_type[ident] = "int",func_ret = "i32";
    else
      ret_type[ident] = "void", func_ret = "void";
    out << "{\n";
    leave = false;
    block->dump();
    if (!leave)
    {
      if (func_type == "int")
        out << "\tret 0" << endl;
      else if (func_type == "void")
        out << "\tret" << endl;
    }
    out << "}\n";
    return to_string(__LINE__);
    ;
  }
};
struct FuncFParamAST : public BaseAST
{
  string b_type;
  string ident;
  vector<unique_ptr<BaseAST>> const_exp_list;
  string dump() override
  {
    string name = "%" + to_string(symbol_num++);
    string newIdent = "@p_" + ident;
    param_map[ident] = name;
    string IdentType;
    out << newIdent << ": ";
    if(type != "list")
    {
      if (b_type == "int")
        IdentType = "i32";
      else if (b_type == "void")
        IdentType = "void";
      else
        assert(0);
      out << IdentType;
    }
    else
    {
      int l = const_exp_list.size();
      var_type[newIdent] = "para";
      array_siz[name] = l + 1;
      //for(int i=0;i<=l;i++)
      IdentType += '*';
      for(int i=0;i<l;i++)
        IdentType += '[';
      IdentType += "i32";
      for(int i=l-1;i>=0;i--)
        IdentType += (", "+ to_string(const_exp_list[i]->dumpExp())+"]");
      out<<IdentType;
    }
    string ret;
    ret += '\t' + name + " = alloc ";
    ret += IdentType + "\n";
    ret += "\tstore " + newIdent + ", " + name + "\n";
    return ret;
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
    for (auto &it : param_map)
    {
      symbol_table[it.first] = {0, it.second};
    }
    symbol_tables.push_back(symbol_table);
    if (name != "")
      out << '%' << name << ":\n";
    out<<params_init;
    params_init = "";
    for (auto &it : block_item_list)
      it->dump();
    symbol_tables.pop_back();
    return to_string(__LINE__);
    ;
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
  std::unique_ptr<BaseAST> SingleStmt;
  std::unique_ptr<BaseAST> ifStmt;
  std::unique_ptr<BaseAST> elseStmt;
  std::unique_ptr<BaseAST> whileStmt;
  std::unique_ptr<BaseAST> forStmt;
  std::unique_ptr<BaseAST> forStmt1;
  std::unique_ptr<BaseAST> forStmt2;

  string dump() override
  {
    if (leave)
      return to_string(__LINE__);
    if (type == "if")
    {
      string result = Exp->dump();
      int cnt_num = if_num++;
      string then = "%if_then" + to_string(cnt_num);
      // string else_ = "%else" + to_string(cnt_num);
      string end = "%if_end" + to_string(cnt_num);
      out << "\tbr  " << result << ", " << then << ", " << end << endl;

      out << then << ":" << endl;
      leave = false;
      ifStmt->dump();
      // cout<<"!!!: "<<lastStmt<<endl;
      if (!leave)
        out << "\tjump " << end << endl;
      out << end << ":" << endl;
      leave = false;
      return to_string(__LINE__);
      ;
    }
    else if (type == "if else")
    {
      string result = Exp->dump();
      int cnt_num = if_num++;
      string then = "%if_then" + to_string(cnt_num);
      string else_ = "%if_else" + to_string(cnt_num);
      string end = "%if_end" + to_string(cnt_num);
      out << "\tbr  " << result << ", " << then << ", " << else_ << endl;

      out << then << ":" << endl;
      leave = false;
      string lastStmt = ifStmt->dump();
      if (lastStmt != "return" && !leave)
        out << "\tjump " << end << endl;

      out << else_ << ":" << endl;
      leave = false;
      string lastStmt2 = elseStmt->dump();
      if (lastStmt2 != "return" && !leave)
        out << "\tjump " << end << endl;

      out << end << ":" << endl;
      leave = false;
      return to_string(__LINE__);
      ;
    }
    else if (type == "single")
    {
      // out<<__LINE__<<'\n';
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
    else if (type == "for")
    {
      int cnt_num = while_num++;
      string entry = "%while_entry" + to_string(cnt_num);
      string body = "%while_body" + to_string(cnt_num);
      string end = "%while_end" + to_string(cnt_num);
      forStmt1->dump();
      while_info.push_back({entry, end});
      out << "\tjump " << entry << endl;
      out << entry << ":" << endl;
      leave = false;
      auto result = Exp->dump();
      out << "\tbr " << result << ", " << body << ", " << end << endl;
      out << body << ":" << endl;
      forStmt->dump();
      if (!leave)
      {
        forStmt2->dump();
        out << "\tjump " << entry << endl;
      }
      out << end << ":" << endl;
      leave = false;
      while_info.pop_back();
    }
    else
      assert(0);
    //cout << __LINE__ << endl;
    return to_string(__LINE__);
    ;
  }
};
struct SingleStmtAST : public BaseAST
{
  std::unique_ptr<BaseAST> Exp;
  std::unique_ptr<BaseAST> Block;
  string LVal;

  vector<unique_ptr<BaseAST>> exp_list;
  string dump() override
  {
    // out<<__LINE__<<'\n';
    if (leave)
      return to_string(__LINE__);
    // out<<type<<'\n';
    if (type == "return")
    {
      auto ret = Exp->dump();
      out << "\tret " << ret << '\n';
      leave = true;
      return "return";
    }
    else if (type == "return empty")
    {
      if(func_ret == "void")
      {
        out << "\tret " << endl;
      }
      else
      {
        int ret = symbol_num++;
        out << "\t%" << ret << " = add 0, 0" << endl;
        out << "\tret %" << ret << endl;
      }
      leave = true;
      return "return";
    }
    else if (type == "empty")
    {
      // pass
      // out<<__LINE__<<'\n';
    }
    else if (type == "assignment")
    {
      std::pair<int, std::string> value = look_up_symbol_tables(LVal);
      auto ret = Exp->dump();
      out << "\tstore " << ret << ", " << value.second << '\n';
      return value.second;
    }
    else if (type == "array assignment")
    {
      string old_ptr = look_up_symbol_tables(LVal).second;
      string ptr;
      for (int i = 0; i < exp_list.size(); i++)
      {
        string pos = exp_list[i]->dump();
        //out<<"fuck "<<<<endl;
        ptr = get_ptr();
        if(i == 0&&param_map.count(LVal))
        {
          string name = "%" + std::to_string(symbol_num++);
          out << '\t' << name << " = load " << old_ptr <<endl;
          out<<"\t"<<ptr<<" = getptr "<<name<<", "<<pos<<endl;
          
        }
        else
        out << "\t" << ptr << " = getelemptr " << old_ptr << ", " << pos << endl;
        old_ptr = ptr;
      }
      auto ret = Exp->dump();
      out << "\tstore " << ret << ", " << ptr << '\n';
      return ptr;
    }
    else if (type == "function call")
    {
      for (int i = 0; i < exp_list.size(); i++)
      {
        exp_list[i]->dump();
      }
      assert(0); // TODO
    }
    else if (type == "block")
    {
      Block->dump();
      return to_string(__LINE__);
      ;
    }
    else if (type == "break")
    {
      out << "\tjump " << while_info.back().second << endl;
      leave = true;
      return to_string(__LINE__);
      ;
    }
    else if (type == "continue")
    {
      out << "\tjump " << while_info.back().first << endl;
      leave = true;
      return to_string(__LINE__);
      ;
    }
    else if (type == "exp")
    {
      Exp->dump();
      return to_string(__LINE__);
    }
    else
      assert(0);
    return to_string(__LINE__);
    ;
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

  std::unique_ptr<BaseAST> Exp;
  string LVal;
  int number;
  vector<std::unique_ptr<BaseAST>> exp_list;
  string dump() override
  {
    if (type == "Exp")
    {
      return Exp->dump();
    }
    else if (type == "Number")
    {
      // cerr<<number<<endl;
      return to_string(number);
    }
    else if (type == "LVal")
    {
      //out<<__LINE__<<endl;
      //if (param_map.count(LVal))
      //  return param_map[LVal];
      std::pair<int, std::string> value = look_up_symbol_tables(LVal);
      if (value.second == "")
        return std::to_string(value.first);
      else
      {//out<<"here!!!!!!!!!!!!"<<endl;
        string symbol = "%" + to_string(symbol_num++);
        if(var_type[value.second]=="list")
          out << '\t' << symbol << " = getelemptr " << value.second <<", 0"<< endl;
        else
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
    else if (type == "array assignment")
    {
      string old_ptr = look_up_symbol_tables(LVal).second;
      string ptr;
      assert(array_siz.count(old_ptr));
      int dim = array_siz[old_ptr];
      
      for (int i = 0; i < exp_list.size(); i++)
      {
        string pos = exp_list[i]->dump();
        //out<<"fuck "<<<<endl;
        ptr = get_ptr();
        if(i == 0&&param_map.count(LVal))
        {
          string name = "%" + std::to_string(symbol_num++);
          out << '\t' << name << " = load   " << old_ptr << endl;
          out<<"\t"<<ptr<<" = getptr "<<name<<", "<<pos<<endl;
          
        }
        else
        out << "\t" << ptr << " = getelemptr " << old_ptr << ", " << pos << endl;
        old_ptr = ptr;
      }
      string symbol = "%" + to_string(symbol_num++);
      if(exp_list.size() == dim)
        out << '\t' << symbol << " = load  " << ptr << endl;
      else
          out << '\t' << symbol << " = getelemptr " << ptr <<", 0"<< endl;
        
      //out << '\t' << symbol << " = load " << ptr << endl;
      return symbol;
    }
    else
      assert(0);
    return to_string(__LINE__);
    ;
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

  std::unique_ptr<BaseAST> PrimaryExp;
  string UnaryOp;
  std::unique_ptr<BaseAST> UnaryExp;
  string ident;
  vector<std::unique_ptr<BaseAST>> params; // for func call
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
      return to_string(__LINE__);
      ;
    }
    else if (type == "Func")
    {
      std::vector<std::string> param_vars;
      for (auto &&param : params)
        param_vars.push_back(param->dump());
      out << '\t';
      string result_var = "";
      if (ret_type[ident] == "int")
      {
        result_var = "%" + to_string(symbol_num++);
        out << result_var << " = ";
      }
      out << "call    @" << ident << '(';
      for (int i = 0; i < param_vars.size(); i++)
      {
        out << param_vars[i];
        if (i != param_vars.size() - 1)
          out << ", ";
      }
      out << ')' << endl;
      return result_var;
      ;
    }
    else
      // assert(0);
      out << __LINE__ << '\n';
    return to_string(__LINE__);
    ;
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

    std::string left_result = left->dump();
    std::string then_label = "\%then__" + std::to_string(if_num);
    std::string else_label = "\%else__" + std::to_string(if_num);
    std::string end_label = "\%end__" + std::to_string(if_num++);
    std::string result_var_ptr = "%" + std::to_string(symbol_num++);
    out << '\t' << result_var_ptr << " = alloc i32" << std::endl;
    out << "\tbr " << left_result << ", " << then_label << ", "
        << else_label << std::endl;
    out << then_label << ":" << std::endl;
    std::string tmp_result_var = "%" + std::to_string(symbol_num++);
    std::string right_result = right->dump();
    out << '\t' << tmp_result_var << " = ne " << right_result
        << ", 0" << std::endl;
    out << "\tstore " << tmp_result_var << ", " << result_var_ptr
        << std::endl;
    out << "\tjump " << end_label << std::endl;
    out << else_label << ":" << std::endl;
    out << "\tstore 0, " << result_var_ptr << std::endl;
    out << "\tjump " << end_label << std::endl;
    out << end_label << ":" << std::endl;
    string result_var = "%" + std::to_string(symbol_num++);
    out << '\t' << result_var << " = load " << result_var_ptr
        << std::endl;
    return result_var;
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
    std::string left_result = left->dump();
    std::string then_label = "\%then__" + std::to_string(if_num);
    std::string else_label = "\%else__" + std::to_string(if_num);
    std::string end_label = "\%end__" + std::to_string(if_num++);
    std::string result_var_ptr = "%" + std::to_string(symbol_num++);
    out << '\t' << result_var_ptr << " = alloc i32" << std::endl;
    out << "\tbr " << left_result << ", " << then_label << ", "
        << else_label << std::endl;
    out << then_label << ":" << std::endl;
    out << "\tstore 1, " << result_var_ptr << std::endl;
    out << "\tjump " << end_label << std::endl;
    out << else_label << ":" << std::endl;
    std::string tmp_result_var = "%" + std::to_string(symbol_num++);
    std::string right_result = right->dump();
    out << '\t' << tmp_result_var << " = ne " << right_result
        << ", 0" << std::endl;
    out << "\tstore " << tmp_result_var << ", " << result_var_ptr
        << std::endl;
    out << "\tjump " << end_label << std::endl;
    out << end_label << ":" << std::endl;
    string result_var = "%" + std::to_string(symbol_num++);
    out << '\t' << result_var << " = load " << result_var_ptr
        << std::endl;
    return result_var;
    // string symbol ="%"+to_string(symbol_num++);
    auto left_ret = left->dump();
    auto right_ret = right->dump();
    if (op == "&&")
    {
      std::cerr << op << '\n';
      assert(0);
    }
    string symbol_left = "%" + to_string(symbol_num++);
    out << "\t" << symbol_left << " = ne " << left_ret << ", " << 0 << endl;
    string symbol_right = "%" + to_string(symbol_num++);
    out << "\t" << symbol_right << " = ne " << right_ret << ", " << 0 << endl;
    string symbol = "%" + to_string(symbol_num++);
    out << "\t" << symbol << " = or " << symbol_left << ", " << symbol_right << endl;
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

  vector<std::unique_ptr<BaseAST>> const_def_list;
  string dump() override
  {
    for (auto &it : const_def_list)
      it->dump();
    return to_string(__LINE__);
    ;
  }
};
struct ConstDefAST : public BaseAST
{

  string ident;
  std::unique_ptr<BaseAST> ConstInitVal;
  vector<unique_ptr<BaseAST>> const_exp_list;
  string dump() override
  {
    if (type == "single")
    {
      int value = ConstInitVal->dumpExp();
      (*symbol_tables.rbegin())[ident] = {value, ""};
      return to_string(__LINE__);
    }
    else if (type == "list")
    {
      string name = '@' + ident + to_string(label_num++);
      list_name = name;
      (*symbol_tables.rbegin())[ident] = {-1, name};
      var_type[name] = "list";
      int l = const_exp_list.size();
      dim = l;
      array_siz[name] = l;
      if (global_var)
        out << "global ";
      else
        out << '\t';
      out << name << " = alloc ";
      for (int i = 0; i < l; i++)
        out << '[';
      out << "i32, ";
      for (int i = l - 1; i >= 0; i--)
      {
        int k = const_exp_list[i]->dumpExp();
        len[l - 1 - i] = k;
        out << k << "]";
        if (i)
          out << ", ";
      }
      if (global_var)
        out << ", " << flush;
      ConstInitVal->dump();
      out << endl;
      return to_string(__LINE__);
    }
    return to_string(__LINE__);
  }
};
struct ConstInitValAST : public BaseAST
{

  std::unique_ptr<BaseAST> ConstExp;
  vector<unique_ptr<BaseAST>> init_val_list;
  string dump() override
  {
    if (type == "ConstExp")
    {
      out << to_string(ConstExp->dumpExp()) << flush;
      return to_string(__LINE__);
    }
    assert(type == "list");
    int total_size = get_size(dim);
    int *ptr = new int[total_size]{};
    base_ptr = ptr;
    dumpList2(ptr, dim);
    //cout << __LINE__ << ' ' << *ptr << ' ' << *(ptr + 1) << endl;
    // cout<<dim<<endl;
    Output(ptr, dim, total_size);

    return "";
  }
  void dumpList2(int *ptr, int dim) override
  {
    for (auto &it : init_val_list)
    {
      if (it->type == "ConstExp")
      {
        *ptr = it->dumpExp();
        //cout << __LINE__ << ' ' << *ptr << endl;
        ++ptr;
        continue;
      }
      assert(it->type == "list");
      it->dumpList2(ptr, dim - 1);
      int level = get_level(base_ptr - ptr, dim);
      ptr += get_size(level);
    }
  }
  int dumpExp() const override
  {
    assert(type == "ConstExp");
    return ConstExp->dumpExp();
  }
};
struct LValAST : public BaseAST
{
  string ident;
  string dump() override
  {
    return to_string(__LINE__);
    ;
  }
};
struct ConstExpAST : public BaseAST
{
  std::unique_ptr<BaseAST> Exp;
  string dump() override
  {
    return Exp->dump();
  }
  virtual int dumpExp() const override
  {
    return Exp->dumpExp();
  }
};
struct VarDeclAST : public BaseAST
{

  vector<unique_ptr<BaseAST>> VarDefList;
  string dump() override
  {
    for (auto &it : VarDefList)
      it->dump();
    return to_string(__LINE__);
    ;
  }
};
struct VarDefAST : public BaseAST
{
  bool init;
  string ident;
  std::unique_ptr<BaseAST> InitVal;
  vector<unique_ptr<BaseAST>> exp_list;
  string dump() override
  {
    if(leave)
      return to_string(__LINE__);
    string name = '@' + ident + to_string(label_num++);
    list_name = name;
    (*symbol_tables.rbegin())[ident] = {-1, name};
    if (!exp_list.empty())
    {
      int l = exp_list.size();
      dim = l;
      array_siz[name] = l;
      var_type[name] = "list";
      if (global_var)
        out << "global ";
      else
        out << '\t';
      out << name << " = alloc ";
      for (int i = 0; i < l; i++)
        out << '[';
      out << "i32, ";
      for (int i = l - 1; i >= 0; i--)
      {
        int k = exp_list[i]->dumpExp();
        len[l - 1 - i] = k;
        out << k << "]";
        if (i)
          out << ", ";
      }
      if (init)
      {
        InitVal->dump();
        out << endl;
      }
      else if (global_var)
      {
        out << ", zeroinit";
        //zero_dump(dim);
      }
      out << endl;
      return to_string(__LINE__);
    }
    // 接下来是int
    if (global_var)
    {
      // global @var = alloc i32, zeroinit
      out << "global " << name << " = alloc i32, ";
      if (init)
      {
        int value = InitVal->dumpExp();
        out << value << endl;
      }
      else
        out << 0 << endl;
      return to_string(__LINE__);
    }
    out << '\t' << name << " = alloc i32" << endl;
    if (init)
    {
      string reg = InitVal->dump();
      out << "\tstore " << reg << ", " << name << endl;
    }

    return to_string(__LINE__);
  }
};
struct InitValAST : public BaseAST
{
  string ident;

  std::unique_ptr<BaseAST> Exp;
  vector<unique_ptr<BaseAST>> init_val_list;
  string dump() override
  {
    if (type == "exp")
    {
      return Exp->dump();
    }
    assert(type == "list");
    int total_size = get_size(dim);
    if (global_var)
    {
      out << ", ";
      int *ptr = new int[total_size]{};
      base_ptr = ptr;
      dumpList2(ptr, dim);
      Output(ptr, dim);
    }
    else
    {
      out << endl;
      string *ptr = new string[total_size]{};
      base_string_ptr = ptr;
      dumpList3(ptr, dim);
      // cout << total_size << endl;
      // for (int i = 0; i < total_size; i++)
      //   cout << ptr[i].size() << ' ' << ptr[i] << endl;
      Output(ptr, dim, total_size);
    }
    return to_string(__LINE__);
  }
  void dumpList2(int *ptr, int dim) override
  {
    for (auto &it : init_val_list)
    {
      if (it->type == "exp")
      {
        *ptr = it->dumpExp();
        //cout << __LINE__ << ' ' << *ptr << endl;
        ++ptr;
        continue;
      }
      assert(it->type == "list");
      it->dumpList2(ptr, dim - 1);
      int level = get_level(base_ptr - ptr, dim);
      ptr += get_size(level);
    }
  }
  void dumpList3(string *ptr, int dim) override
  {
    for (auto &it : init_val_list)
    {
      if (it->type == "exp")
      {
        *ptr = it->dump();
        //cout << __LINE__ << ' ' << *ptr << endl;
        ++ptr;
        continue;
      }
      assert(it->type == "list");
      it->dumpList3(ptr, dim - 1);
      int level = get_level(base_string_ptr - ptr, dim);
      ptr += get_size(level);
    }
  }
  int dumpExp() const override
  {
    return Exp->dumpExp();
  }
};
// struct FuncDefAST : public BaseAST
// {
//
//   string ident;
//   vector<unique_ptr<BaseAST>>params;
//   std::unique_ptr<BaseAST>block;
//   string dump() override
//   {
//     return to_string(__LINE__);;
//   }
// };

// static bool combine(vector<unique_ptr<BaseAST>> &init_val_list, int level)
// {
//   bool ret = false;
//   vector<unique_ptr<BaseAST>> new_list;
//   vector<unique_ptr<BaseAST>> temp;
//   for (int i = 0; i < init_val_list.size(); i++)
//   {
//     cout << init_val_list.size() << endl;
//     assert(init_val_list[i]->level >= level);
//     if (init_val_list[i]->level == level)
//     {
//       temp.push_back(move(init_val_list[i]));
//       if (temp.size() == len[level])
//       {
//         add(init_val_list, new_list, temp, level, ret);
//       }
//     }
//     else
//     {
//       assert(temp.size() == 0);
//       new_list.push_back(move(init_val_list[i]));
//     }
//   }
//   if (temp.size() > 0)
//     add(init_val_list, new_list, temp, level, ret);
//   init_val_list.swap(new_list);
//   return ret;
// }
// static void add(vector<unique_ptr<BaseAST>> &init_val_list,
//                 vector<unique_ptr<BaseAST>> &new_list,
//                 vector<unique_ptr<BaseAST>> &temp,
//                 int &level,
//                 bool &ret)
// {
//   auto it = new ConstInitValAST();
//   it->level = level + 1;
//   it->type = "list";
//   temp.swap(it->init_val_list);
//   new_list.push_back(unique_ptr<BaseAST>(it));
//   ret = true;
// }
static void Output(int *ptr, int level, int total_size)
{
  if (global_var)
  {
    if (level == 0)
    {
      out << *ptr;
      return;
    }
    out << '{';
    int cnt_size = get_size(level - 1);
    for (int i = 0; i < len[level - 1]; i++)
    {
      if (i)
        out << ", ";
      Output(ptr + i * cnt_size, level - 1);
    }
    out << '}';
  }
  else
  {
    string name = list_name;
    for (int i = 1; i < dim; i++)
    {
      string symbol = "%" + to_string(symbol_num++);
      out << "\t" << symbol << " = getelemptr "
          << name << ", 0" << endl;
      name = symbol;
    }
    for (int i = 0; i < total_size; i++)
    {
      string symbol = "%" + to_string(symbol_num++);
      // string new_ptr = get_ptr();
      out << "\t" << symbol << " = getelemptr "
          << name << ", " << i << endl;
      out << "\tstore " << *(ptr + i) << ", " << symbol << endl;
    }
  }
}
static void Output(string *ptr, int level, int total_size)
{
  string name = list_name;
  for (int i = 1; i < dim; i++)
  {
    string symbol = "%" + to_string(symbol_num++);
    out << "\t" << symbol << " = getelemptr "
        << name << ", 0" << endl;
    name = symbol;
  }
  for (int i = 0; i < total_size; i++)
  {
    if ((ptr + i)->size() == 0)
    {
      // continue;
      ptr[i] = "0";
    }
    string symbol = "%" + to_string(symbol_num++);
    // string new_ptr = get_ptr();
    out << "\t" << symbol << " = getelemptr "
        << name << ", " << i << endl;
    out << "\tstore " << *(ptr + i) << ", " << symbol << endl;
  }
  return;
}