#pragma once
#include<string>
#include<memory>
#include<iostream>
#include<vector>
#include<map>

using namespace std;

//最终照搬了这种写法，用Int来做省去不少麻烦

#define KOOPA_MODE 0
#define RISCV_MODE 1

#define NoOp 0
#define Invert 1
#define EqZero 2
#define Add 3
#define Sub 4
#define Mul 5
#define Div 6
#define Mod 7
#define Less 8
#define Greater 9
#define LEq 10
#define GEq 11
#define Equal 12
#define NotEqual 13
#define And 14
#define Or 15
#define NotEqZero 16

#define VarDecl 0
#define ConstDecl 1
#define ParamDecl 2
#define ArrayDecl 3

#define TypeInt 0
#define TypeVoid 1
#define TypePointer 2  //数组

#define Assign 0
#define Return 1
#define Break 2
#define Continue 3
#define Other 4

/*声明区*/

class BaseAST;
class Expr;
class Stmt;
class JumpStmt;
class InitVal;
/*符号表区*/

class Symbol {
public:
    int declType, type, value, len = 0;
    bool is_alloc = false;
    string name;

    Symbol(int dtp, string n, int tp = TypeInt, int v = 0) : declType(dtp), type(tp), value(v), name(n) {}
};

typedef map<string,Symbol*> SymbolMap;
static vector<SymbolMap> symbolTable(1);

static void cleanSymbolMap(SymbolMap &map) { for (auto &iter : map) delete iter.second; }

static Symbol* findSymbol(string &id) {
    for (int i = symbolTable.size() - 1; i >= 0; i--)
        if (symbolTable[i].count(id)) return symbolTable[i][id];
    throw runtime_error("No such symbol in table");
    return nullptr;
}



/*函数表区*/

class FuncInfo {
public:
    int func_type;
    vector<int> param_type;

    FuncInfo(int type = 0, vector<int> param = {}) : func_type(type), param_type(param) {}
    FuncInfo(const FuncInfo &a) : func_type(a.func_type), param_type(a.param_type) {}
};

static map<string,FuncInfo*> func_table;


/*辅助函数区*/


static int var_idx_global=0;
static int genVarIdx() { return var_idx_global++; }
static int block_idx_global=0;
static int genBlockIdx() { return block_idx_global++; }
static int jump_statement_idx_global=0;
static int genJumpStmtIdx() { return jump_statement_idx_global++; }
static int result_idx_global = 0;
static int genResultIdx() { return result_idx_global++; }
static int ptr_id=0;
static int genPtrId() { return ptr_id++; }
static int current_block_idx;
static bool has_break_point = false;
static vector<int> while_stmt_idx_global;
static void genBlock(const char *name, int id) { printf("%%%s_%d:\n", name, id), has_break_point = false; }


/*因为依赖关系，只能先声明最后再定义的辅助函数*/
static void printConstInit(InitVal *init,vector<int> offset);
static void printVarInit(InitVal *init,vector<int> offset,string id,int now_num);
static void printType(int type,vector<int> offset);
static bool isJumpInstr(BaseAST *s);


/*各个节点的定义，其附属函数和特征*/

class BlockItems
{
    public:
        vector<BaseAST*> vec;
};


class BaseAST
{
    public:
        virtual ~BaseAST() = default;
        virtual void print_ir(){};
        virtual void genInstr(int instrType){};
        virtual void base_print(){};
        virtual int eval(){return 0;}
        virtual void load_array(){}

        virtual bool is_number(){return false;};
        virtual bool is_boolean(){return false;}
        virtual bool is_decls(){return false;}
        virtual bool is_statement(){return false;}
        virtual bool is_expr(){return false;}
        virtual bool is_var(){return false;}
        virtual bool is_block(){return false;}
        virtual bool is_empty_block(){return false;}
        virtual bool is_jump_statement(){return false;}
        virtual bool is_initval(){return false;}
        
};

class Var : public BaseAST {
public:
    string id;
    vector<BaseAST*> offset;
    int ptr_id = -1, temp_var = -1;

    explicit Var(string a) : id(a) {}

    virtual void print_ir() {
        Symbol *symbol = findSymbol(id);
        if (symbol->declType == VarDecl) {
            printf("  %%%d = load %s\n", temp_var= genVarIdx(), symbol->name.c_str());
        } else if (symbol->declType == ArrayDecl) {
            load_array();
            printf("  %%%d = load %%ptr_%d\n", temp_var= genVarIdx(), ptr_id);
        }
    }

    virtual void load_array() {
        if (offset.empty()) return;
        Symbol *symbol = findSymbol(id);
        int old_ptr_id;
        ptr_id = genPtrId();
        offset[0]->print_ir();
        if (symbol->type != TypePointer)
            printf("  %%ptr_%d = getelemptr %s, ", ptr_id, symbol->name.c_str());
        else {
            printf("  %%ptr_%d = load %s\n", ptr_id, symbol->name.c_str());
            old_ptr_id = ptr_id, ptr_id = genPtrId();
            printf("  %%ptr_%d = getptr %%ptr_%d, ", ptr_id, old_ptr_id);
        }
        offset[0]->base_print(), printf("\n");
        for (size_t i = 1; i < offset.size(); i++) {
            old_ptr_id = ptr_id, ptr_id = genPtrId();
            offset[i]->print_ir();
            printf("  %%ptr_%d = getelemptr %%ptr_%d, ", ptr_id, old_ptr_id);
            offset[i]->base_print(), printf("\n");
        }
    }

    virtual void base_print() {
        Symbol *symbol = findSymbol(id);
        if (symbol->declType == ConstDecl)
            printf("%d", symbol->value);
        else if (symbol->declType == VarDecl)
            temp_var != -1 ? printf("%%%d", temp_var) : printf("%s", symbol->name.c_str());
        else if (symbol->declType == ArrayDecl) {
            if (offset.empty()) printf("%s", symbol->name.c_str());
            else temp_var != -1 ? printf("%%%d", temp_var) : printf("%%ptr_%d", ptr_id);
        }
    }

    virtual int eval() { return findSymbol(id)->value; }
    virtual bool is_var() { return true; }
};


class InitVal : public BaseAST {
public:
    vector<BaseAST*> inits;
    virtual void print_ir() {
        for (auto &init : inits)
            if (init) init->print_ir();
    }
    virtual bool is_initval() { return true; }
};

class Number : public BaseAST {
public:
    int num;
    virtual void print_ir() {}
    virtual void base_print() { printf("%d", num); }
    virtual bool is_number() { return true; }
    virtual int eval() { return num; }
};


class DeclareDef : public BaseAST {
public:
    string name;
    BaseAST *init;
    int declType = VarDecl, type = TypeInt;
    vector<BaseAST*> offset;

    DeclareDef(string n, BaseAST *e = NULL) : name(n), init(e) {}

    virtual void print_ir() {
        Symbol *symbol;
        int depth = symbolTable.size() - 1;
        string id = "@_" + to_string(current_block_idx) + name;

        if (declType == ConstDecl && offset.empty()) 
            symbol = new Symbol(ConstDecl, id, type, init->eval());

        else if (declType == VarDecl && offset.empty()) {
            symbol = new Symbol(VarDecl, id, type);
            if (!(symbolTable[depth].count(name) && symbolTable[depth][name]->is_alloc))
                printf("  %s = alloc %s\n", id.c_str(), type == TypeInt ? "i32" : ""), symbol->is_alloc = true;
            if (init) {
                init->print_ir();
                printf("  store "), init->base_print(), printf(", %s\n", id.c_str());
                symbol->value = init->eval();
            }
        }

        else if (declType == ParamDecl) {
            symbol = new Symbol(type == TypePointer ? ArrayDecl : VarDecl, id, type);
            symbol->len = type == TypePointer ? offset.size() : 0;
            printf("  %s = alloc ", id.c_str());
            vector<int> temp(offset.size());
            for (int i = 0; i < offset.size(); i++) temp[i] = offset[i]->eval();
            if (type == TypePointer) printf("*");
            printType(type, temp), printf("\n  store @%s, %s\n", name.c_str(), id.c_str());
        }

        else if (!offset.empty()) {
            vector<int> temp(offset.size()), vec = {1};
            for (int i = 0; i < offset.size(); i++) temp[i] = offset[i]->eval();
            symbol = new Symbol(ArrayDecl, id, type), symbol->len = offset.size();
            printf("  %s = alloc ", id.c_str());
            printType(type, temp), printf("\n  store zeroinit, %s\n", id.c_str());
            if (init) {
                int num = 1;
                for (int j = int(temp.size()) - 1; j >= 0; j--) vec.push_back(num *= temp[j]);
                vec.pop_back();
                printVarInit((InitVal*)init, vec, id, 0);
            }
        }

        symbolTable[depth][name] = symbolTable[depth].count(name) ? (symbol->is_alloc = symbolTable[depth][name]->is_alloc, symbol) : symbol;
    }
};

class Decls : public BaseAST {
public:
    vector<DeclareDef*> defs;
    int type;

    Decls(int t = TypeInt) : type(t) {}

    virtual bool is_decls() { return true; }

    virtual void print_ir() {
        for (auto *def : defs) {
            Symbol *symbol;
            string id = "@global_" + def->name;

            if (def->declType == ConstDecl && def->offset.empty()) {
                symbol = new Symbol(ConstDecl, def->name, def->type, def->init->eval());
                symbolTable[0].emplace(def->name, symbol);
                continue;
            }

            if (def->declType == VarDecl && def->offset.empty()) {
                printf("global %s = alloc i32, ", id.c_str());
                if (def->init) {
                    symbol = new Symbol(VarDecl, id, def->type, def->init->eval());
                    printf("%d", symbol->value);
                } else {
                    symbol = new Symbol(VarDecl, id, def->type);
                    printf("0");
                }
            } else if (!def->offset.empty()) {
                vector<int> temp, vec;
                for (auto *offset : def->offset) temp.push_back(offset->eval());
                printf("global %s = alloc ", id.c_str());
                printType(def->type, temp);
                for (int num = 1, j = temp.size() - 1; j >= 0; j--) vec.push_back(num *= temp[j]);
                printf(", ");
                symbol = new Symbol(ArrayDecl, id, type);
                symbol->len = def->offset.size();
                printConstInit((InitVal *)def->init, vec);
            }
            symbolTable[0].emplace(def->name, symbol);
            printf("\n");
        }
        printf("\n");
    }
};


class Stmt : public BaseAST {
public:
    BaseAST *expr;
    Var *var;
    int stmt_type;
    Stmt(BaseAST *e, int type, Var *name = nullptr) : expr(e), var(name), stmt_type(type) {}
    virtual void print_ir() { if (expr) expr->print_ir(); genInstr(); }
    virtual void genInstr(int instrType = NoOp) {
        switch (stmt_type) {
            case Return:
                has_break_point = true;
                printf("  ret ");
                if (expr) expr->base_print();
                break;
            case Assign: {
                var->load_array();
                printf("  store "), expr->base_print(), printf(", "), var->base_print();
                findSymbol(var->id)->value = expr->eval();
                break;
            }
            case Break:
                has_break_point = true;
                printf("  jump %%break_%d", *(while_stmt_idx_global.rbegin()));
                break;
            case Continue:
                has_break_point = true;
                printf("  jump %%while_%d", *(while_stmt_idx_global.rbegin()));
                break;
            default:
                return;
        }
        printf("\n");
    }

    virtual bool is_statement() { return true; }
};

class JumpStmt : public BaseAST {
public:
    BaseAST *expr, *then_stmt, *else_stmt;
    int id;

    JumpStmt(BaseAST *e, BaseAST *s1, BaseAST *s2 = nullptr) 
        : expr(e), then_stmt(s1), else_stmt(s2) {}

    virtual void print_ir() {
        id = genJumpStmtIdx();
        expr->print_ir();

        int exist_then_block = then_stmt && !then_stmt->is_empty_block();
        int exist_else_block = else_stmt && !else_stmt->is_empty_block();
        if (!exist_then_block && !exist_else_block) return;

        printf("  br ");
        expr->base_print();
        int ret1 = 0, ret2 = 0;

        switch (exist_then_block * 2 + exist_else_block) {
            case 3:
                printf(", %%then_%d, %%else_%d\n", id, id);
                genBlock("then", id), then_stmt->print_ir();
                ret1 = isJumpInstr(then_stmt);
                if (!ret1) printf("  jump %%end_%d\n", id);
                genBlock("else", id), else_stmt->print_ir();
                ret2 = isJumpInstr(else_stmt);
                if (!ret2) printf("  jump %%end_%d\n", id);
                break;

            case 2:
                printf(", %%then_%d, %%end_%d\n", id, id);
                genBlock("then", id), then_stmt->print_ir();
                ret1 = isJumpInstr(then_stmt);
                if (!ret1) printf("  jump %%end_%d\n", id);
                break;

            case 1:
                printf(", %%end_%d, %%else_%d\n", id, id);
                genBlock("else", id), else_stmt->print_ir();
                ret2 = isJumpInstr(else_stmt);
                if (!ret2) printf("  jump %%end_%d\n", id);
                break;
        }

        if (ret1 && ret2) return;
        genBlock("end", id);
    }

    virtual bool is_jump_statement() { return true; }
};


class WhileStmt : public BaseAST {
public:
    BaseAST *expr, *stmt;
    int id;

    WhileStmt(BaseAST *e, BaseAST *s = nullptr) : expr(e), stmt(s) {}

    virtual void print_ir() {
        id = genBlockIdx();
        while_stmt_idx_global.push_back(id);
        printf("  jump %%while_%d\n", id);
        genBlock("while", id);
        expr->print_ir();
        printf("  br "), expr->base_print(), printf(", %%stmt_%d, %%break_%d\n", id, id);
        genBlock("stmt", id);
        if (stmt) {
            stmt->print_ir();
            if (!isJumpInstr(stmt) && !has_break_point)
                printf("  jump %%while_%d\n", id);
        } else
            printf("  jump %%while_%d\n", id);
        genBlock("break", id);
        while_stmt_idx_global.pop_back();
    }
};


class Expr : public BaseAST {
public:
    BaseAST *left_expr, *right_expr;
    int operation, var = -1;

    Expr(BaseAST *right, BaseAST *left = nullptr, int oper = NoOp)
        : left_expr(left), right_expr(right), operation(oper) {}

    void printSubExpr() { left_expr->base_print(), printf(", "), right_expr->base_print(); }
    virtual bool is_expr() { return true; }
    virtual void base_print() { printf("%%%d", var); }
    virtual bool is_boolean() { return operation >= Less && operation <= NotEqZero; }

    virtual void print_ir() {
        if (operation == Or || operation == And) {
            Number num;
            num.num = (operation == Or ? 1 : 0);
            string name = "result" + to_string(genResultIdx());
            Var *v = new Var(name);
            DeclareDef def(name, (BaseAST *)&num);
            def.declType = VarDecl;
            Stmt s1(right_expr, Assign, v);
            JumpStmt stmt(left_expr, operation == Or ? nullptr : (BaseAST *)&s1, operation == Or ? (BaseAST *)&s1 : nullptr);
            def.print_ir(), stmt.print_ir(), v->print_ir();
            var = v->temp_var;
            return;
        }
        var = genVarIdx();
        if (left_expr) left_expr->print_ir();
        right_expr->print_ir();
        genInstr(operation);
    }

    virtual void genInstr(int instrType) {
        if (instrType == NoOp) return;
        printf("  %%%d = ", var);
        static const char *ops[] = {
            nullptr, "sub 0, ", "eq ", "add ", "sub ", "mul ", "div ",
            "mod ", "lt ", "gt ", "le ", "ge ", "eq ", "ne ", "and ", "or "};
        if (instrType >= Add && instrType <= Or) printf("%s", ops[instrType - Add + 3]), printSubExpr();
        else if (instrType == EqZero) printf("eq "), right_expr->base_print(), printf(", 0");
        else if (instrType == Invert) printf("sub 0, "), right_expr->base_print();
        else if (instrType == NotEqZero) printf("ne "), right_expr->base_print(), printf(", 0");
        printf("\n");
    }

    virtual int eval() {
        if (operation >= Add && operation <= Or) {
            int temp1 = left_expr->eval(), temp2 = right_expr->eval();
            switch (operation) {
                case Add: return temp1 + temp2;
                case Sub: return temp1 - temp2;
                case Mul: return temp1 * temp2;
                case Div: return temp2 == 0 ? 0 : temp1 / temp2;
                case Mod: return temp2 == 0 ? 0 : temp1 % temp2;
                case Less: return temp1 < temp2;
                case Greater: return temp1 > temp2;
                case LEq: return temp1 <= temp2;
                case GEq: return temp1 >= temp2;
                case Equal: return temp1 == temp2;
                case NotEqual: return temp1 != temp2;
                case And: return temp1 & temp2;
                case Or: return temp1 | temp2;
            }
        }
        int temp = right_expr->eval();
        if (operation == Invert) return -temp;
        if (operation == EqZero) return temp == 0;
        if (operation == NotEqZero) return temp != 0;
        return temp;
    }
};





class Block : public BaseAST {
public:
    vector<BaseAST*> stmts;

    virtual void print_ir() {
        symbolTable.emplace_back();
        int block_id = genBlockIdx();
        for (auto *stmt : stmts) {
            current_block_idx = block_id;
            if (has_break_point) break;
            stmt->print_ir();
        }
        cleanSymbolMap(symbolTable.back());
        symbolTable.pop_back();
    }

    virtual bool is_block() { return true; }
    virtual bool is_empty_block() { return stmts.empty(); }
};

class FuncDef : public BaseAST {
public:
    int func_type;
    string id;
    BaseAST *block;
    vector<DeclareDef*> params;

    virtual void print_ir() {
        FuncInfo *func_info = new FuncInfo(func_type);
        has_break_point = false;
        printf("fun @%s(", id.c_str());
        for (size_t i = 0; i < params.size(); ++i) {
            func_info->param_type.push_back(params[i]->type);
            printf("@%s: ", params[i]->name.c_str());
            vector<int> vec(params[i]->offset.size());
            for (size_t j = 0; j < params[i]->offset.size(); ++j) vec[j] = params[i]->offset[j]->eval();
            if (params[i]->type == TypePointer) printf("*");
            printType(params[i]->type, vec);
            if (i != params.size() - 1) printf(", ");
        }
        printf(")");
        func_table.emplace(id, func_info);
        if (func_type == TypeInt) printf(": i32");
        printf(" {\n%%entry:\n");
        for (auto it = params.rbegin(); it != params.rend(); ++it)
            ((Block *)block)->stmts.insert(((Block *)block)->stmts.begin(), *it);
        if (func_type == TypeInt || func_type == TypeVoid) {
            Number *num = new Number();
            num->num = 0;
            Stmt *stmt = new Stmt(func_type == TypeInt ? (BaseAST *)num : nullptr, Return);
            ((Block *)block)->stmts.push_back((BaseAST *)stmt);
        }
        block->print_ir();
        printf("}\n\n");
    }
};


class Program : public BaseAST {
public:
    vector<BaseAST*> units;

    virtual void print_ir() {
        static const struct {
            const char *decl;
            const char *name;
            int type;
            vector<int> param_types;
        } builtins[] = {
            {"decl @getint(): i32", "getint", TypeInt, {}},
            {"decl @getch(): i32", "getch", TypeInt, {}},
            {"decl @getarray(*i32): i32", "getarray", TypeInt, {TypePointer}},
            {"decl @putint(i32)", "putint", TypeVoid, {TypeInt}},
            {"decl @putch(i32)", "putch", TypeVoid, {TypeInt}},
            {"decl @putarray(i32, *i32)", "putarray", TypeVoid, {TypeInt, TypePointer}},
            {"decl @starttime()", "starttime", TypeVoid, {}},
            {"decl @stoptime()", "stoptime", TypeVoid, {}}
        };

        for (auto &builtin : builtins) {
            printf("%s\n", builtin.decl);
            func_table.emplace(builtin.name, new FuncInfo(builtin.type, builtin.param_types));
        }
        printf("\n");

        for (auto *unit : units)
            if (unit->is_decls()) unit->print_ir();
        for (auto *unit : units)
            if (!unit->is_decls()) unit->print_ir();
    }
};

class FuncCall : public BaseAST
{
public:
    string name;
    vector<BaseAST*> params;
    int temp_var;

    virtual void print_ir()
    {
        FuncInfo *func_info = func_table[name];
        int return_type = func_info->func_type;
        temp_var = genVarIdx();

        // Load each parameter
        for (size_t i = 0; i < params.size(); i++)
        {
            switch (func_info->param_type[i])
            {
                case TypeInt:
                    params[i]->print_ir();
                    break;
                case TypePointer:
                    Number *num = new Number();
                    num->num = 0;
                    ((Var*)params[i])->offset.push_back(num);
                    params[i]->load_array();
                    break;
            }
        }

        // Print function call
        printf("  ");
        if (return_type == TypeInt) 
        {
            printf("%%%d = ", temp_var);
        }
        printf("call @%s(", name.c_str());

        for (size_t i = 0; i < params.size(); i++) 
        {
            params[i]->base_print();
            if (i < params.size() - 1) 
            {
                printf(", ");
            }
        }
        printf(")\n");
    }

    virtual void base_print()
    {
        printf("%%%d", temp_var);
    }
};


static bool isJumpInstr(BaseAST *s)
{
    if (s->is_statement()) {
        Stmt *stmt = (Stmt*)s;
        return stmt->stmt_type == Return || stmt->stmt_type == Break || stmt->stmt_type == Continue;
    } 
    if (s->is_block()) {
        Block *block = (Block*)s;
        return !block->is_empty_block() && isJumpInstr(*block->stmts.rbegin());
    } 
    if (s->is_jump_statement()) {
        JumpStmt *stmt = (JumpStmt*)s;
        return (stmt->then_stmt ? isJumpInstr(stmt->then_stmt) : 0) &&
               (stmt->else_stmt ? isJumpInstr(stmt->else_stmt) : 0);
    }
    return false;
}


static int findArraySize(vector<int> offset, int now_num)
{
    for (int i = 0; i < offset.size(); i++)
        if (now_num % offset[i] != 0) return 1;
    return offset.back();
}

static void printType(int type, vector<int> offset)
{
    if (offset.empty()) {
        if (type == TypeInt || type == TypePointer) printf("i32");
    } else {
        printf("[");
        int o = offset.front(); offset.erase(offset.begin());
        printType(type, offset);
        printf(", %d]", o);
    }
}


static void printConstInit(InitVal *init, vector<int> offset)
{
    if (!init) { printf("zeroinit"); return; }

    int need_num = offset.back();
    offset.pop_back();

    if (offset.empty()) {
        printf("{");
        for (int i = 0; i < init->inits.size(); i++) {
            printf("%d", init->inits[i]->eval());
            if (i != need_num - 1) printf(",");
        }
        for (int i = init->inits.size(); i < need_num; i++) {
            printf("0");
            if (i != need_num - 1) printf(",");
        }
        printf("}");
        return;
    }

    int child_num = offset.back(), now_num = 0;
    InitVal *val = new InitVal();
    printf("{");
    for (int i = 0; i < init->inits.size(); i++) {
        if (!init->inits[i] || init->inits[i]->is_initval()) {
            int rule_num = findArraySize(offset, now_num);
            now_num += rule_num;
            if (rule_num == child_num) {
                printConstInit((InitVal*)init->inits[i], offset);
                if (now_num != need_num) printf(",");
                continue;
            }
            val->inits.push_back(init->inits[i]);
            if (now_num % child_num == 0) {
                printConstInit(val, offset);
                val->inits.clear();
                if (now_num != need_num) printf(",");
            }
        } else {
            val->inits.push_back(init->inits[i]);
            if (++now_num % child_num == 0) {
                printConstInit(val, offset);
                val->inits.clear();
                if (now_num != need_num) printf(",");
            }
        }
    }

    if (!val->inits.empty()) {
        printConstInit(val, offset);
        now_num = (now_num / child_num + 1) * child_num;
        if (now_num != need_num) printf(",");
    }
    delete val;

    while (now_num < need_num) {
        printf("zeroinit");
        now_num += child_num;
        if (now_num != need_num) printf(",");
    }
    printf("}");
}


static void printVarInit(InitVal *init, vector<int> offset, string id, int now_num)
{
    for (int i = 0; i < init->inits.size(); i++) {
        if (!init->inits[i]) {
            now_num += findArraySize(offset, now_num);
        } else if (init->inits[i]->is_initval()) {
            now_num += findArraySize(offset, now_num);
            printVarInit((InitVal*)init->inits[i], offset, id, now_num);
        } else {
            int ptr_id = genPtrId(), temp_num = now_num;
            (init->inits[i])->print_ir();
            printf("  %%ptr_%d = getelemptr %s, %d", ptr_id, id.c_str(), temp_num / offset.back());
            temp_num %= offset.back();
            for (int j = offset.size() - 2; j >= 0; j--) {
                int old_ptr_id = ptr_id;
                ptr_id = genPtrId();
                printf("  %%ptr_%d = getelemptr %%ptr_%d, %d", ptr_id, old_ptr_id, temp_num / offset[j]);
                temp_num %= offset[j];
            }
            printf("  store ");
            (init->inits[i])->base_print();
            printf(", %%ptr_%d\n", ptr_id);
            now_num++;
        }
    }
}
