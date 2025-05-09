#pragma once

#include <string>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include "symtab.hpp"

typedef std::unique_ptr<std::string> ps_t;
typedef std::string s_t;

using std::cout;

extern int exp_var_cnt;
extern int if_cnt;

static int block_ret;

// base class for AST
class BaseAST {
public:
    // useful for mutirule-symbol
    int rule;
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;

    virtual void DumpIR() const { }
    virtual int Eval() const { throw "Not implemented\n"; return 0; }
};

typedef std::unique_ptr<BaseAST> past_t;

struct Variable {
    s_t ident;
    s_t type;
    std::unique_ptr< std::vector<past_t> > arr_exp_list;
};

typedef std::unique_ptr<Variable> pvar_t;

//  LVal ::= IDENT | IDENT "[" Exp "]"
struct LVal
{
    s_t ident;
    int rule;
    std::unique_ptr<std::vector<past_t> > arr_exp_list;
};

// CompUnit
class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<std::vector<past_t> > comp_unit_list;
    void Dump() const override {
        cout << "CompUnit { ";
        for (auto i = comp_unit_list->begin(); i != comp_unit_list->end(); i++) {
            (*i)->Dump();
            cout << ", ";
        }
        cout << " }";
    }

    void DumpIR() const override {
        init_symtab();

        cout << "decl @getint(): i32\n"
                "decl @getch(): i32\n"
                "decl @getarray(*i32): i32\n"
                "decl @putint(i32)\n"
                "decl @putch(i32)\n"
                "decl @putarray(i32, *i32)\n"
                "decl @starttime()\n"
                "decl @stoptime()\n\n";

        symbol_insert("getint", S_FUNC, F_INT);
        symbol_insert("getch", S_FUNC, F_INT);
        symbol_insert("getarray", S_FUNC, F_INT);
        symbol_insert("putint", S_FUNC, F_VOID);
        symbol_insert("putch", S_FUNC, F_VOID);
        symbol_insert("putarray", S_FUNC, F_VOID);
        symbol_insert("starttime", S_FUNC, F_VOID);
        symbol_insert("stoptime", S_FUNC, F_VOID);

        for (auto i = comp_unit_list->begin(); i != comp_unit_list->end(); i++) {
            (*i)->DumpIR();
        }
        destroy_symtab();
    }
};

// FuncDef ::= FuncType IDENT "(" [FuncFParams] ")" Block
class FuncDefAST : public BaseAST {
public:
    s_t func_type;
    s_t ident;
    past_t block;
    std::unique_ptr<std::vector<pvar_t> > func_param_list;
    void Dump() const override {
        cout << "FuncDef { ";
        
        cout << func_type << " " << ident << " ( ";
        for (auto i = func_param_list->begin(); i != func_param_list->end(); i++) {
            if ((*i)->type[0] == 'i') {
                cout << (*i)->type << " " << (*i)->ident;
            }
            else if ((*i)->type[0] == '*') {
                cout << (*i)->type << "[]";
                for (auto j = (*i)->arr_exp_list->begin(); j != (*i)->arr_exp_list->end(); j++) {
                    cout << "[ ";
                    (*j)->Dump();
                    cout << " ]";
                }
                cout << " " << (*i)->ident;
            }
            cout << ", ";
        }
        cout << ") ";
        block->Dump();
        cout << " }";
    }

    void DumpIR() const override {
        std::shared_ptr<Symbol> func, var;
        int temp = 0;
        exp_var_cnt = 0;
        func = symbol_insert(ident, S_FUNC, func_type[0] == 'i' ? F_INT : F_VOID);
        enter_block();

        cout << "fun @" << func->name << "(";
        for (auto i = func_param_list->begin(); i != func_param_list->end(); i++) {
            if ((*i)->type[0] == 'i') { 
                cout << "@a" << i - func_param_list->begin() << ": i32";
            }
            else if ((*i)->type[0] == '*') {
                cout << "@a" << i - func_param_list->begin() << ": *";
                temp = (*i)->arr_exp_list->size();
                cout << std::string(temp, '[') << "i32";
                for (auto j = (*i)->arr_exp_list->begin(); j != (*i)->arr_exp_list->end(); j++) {
                    cout << ", " << (*j)->Eval() << ']';
                }
            }
            if (i + 1 != func_param_list->end()) {
                cout << ", ";
            }
        }
        cout << ")" << (func_type[0] == 'i' ? ": i32" : "") << " {\n";
        cout << "\%entry:\n";

        for (auto i = func_param_list->begin(); i != func_param_list->end(); i++) {
            if ((*i)->type[0] == 'i') {
                var = symbol_insert((*i)->ident, S_VAR, 0);
                cout << "  @" << var->name << " = alloc i32\n";
                cout << "  store @a" << i - func_param_list->begin() << ", @" << var->name << "\n";
            }
            else if ((*i)->type[0] == '*') {
                var = symbol_insert((*i)->ident, S_POINTER, (*i)->arr_exp_list->size()+1);
                cout << "  @" << var->name << " = alloc *" << std::string((*i)->arr_exp_list->size(), '[') << "i32";
                for (auto j = (*i)->arr_exp_list->begin(); j != (*i)->arr_exp_list->end(); j++) {
                    cout << ", " << (*j)->Eval() << ']';
                }
                cout << '\n';
                cout << "  store @a" << i - func_param_list->begin() << ", @" << var->name << "\n";
            }
        }

        block->DumpIR();
        if (block_ret == 0) {
            printf("warning: func %s: no return statement in all braches\n", ident.c_str());
            cout << "  ret" << (func_type[0] == 'i' ? " 0" : "") << "\n";
        }
        cout << "}\n";
        exit_block(); 
    }
};

// // FuncFParam ::= BType IDENT
// class FuncFParamAST : public BaseAST {
//     s_t b_type;
//     s_t ident;
//     void Dump() const override {
//         cout << "FuncFParam { " << b_type << ", " << ident << " }";
//     }
//     void DumpIR() const override {
//         assert(!"FuncFParamAST::DumpIR() not implemented"); 
//     }
// };


// // FuncType
// class FuncTypeAST : public BaseAST {
// public:
//     s_t type_name;
//     void Dump() const override {
//         cout << "FuncType { " << type_name << " }";
//     }
//     void DumpIR() const override {
//         if (type_name == "int") {
//             cout << "i32";
//         }
//     }
// };


// Block ::= "{" {BlockItem} "}"
class BlockAST : public BaseAST {
public:
    std::unique_ptr<std::vector<past_t> > block_item_list;
    void Dump() const override {
        cout << "Block { ";
        for (auto i = block_item_list->begin(); i != block_item_list->end(); i++) {
            (*i)->Dump();
            cout << ", ";
        }
        cout << " }";
    }
    void DumpIR() const override {
        block_ret = 0;
        for (auto i = block_item_list->begin(); i != block_item_list->end(); i++) {
            (*i)->DumpIR();
            if (block_ret) 
                break;
        }
    }
};


// BlockItem ::= Decl | Stmt 
class BlockItemAST : public BaseAST {
public:
    past_t decl, stmt;
    void Dump() const override {
        cout << "BlockItem { ";
        switch (rule)
        {
        case 1:
            decl->Dump();
            break;
        case 2:
            stmt->Dump();
            break;
        default:
            break;
        }
        cout << " }";
    }
    void DumpIR() const override {
        switch (rule)
        {
        case 1:
            decl->DumpIR();
            break;
        case 2:
            stmt->DumpIR();
            break;
        default:
            break;
        }
    }
};


// Statement : LVal "=" Exp ";" | [Exp] ";" | Block | "return" [Exp] ";" | IF "(" Exp ")" (then)Stmt [ELSE (else)Stmt]
//           | "while" "(" Exp ")" Stmt 
class StmtAST : public BaseAST {
public:
    past_t exp, block, then_stmt, else_stmt;
    LVal *l_val;
    void Dump() const override {
        cout << "Stmt { ";
        switch (rule)
        {
        case 1:
            if (l_val->rule == 1) {
                cout << l_val->ident;
            }
            else if (l_val->rule == 2) {
                for (auto i = l_val->arr_exp_list->begin(); i != l_val->arr_exp_list->end(); i++) {
                    cout << " [ ";
                    (*i)->Dump();
                    cout << " ] ";
                }
            }
            cout << " = ";
            exp->Dump();
            cout << "; ";
            break;
        case 201:
            exp->Dump();
            cout << "; ";
            break;
        case 202:
            cout << " ; ";
            break;
        case 3:
            block->Dump();
            break;
        case 401:
            cout << "return, ";
            exp->Dump();
            cout << "; ";
            break;
        case 402:
            cout << "return; ";
            break;
        case 501:
            cout << "if, ( ";
            exp->Dump();
            cout << ") then ";
            then_stmt->Dump();
            break;
        case 502:
            cout << "if ( ";
            exp->Dump();
            cout << " ) then ";
            then_stmt->Dump();
            cout << " else ";
            else_stmt->Dump();
            break;
        case 6:
            cout << "while, ( ";
            exp->Dump();
            cout << ") ";
            then_stmt->Dump();
            break;
        case 7:
            cout << "break;";
            break;
        case 8:
            cout << "continue;";
            break;
        default:
            break;
        }
        cout << " }";
    }
    void DumpIR() const override {
        std::shared_ptr<Symbol> symbol;
        int if_num, then_ret = 0, else_ret = 0, last_ptr, exp_var;
        switch (rule)
        {
        case 1:
            if (l_val->rule == 1) {
                symbol = symbol_query(l_val->ident);
                if (symbol->type == SymType::S_CONST) {
                    assert(!"Error: try to assign to a const variable\n");
                }
                else if (symbol->type == SymType::S_VAR) {
                    exp->DumpIR();
                    cout << "  store \%e" << exp_var_cnt-1 << ", @" << symbol->name << "\n";
                }
                else if (symbol->type == SymType::S_UNDEF) {
                    assert(!"Error: try to assign to an undefined variable\n");
                }
            }
            else if (l_val->rule == 2) {
                symbol = symbol_query(l_val->ident);
                if (symbol->type == SymType::S_ARRAY) {
                    assert(l_val->arr_exp_list->size() > 0);
                    exp->DumpIR();
                    exp_var = exp_var_cnt-1;
                    for (auto i = l_val->arr_exp_list->begin(); i != l_val->arr_exp_list->end(); i++)
                    {
                        if (i == l_val->arr_exp_list->begin()) {
                            (*i)->DumpIR();
                            cout << "  \%e" << exp_var_cnt << " = getelemptr @" << symbol->name << ", \%e" << exp_var_cnt-1 << "\n";
                            last_ptr = exp_var_cnt;
                            exp_var_cnt++;
                        }
                        else {
                            (*i)->DumpIR();
                            cout << "  \%e" << exp_var_cnt << " = getelemptr \%e" << last_ptr << ", \%e" << exp_var_cnt-1 << "\n";
                            last_ptr = exp_var_cnt;
                            exp_var_cnt++;
                        }
                    }
                    cout << "  store \%e" << exp_var << ", \%e" << last_ptr << "\n";
                }
                else if (symbol->type == SymType::S_POINTER) {
                    assert(l_val->arr_exp_list->size() > 0);
                    exp->DumpIR();
                    exp_var = exp_var_cnt-1;
                    l_val->arr_exp_list->front()->DumpIR();
                    cout << "  \%e" << exp_var_cnt << " = load @" << symbol->name << "\n";
                    exp_var_cnt++;
                    cout << "  \%e" << exp_var_cnt << " = getptr \%e" << exp_var_cnt-1 << ", \%e" << exp_var_cnt-2 << "\n";
                    last_ptr = exp_var_cnt;
                    exp_var_cnt++;
                    for (auto i = l_val->arr_exp_list->begin()+1; i != l_val->arr_exp_list->end(); i++) {
                        (*i)->DumpIR();
                        cout << "  \%e" << exp_var_cnt << " = getelemptr \%e" << last_ptr << ", \%e" << exp_var_cnt-1 << "\n";
                        last_ptr = exp_var_cnt;
                        exp_var_cnt++;
                    }
                    cout << "  store \%e" << exp_var << ", \%e" << last_ptr << "\n";
                }
            }
            break;
        case 201:
            exp->DumpIR();
            break;
        case 202:
            break;
        case 3:
            enter_block();
            block->DumpIR();
            exit_block();
            break;
        case 401: 
            exp->DumpIR();
            cout << "  ret \%e" << exp_var_cnt-1;
            cout << "\n";
            block_ret = 1;
            break;
        case 402:
            cout << "  ret\n";
            block_ret = 1;
            break;
        case 501:
            if_num = if_cnt++;
            exp->DumpIR();
            cout << "  br \%e" << exp_var_cnt-1 << ", \%then_" << if_num << ", \%end_" << if_num << "\n";
            cout << "\%then_" << if_num << ":\n";
            then_stmt->DumpIR();
            // the ret in %then not related to %end
            if (block_ret == 0) {
                cout << "  jump \%end_" << if_num << "\n";
            }
            else {
                block_ret = 0;
            }
            cout << "\%end_" << if_num << ":\n";
            break;
        case 502:
            if_num = if_cnt++;
            exp->DumpIR();
            cout << "  br \%e" << exp_var_cnt-1 << ", \%then_" << if_num << ", \%else_" << if_num << "\n";
            cout << "\%then_" << if_num << ":\n";
            then_stmt->DumpIR();
            // the ret in %then not related to %emd
            if (block_ret == 0) {
                cout << "  jump \%end_" << if_num << "\n";
            }
            else {
                then_ret = 1;
                block_ret = 0;
            }
            cout << "\%else_" << if_num << ":\n";
            else_stmt->DumpIR();
            // the ret in %else not related to %end
            if (block_ret == 0) {
                cout << "  jump \%end_" << if_num << "\n";
            }
            else {
                else_ret = 1;
                block_ret = 0;
            }

            // if both %then and %else have ret, the ret in %end is useless
            if (then_ret == 1 && else_ret == 1) {
                block_ret = 1;
            }
            else {
                cout << "\%end_" << if_num << ":\n";
            }
            break;
        case 6:
            if_num = if_cnt++;
            enter_loop(if_num);
            cout << "  jump \%loop_" << if_num << "\n";
            cout << "\%loop_" << if_num << ":\n";
            exp->DumpIR();
            cout << "  br \%e" << exp_var_cnt-1 << ", \%then_" << if_num << ", \%end_" << if_num << "\n";
            cout << "\%then_" << if_num << ":\n";
            then_stmt->DumpIR();
            if (block_ret == 0) {
                cout << "  jump \%loop_" << if_num << "\n";
            }
            else {
                block_ret = 0;
            }
            exit_loop();
            cout << "\%end_" << if_num << ":\n";
            break;
        case 7:
            cout << "  jump \%end_" << cur_loop_cnt() << "\n";
            block_ret = 1;
            break;
        case 8:
            cout << "  jump \%loop_" << cur_loop_cnt() << "\n";
            block_ret = 1;
            break;
        default:
            printf("StmtAST::DumpIR %d: not implemented yet\n", rule);
            break;
        }
    }
};
