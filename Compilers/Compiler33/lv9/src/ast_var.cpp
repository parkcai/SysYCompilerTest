#include <string>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <cassert>
#include <vector>
#include "ast_var.hpp"
#include "symtab.hpp"

extern int exp_var_cnt;

void get_arr_elem(int pos, const std::vector<int> &arr_sizes, const std::string &arr_name) {
    assert(arr_sizes.size() > 0);
    int mul = 1;
    for (int i = 0; i < arr_sizes.size()-1; i++) {
        mul *= arr_sizes[i];
    }
    for (int i = arr_sizes.size()-1; i >= 0; i--) {
        cout << "  \%e" << exp_var_cnt << " = getelemptr " << 
            ((i == arr_sizes.size()-1) ? "@" + arr_name : "\%e"+std::to_string(exp_var_cnt-1))
             << ", " << (pos / mul) % arr_sizes[i] << "\n";
        if (i > 0) {
            mul /= arr_sizes[i-1];
        }
        exp_var_cnt++;
    }
}

//  Decl ::= ConstDecl | VarDecl
void DeclAST::Dump() const {
    cout << "Decl { ";
    switch (rule)
    {
    case 1:
        const_decl->Dump();
        break;
    case 2:
        var_decl->Dump();
        break;
    default:
        break;
    }
    cout << " }";
}
void DeclAST::DumpIR() const {
    switch (rule)
    {
    case 1:
        const_decl->DumpIR();
        break;
    case 2:
        var_decl->DumpIR();
        break;
    default:
        break;
    }
}


//  ConstDecl ::= "const" BType ConstDef {"," ConstDef} ";"
void ConstDeclAST::Dump() const {
    cout << "ConstDecl { const " << b_type << " ";
    for (auto i = const_def_list->begin(); i != const_def_list->end(); i++) {
        (*i)->Dump();
        cout << ", ";
    }
    cout << "; }";
}
void ConstDeclAST::DumpIR() const {
    switch (b_type[0])
    {
    case 'i': // int
        for (auto i = const_def_list->begin(); i != const_def_list->end(); i++) {
            (*i)->DumpIR();
        }
        break;
    
    default:
        break;
    }
}


//  VarDecl ::= BType VarDef {"," VarDef} ";"
void VarDeclAST::Dump() const {
    cout << "VarDecl { " << b_type << " ";
    for (auto i = var_def_list->begin(); i != var_def_list->end(); i++) {
        (*i)->Dump();
        cout << ", ";
    }
    cout << "; }";
}
void VarDeclAST::DumpIR() const {
    for (auto i = var_def_list->begin(); i != var_def_list->end(); i++) {
        (*i)->DumpIR();
    }
}


//  ConstDef ::= IDENT "=" ConstInitVal | IDENT [ "[" ConstExp "]" ] "=" ConstInitVal
void ConstDefAST::Dump() const {
    cout << "ConstDef { ";
    switch (rule)
    {
    case 1:
        cout << ident << " = ";
        const_init_val->Dump();
        break;
    case 2:
        for (auto i = arr_exp_list->rbegin(); i != arr_exp_list->rend(); i++) {
            cout << " [ ";
            (*i)->Dump();
            cout << " ] ";
        }
        cout << " = ";
        const_init_val->Dump();
        break;    
    default:
        break;
    }
    cout << " }";
}
void ConstDefAST::DumpIR() const {
    std::shared_ptr<Symbol> symbol;
    std::vector<int> arr_sizes;
    std::vector<int> elems;
    int temp = 0, mul = 0;
    switch (rule)
    {
    case 1:
        symbol_insert(ident, SymType::S_CONST, const_init_val->Eval());
        break;
    case 2:
        if (cur_symtab_cnt() == 0) {
            symbol = symbol_insert(ident, SymType::S_ARRAY, arr_exp_list->size());
            cout << "global @" << symbol->name << " = alloc ";
        }
        else {
            symbol = symbol_insert(ident, SymType::S_ARRAY, arr_exp_list->size());
            cout << "  @" << symbol->name << " = alloc ";
        }
        cout << std::string(arr_exp_list->size(), '[') << "i32";
        for (auto i = arr_exp_list->begin(); i != arr_exp_list->end(); i++) {
            temp = (*i)->Eval();
            arr_sizes.push_back(temp);
            cout << ", " << temp << ']';
        }

        if (cur_symtab_cnt() == 0) {
            cout << ", ";
            dynamic_cast<ConstInitValAST*>(const_init_val.get())->Aggregate(arr_sizes);
            cout << "\n";
        }
        else {
            cout << "\n";
            elems = dynamic_cast<ConstInitValAST*>(const_init_val.get())->all_elem(arr_sizes);
            mul = 1;
            for (auto i = arr_sizes.begin(); i != arr_sizes.end(); i++) {
                mul *= (*i);
            }
            for (int i = 0; i < mul; i++) {
                get_arr_elem(i, arr_sizes, symbol->name);
                cout << "  store " << elems[i] << ", \%e" << exp_var_cnt-1 << "\n";
            }
        }

        break;
    default:
        break;
    }
}


//  VarDef ::= IDENT ["=" InitVal] | IDENT [ "[" ConstExp "]" ] ["=" InitVal]
void VarDefAST::Dump() const {
    cout << "VarDef { ";
    switch (rule)
    {
    case 101:
        cout << ident;
        break;
    case 102:
        cout << ident << " = ";
        init_val->Dump();
        break;
    case 201:
        cout << ident;
        for (auto i = arr_exp_list->rbegin(); i != arr_exp_list->rend(); i++) {
            cout << " [ ";
            (*i)->Dump();
            cout << " ] ";
        }
        break;
    case 202:
        cout << ident;
        for (auto i = arr_exp_list->rbegin(); i != arr_exp_list->rend(); i++) {
            cout << " [ ";
            (*i)->Dump();
            cout << " ] ";
        }
        cout << " = ";
        init_val->Dump();
        break;
    
    default:
        break;
    }
    cout << " }";
}
void VarDefAST::DumpIR() const {
    std::shared_ptr<Symbol> symbol;
    int temp = 0, mul = 0, last_ptr = 0;
    int eval_success = 0;
    std::vector<int> arr_sizes, used_arr_sizes;
    std::vector<BaseAST*> exps;
    switch (rule)
    {
    case 101:
        if (cur_symtab_cnt() == 0) {
            symbol = symbol_insert(ident, SymType::S_VAR, 0);
            cout << "global @" << symbol->name << " = alloc i32, 0\n";
        }
        else {
            symbol = symbol_insert(ident, SymType::S_VAR, -1);
            cout << "  @" << symbol->name << " = alloc i32\n";
        }
        break;
    case 102:
        if (cur_symtab_cnt() == 0) { 
            temp = init_val->Eval();
            symbol = symbol_insert(ident, SymType::S_VAR, temp);
            cout << "global @" << symbol->name << " = alloc i32, " << temp << "\n";
        }
        else {
            try {
                temp = init_val->Eval();
                eval_success = 1;
            }
            catch (...){
                temp = 0;
                eval_success = 0;
            }
            symbol = symbol_insert(ident, SymType::S_VAR, -1);
            cout << "  @" << symbol->name << " = alloc i32\n";
            if (eval_success) {
                cout << "  store " << temp << ", @" << symbol->name << "\n";
            }
            else {
                init_val->DumpIR();
                cout << "  store \%e" << exp_var_cnt-1 << ", @" << symbol->name << "\n";
            }
        }
        break;
    case 201:
        symbol = symbol_insert(ident, SymType::S_ARRAY, arr_exp_list->size());
        if (cur_symtab_cnt() == 0) {
            cout << "global @";
        }
        else {
            cout << "  @";
        }
        cout << symbol->name << " = alloc ";
        cout << std::string(arr_exp_list->size(), '[') << "i32";
        for (auto i = arr_exp_list->begin(); i != arr_exp_list->end(); i++) {
            temp = (*i)->Eval();
            arr_sizes.push_back(temp);
            cout << ", " << temp << ']';
        }
        if (cur_symtab_cnt() == 0) {
            // all initialized with zeros
            cout << ", zeroinit\n";
        }
        cout << "\n";
        break;
    case 202:
        symbol = symbol_insert(ident, SymType::S_ARRAY, arr_exp_list->size());
        if (cur_symtab_cnt() == 0) {
            cout << "global @";
        }
        else {
            cout << "  @";
        }
        cout << symbol->name << " = alloc ";
        cout << std::string(arr_exp_list->size(), '[') << "i32";
        for (auto i = arr_exp_list->begin(); i != arr_exp_list->end(); i++) {
            temp = (*i)->Eval();
            arr_sizes.push_back(temp);
            cout << ", " << temp << ']';
        }
        if (cur_symtab_cnt() == 0) {
            cout << ", ";
            dynamic_cast<InitValAST*>(init_val.get())->Aggregate(arr_sizes);
            cout << "\n";
        }
        else {
            cout << "\n";
            exps = dynamic_cast<InitValAST*>(init_val.get())->all_elem(arr_sizes);
            mul = 1;
            for (auto i = arr_sizes.begin(); i != arr_sizes.end(); i++) {
                mul *= (*i);
            }
            for (int i = 0; i < mul; i++) {
                get_arr_elem(i, arr_sizes, symbol->name);
                last_ptr = exp_var_cnt-1;
                try {
                    if (exps[i])
                        temp = exps[i]->Eval();
                    else 
                        temp = 0;
                    eval_success = 1;
                }
                catch (...) {
                    temp = 0;
                    eval_success = 0;
                }
                if (eval_success) {
                    cout << "  store " << temp << ", \%e" << last_ptr << "\n";
                }
                else {
                    exps[i]->DumpIR();
                    cout << "  store \%e" << exp_var_cnt-1 << ", \%e" << last_ptr << "\n";
                }
            }
        }
        break;
    default:
        break;
    }
}


//  ConstInitVal ::= ConstExp | "{" [ ConstInitVal {"," ConstInitVal} ] "}"
void ConstInitValAST::Dump() const {
    cout << "ConstInitVal { ";
    switch (rule)
    {
    case 1:
        const_exp->Dump();
        break;
    case 2:
        cout << " { ";
        for (auto i = const_init_val_list->begin(); i != const_init_val_list->end(); i++) {
            (*i)->Dump();
            cout << ", ";
        }
        cout << " } ";
        break;
    default:
        break;
    }
    cout << " }";
}
void ConstInitValAST::Aggregate(const std::vector<int> &arr_sizes) const {
    int temp = 0;
    std::vector<int> used_arr_sizes = std::vector<int>(arr_sizes.size(), 0), new_arr_sizes;
    for (auto i = const_init_val_list->begin(); i != const_init_val_list->end(); i++) {
        if ((*i)->rule == 1) {
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == 0) {
                    cout << '{';
                }
                else {
                    break;
                }
            }
            temp = (*i)->Eval();
            cout << temp;
            used_arr_sizes[0]++;
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    cout << '}';
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    cout << ',';
                    break;
                }
            }
        }
        else if ((*i)->rule == 2) {
            // must aliged
            assert(used_arr_sizes[0] == 0);
            new_arr_sizes = std::vector<int>();
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (j == used_arr_sizes.size()-1) {
                    temp = j;
                    break;
                }
                if (used_arr_sizes[j] == 0) {
                    new_arr_sizes.push_back(arr_sizes[j]);
                }
                else {
                    temp = j;
                    break;
                }
            }
            if (used_arr_sizes[temp] == 0) {
                cout << '{';
            }
            dynamic_cast<ConstInitValAST*>((*i).get())->Aggregate(new_arr_sizes);
            used_arr_sizes[temp]++;
            for (int j = temp; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    cout << '}';
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    cout << ',';
                    break;
                }
            }
        }
    }
    // add zeros
    while (used_arr_sizes[used_arr_sizes.size()-1] < arr_sizes[used_arr_sizes.size()-1])
    {
        for (int j = 0; j < used_arr_sizes.size(); j++) {
            if (used_arr_sizes[j] == 0) {
                cout << '{';
            }
            else {
                break;
            }
        }
        cout << 0;
        used_arr_sizes[0]++;
        for (int j = 0; j < used_arr_sizes.size(); j++) {
            if (used_arr_sizes[j] == arr_sizes[j]) {
                cout << '}';
                if (j < used_arr_sizes.size()-1) {
                    used_arr_sizes[j+1]++;
                    used_arr_sizes[j] = 0;
                }
            }
            else {
                cout << ',';
                break;
            }
        }
    }

    // asserts
    for (int i = 0; i < used_arr_sizes.size()-1; i++) {
        assert(used_arr_sizes[i] == 0);
    }
    assert(used_arr_sizes[used_arr_sizes.size()-1] == arr_sizes[used_arr_sizes.size()-1]);
}
std::vector<int> ConstInitValAST::all_elem(const std::vector<int> &arr_sizes) const {
    int temp = 0;
    std::vector<int> ret = std::vector<int>();
    std::vector<int> used_arr_sizes = std::vector<int>(arr_sizes.size(), 0), new_arr_sizes;
    for (auto i = const_init_val_list->begin(); i != const_init_val_list->end(); i++) {
        if ((*i)->rule == 1) {
            temp = (*i)->Eval();
            ret.push_back(temp);
            used_arr_sizes[0]++;
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    break;
                }
            }
        }
        else if ((*i)->rule == 2) {
            // must aliged
            assert(used_arr_sizes[0] == 0);
            new_arr_sizes = std::vector<int>();
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (j == used_arr_sizes.size()-1) {
                    temp = j;
                    break;
                }
                if (used_arr_sizes[j] == 0) {
                    new_arr_sizes.push_back(arr_sizes[j]);
                }
                else {
                    temp = j;
                    break;
                }
            }
            new_arr_sizes = dynamic_cast<ConstInitValAST*>((*i).get())->all_elem(new_arr_sizes);
            ret.insert(ret.end(), new_arr_sizes.begin(), new_arr_sizes.end());
            used_arr_sizes[temp]++;
            for (int j = temp; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    break;
                }
            }
        }
    }
    // add zeros
    while (used_arr_sizes[used_arr_sizes.size()-1] < arr_sizes[used_arr_sizes.size()-1])
    {
        ret.push_back(0);
        used_arr_sizes[0]++;
        for (int j = 0; j < used_arr_sizes.size(); j++) {
            if (used_arr_sizes[j] == arr_sizes[j]) {
                if (j < used_arr_sizes.size()-1) {
                    used_arr_sizes[j+1]++;
                    used_arr_sizes[j] = 0;
                }
            }
            else {
                break;
            }
        }
    }

    // asserts
    for (int i = 0; i < used_arr_sizes.size()-1; i++) {
        assert(used_arr_sizes[i] == 0);
    }
    assert(used_arr_sizes[used_arr_sizes.size()-1] == arr_sizes[used_arr_sizes.size()-1]);
    
    return ret;
}
void ConstInitValAST::DumpIR() const {
    assert(!"ConstInitValAST::DumpIR called but not implemented!\n");
}
int ConstInitValAST::Eval() const {
    return const_exp->Eval();
}


//  InitVAl ::= Exp | "{ " [ InitVal { "," InitVal } ] "}"
void InitValAST::Dump() const {
    cout << "InitVal { ";
    switch (rule)
    {
    case 1:
        exp->Dump();
        break;
    case 2:
        cout << " { ";
        for (auto i = init_val_list->begin(); i != init_val_list->end(); i++) {
            (*i)->Dump();
            cout << ", ";
        }
        cout << " } ";
        break;
    
    default:
        break;
    }
    cout << " }";
}
void InitValAST::Aggregate(const std::vector<int> &arr_sizes) const {
    int temp = 0;
    std::vector<int> used_arr_sizes = std::vector<int>(arr_sizes.size(), 0), new_arr_sizes;
    for (auto i = init_val_list->begin(); i != init_val_list->end(); i++) {
        if ((*i)->rule == 1) {
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == 0) {
                    cout << '{';
                }
                else {
                    break;
                }
            }
            temp = (*i)->Eval();
            cout << temp;
            used_arr_sizes[0]++;
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    cout << '}';
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    cout << ',';
                    break;
                }
            }
        }
        else if ((*i)->rule == 2) {
            // must aliged
            assert(used_arr_sizes[0] == 0);
            new_arr_sizes = std::vector<int>();
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (j == used_arr_sizes.size()-1) {
                    temp = j;
                    break;
                }
                if (used_arr_sizes[j] == 0) {
                    new_arr_sizes.push_back(arr_sizes[j]);
                }
                else {
                    temp = j;
                    break;
                }
            }
            if (used_arr_sizes[temp] == 0) {
                cout << '{';
            }
            dynamic_cast<InitValAST*>((*i).get())->Aggregate(new_arr_sizes);
            used_arr_sizes[temp]++;
            for (int j = temp; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    cout << '}';
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    cout << ',';
                    break;
                }
            }
        }
    }
    // add zeros
    while (used_arr_sizes[used_arr_sizes.size()-1] < arr_sizes[used_arr_sizes.size()-1])
    {
        for (int j = 0; j < used_arr_sizes.size(); j++) {
            if (used_arr_sizes[j] == 0) {
                cout << '{';
            }
            else {
                break;
            }
        }
        cout << 0;
        used_arr_sizes[0]++;
        for (int j = 0; j < used_arr_sizes.size(); j++) {
            if (used_arr_sizes[j] == arr_sizes[j]) {
                cout << '}';
                if (j < used_arr_sizes.size()-1) {
                    used_arr_sizes[j+1]++;
                    used_arr_sizes[j] = 0;
                }
            }
            else {
                cout << ',';
                break;
            }
        }
    }

    // asserts
    for (int i = 0; i < used_arr_sizes.size()-1; i++) {
        assert(used_arr_sizes[i] == 0);
    }
    assert(used_arr_sizes[used_arr_sizes.size()-1] == arr_sizes[used_arr_sizes.size()-1]);
}
std::vector<BaseAST*> InitValAST::all_elem(const std::vector<int> &arr_sizes) const {
    int temp = 0;
    std::vector<BaseAST*> ret;
    std::vector<int> used_arr_sizes = std::vector<int>(arr_sizes.size(), 0), new_arr_sizes;
    for (auto i = init_val_list->begin(); i != init_val_list->end(); i++) {
        if ((*i)->rule == 1) {
            ret.push_back((*i).get());
            used_arr_sizes[0]++;
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    break;
                }
            }
        }
        else if ((*i)->rule == 2) {
            // must aliged
            assert(used_arr_sizes[0] == 0);
            new_arr_sizes = std::vector<int>();
            for (int j = 0; j < used_arr_sizes.size(); j++) {
                if (j == used_arr_sizes.size()-1) {
                    temp = j;
                    break;
                }
                if (used_arr_sizes[j] == 0) {
                    new_arr_sizes.push_back(arr_sizes[j]);
                }
                else {
                    temp = j;
                    break;
                }
            }
            auto vec = dynamic_cast<InitValAST*>((*i).get())->all_elem(new_arr_sizes);
            ret.insert(ret.end(), vec.begin(), vec.end());
            used_arr_sizes[temp]++;
            for (int j = temp; j < used_arr_sizes.size(); j++) {
                if (used_arr_sizes[j] == arr_sizes[j]) {
                    if (j < used_arr_sizes.size()-1) {
                        used_arr_sizes[j+1]++;
                        used_arr_sizes[j] = 0;
                    }
                }
                else {
                    break;
                }
            }
        }
    }
    // add zeros
    while (used_arr_sizes[used_arr_sizes.size()-1] < arr_sizes[used_arr_sizes.size()-1])
    {
        ret.push_back(NULL);
        used_arr_sizes[0]++;
        for (int j = 0; j < used_arr_sizes.size(); j++) {
            if (used_arr_sizes[j] == arr_sizes[j]) {
                if (j < used_arr_sizes.size()-1) {
                    used_arr_sizes[j+1]++;
                    used_arr_sizes[j] = 0;
                }
            }
            else {
                break;
            }
        }
    }

    // asserts
    for (int i = 0; i < used_arr_sizes.size()-1; i++) {
        assert(used_arr_sizes[i] == 0);
    }
    assert(used_arr_sizes[used_arr_sizes.size()-1] == arr_sizes[used_arr_sizes.size()-1]);
    
    return ret;
}
void InitValAST::DumpIR() const {
    exp->DumpIR();
}
int InitValAST::Eval() const {
    return exp->Eval();
}