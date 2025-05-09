#include <string>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <cassert>
#include "ast_exp.hpp"
#include "ast.hpp"
#include "symtab.hpp"

using std::cout;

extern int exp_var_cnt;

// Exp ::= LOrExp
void ExpAST::Dump() const {
    cout << "Exp { ";
    l_or_exp->Dump();
    cout << " }";
}
void ExpAST::DumpIR() const {
    l_or_exp->DumpIR();
}
int ExpAST::Eval() const {
    return l_or_exp->Eval();
}


//  RelExp ::= AddExp | RelExp RelOp AddExp
void RelExpAST::Dump() const {
    cout << "RelExp_" << rule << " {";
    switch (rule)
    {
    case 1:
        add_exp->Dump();
        break;
    case 2:
        rel_exp->Dump();
        cout << ", " << rel_op << ", ";
        add_exp->Dump();
        break;
    default:
        break;
    }
    cout << " }";
} 
void RelExpAST::DumpIR() const {
    int left_var, right_var;
    switch (rule)
    {
    case 1:
        add_exp->DumpIR();
        break;
    case 2:
        rel_exp->DumpIR();
        left_var = exp_var_cnt-1;
        add_exp->DumpIR();
        right_var = exp_var_cnt-1;

        cout << "  \%e" << exp_var_cnt << " = ";
        switch (rel_op)
        {
        case '<':
            cout << "lt ";
            break;
        case '>':
            cout << "gt ";
            break;
        case 'L':
            cout << "le ";
            break;
        case 'G':
            cout << "ge ";
            break;
        default:
            break;
        }
        cout << "\%e" << left_var << ", \%e" << right_var << "\n";
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int RelExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = add_exp->Eval();
        break;
    case 2:
        switch (rel_op)
        {
        case '<':
            ret = rel_exp->Eval() < add_exp->Eval();
            break;
        case '>':
            ret = rel_exp->Eval() > add_exp->Eval();
            break;
        case 'L':
            ret = rel_exp->Eval() <= add_exp->Eval();
            break;
        case 'G':
            ret = rel_exp->Eval() >= add_exp->Eval();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}


//  EqExp ::= RelExp | EqExp EqOp RelExp
void EqExpAST::Dump() const {
    cout << "EqExp_" << rule << " {";
    switch (rule)
    {
    case 1:
        rel_exp->Dump();
        break;
    case 2:
        eq_exp->Dump();
        cout << ", " << eq_op << ", ";
        rel_exp->Dump();
        break;
    default:
        break;
    }
    cout << " }";
} 
void EqExpAST::DumpIR() const {
    int left_var, right_var;
    switch (rule)
    {
    case 1:
        rel_exp->DumpIR();
        break;
    case 2:
        eq_exp->DumpIR();
        left_var = exp_var_cnt-1;
        rel_exp->DumpIR();
        right_var = exp_var_cnt-1;

        cout << "  \%e" << exp_var_cnt << " = ";
        switch (eq_op)
        {
        case 'E':
            cout << "eq ";
            break;
        case 'N':
            cout << "ne ";
            break;
        default:
            break;
        }
        cout << "\%e" << left_var << ", \%e" << right_var << "\n";
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int EqExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = rel_exp->Eval();
        break;
    case 2:
        switch (eq_op)
        {
        case 'E':
            ret = eq_exp->Eval() == rel_exp->Eval();
            break;
        case 'N':
            ret = eq_exp->Eval() != rel_exp->Eval();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}


//  LAndExp ::= EqExp | LAndExp LAndOp EqExp
void LAndExpAST::Dump() const {
    cout << "LAndExp_" << rule << " {";
    switch (rule)
    {
    case 1:
        eq_exp->Dump();
        break;
    case 2:
        l_and_exp->Dump();
        cout << ", " << l_and_op << ", ";
        eq_exp->Dump();
        break;
    default:
        break;
    }
    cout << " }";
} 
void LAndExpAST::DumpIR() const {
    int left_var, right_var;
    int if_num;
    switch (rule)
    {
    case 1:
        eq_exp->DumpIR();
        break;
    case 2:
        if_num = if_cnt++;
        cout << "  @cond_" << if_num << " = alloc i32\n";
        l_and_exp->DumpIR();
        left_var = exp_var_cnt-1;

        cout << "  br \%e" << left_var << ", \%then_" << if_num << ", \%else_" << if_num << '\n'
             << "\%then_" << if_num << ":\n";

        eq_exp->DumpIR();
        right_var = exp_var_cnt-1;
        //  A && B <==> (A!=0) & (B!=0)
        cout << "  \%e" << exp_var_cnt << " = ne 0, \%e" << right_var << '\n';
        right_var = exp_var_cnt;
        exp_var_cnt++;
        cout << "  store \%e" << exp_var_cnt-1 << ", @cond_" << if_num << '\n'
             << "  jump \%end_" << if_num << '\n';
        cout << "\%else_" << if_num << ":\n"
             << "  store 0, @cond_" << if_num << '\n'
             << "  jump \%end_" << if_num << '\n'
             << "\%end_" << if_num << ":\n"
             << "  \%e" << exp_var_cnt << " = load @cond_" << if_num << '\n';
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int LAndExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = eq_exp->Eval();
        break;
    case 2:
        ret = l_and_exp->Eval() && eq_exp->Eval();
        break;
    default:
        break;
    }
    return ret;
}


//  LOrExp ::= LAndExp | LOrExp LOrOp LAndExp
void LOrExpAST::Dump() const {
    cout << "LOrExp_" << rule << " {";
    switch (rule)
    {
    case 1:
        l_and_exp->Dump();
        break;
    case 2:
        l_or_exp->Dump();
        cout << ", " << l_or_op << ", ";
        l_and_exp->Dump();
        break;
    default:
        break;
    }
    cout << " }";
} 
void LOrExpAST::DumpIR() const {
    int left_var, right_var;
    int if_num;
    switch (rule)
    {
    case 1:
        l_and_exp->DumpIR();
        break;
    case 2:
        if_num = if_cnt++;
        cout << "  @cond_" << if_num << " = alloc i32\n";
        l_or_exp->DumpIR();
        left_var = exp_var_cnt-1;

        cout << "  br \%e" << left_var << ", \%then_" << if_num << ", \%else_" << if_num << '\n'
             << "\%then_" << if_num << ":\n"
             << "  store 1, @cond_" << if_num << '\n' 
             << "  jump \%end_" << if_num << '\n' 
             << "\%else_" << if_num << ":\n";

        l_and_exp->DumpIR();
        right_var = exp_var_cnt-1;
        //  A || B <==> (A!=0) | (B!=0)
        //  if enter here, then A==0
        cout << "  \%e" << exp_var_cnt << " = ne 0, \%e" << right_var << '\n';
        right_var = exp_var_cnt;
        exp_var_cnt++;
        cout << "  store \%e" << exp_var_cnt-1 << ", @cond_" << if_num << '\n'
             << "  jump \%end_" << if_num << '\n'
             << "\%end_" << if_num << ":\n" 
             << "  \%e" << exp_var_cnt << " = load @cond_" << if_num << '\n';
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int LOrExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = l_and_exp->Eval();
        break;
    case 2:
        ret = l_or_exp->Eval() || l_and_exp->Eval();
        break;
    default:
        break;
    }
    return ret;
}


//  AddExp ::= MulExp | AddExp AddOp MulExp
void AddExpAST::Dump() const {
    cout << "AddExp_" << rule << " {";
    switch (rule)
    {
    case 1:
        mul_exp->Dump();
        break;
    case 2:
        add_exp->Dump();
        cout << ", " << add_op << ", ";
        mul_exp->Dump();
        break;
    default:
        break;
    }
    cout << " }";
}
void AddExpAST::DumpIR() const {
    int left_var, right_var;
    bool left_eval_seccess = 0, right_eval_seccess = 0;
    switch (rule)
    {
    case 1:
        mul_exp->DumpIR();
        break;
    case 2:
        try {
            left_var = add_exp->Eval();
            left_eval_seccess = 1;
        }
        catch(...) {
            left_eval_seccess = 0;
        }
        try {
            right_var = mul_exp->Eval();
            right_eval_seccess = 1;
        }
        catch(...) {
            right_eval_seccess = 0;
        }
        
        if (!left_eval_seccess) { 
            add_exp->DumpIR();
            left_var = exp_var_cnt-1;
        }
        if (!right_eval_seccess) {
            mul_exp->DumpIR();
            right_var = exp_var_cnt-1;
        }

        cout << "  \%e" << exp_var_cnt << " = ";
        switch (add_op)
        {
        case '+':
            cout << "add ";
            break;
        case '-':
            cout << "sub ";
            break;
        default:
            break;
        }
        cout << (left_eval_seccess ? std::to_string(left_var) : "\%e" + std::to_string(left_var)) << ", " 
             << (right_eval_seccess ? std::to_string(right_var) : "\%e" + std::to_string(right_var) ) << "\n";
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int AddExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = mul_exp->Eval();
        break;
    case 2:
        switch (add_op)
        {
        case '+':
            ret = add_exp->Eval() + mul_exp->Eval();
            break;
        case '-':
            ret = add_exp->Eval() - mul_exp->Eval();
            break;
        default:
            assert(!"AddExp: no such op");
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}


//  MulExp ::= UnaryExp | MulExp MulOp UnaryExp
void MulExpAST::Dump() const {
    cout << "MulExp_" << rule << " {";
    switch (rule)
    {
    case 1:
        unary_exp->Dump();
        break;
    case 2:
        mul_exp->Dump();
        cout << ", " << mul_op << ", ";
        unary_exp->Dump();
        break;
    default:
        break;
    }
    cout << " }";
}
void MulExpAST::DumpIR() const {
    int left_var, right_var;
    switch (rule)
    {
    case 1:
        unary_exp->DumpIR();
        break;
    case 2:
        mul_exp->DumpIR();
        left_var = exp_var_cnt-1;
        unary_exp->DumpIR();
        right_var = exp_var_cnt-1;

        cout << "  \%e" << exp_var_cnt << " = ";
        switch (mul_op)
        {
        case '*':
            cout << "mul ";
            break;
        case '/':
            cout << "div ";
            break;
        case '%':
            cout << "mod ";
        default:
            break;
        }
        cout << "\%e" << left_var << ", \%e" << right_var << "\n";
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int MulExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = unary_exp->Eval();
        break;
    case 2:
        switch (mul_op)
        {
        case '*':
            ret = mul_exp->Eval() * unary_exp->Eval();
            break;
        case '/':
            ret = mul_exp->Eval() / unary_exp->Eval(); 
            break;
        case '%':
            ret = mul_exp->Eval() % unary_exp->Eval();
            break;
        default:
            assert(!"MulExp: no such op!\n");
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}


//  UnaryExp ::= PrimaryExp | UnaryOp UnaryExp | IDENT "(" [FuncRParams] ")"
void UnaryExpAST::Dump() const {
    cout << "UnaryExp_" << rule <<" { ";
    switch (rule)
    {
    case 1:
        primary_exp->Dump();
        break;
    case 2:
        cout << unary_op; 
        cout << ", ";
        unary_exp->Dump();
        break;
    case 3:
        cout << func_name << " ( ";
        for (auto i = func_args->begin(); i != func_args->end(); i++)
        {
            (*i)->Dump();
            if (i != func_args->end()-1)
                cout << ", ";
        }
        cout << " )";
        break;
    default:
        assert(!"UnaryExp: no such rule!\n");
        break;
    }
    cout << " }";
}
void UnaryExpAST::DumpIR() const {
    std::shared_ptr<Symbol> symbol;
    std::vector<int> func_args_val;
    switch (rule)
    {
    case 1:
        primary_exp->DumpIR();
        break;
    case 2:
        unary_exp->DumpIR();
        switch (unary_op)
        {
        case '-':
            cout << "  \%e" << exp_var_cnt << " = sub 0, \%e" << exp_var_cnt-1 << "\n";
            exp_var_cnt++;
            break;
        case '!':
            cout << "  \%e" << exp_var_cnt << " = eq 0, \%e" << exp_var_cnt-1 << "\n";
            exp_var_cnt++;
        default:
            break;
        }
        break;
    case 3:
        symbol = symbol_query(func_name);
        assert(symbol->type == SymType::S_FUNC);
        for (auto i = func_args->begin(); i != func_args->end(); i++)
        {
            (*i)->DumpIR();
            func_args_val.push_back(exp_var_cnt-1);
        }
        if (symbol->value == F_VOID) {
            cout << "  call @" << func_name << "(";
            for (auto i = func_args_val.begin(); i != func_args_val.end(); i++)
            {
                cout << "\%e" << *i;
                if (i != func_args_val.end()-1)
                    cout << ", ";
            }
            cout << ")\n";
        }
        else if (symbol->value == F_INT) {
            cout << "  \%e" << exp_var_cnt << " = call @" << func_name << "(";
            for (auto i = func_args_val.begin(); i != func_args_val.end(); i++)
            {
                cout << "\%e" << *i;
                if (i != func_args_val.end()-1)
                    cout << ", ";
            }
            cout << ")\n";
            exp_var_cnt++;
        }
        break;
    default:
        break;
    }
}
int UnaryExpAST::Eval() const {
    int ret=-1;
    switch (rule)
    {
    case 1:
        ret = primary_exp->Eval();
        break;
    case 2:
        ret = unary_exp->Eval();
        switch (unary_op)
        {
        case '-':
            ret = -ret;
            break;
        case '!':
            ret = !ret;
            break;
        default:
            assert(!"UnaryExp: no such op!\n");
            break;
        }
        break;
    default:
        throw "Can not eval func\n";
        break;
    }
    return ret;
}


//  PrimaryExp ::= "(" Exp ")" | LVal | Number
void PrimaryExpAST::Dump() const {
    cout << "PrimaryExp_" << rule << " { ";
    switch (rule)
    {
    case 1:
        exp->Dump();
        break;
    case 2:
        if (l_val->rule == 1) {
            cout << l_val->ident;
        }
        else if (l_val->rule == 2) {
            cout << l_val->ident;
            for (auto i = l_val->arr_exp_list->begin(); i != l_val->arr_exp_list->end(); i++)
            {
                cout << " [ ";
                (*i)->Dump();
                cout << " ] ";
            }
        }
        break;
    case 3:
        cout << number;
        break;
    default:
        assert(!"PrimaryExp: no such rule!\n");
        break;
    }
    cout << " }";
}
void PrimaryExpAST::DumpIR() const {
    std::shared_ptr<Symbol> symbol;
    int last_ptr;
    switch (rule)
    {
    case 1:
        exp->DumpIR();
        break;
    case 2:
        if (l_val->rule == 1) {
            symbol = symbol_query(l_val->ident);
            switch (symbol->type)
            {
            case SymType::S_CONST:
                cout << "  \%e" << exp_var_cnt << " = add 0, " << symbol->value << "\n";
                exp_var_cnt++;
                break;
            case SymType::S_VAR:
                cout << "  \%e" << exp_var_cnt << " = load @" << symbol->name << "\n";
                exp_var_cnt++;
                break;
            case SymType::S_POINTER:
                cout << "  \%e" << exp_var_cnt << " = load @" << symbol->name << "\n";
                exp_var_cnt++;
                break;
            case SymType::S_ARRAY:
                cout << "  \%e" << exp_var_cnt << " = getelemptr @" << symbol->name << ", 0\n";
                exp_var_cnt++;
                break;
            case SymType::S_UNDEF:
                assert(!"PrimaryExp::DumpIR: undefined symbol!\n");
                break;
            default:
                break;
            }
        }
        else if (l_val->rule == 2) {
            symbol = symbol_query(l_val->ident);
            if (symbol->type == SymType::S_ARRAY) {
                assert(l_val->arr_exp_list->size() > 0);
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
                // if the arr exp is matched, then it is an array access
                if (l_val->arr_exp_list->size() == symbol->value) {
                    cout << "  \%e" << exp_var_cnt << " = load \%e" << last_ptr << "\n";
                    exp_var_cnt++;
                }
                // if the arr exp is not matched, then it is a pointer access
                else {
                    cout << "  \%e" << exp_var_cnt << " = getelemptr \%e" << last_ptr << ", 0\n";
                    exp_var_cnt++;
                }
            }
            else if (symbol->type == SymType::S_POINTER) {
                assert(l_val->arr_exp_list->size() > 0);
                l_val->arr_exp_list->front()->DumpIR();
                cout << "  \%e" << exp_var_cnt << " = load @" << symbol->name << "\n";
                exp_var_cnt++;
                cout << "  \%e" << exp_var_cnt << " = getptr \%e" << exp_var_cnt-1 << ", \%e" << exp_var_cnt-2 << "\n";
                last_ptr = exp_var_cnt;
                exp_var_cnt++;
                for (auto i = l_val->arr_exp_list->begin()+1; i != l_val->arr_exp_list->end(); i++)
                {
                    (*i)->DumpIR();
                    cout << "  \%e" << exp_var_cnt << " = getelemptr \%e" << last_ptr << ", \%e" << exp_var_cnt-1 << "\n";
                    last_ptr = exp_var_cnt;
                    exp_var_cnt++;
                }
                // if the arr exp is matched, then it is an array access
                if (l_val->arr_exp_list->size() == symbol->value) {
                    cout << "  \%e" << exp_var_cnt << " = load \%e" << last_ptr << "\n";
                    exp_var_cnt++;
                }
                // if the arr exp is not matched, then it is a pointer access
                else {
                    cout << "  \%e" << exp_var_cnt << " = getelemptr \%e" << last_ptr << ", 0\n";
                    exp_var_cnt++;
                }
            }
            else {
                printf("PrimaryExp::DumpIR: wrong type!\n");
                assert(false);
            }
        }
        break;
    case 3:
        cout << "  \%e" << exp_var_cnt << " = add 0, " << number << "\n";
        exp_var_cnt++;
        break;
    default:
        break;
    }
}
int PrimaryExpAST::Eval() const {
    int ret=-1;
    std::shared_ptr<Symbol> symbol;
    switch (rule)
    {
    case 1:
        ret = exp->Eval();
        break;
    case 2:
        symbol = symbol_query(l_val->ident);
        switch (symbol->type)
        {
        case SymType::S_CONST:
            ret = symbol->value;
            break;
        case SymType::S_VAR:
            throw "PrimaryExp: variable not initialized in compiler!\n";
            break;
        case SymType::S_UNDEF:
            throw "Undefined symbol!\n";
            break;
        case SymType::S_ARRAY:
            throw "PrimaryExp: array not initialized in compiler!\n";
            break;
        case SymType::S_POINTER:
            throw "PrimaryExp: pointer not initialized in compiler!\n";
            break;
        default:
            break;
        }
        break;
    case 3:
        ret = number;
        break;
    default:
        break;
    }
    return ret;
}


//  ConstExp ::= Exp
void ConstExpAST::Dump() const {
    cout << "ConstExp { ";
    exp->Dump();
    cout << " }";
}
void ConstExpAST::DumpIR() const {
    assert(!"ConstExp::DumpIR: called but not implemented!\n");
}
int ConstExpAST::Eval() const {
    return exp->Eval();
}
