#pragma once
#include <iostream>
#include <memory>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include "env.hpp"
#include <cassert>
#include <algorithm>

extern std::string koopa_str;

// 所有 AST 的基类
class BaseAST {
    public:
        virtual ~BaseAST() = default;

        virtual void Dump() const = 0;
        virtual void generateKoopaIR() const = 0;
        virtual int calculateValue();
        virtual std::string LValIDENT();
        virtual int ConstInitType();
        virtual int InitType();
};
// CompUnit 是 BaseAST
// CompUnit     ::= CompUnitList
class CompUnitAST : public BaseAST {
    public:
        // 用智能指针管理对象 
        std::vector<std::unique_ptr<BaseAST>> compUnitItemVec;

        void Dump() const override;
        void generateKoopaIR() const override;
};
// CompUnitList     ::= CompUnitItem | CompUnitItem CompUnitList

// CompUnitItem     ::= FuncDef | Decl
class CompUnitItemType_1_AST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> funcDef;

        void Dump() const override;
        void generateKoopaIR() const override;
};
class CompUnitItemType_2_AST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> decl;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// FuncDef 也是 BaseAST
// FuncDef     ::= FuncType IDENT "(" [FuncFParams] ")" Block;
class FuncDefAST : public BaseAST {
    public:
        std::string func_type;
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> funcFParamsVec;
        std::unique_ptr<BaseAST> block;

        void Dump() const override;
        void generateKoopaIR() const override;
};
// FuncType 也是 BaseAST
// FuncType    ::= "void" | "int";
// class FuncTypeAST : public BaseAST {
//     public:
//         std::string type;

//         void Dump() const override;
//         void generateKoopaIR() const override;
// };
// FuncFParams ::= FuncFParam {"," FuncFParam};

// FuncFParam ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];
class FuncFParamAST : public BaseAST{
    public:
        std::string bType;
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> constExpVec;
        int type;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// Block 也是 BaseAST
// Block         ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {
    public:
        std::vector<std::unique_ptr<BaseAST>> blockItemVec;

        void Dump() const override;
        void generateKoopaIR() const override;
};

//BlockItem
//BlockItem     ::= Decl | Stmt;
class BlockItemType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> decl;

        void Dump() const override;
        void generateKoopaIR() const override;
};
class BlockItemType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> stmt;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// Decl
// Decl          ::= ConstDecl | VarDecl;
class DeclType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> constDecl;

        void Dump() const override;
        void generateKoopaIR() const override;
};

class DeclType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> varDecl;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// ConstDecl
// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDeclAST : public BaseAST{
    public:
        std::string bType;
        std::vector<std::unique_ptr<BaseAST>> constDefVec;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// BType
// BType         ::= "int";
// class BTypeAST : public BaseAST {
//     public:
//         std::string type;

//         void Dump() const override;
//         void generateKoopaIR() const override;
// };

// ConstDef
// ConstDef      ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
class ConstDefAST : public BaseAST{
    public:
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> constExpVec;
        std::unique_ptr<BaseAST> constInitVal;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// ConstInitVal
// ConstInitVal  ::= ConstExp | "{" [ConstExp {"," ConstExp}] "}";
class ConstInitValType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> constExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
        int ConstInitType() override;
};
class ConstInitValType_2_AST : public BaseAST{
    public:
        std::vector<std::unique_ptr<BaseAST>> constInitValVec;

        void Dump() const override;
        void generateKoopaIR() const override;
        std::vector<int> Aggregate(std::vector<int> vec);
        int ConstInitType() override;
};

// ConstExp
// ConstExp      ::= Exp;
class ConstExpAST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> exp;
        
        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};

// VarDecl
// VarDecl       ::= BType VarDef {"," VarDef} ";";
class VarDeclAST : public BaseAST{
    public:
        std::string bType;
        std::vector<std::unique_ptr<BaseAST>> varDefVec;

        void Dump() const override;
        void generateKoopaIR() const override;
};

// VarDef
// VarDef        ::= IDENT {"[" ConstExp "]"}
//                 | IDENT {"[" ConstExp "]"} "=" InitVal;
class VarDefType_1_AST : public BaseAST{
    public:
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> constExpVec;

        void Dump() const override;
        void generateKoopaIR() const override;
};
class VarDefType_2_AST : public BaseAST{
    public:
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> constExpVec;
        std::unique_ptr<BaseAST> initVal;
        
        void Dump() const override;
        void generateKoopaIR() const override;
};

// InitVal
// InitVal       ::= Exp | "{" [Exp {"," Exp}] "}";
class InitValType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> exp;
        
        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
        int InitType() override;
};
class InitValType_2_AST : public BaseAST{
    public:
        std::vector<std::unique_ptr<BaseAST>> initValVec;
        
        void Dump() const override;
        void generateKoopaIR() const override;
        std::vector<int> Aggregate(std::vector<int> vec);
        int InitType() override;
};

// Stmt 也是 BaseAST
// Stmt        ::= "return" [Exp] ";"                     1
                // |  LVal "=" Exp ";"                    2
                // | [Exp] ";"                            3
                // | Block                                4
                // | "if" "(" Exp ")" Stmt ["else" Stmt]  5
                // | "while" "(" Exp ")" Stmt             6
                // | "break" ";"                          7
                // | "continue" ";";                      8
class StmtType_1_AST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> exp;

        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> lVal;
        std::unique_ptr<BaseAST> exp;
        
        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_3_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> exp;
        
        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_4_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> block;
        
        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_5_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> exp;
        std::unique_ptr<BaseAST> ifStmt;
        std::unique_ptr<BaseAST> elseStmt;
        
        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_6_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> exp;
        std::unique_ptr<BaseAST> stmt;
                
        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_7_AST : public BaseAST{
    public:                
        void Dump() const override;
        void generateKoopaIR() const override;
};
class StmtType_8_AST : public BaseAST{
    public:                
        void Dump() const override;
        void generateKoopaIR() const override;
};
//Exp  
//Exp         ::= AddExp;
class ExpAST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> lOrExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
//MulExp
//MulExp  ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class MulExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> unaryExp;
        
        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class MulExpType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> mulExp;
        char mulOp;
        std::unique_ptr<BaseAST> unaryExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
//AddExp
//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
class AddExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> mulExp;
        
        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class AddExpType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> addExp;
        char addOp;
        std::unique_ptr<BaseAST> mulExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
//PrimaryExp
//PrimaryExp  ::= "(" Exp ")" | Number | LVal;
class PrimaryExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> exp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class PrimaryExpType_2_AST : public BaseAST{
    public:
        int number;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class PrimaryExpType_3_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> lVal;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};

// LVal
// LVal          ::= IDENT {"[" Exp "]"};
class LValAST : public BaseAST{
    public:
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> expVec;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
        std::string LValIDENT() override;
};

//UnaryExp
//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp | IDENT "(" [FuncRParams] ")";
class UnaryExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> primaryExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class UnaryExpType_2_AST : public BaseAST{
    public:
        char unaryOp;
        std::unique_ptr<BaseAST> unaryExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class UnaryExpType_3_AST : public BaseAST {
    public:
        std::string ident;
        std::vector<std::unique_ptr<BaseAST>> expVec;
        
        void Dump() const override;
        void generateKoopaIR() const override;
};

//RelExp
//RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
class RelExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> addExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class RelExpType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> relExp;
        std::string relOp;
        std::unique_ptr<BaseAST> addExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
//EqExp
//EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
class EqExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> relExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class EqExpType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> eqExp;
        std::string eqOp;
        std::unique_ptr<BaseAST> relExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
//LAndExp
//LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> eqExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class LAndExpType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> lAndExp;
        std::unique_ptr<BaseAST> eqExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
//LOrExp
//LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpType_1_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> lAndExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};
class LOrExpType_2_AST : public BaseAST{
    public:
        std::unique_ptr<BaseAST> lOrExp;
        std::unique_ptr<BaseAST> lAndExp;

        void Dump() const override;
        void generateKoopaIR() const override;
        int calculateValue() override;
};