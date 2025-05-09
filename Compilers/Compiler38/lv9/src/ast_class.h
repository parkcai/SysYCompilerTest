#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace std;

enum class BlockItem_type {Decl, Stmt};
enum class Stmt_type {If_Then, If_Then_Else, Return_Exp, Assign, Assign_Array, Block, Exp, Empty, Return_Void, While, Break, Continue};
enum class Decl_type {Const, Var};
enum class Def_type {Const, Var};
enum class Value_type {LVal, ConstIdent, VarIdent};
enum class Initval_type {Const, Var};
enum class PrimaryExp_type {LVal, INT_CONST, FuncCall};
enum class Exp_op {ADD, SUB, MUL, DIV, MOD, LAND, LOR, LT, GT, LE, GE, EQ, NE, PRI, POS, NEG, NOT};

class Symbol;
class BaseAST;
class CompUnitAST;
class FuncDefAST;
class BlockAST;
class BlockItemAST;
class StmtAST;
class ExpAST;
class PrimaryExpAST;
class DeclAST;
class DefAST;
class FuncFParamsAST;
class FuncFParamAST;
class FuncRParamsAST;
class ValueAST;
class InitvalAST;

class BaseAST
{
  public:
    std::string class_name;
    BaseAST() = default;
    virtual ~BaseAST() = default;
    virtual void setSymbolTable() = 0;
    virtual void generate() = 0;
};

class CompUnitAST : public BaseAST
{
  public:
    std::vector<DeclAST *> decls;
    std::vector<FuncDefAST *> funcs;
    CompUnitAST();
    virtual ~CompUnitAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    void dump(std::string &code);
};

class FuncDefAST : public BaseAST
{
  public:
    std::string func_type;
    std::string ident;
    FuncFParamsAST *params = nullptr;
    BlockAST *block = nullptr;
    FuncDefAST();
    virtual ~FuncDefAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
};

class BlockAST : public BaseAST
{
  public:
    std::vector<BlockItemAST *> items;
    std::vector<StmtAST *> stmts;
    BlockAST();
    virtual ~BlockAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    
};

class BlockItemAST : public BaseAST
{
  public:
    BlockItem_type type;
    DeclAST *decl = nullptr;
    StmtAST *stmt = nullptr;
    BlockItemAST();
    virtual ~BlockItemAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
};

class StmtAST : public BaseAST
{
  public:
    Stmt_type type;
    ValueAST *lval = nullptr;
    ExpAST *exp = nullptr;
    BlockAST *block = nullptr;
    StmtAST *stmt1 = nullptr;
    StmtAST *stmt2 = nullptr;
    std::vector<int> initvalArray;
    StmtAST();
    virtual ~StmtAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
};

class PrimaryExpAST : public BaseAST
{
  public:
    PrimaryExp_type type;
    std::string func_name;
    int const_val;
    FuncRParamsAST *params = nullptr;
    ValueAST *lval = nullptr;
    int *cache = nullptr;
    PrimaryExpAST();
    virtual ~PrimaryExpAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    std::string get_value_ir();
    int calc();
};

class ExpAST : public BaseAST
{
  public:
    Exp_op op;
    ExpAST *left = nullptr;
    ExpAST *right = nullptr;
    PrimaryExpAST *primary = nullptr;
    int *cache = nullptr;
    ExpAST();
    ~ExpAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    std::string get_value_ir();
    int calc();
};

class DeclAST : public BaseAST
{
  public:
    Decl_type type;
    std::string btype;
    std::vector<DefAST *> defs;
    DeclAST();
    virtual ~DeclAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
};

class DefAST : public BaseAST
{
  public:
    Def_type type;
    ValueAST *value = nullptr;
    InitvalAST *initval = nullptr;
    StmtAST *init_stmt = nullptr;
    DefAST();
    virtual ~DefAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    void construct_init_stmt(ExpAST *exp);
    void construct_init_stmt(std::vector<int> value_vec);
};

class FuncFParamsAST : public BaseAST
{
  public:
    std::vector<FuncFParamAST *> params;
    FuncFParamsAST();
    virtual ~FuncFParamsAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    std::string get_params_ir();
};

class FuncFParamAST : public BaseAST
{
  public:
    ValueAST *value = nullptr;
    FuncFParamAST();
    virtual ~FuncFParamAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
};

class FuncRParamsAST : public BaseAST
{
  public:
    std::vector<ExpAST *> exps;
    FuncRParamsAST();
    virtual ~FuncRParamsAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    std::string get_params_ir();
};

class ValueAST : public BaseAST
{
  public:
    Value_type type;
    std::string ident;
    std::vector<ExpAST *> exps;
    bool empty_array_init = false;
    Symbol *symbol = nullptr;   // 能否简化？？
    ValueAST();
    virtual ~ValueAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    std::vector<int> getArrayDim();
    std::string get_addr_ir();
    std::string get_value_ir();
};

class InitvalAST : public BaseAST
{
  public:
    Initval_type type;
    ExpAST *exp = nullptr;
    std::vector<InitvalAST *> inits;
    InitvalAST();
    virtual ~InitvalAST() override;
    virtual void setSymbolTable() override;
    virtual void generate() override;
    virtual std::vector<int> getInitVector(std::vector<int> dims);
};
