#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>

enum symbol_type{
  CONST_INT,
  VAR_INT,
  FUNC_INT,
  FUNC_VOID,
  CONST_ARRAY,
  VAR_ARRAY,
  PTR
};

struct symbol_info{
  symbol_type type;
  std::optional<int> value;
  int times;//处理重名变量
};

/*template<>
struct std::hash<symbol_name>{
  std::size_t operator()(const symbol_name &sn) const{
    return std::hash<std::string>()(sn.ident);
  }
};*/

class SymbolTable{
  public:
    std::vector<std::unordered_map<std::string, symbol_info> > table;
    std::unordered_map<std::string, int> times_table;//记录同名变量出现次数

    SymbolTable(){
      table.emplace_back();
    }

    void enterScope() {
        table.emplace_back(); 
        //std::cout << "enter scope" << table.size()<<std::endl;
    }

    void exitScope() {
        if (table.size() > 1) {
            table.pop_back(); 
            //std::cout << "exit scope" << table.size()<<std::endl;
        } 
    }

    void insert(std::string ident, symbol_type type, std::optional<int> value) {
        if(times_table.find(ident)==times_table.end())times_table[ident]=0;
        int times=times_table[ident];
        times_table[ident]++;
        symbol_info si{type, value, times};
        table.back()[ident] = si; 
    }

    std::optional<symbol_info> find(std::string ident) {
        int i=table.size();
        while(i--){
          auto found=table[i].find(ident);
          if (found!=table[i].end())return found->second;
        }
        return std::nullopt;
    }

    std::optional<symbol_info> find(std::string ident,symbol_type type) {
      auto found=find(ident);
      if(found.has_value()&&found->type==type)return found;
      return std::nullopt;
    }

    

    void updateValue(std::string ident, std::optional<int> value) {
        int i=table.size();
        while(i--){
          auto found=table[i].find(ident);
          if (found!=table[i].end()){
            table[i][ident].value=value;
            return;
          }
        }
    }

    void print() {
        for (auto &scope : table) {
          std::cout<<"scope:"<<std::endl;
            for (auto &entry : scope) {
                std::cout << entry.first << " : " << entry.second.type << " : " << entry.second.value.value_or(-1) 
                <<":"<<entry.second.times<< std::endl;
            }
        }
    }

};


extern SymbolTable sym_table;


// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  //virtual void Dump() const = 0;
  virtual void Koopa() const = 0;
};

// CompUnit 是 BaseAST
//CompUnit  ::= FuncDef;
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > comp_units;

  //void Dump() const override;
  void Koopa() const override;
};

class CompUnitItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> func_def;\
  int case_;

  void Koopa() const override;
};

// FuncDef 也是 BaseAST
//FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > func_f_params;
  std::unique_ptr<BaseAST> block;

  //void Dump() const override;
  void Koopa() const override;
};

class BaseExpAST:public BaseAST{
public:
  //std::optional<bool> hasIdent=std::nullopt;
  std::optional<int> value;

  virtual bool isNumber() = 0;
  virtual int getValue() = 0;
  virtual bool loadIdent() = 0;
};

//FuncFParam  ::= BType IDENT;
class FuncFParamAST : public BaseAST {
 public:
  std::string ident;
  int case_;
  std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > index;

  void Koopa() const override;
};

//FuncType  ::= "int";
class TypeAST : public BaseAST {
 public:
  std::string type;

  //void Dump() const override;
  void Koopa() const override;
};

//Block         ::= "{" {BlockItem} "}";
class BlockAST:public BaseAST{
  public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > block_items;

    void Koopa() const override;
};

//BlockItem     ::= Decl | Stmt;
class BlockItemAST:public BaseAST{
  public:
    int case_;
    std::unique_ptr<BaseAST> decl_or_stmt;

    void Koopa() const override;
};

//LVal          ::= IDENT;
class LValAST:public BaseExpAST{
  public:
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > exp_index;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//Stmt          ::= LVal "=" Exp ";"
//              | "return" [Exp] ";";
//              | Block
//              | [Exp] ";"
class StmtAST:public BaseAST{
public:
  std::unique_ptr<BaseAST> stmt;
  int case_;

  void Koopa() const override;
};

/*open_statement: IF '(' expression ')' statement
              | IF '(' expression ')' closed_statement ELSE open_statement
              | WHILE '(' expression ')' open_statement
              ;*/
class OpenStmtAST:public BaseAST{
public:
  std::unique_ptr<BaseExpAST> exp;
  std::unique_ptr<BaseAST> stmt;
  std::unique_ptr<BaseAST> else_stmt;
  int case_;

  void Koopa() const override;
};

/*closed_statement:  IF '(' expression ')' closed_statement ELSE closed_statement
                | WHILE '(' expression ')' closed_statement
                ;*/
class ClosedStmtAST:public BaseAST{
public:
  std::unique_ptr<BaseExpAST> l_val;
  std::unique_ptr<BaseExpAST> exp;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<BaseAST> stmt;
  std::unique_ptr<BaseAST> else_stmt;
  int case_;

  void Koopa() const override;
};

//Exp         ::= LOrExp;
class ExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> l_or_exp;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//PrimaryExp    ::= "(" Exp ")" | LVal | Number;
class PrimaryExpAST:public BaseExpAST{
  public:
    int case_;
    std::unique_ptr<BaseExpAST> exp_or_l_val;
    int number;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST:public BaseExpAST{
  public:
    int case_; 
    std::unique_ptr<BaseExpAST> primary_exp_or_unary_exp;
    //UnaryOp     ::= "+" | "-" | "!";
    char unary_op;
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > func_r_params;

    bool isNumber() override;
    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
};

//MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class MulExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> unary_exp;
    std::unique_ptr<BaseExpAST> mul_exp;
    char mul_op;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
class AddExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> mul_exp;
    std::unique_ptr<BaseExpAST> add_exp;
    char add_op;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
class RelExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> add_exp;
    std::unique_ptr<BaseExpAST> rel_exp;
    std::string rel_op;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
class EqExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> rel_exp;
    std::unique_ptr<BaseExpAST> eq_exp;
    std::string eq_op;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> eq_exp;
    std::unique_ptr<BaseExpAST> l_and_exp;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> l_and_exp;
    std::unique_ptr<BaseExpAST> l_or_exp;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//Decl          ::= ConstDecl | VarDecl;
class DeclAST:public BaseAST{
  public:
    std::unique_ptr<BaseAST> const_decl_or_var_decl;
    int case_;

    void Koopa() const override;
};

//ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDeclAST:public BaseAST{
  public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_defs;

    void Koopa() const override;
};

//ConstDef      ::= IDENT "=" ConstInitVal;
class ConstDefAST:public BaseAST{
  public:
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > index;
    std::unique_ptr<BaseExpAST> const_init_val;

    void Koopa() const override;
};

//ConstInitVal  ::= ConstExp;
class ConstInitValAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> const_exp;
    std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > const_init_vals;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    std::vector<int> InitArray(std::vector<int> vec) const;
    bool isNumber() override;
};

//ConstExp      ::= Exp;
class ConstExpAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> exp;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    bool isNumber() override;
};

//VarDecl       ::= BType VarDef {"," VarDef} ";";
class VarDeclAST:public BaseAST{
  public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > var_defs;

    void Koopa() const override;
};
//VarDef        ::= IDENT | IDENT "=" InitVal;
class VarDefAST:public BaseAST{
  public:
    std::string ident;
    std::unique_ptr<BaseExpAST> init_val;
    std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > index;
    int case_;

    void Koopa() const override;
};
//InitVal       ::= Exp;
class InitValAST:public BaseExpAST{
  public:
    std::unique_ptr<BaseExpAST> exp;
    std::unique_ptr<std::vector<std::unique_ptr<BaseExpAST> > > init_vals;
    int case_;

    void Koopa() const override;
    int getValue() override;
    bool loadIdent() override;
    std::vector<std::pair<int,bool> > InitArray(std::vector<int> vec) const;
    bool isNumber() override;
};
