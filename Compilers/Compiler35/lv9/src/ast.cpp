#include <memory>
#include <map>
#include <iostream>
#include <utility>
#include "ast.h"

void error(const std::string &s) {
    std::cerr << "error: " << s << std::endl;
    std::exit(1);
}

int Temp::curIndex = 0;
int Label::curIndex = 0;
static Label curLabel;

Label genLabel() {
    if (curLabel == unreachable) {
        return unreachable;
    }
    Label l;
    l.index = Label::curIndex++;
    return l;
}

std::ostream &operator<<(std::ostream &os, const Result &r) {
    std::visit([&os](auto &&r) {
        os << r;
    }, r);
    return os;
}
bool isUndefined(const Result &r) {
    return r.index() == 0;
}
bool isTemp(const Result &r) {
    return r.index() == 1;
}
bool isConst(const Result &r) {
    return r.index() == 2;
}
bool isVar(const Result &r) {
    return r.index() == 3;
}
bool isFunc(const Result &r) {
    return r.index() == 4;
}
template<class T>
static void dumpList(std::ostream &os, const std::vector<std::unique_ptr<T>> &list) {
    os << "[ ";
    for (int i = 0; i < list.size(); i++) {
        if (i > 0) {
            os << ", ";
        }
        if (list[i]) {
            list[i]->dump(os);
        }
    }
    os << " ]";
}

struct Scope {
    int index;
    std::map<std::string, Result> table;
    static int curIndex;
    Scope() : index(curIndex++) {}
    void addConst(std::string ident, Const val) {
        if (table.contains(ident)) {
            error("\"" + ident + "\" already defined");
        }
        table[ident] = val;
    }
    void addVar(std::string ident, Type type) {
        if (table.contains(ident)) {
            error("\"" + ident + "\" already defined");
        }
        table[ident] = Var(ident + '_' + std::to_string(index), type);
    }
    void addFunc(std::string ident, int type) {
        if (table.contains(ident)) {
            error("\"" + ident + "\" already defined");
        }
        table[ident] = Func(type);
    }
    Result getVar(std::string ident) {
        if (!table.contains(ident)) {
            return Undefined();
        }
        auto res = table[ident];
        if (!isVar(res)) {
            error("\"" + ident + "\" is not variable");
        }
        return res;
    }
    Result getResult(std::string ident) {
        if (!table.contains(ident)) {
            return Undefined();
        }
        return table[ident];
    }
};
int Scope::curIndex = 0;

static std::vector<Scope> scopes(1);

bool isGlobal() {
    return scopes.size() == 1;
}

static void addConst(std::string ident, Const val) {
    scopes.back().addConst(ident, val);
}
static void addVar(std::string ident, Type type = Type()) {
    scopes.back().addVar(ident, type);
}
static void addFunc(std::string ident, int type) {
    scopes.back().addFunc(ident, type);
}
static Var getVar(std::string ident) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        auto v = scopes[i].getVar(ident);
        if (isVar(v)) {
            return std::get<Var>(v);
        }
    }
    error("variable \"" + ident + "\" not defined");
    __builtin_unreachable();
}
static Result getResult(std::string ident) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        auto r = scopes[i].getResult(ident);
        if (!isUndefined(r)) {
            return r;
        }
    }
    error("\"" + ident + "\" not defined");
    __builtin_unreachable();
}
static void enterScope() {
    scopes.push_back(Scope());
}
static void exitScope() {
    scopes.pop_back();
}

struct Instr {
    void appendTo(std::ostream &os) {
        if (curLabel == unreachable) {
            return;
        }
        if (!isGlobal()) {
            if (curLabel != nextInstr) {
                os << curLabel << ":\n";
                curLabel = nextInstr;
            }
            os << "    ";
        }
        printLine(os);
    }
    virtual void printLine(std::ostream &os) = 0;
};
struct Store : Instr {
    Result res;
    Result var;
    Store(Result res_, Result var_) : res(res_), var(var_) {}
    void printLine(std::ostream &os) override {
        os << "store " << res << ", " << var << "\n";
    }
};
struct Branch : Instr {
    Result cond;
    Label l1;
    Label l2;
    Branch(Result cond_, Label l1_, Label l2_) : cond(cond_), l1(l1_), l2(l2_) {}
    void printLine(std::ostream &os) override {
        os << "br " << cond << ", " << l1 << ", " << l2 << "\n";
    }
};
struct Jump : Instr {
    Label l;
    Jump(Label l_) : l(l_) {}
    void printLine(std::ostream &os) override {
        os << "jump " << l << "\n";
    }
};
struct Return : Instr {
    std::optional<Result> ret;
    Return() {}
    Return(Result ret_) : ret(ret_) {}
    void printLine(std::ostream &os) override {
        if (ret) {
            os << "ret " << *ret << "\n";
        } else {
            os << "ret\n";
        }
    }
};
struct Load : Instr {
    Temp temp;
    Result var;
    Load(Temp temp_, Result var_) : temp(temp_), var(var_) {}
    void printLine(std::ostream &os) override {
        os << temp << " = load " << var << "\n";
    }
};
struct Binary : Instr {
    std::string op;
    Temp res;
    Result arg1;
    Result arg2;
    Binary(std::string op_, Temp res_, Result arg1_, Result arg2_) : op(op_), res(res_), arg1(arg1_), arg2(arg2_) {}
    void printLine(std::ostream &os) override {
        os << res << " = " << op << " " << arg1 << ", " << arg2 << "\n";
    }
};
struct Alloc : Instr {
    Result res;
    std::optional<Result> init;
    Alloc(Result res_) : res(res_) {}
    Alloc(Result res_, Result init_) : res(res_), init(init_) {}
    void printLine(std::ostream &os) override {
        if (isGlobal()) {
            os << "global ";
        }
        os << res << " = alloc ";
        if (isVar(res)) {
            os << std::get<Var>(res).type;
        } else {
            os << "i32";
        }
        if (init) {
            os << ", " << *init;
        }
        os << "\n";
    }
};
struct GetElemPtr : Instr {
    Temp temp;
    Result arr;
    Result index;
    GetElemPtr(Temp temp_, Result arr_, Result index_) : temp(temp_), arr(arr_), index(index_) {}
    void printLine(std::ostream &os) override {
        os << temp << " = getelemptr " << arr << ", " << index << "\n";
    }
};
struct GetPtr : Instr {
    Temp temp;
    Result arr;
    Result index;
    GetPtr(Temp temp_, Result arr_, Result index_) : temp(temp_), arr(arr_), index(index_) {}
    void printLine(std::ostream &os) override {
        os << temp << " = getptr " << arr << ", " << index << "\n";
    }
};
struct ArrAlloc : Instr {
    Result res;
    std::vector<int> dims;
    std::optional<std::vector<int>> init;
    ArrAlloc(Result res_, std::vector<int> dims_) : res(res_), dims(dims_) {}
    ArrAlloc(Result res_, std::vector<int> dims_, std::vector<int> init_) : res(res_), dims(dims_), init(init_) {}
    void printLine(std::ostream &os) override {
        if (isGlobal()) {
            os << "global ";
        }
        os << res << " = alloc ";
        
        for (int i = 0; i < dims.size(); i++) {
            os << "[";
        }
        os << "i32";
        for (int i = dims.size() - 1; i >= 0; i--) {
            os << ", " << dims[i] << "]";
        }
        
        if (init) {
            os << ", ";
            int cur = 0;
            auto dfs = [&](auto &&self, int i) {
                if (i == dims.size()) {
                    os << (*init)[cur++];
                    return;
                }
                os << "{";
                for (int x = 0; x < dims[i]; x++) {
                    if (x) {
                        os << ", ";
                    }
                    self(self, i + 1);
                }
                os << "}";
            };
            dfs(dfs, 0);
        } else if (isGlobal()) {
            os << ", zeroinit\n";
        }
        os << "\n";
    }
};
struct Call : Instr {
    Result ret;
    std::string ident;
    std::vector<Result> args;
    Call(Result ret_, std::string ident_, std::vector<Result> args_) : ret(ret_), ident(ident_), args(args_) {}
    void printLine(std::ostream &os) override {
        if (!isUndefined(ret)) {
            os << ret << " = ";
        }
        os << "call @" << ident << "(";
        for (int i = 0; i < args.size(); i++) {
            if (i > 0) {
                os << ", ";
            }
            os << args[i];
        }
        os << ")\n";
    }
};

void CompUnit::dump(std::ostream &os) const {
    os << "CompUnit { ";
    dumpList(os, items);
    os << " }";
}
void CompUnit::koopa(std::ostream &os) {
    os << "decl @getint(): i32\n";
    os << "decl @getch(): i32\n";
    os << "decl @getarray(*i32): i32\n";
    os << "decl @putint(i32)\n";
    os << "decl @putch(i32)\n";
    os << "decl @putarray(i32, *i32)\n";
    os << "decl @starttime()\n";
    os << "decl @stoptime()\n";
    
    addFunc("getint", 1);
    addFunc("getch", 1);
    addFunc("getarray", 1);
    addFunc("putint", 0);
    addFunc("putch", 0);
    addFunc("putarray", 0);
    addFunc("starttime", 0);
    addFunc("stoptime", 0);
    
    for (const auto &item : items) {
        item->koopa(os);
    }
}

void GlobalDecl::dump(std::ostream &os) const {
    os << "GlobalDecl { ";
    decl->dump(os);
    os << " }";
}
void GlobalDecl::koopa(std::ostream &os) {
    decl->koopa(os);
}

void GlobalFuncDef::dump(std::ostream &os) const {
    os << "GlobalFuncDef { ";
    funcDef->dump(os);
    os << " }";
}
void GlobalFuncDef::koopa(std::ostream &os) {
    funcDef->koopa(os);
}

void FuncDef::dump(std::ostream &os) const {
    os << "FuncDef { ";
    os << (funcType ? "int" : "void");
    os << ", " << ident << ", ";
    block->dump(os);
    os << " }";
}
void FuncDef::koopa(std::ostream &os) {
    addFunc(ident, funcType);
    os << "fun @" << ident << "(";
    for (int i = 0; i < funcFParams.size(); i++) {
        const auto &funcFParam = funcFParams[i];
        if (i > 0) {
            os << ", ";
        }
        funcFParam->koopa(os);
    }
    os << ")";
    if (funcType == 1) {
        os << ": i32";
    }
    os << " {\n";
    curLabel = genLabel();
    enterScope();
    for (int i = 0; i < funcFParams.size(); i++) {
        const auto &funcFParam = funcFParams[i];
        auto ident = funcFParam->arrIdent->ident;
        auto dims = funcFParam->arrIdent->dims;
        if (dims.empty()) {
            addVar(ident);
        } else {
            std::vector<int> cdims;
            for (int i = 1; i < dims.size(); i++) {
                cdims.push_back(std::get<Const>(dims[i]).value);
            }
            addVar(ident, Type(cdims, true));
        }
        Alloc(getVar(ident)).appendTo(os);
        Store(Var(ident + "_param"), getVar(ident)).appendTo(os);
    }
    block->succ.lNext = nextInstr;
    block->koopa(os);
    if (curLabel != unreachable) {
        if (funcType) {
            Return(Const(0)).appendTo(os);
        } else {
            Return().appendTo(os);
        }
    }
    curLabel = nextInstr;
    exitScope();
    os << "}\n";
}

void Block::dump(std::ostream &os) const {
    os << "Block { ";
    dumpList(os, blockItems);
    os << " }";
}
void Block::koopa(std::ostream &os) {
    enterScope();
    for (int i = 0; i < blockItems.size(); i++) {
        const auto &blockItem = blockItems[i];
        blockItem->succ = succ;
        if (i + 1 < blockItems.size()) {
            blockItem->succ.lNext = nextInstr;
        }
        blockItem->koopa(os);
    }
    exitScope();
}

void StmtAssign::dump(std::ostream &os) const {
    os << "StmtAssign { ";
    lVal->dump(os);
    os << ", ";
    exp->dump(os);
    os << " }";
}
void StmtAssign::koopa(std::ostream &os) {
    exp->succ = succ;
    exp->succ.lNext = nextInstr;
    exp->koopa(os);
    lVal->succ = succ;
    lVal->succ.lNext = nextInstr;
    lVal->koopa(os);
    Store(exp->result, lVal->result).appendTo(os);
}

void StmtExp::dump(std::ostream &os) const {
    os << "StmtExp { ";
    if (exp) {
        exp->dump(os);
    }
    os << " }";
}
void StmtExp::koopa(std::ostream &os) {
    if (exp) {
        exp->succ = succ;
        exp->koopa(os);
    }
}

void StmtBlock::dump(std::ostream &os) const {
    os << "StmtBlock { ";
    if (block) {
        block->dump(os);
    }
    os << " }";
}
void StmtBlock::koopa(std::ostream &os) {
    if (block) {
        block->succ = succ;
        block->koopa(os);
    }
}

void StmtIf::dump(std::ostream &os) const {
    os << "StmtIf { ";
    exp->dump(os);
    os << ", ";
    stmtIf->dump(os);
    os << ", ";
    if (stmtElse) {
        stmtElse->dump(os);
    }
    os << " }";
}
void StmtIf::koopa(std::ostream &os) {
    if (stmtElse) {
        auto l1 = genLabel();
        auto l2 = genLabel();
        auto l3 = succ.lNext;
        if (l3 == nextInstr) {
            l3 = genLabel();
        }
        exp->succ = succ;
        exp->succ.lNext = nextInstr;
        exp->koopa(os);
        Branch(exp->result, l1, l2).appendTo(os);
        curLabel = l1;
        stmtIf->succ = succ;
        stmtIf->succ.lNext = l3;
        stmtIf->koopa(os);
        Jump(l3).appendTo(os);
        curLabel = l2;
        stmtElse->succ = succ;
        stmtElse->succ.lNext = l3;
        stmtElse->koopa(os);
        if (curLabel == nextInstr || curLabel == l2) {
            Jump(l3).appendTo(os);
        }
        curLabel = unreachable;
        if (succ.lNext == nextInstr) {
            curLabel = l3;
        }
    } else {
        auto l1 = genLabel();
        auto l2 = succ.lNext;
        if (l2 == nextInstr) {
            l2 = genLabel();
        }
        exp->succ = succ;
        exp->succ.lNext = nextInstr;
        exp->koopa(os);
        Branch(exp->result, l1, l2).appendTo(os);
        curLabel = l1;
        stmtIf->succ = succ;
        stmtIf->succ.lNext = l2;
        stmtIf->koopa(os);
        if (curLabel == nextInstr || curLabel == l1) {
            Jump(l2).appendTo(os);
        }
        curLabel = unreachable;
        if (succ.lNext == nextInstr) {
            curLabel = l2;
        }
    }
}

void StmtWhile::dump(std::ostream &os) const {
    os << "StmtWhile { ";
    exp->dump(os);
    os << ", ";
    stmt->dump(os);
    os << " }";
}
void StmtWhile::koopa(std::ostream &os) {
    auto l1 = genLabel();
    auto l2 = genLabel();
    auto l3 = succ.lNext;
    if (l3 == nextInstr) {
        l3 = genLabel();
    }
    Jump(l1).appendTo(os);
    curLabel = l1;
    exp->succ = succ;
    exp->succ.lNext = nextInstr;
    exp->koopa(os);
    Branch(exp->result, l2, l3).appendTo(os);
    curLabel = l2;
    stmt->succ = succ;
    stmt->succ.lNext = l1;
    stmt->succ.lBreak = l3;
    stmt->succ.lContinue = l1;
    stmt->koopa(os);
    if (curLabel == nextInstr || curLabel == l2) {
        Jump(l1).appendTo(os);
    }
    curLabel = unreachable;
    if (succ.lNext == nextInstr) {
        curLabel = l3;
    }
}

void StmtBreak::dump(std::ostream &os) const {
    os << "StmtBreak";
}
void StmtBreak::koopa(std::ostream &os) {
    if (succ.lBreak == illegal) {
        error("break must be inside a loop");
    }
    Jump(succ.lBreak).appendTo(os);
    curLabel = unreachable;
}

void StmtContinue::dump(std::ostream &os) const {
    os << "StmtContinue";
}
void StmtContinue::koopa(std::ostream &os) {
    if (succ.lBreak == illegal) {
        error("continue must be inside a loop");
    }
    Jump(succ.lContinue).appendTo(os);
    curLabel = unreachable;
}

void StmtReturn::dump(std::ostream &os) const {
    os << "StmtReturn { ";
    if (exp) {
        exp->dump(os);
    }
    os << " }";
}
void StmtReturn::koopa(std::ostream &os) {
    if (exp) {
        exp->succ = succ;
        exp->succ.lNext = nextInstr;
        exp->koopa(os);
        Return(exp->result).appendTo(os);
    } else {
        Return().appendTo(os);
    }
    curLabel = unreachable;
}

void Exp::dump(std::ostream &os) const {
    os << "Exp { ";
    lOrExp->dump(os);
    os << " }";
}
void Exp::koopa(std::ostream &os) {
    lOrExp->succ = succ;
    lOrExp->koopa(os);
    result = lOrExp->result;
}

void PrimaryExpParentheses::dump(std::ostream &os) const {
    os << "PrimaryExpParentheses { ";
    exp->dump(os);
    os << " }";
}
void PrimaryExpParentheses::koopa(std::ostream &os) {
    exp->succ = succ;
    exp->koopa(os);
    result = exp->result;
}

void PrimaryExpNumber::dump(std::ostream &os) const {
    os << "PrimaryExpNumber { ";
    os << number;
    os << " }";
}
void PrimaryExpNumber::koopa(std::ostream &os) {
    result = Const(number);
}

void PrimaryExpLVal::dump(std::ostream &os) const {
    os << "PrimaryExpLVal { ";
    lVal->dump(os);
    os << " }";
}
void PrimaryExpLVal::koopa(std::ostream &os) {
    lVal->succ = succ;
    lVal->succ.lNext = nextInstr;
    lVal->koopa(os);
    if (!isConst(lVal->result)) {
        auto res = getResult(lVal->arrIdent->ident);
        auto temp = Temp();
        if (isVar(res) && std::get<Var>(res).type.dims.size() + std::get<Var>(res).type.ptr != lVal->arrIdent->dims.size() && (!std::get<Var>(res).type.ptr || !lVal->arrIdent->dims.empty())) {
            GetElemPtr(temp, lVal->result, Const(0)).appendTo(os);
            result = temp;
        } else {
            Load(temp, lVal->result).appendTo(os);
            result = temp;
        }
    } else {
        result = lVal->result;
    }
}

void UnaryExpPrimary::dump(std::ostream &os) const {
    os << "UnaryExpPrimary { ";
    primaryExp->dump(os);
    os << " }";
}
void UnaryExpPrimary::koopa(std::ostream &os) {
    primaryExp->succ = succ;
    primaryExp->koopa(os);
    result = primaryExp->result;
}

void UnaryExpUnaryOp::dump(std::ostream &os) const {
    os << "UnaryExpUnaryOp { ";
    unaryOp->dump(os);
    os << ", ";
    unaryExp->dump(os);
    os << " }";
}
void UnaryExpUnaryOp::koopa(std::ostream &os) {
    unaryExp->succ = succ;
    unaryExp->succ.lNext = nextInstr;
    unaryExp->koopa(os);
    unaryOp->succ = succ;
    unaryOp->arg = unaryExp->result;
    unaryOp->koopa(os);
    result = unaryOp->result;
}

void UnaryExpCall::dump(std::ostream &os) const {
    os << "UnaryExpCall { ";
    os << ident;
    os << ",";
    dumpList(os, args);
    os << " }";
}
void UnaryExpCall::koopa(std::ostream &os) {
    for (const auto &exp : args) {
        exp->koopa(os);
    }
    auto func = getResult(ident);
    if (!isFunc(func)) {
        error(ident + " is not function");
    }
    int type = std::get<Func>(func).type;
    if (type) {
        result = Temp();
    }
    std::vector<Result> argres;
    for (const auto &exp : args) {
        argres.push_back(exp->result);
    }
    Call(result, ident, argres).appendTo(os);
}

void UnaryOpPos::dump(std::ostream &os) const {
    os << "UnaryOpPos";
}
void UnaryOpPos::koopa(std::ostream &os) {
    result = arg;
}

void UnaryOpNeg::dump(std::ostream &os) const {
    os << "UnaryOpNeg";
}
void UnaryOpNeg::koopa(std::ostream &os) {
    if (isConst(arg)) {
        result = Const(-std::get<Const>(arg).value);
        return;
    }
    auto temp = Temp();
    Binary("sub", temp, Const(0), arg).appendTo(os);
    result = temp;
}

void UnaryOpNot::dump(std::ostream &os) const {
    os << "UnaryOpNot";
}
void UnaryOpNot::koopa(std::ostream &os) {
    if (isConst(arg)) {
        result = Const(!std::get<Const>(arg).value);
        return;
    }
    auto temp = Temp();
    Binary("eq", temp, Const(0), arg).appendTo(os);
    result = temp;
}

void AddExpMulExp::dump(std::ostream &os) const {
    os << "AddExpMulExp { ";
    mulExp->dump(os);
    os << " }";
}
void AddExpMulExp::koopa(std::ostream &os) {
    mulExp->succ = succ;
    mulExp->koopa(os);
    result = mulExp->result;
}

void AddExpAddOp::dump(std::ostream &os) const {
    os << "AddExpAddOp { ";
    addExp->dump(os);
    os << ", ";
    addOp->dump(os);
    os << ", ";
    mulExp->dump(os);
    os << " }";
}
void AddExpAddOp::koopa(std::ostream &os) {
    addExp->succ = succ;
    addExp->succ.lNext = nextInstr;
    addExp->koopa(os);
    mulExp->succ = succ;
    mulExp->succ.lNext = nextInstr;
    mulExp->koopa(os);
    addOp->succ = succ;
    addOp->arg1 = addExp->result;
    addOp->arg2 = mulExp->result;
    addOp->koopa(os);
    result = addOp->result;
}

void MulExpUnaryExp::dump(std::ostream &os) const {
    os << "MulExpUnaryExp { ";
    unaryExp->dump(os);
    os << " }";
}
void MulExpUnaryExp::koopa(std::ostream &os) {
    unaryExp->succ = succ;
    unaryExp->koopa(os);
    result = unaryExp->result;
}

void MulExpMulOp::dump(std::ostream &os) const {
    os << "MulExpMulOp { ";
    mulExp->dump(os);
    os << ", ";
    mulOp->dump(os);
    os << ", ";
    unaryExp->dump(os);
    os << " }";
}
void MulExpMulOp::koopa(std::ostream &os) {
    mulExp->succ = succ;
    mulExp->succ.lNext = nextInstr;
    mulExp->koopa(os);
    unaryExp->succ = succ;
    unaryExp->succ.lNext = nextInstr;
    unaryExp->koopa(os);
    mulOp->succ = succ;
    mulOp->arg1 = mulExp->result;
    mulOp->arg2 = unaryExp->result;
    mulOp->koopa(os);
    result = mulOp->result;
}

void AddOpAdd::dump(std::ostream &os) const {
    os << "AddOpAdd";
}
void AddOpAdd::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value + std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("add", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void AddOpSub::dump(std::ostream &os) const {
    os << "AddOpSub";
}
void AddOpSub::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value - std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("sub", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void MulOpMul::dump(std::ostream &os) const {
    os << "MulOpMul";
}
void MulOpMul::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value * std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("mul", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void MulOpDiv::dump(std::ostream &os) const {
    os << "MulOpDiv";
}
void MulOpDiv::koopa(std::ostream &os) {
    if (isConst(arg2) && std::get<Const>(arg2).value == 0) {
        error("divided by zero");
    }
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value / std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("div", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void MulOpMod::dump(std::ostream &os) const {
    os << "MulOpMod";
}
void MulOpMod::koopa(std::ostream &os) {
    if (isConst(arg2) && std::get<Const>(arg2).value == 0) {
        error("divided by zero");
    }
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value % std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("mod", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void RelExpAddExp::dump(std::ostream &os) const {
    os << "RelExpAddExp { ";
    addExp->dump(os);
    os << " }";
}
void RelExpAddExp::koopa(std::ostream &os) {
    addExp->succ = succ;
    addExp->koopa(os);
    result = addExp->result;
}

void RelExpRelOp::dump(std::ostream &os) const {
    os << "RelExpRelOp { ";
    relExp->dump(os);
    os << ", ";
    relOp->dump(os);
    os << ", ";
    addExp->dump(os);
    os << " }";
}
void RelExpRelOp::koopa(std::ostream &os) {
    relExp->succ = succ;
    relExp->succ.lNext = nextInstr;
    relExp->koopa(os);
    addExp->succ = succ;
    addExp->succ.lNext = nextInstr;
    addExp->koopa(os);
    relOp->succ = succ;
    relOp->arg1 = relExp->result;
    relOp->arg2 = addExp->result;
    relOp->koopa(os);
    result = relOp->result;
}

void RelOpLt::dump(std::ostream &os) const {
    os << "RelOpLt";
}
void RelOpLt::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value < std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("lt", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void RelOpGt::dump(std::ostream &os) const {
    os << "RelOpGt";
}
void RelOpGt::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value > std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("gt", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void RelOpLeq::dump(std::ostream &os) const {
    os << "RelOpLeq";
}
void RelOpLeq::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value <= std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("le", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void RelOpGeq::dump(std::ostream &os) const {
    os << "RelOpGeq";
}
void RelOpGeq::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value >= std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("ge", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void EqExpRelExp::dump(std::ostream &os) const {
    os << "EqExpRelExp { ";
    relExp->dump(os);
    os << " }";
}
void EqExpRelExp::koopa(std::ostream &os) {
    relExp->succ = succ;
    relExp->koopa(os);
    result = relExp->result;
}

void EqExpEqOp::dump(std::ostream &os) const {
    os << "EqExpEqOp { ";
    eqExp->dump(os);
    os << ", ";
    eqOp->dump(os);
    os << ", ";
    relExp->dump(os);
    os << " }";
}
void EqExpEqOp::koopa(std::ostream &os) {
    eqExp->succ = succ;
    eqExp->succ.lNext = nextInstr;
    eqExp->koopa(os);
    relExp->succ = succ;
    relExp->succ.lNext = nextInstr;
    relExp->koopa(os);
    eqOp->succ = succ;
    eqOp->arg1 = eqExp->result;
    eqOp->arg2 = relExp->result;
    eqOp->koopa(os);
    result = eqOp->result;
}

void EqOpEq::dump(std::ostream &os) const {
    os << "EqOpEq";
}
void EqOpEq::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value == std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("eq", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void EqOpNeq::dump(std::ostream &os) const {
    os << "EqOpNeq";
}
void EqOpNeq::koopa(std::ostream &os) {
    if (isConst(arg1) && isConst(arg2)) {
        result = Const(std::get<Const>(arg1).value != std::get<Const>(arg2).value);
        return;
    }
    auto temp = Temp();
    Binary("ne", temp, arg1, arg2).appendTo(os);
    result = temp;
}

void LAndExpEqExp::dump(std::ostream &os) const {
    os << "LAndExpEqExp { ";
    eqExp->dump(os);
    os << " }";
}
void LAndExpEqExp::koopa(std::ostream &os) {
    eqExp->succ = succ;
    eqExp->koopa(os);
    result = eqExp->result;
}

void LAndExpAnd::dump(std::ostream &os) const {
    os << "LAndExpAnd { ";
    lAndExp->dump(os);
    os << ", And , ";
    eqExp->dump(os);
    os << " }";
}
void LAndExpAnd::koopa(std::ostream &os) {
    lAndExp->succ = succ;
    lAndExp->succ.lNext = nextInstr;
    lAndExp->koopa(os);
    auto arg1 = lAndExp->result;
    if (isConst(arg1)) {
        if (!std::get<Const>(arg1).value) {
            result = Const(0);
            return;
        }
        eqExp->succ = succ;
        eqExp->succ.lNext = nextInstr;
        eqExp->koopa(os);
        auto arg2 = eqExp->result;
        if (isConst(arg2)) {
            result = Const(std::get<Const>(arg2).value != 0);
        } else {
            auto temp = Temp();
            Binary("ne", temp, Const(0), arg2).appendTo(os);
            result = temp;
        }
    } else {
        auto temp = Temp();
        auto l1 = genLabel();
        auto l2 = genLabel();
        Alloc(temp).appendTo(os);
        Store(Const(0), temp).appendTo(os);
        Branch(arg1, l1, l2).appendTo(os);
        curLabel = l1;
        eqExp->succ = succ;
        eqExp->succ.lNext = nextInstr;
        eqExp->koopa(os);
        auto arg2 = eqExp->result;
        auto arg2nz = Temp();
        Binary("ne", arg2nz, Const(0), arg2).appendTo(os);
        Store(arg2nz, temp).appendTo(os);
        Jump(l2).appendTo(os);
        curLabel = l2;
        auto res = Temp();
        Load(res, temp).appendTo(os);
        result = res;
    }
}

void LOrExpLAndExp::dump(std::ostream &os) const {
    os << "LOrExpLAndExp { ";
    lAndExp->dump(os);
    os << " }";
}
void LOrExpLAndExp::koopa(std::ostream &os) {
    lAndExp->succ = succ;
    lAndExp->koopa(os);
    result = lAndExp->result;
}

void LOrExpOr::dump(std::ostream &os) const {
    os << "LOrExpOr { ";
    lOrExp->dump(os);
    os << ", Or , ";
    lAndExp->dump(os);
    os << " }";
}
void LOrExpOr::koopa(std::ostream &os) {
    lOrExp->succ = succ;
    lOrExp->succ.lNext = nextInstr;
    lOrExp->koopa(os);
    auto arg1 = lOrExp->result;
    if (isConst(arg1)) {
        if (std::get<Const>(arg1).value) {
            result = Const(1);
            return;
        }
        lAndExp->succ = succ;
        lAndExp->succ.lNext = nextInstr;
        lAndExp->koopa(os);
        auto arg2 = lAndExp->result;
        if (isConst(arg2)) {
            result = Const(std::get<Const>(arg2).value != 0);
        } else {
            auto temp = Temp();
            Binary("ne", temp, Const(0), arg2).appendTo(os);
            result = temp;
        }
    } else {
        auto temp = Temp();
        auto l1 = genLabel();
        auto l2 = genLabel();
        Alloc(temp).appendTo(os);
        Store(Const(1), temp).appendTo(os);
        Branch(arg1, l2, l1).appendTo(os);
        curLabel = l1;
        lAndExp->succ = succ;
        lAndExp->succ.lNext = nextInstr;
        lAndExp->koopa(os);
        auto arg2 = lAndExp->result;
        auto arg2nz = Temp();
        Binary("ne", arg2nz, Const(0), arg2).appendTo(os);
        Store(arg2nz, temp).appendTo(os);
        Jump(l2).appendTo(os);
        curLabel = l2;
        auto res = Temp();
        Load(res, temp).appendTo(os);
        result = res;
    }
}

void DeclConstDecl::dump(std::ostream &os) const {
    os << "DeclConstDecl { ";
    constDecl->dump(os);
    os << " }";
}
void DeclConstDecl::koopa(std::ostream &os) {
    constDecl->succ = succ;
    constDecl->koopa(os);
}

void DeclVarDecl::dump(std::ostream &os) const {
    os << "DeclVarDecl { ";
    varDecl->dump(os);
    os << " }";
}
void DeclVarDecl::koopa(std::ostream &os) {
    varDecl->succ = succ;
    varDecl->koopa(os);
}

void ConstDecl::dump(std::ostream &os) const {
    os << "ConstDecl { ";
    dumpList(os, constDefs);
    os << " }";
}
void ConstDecl::koopa(std::ostream &os) {
    for (int i = 0; i < constDefs.size(); i++) {
        const auto &constDef = constDefs[i];
        if (i + 1 < constDefs.size()) {
            constDef->succ = succ;
            constDef->succ.lNext = nextInstr;
        } else {
            constDef->succ = succ;
        }
        constDef->koopa(os);
    }
}

void ConstDef::dump(std::ostream &os) const {
    os << "ConstDef { ";
    arrIdent->dump(os);
    os << ", ";
    constInitVal->dump(os);
    os << " }";
}
void ConstDef::koopa(std::ostream &os) {
    arrIdent->koopa(os);
    std::vector<int> dims;
    for (auto d : arrIdent->dims) {
        dims.push_back(std::get<Const>(d).value);
    }
    if (dims.empty()) {
        constInitVal->succ = succ;
        constInitVal->koopa(os);
        addConst(arrIdent->ident, std::get<Const>(constInitVal->result));
    } else {
        constInitVal->succ = succ;
        addVar(arrIdent->ident, Type(dims));
        constInitVal->dims = dims;
        
        int size = 1;
        for (auto d : dims) {
            size *= d;
        }
        std::vector<int> arr(size);
        constInitVal->arr = &arr;
        constInitVal->index = 0;
        constInitVal->koopa(os);
        if (isGlobal()) {
            ArrAlloc(getVar(arrIdent->ident), dims, arr).appendTo(os);
        } else {
            ArrAlloc(getVar(arrIdent->ident), dims).appendTo(os);
            int cur = 0;
            auto dfs = [&](auto &&self, int i, Result ptr) {
                if (i == dims.size()) {
                    Store(Const(arr[cur++]), ptr).appendTo(os);
                    return;
                }
                for (int x = 0; x < dims[i]; x++) {
                    auto temp = Temp();
                    GetElemPtr(temp, ptr, Const(x)).appendTo(os);
                    self(self, i + 1, temp);
                }
            };
            dfs(dfs, 0, getVar(arrIdent->ident));
        }
    }
}

void ConstInitValExp::dump(std::ostream &os) const {
    os << "ConstInitValExp { ";
    constExp->dump(os);
    os << " }";
}
void ConstInitValExp::koopa(std::ostream &os) {
    constExp->succ = succ;
    constExp->koopa(os);
    result = constExp->result;
    if (arr) {
        (*arr)[index] = std::get<Const>(result).value;
        index++;
    }
}

void ConstInitList::dump(std::ostream &os) const {
    os << "ConstInitList { ";
    dumpList(os, list);
    os << " }";
}
void ConstInitList::koopa(std::ostream &os) {
    int size = 1;
    for (auto d : dims) {
        size *= d;
    }
    int end = index + size;
    for (const auto &val : list) {
        val->succ = succ;
        int k = dims.size();
        int prod = 1;
        while (k > 1 && index % (prod * dims[k - 1]) == 0) {
            k--;
            prod *= dims[k];
        }
        val->dims = std::vector(dims.begin() + k, dims.end());
        val->arr = arr;
        val->index = index;
        val->koopa(os);
        index = val->index;
    }
    index = end;
}

void ConstExp::dump(std::ostream &os) const {
    os << "ConstExp { ";
    exp->dump(os);
    os << " }";
}
void ConstExp::koopa(std::ostream &os) {
    exp->succ = succ;
    exp->koopa(os);
    if (!isConst(exp->result)) {
        error("not const expression");
    }
    result = exp->result;
}

void LVal::dump(std::ostream &os) const {
    os << "LVal { ";
    arrIdent->dump(os);
    os << " }";
}
void LVal::koopa(std::ostream &os) {
    arrIdent->koopa(os);
    auto res = getResult(arrIdent->ident);
    if (isConst(res)) {
        result = res;
        return;
    }
    auto dims = arrIdent->dims;
    for (int i = 0; i < dims.size(); i++) {
        if (i == 0 && std::get<Var>(res).type.ptr) {
            auto temp = Temp();
            Load(temp, res).appendTo(os);
            res = temp;
            temp = Temp();
            GetPtr(temp, res, dims[i]).appendTo(os);
            res = temp;
        } else {
            auto temp = Temp();
            GetElemPtr(temp, res, dims[i]).appendTo(os);
            res = temp;
        }
    }
    result = res;
}

void BlockItemDecl::dump(std::ostream &os) const {
    os << "BlockItemDecl { ";
    decl->dump(os);
    os << " }";
}
void BlockItemDecl::koopa(std::ostream &os) {
    decl->succ = succ;
    decl->koopa(os);
}

void BlockItemStmt::dump(std::ostream &os) const {
    os << "BlockItemStmt { ";
    stmt->dump(os);
    os << " }";
}
void BlockItemStmt::koopa(std::ostream &os) {
    stmt->succ = succ;
    stmt->koopa(os);
}

void VarDecl::dump(std::ostream &os) const {
    os << "VarDecl { ";
    dumpList(os, varDefs);
    os << " }";
}
void VarDecl::koopa(std::ostream &os) {
    for (int i = 0; i < varDefs.size(); i++) {
        const auto &varDef = varDefs[i];
        if (i + 1 < varDefs.size()) {
            varDef->succ = succ;
            varDef->succ.lNext = nextInstr;
        } else {
            varDef->succ = succ;
        }
        varDef->koopa(os);
    }
}

void VarDef::dump(std::ostream &os) const {
    os << "VarDef { ";
    arrIdent->dump(os);
    os << ", ";
    if (initVal) {
        initVal->dump(os);
    }
    os << " }";
}
void VarDef::koopa(std::ostream &os) {
    arrIdent->koopa(os);
    std::vector<int> dims;
    for (auto d : arrIdent->dims) {
        dims.push_back(std::get<Const>(d).value);
    }
    addVar(arrIdent->ident, Type(dims));
    if (dims.empty()) {
        if (!isGlobal()) {
            Alloc(getVar(arrIdent->ident)).appendTo(os);
            if (initVal) {
                initVal->succ = succ;
                initVal->succ.lNext = nextInstr;
                initVal->koopa(os);
                Store(initVal->result, getVar(arrIdent->ident)).appendTo(os);
            }
        } else {
            Result init = Const(0);
            if (initVal) {
                initVal->succ = succ;
                initVal->succ.lNext = nextInstr;
                initVal->koopa(os);
                init = initVal->result;
            }
            if (!isConst(init)) {
                error("global variable must be initialized with constexpr");
            }
            Alloc(getVar(arrIdent->ident), init).appendTo(os);
        }
    } else {
        if (!initVal) {
            ArrAlloc(getVar(arrIdent->ident), dims).appendTo(os);
        } else {
            initVal->succ = succ;
            initVal->dims = dims;
            
            int size = 1;
            for (auto d : dims) {
                size *= d;
            }
            std::vector<Result> arr(size, Const(0));
            initVal->arr = &arr;
            initVal->index = 0;
            initVal->koopa(os);
            
            if (isGlobal()) {
                std::vector<int> num;
                for (auto r : arr) {
                    num.push_back(std::get<Const>(r).value);
                }
                ArrAlloc(getVar(arrIdent->ident), dims, num).appendTo(os);
            } else {
                ArrAlloc(getVar(arrIdent->ident), dims).appendTo(os);
                int cur = 0;
                auto dfs = [&](auto &&self, int i, Result ptr) {
                    if (i == dims.size()) {
                        Store(arr[cur++], ptr).appendTo(os);
                        return;
                    }
                    for (int x = 0; x < dims[i]; x++) {
                        auto temp = Temp();
                        GetElemPtr(temp, ptr, Const(x)).appendTo(os);
                        self(self, i + 1, temp);
                    }
                };
                dfs(dfs, 0, getVar(arrIdent->ident));
            }
        }
    }
}

void InitValExp::dump(std::ostream &os) const {
    os << "InitValExp { ";
    exp->dump(os);
    os << " }";
}
void InitValExp::koopa(std::ostream &os) {
    exp->succ = succ;
    exp->koopa(os);
    result = exp->result;
    if (arr) {
        (*arr)[index] = result;
        index++;
    }
}

void InitList::dump(std::ostream &os) const {
    os << "InitList { ";
    dumpList(os, list);
    os << " }";
}
void InitList::koopa(std::ostream &os) {
    int size = 1;
    for (auto d : dims) {
        size *= d;
    }
    int end = index + size;
    for (const auto &val : list) {
        val->succ = succ;
        int k = dims.size();
        int prod = 1;
        while (k > 1 && index % (prod * dims[k - 1]) == 0) {
            k--;
            prod *= dims[k];
        }
        val->dims = std::vector(dims.begin() + k, dims.end());
        val->arr = arr;
        val->index = index;
        val->koopa(os);
        index = val->index;
    }
    index = end;
}

void FuncFParam::dump(std::ostream &os) const {
    os << "FuncFParam { ";
    arrIdent->dump(os);
    os << " }";
}
void FuncFParam::koopa(std::ostream &os) {
    arrIdent->koopa(os);
    os << "@" << arrIdent->ident << "_param: ";
    auto dims = arrIdent->dims;
    if (dims.empty()) {
        os << "i32";
    } else {
        os << "*";
        for (int i = 1; i < dims.size(); i++) {
            os << "[";
        }
        os << "i32";
        for (int i = dims.size() - 1; i >= 1; i--) {
            os << ", " << std::get<Const>(dims[i]).value << "]";
        }
    }
}

void ArrIdent::dump(std::ostream &os) const {
    os << "ArrIdent { ";
    os << ident;
    os << ", ";
    dumpList(os, dimExps);
    os << " }";
}
void ArrIdent::koopa(std::ostream &os) {
    for (const auto &dimExp : dimExps) {
        if (dimExp) {
            dimExp->koopa(os);
            dims.push_back(dimExp->result);
        } else {
            dims.push_back(Undefined());
        }
    }
}