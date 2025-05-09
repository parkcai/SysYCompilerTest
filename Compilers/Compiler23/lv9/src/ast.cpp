#include "ast.hpp"
#include "koopa.h"
#include <iostream>
#include <stack>
#include <utility>

static int reg = 0, if_num = 0, while_num = 0;
SymbolTable sym_table;
static int returned = 0;
static bool global_decl = false;
std::stack<int> cur_while;

std::string getVar(std::string ident)
{
    int t = sym_table.find(ident)->times;
    if (t == 0)
        return ident;
    return ident + '_' + std::to_string(t);
}

/*void CompUnitAST::Dump() const {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
}*/
void CompUnitAST::Koopa() const
{
    std::cout << "decl @getint(): i32\n"
              << "decl @getch(): i32\n"
              << "decl @getarray(*i32): i32\n"
              << "decl @putint(i32)\n"
              << "decl @putch(i32)\n"
              << "decl @putarray(i32, *i32)\n"
              << "decl @starttime()\n"
              << "decl @stoptime()\n"
              << std::endl;
    sym_table.insert("getint", FUNC_INT, std::nullopt);
    sym_table.insert("getch", FUNC_INT, std::nullopt);
    sym_table.insert("getarray", FUNC_INT, std::nullopt);
    sym_table.insert("putint", FUNC_VOID, std::nullopt);
    sym_table.insert("putch", FUNC_VOID, std::nullopt);
    sym_table.insert("putarray", FUNC_VOID, std::nullopt);
    sym_table.insert("starttime", FUNC_VOID, std::nullopt);
    sym_table.insert("stoptime", FUNC_VOID, std::nullopt);

    for (auto &comp_unit_item : *comp_units)
    {
        comp_unit_item->Koopa();
    }
}

void CompUnitItemAST::Koopa() const
{
    if (case_ == 1)
    {
        global_decl = true;
        decl->Koopa();
        global_decl = false;
    }
    else if (case_ == 2)
    {
        func_def->Koopa();
    }
}

/*void FuncDefAST::Dump() const {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }*/
void FuncDefAST::Koopa() const
{
    auto funcType = dynamic_cast<TypeAST *>(func_type.get());
    if (funcType->type == "int")
    {
        sym_table.insert(ident, FUNC_INT, std::nullopt);
    }
    else if (funcType->type == "void")
    {
        sym_table.insert(ident, FUNC_VOID, std::nullopt);
    }
    sym_table.enterScope();
    std::cout << "fun ";
    std::cout << '@' << ident << '(';
    bool comma = 0;
    auto var_name = std::vector<std::string>();
    for (auto &func_f_param : *func_f_params)
    {
        if (comma)
            std::cout << ", ";
        else
            comma = true;
        auto param = dynamic_cast<FuncFParamAST *>(func_f_param.get());
        if (param->case_ == 1)
        {
            sym_table.insert(param->ident, VAR_INT, std::nullopt);
            var_name.push_back(getVar(param->ident));
            std::cout << "@_" << var_name.back() << ": i32";
        }
        else
        {                                                                  // 数组参数
            sym_table.insert(param->ident, PTR, param->index->size() + 1); // 指针维度+1是数组维度
            var_name.push_back(getVar(param->ident));
            std::cout << "@_" << var_name.back() << ": ";
            param->Koopa();
        }
    }
    std::cout << ')';
    func_type->Koopa();
    std::cout << '{' << std::endl;
    std::cout << "%entry:" << std::endl;
    returned = 0;
    for (int i = 0; i < func_f_params->size(); i++)
    {
        std::cout << "  @" << var_name.at(i) << " = alloc ";
        func_f_params->at(i)->Koopa();
        std::cout << "  store @_" << var_name.at(i) << ", @" << var_name.at(i) << std::endl;
    }

    block->Koopa();
    if (!returned)
    {
        if (funcType->type == "void")
        {
            std::cout << "  ret" << std::endl;
        }
        else if (funcType->type == "int")
        {
            std::cout << "  ret 0" << std::endl;
        }
    }
    std::cout << '}' << std::endl
              << std::endl;
    sym_table.exitScope();
}

void FuncFParamAST::Koopa() const // 输出类型
{
    if (case_ == 1)
    {
        std::cout << "i32";
    }
    else if (case_ == 2)
    {
        std::cout << "*";
        int len = index->size();
        for (int i = 0; i < len; i++)
        {
            std::cout << "[";
        }
        std::cout << "i32";
        for (int i = 0; i < len; i++)
        {
            std::cout << ", " << index->at(i)->getValue() << "]"; // 一定是ConstExp
        }
    }
}

/*void FuncTypeAST::Dump() const {
    std::cout << "FuncTypeAST { " << type << " }";
}*/
void TypeAST::Koopa() const
{
    if (type == "int")
    {
        std::cout << ": i32 ";
    }
}

/*void BlockAST::Dump() const {
    std::cout << "BlockAST { ";
    stmt->Dump();
    std::cout << " }";
}*/
void BlockAST::Koopa() const
{
    for (auto &block_item : *block_items)
    {
        block_item->Koopa();
        if (returned)
            break;
    }
}

void BlockItemAST::Koopa() const
{
    decl_or_stmt->Koopa();
}

void ClosedStmtAST::Koopa() const
{

    switch (case_)
    {
    case 1:
    { // LVal "=" Exp ";"
        auto lval = dynamic_cast<LValAST *>(l_val.get());
        if (lval->exp_index->empty())
        {
            exp->loadIdent();
            if (!exp->isNumber())
            {
                exp->Koopa();
                std::cout << "  store %" << reg - 1 << ", @" << getVar(lval->ident) << std::endl;
            }
            else
            {
                std::cout << "  store ";
                exp->Koopa();
                std::cout << ", @" << getVar(lval->ident) << std::endl;
            }
            sym_table.updateValue(lval->ident, std::nullopt);
        }
        else
        {
            lval->Koopa();
            int addr = reg - 1;
            exp->loadIdent();
            if (!exp->isNumber())
            {
                exp->Koopa();
                std::cout << "  store %" << reg - 1 << ", %" << addr << std::endl;
            }
            else
            {
                std::cout << "  store ";
                exp->Koopa();
                std::cout << ", %" << addr << std::endl;
            }
        }
        break;
    }
    case 2:
    { //"return" Exp ";";
        exp->loadIdent();
        if (exp->isNumber())
        {
            std::cout << "  ret ";
            exp->Koopa();
            std::cout << std::endl;
        }
        else
        { // 表达式
            exp->Koopa();
            std::cout << "  ret %" << reg - 1 << std::endl;
        }
        returned = 1;
        break;
    }
    case 3:
    {
        sym_table.enterScope();
        block->Koopa();
        sym_table.exitScope();
        break;
    }
    case 4:
    { // return ";"
        returned = 1;
        std::cout << "  ret" << std::endl;
        break;
    }
    case 5:
    { // Exp ";"
        exp->loadIdent();
        if (!exp->isNumber())
            exp->Koopa();
        break;
    }
    case 6:
    { //";"
        return;
    }
    case 7:
    { // IF '(' expression ')' closed_statement ELSE closed_statement
        exp->loadIdent();
        int num = if_num + 1;
        if_num++;
        if (exp->isNumber())
        {
            std::cout << "  br ";
            exp->Koopa();
        }
        else
        { // 表达式
            exp->Koopa();
            std::cout << "  br %" << reg - 1;
        }
        std::cout << ", %then" << num << ", %else" << num << std::endl;
        std::cout << std::endl
                  << "%then" << num << ':' << std::endl;
        returned = 0;
        stmt->Koopa();
        if (!returned)
            std::cout << "  jump %end" << num << std::endl;
        std::cout << std::endl
                  << "%else" << num << ':' << std::endl;
        returned = 0;
        else_stmt->Koopa();
        if (!returned)
            std::cout << "  jump %end" << num << std::endl;
        std::cout << std::endl
                  << "%end" << num << ':' << std::endl;
        returned = 0;
        break;
    }
    case 8:
    {
        int num = while_num + 1;
        while_num++;
        cur_while.push(num);
        std::cout << "  jump %while_entry" << num << std::endl;
        std::cout << std::endl
                  << "%while_entry" << num << ':' << std::endl;
        exp->loadIdent();
        if (exp->isNumber())
        {
            std::cout << "  br ";
            exp->Koopa();
        }
        else
        { // 表达式
            exp->Koopa();
            std::cout << "  br %" << reg - 1;
        }
        std::cout << ", %while_body" << num << ", %while_end" << num << std::endl;
        std::cout << std::endl
                  << "%while_body" << num << ':' << std::endl;
        returned = 0;
        stmt->Koopa();
        if (!returned)
            std::cout << "  jump %while_entry" << num << std::endl;
        std::cout << std::endl
                  << "%while_end" << num << ':' << std::endl;
        returned = 0;
        cur_while.pop();
        break;
    }
    case 9: // break
    {
        std::cout << "  jump %while_end" << cur_while.top() << std::endl;
        returned = 1;
        break;
    }
    case 10: // continue
    {
        std::cout << "  jump %while_entry" << cur_while.top() << std::endl;
        returned = 1;
        break;
    }
    default:
        break;
    }
}

void OpenStmtAST::Koopa() const
{
    if (case_ == 1 || case_ == 2)
    {
        exp->loadIdent();
        int num = if_num + 1;
        if_num++;
        if (exp->isNumber())
        {
            std::cout << "  br ";
            exp->Koopa();
        }
        else
        { // 表达式
            exp->Koopa();
            std::cout << "  br %" << reg - 1;
        }
        if (case_ == 1)
        {
            std::cout << ", %then" << num << ", %end" << num << std::endl;
            std::cout << std::endl
                      << "%then" << num << ':' << std::endl;
            returned = 0;
            stmt->Koopa();
            if (!returned)
                std::cout << "  jump %end" << num << std::endl;
            std::cout << std::endl
                      << "%end" << num << ':' << std::endl;
        }
        else
        {
            std::cout << ", %then" << num << ", %else" << num << std::endl;
            std::cout << std::endl
                      << "%then" << num << ':' << std::endl;
            returned = 0;
            stmt->Koopa();
            if (!returned)
                std::cout << "  jump %end" << num << std::endl;
            std::cout << std::endl
                      << "%else" << num << ':' << std::endl;
            returned = 0;
            else_stmt->Koopa();
            if (!returned)
                std::cout << "  jump %end" << num << std::endl;
            std::cout << std::endl
                      << "%end" << num << ':' << std::endl;
        }
    }
    else if (case_ == 3) // WHILE '(' expression ')' open_statement
    {
        int num = while_num + 1;
        while_num++;
        cur_while.push(num);
        std::cout << "  jump %while_entry" << num << std::endl;
        std::cout << std::endl
                  << "%while_entry" << num << ':' << std::endl;
        exp->loadIdent();
        if (exp->isNumber())
        {
            std::cout << "  br ";
            exp->Koopa();
        }
        else
        { // 表达式
            exp->Koopa();
            std::cout << "  br %" << reg - 1;
        }
        std::cout << ", %while_body" << num << ", %while_end" << num << std::endl;
        std::cout << std::endl
                  << "%while_body" << num << ':' << std::endl;
        returned = 0;
        stmt->Koopa();
        if (!returned)
            std::cout << "  jump %while_entry" << num << std::endl;
        std::cout << std::endl
                  << "%while_end" << num << ':' << std::endl;
        cur_while.pop();
    }
    returned = 0;
}

void StmtAST::Koopa() const
{
    stmt->Koopa();
}

void ExpAST::Koopa() const
{
    l_or_exp->Koopa();
}
int ExpAST::getValue()
{
    if (value)
        return *value;
    value = l_or_exp->getValue();
    return *value;
}
bool ExpAST::loadIdent()
{
    return l_or_exp->loadIdent();
}
bool ExpAST::isNumber()
{
    return l_or_exp->isNumber();
}

void PrimaryExpAST::Koopa() const
{
    if (case_ == 1)
    {
        exp_or_l_val->Koopa();
    }
    else if (case_ == 2) // LVal
    {
        exp_or_l_val->Koopa();
        auto lval = dynamic_cast<LValAST *>(exp_or_l_val.get());
        if (!lval->exp_index->empty())
        {
            auto info = sym_table.find(lval->ident);
            if (lval->exp_index->size() == info->value)
            {
                std::cout << "  %" << reg << " = load %" << reg - 1 << std::endl;
                reg++;
            }
        }
    }
    else if (case_ == 3)
    {
        std::cout << number;
    }
}
int PrimaryExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 3)
    {
        value = number;
        return *value;
    }
    value = exp_or_l_val->getValue();
    return *value;
}
bool PrimaryExpAST::loadIdent()
{
    if (case_ == 1 || case_ == 2)
        return exp_or_l_val->loadIdent();
    return false;
}

void UnaryExpAST::Koopa() const
{
    if (case_ == 1)
    {
        primary_exp_or_unary_exp->Koopa();
    }
    else if (case_ == 2)
    {
        if (unary_op == '+' && primary_exp_or_unary_exp->isNumber())
        { // 如果是+数字 直接输出数字
            primary_exp_or_unary_exp->Koopa();
            return;
        }
        if (unary_op == '+')
        { // 如果是+表达式
            primary_exp_or_unary_exp->Koopa();
            return;
        }
        if (!primary_exp_or_unary_exp->isNumber())
            primary_exp_or_unary_exp->Koopa();
        std::cout << "  %" << reg << " = ";
        switch (unary_op)
        {
        case '-':
            std::cout << "sub 0, ";
            break;
        case '!':
            std::cout << "eq 0, ";
            break;
        case '+':
            std::cout << "add 0, ";
            break;
        default:
            break;
        }
        if (primary_exp_or_unary_exp->isNumber())
        {
            primary_exp_or_unary_exp->Koopa();
        }
        else
        {
            std::cout << "%" << reg - 1;
        }
        std::cout << std::endl;
        reg++;
    }
    else if (case_ == 3)
    {
        auto func = sym_table.find(ident);
        if (func->type != FUNC_INT && func->type != FUNC_VOID)
        {
            return;
        }
        auto params_vec = new std::vector<int>(); // 保存参数的内容或寄存器
        auto kind_vec = new std::vector<int>();   // 保存参数的类型,数字（0）或寄存器（1）
        for (auto &func_r_param : *func_r_params)
        {
            if (!func_r_param->loadIdent())
            { // 不需要load变量，只含有数字
                params_vec->push_back(func_r_param->getValue());
                kind_vec->push_back(0);
                continue;
            }
            else
            {
                if (!func_r_param->isNumber())
                    func_r_param->Koopa();
                params_vec->push_back(reg - 1);
                kind_vec->push_back(1);
            }
        }
        std::cout << "  ";
        if (func->type == FUNC_INT)
        {
            std::cout << '%' << reg << " = ";
            reg++;
        }
        std::cout << "call @" << ident << '(';
        bool comma = 0;
        int size = params_vec->size();
        for (int i = 0; i < size; i++)
        {
            if (comma)
                std::cout << ", ";
            else
                comma = 1;
            if (kind_vec->at(i))
            {
                std::cout << '%';
            }
            std::cout << params_vec->at(i);
        }
        std::cout << ')' << std::endl;
    }
}
bool PrimaryExpAST::isNumber()
{
    if (case_ == 3)
        return true;
    return exp_or_l_val->isNumber();
}

int UnaryExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 2)
    {
        switch (unary_op)
        {
        case '-':
            value = -primary_exp_or_unary_exp->getValue();
            break;
        case '!':
            value = !primary_exp_or_unary_exp->getValue();
            break;
        case '+':
            value = primary_exp_or_unary_exp->getValue();
            break;

        default:
            break;
        }
    }
    else if (case_ == 1)
    {
        value = primary_exp_or_unary_exp->getValue();
    }
    return *value; // 调用函数不能getvalue
}
bool UnaryExpAST::loadIdent()
{
    if (case_ == 1 || case_ == 2)
        return primary_exp_or_unary_exp->loadIdent();
    return true;
}
bool UnaryExpAST::isNumber()
{
    if (case_ == 1)
        return primary_exp_or_unary_exp->isNumber();
    else if (case_ == 2)
        if (unary_op == '+')
            return primary_exp_or_unary_exp->isNumber();
    return false;
}

void MulExpAST::Koopa() const
{
    if (case_ == 1)
    {
        unary_exp->Koopa();
    }
    else if (case_ == 2)
    {
        if (!mul_exp->isNumber())
            mul_exp->Koopa();
        int reg1 = reg - 1;
        if (!unary_exp->isNumber())
            unary_exp->Koopa();
        int reg2 = reg - 1;
        std::cout << "  %" << reg << " = ";
        switch (mul_op)
        {
        case '*':
            std::cout << "mul ";
            break;
        case '/':
            std::cout << "div ";
            break;
        case '%':
            std::cout << "mod ";
            break;
        default:
            break;
        }
        if (mul_exp->isNumber())
        {
            mul_exp->Koopa();
        }
        else
        {
            std::cout << "%" << reg1;
        }
        std::cout << ", ";
        if (unary_exp->isNumber())
        {
            unary_exp->Koopa();
        }
        else
            std::cout << "%" << reg2;
        std::cout << std::endl;
        reg++;
    }
}
int MulExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 1)
    {
        value = unary_exp->getValue();
    }
    else if (case_ == 2)
    {
        switch (mul_op)
        {
        case '*':
            value = mul_exp->getValue() * unary_exp->getValue();
            break;
        case '/':
            value = mul_exp->getValue() / unary_exp->getValue();
            break;
        case '%':
            value = mul_exp->getValue() % unary_exp->getValue();
            break;
        default:
            break;
        }
    }
    return *value;
}
bool MulExpAST::loadIdent()
{
    if (case_ == 1)
    {
        return unary_exp->loadIdent();
    }
    else
    {
        return mul_exp->loadIdent() | unary_exp->loadIdent();
    }
}
bool MulExpAST::isNumber()
{
    if (case_ == 1)
        return unary_exp->isNumber();
    return false;
}

void AddExpAST::Koopa() const
{
    if (case_ == 1)
    {
        mul_exp->Koopa();
    }
    else if (case_ == 2)
    {
        if (!add_exp->isNumber())
            add_exp->Koopa();
        int reg1 = reg - 1;
        if (!mul_exp->isNumber())
            mul_exp->Koopa();
        int reg2 = reg - 1;
        (void)reg2;
        std::cout << "  %" << reg << " = ";
        switch (add_op)
        {
        case '+':
            std::cout << "add ";
            break;
        case '-':
            std::cout << "sub ";
            break;
        default:
            break;
        }
        if (add_exp->isNumber())
        {
            add_exp->Koopa(); // 如果是数字 输出数字
        }
        else
        {
            std::cout << "%" << reg1;
        }
        std::cout << ", ";
        if (mul_exp->isNumber())
            mul_exp->Koopa();
        else
            std::cout << "%" << reg - 1;
        std::cout << std::endl;
        reg++;
    }
}
int AddExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 1)
    {
        value = mul_exp->getValue();
    }
    else if (case_ == 2)
    {
        switch (add_op)
        {
        case '+':
            value = add_exp->getValue() + mul_exp->getValue();
            break;
        case '-':
            value = add_exp->getValue() - mul_exp->getValue();
            break;
        default:
            break;
        }
    }
    return *value;
}
bool AddExpAST::loadIdent()
{
    if (case_ == 1)
    {
        return mul_exp->loadIdent();
    }
    else
    {
        return add_exp->loadIdent() | mul_exp->loadIdent();
    }
}
bool AddExpAST::isNumber()
{
    if (case_ == 1)
        return mul_exp->isNumber();
    return false;
}

void RelExpAST::Koopa() const
{
    if (case_ == 1)
    {
        add_exp->Koopa();
    }
    else if (case_ == 2)
    {
        if (!rel_exp->isNumber())
            rel_exp->Koopa();
        int reg1 = reg - 1;
        if (!add_exp->isNumber())
            add_exp->Koopa();
        int reg2 = reg - 1;
        std::cout << "  %" << reg << " = ";
        switch (rel_op.size())
        {
        case 1:
            if (rel_op[0] == '<')
                std::cout << "lt ";
            else if (rel_op[0] == '>')
                std::cout << "gt ";
            break;
        case 2:
            if (rel_op[0] == '<')
                std::cout << "le ";
            else if (rel_op[0] == '>')
                std::cout << "ge ";
            break;
        default:
            break;
        }
        if (rel_exp->isNumber())
        {
            rel_exp->Koopa();
        }
        else
        {
            std::cout << "%" << reg1;
        }
        std::cout << ", ";
        if (add_exp->isNumber())
        {
            add_exp->Koopa();
        }
        else
            std::cout << "%" << reg2;
        std::cout << std::endl;
        reg++;
    }
}
int RelExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 1)
    {
        value = add_exp->getValue();
    }
    else if (case_ == 2)
    {
        switch (rel_op.size())
        {
        case 1:
            if (rel_op[0] == '<')
                value = rel_exp->getValue() < add_exp->getValue();
            else if (rel_op[0] == '>')
                value = rel_exp->getValue() > add_exp->getValue();
            break;
        case 2:
            if (rel_op[0] == '<')
                value = rel_exp->getValue() <= add_exp->getValue();
            else if (rel_op[0] == '>')
                value = rel_exp->getValue() >= add_exp->getValue();
            break;
        default:
            break;
        }
    }
    return *value;
}
bool RelExpAST::loadIdent()
{
    if (case_ == 1)
    {
        return add_exp->loadIdent();
    }
    else
    {
        return rel_exp->loadIdent() | add_exp->loadIdent();
    }
}
bool RelExpAST::isNumber()
{
    if (case_ == 1)
        return add_exp->isNumber();
    return false;
}

void EqExpAST::Koopa() const
{
    if (case_ == 1)
    {
        rel_exp->Koopa();
    }
    else if (case_ == 2)
    {
        if (!eq_exp->isNumber())
            eq_exp->Koopa();
        int reg1 = reg - 1;
        if (!rel_exp->isNumber())
            rel_exp->Koopa();
        int reg2 = reg - 1;
        std::cout << "  %" << reg << " = ";
        switch (eq_op[0])
        {
        case '=':
            std::cout << "eq ";
            break;
        case '!':
            std::cout << "ne ";
            break;
        default:
            break;
        }
        if (eq_exp->isNumber())
        {
            eq_exp->Koopa();
        }
        else
        {
            std::cout << "%" << reg1;
        }
        std::cout << ", ";
        if (rel_exp->isNumber())
        {
            rel_exp->Koopa();
        }
        else
            std::cout << "%" << reg2;
        std::cout << std::endl;
        reg++;
    }
}
int EqExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 1)
    {
        value = rel_exp->getValue();
    }
    else if (case_ == 2)
    {
        switch (eq_op[0])
        {
        case '=':
            value = eq_exp->getValue() == rel_exp->getValue();
            break;
        case '!':
            value = eq_exp->getValue() != rel_exp->getValue();
            break;
        default:
            break;
        }
    }
    return *value;
}
bool EqExpAST::loadIdent()
{
    if (case_ == 1)
    {
        return rel_exp->loadIdent();
    }
    else
    {
        return eq_exp->loadIdent() | rel_exp->loadIdent();
    }
}
bool EqExpAST::isNumber()
{
    if (case_ == 1)
        return rel_exp->isNumber();
    return false;
}

void LAndExpAST::Koopa() const
{
    if (case_ == 1)
    {
        eq_exp->Koopa();
    }
    else if (case_ == 2)
    {
        if (!l_and_exp->isNumber())
        {
            l_and_exp->Koopa();
            std::cout << "  %" << reg << " = ne 0, %" << reg - 1 << std::endl;
            reg++;
        }
        else if (l_and_exp->isNumber())
        {
            std::cout << "  %" << reg << " = ne 0, ";
            l_and_exp->Koopa();
            reg++;
            std::cout << std::endl;
        }
        // 短路求值
        /*int result = 0;
        if (lhs != 0) {
        result = rhs != 0;
        }*/
        int num = if_num + 1;
        if_num++;
        std::cout << "  @result" << num << " = alloc i32" << std::endl;
        std::cout << "  store 0, @result" << num << std::endl;
        std::cout << "  br %" << reg - 1 << ", %then" << num << ", %end" << num << std::endl; // lhs!=0，跳转到then
        std::cout << std::endl
                  << "%then" << num << ':' << std::endl;
        returned = 0;
        if (!eq_exp->isNumber())
        {
            eq_exp->Koopa();
            std::cout << "  %" << reg << " = ne 0, %" << reg - 1 << std::endl;
            reg++;
        }
        else if (eq_exp->isNumber())
        {
            std::cout << "  %" << reg << " = ne 0, ";
            eq_exp->Koopa();
            std::cout << std::endl;
            reg++;
        }
        std::cout << "  store %" << reg - 1 << ", @result" << num << std::endl;
        if (!returned)
            std::cout << "  jump %end" << num << std::endl;
        std::cout << std::endl
                  << "%end" << num << ':' << std::endl;
        returned = 0;
        std::cout << "  %" << reg << " = load @result" << num << std::endl;
        reg++;
    }
}
int LAndExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 1)
    {
        value = eq_exp->getValue();
    }
    else if (case_ == 2)
    {
        value = l_and_exp->getValue() && eq_exp->getValue();
    }
    return *value;
}
bool LAndExpAST::loadIdent()
{
    if (case_ == 1)
    {
        return eq_exp->loadIdent();
    }
    else
    {
        return l_and_exp->loadIdent() | eq_exp->loadIdent();
    }
}
bool LAndExpAST::isNumber()
{
    if (case_ == 1)
        return eq_exp->isNumber();
    return false;
}

void LOrExpAST::Koopa() const
{
    if (case_ == 1) // LOrExp     ::= LAndExp
    {
        l_and_exp->Koopa();
    }
    else if (case_ == 2) // LOrExp     ::= LOrExp "||" LAndExp
    {
        if (!l_or_exp->isNumber())
        {
            l_or_exp->Koopa();
            std::cout << "  %" << reg << " = ne 0, %" << reg - 1 << std::endl;
            reg++;
        }
        else
        {
            std::cout << "  %" << reg << " = ne 0, ";
            l_or_exp->Koopa();
            reg++;
            std::cout << std::endl;
        }
        // 短路求值
        /*int result = 1;
        if (lhs == 0) {
        result = rhs != 0;
        }*/
        int num = if_num + 1;
        if_num++;
        std::cout << "  @result" << num << " = alloc i32" << std::endl;
        std::cout << "  store 1, @result" << num << std::endl;
        std::cout << "  br %" << reg - 1 << ", %end" << num << ", %then" << num << std::endl; // lhs==0，跳转到then
        std::cout << std::endl
                  << "%then" << num << ':' << std::endl;
        returned = 0;
        if (!l_and_exp->isNumber())
        {
            l_and_exp->Koopa();
            std::cout << "  %" << reg << " = ne 0, %" << reg - 1 << std::endl;
            reg++;
        }
        else
        {
            std::cout << "  %" << reg << " = ne 0, ";
            l_and_exp->Koopa();
            reg++;
            std::cout << std::endl;
        }
        std::cout << "  store %" << reg - 1 << ", @result" << num << std::endl;
        if (!returned)
            std::cout << "  jump %end" << num << std::endl;
        std::cout << std::endl
                  << "%end" << num << ':' << std::endl;
        returned = 0;
        std::cout << "  %" << reg << " = load @result" << num << std::endl;
        reg++;
    }
}
int LOrExpAST::getValue()
{
    if (value)
        return *value;
    if (case_ == 1)
    {
        value = l_and_exp->getValue();
    }
    else if (case_ == 2)
    {
        value = l_or_exp->getValue() || l_and_exp->getValue();
    }
    return *value;
}
bool LOrExpAST::loadIdent()
{
    if (case_ == 1)
    {
        return l_and_exp->loadIdent();
    }
    else
    {
        return l_or_exp->loadIdent() | l_and_exp->loadIdent();
    }
}
bool LOrExpAST::isNumber()
{
    if (case_ == 1)
        return l_and_exp->isNumber();
    return false;
}

void DeclAST::Koopa() const
{
    const_decl_or_var_decl->Koopa();
}

void ConstDeclAST::Koopa() const
{
    for (auto &const_def : *const_defs)
    {
        const_def->Koopa();
    }
}

void globalPrintAggregate(std::vector<int> vec, std::vector<int> aggregate, int begin, int len)
{
    bool comma = 0;
    std::cout << "{";
    if (len != vec[0])
    {
        len /= vec.back();
        auto part_vec = std::vector<int>(vec.begin(), vec.end() - 1);
        for (int i = 0; i < vec.back(); i++)
        {
            if (comma)
                std::cout << ", ";
            else
                comma = 1;
            globalPrintAggregate(part_vec, aggregate, begin, len);
            begin += len;
        }
    }
    else
    {
        for (int i = begin; i < begin + len; i++)
        {
            if (comma)
                std::cout << ", ";
            else
                comma = 1;
            std::cout << aggregate[i];
        }
    }
    std::cout << "}";
}

void storeAggregate(std::string ident, std::vector<int> vec, std::vector<int> aggregate, int begin, int len, bool load)
{
    int parent = reg - 1; // 储存上一位数组的地址
    if (len != 1)
    {
        auto part_vec = std::vector<int>(vec.begin(), vec.end() - 1);
        len /= vec.back();
        for (int i = 0; i < vec.back(); i++)
        {
            std::cout << "  %" << reg << " = getelemptr ";
            if (len * vec.back() == aggregate.size() && !load)
            {
                std::cout << "@" << getVar(ident) << ", " << i << std::endl;
            }
            else
            {
                std::cout << "%" << parent << ", " << i << std::endl;
            }
            reg++;
            storeAggregate(ident, part_vec, aggregate, begin, len, 1);
            begin += len;
        }
    }
    else
    {
        if (!vec.empty()) // 处理a[1][1][...]的情况
        {                 // vec.front==1
            if (!load)
            {
                std::cout << "  %" << reg << " = getelemptr @" << getVar(ident) << ", 0" << std::endl;
                load = 1;
            }
            else
            {
                std::cout << "  %" << reg << " = getelemptr %" << parent << ", 0" << std::endl;
            }
            reg++;
            storeAggregate(ident, std::vector<int>(vec.begin(), vec.end() - 1), aggregate, begin, len, 1);
        }
        else
        {
            std::cout << "  store " << aggregate[begin] << ", %" << reg - 1 << std::endl;
        }
    }
}

void ConstDefAST::Koopa() const
{
    if (index->empty())
    {
        sym_table.insert(ident, CONST_INT, const_init_val->getValue());
    }
    else
    { // 数组
        sym_table.insert(ident, CONST_ARRAY, index->size());
        if (!global_decl)
        {
            std::cout << "  ";
        }
        else
            std::cout << "global ";
        std::cout << "@" << getVar(ident) << "= alloc ";
        for (auto &i : *index)
        {
            (void)i;
            std::cout << '[';
        }
        std::cout << "i32";
        std::vector<int> vec; // 内容是反过来的
        for (int i = index->size() - 1; i >= 0; i--)
        {
            std::cout << ", ";
            vec.push_back(index->at(i)->getValue());
            std::cout << vec.back();
            std::cout << ']';
        }
        auto constInitVal = dynamic_cast<ConstInitValAST *>(const_init_val.get());
        auto aggregate = constInitVal->InitArray(vec);
        if (global_decl)
        {
            std::cout << ",";
            globalPrintAggregate(vec, aggregate, 0, aggregate.size());
            std::cout << std::endl;
        }
        else
        {
            std::cout << std::endl;
            storeAggregate(ident, vec, aggregate, 0, aggregate.size(), 0);
        }
    }
}

std::vector<int> ConstInitValAST::InitArray(std::vector<int> vec) const
{
    std::vector<int> content;
    for (auto &init_val : *const_init_vals)
    {
        auto initVal = dynamic_cast<ConstInitValAST *>(init_val.get());
        if (initVal->case_ == 1)
        {
            content.push_back(initVal->getValue());
        }
        else
        {
            if (content.size() % vec[0] == 0)
            {
                int len = content.size() / vec[0], i = 1;
                if (len != 0)
                    for (i = 1; i < vec.size(); i++)
                    {
                        if (len % vec[i] != 0)
                        {
                            break;
                        }
                        len /= vec[i];
                    }
                std::vector<int> part_vec(vec.begin(), vec.end()-vec.size()+i); // 检查对齐到哪一个边界
                auto temp = initVal->InitArray(part_vec);              // 递归处理
                content.insert(content.end(), temp.begin(), temp.end());
            }
            else
            {
                std::cerr << "Error: 初始化列表没有对齐数组维度的边界. Exiting." << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
    }
    int length = 1; // 数组总长度
    for (auto &i : vec)
    {
        length *= i;
    }
    content.resize(length, 0); // 补0
    return content;
}

void ConstInitValAST::Koopa() const
{
    const_exp->Koopa();
}
int ConstInitValAST::getValue()
{
    if (value)
        return *value;
    value = const_exp->getValue();
    return *value;
}
bool ConstInitValAST::loadIdent()
{
    // return const_exp->loadIdent();
    return false;
}
bool ConstInitValAST::isNumber()
{
    if (case_ == 1)
        return const_exp->isNumber();
    return false;
}

void LValAST::Koopa() const
{
    if (exp_index->empty()) // 可能为标识符或者指针
    {
        std::optional<symbol_info> info = sym_table.find(ident);
        if (!info)
        {
            std::cerr << "Error: Variable '" << ident << "' is not defined. Exiting." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (info->type == CONST_INT)
        {
            std::cout << *(info->value);
            return;
        }
        else if (info->type == VAR_INT)
        {
            if (info->value.has_value())
            {
                std::cout << "%" << *(info->value);
            }
        }
        else if (info->type == CONST_ARRAY || info->type == VAR_ARRAY)
        {
            std::cout << "  %" << reg << " = getelemptr @" << getVar(ident) << ", 0" << std::endl;
            reg++;
        }
        else if (info->type == PTR)
        {
            std::cout << "  %" << reg << " = load @" << getVar(ident) << std::endl;
            reg++;
            std::cout << "  %" << reg << " = getptr %" << reg - 1 << ", 0" << std::endl;
            reg++;
        }
    }
    else
    { // LVal          ::= IDENT {"[" Exp "]"};
        auto info = sym_table.find(ident);
        if (info->type == CONST_ARRAY || info->type == VAR_ARRAY)
        {
            for (auto &exp : *exp_index)
            {
                int parent = reg - 1;
                if (!exp->loadIdent())
                {
                    std::cout << "  %" << reg << " = getelemptr ";
                    if (exp == exp_index->front())
                    {
                        std::cout << "@" << getVar(ident) << ", ";
                    }
                    else
                    {
                        std::cout << "%" << parent << ", ";
                    }
                    std::cout << exp->getValue() << std::endl;
                }
                else
                {
                    if (!exp->isNumber())
                        exp->Koopa();
                    std::cout << "  %" << reg << " = getelemptr ";
                    if (exp == exp_index->front())
                    {
                        std::cout << "@" << getVar(ident) << ", ";
                    }
                    else
                    {
                        std::cout << "%" << parent << ", ";
                    }
                    std::cout << "%" << reg - 1 << std::endl;
                }
                reg++;
            }
            if (exp_index->size() < info->value)
            {
                std::cout << "  %" << reg << " = getelemptr %" << reg - 1 << ", 0" << std::endl;
                reg++;
            }
        }
        else if (info->type == PTR)
        {
            std::cout << "  %" << reg << " = load @" << getVar(ident) << std::endl;
            reg++;
            auto &exp1 = exp_index->front();
            int parent = reg - 1;
            if (!exp1->loadIdent())
            {
                std::cout << "  %" << reg << " = getptr %" << parent << ", " << exp1->getValue() << std::endl;
            }
            else
            {
                if (!exp1->isNumber())
                    exp1->Koopa();
                std::cout << "  %" << reg << " = getptr %" << parent << ", %" << reg - 1 << std::endl;
            }
            reg++;
            if (exp_index->size() > 1)
            {
                for (auto it = exp_index->begin() + 1; it != exp_index->end(); it++)
                {
                    int parent = reg - 1;
                    if (!(*it)->loadIdent())
                    {
                        std::cout << "  %" << reg << " = getelemptr %" << parent << ", " << (*it)->getValue() << std::endl;
                    }
                    else
                    {
                        if (!(*it)->isNumber())
                            (*it)->Koopa();
                        std::cout << "  %" << reg << " = getelemptr %" << parent << ", %" << reg - 1 << std::endl;
                    }
                    reg++;
                }
            }
            if (exp_index->size() < info->value)
            {
                std::cout << "  %" << reg << " = getelemptr %" << reg - 1 << ", 0" << std::endl;
                reg++;
            }
        }
    }
}
int LValAST::getValue() // 得到常量标识符的值和变量标识符的寄存器地址
{
    std::optional<symbol_info> info = sym_table.find(ident);
    if (info->type == CONST_INT)
    {
        return *(info->value);
    }
    else if (info->type == VAR_INT)
    {
        if (info->value.has_value())
        {
            return *(info->value);
        }
    }
    return -1;
}
bool LValAST::loadIdent()
{
    std::optional<symbol_info> info = sym_table.find(ident);
    if (info->type == CONST_INT)
    {
        return false;
    }
    else if (info->type == VAR_INT)
    {
        if (info->value.has_value())
        {
            sym_table.updateValue(ident, reg);
            std::cout << "  %" << reg << " = load @" << getVar(ident) << std::endl;
            reg++; // debug

            return true; // 含有标识符
        }
        else
        {
            info->value = reg;
            sym_table.updateValue(ident, reg);
            std::cout << "  %" << reg << " = load @" << getVar(ident) << std::endl;
            reg++;
            return true;
        }
    }
    return true; // 并非只含有数字
}
bool LValAST::isNumber()
{
    std::optional<symbol_info> info = sym_table.find(ident);
    if (info->type == CONST_INT || info->type == VAR_INT)
    {
        return true;
    }
    return false;
}

void ConstExpAST::Koopa() const
{
    exp->Koopa();
}
int ConstExpAST::getValue()
{
    if (value)
        return *value;
    value = exp->getValue();
    return *value;
}
bool ConstExpAST::loadIdent()
{
    return exp->loadIdent();
}
bool ConstExpAST::isNumber()
{
    return exp->isNumber();
}

void VarDeclAST::Koopa() const
{
    for (auto &var_def : *var_defs)
    {
        var_def->Koopa();
    }
}

void globalPrintAggregate(std::vector<int> vec, std::vector<std::pair<int, bool>> aggregate, int begin, int len)
{ // 全局声明一定不包含标识符
    bool comma = 0;
    std::cout << "{";
    if (len != vec[0])
    {
        len /= vec.back();
        auto part_vec = std::vector<int>(vec.begin(), vec.end() - 1);
        for (int i = 0; i < vec.back(); i++)
        {
            if (comma)
                std::cout << ", ";
            else
                comma = 1;
            globalPrintAggregate(part_vec, aggregate, begin, len);
            begin += len;
        }
    }
    else
    {
        for (int i = begin; i < begin + len; i++)
        {
            if (comma)
                std::cout << ", ";
            else
                comma = 1;
            std::cout << aggregate[i].first;
        }
    }
    std::cout << "}";
}

void storeAggregate(std::string ident, std::vector<int> vec, std::vector<std::pair<int, bool>> aggregate, int begin, int len, bool load)
{
    int parent = reg - 1; // 储存上一维数组的地址
    if (len != 1)
    {
        auto part_vec = std::vector<int>(vec.begin(), vec.end() - 1);
        len /= vec.back();
        for (int i = 0; i < vec.back(); i++)
        {
            std::cout << "  %" << reg << " = getelemptr ";
            if (len * vec.back() == aggregate.size() && !load)
            // 如果数组为a[1][...]，则不需要再次getelemptr @a
            {
                std::cout << "@" << getVar(ident) << ", " << i << std::endl;
            }
            else
            {
                std::cout << "%" << parent << ", " << i << std::endl;
            }
            reg++;
            storeAggregate(ident, part_vec, aggregate, begin, len, 1);
            begin += len;
        }
    }
    else // len=1
    {
        if (!vec.empty()) // 处理a[1][1][...]的情况
        {                 // vec.front==1
            if (!load)
            {
                std::cout << "  %" << reg << " = getelemptr @" << getVar(ident) << ", 0" << std::endl;
                load = 1;
            }
            else
            {
                std::cout << "  %" << reg << " = getelemptr %" << parent << ", 0" << std::endl;
            }
            reg++;
            storeAggregate(ident, std::vector<int>(vec.begin(), vec.end() - 1), aggregate, begin, len, 1);
        }
        else
        {
            if (aggregate[begin].second)
            {
                std::cout << "  store %" << aggregate[begin].first << ", %" << reg - 1 << std::endl;
            }
            else
            {
                std::cout << "  store " << aggregate[begin].first << ", %" << reg - 1 << std::endl;
            }
        }
    }
}

void VarDefAST::Koopa() const
{
    if (index->empty())
    {
        sym_table.insert(ident, VAR_INT, std::nullopt);
        if (!global_decl)
        {
            std::cout << "  @" << getVar(ident) << " = alloc i32" << std::endl;
            if (case_ == 2)
            {
                // std::cout<<init_val->isNumber<<' '<<init_val->hasIdent<<std::endl;
                if (!init_val->loadIdent())
                {
                    std::cout << "  store " << init_val->getValue() << ", @" << getVar(ident) << std::endl;
                }
                else
                {
                    // init_val->loadIdent();
                    if (init_val->isNumber())
                    { // 只有一个标识符的情况
                        std::cout << "  store ";
                        init_val->Koopa();
                        std::cout << ", @" << getVar(ident) << std::endl;
                    }
                    else
                    {
                        init_val->Koopa();
                        std::cout << "  store %" << reg - 1 << ", @" << getVar(ident) << std::endl;
                    }
                }
            }
        }
        else
        {
            if (case_ == 1)
            {
                std::cout << "global @" << getVar(ident) << "= alloc i32, zeroinit" << std::endl;
            }
            else
            {
                if (!init_val->loadIdent())
                {
                    std::cout << "global @" << getVar(ident) << "= alloc i32, ";
                    std::cout << init_val->getValue() << std::endl;
                }
            }
        }
    }
    else
    {                                                      // 数组
        sym_table.insert(ident, VAR_ARRAY, index->size()); // 记录数组维数
        // init_val->loadIdent();//可能含有标识符
        if (!global_decl)
        {
            std::cout << "  ";
        }
        else
            std::cout << "global ";
        std::cout << "@" << getVar(ident) << "= alloc ";
        for (auto &i : *index)
        {
            (void)i;
            std::cout << '[';
        }
        std::cout << "i32";
        std::vector<int> vec; // 内容是反过来的
        for (int i = index->size() - 1; i >= 0; i--)
        {
            std::cout << ", ";
            vec.push_back(index->at(i)->getValue());
            std::cout << vec.back();
            std::cout << ']';
        }
        if (case_ == 1)
        {
            if (global_decl)
            {
                std::cout << ", zeroinit" << std::endl;
            }
            else
            {
                int length = 1; // 数组总长度
                for (auto &i : vec)
                {
                    length *= i;
                }
                auto aggregate = std::vector<std::pair<int, bool>>(length, std::make_pair(0, 0));
                std::cout << std::endl;
                storeAggregate(ident, vec, aggregate, 0, aggregate.size(), 0);
            }
        }
        else
        {
            auto initVal = dynamic_cast<InitValAST *>(init_val.get());
            if (global_decl)
            {
                auto aggregate = initVal->InitArray(vec);
                std::cout << ",";
                globalPrintAggregate(vec, aggregate, 0, aggregate.size());
                std::cout << std::endl;
            }
            else
            {
                std::cout << std::endl;
                auto aggregate = initVal->InitArray(vec);
                storeAggregate(ident, vec, aggregate, 0, aggregate.size(), 0);
            }
        }
    }
}

std::vector<std::pair<int, bool>> InitValAST::InitArray(std::vector<int> vec) const
{
    std::vector<std::pair<int, bool>> content; // first记录值，second记录是否是标识符
    for (auto &init_val : *init_vals)
    {
        auto initVal = dynamic_cast<InitValAST *>(init_val.get());
        if (initVal->case_ == 1) // ConstExp
        {
            if (!initVal->loadIdent())
            {
                content.push_back(std::make_pair(initVal->getValue(), 0));
            }
            else
            {
                if (!initVal->isNumber())
                { // 需要计算的表达式
                    initVal->Koopa();
                    content.push_back(std::make_pair(reg - 1, 1));
                }
                else
                { // 单个已经被加载过的标识符
                    content.push_back(std::make_pair(reg - 1, 1));
                }
            }
        }
        else
        {
            if (content.size() % vec[0] == 0)
            {
                int len = content.size() / vec[0], i = 1;
                if (len != 0)
                {
                    for (i = 1; i < vec.size(); i++)//根据当前数组内容的长度，检查对齐到哪个边界
                    {
                        if (len % vec[i] != 0)
                        {
                            break;
                        } 
                        len /= vec[i];
                    }
                }
                std::vector<int> part_vec(vec.begin(), vec.end() -vec.size()+i); // 检查对齐到哪一个边界
                auto temp = initVal->InitArray(part_vec);                        // 递归处理
                content.insert(content.end(), temp.begin(), temp.end());
                //std::cout<<"content.size():"<<content.size()<<std::endl;
            }
            else
            {
                std::cerr << "Error: 初始化列表没有对齐数组维度的边界. Exiting." << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
    }
    int length = 1; // 数组总长度
    for (auto &i : vec)
    {
        length *= i;
    }
    content.resize(length, std::make_pair(0, 0)); // 补0
    return content;
}

void InitValAST::Koopa() const
{
    exp->Koopa();
}
int InitValAST::getValue()
{
    if (value)
        return *value;
    value = exp->getValue();
    return *value;
}
bool InitValAST::loadIdent()
{
    if (case_ == 1)
        return exp->loadIdent();
    else
    {
        bool result = 0;
        for (auto &init_val : *init_vals)
        {
            result |= init_val->loadIdent();
        }
        return result;
    }
}
bool InitValAST::isNumber()
{
    if (case_ == 1)
        return exp->isNumber();
    return false;
}
