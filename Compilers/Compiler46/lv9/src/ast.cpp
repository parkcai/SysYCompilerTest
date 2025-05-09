#include "include/ast.hpp"
#include "include/symtab.hpp"

static int sym_cnt = 0;

static SymbolTable sym_tab;

void decl_IR()
{
    std::cout << "decl @getint(): i32" << std::endl;
    std::cout << "decl @getch(): i32" << std::endl;
    std::cout << "decl @getarray(*i32): i32" << std::endl;
    std::cout << "decl @putint(i32)" << std::endl;
    std::cout << "decl @putch(i32)" << std::endl;
    std::cout << "decl @putarray(i32, *i32)" << std::endl;
    std::cout << "decl @starttime()" << std::endl;
    std::cout << "decl @stoptime()" << std::endl;
    std::cout << std::endl;
}

void CompUnitAST::Dump() const {}

void CompUnitAST::IR()
{
    dbg_printf("in CompUnitAST\n");
    for (auto &unit : comp_units)
    {
        unit->IR();
    }
}

void DeclAST::Dump() const {}

void DeclAST::IR()
{
    dbg_printf("in DeclAST\n");
    if (StmtAST::has_jp)
    {
        return;
    }
    if (tag == Tag::CONST)
    {
        const_decl->IR();
    }
    else
    {
        var_decl->IR();
    }
}

void ConstDeclAST::Dump() const {}

void ConstDeclAST::IR()
{
    dbg_printf("in ConstDeclAST\n");
    for (auto &def : const_defs)
    {
        def->IR();
    }
}

void ConstDefAST::Dump() const {}

void ConstDefAST::IR()
{
    dbg_printf("in ConstDefAST\n");
    if (const_exps.empty())
    {
        const_init_val->IR();
        sym_tab.insert(ident, SymbolTag::CONST, const_init_val->symbol);
    }
    else
    {
        std::vector<int> dims;
        for (auto &exp : const_exps)
        {
            exp->IR();
            dims.emplace_back(atoi(exp->symbol.c_str()));
        }

        auto symbol = "@" + ident + "_" + std::to_string(sym_cnt++);
        sym_tab.insert(ident, SymbolTag::ARRAY, symbol, dims);

        std::vector<std::string> full_init_vals;
        fill_init_vals(const_init_val->const_init_vals, full_init_vals, true);

        if (sym_tab.in_global_scope())
        {
            std::cout << "global  " << symbol << " = alloc ";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << "[";
            }
            std::cout << "i32";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << ", " << const_exps[static_cast<int>(const_exps.size()) - i - 1]->symbol << "]";
            }
            std::cout << ", ";
            print_aggr(full_init_vals, 0);
            std::cout << std::endl;
        }
        else
        {
            std::cout << "  " << symbol << " = alloc ";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << "[";
            }
            std::cout << "i32";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << ", " << const_exps[static_cast<int>(const_exps.size()) - i - 1]->symbol << "]";
            }
            std::cout << std::endl;
            get_ptr_store_val(full_init_vals, symbol, 0);
        }
    }
}

int ConstDefAST::fill_init_vals(const std::vector<std::unique_ptr<ConstInitValAST>>
                                    &init_vals,
                                std::vector<std::string> &full_init_vals,
                                bool is_first)
{
    int brace_len = 1; // 当前大括号负责初始化的长度
    if (is_first)
    {
        for (auto &exp : const_exps)
        {
            brace_len *= atoi(exp->symbol.c_str());
        }
    }
    else
    {
        brace_len = aligned_len(static_cast<int>(full_init_vals.size()));
    }
    int cur_len = 0; // 当前大括号已填充长度

    for (auto &val : init_vals)
    {
        if (val->tag == ConstInitValAST::Tag::EXP)
        {
            val->IR();
            full_init_vals.emplace_back(val->symbol);
            cur_len++;
        }
        else
        {
            cur_len += fill_init_vals(val->const_init_vals, full_init_vals);
        }
    }

    for (; cur_len < brace_len; cur_len++)
    {
        full_init_vals.emplace_back("0");
    }

    return brace_len;
}

int ConstDefAST::aligned_len(int len)
{
    int result = 1;
    for (auto it = const_exps.rbegin(); it != const_exps.rend() - 1; ++it)
    {
        auto dim_len = atoi((*it)->symbol.c_str());
        if (len % (result * dim_len) != 0)
        {
            if (it == const_exps.rbegin())
            {
                assert(false);
            }
            break;
        }
        result *= dim_len;
    }
    return result;
}

void ConstDefAST::get_ptr_store_val(const std::vector<std::string> &full_init_vals,
                                    const std::string &symbol, int dim)
{
    if (dim == static_cast<int>(const_exps.size()))
    {
        std::cout << "  store " << full_init_vals[0] << ", " << symbol << std::endl;
        return;
    }

    auto dim_len = atoi(const_exps[dim]->symbol.c_str());
    for (int i = 0; i < dim_len; ++i)
    {
        auto ptr_sym = "%ptr_" + std::to_string(sym_cnt++);
        std::cout << "  " << ptr_sym << " = getelemptr " << symbol << ", " << i << std::endl;
        auto next_begin_idx = full_init_vals.size() / dim_len * i;
        auto next_end_idx = full_init_vals.size() / dim_len * (i + 1);
        std::vector<std::string> next_init_vals(full_init_vals.begin() + next_begin_idx,
                                                full_init_vals.begin() + next_end_idx);
        get_ptr_store_val(next_init_vals, ptr_sym, dim + 1);
    }
}

void ConstDefAST::print_aggr(const std::vector<std::string> &full_init_vals, int dim)
{
    std::cout << "{";
    bool is_first = true;
    auto dim_len = atoi(const_exps[dim]->symbol.c_str());
    auto sub_brace_len = full_init_vals.size() / dim_len;
    for (auto i = 0; i < dim_len; ++i)
    {
        if (is_first)
        {
            is_first = false;
        }
        else
        {
            std::cout << ", ";
        }

        if (dim == static_cast<int>(const_exps.size()) - 1)
        {
            std::cout << full_init_vals[i];
        }
        else
        {
            std::vector<std::string> sub_init_vals(full_init_vals.begin() + sub_brace_len * i,
                                                   full_init_vals.begin() + sub_brace_len * (i + 1));
            print_aggr(sub_init_vals, dim + 1);
        }
    }
    std::cout << "}";
}

void ConstInitValAST::Dump() const {}

void ConstInitValAST::IR()
{
    dbg_printf("in ConstInitValAST\n");
    const_exp->IR();
    is_const = const_exp->is_const;
    symbol = const_exp->symbol;
}

void VarDeclAST::Dump() const {}

void VarDeclAST::IR()
{
    dbg_printf("in VarDeclAST\n");
    for (auto &def : var_defs)
    {
        def->IR();
    }
}

void VarDefAST::Dump() const {}

void VarDefAST::IR()
{
    dbg_printf("in VarDefAST\n");
    if (const_exps.empty())
    {
        auto symbol = "@" + ident + "_" + std::to_string(sym_cnt++);
        sym_tab.insert(ident, SymbolTag::VAR, symbol);
        if (sym_tab.in_global_scope())
        {
            std::cout << "global " << symbol << " = alloc i32, ";
            if (init_val)
            {
                init_val->IR();
                std::cout << init_val->symbol << std::endl;
            }
            else
            {
                std::cout << "zeroinit" << std::endl;
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "  " << symbol << " = alloc i32" << std::endl;
            if (init_val)
            {
                init_val->IR();
                std::cout << "  store " << init_val->symbol
                          << ", " << symbol << std::endl;
            }
        }
    }
    else
    {
        std::vector<int> dims;
        for (auto &exp : const_exps)
        {
            exp->IR();
            dims.emplace_back(atoi(exp->symbol.c_str()));
        }

        auto symbol = "@" + ident + "_" + std::to_string(sym_cnt++);
        sym_tab.insert(ident, SymbolTag::ARRAY, symbol, dims);

        std::vector<std::string> full_init_vals;
        if (init_val)
        {
            fill_init_vals(init_val->init_vals, full_init_vals, true);
        }
        else
        {
            fill_init_vals({}, full_init_vals, true);
        }

        if (sym_tab.in_global_scope())
        {
            std::cout << "global  " << symbol << " = alloc ";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << "[";
            }
            std::cout << "i32";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << ", " << const_exps[static_cast<int>(const_exps.size()) - i - 1]->symbol << "]";
            }
            std::cout << ", ";
            if (init_val)
            {
                print_aggr(full_init_vals, 0);
            }
            else
            {
                std::cout << "zeroinit";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "  " << symbol << " = alloc ";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << "[";
            }
            std::cout << "i32";
            for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
            {
                std::cout << ", " << const_exps[static_cast<int>(const_exps.size()) - i - 1]->symbol << "]";
            }
            std::cout << std::endl;
            if (init_val)
            {
                get_ptr_store_val(full_init_vals, symbol, 0);
            }
        }
    }
    dbg_printf("out VarDefAST\n");
}

int VarDefAST::fill_init_vals(const std::vector<std::unique_ptr<InitValAST>>
                                  &init_vals,
                              std::vector<std::string> &full_init_vals,
                              bool is_first)
{
    int brace_len = 1; // 当前大括号负责初始化的长度
    if (is_first)
    {
        for (auto &exp : const_exps)
        {
            brace_len *= atoi(exp->symbol.c_str());
        }
    }
    else
    {
        brace_len = aligned_len(static_cast<int>(full_init_vals.size()));
    }
    int cur_len = 0; // 当前大括号已填充长度

    for (auto &val : init_vals)
    {
        if (val->tag == InitValAST::Tag::EXP)
        {
            val->IR();
            full_init_vals.emplace_back(val->symbol);
            cur_len++;
        }
        else
        {
            cur_len += fill_init_vals(val->init_vals, full_init_vals);
        }
    }

    for (; cur_len < brace_len; cur_len++)
    {
        full_init_vals.emplace_back("0");
    }

    return brace_len;
}

int VarDefAST::aligned_len(int len)
{
    int result = 1;
    for (auto it = const_exps.rbegin(); it != const_exps.rend() - 1; ++it)
    {
        auto dim_len = atoi((*it)->symbol.c_str());
        if (len % (result * dim_len) != 0)
        {
            if (it == const_exps.rbegin())
            {
                assert(false);
            }
            break;
        }
        result *= dim_len;
    }
    return result;
}

void VarDefAST::get_ptr_store_val(const std::vector<std::string> &full_init_vals,
                                  const std::string &symbol, int dim)
{
    if (dim == static_cast<int>(const_exps.size()))
    {
        std::cout << "  store " << full_init_vals[0] << ", " << symbol << std::endl;
        return;
    }

    auto dim_len = atoi(const_exps[dim]->symbol.c_str());
    for (int i = 0; i < dim_len; ++i)
    {
        auto ptr_sym = "%ptr_" + std::to_string(sym_cnt++);
        std::cout << "  " << ptr_sym << " = getelemptr " << symbol << ", " << i << std::endl;
        auto next_begin_idx = full_init_vals.size() / dim_len * i;
        auto next_end_idx = full_init_vals.size() / dim_len * (i + 1);
        std::vector<std::string> next_init_vals(full_init_vals.begin() + next_begin_idx,
                                                full_init_vals.begin() + next_end_idx);
        get_ptr_store_val(next_init_vals, ptr_sym, dim + 1);
    }
}

void VarDefAST::print_aggr(const std::vector<std::string> &full_init_vals, int dim)
{
    std::cout << "{";
    bool is_first = true;
    auto dim_len = atoi(const_exps[dim]->symbol.c_str());
    auto sub_brace_len = full_init_vals.size() / dim_len;
    for (auto i = 0; i < dim_len; ++i)
    {
        if (is_first)
        {
            is_first = false;
        }
        else
        {
            std::cout << ", ";
        }

        if (dim == static_cast<int>(const_exps.size()) - 1)
        {
            std::cout << full_init_vals[i];
        }
        else
        {
            std::vector<std::string> sub_init_vals(full_init_vals.begin() + sub_brace_len * i,
                                                   full_init_vals.begin() + sub_brace_len * (i + 1));
            print_aggr(sub_init_vals, dim + 1);
        }
    }
    std::cout << "}";
}

void InitValAST::Dump() const {}

void InitValAST::IR()
{
    dbg_printf("in InitValAST\n");
    exp->IR();
    is_const = exp->is_const;
    symbol = exp->symbol;
}

void FuncDefAST::Dump() const
{
    std::cout << "FuncDefAST { ";
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << "}";
}

void FuncDefAST::IR()
{
    dbg_printf("in FuncDefAST\n");
    auto symbol = "@" + ident;
    if (ident != "main")
    {
        symbol += "_" + std::to_string(sym_cnt++);
    }
    auto sym_tag = func_type->type == FuncTypeAST::Type::VOID ? SymbolTag::VOID
                                                              : SymbolTag::INT;
    assert(sym_tab.in_global_scope());
    sym_tab.insert(ident, sym_tag, symbol);

    sym_tab.push(); // 装函数参数符号
    std::cout << "fun " << symbol << "(";
    if (func_f_params)
    {
        func_f_params->IR();
    }
    std::cout << ")";
    func_type->IR();
    std::cout << " {" << std::endl;
    std::cout << "%entry:" << std::endl;
    StmtAST::has_jp = false;
    if (func_f_params)
    {
        sym_tab.push(); // 为了函数参数的符号表，装函数作用域内的符号
        for (auto &param : func_f_params->func_f_params)
        {
            auto sym_info = sym_tab[param->ident];
            auto symbol = "%" + param->ident + "_" + std::to_string(sym_cnt++);
            if (sym_info->tag == SymbolTag::VAR)
            {
                sym_tab.insert(param->ident, SymbolTag::VAR, symbol);
                std::cout << "  " << symbol << " = alloc i32" << std::endl;
                std::cout << "  store " << sym_info->symbol
                          << ", " << symbol << std::endl;
            }
            else if (sym_info->tag == SymbolTag::PTR)
            {
                /**
                 * 涉及数组参数的代码2
                 * 当ident对应变量类型为int*时，dims为空vector
                 */
                sym_tab.insert(param->ident, SymbolTag::PTR, symbol, sym_info->dims); // 注意这里的symbol在原指针基础上加了一个*
                std::cout << "  " << symbol << " = alloc *";
                for (int i = 0; i < static_cast<int>(sym_info->dims.size()); ++i)
                {
                    std::cout << "[";
                }
                std::cout << "i32";
                for (auto it = sym_info->dims.rbegin(); it != sym_info->dims.rend(); ++it)
                {
                    std::cout << ", " << *it << "]";
                }
                std::cout << std::endl;
                std::cout << "  store " << sym_info->symbol
                          << ", " << symbol << std::endl;
            }
        }
    }
    block->IR();
    if (!StmtAST::has_jp) // TODO int函数要补成return 0
    {
        std::cout << "  ret" << std::endl;
    }
    StmtAST::has_jp = false;
    std::cout << "}" << std::endl;
    std::cout << std::endl;
    if (func_f_params)
    {
        sym_tab.pop();
    }
    sym_tab.pop();
}

void FuncTypeAST::Dump() const {}

void FuncTypeAST::IR()
{
    dbg_printf("in FuncTypeAST\n");
    std::cout << type_ir[type];
    dbg_printf("not in FuncTypeAST\n");
}

void FuncFParamsAST::Dump() const {}

void FuncFParamsAST::IR()
{
    dbg_printf("in FuncFParamsAST\n");
    bool is_first = true;
    for (auto &param : func_f_params)
    {
        if (is_first)
        {
            is_first = false;
        }
        else
        {
            std::cout << ", ";
        }
        param->IR();
    }
}

void FuncFParamAST::Dump() const {}

void FuncFParamAST::IR()
{
    dbg_printf("in FuncFParamAST\n");
    if (tag == Tag::INT)
    {
        auto symbol = "@" + ident + "_" + std::to_string(sym_cnt++);
        sym_tab.insert(ident, SymbolTag::VAR, symbol);
        std::cout << symbol << ": i32";
    }
    else
    {
        /**
         * 涉及数组参数的代码1
         */
        std::vector<int> dims;
        for (auto &exp : const_exps)
        {
            exp->IR();
            dims.emplace_back(atoi(exp->symbol.c_str()));
        }

        auto symbol = "@" + ident + "_" + std::to_string(sym_cnt++);
        sym_tab.insert(ident, SymbolTag::PTR, symbol, dims);

        std::cout << symbol << ": *";
        for (int i = 0; i < static_cast<int>(const_exps.size()); ++i)
        {
            std::cout << "[";
        }
        std::cout << "i32";
        for (auto it = const_exps.rbegin(); it != const_exps.rend(); ++it)
        {
            std::cout << ", " << (*it)->symbol << "]";
        }
    }
}

void BlockAST::Dump() const
{
    std::cout << "BlockAST { ";
    for (auto &item : block_items)
    {
        item->Dump();
    }
    std::cout << " }";
}

void BlockAST::IR()
{
    dbg_printf("in BlockAST\n");
    sym_tab.push();
    for (auto &item : block_items)
    {
        item->IR();
    }
    sym_tab.pop();
}

void BlockItemAST::Dump() const {}

void BlockItemAST::IR()
{
    dbg_printf("in BlockItemAST\n");
    switch (tag)
    {
    case Tag::DECL:
        decl->IR();
        break;
    case Tag::STMT:
        stmt->IR();
        break;
    default:
        assert(false);
    }
}

void StmtAST::Dump() const
{
    std::cout << "StmtAST { ";
    exp->Dump();
    std::cout << " }";
}

void StmtAST::IR()
{
    dbg_printf("in StmtAST\n");
    if (has_jp)
    {
        return;
    }

    switch (tag)
    {
    case Tag::LVAL:
    {
        lval->IR();
        exp->IR();
        assert(!lval->is_const);
        std::cout << "  store " << exp->symbol << ", " << lval->loc_sym << std::endl;
        break;
    }

    case Tag::EXP:
    {
        if (exp)
        {
            exp->IR();
        }
        break;
    }

    case Tag::BLOCK:
    {
        block->IR();
        break;
    }

    case Tag::IF:
    {
        exp->IR(); // TODO 常数exp条件语句的消除
        auto cur_sym_cnt = sym_cnt++;
        if (else_stmt)
        {
            std::cout << "  br " << exp->symbol << ", %then_" << cur_sym_cnt
                      << ", %else_" << cur_sym_cnt << std::endl;
        }
        else
        {
            std::cout << "  br " << exp->symbol << ", %then_" << cur_sym_cnt
                      << ", %if_end_" << cur_sym_cnt << std::endl;
        }
        std::cout << std::endl;
        std::cout << "%then_" << cur_sym_cnt << ":" << std::endl;
        has_jp = false;
        if_stmt->IR();
        if (!has_jp)
        {
            std::cout << "  jump %if_end_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
        }
        if (else_stmt)
        {
            std::cout << "%else_" << cur_sym_cnt << ":" << std::endl;
            has_jp = false;
            else_stmt->IR();
            if (!has_jp)
            {
                std::cout << "  jump %if_end_" << cur_sym_cnt << std::endl;
                std::cout << std::endl;
            }
        }
        std::cout << "%if_end_" << cur_sym_cnt << ":" << std::endl;
        has_jp = false;
        break;
    }

    case Tag::WHILE:
    {
        auto cur_sym_cnt = sym_cnt++;
        std::cout << "  jump %while_entry_" << cur_sym_cnt << std::endl;
        std::cout << std::endl;
        std::cout << "%while_entry_" << cur_sym_cnt << ":" << std::endl;
        while_cnt_stk.push(cur_sym_cnt);
        exp->IR();
        std::cout << "  br " << exp->symbol
                  << ", %while_body_" << cur_sym_cnt
                  << ", %while_end_" << cur_sym_cnt << std::endl;
        std::cout << std::endl;
        std::cout << "%while_body_" << cur_sym_cnt << ":" << std::endl;
        has_jp = false;
        while_stmt->IR();
        if (!has_jp)
        {
            std::cout << "  jump %while_entry_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
        }
        std::cout << "%while_end_" << cur_sym_cnt << ":" << std::endl;
        while_cnt_stk.pop();
        has_jp = false;
        break;
    }

    case Tag::BREAK:
    {
        auto cur_sym_cnt = while_cnt_stk.top();
        std::cout << "  jump %while_end_" << cur_sym_cnt << std::endl;
        std::cout << std::endl;
        has_jp = true;
        break;
    }

    case Tag::CONTINUE:
    {
        auto cur_sym_cnt = while_cnt_stk.top();
        std::cout << "  jump %while_entry_" << cur_sym_cnt << std::endl;
        std::cout << std::endl;
        has_jp = true;
        break;
    }

    case Tag::RETURN:
    {
        if (exp)
        {
            exp->IR();
            std::cout << "  ret " << exp->symbol << std::endl;
        }
        else
        {
            std::cout << "  ret" << std::endl; // TODO int函数要补成return 0
        }
        std::cout << std::endl;
        has_jp = true;
        break;
    }

    default:
        assert(false);
    }
}

void ExpAST::Dump() const
{
    lor_exp->Dump();
}

void ExpAST::IR()
{
    dbg_printf("in ExpAST\n");
    lor_exp->IR();
    is_const = lor_exp->is_const;
    symbol = lor_exp->symbol;
    dbg_printf("not in exp\n");
}

void LValAST::Dump() const {}

void LValAST::IR()
{
    dbg_printf("in LValAST\n");
    auto sym_info = sym_tab[ident];
    switch (sym_info->tag)
    {
    case SymbolTag::CONST:
    {
        is_const = true;
        symbol = sym_info->symbol;
        break;
    }
    case SymbolTag::VAR:
    {
        is_const = false;
        symbol = "%" + std::to_string(sym_cnt++);
        std::cout << "  " << symbol << " = load " << sym_info->symbol << std::endl;
        loc_sym = sym_info->symbol;
        break;
    }
    // 涉及数组参数的代码3
    // 引用的数组只会是之前定义的局部和全局数组
    case SymbolTag::ARRAY:
    {
        is_const = false;
        for (auto &exp : exps)
        {
            exp->IR();
        }
        auto ptr_sym = sym_info->symbol;
        for (auto &exp : exps)
        {
            auto next_sym = "%ptr_" + std::to_string(sym_cnt++);
            std::cout << "  " << next_sym << " = getelemptr " << ptr_sym << ", " << exp->symbol << std::endl;
            ptr_sym = next_sym;
        }
        symbol = "%" + std::to_string(sym_cnt++);
        if (exps.size() == sym_info->dims.size())
        {
            std::cout << "  " << symbol << " = load " << ptr_sym << std::endl;
            loc_sym = ptr_sym;
        }
        else
        {
            std::cout << "  " << symbol << " = getelemptr " << ptr_sym << ", 0" << std::endl;
        }
        break;
    }
    case SymbolTag::PTR:
    {
        is_const = false;
        if (exps.empty())
        {
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = load " << sym_info->symbol << std::endl;
        }
        else
        {
            for (auto &exp : exps)
            {
                exp->IR();
            }
            auto ptr_sym = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << ptr_sym << " = load " << sym_info->symbol << std::endl;
            auto next_sym = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << next_sym << " = getptr " << ptr_sym << ", " << exps[0]->symbol << std::endl;
            ptr_sym = next_sym;
            for (int i = 1; i < static_cast<int>(exps.size()); ++i)
            {
                next_sym = "%" + std::to_string(sym_cnt++);
                std::cout << "  " << next_sym << " = getelemptr " << ptr_sym << ", " << exps[i]->symbol << std::endl;
                ptr_sym = next_sym;
            }
            symbol = "%" + std::to_string(sym_cnt++);
            if (exps.size() == sym_info->dims.size() + 1)
            {
                std::cout << "  " << symbol << " = load " << ptr_sym << std::endl;
                loc_sym = ptr_sym;
            }
            else
            {
                std::cout << "  " << symbol << " = getelemptr " << ptr_sym << ", 0" << std::endl;
            }
        }
        break;
    }
    }
}

void PrimaryExpAST::Dump() const
{
    if (tag == Tag::EXP)
    {
        std::cout << "(";
        exp->Dump();
        std::cout << ")";
    }
    else if (tag == Tag::NUMBER)
    {
        std::cout << number;
    }
}

void PrimaryExpAST::IR()
{
    dbg_printf("in PrimaryExpAST\n");

    if (tag == Tag::EXP)
    {
        dbg_printf("is exp\n");

        exp->IR();
        is_const = exp->is_const;
        symbol = exp->symbol;
    }
    else if (tag == Tag::LVAL)
    {
        // 只要程序没有语义错误，LVal就一定在符号表中
        // 看左值能不能编译期求值，如果能则symbol为其值，否则为其koopa ir符号
        // 要求符号表存左值与其值、符号
        lval->IR();
        is_const = lval->is_const;
        symbol = lval->symbol;
    }
    else
    {
        dbg_printf("is number\n");
        is_const = true;
        symbol = std::to_string(number);
    }
    dbg_printf("not here\n");
}

void UnaryExpAST::Dump() const
{
    if (tag == Tag::PRIMARY)
    {
        primary_exp->Dump();
    }
    else
    {
        std::cout << unary_op << " ";
        unary_exp->Dump();
    }
}

void UnaryExpAST::IR()
{
    dbg_printf("in UnaryExpAST\n");
    switch (tag)
    {
    case Tag::PRIMARY:
    {
        dbg_printf("is primary\n");
        primary_exp->IR();
        is_const = primary_exp->is_const;
        symbol = primary_exp->symbol;
        break;
    }
    case Tag::IDENT:
    {
        dbg_printf("is ident\n");
        if (func_r_params)
        {
            func_r_params->IR();
        }
        auto sym_info = sym_tab.find_in_global_scope(ident);
        if (sym_info->tag == SymbolTag::VOID)
        {
            std::cout << "  call " << sym_info->symbol << "(";
        }
        else
        {
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = call " << sym_info->symbol << "(";
        }
        if (func_r_params)
        {
            bool is_first = true;
            for (auto &exp : func_r_params->exps)
            {
                if (is_first)
                {
                    is_first = false;
                }
                else
                {
                    std::cout << ", ";
                }
                std::cout << exp->symbol;
            }
        }

        std::cout << ")" << std::endl;
        break;
    }
    case Tag::UNARY:
    {
        dbg_printf("is unary\n");
        unary_exp->IR();
        is_const = unary_exp->is_const;
        if (is_const)
        {
            int unary_val = atoi(unary_exp->symbol.c_str());
            if (unary_op == "-")
            {
                symbol = std::to_string(-unary_val);
            }
            else if (unary_op == "!")
            {
                symbol = std::to_string(!unary_val);
            }
            else
            {
                symbol = std::to_string(unary_val);
            }
        }
        else
        {
            if (unary_op == "+")
            {
                symbol = unary_exp->symbol;
            }
            else
            {
                symbol = "%" + std::to_string(sym_cnt++);
                std::cout << "  " << symbol << " = " << op_ir.at(unary_op)
                          << " 0, " << unary_exp->symbol << std::endl;
            }
        }
        break;
    default:
        assert(false);
    }
    }

    dbg_printf("not in unary\n");
}

void FuncRParamsAST::Dump() const {}

void FuncRParamsAST::IR()
{
    dbg_printf("in FuncRParamsAST\n");
    for (auto &exp : exps)
    {
        exp->IR();
    }
}

void MulExpAST::Dump() const {}

void MulExpAST::IR()
{
    dbg_printf("in MulExpAST\n");

    if (tag == Tag::UNARY)
    {
        unary_exp->IR();
        is_const = unary_exp->is_const;
        symbol = unary_exp->symbol;
    }
    else
    {
        unary_exp->IR();
        mul_exp->IR();
        is_const = mul_exp->is_const && unary_exp->is_const;
        if (is_const)
        {
            int mul_val = atoi(mul_exp->symbol.c_str());
            int unary_val = atoi(unary_exp->symbol.c_str());
            if (op == "*")
            {
                symbol = std::to_string(mul_val * unary_val);
            }
            else if (op == "/")
            {
                symbol = std::to_string(mul_val / unary_val);
            }
            else
            {
                symbol = std::to_string(mul_val % unary_val);
            }
        }
        else
        {
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = " << op_ir.at(op) << " "
                      << mul_exp->symbol << ", " << unary_exp->symbol << std::endl;
        }
    }
    dbg_printf("not in mul\n");
}

void AddExpAST::Dump() const {}

void AddExpAST::IR()
{
    dbg_printf("in AddExpAST\n");
    if (tag == Tag::MUL)
    {
        mul_exp->IR();
        is_const = mul_exp->is_const;
        symbol = mul_exp->symbol;
    }
    else
    {
        mul_exp->IR();
        add_exp->IR();
        is_const = add_exp->is_const && mul_exp->is_const;
        if (is_const)
        {
            int add_val = atoi(add_exp->symbol.c_str());
            int mul_val = atoi(mul_exp->symbol.c_str());
            if (op == "+")
            {
                symbol = std::to_string(add_val + mul_val);
            }
            else
            {
                symbol = std::to_string(add_val - mul_val);
            }
        }
        else
        {
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = " << op_ir.at(op) << " "
                      << add_exp->symbol << ", " << mul_exp->symbol << std::endl;
        }
    }
    dbg_printf("not in add\n");
}

void RelExpAST::Dump() const {}

void RelExpAST::IR()
{
    dbg_printf("in RelExpAST\n");
    if (tag == Tag::ADD)
    {
        add_exp->IR();
        is_const = add_exp->is_const;
        symbol = add_exp->symbol;
    }
    else
    {
        add_exp->IR();
        rel_exp->IR();
        is_const = rel_exp->is_const && add_exp->is_const;
        if (is_const)
        {
            int rel_val = atoi(rel_exp->symbol.c_str());
            int add_val = atoi(add_exp->symbol.c_str());
            if (op == "<")
            {
                symbol = std::to_string(rel_val < add_val);
            }
            else if (op == ">")
            {
                symbol = std::to_string(rel_val > add_val);
            }
            else if (op == "<=")
            {
                symbol = std::to_string(rel_val <= add_val);
            }
            else
            {
                symbol = std::to_string(rel_val >= add_val);
            }
        }
        else
        {
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = " << op_ir.at(op) << " "
                      << rel_exp->symbol << ", " << add_exp->symbol << std::endl;
        }
    }
}

void EqExpAST::Dump() const {}

void EqExpAST::IR()
{
    dbg_printf("in EqExpAST\n");
    if (tag == Tag::REL)
    {
        rel_exp->IR();
        is_const = rel_exp->is_const;
        symbol = rel_exp->symbol;
    }
    else
    {
        rel_exp->IR();
        eq_exp->IR();
        is_const = eq_exp->is_const && rel_exp->is_const;
        if (is_const)
        {
            int eq_val = atoi(eq_exp->symbol.c_str());
            int rel_val = atoi(rel_exp->symbol.c_str());
            if (op == "==")
            {
                symbol = std::to_string(eq_val == rel_val);
            }
            else
            {
                symbol = std::to_string(eq_val != rel_val);
            }
        }
        else
        {
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = " << op_ir.at(op) << " "
                      << eq_exp->symbol << ", " << rel_exp->symbol << std::endl;
        }
    }
}

void LAndExpAST::Dump() const {}

void LAndExpAST::IR()
{
    dbg_printf("in LAndExpAST\n");
    if (tag == Tag::EQ)
    {
        eq_exp->IR();
        is_const = eq_exp->is_const;
        symbol = eq_exp->symbol;
    }
    else
    {
        /**
         * 本质上做了这个操作:
         * int result = 0;
         * if (lhs == 1)
         * {
         *      result = rhs != 0;
         * }
         * 表达式的结果即是 result
         */
        land_exp->IR();
        if (land_exp->is_const)
        {
            bool land_true = atoi(land_exp->symbol.c_str());
            if (!land_true)
            {
                is_const = true;
                symbol = "0";
            }
            else
            {
                eq_exp->IR();
                is_const = eq_exp->is_const;
                if (is_const)
                {
                    bool eq_true = atoi(eq_exp->symbol.c_str());
                    symbol = std::to_string(eq_true);
                }
                else
                {
                    symbol = "%" + std::to_string(sym_cnt++);
                    std::cout << "  " << symbol << " = ne "
                              << eq_exp->symbol << ", 0" << std::endl;
                }
            }
        }
        else
        {
            is_const = false;
            auto cur_sym_cnt = sym_cnt++;
            auto res_sym = "%land_res_" + std::to_string(sym_cnt++);
            sym_tab.insert(res_sym, SymbolTag::VAR, res_sym);
            std::cout << "  " << res_sym << " = alloc i32" << std::endl;
            std::cout << "  store 0, " << res_sym << std::endl;
            std::cout << "  br " << land_exp->symbol
                      << ", %left_true_" << cur_sym_cnt
                      << ", %land_end_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
            std::cout << "%left_true_" << cur_sym_cnt << ":" << std::endl;
            eq_exp->IR();
            std::cout << "  store " << eq_exp->symbol
                      << ", " << res_sym << std::endl;
            std::cout << "  jump %land_end_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
            std::cout << "%land_end_" << cur_sym_cnt << ":" << std::endl;
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = load "
                      << res_sym << std::endl;
        }
    }
}

void LOrExpAST::Dump() const {}

void LOrExpAST::IR()
{
    dbg_printf("in LOrExpAST\n");
    if (tag == Tag::LAND)
    {
        land_exp->IR();
        is_const = land_exp->is_const;
        symbol = land_exp->symbol;
    }
    else
    {
        /**
         * 本质上做了这个操作:
         * int result = 1;
         * if (lhs == 0)
         * {
         *      result = rhs != 0;
         * }
         * 表达式的结果即是 result
         */
        lor_exp->IR();
        if (lor_exp->is_const)
        {
            bool lor_true = atoi(lor_exp->symbol.c_str());
            if (lor_true)
            {
                is_const = true;
                symbol = "1";
            }
            else
            {
                land_exp->IR();
                is_const = land_exp->is_const;
                if (is_const)
                {
                    bool land_true = atoi(land_exp->symbol.c_str());
                    symbol = std::to_string(land_true);
                }
                else
                {
                    symbol = "%" + std::to_string(sym_cnt++);
                    std::cout << "  " << symbol << " = ne "
                              << land_exp->symbol << ", 0" << std::endl;
                }
            }
        }
        else
        {
            is_const = false;
            auto cur_sym_cnt = sym_cnt++;
            auto res_sym = "%lor_res_" + std::to_string(sym_cnt++);
            sym_tab.insert(res_sym, SymbolTag::VAR, res_sym);
            std::cout << "  " << res_sym << " = alloc i32" << std::endl;
            std::cout << "  store 1, " << res_sym << std::endl;
            std::cout << "  br " << lor_exp->symbol
                      << ", %lor_end_" << cur_sym_cnt
                      << ", %left_false_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
            std::cout << "%left_false_" << cur_sym_cnt << ":" << std::endl;
            land_exp->IR();
            std::cout << "  store " << land_exp->symbol
                      << ", " << res_sym << std::endl;
            std::cout << "  jump %lor_end_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
            std::cout << "%lor_end_" << cur_sym_cnt << ":" << std::endl;
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = load "
                      << res_sym << std::endl;
        }
    }
}

void ConstExpAST::Dump() const {}

void ConstExpAST::IR()
{
    dbg_printf("in ConstExpAST\n");
    exp->IR();
    is_const = exp->is_const;
    symbol = exp->symbol;
    dbg_printf("not in ConstExpAST\n");
}