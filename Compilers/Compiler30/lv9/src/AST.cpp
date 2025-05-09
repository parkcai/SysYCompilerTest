#include "AST.h"
#include "SymbolTable.h"

inline std::ostream &tab(std::ostream &os) {
    return os << "  ";
}

static int entry_cnt = 0;
static int var_cnt = 0;
static int br_cnt = 0;
static int while_cnt = 0;
static int short_circuit = 0;
static bool ret = 0;
static bool assign_array = 0;
static bool assign_stmt = 0;

static std::unordered_map<std::string, std::string> op_to_string = {
    {"!", "eq"}, {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"}, {"==", "eq"}, {"!=", "ne"}, {">", "gt"}, {"<", "lt"}, {">=", "ge"}, {"<=", "le"}, {"&&", "and"}, {"||", "or"}};

inline std::string to_prefix(int idx) {
    return "%" + std::to_string(idx);
}

inline void allocIR(std::string ident) {
    std::cout << tab << alloc_str(ident) << " = alloc i32" << std::endl;
}

inline void storeIR(std::string n_str, std::string ident) {
    std::cout << tab << "store " << n_str << ", " << alloc_str(ident) << std::endl;
}

inline void jumpIR(std::string basic_block) {
    std::cout << tab << "jump " << basic_block << std::endl;
}

inline void loadIR(std::string ident) {
    std::cout << tab << "%" << var_cnt++ << " = load " << alloc_str(ident) << std::endl;
}

void operationIR(std::string first, std::string second, std::string op) {
    auto op_iter = op_to_string.find(op);
    if (op_iter == op_to_string.end()) {
        std::cerr << "err : " << op << " is not a legal operator" << std::endl;
    }
    std::string op_string = op_iter->second;
    std::cout << tab << "%" << var_cnt++ << " = " << op_string << " " << first << ", " << second << std::endl;
}

void expressionIR(int var_idx1, int var_idx2, int value1, int value2, 
                        int& var_idx, std::string op) {
    if (var_idx1 && var_idx2) {
        operationIR(to_prefix(var_idx1 - 1), to_prefix(var_idx2 - 1), op);
        var_idx = var_idx2 + 1;
    } else if (var_idx1 && !var_idx2) {
        operationIR(to_prefix(var_idx1 - 1), std::to_string(value2), op);
        var_idx = var_idx1 + 1;
    } else if (!var_idx1 && var_idx2) {
        operationIR(std::to_string(value1), to_prefix(var_idx2 - 1), op);
        var_idx = var_idx2 + 1;
    }
}

void branchIR(int var_idx, int value, int br_idx, bool else_exist) {
    std::cout << tab << "br ";
    if(var_idx)
        std::cout << to_prefix(var_idx - 1);
    else
        std::cout << value;
    if(else_exist)
        std::cout << ", %then_" << br_idx << ", %else_" << br_idx << std::endl;
    else
        std::cout << ", %then_" << br_idx << ", %end_" << br_idx << std::endl;
}

void zeroArray(std::string ident_ptr, int current_dimension, const std::vector<int> &size) {
    if (current_dimension >= size.size())
        return;
    for (int i = 0; i < size[current_dimension]; i++) {
        std::string new_ident_ptr = to_prefix(var_cnt++);
        std::cout << tab << new_ident_ptr + " = getelemptr " << ident_ptr << ", " << i << std::endl;
        zeroArray(new_ident_ptr, current_dimension + 1, size);
        if (current_dimension == size.size() - 1) {
            std::cout << tab << "store 0, " + new_ident_ptr << std::endl;
        }
    }
}

int constInitArray(std::string ident_ptr, int current_dimension, const std::vector<int> &size, 
                const std::unique_ptr<BaseAST> &init_val, int begin, int global) {
    if (current_dimension >= size.size())
        return 0;
    int end = 0;
    if (global == 1)
        std::cout << "{";
    std::string new_ident_ptr;
    if (global != 1)
        new_ident_ptr = to_prefix(var_cnt++);
    // auto init_val_list = dynamic_cast<InitValAST *>(init_val.get())->init_vals;
    if (current_dimension < size.size() - 1) {
        int current = begin;
        for (int i = 0; i < size[current_dimension]; i++) {
            if (global != 1) {
                std::cout << tab << new_ident_ptr << " = getelemptr " + ident_ptr << ", " << i << std::endl;
            }
            if (i && global == 1)
                std::cout << ", ";
            if (current >= dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals.size()) {
                constInitArray(new_ident_ptr, current_dimension + 1, size, init_val, current, global);
            } else if (dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[current].get())->const_exp) {
                current = constInitArray(new_ident_ptr, current_dimension + 1, size, init_val, current, global);
            } else {
                constInitArray(new_ident_ptr, current_dimension + 1, size, dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[current], 0, global);
                current++;
            }
            if (global != 1)
                new_ident_ptr = to_prefix(var_cnt++);
        }
        end = current;
    } else {
        for (int j = begin; j < begin + size[current_dimension]; j++) {
            if (global != 1) {
                std::cout << tab << new_ident_ptr << " = getelemptr " + ident_ptr << ", " << j - begin << std::endl;
            }
            if (j >= dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals.size()) {
                if (global == 1) {
                    if (j == begin)
                        std::cout << 0;
                    else
                        std::cout << ", 0";
                } else {
                    std::cout << tab << "store 0, " << new_ident_ptr << std::endl;
                }
            } else {
                if (dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[j].get())->const_exp) {
                    if (global == 1) {
                        if (j == begin) {
                            std::cout << dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[j].get())->const_exp->CompValue();
                        } else {
                            std::cout << ", ";
                            std::cout << dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[j].get())->const_exp->CompValue();
                        }
                    } else {
                        dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[j].get())->const_exp->GenerateIR();
                        int var_idx = dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[j].get())->const_exp->var_idx;
                        if (var_idx) {
                            std::cout << tab << "store " << to_prefix(var_idx - 1) << ", " << new_ident_ptr << std::endl;
                        } else {
                            std::cout << tab << "store " << dynamic_cast<ConstInitValAST *>(dynamic_cast<ConstInitValAST *>(init_val.get())->const_init_vals[j].get())->const_exp->CompValue()
                                      << ", " << new_ident_ptr << std::endl;
                        }
                    }
                } else {
                    end = j;
                    for (int k = j; k < begin + size[current_dimension]; k++) {
                        if (global != 1) {
                            std::cout << tab << new_ident_ptr << " = getelemptr " + ident_ptr << ", " << k - begin << std::endl;
                        }
                        if (global == 1) {
                            std::cout << ", 0";
                        } else {
                            std::cout << tab << "store 0, " << new_ident_ptr << std::endl;
                        }
                    }
                    break;
                }
            }
            end = j + 1;
            new_ident_ptr = to_prefix(var_cnt++);
        }
    }
    if (global == 1)
            std::cout << "}";
    return end;
}

int initArray(std::string ident_ptr, int current_dimension, const std::vector<int> &size, 
                const std::unique_ptr<BaseAST> &init_val, int begin, int global) {
    if (current_dimension >= size.size())
        return 0;
    int end = 0;
    if (global == 1)
        std::cout << "{";
    std::string new_ident_ptr;
    if (global != 1)
        new_ident_ptr = to_prefix(var_cnt++);
    // auto init_val_list = dynamic_cast<InitValAST *>(init_val.get())->init_vals;
    if (current_dimension < size.size() - 1) {
        int current = begin;
        for (int i = 0; i < size[current_dimension]; i++) {
            if (global != 1) {
                std::cout << tab << new_ident_ptr << " = getelemptr " + ident_ptr << ", " << i << std::endl;
            }
            if (i && global == 1)
                std::cout << ", ";
            if (current >= dynamic_cast<InitValAST *>(init_val.get())->init_vals.size()) {
                initArray(new_ident_ptr, current_dimension + 1, size, init_val, current, global);
            } else if (dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[current].get())->exp) {
                current = initArray(new_ident_ptr, current_dimension + 1, size, init_val, current, global);
            } else {
                initArray(new_ident_ptr, current_dimension + 1, size, dynamic_cast<InitValAST *>(init_val.get())->init_vals[current], 0, global);
                current++;
            }
            if (global != 1)
                new_ident_ptr = to_prefix(var_cnt++);
        }
        end = current;
    } else {
        for (int j = begin; j < begin + size[current_dimension]; j++) {
            if (global != 1) {
                std::cout << tab << new_ident_ptr << " = getelemptr " + ident_ptr << ", " << j - begin << std::endl;
            }
            if (j >= dynamic_cast<InitValAST *>(init_val.get())->init_vals.size()) {
                if (global == 1) {
                    if (j == begin)
                        std::cout << 0;
                    else
                        std::cout << ", 0";
                } else {
                    std::cout << tab << "store 0, " << new_ident_ptr << std::endl;
                }
            } else {
                if (dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[j].get())->exp) {
                    if (global == 1) {
                        if (j == begin) {
                            std::cout << dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[j].get())->exp->CompValue();
                        } else {
                            std::cout << ", ";
                            std::cout << dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[j].get())->exp->CompValue();
                        }
                    } else {
                        dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[j].get())->exp->GenerateIR();
                        int var_idx = dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[j].get())->exp->var_idx;
                        if (var_idx) {
                            std::cout << tab << "store " << to_prefix(var_idx - 1) << ", " << new_ident_ptr << std::endl;
                        } else {
                            std::cout << tab << "store " << dynamic_cast<InitValAST *>(dynamic_cast<InitValAST *>(init_val.get())->init_vals[j].get())->exp->CompValue()
                                      << ", " << new_ident_ptr << std::endl;
                        }
                    }
                } else {
                    end = j;
                    for (int k = j; k < begin + size[current_dimension]; k++) {
                        if (global != 1) {
                            std::cout << tab << new_ident_ptr << " = getelemptr " + ident_ptr << ", " << k - begin << std::endl;
                        }
                        if (global == 1) {
                            std::cout << ", 0";
                        } else {
                            std::cout << tab << "store 0, " << new_ident_ptr << std::endl;
                        }
                    }
                    break;
                }
            }
            end = j + 1;
            new_ident_ptr = to_prefix(var_cnt++);
        }
    }
    if (global == 1)
            std::cout << "}";
    return end;
}

void allocArray(std::string ident, int dimension, const std::vector<int> &size, 
                const std::unique_ptr<BaseAST> &init_val, int global, bool is_const) {
    std::string pre_str(dimension, '[');
    if (global == 1) {
        std::cout << "global " << alloc_str(ident) << " = alloc " << pre_str;
    } else {
        std::cout << tab << alloc_str(ident) << " = alloc " << pre_str;
    }
    for (int i = dimension - 1; i >= 0; i--) {
        if (i == dimension - 1) {
            std::cout << "i32, " << size[i] << "]";
        } else {
            std::cout << ", " << size[i] << "]";
        }
    }
    if (global == 1 && init_val == nullptr) {
        std::cout << ", zeroinit" << std::endl;
        return;
    }
    if (global == 1 && init_val) {
        std::cout << ", ";
        if (is_const)
            constInitArray(alloc_str(ident), 0, size, init_val, 0, 1);
        else
            initArray(alloc_str(ident), 0, size, init_val, 0, 1);
        std::cout << std::endl;
        return;
    }
    std::cout << std::endl;
    if (init_val && is_const) {
        constInitArray(alloc_str(ident), 0, size, init_val, 0, 0);
    } else if (init_val && !is_const) {
        initArray(alloc_str(ident), 0, size, init_val, 0, 0);
    } else {
        return;
        zeroArray(alloc_str(ident), 0, size);
    }
}

void declGlobalFunc() {
    func_insert("getint", Type::TYPE_FUN_INT);
    func_insert("getch", Type::TYPE_FUN_INT);
    func_insert("getarray", Type::TYPE_FUN_INT);
    func_insert("putint", Type::TYPE_FUN_VOID);
    func_insert("putch", Type::TYPE_FUN_VOID);
    func_insert("putarray", Type::TYPE_FUN_VOID);
    func_insert("starttime", Type::TYPE_FUN_VOID);
    func_insert("stoptime", Type::TYPE_FUN_VOID);
    std::cout << "decl @getint(): i32 \ndecl @getch(): i32 \ndecl @getarray(*i32): i32 \n"
              << "decl @putint(i32) \ndecl @putch(i32) \ndecl @putarray(i32, *i32) \n"
              << "decl @starttime() \ndecl @stoptime() \n"
              << std::endl;
}

// GenerateIR

void CompUnitAST::GenerateIR() const {
    declGlobalFunc();
    enter_new_scope();
    for (auto &decl_func : decl_func_list) {
        ret = 0;
        var_cnt = 0;
        //br_cnt = 0;
        //while_cnt = 0;
        //short_circuit = 0;
        reset_alloc_id();
        decl_func->GenerateIR();
    }
    exit_scope();
}

void DeclAST::GenerateIR() const {
    if (var_decl) {
        var_decl->GenerateIR();
    }
    if (const_decl) {
        const_decl->GenerateIR();
    }
}

void ConstDeclAST::GenerateIR() const {
    for (int i = 0; i < const_def.size(); i++) {
        const_def[i]->GenerateIR();
    }
}

void ConstDefAST::GenerateIR() const {
    int dimension = const_exps.size();
    if (dimension) {
        insert_symbol(ident, Type::TYPE_ARRAY, 0, dimension);
        std::vector<int> size;
        for (int i = 0; i < dimension; i++) {
            size.push_back(const_exps[i]->CompValue());
        }
        allocArray(ident, dimension, size, const_init_val, current_scope(), 1);
    } else {
        insert_symbol(ident, Type::TYPE_CONST, CompValue());
    }
}

void VarDeclAST::GenerateIR() const {
    for (int i = 0; i < var_def.size(); i++) {
        var_def[i]->GenerateIR();
    }
}

void VarDefAST::GenerateIR() const {
    int dimension = const_exps.size();
    if (dimension) {
        insert_symbol(ident, Type::TYPE_ARRAY, 0, dimension);
        std::vector<int> size;
        for (int i = 0; i < dimension; i++) {
            size.push_back(const_exps[i]->CompValue());
        }
        allocArray(ident, dimension, size, init_val, current_scope(), 0);
        return;
    }
    int value = 0;
    if (init_val) {
        init_val->GenerateIR();
        //value = CompValue();
        var_idx = init_val->var_idx;
        insert_symbol(ident, Type::TYPE_VAR, value);
        if (current_scope() == 1) {
            std::cout << "global @" << ident << "_1 = alloc i32, " << CompValue() << std::endl;
            return;
        }
        allocIR(ident);
        if(var_idx)
            storeIR(to_prefix(var_idx - 1), ident);
        else
            storeIR(std::to_string(CompValue()), ident);
        return;
    }
    insert_symbol(ident, Type::TYPE_VAR, value);
    if (current_scope() == 1) {
        std::cout << "global @" << ident << "_1 = alloc i32, zeroinit" << std::endl;
        return;
    }
    allocIR(ident);
}

void InitValAST::GenerateIR() const {
    exp->GenerateIR();
    var_idx = exp->var_idx;
}

void FuncDefAST::GenerateIR() const {
    if (func_type == "int")
        func_insert(ident, Type::TYPE_FUN_INT);
    else
        func_insert(ident, Type::TYPE_FUN_VOID);
    std::cout << "fun @" << ident << "(";
    for (int i = 0; i < func_params.size(); i++) {
        if (i)
            std::cout << ", ";
        dynamic_cast<FuncFParamAST *>(func_params[i].get())->FParamIR();
    }
    std::cout << ")";
    if (func_type == "int") {
        std::cout << ": i32 ";
    } else if (func_type == "void") {
        std::cout << " ";
    }
    std::cout << "{" << std::endl;
    enter_new_scope();
    std::cout << "%entry_" << ++entry_cnt << ":" << std::endl;
    for (auto &func_param : func_params) {
        func_param->GenerateIR();
    }
    block->GenerateIR();
    exit_scope();
    if (!ret) {
        if (func_type == "int")
            std::cout << tab << "ret 0" << std::endl;
        else
            std::cout << tab << "ret" << std::endl;
    }
    std::cout << "}" << std::endl;
}

void FuncFParamAST::FParamIR() const {
    if (type == "int") {
        std::cout << "@" + ident + ": i32";
        return;
    }
    std::cout << "@" + ident + ": ";
    final_type = "*";
    for (int i = const_exps.size() - 1; i >= 0; i--) {
       final_type += "[";
    }
    final_type += "i32";
    for (int i = const_exps.size() - 1; i >= 0; i--) {
        final_type += ", " + std::to_string(const_exps[i]->CompValue()) + "]";
    }
    std::cout << final_type;
}

void FuncFParamAST::GenerateIR() const {
    if (type == "int") {
        insert_symbol(ident, Type::TYPE_VAR, 0);
        allocIR(ident);
        storeIR("@" + ident, ident);
        return;
    }
    if (type == "array") {
        insert_symbol(ident, Type::TYPE_PTR, 0, const_exps.size() + 1);
        std::cout << tab << alloc_str(ident) << " = alloc " + final_type << std::endl;
        storeIR("@" + ident, ident);
    }
}

void BlockAST::GenerateIR() const {
    if (ret)
        return;
    for (int i = 0; i < block_items.size(); i++) {
        dynamic_cast<BlockItemAST *> (block_items[i].get())->while_idx = while_idx;
        block_items[i]->GenerateIR();
    }
}

void BlockItemAST::GenerateIR() const {
    if(ret)
        return;
    if (stmt) {
        dynamic_cast<StmtAST *> (stmt.get())->while_idx = while_idx;
        stmt->GenerateIR();
    }
    if (decl) {
        decl->GenerateIR();
    }
}

void StmtAST::GenerateIR() const {
    if (matched_stmt) {
        dynamic_cast<MatchedStmtAST *> (matched_stmt.get())->while_idx = while_idx;
        matched_stmt->GenerateIR();
    } else if (open_stmt) {
        dynamic_cast<OpenStmtAST *> (open_stmt.get())->while_idx = while_idx;
        open_stmt->GenerateIR();
    }
}

void MatchedStmtAST::GenerateIR() const {
    if (return_stmt) {
        return_stmt->GenerateIR();
    } else if (assign_stmt) {
        assign_stmt->GenerateIR();
    } else if (exp_stmt) {
        exp_stmt->GenerateIR();
    } else if (block) {
        dynamic_cast<BlockAST *> (block.get())->while_idx = while_idx;
        enter_new_scope();
        block->GenerateIR();
        exit_scope();
    } else if (while_stmt) {
        while_stmt->GenerateIR();
    } else if (while_jump_stmt) {
        dynamic_cast<WhileJumpStmtAST *> (while_jump_stmt.get())->while_idx = while_idx;
        while_jump_stmt->GenerateIR();
    } else if (cond) {
        dynamic_cast<MatchedStmtAST *> (then_stmt.get())->while_idx = while_idx;
        dynamic_cast<MatchedStmtAST *> (else_stmt.get())->while_idx = while_idx;
        cond->GenerateIR();
        int value = CompValue();
        var_idx = cond->var_idx;

        int br_idx = ++br_cnt;
        branchIR(var_idx, value, br_idx, 1);

        ret = 0;
        std::cout << "%then_" << br_idx << ":" << std::endl;
        then_stmt->GenerateIR();
        if (!ret)
            jumpIR("%end_" + std::to_string(br_idx));
        
        ret = 0;
        std::cout << "%else_" << br_idx << ":" << std::endl;
        else_stmt->GenerateIR();
        if (!ret)
            jumpIR("%end_" + std::to_string(br_idx));
        std::cout << "%end_" << br_idx << ":" << std::endl;

        ret = 0;
    }  
}

void OpenStmtAST::GenerateIR() const {
    cond->GenerateIR();
    int value = CompValue();
    var_idx = cond->var_idx;
    int br_idx = ++br_cnt;

    if (stmt) {
        dynamic_cast<StmtAST *> (stmt.get())->while_idx = while_idx;
        branchIR(var_idx, value, br_idx, 0);
        ret = 0;
        std::cout << "%then_" << br_idx << ":" << std::endl;
        stmt->GenerateIR();
        if(!ret)
            jumpIR("%end_" + std::to_string(br_idx));
        std::cout << "%end_" << br_idx << ":" << std::endl;
    } else {
        dynamic_cast<MatchedStmtAST *> (then_stmt.get())->while_idx = while_idx;
        dynamic_cast<OpenStmtAST *> (else_stmt.get())->while_idx = while_idx;
        branchIR(var_idx, value, br_idx, 1);

        ret = 0;
        std::cout << "%then_" << br_idx << ":" << std::endl;
        then_stmt->GenerateIR();
        if (!ret)
            jumpIR("%end_" + std::to_string(br_idx));

        ret = 0;
        std::cout << "%else_" << br_idx << ":" << std::endl;
        else_stmt->GenerateIR();
        if (!ret)
            jumpIR("%end_" + std::to_string(br_idx));

        std::cout << "%end_" << br_idx << ":" << std::endl;
    }
    ret = 0;
}

void AssignStmtAST::GenerateIR() const {
    assign_stmt = 1;
    if (dynamic_cast<LValAST *>(lval.get())->exps.size()) {
        assign_array = 1;
    }
    lval->GenerateIR();
    assign_stmt = 0;
    exp->GenerateIR();
    var_idx = exp->var_idx;
    if (var_idx) {
        if (assign_array) {
            std::cout << tab << "store " << to_prefix(var_idx - 1) << ", " << to_prefix(lval->var_idx - 1) << std::endl;
        } else {
            storeIR(to_prefix(var_idx - 1), dynamic_cast<LValAST *>(lval.get())->ident);
        }
    } else {
        if (assign_array) {
            std::cout << tab << "store " << CompValue() << ", " << to_prefix(lval->var_idx - 1) << std::endl;
        } else {
            storeIR(std::to_string(CompValue()), dynamic_cast<LValAST *>(lval.get())->ident);
        }
    }
    assign_array = 0;
}

void ReturnStmtAST::GenerateIR() const {
    ret = 1;
    if (exp) {
        exp->GenerateIR();
        var_idx = exp->var_idx;
        if (var_idx) {
            std::cout << tab << "ret " << to_prefix(var_idx - 1)  << std::endl;
        } else
            std::cout << tab << "ret " << CompValue() << std::endl;
    } else
        std::cout << tab << "ret" << std::endl;
}

void ExpStmtAST::GenerateIR() const {
    if (exp) {
        exp->GenerateIR();
    }
}

void WhileStmtAST::GenerateIR() const {
    int while_idx = ++while_cnt;
    jumpIR("%while_entry_" + std::to_string(while_idx));
    std::cout << "%while_entry_" << while_idx << ":" << std::endl;
    cond->GenerateIR();
    var_idx = cond->var_idx;

    std::cout << tab << "br ";
    if (var_idx)
        std::cout << to_prefix(var_idx - 1);
    else
        std::cout << CompValue();
    std::cout << ", %while_body_" << while_idx << ", %while_end_" << while_idx << std::endl;
    std::cout << "%while_body_" << while_idx << ":" << std::endl;
    ret = 0;
    dynamic_cast<StmtAST *> (stmt.get())->while_idx = while_idx;
    stmt->GenerateIR();
    if (!ret)
        jumpIR("%while_entry_" + std::to_string(while_idx));
    std::cout << "%while_end_" << while_idx << ":" << std::endl;
    ret = 0;
}

void WhileJumpStmtAST::GenerateIR() const {
    ret = 1;
    if (type == "break") {
        jumpIR("%while_end_" + std::to_string(while_idx));
    } else if (type == "continue") {
        jumpIR("%while_entry_" + std::to_string(while_idx));
    }
}

void ExpAST::GenerateIR() const {
    lor_exp->GenerateIR();
    var_idx = lor_exp->var_idx;
}

void LOrExpAST::GenerateIR() const {
    if (lor_exp == nullptr) {
        land_exp->GenerateIR();
        var_idx = land_exp->var_idx;
    } else {
        lor_exp->GenerateIR();
        std::string str1 = to_prefix(lor_exp->var_idx - 1);
        if (!lor_exp->var_idx) {
            str1 = std::to_string(lor_exp->CompValue());
        }
        operationIR(str1, "0", "!=");
        int br_idx = ++short_circuit;
        std::cout << tab << "%short_circuit_" << br_idx << " = alloc i32" << std::endl;
        std::cout << tab << "store " << to_prefix(var_cnt - 1) << ", %short_circuit_" << br_idx << std::endl;
        std::cout << tab << "br " << to_prefix(var_cnt - 1) << ", %true_" << br_idx
                  << ", %false_" << br_idx << std::endl;
        std::cout << "%false_" << br_idx << ":" << std::endl;

        land_exp->GenerateIR();
        std::string str2 = to_prefix(land_exp->var_idx - 1);
        if (!land_exp->var_idx) {
            str2 = std::to_string(land_exp->CompValue());
        }
        operationIR(str2, "0", "!=");
        std::cout << tab << "store " << to_prefix(var_cnt - 1) << ", %short_circuit_" << br_idx << std::endl;
        jumpIR("%true_" + std::to_string(br_idx));
        std::cout << "%true_" << br_idx << ":" << std::endl;
        std::cout << tab << to_prefix(var_cnt++) << " = load %short_circuit_" << br_idx << std::endl;
        //jumpIR("%circuit_end_" + std::to_string(br_idx));
        //std::cout << "%circuit_end_" << br_idx << ":" << std::endl;
        var_idx = var_cnt;
    }
}

void LAndExpAST::GenerateIR() const {
    if (land_exp == nullptr) {
        eq_exp->GenerateIR();
        var_idx = eq_exp->var_idx;
    } else {
        land_exp->GenerateIR();
        std::string str1 = to_prefix(land_exp->var_idx - 1);
        if (!land_exp->var_idx) {
            str1 = std::to_string(land_exp->CompValue());
        }
        operationIR(str1, "0", "!=");
        int br_idx = ++short_circuit;
        std::cout << tab << "%short_circuit_" << br_idx << " = alloc i32" << std::endl;
        std::cout << tab << "store " << to_prefix(var_cnt - 1) << ", %short_circuit_" << br_idx << std::endl;
        std::cout << tab << "br " << to_prefix(var_cnt - 1) << ", %true_" << br_idx
                  << ", %false_" << br_idx << std::endl;
        std::cout << "%true_" << br_idx << ":" << std::endl;

        eq_exp->GenerateIR();
        std::string str2 = to_prefix(eq_exp->var_idx - 1);
        if (!eq_exp->var_idx) {
            str2 = std::to_string(eq_exp->CompValue());
        }
        operationIR(str2, "0", "!=");
        std::cout << tab << "store " << to_prefix(var_cnt - 1) << ", %short_circuit_" << br_idx << std::endl;
        jumpIR("%false_" + std::to_string(br_idx));
        std::cout << "%false_" << br_idx << ":" << std::endl;
        std::cout << tab << to_prefix(var_cnt++) << " = load %short_circuit_" << br_idx << std::endl;
        //jumpIR("%circuit_end_" + std::to_string(br_idx));
        //std::cout << "%circuit_end_" << br_idx << ":" << std::endl;
        var_idx = var_cnt;
    }
}

void EqExpAST::GenerateIR() const {
    if (eq_exp == nullptr) {
        rel_exp->GenerateIR();
        var_idx = rel_exp->var_idx;
    } else {
        eq_exp->GenerateIR();
        rel_exp->GenerateIR();
        int value1 = 0, value2 = 0;
        if (!eq_exp->var_idx)
            value1 = eq_exp->CompValue();
        if (!rel_exp->var_idx)
            value2 = rel_exp->CompValue();
        expressionIR(eq_exp->var_idx, rel_exp->var_idx, value1, value2, var_idx, eq_op);
    }
}

void RelExpAST::GenerateIR() const {
    if (rel_exp == nullptr) {
        add_exp->GenerateIR();
        var_idx = add_exp->var_idx;
    } else {
        rel_exp->GenerateIR();
        add_exp->GenerateIR();
        int value1 = 0, value2 = 0;
        if (!rel_exp->var_idx)
            value1 = rel_exp->CompValue();
        if (!add_exp->var_idx)
            value2 = add_exp->CompValue();
        expressionIR(rel_exp->var_idx, add_exp->var_idx, value1, value2, var_idx, rel_op);
    }
}

void AddExpAST::GenerateIR() const {
    if (add_exp == nullptr) {
        mul_exp->GenerateIR();
        var_idx = mul_exp->var_idx;
    } else {
        add_exp->GenerateIR();
        mul_exp->GenerateIR();
        int value1 = 0, value2 = 0;
        if (!add_exp->var_idx)
            value1 = add_exp->CompValue();
        if (!mul_exp->var_idx)
            value2 = mul_exp->CompValue();
        expressionIR(add_exp->var_idx, mul_exp->var_idx, value1, value2, var_idx, add_op);
    }
}

void MulExpAST::GenerateIR() const {
    if (mul_exp == nullptr) {
        unary_exp->GenerateIR();
        var_idx = unary_exp->var_idx;
    } else {
        mul_exp->GenerateIR();
        unary_exp->GenerateIR();
        int value1 = 0, value2 = 0;
        if (!mul_exp->var_idx)
            value1 = mul_exp->CompValue();
        if (!unary_exp->var_idx)
            value2 = unary_exp->CompValue();
        expressionIR(mul_exp->var_idx, unary_exp->var_idx, value1, value2, var_idx, mul_op);
    }
}

void UnaryExpAST::GenerateIR() const {
    if (primary_exp) {
        primary_exp->GenerateIR();
        var_idx = primary_exp->var_idx;
    } else if (unary_exp) {
        unary_exp->GenerateIR();
        var_idx = unary_exp->var_idx;
        if(var_idx) {
            operationIR("0", to_prefix(var_idx - 1), unary_op);
            var_idx++;
        }
    } else {
        int params = func_params.size();
        for (int i = 0; i < params; i++) {
            func_params[i]->GenerateIR();
        }
        Type type = func_type(ident);
        std::cout << tab;
        if (type == Type::TYPE_FUN_INT) {
            std::cout << "%" << var_cnt++ << " = ";
            var_idx = var_cnt;
        }
        std::cout << "call @" << ident << "(";
        for (int i = 0; i < params; i++) {
            if (i)
                std::cout << ", ";
            if (func_params[i]->var_idx) {
                std::cout << "%" << func_params[i]->var_idx - 1;
            } else {
                std::cout << func_params[i]->CompValue();
            }
        }
        std::cout << ")" << std::endl;
    }
}

void PrimaryExpAST::GenerateIR() const {
    if (exp) {
        exp->GenerateIR();
        var_idx = exp->var_idx;
    } else if (lval) {
        lval->GenerateIR();
        var_idx = lval->var_idx;
    }
}

void LValAST::GenerateIR() const {
    auto var = find_symbol(ident);
    if (var->type == Type::TYPE_CONST) {
        return;
    } else if (var->type == Type::TYPE_VAR) {
        if (assign_stmt && !assign_array) {
            return;
        }
        loadIR(var->ident);
        var_idx = var_cnt;
    } else if (var->type == Type::TYPE_ARRAY || var->type == Type::TYPE_PTR) {
        bool assigned = assign_stmt;
        assign_stmt = 0;
        std::string pre_str = alloc_str(ident);
        int dimension = exps.size();
        bool is_param = 0;
        if (dimension < var->dimension)
            is_param = 1;
        for (int i = 0; i < dimension; i++) {
            if (i == 0) {
                if (var->type == Type::TYPE_PTR) {
                    exps[0]->GenerateIR();
                    pre_str = to_prefix(var_cnt++);
                    std::cout << tab << pre_str << " = load " << alloc_str(ident) << std::endl;
                    std::string getOp = "getptr ";
                    if (exps[0]->var_idx) {
                        std::cout << tab << to_prefix(var_cnt++) << " = " + getOp
                                  << pre_str << ", " + to_prefix(exps[0]->var_idx - 1) << std::endl;
                    } else {
                        std::cout << tab << to_prefix(var_cnt++) << " = " + getOp
                                  << pre_str << ", " << exps[0]->CompValue() << std::endl;
                    }
                } else if (var->type == Type::TYPE_ARRAY) {
                    exps[0]->GenerateIR();
                    if (exps[0]->var_idx) {
                        std::cout << tab << to_prefix(var_cnt++) << " = getelemptr "
                                  << pre_str << ", " + to_prefix(exps[0]->var_idx - 1) << std::endl;
                    } else {
                        std::cout << tab << to_prefix(var_cnt++) << " = getelemptr "
                                  << pre_str << ", " << exps[0]->CompValue() << std::endl;
                    }
                }
            } else {
                exps[i]->GenerateIR();
                if (exps[i]->var_idx) {
                    std::cout << tab << to_prefix(var_cnt++) << " = getelemptr "
                              << pre_str << ", " + to_prefix(exps[i]->var_idx - 1) << std::endl;
                } else {
                    std::cout << tab << to_prefix(var_cnt++) << " = getelemptr "
                              << pre_str << ", " << exps[i]->CompValue() << std::endl;
                }
            }
            pre_str = to_prefix(var_cnt - 1);
            // var_idx = var_cnt;
        }
        if (is_param && dimension) {
            std::cout << tab << to_prefix(var_cnt++) << " = getelemptr " + pre_str + ", 0" << std::endl;
        }
        if (is_param && dimension == 0) {
            if (var->type == Type::TYPE_PTR) {
                std::cout << tab << to_prefix(var_cnt++) << " = load " << pre_str << std::endl;
            } else if (var->type == Type::TYPE_ARRAY) {
                std::cout << tab << to_prefix(var_cnt++) << " = getelemptr " << pre_str << ", 0" << std::endl;
            }
        }
        if (!assigned && dimension == var->dimension) {
            std::cout << tab << to_prefix(var_cnt++) << " = load " + pre_str << std::endl;
        }
        var_idx = var_cnt;
    }
}

// CompValue

int ConstDefAST::CompValue() const {
    if (const_exps.size() == 0)
        return const_init_val->CompValue();
}

int ConstInitValAST::CompValue() const {
    return const_exp->CompValue();
}

int ConstExpAST::CompValue() const {
    return exp->CompValue();
}

int VarDefAST::CompValue() const {
    return init_val->CompValue();
}

int InitValAST::CompValue() const {
    return exp->CompValue();
}

int MatchedStmtAST::CompValue() const {
    return cond->CompValue();
}

int OpenStmtAST::CompValue() const {
    return cond->CompValue();
}

int AssignStmtAST::CompValue() const {
    return exp->CompValue();
}

int ExpStmtAST::CompValue() const {
    return exp->CompValue();
}

int ReturnStmtAST::CompValue() const {
    return exp->CompValue();
}

int WhileStmtAST::CompValue() const {
    return cond->CompValue();
}

int ExpAST::CompValue() const {
    return lor_exp->CompValue();
}

int LOrExpAST::CompValue() const {
    if (lor_exp == nullptr) {
        return land_exp->CompValue();
    } else {
        int value1 = lor_exp->CompValue();
        if (value1)
            return 1;
        int value2 = land_exp->CompValue();

        return value1 || value2;
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int LAndExpAST::CompValue() const {
    if (land_exp == nullptr) {
        return eq_exp->CompValue();
    } else {
        int value1 = land_exp->CompValue();
        if (!value1)
            return 0;
        int value2 = eq_exp->CompValue();
        return value1 && value2;
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int EqExpAST::CompValue() const {
    if (eq_exp == nullptr) {
        return rel_exp->CompValue();
    } else {
        int value1 = eq_exp->CompValue();
        int value2 = rel_exp->CompValue();

        if (eq_op == "==") {
            return value1 == value2;
        }
        else if (eq_op == "!=") {
            return value1 != value2;
        }
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int RelExpAST::CompValue() const {
    if (rel_exp == nullptr) {
        return add_exp->CompValue();
    } else {
        int value1 = rel_exp->CompValue();
        int value2 = add_exp->CompValue();

        if (rel_op == ">=") {
            return value1 >= value2;
        } else if (rel_op == ">") {
            return value1 > value2;
        } else if (rel_op == "<=") {
            return value1 <= value2;
        } else if (rel_op == "<") {
            return value1 < value2;
        }
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int AddExpAST::CompValue() const {
    if (add_exp == nullptr) {
        return mul_exp->CompValue();
    } else {
        int value1 = add_exp->CompValue();
        int value2 = mul_exp->CompValue();

        if (add_op == "+") {
            return value1 + value2;
        }
        else if (add_op == "-") {
            return value1 - value2;
        }
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int MulExpAST::CompValue() const {
    if (mul_exp == nullptr) {
        return unary_exp->CompValue();
    } else {
        int value1 = mul_exp->CompValue();
        int value2 = unary_exp->CompValue();

        if (mul_op == "*") {
            return value1 * value2;
        } else if (mul_op == "/") {
            if (value2 == 0)
                return 0;
            return value1 / value2;
        } else if (mul_op == "%") {
            if (value2 == 0)
                return 0;
            return value1 % value2;
        }
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int UnaryExpAST::CompValue() const {
    if (primary_exp) {
        int value = primary_exp->CompValue();
        return value;
    } else if (unary_exp) {
        int value = unary_exp->CompValue();
        if (unary_op == "+") {
            return value;
        }
        else if (unary_op == "!") {
            return !value;
        }
        else if (unary_op == "-") {
            return -value;
        }
    } else {
        return 0;
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int PrimaryExpAST::CompValue() const {
    if (exp) {
        return exp->CompValue();
    } else if (lval) {
        return lval->CompValue();
    } else {
        return number;
    }
    std::cerr << "err" << std::endl;
    exit(1);
}

int LValAST::CompValue() const {
    auto var = find_symbol(ident);
    if (var->type == Type::TYPE_CONST) {
        return var->value;
    }
    return 0;
}
