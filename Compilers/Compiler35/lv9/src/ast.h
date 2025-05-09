#pragma once
#include <memory>
#include <vector>
#include <variant>
#include <iostream>

void error(const std::string &s);

struct Label {
    int index;
    static int curIndex;
    friend std::ostream &operator<<(std::ostream &os, const Label &l) {
        return os << "%L" << l.index;
    }
    bool operator==(const Label &l) const = default;
};

constexpr Label nextInstr = Label {-1};
constexpr Label unreachable = Label {-2};
constexpr Label illegal = Label {-3};

struct Succ {
    Label lNext;
    Label lBreak = illegal;
    Label lContinue = illegal;
};

struct Undefined {};

struct Temp {
    int index;
    static int curIndex;
    Temp() : index(curIndex++) {}
    friend std::ostream &operator<<(std::ostream &os, const Temp &t) {
        return os << "%" << t.index;
    }
};

struct Const {
    int value;
    Const(int value_) : value(value_) {}
    friend std::ostream &operator<<(std::ostream &os, const Const &c) {
        return os << c.value;
    }
};

struct Type {
    std::vector<int> dims;
    bool ptr;
    Type(std::vector<int> dims_ = {}, bool ptr_ = false) : dims(dims_), ptr(ptr_) {}
    friend std::ostream &operator<<(std::ostream &os, const Type &type) {
        if (type.ptr) {
            os << "*";
        }
        for (int i = 0; i < type.dims.size(); i++) {
            os << "[";
        }
        os << "i32";
        for (int i = type.dims.size() - 1; i >= 0; i--) {
            os << ", " << type.dims[i] << "]";
        }
        return os;
    }
};

struct Var {
    std::string name;
    Type type;
    Var(std::string name_, Type type_ = Type()) : name(name_), type(type_) {}
    friend std::ostream &operator<<(std::ostream &os, const Var &var) {
        return os << "@" << var.name;
    }
};

struct Func {
    int type;
    Func(int type_) : type(type_) {}
};

using Result = std::variant<Undefined, Temp, Const, Var, Func>;
std::ostream &operator<<(std::ostream &os, const Result &r);
bool isUndefined(const Result &r);
bool isTemp(const Result &r);
bool isConst(const Result &r);
bool isVar(const Result &r);
bool isFunc(const Result &r);

class BaseAST {
public:
    Succ succ;
    Result result;
    virtual ~BaseAST() = default;
    virtual void dump(std::ostream &os) const = 0;
    virtual void koopa(std::ostream &os) = 0;
};

template<class T, class U>
std::unique_ptr<T> convertToUnique(U *p) {
    return std::unique_ptr<T>((T *) p);
}

template<class T, class U>
std::vector<std::unique_ptr<T>> convertToVector(std::vector<U *> *p) {
    std::vector<std::unique_ptr<T>> vec;
    for (auto i : *p) {
        vec.push_back(std::unique_ptr<T>((T *) i));
    }
    return vec;
}

class CompUnit;
class GlobalItem;
class GlobalDecl;
class GlobalFuncDef;
class FuncDef;
class Block;
class Stmt;
class StmtAssign;
class StmtExp;
class StmtBlock;
class StmtIf;
class StmtWhile;
class StmtBreak;
class StmtContinue;
class StmtReturn;
class Exp;
class PrimaryExp;
class PrimaryExpParentheses;
class PrimaryExpNumber;
class PrimaryExpLVal;
class UnaryExp;
class UnaryExpPrimary;
class UnaryExpUnaryOp;
class UnaryExpCall;
class UnaryOp;
class UnaryOpPos;
class UnaryOpNeg;
class UnaryOpNot;
class AddExp;
class AddExpMulExp;
class AddExpAddOp;
class MulExp;
class MulExpUnaryExp;
class MulExpMulOp;
class AddOp;
class AddOpAdd;
class AddOpSub;
class MulOp;
class MulOpMul;
class MulOpDiv;
class MulOpMod;
class RelExp;
class RelExpAddExp;
class RelExpRelOp;
class RelOp;
class RelOpLt;
class RelOpGt;
class RelOpLeq;
class RelOpGeq;
class EqExp;
class EqExpRelExp;
class EqExpEqOp;
class EqOp;
class EqOpEq;
class EqOpNeq;
class LAndExp;
class LAndExpEqExp;
class LAndExpAnd;
class LOrExp;
class LOrExpLAndExp;
class LOrExpOr;
class Decl;
class DeclConstDecl;
class DeclVarDecl;
class ConstDecl;
class ConstDef;
class ConstInitVal;
class ConstExp;
class LVal;
class BlockItem;
class BlockItemDecl;
class BlockItemStmt;
class VarDecl;
class VarDef;
class InitVal;
class InitValExp;
class InitList;
class FuncFParam;
class ArrIdent;

class CompUnit : public BaseAST {
public:
    std::vector<std::unique_ptr<GlobalItem>> items;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class GlobalItem : public BaseAST {
public:
};

class GlobalDecl : public GlobalItem {
public:
    std::unique_ptr<Decl> decl;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class GlobalFuncDef : public GlobalItem {
public:
    std::unique_ptr<FuncDef> funcDef;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class FuncDef : public BaseAST {
public:
    int funcType;
    std::string ident;
    std::vector<std::unique_ptr<FuncFParam>> funcFParams;
    std::unique_ptr<Block> block;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class Block : public BaseAST {
public:
    std::vector<std::unique_ptr<BlockItem>> blockItems;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class Stmt : public BaseAST {
public:
};

class StmtAssign : public Stmt {
public:
    std::unique_ptr<LVal> lVal;
    std::unique_ptr<Exp> exp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtExp : public Stmt {
public:
    std::unique_ptr<Exp> exp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtBlock : public Stmt {
public:
    std::unique_ptr<Block> block;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtIf : public Stmt {
public:
    std::unique_ptr<Exp> exp;
    std::unique_ptr<Stmt> stmtIf;
    std::unique_ptr<Stmt> stmtElse;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtWhile : public Stmt {
public:
    std::unique_ptr<Exp> exp;
    std::unique_ptr<Stmt> stmt;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtBreak : public Stmt {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtContinue : public Stmt {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class StmtReturn : public Stmt {
public:
    std::unique_ptr<Exp> exp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class Exp : public BaseAST {
public:
    std::unique_ptr<LOrExp> lOrExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class PrimaryExp : public BaseAST {
public:
};

class PrimaryExpParentheses : public PrimaryExp {
public:
    std::unique_ptr<Exp> exp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class PrimaryExpNumber : public PrimaryExp {
public:
    int number;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class PrimaryExpLVal : public PrimaryExp {
public:
    std::unique_ptr<LVal> lVal;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class UnaryExp : public BaseAST {
public:
};

class UnaryExpPrimary : public UnaryExp {
public:
    std::unique_ptr<PrimaryExp> primaryExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class UnaryExpUnaryOp : public UnaryExp {
public:
    std::unique_ptr<UnaryOp> unaryOp;
    std::unique_ptr<UnaryExp> unaryExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class UnaryExpCall : public UnaryExp {
public:
    std::string ident;
    std::vector<std::unique_ptr<Exp>> args;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class UnaryOp : public BaseAST {
public:
    Result arg;
};

class UnaryOpPos : public UnaryOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class UnaryOpNeg : public UnaryOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class UnaryOpNot : public UnaryOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class AddExp : public BaseAST {
public:
};

class AddExpMulExp : public AddExp {
public:
    std::unique_ptr<MulExp> mulExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class AddExpAddOp : public AddExp {
public:
    std::unique_ptr<AddExp> addExp;
    std::unique_ptr<AddOp> addOp;
    std::unique_ptr<MulExp> mulExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class MulExp : public BaseAST {
public:
};

class MulExpUnaryExp : public MulExp {
public:
    std::unique_ptr<UnaryExp> unaryExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class MulExpMulOp : public MulExp {
public:
    std::unique_ptr<MulExp> mulExp;
    std::unique_ptr<MulOp> mulOp;
    std::unique_ptr<UnaryExp> unaryExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class AddOp : public BaseAST {
public:
    Result arg1;
    Result arg2;
};

class AddOpAdd : public AddOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class AddOpSub : public AddOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class MulOp : public BaseAST {
public:
    Result arg1;
    Result arg2;
};

class MulOpMul : public MulOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class MulOpDiv : public MulOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class MulOpMod : public MulOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class RelExp : public BaseAST {
public:
};

class RelExpAddExp : public RelExp {
public:
    std::unique_ptr<AddExp> addExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class RelExpRelOp : public RelExp {
public:
    std::unique_ptr<RelExp> relExp;
    std::unique_ptr<RelOp> relOp;
    std::unique_ptr<AddExp> addExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class RelOp : public BaseAST {
public:
    Result arg1;
    Result arg2;
};

class RelOpLt : public RelOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class RelOpGt : public RelOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class RelOpLeq : public RelOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class RelOpGeq : public RelOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class EqExp : public BaseAST {
public:
};

class EqExpRelExp : public EqExp {
public:
    std::unique_ptr<RelExp> relExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class EqExpEqOp : public EqExp {
public:
    std::unique_ptr<EqExp> eqExp;
    std::unique_ptr<EqOp> eqOp;
    std::unique_ptr<RelExp> relExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class EqOp : public BaseAST {
public:
    Result arg1;
    Result arg2;
};

class EqOpEq : public EqOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class EqOpNeq : public EqOp {
public:
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class LAndExp : public BaseAST {
public:
};

class LAndExpEqExp : public LAndExp {
public:
    std::unique_ptr<EqExp> eqExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class LAndExpAnd : public LAndExp {
public:
    std::unique_ptr<LAndExp> lAndExp;
    std::unique_ptr<EqExp> eqExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class LOrExp : public BaseAST {
public:
};

class LOrExpLAndExp : public LOrExp {
public:
    std::unique_ptr<LAndExp> lAndExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class LOrExpOr : public LOrExp {
public:
    std::unique_ptr<LOrExp> lOrExp;
    std::unique_ptr<LAndExp> lAndExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class Decl : public BaseAST {
public:
};

class DeclConstDecl : public Decl {
public:
    std::unique_ptr<ConstDecl> constDecl;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class DeclVarDecl : public Decl {
public:
    std::unique_ptr<VarDecl> varDecl;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class ConstDecl : public BaseAST {
public:
    std::vector<std::unique_ptr<ConstDef>> constDefs;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class ConstDef : public BaseAST {
public:
    std::unique_ptr<ArrIdent> arrIdent;
    std::unique_ptr<ConstInitVal> constInitVal;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class ConstInitVal : public BaseAST {
public:
    std::vector<int> dims;
    std::vector<int> *arr = nullptr;
    int index;
    Result result;
};

class ConstInitValExp : public ConstInitVal {
public:
    std::unique_ptr<ConstExp> constExp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class ConstInitList : public ConstInitVal {
public:
    std::vector<std::unique_ptr<ConstInitVal>> list;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class ConstExp : public BaseAST {
public:
    std::unique_ptr<Exp> exp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class LVal : public BaseAST {
public:
    std::unique_ptr<ArrIdent> arrIdent;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class BlockItem : public BaseAST {
public:
};

class BlockItemDecl : public BlockItem {
public:
    std::unique_ptr<Decl> decl;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class BlockItemStmt : public BlockItem {
public:
    std::unique_ptr<Stmt> stmt;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class VarDecl : public BaseAST {
public:
    std::vector<std::unique_ptr<VarDef>> varDefs;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class VarDef : public BaseAST {
public:
    std::unique_ptr<ArrIdent> arrIdent;
    std::unique_ptr<InitVal> initVal;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class InitVal : public BaseAST {
public:
    std::vector<int> dims;
    std::vector<Result> *arr = nullptr;
    int index;
    Result result;
};

class InitValExp : public InitVal {
public:
    std::unique_ptr<Exp> exp;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class InitList : public InitVal {
public:
    std::vector<std::unique_ptr<InitVal>> list;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class FuncFParam : public BaseAST {
public:
    std::unique_ptr<ArrIdent> arrIdent;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};

class ArrIdent : public BaseAST {
public:
    std::string ident;
    std::vector<std::unique_ptr<Exp>> dimExps;
    std::vector<Result> dims;
    void dump(std::ostream &os) const override;
    void koopa(std::ostream &os) override;
};