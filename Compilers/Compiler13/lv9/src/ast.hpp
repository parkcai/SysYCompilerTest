#pragma once

#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <memory>
#include <typeinfo>
#include <algorithm>

#define GLOBAL_DATA -1

#define CONST_SYM 0
#define VAR_SYM   1
#define FUNC_SYM  2
#define ARR_SYM   3
#define PTR_SYM   4

using namespace std;

static bool in_func = false;

static int IR_cnt = 0;
static int ptr_cnt = 0;
static int tab_cnt = 0;
static int if_label_cnt = 0;
static int short_circuit_cnt = 0;
static int loop_cnt = 0;

static vector<int> loop_stk;

struct symbol {
  int type;
  string val;
  vector<int> dim;
  symbol (int t, string v) {
    type=t;
    val=v;
  }
  symbol (int t, vector<int> d) {
    type=t;
    dim=d;
  }
  symbol () {}
};

struct table {
  int cnt;
  map<string, symbol> tab;
  table (int c, map<string, symbol> t) {
    cnt = c;
    tab = t;
  }
  table() {}

  void newTable() {
    cnt = tab_cnt++;
    tab.clear();
  }
};

static table global_func_tab;
static table global_symbol_tab;
static table symbol_tab;
static vector<table> tab_stk;
static set<string> lib_func;

class BlockAST;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual string Koopa() = 0;

  bool returned = false;

  int type_code;

  static int calcuKoopa(string koopa_result) {
    // cout << koopa_result << endl;
    map<string, int> temp_map;
    istringstream stream(koopa_result);
    string token, pre_token;
    while (stream >> token) {
      if (token == "jump") {
        string tar_label;
        stream >> tar_label;
        string temp_1 = string(tar_label.begin() + tar_label.find('(') + 1, 
                             tar_label.end() - 1);
        tar_label = string(tar_label.begin(), 
                           tar_label.begin() + tar_label.find('('));
        while (stream >> token) {
          if (token.find(tar_label) != string::npos && token[token.size() - 1] == ':') {
            string temp_2 = string(token.begin() + token.find('(') + 1, 
                                   token.end() - 1);
            temp_map[temp_2] = temp_map[temp_1];
            stream >> token;
            break;
          }
        }
      }
      else if (token == "br") {
        string cond_temp;
        stream >> cond_temp;
        string tar_label;
        if (temp_map[cond_temp.substr(0, cond_temp.size() - 1)]) {
          stream >> tar_label;
          tar_label = tar_label.substr(0, tar_label.size() - 1);
        }
        else {
          stream >> tar_label;
          stream >> tar_label;
        }
        while (stream >> token) {
          if (token.find(tar_label) != string::npos && token[token.size() - 1] == ':') 
            break;
        }
      }
      else {  
        string op;
        stream >> op;
        stream >> op;
        string temp_1, temp_2;
        stream >> temp_1 >> temp_2;
        temp_1.pop_back();

        int int_1, int_2;
        if (temp_1[0] != '%' && temp_1[0] != '@') {
          int_1 = stoi(temp_1);
        }
        else if (temp_1[0] == '%') {
          int_1 = temp_map[temp_1];
        }
        else if (temp_1[0] == '@') {
          temp_1 = temp_1.substr(0, temp_1.find_last_of('-'));
          int search_result = BaseAST::searchTab(temp_1);
          if (search_result == GLOBAL_DATA)
            int_1 = stoi(global_symbol_tab.tab[temp_1].val);
          else if (search_result == tab_stk.size())
            int_1 = stoi(symbol_tab.tab[temp_1].val);
          else 
            int_1 = stoi(tab_stk[search_result].tab[temp_1].val);
        }

        if (temp_2[0] != '%' && temp_2[0] != '@') {
          int_2 = stoi(temp_2);
        }
        else if (temp_2[0] == '%') {
          int_2 = temp_map[temp_2];
        }
        else if (temp_2[0] == '@') {
          temp_2 = temp_2.substr(0, temp_2.find_last_of('-'));
          int search_result = BaseAST::searchTab(temp_2);
          if (search_result == GLOBAL_DATA)
            int_2 = stoi(global_symbol_tab.tab[temp_2].val);
          else if (search_result == tab_stk.size())
            int_2 = stoi(symbol_tab.tab[temp_2].val);
          else 
            int_2 = stoi(tab_stk[search_result].tab[temp_2].val);
        }
        
        if (op == "ne") 
          temp_map[token] = (int_1 != int_2);
        else if (op == "eq")
          temp_map[token] = (int_1 == int_2);
        else if (op == "gt")
          temp_map[token] = (int_1 > int_2);
        else if (op == "lt")
          temp_map[token] = (int_1 < int_2);
        else if (op == "ge")
          temp_map[token] = (int_1 >= int_2);
        else if (op == "le")
          temp_map[token] = (int_1 <= int_2);
        else if (op == "add")
          temp_map[token] = (int_1 + int_2);
        else if (op == "sub")
          temp_map[token] = (int_1 - int_2);
        else if (op == "mul")
          temp_map[token] = (int_1 * int_2);
        else if (op == "div")
          temp_map[token] = (int_1 / int_2);
        else if (op == "mod")
          temp_map[token] = (int_1 % int_2);
        else if (op == "and")
          temp_map[token] = (int_1 & int_2);
        else if (op == "or")
          temp_map[token] = (int_1 | int_2);
        else if (op == "xor")
          temp_map[token] = (int_1 ^ int_2);

        pre_token = token;
      }
    }
    return temp_map[pre_token];
  }

  static string parseIR(string koopa_result) {
    istringstream stream(koopa_result);
    string temp;
    stream >> temp;
    stream >> temp;
    return temp;
  }

  static int searchTab(string ident) {
    if (symbol_tab.tab.count(ident))
      return tab_stk.size();
    else {
      for (int i=tab_stk.size() - 1; i >=0; i--)
        if (tab_stk[i].tab.count(ident))
          return i;
    }
    if (global_symbol_tab.tab.count(ident))
          return GLOBAL_DATA;
    return -2;
  }

  static string linkLib() {
    string koopa_result;
    global_func_tab.tab["getint"] = symbol(FUNC_SYM, ": i32");
    global_func_tab.tab["getch"] = symbol(FUNC_SYM, ": i32");
    global_func_tab.tab["getarray"] = symbol(FUNC_SYM, ": i32");
    global_func_tab.tab["putint"] = symbol(FUNC_SYM, "");
    global_func_tab.tab["putch"] = symbol(FUNC_SYM, "");
    global_func_tab.tab["putarray"] = symbol(FUNC_SYM, "");
    global_func_tab.tab["starttime"] = symbol(FUNC_SYM, "");
    global_func_tab.tab["stoptime"] = symbol(FUNC_SYM, "");
    lib_func.insert("getint");
    lib_func.insert("getch");
    lib_func.insert("getarray");
    lib_func.insert("putint");
    lib_func.insert("putch");
    lib_func.insert("putarray");
    lib_func.insert("starttime");
    lib_func.insert("stoptime");
    lib_func.insert("main");
    koopa_result += "decl @getint(): i32\n";
    koopa_result += "decl @getch(): i32\n";
    koopa_result += "decl @getarray(*i32): i32\n";
    koopa_result += "decl @putint(i32)\n";
    koopa_result += "decl @putch(i32)\n";
    koopa_result += "decl @putarray(i32, *i32)\n";
    koopa_result += "decl @starttime()\n";
    koopa_result += "decl @stoptime()\n";
    koopa_result += "\n";
    return koopa_result;
  }  
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  vector<unique_ptr<BaseAST> > func_def_vec;
  vector<unique_ptr<BaseAST> > glb_decl_vec;
  vector<int> parse_tag;

  string Koopa() override {
    string koopa_result;
    koopa_result += BaseAST::linkLib();
    for (int tag=0, func=0, glb=0; tag<parse_tag.size(); tag++) {
      if (parse_tag[tag] == 0) {
        in_func = true;
        koopa_result += func_def_vec[func++]->Koopa();
        IR_cnt = 0;
        ptr_cnt = 0;
        in_func = false;
      }
      else if (parse_tag[tag] == 1) {
        koopa_result += glb_decl_vec[glb++]->Koopa();
      }
      // cout << koopa_result << endl;
    }
    return koopa_result;
  }
};

// FuncFParams 是 BaseAST
class FuncFParamsAST : public BaseAST {
 public:
  vector<unique_ptr<BaseAST> > func_f_param_vec; 

  string Koopa() override {
    // redundant
    return "";
  }
};

// FuncFParam 是 BaseAST
class FuncFParamAST : public BaseAST {
 public:
  unique_ptr<BaseAST> type;
  string ident; 
  vector<unique_ptr<BaseAST> > const_exp_vec;

  string fparam_type;
  vector<int> fparam_dim;

  string Koopa() override {
    if (type_code == 0) {
      fparam_type = type->Koopa();
      return "@" + ident + " : " + fparam_type;
    }
    else if (type_code == 1) {
      string type_result = type->Koopa();
      for (int i=const_exp_vec.size() - 1; i>=0; i--) {
        int curr_IR = IR_cnt;
        string exp_result = const_exp_vec[i]->Koopa();
        IR_cnt = curr_IR;
        string temp = BaseAST::parseIR(exp_result);
        if (temp[0] == '=') {
          exp_result = to_string(BaseAST::calcuKoopa(exp_result));
        }
        type_result = "[" + type_result + ", " + exp_result + "]";
        fparam_dim.insert(fparam_dim.begin(), stoi(exp_result));
      }
      fparam_dim.push_back(0);
      fparam_type = "*" + type_result;
      return "@" + ident + " : " + fparam_type;
    }
  }
};

// Type 是 BaseAST
class TypeAST : public BaseAST {
 public:
  string type_name;

  string Koopa() override {
    if (type_code == 1) return "i32";
    else if (type_name == "int") return ": i32";
    else if (type_name == "void") return "";
  }

};

// FuncDef 是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  unique_ptr<BaseAST> type;
  string ident;
  unique_ptr<BaseAST> block;
  vector<unique_ptr<BaseAST> > func_f_param_vec; 

  string Koopa() override {
    string type_koopa = type->Koopa();
    global_func_tab.tab[this->ident] = symbol(FUNC_SYM, type_koopa);
    if (type_code == 0) {
      string koopa_result = block->Koopa();
      if (ident == "main") {
        if (block->returned)
          return "fun @" + ident + "()" + type_koopa + 
                " {\n%entry:\n" + koopa_result + "}\n\n";
        else if (type_koopa == ": i32")
          return "fun @" + ident + "()" + type_koopa + 
                " {\n%entry:\n" + koopa_result + "  ret 0\n}\n\n";
        else if (type_koopa == "")
          return "fun @" + ident + "()" + type_koopa + 
                " {\n%entry:\n" + koopa_result + "  ret\n}\n\n";
      }
      else {
        if (block->returned)
          return "fun @" + ident + "_func()" + type_koopa + 
                " {\n%entry:\n" + koopa_result + "}\n\n";
        else if (type_koopa == ": i32")
          return "fun @" + ident + "_func()" + type_koopa + 
                " {\n%entry:\n" + koopa_result + "  ret 0\n}\n\n";
        else if (type_koopa == "")
          return "fun @" + ident + "_func()" + type_koopa + 
                " {\n%entry:\n" + koopa_result + "  ret\n}\n\n";
      }
    }
    else if (type_code == 1) {
      tab_stk.push_back(symbol_tab);
      symbol_tab.newTable();

      string param_koopa  = func_f_param_vec[0]->Koopa();
      for (int i=1; i<func_f_param_vec.size(); i++)
        param_koopa += ", " + func_f_param_vec[i]->Koopa();

      string alloc_koopa;
      for (int i=0; i<func_f_param_vec.size(); i++) {
        string fparam_ident = (dynamic_cast<FuncFParamAST*>(func_f_param_vec[i].get()))->ident;
        string fparam_type = (dynamic_cast<FuncFParamAST*>(func_f_param_vec[i].get()))->fparam_type;
        vector<int> fparam_dim = (dynamic_cast<FuncFParamAST*>(func_f_param_vec[i].get()))->fparam_dim;
        alloc_koopa += "  @" + fparam_ident + "_" + to_string(symbol_tab.cnt) + " = alloc " + fparam_type + "\n";
        alloc_koopa += "  store @" + fparam_ident + ", @" + fparam_ident + "_" + to_string(symbol_tab.cnt) + "\n";
        if (fparam_type[0] == '*')
          symbol_tab.tab[fparam_ident] = symbol(PTR_SYM, fparam_dim);
        else
          symbol_tab.tab[fparam_ident] = symbol(VAR_SYM, "");
      }

      string koopa_result = alloc_koopa + block->Koopa();

      symbol_tab.cnt = tab_stk[tab_stk.size() - 1].cnt;
      symbol_tab.tab = tab_stk[tab_stk.size() - 1].tab;
      tab_stk.pop_back();

      if (block->returned)
        return "fun @" + ident + "_func(" + param_koopa + ")" + type_koopa + 
              " {\n%entry:\n" + koopa_result + "}\n\n";
      else if (type_koopa == ": i32")
        return "fun @" + ident + "_func(" + param_koopa + ")" + type_koopa + 
              " {\n%entry:\n" + koopa_result + "  ret 0\n}\n\n";
      else if (type_koopa == "")
        return "fun @" + ident + "_func(" + param_koopa + ")" + type_koopa + 
              " {\n%entry:\n" + koopa_result + "  ret\n}\n\n";
    }
  }
};

// Stmt 是 BaseAST
class StmtAST : public BaseAST {
 public:
  unique_ptr<BaseAST> l_val;
  unique_ptr<BaseAST> exp;
  unique_ptr<BaseAST> block;
  unique_ptr<BaseAST> loop_stmt;
  unique_ptr<BaseAST> true_stmt;
  unique_ptr<BaseAST> false_stmt;

  vector<unique_ptr<BaseAST> > exp_vec;

  string Koopa() override {
    if (type_code == 0) {
      string exp_result = exp->Koopa();
      string exp_temp = BaseAST::parseIR(exp_result);

      string val_result = l_val->Koopa();
      if (l_val->type_code == 1) {
        int curr_IR = IR_cnt;
        string koopa_result;
        int tab_level = BaseAST::searchTab(val_result);
        int sym_type;
        string arr_tab_cnt;
        if (tab_level == tab_stk.size()) {
          arr_tab_cnt = to_string(symbol_tab.cnt);
          sym_type = symbol_tab.tab[val_result].type;
        }
        else if (tab_level >= 0) {
          arr_tab_cnt = to_string(tab_stk[tab_level].cnt);
          sym_type = tab_stk[tab_level].tab[val_result].type;
        }
        else if (tab_level == GLOBAL_DATA) {
          arr_tab_cnt = "global";
          sym_type = global_symbol_tab.tab[val_result].type;
        }
        int curr_ptr_cnt = ptr_cnt;
        for (int i=0; i<exp_vec.size(); i++) {
          string exp_vec_result = exp_vec[i]->Koopa();
          string temp = BaseAST::parseIR(exp_vec_result);
          if (i == 0) {
            if (sym_type == PTR_SYM) {
              if (temp[0] == '=') {
                koopa_result += exp_vec_result + "  %ptr_" + to_string(ptr_cnt++) + " = load @" + val_result + "_" + arr_tab_cnt + "\n";
                int curr_ptr = ptr_cnt;
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getptr %ptr_" + to_string(curr_ptr - 1) + ", %" + to_string(IR_cnt) + "\n";
              }
              else {
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = load @" + val_result + "_" + arr_tab_cnt + "\n";
                int curr_ptr = ptr_cnt;
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getptr %ptr_" + to_string(curr_ptr - 1) + ", " + exp_vec_result + "\n";
              }
            }
            else {
              if (temp[0] == '=')
                koopa_result += exp_vec_result + "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr @" + val_result + "_" + arr_tab_cnt + ", %" + to_string(IR_cnt) + "\n";
              else
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr @" + val_result + "_" + arr_tab_cnt + ", " + exp_vec_result + "\n";
            }
          }
          else {
            if (temp[0] == '=')
                koopa_result += exp_vec_result + "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr %ptr_" + to_string(curr_ptr_cnt - 1) + ", %" + to_string(IR_cnt) + "\n";
              else
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr %ptr_" + to_string(curr_ptr_cnt - 1) + ", " + exp_vec_result + "\n";
          }
          curr_ptr_cnt = ptr_cnt;
        }
        if (exp_temp[0] == '=')
          return exp_result + koopa_result + "  store %" + to_string(curr_IR) + ", %ptr_" + to_string(ptr_cnt - 1) + "\n";
        else
          return koopa_result + "  store " + exp_result + ", %ptr_" + to_string(ptr_cnt - 1) + "\n";
      }
      else {
        int tab_level = BaseAST::searchTab(val_result);
        if (exp_temp[0] == '=') { 
          if (tab_level == tab_stk.size()) {
            return exp_result + "  store %" + to_string(IR_cnt) + ", @" + val_result + "_" + to_string(symbol_tab.cnt) + "\n";
          }
          else if (tab_level >= 0) {
            return exp_result + "  store %" + to_string(IR_cnt) + ", @" + val_result + "_" + to_string(tab_stk[tab_level].cnt) + "\n";
          }
          else if (tab_level == GLOBAL_DATA) {
            return exp_result + "  store %" + to_string(IR_cnt) + ", @" + val_result + "_global" + "\n";
          }
        }
        else {
          if (tab_level == tab_stk.size()) {
            return "  store " + exp_result + ", @" + val_result + "_" + to_string(symbol_tab.cnt) + "\n";
          }
          else if (tab_level >= 0) {
            return "  store " + exp_result + ", @" + val_result + "_" + to_string(tab_stk[tab_level].cnt) + "\n";
          }
          else if (tab_level == GLOBAL_DATA) {
            return "  store " + exp_result + ", @" + val_result + "_global" + "\n";
          }
        }
      }
    }
    else if (type_code == 1) {
      if (exp != nullptr) {
        string koopa_result = exp->Koopa();
        string temp = BaseAST::parseIR(koopa_result);
        if (temp[0] == '=' || temp[0] == '@') {
          return koopa_result;
        }
      }
      return "";
    }
    else if (type_code == 2) {
      string koopa_result = block->Koopa();
      if (block->returned)
        this->returned = true;
      return koopa_result;
    }
    else if (type_code == 3) {
      this->returned = true;
      if (exp != nullptr) {
        string koopa_result = exp->Koopa();
        string temp = BaseAST::parseIR(koopa_result);
        if (temp[0] == '=') { 
          return koopa_result + "  ret %" + to_string(IR_cnt) + "\n";
        }
        else {
          return "  ret " + koopa_result + "\n";
        }
      }
      else
        return "  ret\n";
    }
    else if (type_code == 4) {
      string koopa_result = exp->Koopa();
      string temp = BaseAST::parseIR(koopa_result);
      int curr_label = if_label_cnt++;

      string br_koopa;
      if (temp[0] == '=') {
        br_koopa = "  br %" + to_string(IR_cnt) + 
        ", %if_then_" + to_string(curr_label) + 
        ", %if_else_" + to_string(curr_label) + "\n";
      }
      else {
        br_koopa = "  br " + koopa_result + 
        ", %if_then_" + to_string(curr_label) + 
        ", %if_else_" + to_string(curr_label) + "\n";

        koopa_result = "";
      }

      string true_koopa = true_stmt->Koopa();

      string then_koopa = "%if_then_" + to_string(curr_label) + ":\n" + 
                          true_koopa;
      
      string jump_koopa = "  jump %if_end_" + to_string(curr_label) + "\n";
      
      string false_koopa;
      if (false_stmt != nullptr)
        false_koopa = false_stmt->Koopa();
      else 
        false_koopa = "";
      
      string else_koopa = "%if_else_" + to_string(curr_label) + ":\n" +
                          false_koopa;
      
      string end_koopa = "%if_end_" + to_string(curr_label) + ":\n";

      bool true_returned = true_stmt->returned;
      bool false_returned;
      if (false_stmt != nullptr) {
        false_returned = false_stmt->returned;
        if (true_returned && !false_returned) {
          return koopa_result + br_koopa + then_koopa + 
              else_koopa + jump_koopa + end_koopa;
        }
        else if (!true_returned && false_returned) {
          return koopa_result + br_koopa + then_koopa + jump_koopa + 
              else_koopa + end_koopa;
        }
        else if (!true_returned && !false_returned) {
          return koopa_result + br_koopa + then_koopa + jump_koopa + 
              else_koopa + jump_koopa + end_koopa;
        }
        else if (true_returned && false_returned) {
          this->returned = true;
          return koopa_result + br_koopa + then_koopa + 
                else_koopa;
        }
      }
      else {
        if (true_returned) {
          return koopa_result + br_koopa + then_koopa + 
              else_koopa + jump_koopa + end_koopa;
        }
        else if (!true_returned) {
          return koopa_result + br_koopa + then_koopa + jump_koopa + 
              else_koopa + jump_koopa + end_koopa;
        }
      }
    }
    else if (type_code == 5) {
      int local_curr_loop = loop_cnt++;

      string jump_cond = "  jump %while_cond_" + to_string(local_curr_loop) + "\n";
      string while_label = "%while_cond_" + to_string(local_curr_loop) + ":\n";

      string koopa_result = exp->Koopa();
      string temp = BaseAST::parseIR(koopa_result);
      
      string br_koopa;
      if (temp[0] == '=') {
        br_koopa = "  br %" + to_string(IR_cnt) + 
        ", %while_loop_" + to_string(local_curr_loop) + 
        ", %while_end_" +to_string(local_curr_loop) + "\n";
      }
      else {
        br_koopa = "  br " + koopa_result + 
        ", %while_loop_" + to_string(local_curr_loop) + 
        ", %while_end_" +to_string(local_curr_loop) + "\n";

        koopa_result = "";
      }

      loop_stk.push_back(local_curr_loop);
      string stmt_koopa = loop_stmt->Koopa();
      loop_stk.pop_back();

      string loop_koopa = "%while_loop_" + to_string(local_curr_loop) + ":\n" + stmt_koopa;

      string end_label = "%while_end_" + to_string(local_curr_loop) + ":\n";

      if (loop_stmt->returned) 
        return jump_cond + while_label + koopa_result + br_koopa + loop_koopa + end_label;
      else
        return jump_cond + while_label + koopa_result + br_koopa + loop_koopa + jump_cond + end_label;
    
    }
    else if (type_code == 6) {
      this->returned = true;
      int curr_loop = loop_stk[loop_stk.size() - 1];
      return "  jump %while_end_" + to_string(curr_loop) + "\n";
    }
    else if (type_code == 7) {
      this->returned = true;
      int curr_loop = loop_stk[loop_stk.size() - 1];
      return "  jump %while_cond_" + to_string(curr_loop) + "\n";
    }
  }
};

// BlockItem 是 BaseAST
class BlockItemAST : public BaseAST {
 public:
  unique_ptr<BaseAST> decl;
  unique_ptr<BaseAST> stmt;

  string Koopa() override {
    if (type_code == 0)
      return decl->Koopa();
    else if (type_code == 1) {
      string koopa_result = stmt->Koopa();
      if (stmt->returned)
        this->returned = true;
      return koopa_result;
    }
  }
};

// Block 是 BaseAST
class BlockAST : public BaseAST {
 public:
  vector<unique_ptr<BaseAST> > block_item_vec;

  string Koopa() override {
    tab_stk.push_back(symbol_tab);
    symbol_tab.newTable();

    string block_item_koopa;
    for (int i=0; i<block_item_vec.size(); i++) {
      string koopa_result = block_item_vec[i]->Koopa();
      block_item_koopa += koopa_result;
      if (block_item_vec[i]->returned) {
        this->returned = true;
        break;
      }
    }
    
    symbol_tab.cnt = tab_stk[tab_stk.size() - 1].cnt;
    symbol_tab.tab = tab_stk[tab_stk.size() - 1].tab;
    tab_stk.pop_back();
    return block_item_koopa;
  }
};

// Exp 是 BaseAST
class ExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> lor_exp;

  string Koopa() override {
    return lor_exp->Koopa();
  }
};

// PrimaryExp 是 BaseAST
class PrimaryExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;
  int number;
  unique_ptr<BaseAST> l_val;
  vector<unique_ptr<BaseAST> > exp_vec;

  string Koopa() override {
    if (type_code == 0)
      return exp->Koopa();
    else if (type_code == 1)
      return to_string(number);
    else if (type_code == 2) {
      string val_result = l_val->Koopa();
      if (l_val->type_code == 1) {
        string koopa_result;
        int tab_level = BaseAST::searchTab(val_result);
        int sym_type,  dim_size;
        string arr_tab_cnt;
        if (tab_level == tab_stk.size()) {
          arr_tab_cnt = to_string(symbol_tab.cnt);
          sym_type = symbol_tab.tab[val_result].type;
          dim_size = symbol_tab.tab[val_result].dim.size();
        }
        else if (tab_level >= 0) {
          arr_tab_cnt = to_string(tab_stk[tab_level].cnt);
          sym_type = tab_stk[tab_level].tab[val_result].type;
          dim_size = tab_stk[tab_level].tab[val_result].dim.size();
        }
        else if (tab_level == GLOBAL_DATA) {
          arr_tab_cnt = "global";
          sym_type = global_symbol_tab.tab[val_result].type;
          dim_size = global_symbol_tab.tab[val_result].dim.size();
        }

        int curr_ptr_cnt = ptr_cnt;
        for (int i=0; i<exp_vec.size(); i++) {
          string exp_result = exp_vec[i]->Koopa();
          string temp = BaseAST::parseIR(exp_result);
          if (i == 0) {
            if (sym_type == PTR_SYM) {
              if (temp[0] == '=') {
                koopa_result += exp_result + "  %ptr_" + to_string(ptr_cnt++) + " = load @" + val_result + "_" + arr_tab_cnt + "\n";
                int curr_ptr = ptr_cnt;
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getptr %ptr_" + to_string(curr_ptr - 1) + ", %" + to_string(IR_cnt) + "\n";
              }
              else {
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = load @" + val_result + "_" + arr_tab_cnt + "\n";
                int curr_ptr = ptr_cnt;
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getptr %ptr_" + to_string(curr_ptr - 1) + ", " + exp_result + "\n";
              }
            }
            else {
              if (temp[0] == '=')
                koopa_result += exp_result + "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr @" + val_result + "_" + arr_tab_cnt + ", %" + to_string(IR_cnt) + "\n";
              else
                koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr @" + val_result + "_" + arr_tab_cnt + ", " + exp_result + "\n";
            }
          }
          else {
            if (temp[0] == '=')
              koopa_result += exp_result + "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr %ptr_" + to_string(curr_ptr_cnt - 1) + ", %" + to_string(IR_cnt) + "\n";
            else
              koopa_result += "  %ptr_" + to_string(ptr_cnt++) + " = getelemptr %ptr_" + to_string(curr_ptr_cnt - 1) + ", " + exp_result + "\n";
          }
          curr_ptr_cnt = ptr_cnt;
        }
        if (exp_vec.size() == dim_size)
          return koopa_result + "  %" + to_string(++IR_cnt) + " = load %ptr_" + to_string(ptr_cnt - 1) + "\n";
        else 
          return koopa_result + "  %" + to_string(++IR_cnt) + " = getelemptr %ptr_" + to_string(ptr_cnt - 1) + ", 0\n";
      }
      else {
        int tab_level = BaseAST::searchTab(val_result);
        if (tab_level == tab_stk.size()) {
          if (symbol_tab.tab[val_result].type == CONST_SYM)
            return symbol_tab.tab[val_result].val;
          else if (symbol_tab.tab[val_result].type == VAR_SYM)
            return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_" + to_string(symbol_tab.cnt) + "\n";
          else if (symbol_tab.tab[val_result].type == ARR_SYM)
            return "  %" + to_string(++IR_cnt) + " = getelemptr @" + val_result + "_" + to_string(symbol_tab.cnt) + ", 0\n";
          else if (symbol_tab.tab[val_result].type == PTR_SYM)
            return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_" + to_string(symbol_tab.cnt) + "\n";
        
        }
        else if (tab_level >= 0) {
          if (tab_stk[tab_level].tab[val_result].type == CONST_SYM)
            return tab_stk[tab_level].tab[val_result].val;
          else if (tab_stk[tab_level].tab[val_result].type == VAR_SYM)
            return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_" + to_string(tab_stk[tab_level].cnt) + "\n";
          else if (tab_stk[tab_level].tab[val_result].type == ARR_SYM)
            return "  %" + to_string(++IR_cnt) + " = getelemptr @" + val_result + "_" + to_string(tab_stk[tab_level].cnt) + ", 0\n";
          else if (tab_stk[tab_level].tab[val_result].type == PTR_SYM)
            return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_" + to_string(tab_stk[tab_level].cnt) + "\n";
        }
        else if (tab_level == GLOBAL_DATA) {
          if (global_symbol_tab.tab[val_result].type == CONST_SYM)
            return global_symbol_tab.tab[val_result].val;
          else if (global_symbol_tab.tab[val_result].type == VAR_SYM)
            return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_global" + "\n";
          else if (global_symbol_tab.tab[val_result].type == ARR_SYM)
            return "  %" + to_string(++IR_cnt) + " = getelemptr @" + val_result + "_global, 0\n";
          else if (global_symbol_tab.tab[val_result].type == PTR_SYM)
            return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_global\n";
        
        }
      }
    }
  }
};

// UnaryExp 是 BaseAST
class UnaryExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> primary_exp, unary_op, unary_exp;
  string ident;
  vector<unique_ptr<BaseAST> > exp_vec;

  string Koopa() override {
    if (type_code == 0)
      return primary_exp->Koopa();
    else if (type_code == 1) {
      string op = unary_op->Koopa();
      if (op == "+") {
        return unary_exp->Koopa();
      }
      else if (op == "-") {
        string koopa_result = unary_exp->Koopa();
        string temp = BaseAST::parseIR(koopa_result);
        if (temp[0] == '=') {  
          return koopa_result + "  %" + to_string(++IR_cnt) + " = sub 0, %" + to_string(IR_cnt-1) + "\n";
        }
        else {
          return "  %" + to_string(++IR_cnt) + " = sub 0, " + koopa_result + "\n";
        }
      }
      else if (op == "!") {
        string koopa_result = unary_exp->Koopa();
        string temp = BaseAST::parseIR(koopa_result);
        if (temp[0] == '=') {
          return koopa_result + "  %" + to_string(++IR_cnt) + " = eq 0, %" + to_string(IR_cnt-1) + "\n";
        }
        else {
          return "  %" + to_string(++IR_cnt) + " = eq 0, " + koopa_result + "\n";
        }
      }
    } 
    else if (type_code == 2) {
      string koopa_result;
      string param_result;
      if (exp_vec.size() != 0) {
        string exp_result = exp_vec[0]->Koopa();
        string temp = BaseAST::parseIR(exp_result);
        if (temp[0] == '=') {
          param_result += "%" + to_string(IR_cnt);
          koopa_result += exp_result;
        }
        else {
          param_result += exp_result;
        }

        for (int i=1; i<exp_vec.size(); i++) {
          string exp_result = exp_vec[i]->Koopa();
          string temp = BaseAST::parseIR(exp_result);
          if (temp[0] == '=') {
            param_result += ", %" + to_string(IR_cnt);
            koopa_result += exp_result;
          }
          else {
            param_result += ", " + exp_result;
          }
        }
      }
      
      if (global_func_tab.tab[ident].val == "") {
        if (lib_func.count(ident))
          return koopa_result + "  call @" + ident + "(" + param_result +")\n";
        else
          return koopa_result + "  call @" + ident + "_func(" + param_result +")\n";
      }
      else if (global_func_tab.tab[ident].val == ": i32") {
        if (lib_func.count(ident))
          return koopa_result + "  %" + to_string(++IR_cnt) + " = call @" + ident + "(" + param_result +")\n";
        else
          return koopa_result + "  %" + to_string(++IR_cnt) + " = call @" + ident + "_func(" + param_result +")\n";
      }
    }
    return "";
  }
};

// FuncRParams 是 BaseAST
class FuncRParamsAST : public BaseAST {
 public:
  vector<unique_ptr<BaseAST> > exp_vec;

  string Koopa() override {
    // redundant
    return "";
  }

};

// UnaryOp 是 BaseAST
class UnaryOpAST : public BaseAST {
 public:
  string op;

  string Koopa() override {
    return op;
  }
};

// AddExp 是 BaseAST
class AddExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> mul_exp;
  unique_ptr<BaseAST> add_exp;

  string Koopa() override {
    if (type_code == 0)
      return mul_exp->Koopa();
    else if (type_code == 1) {
      if (op == "+") {
        string add_result = add_exp->Koopa();
        string temp = BaseAST::parseIR(add_result);
        if (temp[0] == '=') {  
          string add_cnt = to_string(IR_cnt);
          string mul_result = mul_exp->Koopa();
          string temp = BaseAST::parseIR(mul_result);
          if (temp[0] == '=') {  
            string mul_cnt = to_string(IR_cnt);
            return add_result + mul_result + "  %" + to_string(++IR_cnt) 
                   + " = add %" + add_cnt + ", %" + mul_cnt + "\n";
          }
          else {
            return add_result + "  %" + to_string(++IR_cnt) 
                   + " = add %" + add_cnt + ", " + mul_result + "\n";
          }
        }
        else {
          string mul_result = mul_exp->Koopa();
          string temp = BaseAST::parseIR(mul_result);
          if (temp[0] == '=') {  
            string mul_cnt = to_string(IR_cnt);
            return mul_result + "  %" + to_string(++IR_cnt) + " = add "+ add_result + ", %" + mul_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = add "+ add_result + ", " + mul_result + "\n";
          }
        }
      }
      else if (op == "-") {
        string add_result = add_exp->Koopa();
        string temp = BaseAST::parseIR(add_result);
        if (temp[0] == '=') {  
          string add_cnt = to_string(IR_cnt);
          string mul_result = mul_exp->Koopa();
          string temp = BaseAST::parseIR(mul_result);
          if (temp[0] == '=') {  
            string mul_cnt = to_string(IR_cnt);
            return add_result + mul_result + "  %" + to_string(++IR_cnt) 
                   + " = sub %" + add_cnt + ", %" + mul_cnt + "\n";
          }
          else {
            return add_result + "  %" + to_string(++IR_cnt) 
                   + " = sub %" + add_cnt + ", " + mul_result + "\n";
          }
        }
        else {
          string mul_result = mul_exp->Koopa();
          string temp = BaseAST::parseIR(mul_result);
          if (temp[0] == '=') {  
            string mul_cnt = to_string(IR_cnt);
            return mul_result + "  %" + to_string(++IR_cnt) + " = sub "+ add_result + ", %" + mul_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = sub "+ add_result + ", " + mul_result + "\n";
          }
        }
      }
    }
    else 
      return "";
  }
};

// MulExp 是 BaseAST
class MulExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> unary_exp;
  unique_ptr<BaseAST> mul_exp;

  string Koopa() override {
    if (type_code == 0)
      return unary_exp->Koopa();
    else if (type_code == 1) {
      if (op == "*") {
        string mul_result = mul_exp->Koopa();
        string temp = BaseAST::parseIR(mul_result);
        if (temp[0] == '=') {  
          string mul_cnt = to_string(IR_cnt);
          string unary_result = unary_exp->Koopa();
          string temp = BaseAST::parseIR(unary_result);
          if (temp[0] == '=') {
            string unary_cnt = to_string(IR_cnt);
            return mul_result + unary_result + "  %" + to_string(++IR_cnt) 
                   + " = mul %" + mul_cnt + ", %" + unary_cnt + "\n";
          }
          else {
            return mul_result + "  %" + to_string(++IR_cnt) 
                   + " = mul %" + mul_cnt + ", " + unary_result + "\n";
          }
        }
        else {
          string unary_result = unary_exp->Koopa();
          string temp = BaseAST::parseIR(unary_result);
          if (temp[0] == '=') {
            string unary_cnt = to_string(IR_cnt);
            return unary_result + "  %" + to_string(++IR_cnt) + " = mul "+ mul_result + ", %" + unary_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = mul "+ mul_result + ", " + unary_result + "\n";
          }
        }
      }
      else if (op == "/") {
        string mul_result = mul_exp->Koopa();
        string temp = BaseAST::parseIR(mul_result);
        if (temp[0] == '=') {  
          string mul_cnt = to_string(IR_cnt);
          string unary_result = unary_exp->Koopa();
          string temp = BaseAST::parseIR(unary_result);
          if (temp[0] == '=') {
            string unary_cnt = to_string(IR_cnt);
            return mul_result + unary_result + "  %" + to_string(++IR_cnt) 
                   + " = div %" + mul_cnt + ", %" + unary_cnt + "\n";
          }
          else {
            return mul_result + "  %" + to_string(++IR_cnt) 
                   + " = div %" + mul_cnt + ", " + unary_result + "\n";
          }
        }
        else {
          string unary_result = unary_exp->Koopa();
          string temp = BaseAST::parseIR(unary_result);
          if (temp[0] == '=') {
            string unary_cnt = to_string(IR_cnt);
            return unary_result + "  %" + to_string(++IR_cnt) + " = div "+ mul_result + ", %" + unary_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = div "+ mul_result + ", " + unary_result + "\n";
          }
        }
      }
      else if (op == "%") {
        string mul_result = mul_exp->Koopa();
        string temp = BaseAST::parseIR(mul_result);
        if (temp[0] == '=') {  
          string mul_cnt = to_string(IR_cnt);
          string unary_result = unary_exp->Koopa();
          string temp = BaseAST::parseIR(unary_result);
          if (temp[0] == '=') {
            string unary_cnt = to_string(IR_cnt);
            return mul_result + unary_result + "  %" + to_string(++IR_cnt) 
                   + " = mod %" + mul_cnt + ", %" + unary_cnt + "\n";
          }
          else {
            return mul_result + "  %" + to_string(++IR_cnt) 
                   + " = mod %" + mul_cnt + ", " + unary_result + "\n";
          }
        }
        else {
          string unary_result = unary_exp->Koopa();
          string temp = BaseAST::parseIR(unary_result);
          if (temp[0] == '=') {
            string unary_cnt = to_string(IR_cnt);
            return unary_result + "  %" + to_string(++IR_cnt) + " = mod "+ mul_result + ", %" + unary_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = mod "+ mul_result + ", " + unary_result + "\n";
          }
        }
      }
    }
    else 
      return "";
  }
};

// LOrExp 是 BaseAST
class LOrExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> land_exp;
  unique_ptr<BaseAST> lor_exp;

  string Koopa() override {
    if (type_code == 0)
      return land_exp->Koopa();
    else if (type_code == 1) {
      int curr_label = short_circuit_cnt++;
      string lor_result = lor_exp->Koopa();
      string temp = BaseAST::parseIR(lor_result);
      if (temp[0] == '=') {  
        string lor_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, %" + to_string(IR_cnt) + "\n";
        string lor_cnt = to_string(++IR_cnt);
        string br_koopa = "  br %" + lor_cnt + ", %or_true_" + to_string(curr_label) + 
                                               ", %or_false_" + to_string(curr_label) + "\n";
        string true_koopa = "%or_true_" + to_string(curr_label) + ":\n  jump %or_end_" + to_string(curr_label) + "(%" + lor_cnt + ")\n";

        string land_result = land_exp->Koopa();
        string temp = BaseAST::parseIR(land_result);
        if (temp[0] == '=') {
          string land_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, %" + to_string(IR_cnt) + "\n";
          string land_cnt = to_string(++IR_cnt);
          string false_koopa = "%or_false_" + to_string(curr_label) + ":\n" + land_result + land_ne +
                               "  jump %or_end_" + to_string(curr_label) + "(%" + land_cnt + ")\n";

          string end_koopa = "%or_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return lor_result + lor_ne + br_koopa + true_koopa + false_koopa + end_koopa;
        }
        else {
          string land_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, " + land_result + "\n";
          string land_cnt = to_string(++IR_cnt);
          string false_koopa = "%or_false_" + to_string(curr_label) + ":\n" + land_ne +
                               "  jump %or_end_" + to_string(curr_label) + "(%" + land_cnt + ")\n";

          string end_koopa = "%or_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return lor_result + lor_ne + br_koopa + true_koopa + false_koopa + end_koopa;
        }
      }
      else {
        string lor_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, " + lor_result + "\n";
        string lor_cnt = to_string(++IR_cnt);
        string br_koopa = "  br %" + lor_cnt + ", %or_true_" + to_string(curr_label) + 
                                               ", %or_false_" + to_string(curr_label) + "\n";
        string true_koopa = "%or_true_" + to_string(curr_label) + ":\n  jump %or_end_" + to_string(curr_label) + "(%" + lor_cnt + ")\n";

        string land_result = land_exp->Koopa();
        string temp = BaseAST::parseIR(land_result);
        if (temp[0] == '=') {
          string land_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, %" + to_string(IR_cnt) + "\n";
          string land_cnt = to_string(++IR_cnt);
          string false_koopa = "%or_false_" + to_string(curr_label) + ":\n" + land_result + land_ne +
                               "  jump %or_end_" + to_string(curr_label) + "(%" + land_cnt + ")\n";

          string end_koopa = "%or_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return lor_ne + br_koopa + true_koopa + false_koopa + end_koopa;
        }
        else {
          string land_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, " + land_result + "\n";
          string land_cnt = to_string(++IR_cnt);
          string false_koopa = "%or_false_" + to_string(curr_label) + ":\n" + land_ne +
                               "  jump %or_end_" + to_string(curr_label) + "(%" + land_cnt + ")\n";

          string end_koopa = "%or_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return lor_ne + br_koopa + true_koopa + false_koopa + end_koopa;
        }
      }
    }
  }
};

// LAndExp 是 BaseAST
class LAndExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> eq_exp;
  unique_ptr<BaseAST> land_exp;

  string Koopa() override {
    if (type_code == 0)
      return eq_exp->Koopa();
    else if (type_code == 1) {
      int curr_label = short_circuit_cnt++;
      string land_result = land_exp->Koopa();
      string temp = BaseAST::parseIR(land_result);
      if (temp[0] == '=') {  
        string land_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, %" + to_string(IR_cnt) + "\n";
        string land_cnt = to_string(++IR_cnt);
        string br_koopa = "  br %" + land_cnt + ", %and_true_" + to_string(curr_label) + 
                                                ", %and_false_" + to_string(curr_label) + "\n";
        string false_koopa = "%and_false_" + to_string(curr_label) + ":\n  jump %and_end_" + to_string(curr_label) + "(%" + land_cnt + ")\n";

        string eq_result = eq_exp->Koopa();
        string temp = BaseAST::parseIR(eq_result);
        if (temp[0] == '=') {
          string eq_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, %" + to_string(IR_cnt) + "\n";
          string eq_cnt = to_string(++IR_cnt);
          string true_koopa = "%and_true_" + to_string(curr_label) + ":\n" + eq_result + eq_ne +
                               "  jump %and_end_" + to_string(curr_label) + "(%" + eq_cnt + ")\n";

          string end_koopa = "%and_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return land_result + land_ne + br_koopa + false_koopa + true_koopa + end_koopa;
        }
        else {
          string eq_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, " + eq_result + "\n";
          string eq_cnt = to_string(++IR_cnt);
          string true_koopa = "%and_true_" + to_string(curr_label) + ":\n" + eq_ne +
                               "  jump %and_end_" + to_string(curr_label) + "(%" + eq_cnt + ")\n";

          string end_koopa = "%and_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return land_result + land_ne + br_koopa + false_koopa + true_koopa + end_koopa;
        }
      }
      else {  
        string land_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, " + land_result + "\n";
        string land_cnt = to_string(++IR_cnt);
        string br_koopa = "  br %" + land_cnt + ", %and_true_" + to_string(curr_label) + 
                                                ", %and_false_" + to_string(curr_label) + "\n";
        string false_koopa = "%and_false_" + to_string(curr_label) + ":\n  jump %and_end_" + to_string(curr_label) + "(%" + land_cnt + ")\n";

        string eq_result = eq_exp->Koopa();
        string temp = BaseAST::parseIR(eq_result);
        if (temp[0] == '=') {
          string eq_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, %" + to_string(IR_cnt) + "\n";
          string eq_cnt = to_string(++IR_cnt);
          string true_koopa = "%and_true_" + to_string(curr_label) + ":\n" + eq_result + eq_ne +
                               "  jump %and_end_" + to_string(curr_label) + "(%" + eq_cnt + ")\n";

          string end_koopa = "%and_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return land_ne + br_koopa + false_koopa + true_koopa + end_koopa;
        }
        else {
          string eq_ne = "  %" + to_string(IR_cnt + 1) + " = ne 0, " + eq_result + "\n";
          string eq_cnt = to_string(++IR_cnt);
          string true_koopa = "%and_true_" + to_string(curr_label) + ":\n" + eq_ne +
                               "  jump %and_end_" + to_string(curr_label) + "(%" + eq_cnt + ")\n";

          string end_koopa = "%and_end_" + to_string(curr_label) + "(%" + to_string(++IR_cnt) + ": i32):\n";
          return land_ne + br_koopa + false_koopa + true_koopa + end_koopa;
        }
      }
    }
  }
};

// EqExp 是 BaseAST
class EqExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> rel_exp;
  unique_ptr<BaseAST> eq_exp;

  string Koopa() override {
    if (type_code == 0)
      return rel_exp->Koopa();
    else if (type_code == 1) {
      if (op == "==") {
        string eq_result = eq_exp->Koopa();
        string temp = BaseAST::parseIR(eq_result);
        if (temp[0] == '=') {  
          string eq_cnt = to_string(IR_cnt);
          string rel_result = rel_exp->Koopa();
          string temp = BaseAST::parseIR(rel_result);
        if (temp[0] == '=') {
            string rel_cnt = to_string(IR_cnt);
            return eq_result + rel_result + "  %" + to_string(++IR_cnt) 
                   + " = eq %" + eq_cnt + ", %" + rel_cnt + "\n";
          }
          else {
            return eq_result + "  %" + to_string(++IR_cnt) 
                   + " = eq %" + eq_cnt + ", " + rel_result + "\n";
          }
        }
        else {
          string rel_result = rel_exp->Koopa();
          string temp = BaseAST::parseIR(rel_result);
          if (temp[0] == '=') {
            string rel_cnt = to_string(IR_cnt);
            return rel_result + "  %" + to_string(++IR_cnt) + " = eq "+ eq_result + ", %" + rel_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = eq "+ eq_result + ", " + rel_result + "\n";
          }
        }
      }
      else if (op == "!=") {
        string eq_result = eq_exp->Koopa();
        string temp = BaseAST::parseIR(eq_result);
        if (temp[0] == '=') {  
          string eq_cnt = to_string(IR_cnt);
          string rel_result = rel_exp->Koopa();
          string temp = BaseAST::parseIR(rel_result);
          if (temp[0] == '=') {
            string rel_cnt = to_string(IR_cnt);
            return eq_result + rel_result + "  %" + to_string(++IR_cnt) 
                   + " = ne %" + eq_cnt + ", %" + rel_cnt + "\n";
          }
          else {
            return eq_result + "  %" + to_string(++IR_cnt) 
                   + " = ne %" + eq_cnt + ", " + rel_result + "\n";
          }
        }
        else {
          string rel_result = rel_exp->Koopa();
          string temp = BaseAST::parseIR(rel_result);
          if (temp[0] == '=') {
            string rel_cnt = to_string(IR_cnt);
            return rel_result + "  %" + to_string(++IR_cnt) + " = ne "+ eq_result + ", %" + rel_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = ne "+ eq_result + ", " + rel_result + "\n";
          }
        }
      }
    }
  }
};

// RelExp 是 BaseAST
class RelExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> add_exp;
  unique_ptr<BaseAST> rel_exp;

  string Koopa() override {
    if (type_code == 0)
      return add_exp->Koopa();
    else if (type_code == 1) {
      if (op == "<") {
        string rel_result = rel_exp->Koopa();
        string temp = BaseAST::parseIR(rel_result);
        if (temp[0] == '=') {  
          string rel_cnt = to_string(IR_cnt);
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return rel_result + add_result + "  %" + to_string(++IR_cnt) 
                   + " = lt %" + rel_cnt + ", %" + add_cnt + "\n";
          }
          else {
            return rel_result + "  %" + to_string(++IR_cnt) 
                   + " = lt %" + rel_cnt + ", " + add_result + "\n";
          }
        }
        else {
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return add_result + "  %" + to_string(++IR_cnt) + " = lt "+ rel_result + ", %" + add_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = lt "+ rel_result + ", " + add_result + "\n";
          }
        }
      }
      else if (op == ">") {
        string rel_result = rel_exp->Koopa();
        string temp = BaseAST::parseIR(rel_result);
        if (temp[0] == '=') {  
          string rel_cnt = to_string(IR_cnt);
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return rel_result + add_result + "  %" + to_string(++IR_cnt) 
                   + " = gt %" + rel_cnt + ", %" + add_cnt + "\n";
          }
          else {
            return rel_result + "  %" + to_string(++IR_cnt) 
                   + " = gt %" + rel_cnt + ", " + add_result + "\n";
          }
        }
        else {
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return add_result + "  %" + to_string(++IR_cnt) + " = gt "+ rel_result + ", %" + add_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = gt "+ rel_result + ", " + add_result + "\n";
          }
        }
      }
      else if (op == "<=") {
        string rel_result = rel_exp->Koopa();
        string temp = BaseAST::parseIR(rel_result);
        if (temp[0] == '=') {  
          string rel_cnt = to_string(IR_cnt);
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return rel_result + add_result + "  %" + to_string(++IR_cnt) 
                   + " = le %" + rel_cnt + ", %" + add_cnt + "\n";
          }
          else {
            return rel_result + "  %" + to_string(++IR_cnt) 
                   + " = le %" + rel_cnt + ", " + add_result + "\n";
          }
        }
        else {
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return add_result + "  %" + to_string(++IR_cnt) + " = le "+ rel_result + ", %" + add_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = le "+ rel_result + ", " + add_result + "\n";
          }
        }
      }
      else if (op == ">=") {
        string rel_result = rel_exp->Koopa();
        string temp = BaseAST::parseIR(rel_result);
        if (temp[0] == '=') {  
          string rel_cnt = to_string(IR_cnt);
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return rel_result + add_result + "  %" + to_string(++IR_cnt) 
                   + " = ge %" + rel_cnt + ", %" + add_cnt + "\n";
          }
          else {
            return rel_result + "  %" + to_string(++IR_cnt) 
                   + " = ge %" + rel_cnt + ", " + add_result + "\n";
          }
        }
        else {
          string add_result = add_exp->Koopa();
          string temp = BaseAST::parseIR(add_result);
          if (temp[0] == '=') {
            string add_cnt = to_string(IR_cnt);
            return add_result + "  %" + to_string(++IR_cnt) + " = ge "+ rel_result + ", %" + add_cnt + "\n";
          }
          else {
            return "  %" + to_string(++IR_cnt) + " = ge "+ rel_result + ", " + add_result + "\n";
          }
        }
      }
    }
  }
};

// Decl 是 BaseAST
class DeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> const_decl;
  unique_ptr<BaseAST> var_decl;

  string Koopa() override {
    if (type_code == 0)
      return const_decl->Koopa();
    else if (type_code == 1)
      return var_decl->Koopa();
    return "";
  }
};

// ConstDecl 是 BaseAST
class ConstDeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> type;
  vector<unique_ptr<BaseAST> > const_def_vec;

  string Koopa() override {
    string const_def_koopa;
    for (int i=0; i<const_def_vec.size(); i++) {
      const_def_koopa += const_def_vec[i]->Koopa();
    }
    return const_def_koopa;
  }
};

// ConstDef 是 BaseAST
class ConstDefAST : public BaseAST {
 public:
  string ident;
  unique_ptr<BaseAST> const_init_val;
  vector<unique_ptr<BaseAST> > const_exp_vec;

  string Koopa() override {
    if (type_code == 0) {
      int curr_cnt = IR_cnt;
      string koopa_result = const_init_val->Koopa();
      IR_cnt = curr_cnt;
      string temp = BaseAST::parseIR(koopa_result);
      if (temp[0] == '=') {
        if (!in_func)
          global_symbol_tab.tab[ident] = symbol(CONST_SYM, to_string(BaseAST::calcuKoopa(koopa_result)));
        else
          symbol_tab.tab[ident] = symbol(CONST_SYM, to_string(BaseAST::calcuKoopa(koopa_result)));
      }
      else {
        if (!in_func)
          global_symbol_tab.tab[ident] = symbol(CONST_SYM, koopa_result);
        else
          symbol_tab.tab[ident] = symbol(CONST_SYM, koopa_result);
      }

      return "";
    }
    else if (type_code == 1) {
      vector<int> dim;
      for (int i=0; i<const_exp_vec.size(); i++) {
        string exp_result = const_exp_vec[i]->Koopa();
        string temp = BaseAST::parseIR(exp_result);
        if (temp[0] == '=') 
          dim.push_back(BaseAST::calcuKoopa(exp_result));
        else 
          dim.push_back(stoi(exp_result));
      }

      string dim_result = "i32";
      for (int i=dim.size() - 1; i>=0; i--)
        dim_result = "[" + dim_result + ", "+ to_string(dim[i]) + "]";
      
      if (!in_func) {
        global_symbol_tab.tab[ident] = symbol(ARR_SYM, dim);

        int curr_cnt = IR_cnt;
        string init_result = const_init_val->Koopa();
        IR_cnt = curr_cnt;

        return "global @" + ident + "_global" + " = alloc " + dim_result + ", " + init_result + "\n\n";
      }
      else {
        symbol_tab.tab[ident] = symbol(ARR_SYM, dim);

        int curr_cnt = IR_cnt;
        string init_result = const_init_val->Koopa();
        IR_cnt = curr_cnt;

        return "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc " + dim_result + "\n" +
               "  store " + init_result + ", @" + ident + "_" + to_string(symbol_tab.cnt) + "\n";
      }
    }
  }
};

// ConstInitVal 是 BaseAST
class ConstInitValAST : public BaseAST {
 public:
  unique_ptr<BaseAST> const_exp;
  vector<unique_ptr<BaseAST> > const_init_vec;

  string ident;
  int level;

  string Koopa() override {
    if (type_code == 0)
      return const_exp->Koopa();
    else if (type_code == 1) {
      string koopa_result;
      vector<int> dim;
      int tab_level = BaseAST::searchTab(ident);
      if (tab_level == tab_stk.size())
        dim = symbol_tab.tab[ident].dim;
      else if (tab_level >= 0)
        dim = tab_stk[tab_level].tab[ident].dim;
      else if (tab_level == GLOBAL_DATA)
        dim = global_symbol_tab.tab[ident].dim;

      dim = vector<int>(dim.begin() + level, dim.end());
      vector<int> temp_dim = dim;
      int align = 0;

      stack<string> koopa_stk;
      for (int i=0; ; i++) {
        if (i<const_init_vec.size()) {
          if (dynamic_cast<ConstInitValAST*>(const_init_vec[i].get())->type_code == 0) {
            string const_init_result = const_init_vec[i]->Koopa();
            string temp = BaseAST::parseIR(const_init_result);
            if (temp[0] == '=')
              const_init_result = to_string(BaseAST::calcuKoopa(const_init_result));

            koopa_stk.push(const_init_result);
            temp_dim[temp_dim.size() - 1]--;

            align = temp_dim.size() - 1;
          }
          else {
            temp_dim[align]--;
            dynamic_cast<ConstInitValAST*>(const_init_vec[i].get())->ident = ident;
            dynamic_cast<ConstInitValAST*>(const_init_vec[i].get())->level = level + align + 1;
            koopa_stk.push(const_init_vec[i]->Koopa());
          }
        }
        else {
          koopa_stk.push("0");
          temp_dim[temp_dim.size() - 1]--;

          align = temp_dim.size() - 1;
        }
        
        for (int j=temp_dim.size() - 1; j>=0; j--) {
          if (temp_dim[j] == 0) {
            temp_dim[j] = dim[j];
            if (j != 0)
              temp_dim[j - 1]--;

            string temp_koopa;
            for (int k=0; k<dim[j]; k++) {
              temp_koopa = koopa_stk.top() + string(k==0?"":", ") + temp_koopa;
              koopa_stk.pop();
            }
            temp_koopa = "{" + temp_koopa + "}"; 
            koopa_stk.push(temp_koopa);   
            align = j - 1;
          }
        }
        if (align == -1)
          break;
      }
      koopa_result = koopa_stk.top();
      return koopa_result;
    }
  }
};

// ConstExp 是 BaseAST
class ConstExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;

  string Koopa() override {
    return exp->Koopa();
  }
};

// LVal 是 BaseAST
class LValAST : public BaseAST {
 public:
  string ident;
  vector<unique_ptr<BaseAST> > exp_vec;

  string Koopa() override {
    return ident;
  }
};

// VarDecl 是 BaseAST
class VarDeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> type;
  vector<unique_ptr<BaseAST> > var_def_vec;

  string Koopa() override {
    string koopa_result;
    for (int i=0; i<var_def_vec.size(); i++) {
      koopa_result += var_def_vec[i]->Koopa();
    }
    return koopa_result;
  }
};

// VarDef 是 BaseAST
class VarDefAST : public BaseAST {
 public:
  string ident;
  unique_ptr<BaseAST> init_val;
  vector<unique_ptr<BaseAST> > const_exp_vec;

  string Koopa() override {
    if (type_code == 0) {
      // 初始值为0
      if (!in_func) {
        global_symbol_tab.tab[ident] = symbol(VAR_SYM, "0");
        return "global @" + ident + "_global" + " = alloc i32, zeroinit\n\n";
      }
      else {
        symbol_tab.tab[ident] = symbol(VAR_SYM, "0");
        return "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc i32 \n";
      }
    }
    else if (type_code == 1) {
      int curr_cnt = IR_cnt;
      string koopa_result = init_val->Koopa();
      string temp = BaseAST::parseIR(koopa_result);
      if (temp[0] == '=') {
        if (!in_func) {
          int global_value = BaseAST::calcuKoopa(koopa_result);
          global_symbol_tab.tab[ident] = symbol(VAR_SYM, to_string(global_value));
          IR_cnt = curr_cnt;
          return "global @" + ident + "_global" + " = alloc i32, " + to_string(global_value) + "\n\n";
        }
        else {
          symbol_tab.tab[ident] = symbol(VAR_SYM, "%" + to_string(IR_cnt));
          return koopa_result + 
                "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc i32 \n" + 
                "  store %" + to_string(IR_cnt) + ", @" + ident + "_" + to_string(symbol_tab.cnt) + "\n";
        }
      }
      else {
        if (!in_func) {
          global_symbol_tab.tab[ident] = symbol(VAR_SYM, koopa_result);
          IR_cnt = curr_cnt;
          return "global @" + ident + "_global" + " = alloc i32, " + koopa_result + "\n\n";
        }
        else {
          symbol_tab.tab[ident] = symbol(VAR_SYM, koopa_result);
          return "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc i32 \n" + 
                "  store " + koopa_result + ", @" + ident + "_" + to_string(symbol_tab.cnt) + "\n";
        }
      }
    }
    else if (type_code == 2) {
      vector<int> dim;
      for (int i=0; i<const_exp_vec.size(); i++) {
        string exp_result = const_exp_vec[i]->Koopa();
        string temp = BaseAST::parseIR(exp_result);
        if (temp[0] == '=') 
          dim.push_back(BaseAST::calcuKoopa(exp_result));
        else 
          dim.push_back(stoi(exp_result));
      }

      string dim_result = "i32";
      for (int i=dim.size() - 1; i>=0; i--)
        dim_result = "[" + dim_result + ", "+ to_string(dim[i]) + "]";
      
      if (!in_func) {
        global_symbol_tab.tab[ident] = symbol(ARR_SYM, dim);
        return "global @" + ident + "_global" + " = alloc " + dim_result + ", zeroinit" + "\n\n";
      }
      else {
        symbol_tab.tab[ident] = symbol(ARR_SYM, dim);
        return "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc " + dim_result + "\n";
      }
    }
    else if (type_code == 3) {
      vector<int> dim;
      for (int i=0; i<const_exp_vec.size(); i++) {
        string exp_result = const_exp_vec[i]->Koopa();
        string temp = BaseAST::parseIR(exp_result);
        if (temp[0] == '=') 
          dim.push_back(BaseAST::calcuKoopa(exp_result));
        else 
          dim.push_back(stoi(exp_result));
      }

      string dim_result = "i32";
      for (int i=dim.size() - 1; i>=0; i--)
        dim_result = "[" + dim_result + ", "+ to_string(dim[i]) + "]";
      
      if (!in_func) {
        global_symbol_tab.tab[ident] = symbol(ARR_SYM, dim);
        
        int curr_cnt = IR_cnt;
        string init_result = init_val->Koopa();
        IR_cnt = curr_cnt;

        return "global @" + ident + "_global" + " = alloc " + dim_result + ", " + init_result + "\n\n";
      }
      else {
        symbol_tab.tab[ident] = symbol(ARR_SYM, dim);
        
        int curr_cnt = IR_cnt;
        string init_result = init_val->Koopa();
        IR_cnt = curr_cnt;

        return "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc " + dim_result + "\n" +
               "  store " + init_result + ", @" + ident + "_" + to_string(symbol_tab.cnt) + "\n";
      }
    }
  }
};

// InitVal 是 BaseAST
class InitValAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;
  vector<unique_ptr<BaseAST> > init_vec;

  string ident;
  int level;

  string Koopa() override {
    if (type_code == 0)
      return exp->Koopa();
    else if (type_code == 1) {
      string koopa_result;
      vector<int> dim;
      int tab_level = BaseAST::searchTab(ident);
      if (tab_level == tab_stk.size())
        dim = symbol_tab.tab[ident].dim;
      else if (tab_level >= 0)
        dim = tab_stk[tab_level].tab[ident].dim;
      else if (tab_level == GLOBAL_DATA)
        dim = global_symbol_tab.tab[ident].dim;

      dim = vector<int>(dim.begin() + level, dim.end());
      vector<int> temp_dim = dim;
      int align = 0;

      stack<string> koopa_stk;
      for (int i=0; ; i++) {
        if (i<init_vec.size()) {
          if (dynamic_cast<InitValAST*>(init_vec[i].get())->type_code == 0) {
            string init_result = init_vec[i]->Koopa();
            string temp = BaseAST::parseIR(init_result);
            if (temp[0] == '=')
              init_result = to_string(BaseAST::calcuKoopa(init_result));

            koopa_stk.push(init_result);
            temp_dim[temp_dim.size() - 1]--;

            align = temp_dim.size() - 1;
          }
          else {
            temp_dim[align]--;
            dynamic_cast<InitValAST*>(init_vec[i].get())->ident = ident;
            dynamic_cast<InitValAST*>(init_vec[i].get())->level = level + align + 1;
            koopa_stk.push(init_vec[i]->Koopa());
          }
        }
        else {
          koopa_stk.push("0");
          temp_dim[temp_dim.size() - 1]--;

          align = temp_dim.size() - 1;
        }
        
        for (int j=temp_dim.size() - 1; j>=0; j--) {
          if (temp_dim[j] == 0) {
            temp_dim[j] = dim[j];
            if (j != 0)
              temp_dim[j - 1]--;

            string temp_koopa;
            for (int k=0; k<dim[j]; k++) {
              temp_koopa = koopa_stk.top() + string(k==0?"":", ") + temp_koopa;
              koopa_stk.pop();
            }
            temp_koopa = "{" + temp_koopa + "}"; 
            koopa_stk.push(temp_koopa);   
            align = j - 1;
          }
        }
        if (align == -1)
          break;
      }
      koopa_result = koopa_stk.top();
      return koopa_result;
    }
  }
};
