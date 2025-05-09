#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <cstring>
#include "koopa.h"
#include <unordered_map>
#include <vector>
#include "my_var.h"
#include <set>

extern std::string str;
extern std::vector<std::unordered_map<std::string, MyVar>> symbol_tables;
extern int block_num;
extern int test_val;
static int cnt=0;
static int block_cnt=0;
static int ident_cnt=0;
extern std::unordered_map<std::string, bool> block_end;
extern std::string now_block;
extern std::vector<std::string> nested_while_stack;
extern std::unordered_map<std::string, bool> has_ret;
extern std::unordered_map<std::string, bool> func_ret;
extern std::string now_func;
extern int res_cnt;
extern int ptr_cnt;

class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class FuncParamAST;
class BlockAST;
class BlockItemAST;
class StmtAST;
class BaseExprAST;
class ExprAST;
class UnaryExprAST;
class PrimaryExprAST;
class AddExprAST;
class MulExprAST;
class LOrExpr;
class LAndExpr;
class EqExpr;
class RelExpr;
class LValAST;
class LeftLValAST;
class DeclAST;
class ConstDeclAST;
class ConstDef;
class ConstInitValAST;
class ConstExprAST;
class VarDeclAST;
class VarDefAST;
class InitValAST;

int ident_floor(std::string ident);

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;  
    virtual std::string koopa_ir() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    // std::unique_ptr<BaseAST> func_def;
    std::vector<std::unique_ptr<BaseAST>> unit_list;

    void Dump() const override;
    std::string koopa_ir() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    std::vector<std::unique_ptr<BaseAST>> func_param_list;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class FuncParamAST : public BaseAST{
public:
    enum Flag{
        INT,
        ARRAY,
    }flag;
    std::string btype;
    std::string ident;
    std::vector<std::unique_ptr<BaseExprAST>> more_array_size; // except first dim

    void Dump() const override;
    std::string koopa_ir() const override;
};

class FuncTypeAST : public BaseAST{
public:
    std::string type;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class BlockAST : public BaseAST{
public:
    // std::unique_ptr<BaseAST> block_item;
  std::vector<std::unique_ptr<BaseAST>> block_item_list;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class BlockItemAST : public BaseAST{
public:
    enum Flag{
        STMT=0, // Stmt
        DECL, // Decl
    }flag;

    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class StmtAST : public BaseAST{
public:
    enum Flag{
        ASSIGN,
        RETURN,
        BLOCK,
        EMPTY,
        EXPR,
        RETURN_VOID,
        IF,
        IF_ELSE,
        WHILE,
        BREAK,
        CONTINUE,
    }flag;
    
    std::unique_ptr<BaseExprAST> expr;
    std::unique_ptr<BaseExprAST> lval;
    std::unique_ptr<BaseAST> block; 
    std::unique_ptr<BaseExprAST> cond;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class BaseExprAST : public BaseAST{
public:
    int val;
    std::string ident;

    BaseExprAST(){}
    BaseExprAST(int val) : val(val) {}
    BaseExprAST(const BaseExprAST& other) : val(other.val) {}
    virtual int getVal() const = 0;
};

class ExprAST : public BaseExprAST{
public:

    std::unique_ptr<BaseExprAST> lor_expr;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        return lor_expr->getVal();
    }
};

class UnaryExprAST : public BaseExprAST{
public:
    enum Flag{
        PRIMARY_EXPR=0, //PrimaryExp
        OP_UNARY,   //UnaryOp UnaryExp
        CALL, // Call "(" [Exp {"," Exp}] ")"
    }flag;

    std::string op;
    std::unique_ptr<BaseExprAST> unary_expr;
    std::unique_ptr<BaseExprAST> primary_expr;
    std::string call_ident;
    std::vector<std::unique_ptr<BaseAST>> call_expr_list;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==PRIMARY_EXPR)
            return primary_expr->getVal();
        else if(op=="+")
            return unary_expr->getVal();
        else if(op=="-")
            return -unary_expr->getVal();
        else if(op=="!")
            return !unary_expr->getVal();
        return 0;
    }
};

class CallParamAST : public BaseAST{
public:
    std::unique_ptr<BaseExprAST> expr;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class PrimaryExprAST : public BaseExprAST{
public:
    enum Flag{
        NUMBER=0, // Num
        WITH_BRACKETS, // "(" Exp ")"
        LVAL, // Ident
    }flag;

    std::string number;
    std::unique_ptr<BaseExprAST> expr;
    std::unique_ptr<BaseExprAST> lval;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==NUMBER)
            return val;
        else if(flag==WITH_BRACKETS)
            return expr->getVal();
        else if(flag==LVAL){
            return lval->getVal();}
        return 0;
    }
};

class AddExprAST : public BaseExprAST{
public:
    enum Flag{
        ONLY_MUL=0, // MulExp
        ADD_MUL,    // AddExp Op MulExp;
    }flag;

    std::unique_ptr<BaseExprAST> mul_expr;
    std::unique_ptr<BaseExprAST> add_expr;
    std::string op;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==ONLY_MUL)
            return mul_expr->getVal();
        else if(flag==ADD_MUL){
            if(op=="+")
                return add_expr->getVal()+mul_expr->getVal();
            else if(op=="-")
                return add_expr->getVal()-mul_expr->getVal();
        }
        return 0;
    }
};

class MulExprAST : public BaseExprAST{
public:
    enum Flag{
        ONLY_UNARY=0, // UnaryExp
        MUL_UNARY,    // MulExp Op UnaryExp;
    }flag;

    std::unique_ptr<BaseExprAST> unary_expr;
    std::unique_ptr<BaseExprAST> mul_expr;
    std::string op;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==ONLY_UNARY)
            return unary_expr->getVal();
        else if(flag==MUL_UNARY){
            if(op=="*")
                return mul_expr->getVal()*unary_expr->getVal();
            else if(op=="/"){
                if(unary_expr->getVal()==0)
                    return 1;
                return mul_expr->getVal()/unary_expr->getVal();
            }
            else if(op=="%"){
                if(unary_expr->getVal()==0)
                    return 1;
                return mul_expr->getVal()%unary_expr->getVal();
            }
        }
        return 0;
    }
};

class LOrExprAST : public BaseExprAST{
public:
    enum Flag{
        ONLY_LAND=0, // LAndExpr
        LOR_LAND,    // LOrExpr "||" LAndExpr;
    }flag;

    std::unique_ptr<BaseExprAST> land_expr;
    std::string op;
    std::unique_ptr<BaseExprAST> lor_expr;

    void Dump() const override; 
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==ONLY_LAND)
            return land_expr->getVal();
        else if(flag==LOR_LAND){
            if(op=="||"){
                if(lor_expr->getVal()){
                    return true;
                }
                return bool(land_expr->getVal());
            }
        }
        return 0;
    }
};

class LAndExprAST : public BaseExprAST{
public:
    enum Flag{
        ONLY_EQ=0, // EqExpr
        LAND_EQ,    // LAndExpr "&&" EqExpr;
    }flag;
    std::unique_ptr<BaseExprAST> eq_expr;
    std::string op;
    std::unique_ptr<BaseExprAST> land_expr;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==ONLY_EQ)
            return eq_expr->getVal();
        else if(flag==LAND_EQ){
            if(op=="&&"){
                if(!land_expr->getVal()){
                    return false;
                }
                return bool(eq_expr->getVal());
            }
        }
        return 0;
    }
};

class EqExprAST : public BaseExprAST{
public:
    enum Flag{
        ONLY_REL=0, // RelExpr
        EQ_REL,    // EqExpr ("=="|"!=") RelExpr;
    }flag;
    std::unique_ptr<BaseExprAST> rel_expr;
    std::string op;
    std::unique_ptr<BaseExprAST> eq_expr;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==ONLY_REL)
            return rel_expr->getVal();
        else if(flag==EQ_REL){
            if(op=="==")
                return eq_expr->getVal()==rel_expr->getVal();
            else if(op=="!=")
                return eq_expr->getVal()!=rel_expr->getVal();
        }
        return 0;
    }
};

class RelExprAST : public BaseExprAST{
public:
    enum Flag{
        ONLY_ADD=0, // AddExpr
        REL_ADD,    // RelExpr ("<"|"<="|">"|">=") AddExpr;
    }flag;
    std::unique_ptr<BaseExprAST> add_expr;
    std::string op;
    std::unique_ptr<BaseExprAST> rel_expr;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==ONLY_ADD)
            return add_expr->getVal();
        else if(flag==REL_ADD){
            if(op=="<")
                return rel_expr->getVal()<add_expr->getVal();
            else if(op=="<=")
                return rel_expr->getVal()<=add_expr->getVal();
            else if(op==">")
                return rel_expr->getVal()>add_expr->getVal();
            else if(op==">=")
                return rel_expr->getVal()>=add_expr->getVal();
        }
        return 0;
    }
};

class LValAST : public BaseExprAST{
public:
    enum Flag{
        IDENT=0, // Ident
        LVAL_IDX, // LVal ["[" Exp "]"]
    }flag;
    std::string ident;
    std::vector<std::unique_ptr<BaseExprAST>> idx_lst;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==IDENT)
            return symbol_tables[ident_floor(ident)][ident].val;
        else{
            int idx=0;
            std::vector<int> dim_len=symbol_tables[ident_floor(ident)][ident].dim_len;
            if(dim_len[0]!=-1){
                for(int i=idx_lst.size()-1;i>=0;i--){
                    if(i!=idx_lst.size()-1)
                        idx *= dim_len[i+1];
                    idx += idx_lst[i]->getVal();
                }
                return symbol_tables[ident_floor(ident)][ident].array[idx];
            }
            else{
                return 0;
            }
        }
    }
};

class LeftLValAST: public BaseExprAST{
public:
    enum Flag{
        IDENT=0, // Ident
        LVAL_IDX, // LVal ["[" Exp "]"]
    }flag;
    std::string ident;
    std::vector<std::unique_ptr<BaseExprAST>> idx_lst;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        if(flag==IDENT)
            return symbol_tables[ident_floor(ident)][ident].val;
        else{
            int idx=0;
            std::vector<int> dim_len=symbol_tables[ident_floor(ident)][ident].dim_len;
            if(dim_len[0]!=-1){
                for(int i=idx_lst.size()-1;i>=0;i--){
                    if(i!=idx_lst.size()-1)
                        idx *= dim_len[i+1];
                    idx += idx_lst[i]->getVal();
                }
                return symbol_tables[ident_floor(ident)][ident].array[idx];
            }
            else{
                return 0;
            }
        }
    }
};

class DeclAST : public BaseAST{
public:
    enum Flag{
        CONST_DECL=0, // ConstDecl
        VAR_DECL, // VarDecl
    }flag;
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class ConstDeclAST : public BaseAST{
public:
    std::string btype;
    // std::unique_ptr<BaseAST> const_def;
    std::vector<std::unique_ptr<BaseAST>> const_def_list;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class ConstDefAST : public BaseAST{
public:
    enum Flag{
        VAR=0,
        ARRAY=1,
    } flag;
    std::string ident;
    std::unique_ptr<BaseExprAST> const_init_val;
    std::vector<std::unique_ptr<BaseExprAST>> len;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class ConstInitValAST : public BaseExprAST{
public:
    enum Flag{
        EXPR=0,
        ARRAY=1,
    } flag;
    std::unique_ptr<BaseExprAST> const_expr;
    std::vector<std::unique_ptr<BaseExprAST>> const_expr_list;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        return const_expr->getVal();
    }
};

class ConstExprAST : public BaseExprAST{
public:
    std::unique_ptr<BaseExprAST> expr;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        return expr->getVal();
    }
};

class VarDeclAST : public BaseAST{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> var_def_list;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class VarDefAST : public BaseAST{
public:
    enum Flag{
        VAR=0,
        ARRAY=1,
    } flag;
    std::string ident;
    std::unique_ptr<BaseExprAST> var_init_val;
    bool is_init;
    std::vector<std::unique_ptr<BaseExprAST>> len;

    void Dump() const override;
    std::string koopa_ir() const override;
};

class VarInitValAST : public BaseExprAST{
public:
    enum Flag{
        EXPR=0,
        ARRAY=1,
    } flag;
    std::unique_ptr<BaseExprAST> expr;
    std::vector<std::unique_ptr<BaseExprAST>> expr_list;

    void Dump() const override;
    std::string koopa_ir() const override;
    int getVal() const override{
        return expr->getVal();
    }
};

