#pragma once
#include <memory> 
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <stack>

class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> global_defs;

  void Dump() const override {
    std::cout << "CompUnitAST { ";
    for (const auto &global_def : global_defs) {
      global_def->Dump();
      std::cout << ", ";
    }
    std::cout << " }";
  }
};

class GlobalDefAST: public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> func_def;

  enum class Type {
    DECL,
    FUNC_DEF
  };
  Type type;

  void Dump() const override {
    std::cout << "GlobalDefAST { ";
    if (type == Type::DECL) {
      decl->Dump();
    }
    else if (type == Type::FUNC_DEF) {
      func_def->Dump();
    }
    else {
      std::cout << "unknown";
    }
    std::cout << " }";
  }
};

class DeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_decl;
  std::unique_ptr<BaseAST> var_decl;

  enum class Type {
    CONST_DECL,
    VAR_DECL
  };
  Type type;

  void Dump() const override {
    std::cout << "DeclAST { ";
    const_decl->Dump();
    std::cout << " }";
  }
};

class ConstDeclAST : public BaseAST {
 public:
  std::string btype;
  std::vector<std::unique_ptr<BaseAST>> const_def;

  void Dump() const override {
    std::cout << "ConstDeclAST { ";
    std::cout << btype << ", ";
    for (const auto &const_def : const_def) {
      const_def->Dump();
      std::cout << ", ";
    }
    std::cout << " }";
  }
};

class ArrayIdentAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> array_ident;
  std::unique_ptr<BaseAST> const_exp;
  std::string type;

  void Dump() const override {
    std::cout << "ArrayIdentAST { " << ident << ", ";
    if (array_ident) {
      array_ident->Dump();
    }
    if (const_exp) {
      const_exp->Dump();
    }
    std::cout << " }";
  }
};

class BTypeAST : public BaseAST {
 public:
  std::string type;

  void Dump() const override {
    std::cout << "BTypeAST { " << type << " }";
  }
};

class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> array_ident;
  std::unique_ptr<BaseAST> const_init_val;

  enum class Type {
    CONST_INT,
    CONST_ARRAY
  };
  Type type;

  void Dump() const override {
    std::cout << "ConstDefAST { " << ident << ", ";
    const_init_val->Dump();
    std::cout << " }";
  }
};

class ConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_exp;
  std::vector<std::unique_ptr<BaseAST>> array_const_init_val;
  std::string ret_val;

  enum class Type {
    CONST_EXP,
    CONST_ARRAY
  };
  Type type;

  void Dump() const override {
    std::cout << "ConstInitValAST { ";
    const_exp->Dump();
    std::cout << " }";
  }
};

class VarDeclAST : public BaseAST {
 public:
  std::string btype;
  std::vector<std::unique_ptr<BaseAST>> var_def;

  void Dump() const override {
    std::cout << "VarDeclAST { ";
    std::cout << btype << ", ";
    for (const auto &var_def : var_def) {
      var_def->Dump();
      std::cout << ", ";
    }
    std::cout << " }";
  }
};

class VarDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> array_ident;
  std::unique_ptr<BaseAST> init_val;

  enum class Type {
    INIT,
    UNINIT,
    ARRAY_INIT,
    ARRAY_UNINIT
  };
  Type type;

  void Dump() const override {
    std::cout << "VarDefAST { " << ident << ", ";
    init_val->Dump();
    std::cout << " }";
  }
};

class InitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::vector<std::unique_ptr<BaseAST>> array_init_val;
  std::string ret_val;

  enum class Type {
    EXP,
    ARRAY
  };
  Type type;

  void Dump() const override {
    std::cout << "InitValAST { ";
    exp->Dump();
    std::cout << " }";
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::string func_type;
  std::string ident;
  std::unique_ptr<BaseAST> func_fparams;
  std::unique_ptr<BaseAST> block;

  void Dump() const override {
    std::cout << "FuncDefAST { ";
    std::cout << func_type;
    std::cout << ", " << ident << ", ";
    func_fparams->Dump();
    block->Dump();
    std::cout << " }";
  }
};

class FuncTypeAST : public BaseAST {
 public:
  std::string type;

  void Dump() const override {
    std::cout << "FuncTypeAST { " << type << " }";
  }
};

class FuncFParamsAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> fparams;

  void Dump() const override {
    std::cout << "FuncFParamsAST { ";
    for (const auto &fparam : fparams) {
      fparam->Dump();
      std::cout << ", ";
    }
    std::cout << " }";
  }
};

class FuncFParamTypeAST : public BaseAST {
 public:
  std::string type = "";
  std::unique_ptr<BaseAST> func_param_type;
  std::unique_ptr<BaseAST> const_exp;

  void Dump() const override {
    std::cout << "FuncFParamTypeAST { " << type << " }";
  }
};

class FuncFParamAST : public BaseAST {
 public:
  std::string btype;
  std::string ident;
  std::unique_ptr<BaseAST> func_param_type;
  std::string type;

  void Dump() const override {
    std::cout << "FuncFParamAST { ";
    std::cout << btype << ", " << ident << " }";
  }
};

class BlockAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> block_items;
  std::string ret_val;

  void Dump() const override {
    std::cout << "BlockAST { ";
    for (const auto &block_item : block_items) {
      block_item->Dump();
      std::cout << ", ";
    }
    std::cout << " }";
  }
};

class BlockItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> stmt;
  std::string ret_val;

  enum class Type {
    DECL,
    STMT
  };
  Type type;

  void Dump() const override {
    std::cout << "BlockItemAST { ";
    if (type == Type::DECL) {
      decl->Dump();
    }
    else if (type == Type::STMT) {
      stmt->Dump();
    }
    else {
      std::cout << "unknown";
    }
    std::cout << " }";
  }
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  std::unique_ptr<BaseAST> while_stmt;
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> block;
  std::string ret_val;

  enum class Type {
    RETURN,
    LVAL,
    BLOCK,
    EXP,
    IF,
    WHILE,
    BREAK,
    CONTINUE
  };
  Type type;

  void Dump() const override {
    std::cout << "StmtAST { ";
    exp->Dump();
    std::cout << " }";
  }
};

class IfStmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;
  std::string ret_val;

  void Dump() const override {
    std::cout << "IfStmtAST { ";
    stmt->Dump();
    std::cout << " }";
  }
};

class ElseStmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;
  std::string ret_val;

  void Dump() const override {
    std::cout << "ElseStmtAST { ";
    stmt->Dump();
    std::cout << " }";
  }
};

class PrimaryExpAST : public BaseAST {
 public:
  std::string number;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> lval;
  std::string ret_val;
  bool is_const;

  enum class Type {
    EXP,
    NUMBER,
    LVAL
  };
  Type type;

  void Dump() const override {
    if (type == Type::EXP) {
      std::cout << "PrimaryExpAST { ";
      exp->Dump();
      std::cout << " }";
      return;
    }
    else if (type == Type::NUMBER) {
      std::cout << "PrimaryExpAST { " << number << " }";
      return;
    }
    else if (type == Type::LVAL) {
      std::cout << "PrimaryExpAST { ";
      lval->Dump();
      std::cout << " }";
      return;
    }
    else {
      std::cout << "PrimaryExpAST { unknown }";
    }
  }
};

class UnaryExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> primary_exp;
  std::unique_ptr<BaseAST> unary_exp;
  std::unique_ptr<BaseAST> func_rparams;
  std::string ident;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    PRIMARY_EXP,
    UNARY_EXP,
    FUNC_EXP
  };
  Type type;

  void Dump() const override {
    if (type == Type::PRIMARY_EXP) {
      std::cout << "UnaryExpAST { ";
      primary_exp->Dump();
      std::cout << " }";
    }
    else if (type == Type::UNARY_EXP) {
      std::cout << "UnaryExpAST { ";
      std::cout << op << ", ";
      unary_exp->Dump();
      std::cout << " }";
    }
    else if (type == Type::FUNC_EXP) {
      std::cout << "UnaryExpAST { ";
      std::cout << ident << ", ";
      if (func_rparams) {
        func_rparams->Dump();
      }
      std::cout << " }";
    }
    else {
      std::cout << "UnaryExpAST { unknown }";
    }
  }
};

class FuncRParamsAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> rparams;
  std::vector<std::string> ret_vals;

  void Dump() const override {
    std::cout << "FuncRParamsAST { ";
    for (const auto &rparam : rparams) {
      rparam->Dump();
      std::cout << ", ";
    }
    std::cout << " }";
  }
};

class MulExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> unary_exp;
  std::unique_ptr<BaseAST> mul_exp;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    UNARY_EXP,
    MUL_EXP
  };
  Type type;

  void Dump() const override {
    if (op.empty()) {
      std::cout << "MulExpAST { ";
      unary_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "MulExpAST { ";
      std::cout << op << ", ";
      mul_exp->Dump();
      std::cout << " }";
    }
  }
};

class AddExpAST: public BaseAST {
 public:
  std::unique_ptr<BaseAST> mul_exp;
  std::unique_ptr<BaseAST> add_exp;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    MUL_EXP,
    ADD_EXP
  };
  Type type;

  void Dump() const override {
    if (op.empty()) {
      std::cout << "AddExpAST { ";
      mul_exp->Dump();
      std::cout << " }";
    }
    else {
      std::cout << "AddExpAST { ";
      std::cout << op << ", ";
      add_exp->Dump();
      std::cout << " }";
    }
  }
};

class RelExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> add_exp;
  std::unique_ptr<BaseAST> rel_exp;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    ADD_EXP,
    REL_EXP
  };
  Type type;

  void Dump() const override {
    std::cout << "RelExpAST { ";
    add_exp->Dump();
    std::cout << " }";
  }
};

class EqExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> rel_exp;
  std::unique_ptr<BaseAST> eq_exp;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    REL_EXP,
    EQ_EXP
  };
  Type type;

  void Dump() const override {
    std::cout << "EqExpAST { ";
    rel_exp->Dump();
    std::cout << " }";
  }
};

class LAndExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> eq_exp;
  std::unique_ptr<BaseAST> land_exp;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    EQ_EXP,
    LAND_EXP
  };
  Type type;

  void Dump() const override {
    std::cout << "LAndExpAST { ";
    eq_exp->Dump();
    std::cout << " }";
  }
};

class LOrExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> land_exp;
  std::unique_ptr<BaseAST> lor_exp;
  std::string op;
  std::string ret_val;
  bool is_const;

  enum class Type {
    LAND_EXP,
    LOR_EXP
  };
  Type type;

  void Dump() const override {
    std::cout << "LOrExpAST { ";
    land_exp->Dump();
    std::cout << " }";
  }
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lor_exp;
  std::string ret_val;
  bool is_const;

  void Dump() const override {
    std::cout << "ExpAST { ";
    lor_exp->Dump();
    std::cout << " }";
  }
};

class ConstExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::string ret_val;
  bool is_const;

  void Dump() const override {
    std::cout << "ConstExpAST { ";
    exp->Dump();
    std::cout << " }";
  }
};

class LValAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> parent;
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> exp;
  bool is_const;
  std::string ret_val;
  std::string lval_ret_val = "";

  enum class Type {
    IDENT,
    ARRAY
  };
  Type type;

  void Dump() const override {
    std::cout << "LValAST { " << ident << " }";
  }
};

class IRInfo {
public:
  int current_id;

  void init();
  std::string getNextID();
  std::string getLastID();
};

class NestedArray;

using ValueType = std::variant<std::string, std::shared_ptr<NestedArray>>;

class NestedArray {
public:
  std::vector<ValueType> elements;

  int getSize() const;
  void addInt(int value);
  void addArray(std::shared_ptr<NestedArray> nestedArray);
  void print() const;
  void fromString(const std::string& ori_str);
  void getArraySize(std::string type, std::vector<int>& array_size);

private:
  int size = 0;
};

struct ConstantInfo {
  int value;
};

struct VariableInfo {
  int init_value;
  int nested_layer;
  bool is_param;
  bool is_uninitialized;
  bool is_pointer;
  bool is_array;
};

struct FunctionInfo {
  std::string type;
};

using SymbolInfo = std::variant<ConstantInfo, VariableInfo, FunctionInfo>;

class SymbolTable {
public:
    std::string postfix;
    void insertConstant(const std::string& symbol, int value);
    void insertVariable(const std::string& symbol, int nested_layer, bool is_uninitialized, bool is_array);
    void insertFunction(const std::string& symbol, const std::string& type);
    void insertParams(const std::string& symbol, int nested_layer, bool is_pointer);
    // if_cur_block is true, search in current block, only use in insertVariable/Constant
    bool symbolExists(const std::string& symbol, bool in_cur_block) const;
    bool constSymbolExists(const std::string& symbol, bool in_cur_block) const;
    bool varSymbolExists(const std::string& symbol, bool in_cur_block) const;
    bool funcSymbolExists(const std::string& symbol, bool in_cur_block) const;
    std::string getSymbolPostfix(const std::string& symbol) const;
    int getSymbolNestedLayer(const std::string& symbol) const;
    std::string getFunctionType(const std::string& symbol) const;
    void setParent(SymbolTable* parent);
    void setChild(SymbolTable* child);
    SymbolTable* getParent();
    SymbolTable* getChild();

    std::optional<SymbolInfo> getSymbolInfo(const std::string& symbol) const;

private:
    std::unordered_map<std::string, SymbolInfo> table;
    SymbolTable* parent;
    SymbolTable* child;
};

class AST2IRConverter {
public:
  std::string Convert(BaseAST *ast);
  void init();
  IRInfo ir_info;
  SymbolTable* symbol_table;
  
private:
  bool has_ret = false;
  std::string func_type = "";
  int symbol_table_num = 0;
  int if_block_num = 0;
  int while_block_num = 0;
  std::stack<int> while_block_stack;
  std::string ConvertCompUnit(CompUnitAST *comp_unit);
  std::string ConvertGlobalDef(GlobalDefAST *global_def);
  std::string ConvertFuncDef(FuncDefAST *func_def);
  std::string ConvertFuncType(std::string func_type);
  std::string ConvertFuncFParams(FuncFParamsAST *func_fparams);
  std::string ConvertFuncFParam(FuncFParamAST *func_fparam);
  std::string ConvertFuncFParamType(FuncFParamTypeAST *func_fparam_type);
  std::string ConvertArrayIdent(ArrayIdentAST *array_ident);
  std::string ConvertBlock(BlockAST *block);
  std::string ConvertStmtList(StmtAST *stmt_list);
  std::string ConvertIfStmt(IfStmtAST *if_stmt);
  std::string ConvertElseStmt(ElseStmtAST *else_stmt);
  std::string ConvertPrimaryExp(PrimaryExpAST *primary_exp);
  std::string ConvertUnaryExp(UnaryExpAST *unary_exp);
  std::string ConvertFuncRParams(FuncRParamsAST *func_rparams);
  std::string ConvertMulExp(MulExpAST *mul_exp);
  std::string ConvertAddExp(AddExpAST *add_exp);
  std::string ConvertRelExp(RelExpAST *rel_exp);
  std::string ConvertEqExp(EqExpAST *eq_exp);
  std::string ConvertLAndExp(LAndExpAST *land_exp);
  std::string ConvertLOrExp(LOrExpAST *lor_exp);
  std::string ConvertBlockItem(BlockItemAST *block_item);
  std::string ConvertDecl(DeclAST *decl);
  std::string ConvertConstDecl(ConstDeclAST *const_decl);
  std::string ConvertConstDef(ConstDefAST *const_def);
  std::string ConvertConstInitVal(ConstInitValAST *const_init_val);
  std::string ConvertVarDecl(VarDeclAST *var_decl);
  std::string ConvertVarDef(VarDefAST *var_def);
  std::string ConvertInitVal(InitValAST *init_val);
  std::string ConvertLVal(LValAST *lval);
  std::string ConvertLValStore(LValAST *lval_exp);
  std::string ConvertConstExp(ConstExpAST *const_exp);
  std::string ConvertExp(ExpAST *exp);
  //std::string ArrayInit(std::vector<int>& array_size, std::string value, std::string ptr_name);
  std::string zeroPadding(NestedArray* nestedArray, std::vector<int>& array_size, std::string ptr_name);
  std::string zeroPaddingGlobal(NestedArray* nestedArray, std::vector<int>& array_size);
  std::string getIdent(LValAST *lval);

  void EnterChildSymbolTable();
  void ExitChildSymbolTable();
  std::string IfBlockPostfix();
  std::string WhileBlockPostfix();
  void EnterWhileBlock();
  void ExitWhileBlock();
  bool InWhileBlock();
};