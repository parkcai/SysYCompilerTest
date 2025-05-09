#include "ast.hpp"

void IRInfo::init() {
  current_id = 0;
}

std::string IRInfo::getNextID() {
  return "%" + std::to_string(current_id++);
}

std::string IRInfo::getLastID() {
  if (current_id == 0) {
    return "%0";
  }
  return "%" + std::to_string(current_id - 1);
}

void AST2IRConverter::init() {
  symbol_table = new SymbolTable();
  symbol_table->postfix = "";
  symbol_table->setParent(nullptr);
  symbol_table->setChild(nullptr);
  symbol_table->insertFunction("getint", "int");
  symbol_table->insertFunction("getch", "int");
  symbol_table->insertFunction("getarray", "int");
  symbol_table->insertFunction("putint", "void");
  symbol_table->insertFunction("putch", "void");
  symbol_table->insertFunction("putarray", "void");
  symbol_table->insertFunction("starttime", "void");
  symbol_table->insertFunction("stoptime", "void");
  symbol_table_num = 1;
  ir_info.init();
}

std::string AST2IRConverter::Convert(BaseAST *ast) {
  if (auto comp_unit = dynamic_cast<CompUnitAST *>(ast)) {
    std::string library_func_decl = "decl @getint(): i32\n"
                                    "decl @getch(): i32\n"
                                    "decl @getarray(*i32): i32\n"
                                    "decl @putint(i32)\n"
                                    "decl @putch(i32)\n"
                                    "decl @putarray(i32, *i32)\n"
                                    "decl @starttime()\n"
                                    "decl @stoptime()\n\n";
    return library_func_decl + ConvertCompUnit(comp_unit);
  } 
  else if (auto global_def = dynamic_cast<GlobalDefAST *>(ast)) {
    return ConvertGlobalDef(global_def);
  }
  else if (auto decl = dynamic_cast<DeclAST *>(ast)) {
    return ConvertDecl(decl);
  }
  else if (auto const_decl = dynamic_cast<ConstDeclAST *>(ast)) {
    return ConvertConstDecl(const_decl);
  }
  else if (auto const_def = dynamic_cast<ConstDefAST *>(ast)) {
    return ConvertConstDef(const_def);
  }
  else if (auto const_init_val = dynamic_cast<ConstInitValAST *>(ast)) {
    return ConvertConstInitVal(const_init_val);
  }
  else if (auto var_decl = dynamic_cast<VarDeclAST *>(ast)) {
    return ConvertVarDecl(var_decl);
  }
  else if (auto var_def = dynamic_cast<VarDefAST *>(ast)) {
    return ConvertVarDef(var_def);
  }
  else if (auto init_val = dynamic_cast<InitValAST *>(ast)) {
    return ConvertInitVal(init_val);
  }
  else if (auto func_def = dynamic_cast<FuncDefAST *>(ast)) {
    return ConvertFuncDef(func_def);
  } 
  else if (auto array_ident = dynamic_cast<ArrayIdentAST *>(ast)) {
    return ConvertArrayIdent(array_ident);
  }
  else if (auto func_fparams = dynamic_cast<FuncFParamsAST *>(ast)) {
    return ConvertFuncFParams(func_fparams);
  } 
  else if (auto func_fparam = dynamic_cast<FuncFParamAST *>(ast)) {
    return ConvertFuncFParam(func_fparam);
  }
  else if (auto func_fparam_type = dynamic_cast<FuncFParamTypeAST *>(ast)) {
    return ConvertFuncFParamType(func_fparam_type);
  }
  else if (auto block = dynamic_cast<BlockAST *>(ast)) {
    return ConvertBlock(block);
  } 
  else if (auto block = dynamic_cast<BlockItemAST *>(ast)) {
    return ConvertBlockItem(block);
  }
  else if (auto stmt_list = dynamic_cast<StmtAST *>(ast)) {
    return ConvertStmtList(stmt_list);
  } 
  else if (auto if_stmt = dynamic_cast<IfStmtAST *>(ast)) {
    return ConvertIfStmt(if_stmt);
  } 
  else if (auto else_stmt = dynamic_cast<ElseStmtAST *>(ast)) {
    return ConvertElseStmt(else_stmt);
  }
  else if (auto exp = dynamic_cast<ExpAST *>(ast)) {
    return ConvertExp(exp);
  }
  else if (auto lval = dynamic_cast<LValAST *>(ast)) {
    return ConvertLVal(lval);
  }
  else if (auto primary_exp = dynamic_cast<PrimaryExpAST *>(ast)) {
    return ConvertPrimaryExp(primary_exp);
  } 
  else if (auto unary_exp = dynamic_cast<UnaryExpAST *>(ast)) {
    return ConvertUnaryExp(unary_exp);
  } 
  else if (auto func_rparams = dynamic_cast<FuncRParamsAST *>(ast)) {
    return ConvertFuncRParams(func_rparams);
  }
  else if (auto mul_exp = dynamic_cast<MulExpAST *>(ast)) {
    return ConvertMulExp(mul_exp);
  } 
  else if (auto add_exp = dynamic_cast<AddExpAST *>(ast)) {
    return ConvertAddExp(add_exp);
  }
  else if (auto rel_exp = dynamic_cast<RelExpAST *>(ast)) {
    return ConvertRelExp(rel_exp);
  }
  else if (auto eq_exp = dynamic_cast<EqExpAST *>(ast)) {
    return ConvertEqExp(eq_exp);
  }
  else if (auto land_exp = dynamic_cast<LAndExpAST *>(ast)) {
    return ConvertLAndExp(land_exp);
  }
  else if (auto lor_exp = dynamic_cast<LOrExpAST *>(ast)) {
    return ConvertLOrExp(lor_exp);
  }
  else if (auto const_exp = dynamic_cast<ConstExpAST *>(ast)) {
    return ConvertConstExp(const_exp);
  }
  else {
    return "";
  }
}

std::string AST2IRConverter::ConvertCompUnit(CompUnitAST *comp_unit) {
  std::string ret;
  for (const auto &global_def : comp_unit->global_defs) {
    // type is DECL
    if (dynamic_cast<GlobalDefAST *>(global_def.get())->type == GlobalDefAST::Type::DECL) {
      ret += Convert(global_def.get());
    }
  }
  for (const auto &global_def : comp_unit->global_defs) {
    // type is FUNC_DEF
    if (dynamic_cast<GlobalDefAST *>(global_def.get())->type == GlobalDefAST::Type::FUNC_DEF) {
      has_ret = false;
      ret += Convert(global_def.get());
    }
  }
  return ret;
}

std::string AST2IRConverter::ConvertGlobalDef(GlobalDefAST *global_def) {
  if (global_def->type == GlobalDefAST::Type::FUNC_DEF) {
    return '\n' + Convert(global_def->func_def.get());
  }
  else if (global_def->type == GlobalDefAST::Type::DECL) {
    return Convert(global_def->decl.get());
  }
  else {
    return "error global_def type";
  }
}

std::string AST2IRConverter::ConvertFuncDef(FuncDefAST *func_def) {
  // Insert func ident to symbol table
  func_type = func_def->func_type;
  symbol_table->insertFunction(func_def->ident, func_type);

  // FuncFParams is a seperated symbol table layer
  EnterChildSymbolTable();
  std::string ir_code = "fun @" + func_def->ident + "(";
  if (func_def->func_fparams != nullptr) {
    ir_code += Convert(func_def->func_fparams.get());
  }
  ir_code += ")";
  ir_code += ConvertFuncType(func_type);
  ir_code += "{\n%entry:\n";
  // We need to do the alloc here
  if (func_def->func_fparams != nullptr) {
    for (const auto &func_fparam : dynamic_cast<FuncFParamsAST *>(func_def->func_fparams.get())->fparams) {
      std::string ident = dynamic_cast<FuncFParamAST *>(func_fparam.get())->ident;
      std::string postfix = symbol_table->getSymbolPostfix(ident);
      std::string type = dynamic_cast<FuncFParamAST *>(func_fparam.get())->type;
      ir_code += "\t@" + ident + postfix + " = alloc " + type + '\n';
      ir_code += "\tstore @" + ident + ", @" + ident + postfix + '\n';
    }
  }
  ir_code += Convert(func_def->block.get());
  ExitChildSymbolTable();
  if (!has_ret) {
    if (func_type == "int") {
      ir_code += "\tret 0\n";
    }
    else if (func_type == "void") {
      ir_code += "\tret\n";
    }
  }
  ir_code += "}\n";
  return ir_code;
}

std::string AST2IRConverter::ConvertFuncType(std::string func_type) {
  if (func_type == "int") {
    return ": i32 ";
  }
  else if (func_type == "void") {
    return " ";
  } 
  return "error type";
}

std::string AST2IRConverter::ConvertFuncFParams(FuncFParamsAST *func_fparams) {
  std::string ret;
  for (const auto &func_fparam : func_fparams->fparams) {
    if (func_fparam != func_fparams->fparams.front()) {
      ret += ", ";
    }
    ret += Convert(func_fparam.get());
  }
  return ret;
}

std::string AST2IRConverter::ConvertFuncFParam(FuncFParamAST *func_fparam) {
  std::string ret = "@" + func_fparam->ident + ": ", type;
  if (func_fparam->func_param_type != nullptr) {
    Convert(func_fparam->func_param_type.get());
    type = dynamic_cast<FuncFParamTypeAST *>(func_fparam->func_param_type.get())->type;
    int nested_layer = 1;
    FuncFParamTypeAST* func_fparam_type = dynamic_cast<FuncFParamTypeAST *>(func_fparam->func_param_type.get());
    while (func_fparam_type->func_param_type != nullptr) {
      nested_layer++;
      func_fparam_type = dynamic_cast<FuncFParamTypeAST *>(func_fparam_type->func_param_type.get());
    }
    symbol_table->insertParams(func_fparam->ident, nested_layer, true);
  }
  else {
    type = "i32";
    symbol_table->insertParams(func_fparam->ident, 0, false);
  }
  ret += type;
  func_fparam->type = type;
  return ret;
}

std::string AST2IRConverter::ConvertFuncFParamType(FuncFParamTypeAST *func_fparam_type) {
  std::string ret = "", const_exp_ret, cur_type;
  if (func_fparam_type->const_exp != nullptr) {
    ret = Convert(func_fparam_type->const_exp.get());
    const_exp_ret = dynamic_cast<ConstExpAST *>(func_fparam_type->const_exp.get())->ret_val;
    if (func_fparam_type->type == "") {
      cur_type = "[i32, " + const_exp_ret + "]";
    }
    else {
      cur_type = "[" + func_fparam_type->type + ", " + const_exp_ret + "]";
    }
    dynamic_cast<FuncFParamTypeAST *>(func_fparam_type->func_param_type.get())->type = cur_type;
    ret += Convert(func_fparam_type->func_param_type.get());
    func_fparam_type->type = dynamic_cast<FuncFParamTypeAST *>(func_fparam_type->func_param_type.get())->type;
  }
  else {
    if (func_fparam_type->type == "") {
      func_fparam_type->type = "i32";
    }
    func_fparam_type->type = "*" + func_fparam_type->type;
  }
  return ret;
}

std::string AST2IRConverter::ConvertBlock(BlockAST *block) {
  std::string ir_code;
  EnterChildSymbolTable();
  for (const auto &block_item : block->block_items) {
    if (has_ret) {
      break;
    }
    ir_code += Convert(block_item.get());
  }
  ExitChildSymbolTable();
  return ir_code;
}

std::string AST2IRConverter::ConvertBlockItem(BlockItemAST *block_item) {
  std::string ret;
  if (block_item->type == BlockItemAST::Type::DECL) {
    ret = Convert(block_item->decl.get());
  }
  else if (block_item->type == BlockItemAST::Type::STMT) {
    ret = Convert(block_item->stmt.get());
    block_item->ret_val = dynamic_cast<StmtAST *>(block_item->stmt.get())->ret_val;
  }
  else {
    return "error block_item type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertStmtList(StmtAST *stmt) {
  std::string ret;
  if (stmt->exp != nullptr) {
    ret = Convert(stmt->exp.get());
  }

  switch (stmt->type) {
    case StmtAST::Type::RETURN:
      if (func_type == "void") {
          ret += "\tret\n";
      }
      else if (func_type == "int") {
        if (stmt->exp != nullptr) {
          stmt->ret_val = dynamic_cast<ExpAST *>(stmt->exp.get())->ret_val;
          ret += "\tret " + stmt->ret_val + '\n';
        }
        else {
          ret += "\tret 0\n";
        }
      } 
      else {
        ret += "error: return type not found\n";
      }
      has_ret = true;
      break;
    case StmtAST::Type::IF: {
      std::string if_block_postfix = IfBlockPostfix();
      std::string if_block_name = "%then" + if_block_postfix;
      std::string else_block_name = "%else" + if_block_postfix;
      std::string end_block_name = "%end" + if_block_postfix;

      std::string exp_ret_val = dynamic_cast<ExpAST *>(stmt->exp.get())->ret_val;
      bool has_else_stmt = dynamic_cast<ElseStmtAST *>(stmt->else_stmt.get())->stmt != nullptr;
      bool if_has_ret = true;
      bool else_has_ret = true;

      if (has_else_stmt) {
        ret += "\tbr " + exp_ret_val + ", " + if_block_name + ", " + else_block_name + '\n';
      } else {
        ret += "\tbr " + exp_ret_val + ", " + if_block_name + ", " + end_block_name + '\n';
      }
      has_ret = false;
      ret += if_block_name + ":\n";
      ret += Convert(stmt->if_stmt.get());
      if_has_ret = has_ret;
      if (!has_ret) {
        ret += "\tjump " + end_block_name + '\n';
      }
      if (has_else_stmt) {
        has_ret = false;
        ret += else_block_name + ":\n";
        ret += Convert(stmt->else_stmt.get());
        else_has_ret = has_ret;
        if (!has_ret) {
          ret += "\tjump " + end_block_name + '\n';
        }
      }
      if (if_has_ret && else_has_ret && has_else_stmt) {
        has_ret = true;
      }
      else {
        has_ret = false;
        ret += end_block_name + ":\n";
      }
      break;
    }
    case StmtAST::Type::WHILE: {
      EnterWhileBlock();
      std::string while_block_postfix = WhileBlockPostfix();
      std::string while_block_entry_name = "%while_entry" + while_block_postfix;
      std::string while_block_body_name = "%while_body" + while_block_postfix;
      std::string while_block_end_name = "%while_end" + while_block_postfix;
      std::string ret_entry = "\tjump " + while_block_entry_name + '\n';
      ret_entry += while_block_entry_name + ":\n";
      ret = ret_entry + ret;

      std::string exp_ret_val = dynamic_cast<ExpAST *>(stmt->exp.get())->ret_val;
      ret += "\tbr " + exp_ret_val + ", " + while_block_body_name + ", " + while_block_end_name + '\n';
      ret += while_block_body_name + ":\n";
      has_ret = false;
      ret += Convert(stmt->while_stmt.get());
      if (!has_ret) {
        ret += "\tjump " + while_block_entry_name + '\n';
      }
      has_ret = false;
      ret += while_block_end_name + ":\n";
      ExitWhileBlock();
      break;
    }
    case StmtAST::Type::BREAK: {
      if (!InWhileBlock()) {
        ret += "error: break not in while block\n";
        break;
      }
      std::string while_block_postfix = WhileBlockPostfix();
      std::string while_block_end_name = "%while_end" + while_block_postfix;
      ret += "\tjump " + while_block_end_name + '\n';
      has_ret = true;
      break;
    }
    case StmtAST::Type::CONTINUE: {
      if (!InWhileBlock()) {
        ret += "error: continue not in while block\n";
        break;
      }
      std::string while_block_postfix = WhileBlockPostfix();
      std::string while_block_entry_name = "%while_entry" + while_block_postfix;
      ret += "\tjump " + while_block_entry_name + '\n';
      has_ret = true;
      break;
    }
    case StmtAST::Type::LVAL: {
      // Note: we don't need to convert lval here
      std::string ident = getIdent(dynamic_cast<LValAST *>(stmt->lval.get()));
      bool ident_exists = symbol_table->varSymbolExists(ident, false);
      if (!ident_exists) {
        ret += "error: variable " + ident + " not declared\n";
      } else {
        std::string dst;
        stmt->ret_val = dynamic_cast<ExpAST *>(stmt->exp.get())->ret_val;
        ret += ConvertLValStore(dynamic_cast<LValAST *>(stmt->lval.get()));
        dst = dynamic_cast<LValAST *>(stmt->lval.get())->ret_val;
        ret += "\tstore " + stmt->ret_val + ", " + dst + '\n';
      }
      break;
    }
    case StmtAST::Type::BLOCK:
      ret += Convert(stmt->block.get());
      break;
    case StmtAST::Type::EXP:
      break;
    default:
      ret += "error stmt type";
      break;
  }
  return ret;
}

std::string AST2IRConverter::ConvertIfStmt(IfStmtAST *if_stmt) {
  std::string ret;
  ret += Convert(if_stmt->stmt.get());
  return ret;
}

std::string AST2IRConverter::ConvertElseStmt(ElseStmtAST *else_stmt) {
  std::string ret;
  ret += Convert(else_stmt->stmt.get());
  return ret;
}

std::string AST2IRConverter::ConvertArrayIdent(ArrayIdentAST *array_ident) {
  std::string ret = "";
  ret += Convert(array_ident->const_exp.get());
  std::string size = dynamic_cast<ConstExpAST *>(array_ident->const_exp.get())->ret_val;
  if (array_ident->array_ident != nullptr) {
    ret += Convert(array_ident->array_ident.get());
    std::string type = dynamic_cast<ArrayIdentAST *>(array_ident->array_ident.get())->type;
    std::string ident = dynamic_cast<ArrayIdentAST *>(array_ident->array_ident.get())->ident;
    // replace type i32 with [i32, size]: [i32, size_1] -> [[i32, size_2], size_1]
    size_t _pos = type.find("i32");
    array_ident->type = type.substr(0, _pos) + "[i32, " + size + "]" + type.substr(_pos + 3);
    array_ident->ident = ident;
  }
  else {
    array_ident->type = "[i32, " + size + "]";
  }

  return ret;
}

std::string AST2IRConverter::ConvertDecl(DeclAST *decl) {
  std::string ret;
  if (decl->type == DeclAST::Type::CONST_DECL) {
    ret = Convert(decl->const_decl.get());
  }
  else if (decl->type == DeclAST::Type::VAR_DECL) {
    ret = Convert(decl->var_decl.get());
  }
  else {
    ret = "error decl type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertConstDecl(ConstDeclAST *const_decl) {
  std::string ret;
  for (const auto &const_def : const_decl->const_def) {
    ret += Convert(const_def.get());
  }
  return ret;
}

std::string AST2IRConverter::ConvertConstDef(ConstDefAST *const_def) {
  std::string ret = "";
  if (const_def->type == ConstDefAST::Type::CONST_INT) {
    ret = Convert(const_def->const_init_val.get());
    int const_value = std::stoi(dynamic_cast<ConstInitValAST *>(const_def->const_init_val.get())->ret_val);
    symbol_table->insertConstant(const_def->ident, const_value);
  }
  else if (const_def->type == ConstDefAST::Type::CONST_ARRAY) {
    ret = Convert(const_def->array_ident.get());
    ret += Convert(const_def->const_init_val.get());
    std::string ident, type, value;
    ident = dynamic_cast<ArrayIdentAST *>(const_def->array_ident.get())->ident;
    type = dynamic_cast<ArrayIdentAST *>(const_def->array_ident.get())->type;
    value = dynamic_cast<ConstInitValAST *>(const_def->const_init_val.get())->ret_val;
    
    int nested_layer = 1;
    ArrayIdentAST* array_ident = dynamic_cast<ArrayIdentAST *>(const_def->array_ident.get());
    while (array_ident->array_ident != nullptr) {
      nested_layer++;
      array_ident = dynamic_cast<ArrayIdentAST *>(array_ident->array_ident.get());
    }
    symbol_table->insertVariable(ident, nested_layer, false, true);
    // create a new nested array
    std::unique_ptr<NestedArray> nested_array_ptr = std::make_unique<NestedArray>();
    NestedArray* nested_array = nested_array_ptr.get();
    nested_array->fromString(value);
    std::vector<int> array_size;
    nested_array->getArraySize(type, array_size);

    if (symbol_table->getParent() == nullptr) {
      std::string nested_array_str = zeroPaddingGlobal(nested_array, array_size);
      ret += "global @" + ident + "_glob = alloc " + type + ", " + nested_array_str + '\n';
    }
    else {
      ret += "\t@" + ident + symbol_table->postfix + " = alloc " + type + '\n';
      ret += zeroPadding(nested_array, array_size, "@" + ident + symbol_table->postfix);
    }
    //symbol_table->insertConstant(ident, 0);
  }
  else {
    return "error const_def type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertConstInitVal(ConstInitValAST *const_init_val) {
  std::string ret;
  if (const_init_val->type == ConstInitValAST::Type::CONST_EXP) {
    ret = Convert(const_init_val->const_exp.get());
    const_init_val->ret_val = dynamic_cast<ConstExpAST *>(const_init_val->const_exp.get())->ret_val;
    if (!dynamic_cast<ConstExpAST *>(const_init_val->const_exp.get())->is_const) {
      ret += "uninitialized const value";
    }
  }
  else if (const_init_val->type == ConstInitValAST::Type::CONST_ARRAY) {
    const_init_val->ret_val = "{";
    for (const auto &array_const_init_val : const_init_val->array_const_init_val) {
      ret += Convert(array_const_init_val.get());
      const_init_val->ret_val += dynamic_cast<ConstInitValAST *>(array_const_init_val.get())->ret_val;
      if (array_const_init_val != const_init_val->array_const_init_val.back()) {
        const_init_val->ret_val += ", ";
      }
    }
    const_init_val->ret_val += "}";
    return ret;
  }
  else {
    return "error const_init_val type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertVarDecl(VarDeclAST *var_decl) {
  std::string ret;
  for (const auto &var_def : var_decl->var_def) {
    ret += Convert(var_def.get());
  }
  return ret;
}

std::string AST2IRConverter::ConvertVarDef(VarDefAST *var_def) {
  std::string ret = "";
  if (var_def->type == VarDefAST::Type::INIT) {
    ret = Convert(var_def->init_val.get());
    std::string ret_val = dynamic_cast<InitValAST *>(var_def->init_val.get())->ret_val;
    symbol_table->insertVariable(var_def->ident, 0, false, false);
    // This is a global variable
    if (symbol_table->getParent() == nullptr) {
      ret += "global @" + var_def->ident + "_glob = alloc i32, " + ret_val + '\n';
    }
    else {
      ret += "\t@" + var_def->ident + symbol_table->postfix + " = alloc i32\n";
      ret += "\tstore " + ret_val + ", @" + var_def->ident + symbol_table->postfix + '\n';
    }
  }
  else if (var_def->type == VarDefAST::Type::UNINIT) {
    symbol_table->insertVariable(var_def->ident, 0, true, false);
    if (symbol_table->getParent() == nullptr) {
      ret += "global @" + var_def->ident + "_glob = alloc i32, zeroinit\n";
    }
    else {
      ret += "\t@" + var_def->ident + symbol_table->postfix + " = alloc i32\n";
    }
  }
  else if (var_def->type == VarDefAST::Type::ARRAY_INIT) {
    std::string ident, type, value;
    ret = Convert(var_def->array_ident.get());
    ret += Convert(var_def->init_val.get());
    ident = dynamic_cast<ArrayIdentAST *>(var_def->array_ident.get())->ident;
    type = dynamic_cast<ArrayIdentAST *>(var_def->array_ident.get())->type;
    value = dynamic_cast<InitValAST *>(var_def->init_val.get())->ret_val;

    int nested_layer = 1;
    ArrayIdentAST* array_ident = dynamic_cast<ArrayIdentAST *>(var_def->array_ident.get());
    while (array_ident->array_ident != nullptr) {
      nested_layer++;
      array_ident = dynamic_cast<ArrayIdentAST *>(array_ident->array_ident.get());
    }
    symbol_table->insertVariable(ident, nested_layer, false, true);
    // create a new nested array
    std::unique_ptr<NestedArray> nested_array_ptr = std::make_unique<NestedArray>();
    NestedArray* nested_array = nested_array_ptr.get();
    nested_array->fromString(value);
    std::vector<int> array_size;
    nested_array->getArraySize(type, array_size);

    if (symbol_table->getParent() == nullptr) {
      std::string nested_array_str = zeroPaddingGlobal(nested_array, array_size);
      ret += "global @" + ident + "_glob = alloc " + type + ", " + nested_array_str + '\n';
    }
    else {
      ret += "\t@" + ident + symbol_table->postfix + " = alloc " + type + '\n';
      ret += zeroPadding(nested_array, array_size, "@" + ident + symbol_table->postfix);
    }
  }
  else if (var_def->type == VarDefAST::Type::ARRAY_UNINIT) {
    std::string ident, type;
    ret = Convert(var_def->array_ident.get());
    ident = dynamic_cast<ArrayIdentAST *>(var_def->array_ident.get())->ident;
    type = dynamic_cast<ArrayIdentAST *>(var_def->array_ident.get())->type;

    int nested_layer = 1;
    ArrayIdentAST* array_ident = dynamic_cast<ArrayIdentAST *>(var_def->array_ident.get());
    while (array_ident->array_ident != nullptr) {
      nested_layer++;
      array_ident = dynamic_cast<ArrayIdentAST *>(array_ident->array_ident.get());
    }
    symbol_table->insertVariable(ident, nested_layer, false, true);
    // create a new nested array
    std::unique_ptr<NestedArray> nested_array_ptr = std::make_unique<NestedArray>();
    NestedArray* nested_array = nested_array_ptr.get();
    std::vector<int> array_size;
    nested_array->getArraySize(type, array_size);
    
    if (symbol_table->getParent() == nullptr) {
      ret += "global @" + ident + "_glob = alloc " + type + ", zeroinit\n";
    }
    else {
      ret += "\t@" + ident + symbol_table->postfix + " = alloc " + type + '\n';
      //ret += zeroPadding(nested_array, array_size, "@" + ident + symbol_table->postfix);
    }
  }
  else {
    ret += "error var_def type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertInitVal(InitValAST *init_val) {
  std::string ret;
  if (init_val->type == InitValAST::Type::EXP) {
    ret = Convert(init_val->exp.get());
    init_val->ret_val = dynamic_cast<ExpAST *>(init_val->exp.get())->ret_val;
  }
  else if (init_val->type == InitValAST::Type::ARRAY) {
    init_val->ret_val = "{";
    for (const auto &array_init_val : init_val->array_init_val) {
      ret += Convert(array_init_val.get());
      init_val->ret_val += dynamic_cast<InitValAST *>(array_init_val.get())->ret_val;
      if (array_init_val != init_val->array_init_val.back()) {
        init_val->ret_val += ", ";
      }
    }
    init_val->ret_val += "}";
  }
  else {
    return "error init_val type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertPrimaryExp(PrimaryExpAST *primary_exp) {
  if (primary_exp->type == PrimaryExpAST::Type::EXP) {
    std::string ret = Convert(primary_exp->exp.get());
    primary_exp->ret_val = dynamic_cast<ExpAST *>(primary_exp->exp.get())->ret_val;
    primary_exp->is_const = dynamic_cast<ExpAST *>(primary_exp->exp.get())->is_const;
    return ret;
  }
  else if (primary_exp->type == PrimaryExpAST::Type::NUMBER) {
    primary_exp->ret_val = primary_exp->number;
    primary_exp->is_const = true;
    return "";
  }
  else if (primary_exp->type == PrimaryExpAST::Type::LVAL) {
    std::string ret = Convert(primary_exp->lval.get());
    primary_exp->ret_val = dynamic_cast<LValAST *>(primary_exp->lval.get())->ret_val;
    primary_exp->is_const = dynamic_cast<LValAST *>(primary_exp->lval.get())->is_const;
    return ret;
  }
  else {
    return "error primary_exp type";
  }
}

std::string AST2IRConverter::ConvertUnaryExp(UnaryExpAST *unary_exp) {
  if (unary_exp->type == UnaryExpAST::Type::PRIMARY_EXP) {
    std::string ret = Convert(unary_exp->primary_exp.get());
    unary_exp->ret_val = dynamic_cast<PrimaryExpAST *>(unary_exp->primary_exp.get())->ret_val;
    unary_exp->is_const = dynamic_cast<PrimaryExpAST *>(unary_exp->primary_exp.get())->is_const;
    return ret;
  }
  else if (unary_exp->type == UnaryExpAST::Type::UNARY_EXP) {
    if (unary_exp->op == "-") {
      std::string ret = Convert(unary_exp->unary_exp.get());
      bool operand_const = dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->is_const;
      if (operand_const) {
        int operand = std::stoi(dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->ret_val);
        unary_exp->ret_val = std::to_string(-operand);
        unary_exp->is_const = operand_const;
        return ret;
      } else {
        std::string operand = dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->ret_val;
        std::string id = ir_info.getNextID();
        unary_exp->ret_val = id;
        unary_exp->is_const = false;
        return ret + "\t" + id + " = sub 0, " + operand + '\n';
      }
    }
    else if (unary_exp->op == "!") {
      std::string ret = Convert(unary_exp->unary_exp.get());
      bool operand_const = dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->is_const;
      if (operand_const) {
        int operand = std::stoi(dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->ret_val);
        unary_exp->ret_val = std::to_string(!operand);
        unary_exp->is_const = operand_const;
        return ret;
      }
      else {
        std::string operand = dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->ret_val;
        std::string id = ir_info.getNextID();
        unary_exp->ret_val = id;
        unary_exp->is_const = false;
        return ret + "\t" + id + " = eq 0, " + operand + '\n';
      }
    }
    else if (unary_exp->op == "+") {
      std::string ret = Convert(unary_exp->unary_exp.get());
      unary_exp->ret_val = dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->ret_val;
      unary_exp->is_const = dynamic_cast<UnaryExpAST *>(unary_exp->unary_exp.get())->is_const;
      return ret;
    }
    else {
      return "error unary_exp op";
    }
  }
  else if (unary_exp->type == UnaryExpAST::Type::FUNC_EXP) {
    std::string func_ident = unary_exp->ident;
    bool func_exists = symbol_table->funcSymbolExists(func_ident, false);
    if (!func_exists) {
      return "error: function " + func_ident + " not declared\n";
    }
    std::string func_type = symbol_table->getFunctionType(func_ident);
    std::string ir_code;
    if (unary_exp->func_rparams != nullptr) {
      ir_code += Convert(unary_exp->func_rparams.get());
    }
    if (func_type == "int") {
      unary_exp->ret_val = ir_info.getNextID();
      ir_code += "\t" + unary_exp->ret_val + " = call @" + func_ident + "(";
    }
    else if (func_type == "void") {
      ir_code += "\tcall @" + func_ident + "(";
    }
    else {
      return "error: function " + func_ident + " type\n";
    }
    if (unary_exp->func_rparams != nullptr) {
      auto func_rparams_ast = dynamic_cast<FuncRParamsAST *>(unary_exp->func_rparams.get());
      if (func_rparams_ast) {
        for (auto it = func_rparams_ast->ret_vals.begin(); it != func_rparams_ast->ret_vals.end(); ++it) {
          ir_code += *it;
          if (it != std::prev(func_rparams_ast->ret_vals.end())) {
            ir_code += ", ";
          }
        }
      }
    }
    ir_code += ")\n";
    return ir_code;
  }
  else {
    return "error unary_exp type";
  }
}

std::string AST2IRConverter::ConvertFuncRParams(FuncRParamsAST *func_rparams) {
  std::string ret;
  for (const auto &rparam : func_rparams->rparams) {
    ret += Convert(rparam.get());
    func_rparams->ret_vals.push_back(dynamic_cast<ExpAST *>(rparam.get())->ret_val);
  }
  return ret;
}

std::string AST2IRConverter::ConvertMulExp(MulExpAST *mul_exp) {
  if (mul_exp->type == MulExpAST::Type::UNARY_EXP) {
    std::string ret = Convert(mul_exp->unary_exp.get());
    mul_exp->ret_val = dynamic_cast<UnaryExpAST *>(mul_exp->unary_exp.get())->ret_val;
    mul_exp->is_const = dynamic_cast<UnaryExpAST *>(mul_exp->unary_exp.get())->is_const;
    return ret;
  }
  else if (mul_exp->type == MulExpAST::Type::MUL_EXP) {
    std::string ret_1 = Convert(mul_exp->mul_exp.get());
    std::string operand_1 = dynamic_cast<MulExpAST *>(mul_exp->mul_exp.get())->ret_val;
    bool operand_1_const = dynamic_cast<MulExpAST *>(mul_exp->mul_exp.get())->is_const;
    std::string ret_2 = Convert(mul_exp->unary_exp.get());
    std::string operand_2 = dynamic_cast<UnaryExpAST *>(mul_exp->unary_exp.get())->ret_val;
    bool operand_2_const = dynamic_cast<UnaryExpAST *>(mul_exp->unary_exp.get())->is_const;
    if (operand_1_const && operand_2_const) {
      int operand_1_int = std::stoi(operand_1);
      int operand_2_int = std::stoi(operand_2);
      int result;
      if (mul_exp->op == "*") {
        result = operand_1_int * operand_2_int;
      }
      else if (mul_exp->op == "/") {
        result = operand_1_int / operand_2_int;
      }
      else if (mul_exp->op == "%") {
        result = operand_1_int % operand_2_int;
      }
      else {
        return "error mul_exp op";
      }
      mul_exp->ret_val = std::to_string(result);
      mul_exp->is_const = true;
      return ret_1 + ret_2;
    }
    else {
      std::string id = ir_info.getNextID();
      mul_exp->ret_val = id;
      mul_exp->is_const = false;
      std::string op;
      if (mul_exp->op == "*") {
        op = "mul";
      }
      else if (mul_exp->op == "/") {
        op = "div";
      }
      else if (mul_exp->op == "%") {
        op = "mod";
      }
      else {
        return "error mul_exp op";
      }
      return ret_1 + ret_2 + "\t" + id + " = " + op + " " + operand_1 + ", " + operand_2 + '\n';
    }
  }
  else {
    return "error mul_exp type";
  }
}

std::string AST2IRConverter::ConvertAddExp(AddExpAST *add_exp) {
  if (add_exp->type == AddExpAST::Type::MUL_EXP) {
    std::string ret = Convert(add_exp->mul_exp.get());
    add_exp->ret_val = dynamic_cast<MulExpAST *>(add_exp->mul_exp.get())->ret_val;
    add_exp->is_const = dynamic_cast<MulExpAST *>(add_exp->mul_exp.get())->is_const;
    return ret;
  }
  else if (add_exp->type == AddExpAST::Type::ADD_EXP) {
    std::string ret_1 = Convert(add_exp->add_exp.get());
    std::string operand_1 = dynamic_cast<AddExpAST *>(add_exp->add_exp.get())->ret_val;
    bool operand_1_const = dynamic_cast<AddExpAST *>(add_exp->add_exp.get())->is_const;
    std::string ret_2 = Convert(add_exp->mul_exp.get());
    std::string operand_2 = dynamic_cast<MulExpAST *>(add_exp->mul_exp.get())->ret_val;
    bool operand_2_const = dynamic_cast<MulExpAST *>(add_exp->mul_exp.get())->is_const;
    if (operand_1_const && operand_2_const) {
      int operand_1_int = std::stoi(operand_1);
      int operand_2_int = std::stoi(operand_2);
      int result;
      if (add_exp->op == "+") {
        result = operand_1_int + operand_2_int;
      }
      else if (add_exp->op == "-") {
        result = operand_1_int - operand_2_int;
      }
      else {
        return "error add_exp op";
      }
      add_exp->ret_val = std::to_string(result);
      add_exp->is_const = true;
      return ret_1 + ret_2;
    }
    else {
      std::string id = ir_info.getNextID();
      add_exp->ret_val = id;
      add_exp->is_const = false;
      std::string op;
      if (add_exp->op == "+") {
        op = "add";
      }
      else if (add_exp->op == "-") {
        op = "sub";
      }
      else {
        return "error add_exp op";
      }
      return ret_1 + ret_2 + "\t" + id + " = " + op + " " + operand_1 + ", " + operand_2 + '\n';
    }
  }
  else {
    return "error add_exp type";
  }
}

std::string AST2IRConverter::ConvertRelExp(RelExpAST *rel_exp) {
  if (rel_exp->type == RelExpAST::Type::ADD_EXP) {
    std::string ret = Convert(rel_exp->add_exp.get());
    rel_exp->ret_val = dynamic_cast<AddExpAST *>(rel_exp->add_exp.get())->ret_val;
    rel_exp->is_const = dynamic_cast<AddExpAST *>(rel_exp->add_exp.get())->is_const;
    return ret;
  }
  else if (rel_exp->type == RelExpAST::Type::REL_EXP) {
    std::string ret_1 = Convert(rel_exp->rel_exp.get());
    std::string operand_1 = dynamic_cast<RelExpAST *>(rel_exp->rel_exp.get())->ret_val;
    bool operand_1_const = dynamic_cast<RelExpAST *>(rel_exp->rel_exp.get())->is_const;
    std::string ret_2 = Convert(rel_exp->add_exp.get());
    std::string operand_2 = dynamic_cast<AddExpAST *>(rel_exp->add_exp.get())->ret_val;
    bool operand_2_const = dynamic_cast<AddExpAST *>(rel_exp->add_exp.get())->is_const;
    if (operand_1_const && operand_2_const) {
      int operand_1_int = std::stoi(operand_1);
      int operand_2_int = std::stoi(operand_2);
      bool result;
      if (rel_exp->op == "<") {
        result = operand_1_int < operand_2_int;
      }
      else if (rel_exp->op == "<=") {
        result = operand_1_int <= operand_2_int;
      }
      else if (rel_exp->op == ">") {
        result = operand_1_int > operand_2_int;
      }
      else if (rel_exp->op == ">=") {
        result = operand_1_int >= operand_2_int;
      }
      else {
        return "error rel_exp op";
      }
      rel_exp->ret_val = std::to_string(result);
      rel_exp->is_const = true;
      return ret_1 + ret_2;
    }
    else {
      std::string id = ir_info.getNextID();
      rel_exp->ret_val = id;
      rel_exp->is_const = false;
      std::string op;
      if (rel_exp->op == "<") {
        op = "lt";
      }
      else if (rel_exp->op == "<=") {
        op = "le";
      }
      else if (rel_exp->op == ">") {
        op = "gt";
      }
      else if (rel_exp->op == ">=") {
        op = "ge";
      }
      else {
        return "error rel_exp op";
      }
      return ret_1 + ret_2 + "\t" + id + " = " + op + " " + operand_1 + ", " + operand_2 + '\n';
    }
  }
  else {
    return "error rel_exp type";
  }
}

std::string AST2IRConverter::ConvertEqExp(EqExpAST *eq_exp) {
  if (eq_exp->type == EqExpAST::Type::REL_EXP) {
    std::string ret = Convert(eq_exp->rel_exp.get());
    eq_exp->ret_val = dynamic_cast<RelExpAST *>(eq_exp->rel_exp.get())->ret_val;
    eq_exp->is_const = dynamic_cast<RelExpAST *>(eq_exp->rel_exp.get())->is_const;
    return ret;
  }
  else if (eq_exp->type == EqExpAST::Type::EQ_EXP) {
    std::string ret_1 = Convert(eq_exp->eq_exp.get());
    std::string operand_1 = dynamic_cast<EqExpAST *>(eq_exp->eq_exp.get())->ret_val;
    bool operand_1_const = dynamic_cast<EqExpAST *>(eq_exp->eq_exp.get())->is_const;
    std::string ret_2 = Convert(eq_exp->rel_exp.get());
    std::string operand_2 = dynamic_cast<RelExpAST *>(eq_exp->rel_exp.get())->ret_val;
    bool operand_2_const = dynamic_cast<RelExpAST *>(eq_exp->rel_exp.get())->is_const;
    if (operand_1_const && operand_2_const) {
      int operand_1_int = std::stoi(operand_1);
      int operand_2_int = std::stoi(operand_2);
      bool result;
      if (eq_exp->op == "==") {
        result = operand_1_int == operand_2_int;
      }
      else if (eq_exp->op == "!=") {
        result = operand_1_int != operand_2_int;
      }
      else {
        return "error eq_exp op";
      }
      eq_exp->ret_val = std::to_string(result);
      eq_exp->is_const = true;
      return ret_1 + ret_2;
    }
    else {
      std::string id = ir_info.getNextID();
      eq_exp->ret_val = id;
      eq_exp->is_const = false;
      std::string op;
      if (eq_exp->op == "==") {
        op = "eq";
      }
      else if (eq_exp->op == "!=") {
        op = "ne";
      }
      else {
        return "error eq_exp op";
      }
      return ret_1 + ret_2 + "\t" + id + " = " + op + " " + operand_1 + ", " + operand_2 + '\n';
    }
  }
  else {
    return "error eq_exp type";
  }
}

std::string AST2IRConverter::ConvertLAndExp(LAndExpAST *land_exp) {
  if (land_exp->type == LAndExpAST::Type::EQ_EXP) {
    std::string ret = Convert(land_exp->eq_exp.get());
    land_exp->ret_val = dynamic_cast<EqExpAST *>(land_exp->eq_exp.get())->ret_val;
    land_exp->is_const = dynamic_cast<EqExpAST *>(land_exp->eq_exp.get())->is_const;
    return ret;
  }
  else if (land_exp->type == LAndExpAST::Type::LAND_EXP) {
    std::string ret_1 = Convert(land_exp->land_exp.get());
    std::string operand_1 = dynamic_cast<LAndExpAST *>(land_exp->land_exp.get())->ret_val;
    bool operand_1_const = dynamic_cast<LAndExpAST *>(land_exp->land_exp.get())->is_const;

    if (operand_1_const) {
      int operand_1_int = std::stoi(operand_1);
      if (!operand_1_int) {
        land_exp->ret_val = "0";
        land_exp->is_const = true;
        return ret_1;
      }
    }
    std::string ret_2 = Convert(land_exp->eq_exp.get());
    std::string operand_2 = dynamic_cast<EqExpAST *>(land_exp->eq_exp.get())->ret_val;
    bool operand_2_const = dynamic_cast<EqExpAST *>(land_exp->eq_exp.get())->is_const;
    if (land_exp->op == "&&") {
      if (operand_1_const && operand_2_const) {
        int operand_1_int = std::stoi(operand_1);
        int operand_2_int = std::stoi(operand_2);
        bool result = operand_1_int && operand_2_int;
        land_exp->ret_val = std::to_string(result);
        land_exp->is_const = true;
        return ret_1 + ret_2;
      }
      else {
        // short-circuit evaluation
        std::string ret = ret_1;
        std::string id_0 = ir_info.getNextID();
        std::string id_1 = ir_info.getNextID();
        std::string postfix = IfBlockPostfix();
        std::string if_block_name = "%if" + postfix;
        std::string end_block_name = "%end" + postfix;
        
        ret += "\t" + id_0 + " = alloc i32\n";
        ret += "\tstore 0, " + id_0 + '\n';
        ret += "\tbr " + operand_1 + ", " + if_block_name + ", " + end_block_name + '\n';
        ret += if_block_name + ":\n";
        ret += ret_2;
        ret += "\t" + id_1 + " = ne " + operand_2 + ", 0\n";
        ret += "\tstore " + id_1 + ", " + id_0 + '\n';
        ret += "\tjump " + end_block_name + '\n';
        ret += end_block_name + ":\n";
        std::string id_2 = ir_info.getNextID();
        ret += "\t" + id_2 + " = load " + id_0 + '\n';
        land_exp->ret_val = id_2;
        land_exp->is_const = false;
        return ret;
      }
    }
    else {
      return "error land_exp op";
    }
  }
  else {
    return "error land_exp type";
  }
}

std::string AST2IRConverter::ConvertLOrExp(LOrExpAST *lor_exp) {
  if (lor_exp->type == LOrExpAST::Type::LAND_EXP) {
    std::string ret = Convert(lor_exp->land_exp.get());
    lor_exp->ret_val = dynamic_cast<LAndExpAST *>(lor_exp->land_exp.get())->ret_val;
    lor_exp->is_const = dynamic_cast<LAndExpAST *>(lor_exp->land_exp.get())->is_const;
    return ret;
  }
  else if (lor_exp->type == LOrExpAST::Type::LOR_EXP) {
    std::string ret_1 = Convert(lor_exp->lor_exp.get());
    std::string operand_1 = dynamic_cast<LOrExpAST *>(lor_exp->lor_exp.get())->ret_val;
    bool operand_1_const = dynamic_cast<LOrExpAST *>(lor_exp->lor_exp.get())->is_const;

    if (operand_1_const) {
      int operand_1_int = std::stoi(operand_1);
      if (operand_1_int) {
        lor_exp->ret_val = "1";
        lor_exp->is_const = true;
        return ret_1;
      }
    }
    std::string ret_2 = Convert(lor_exp->land_exp.get());
    std::string operand_2 = dynamic_cast<LAndExpAST *>(lor_exp->land_exp.get())->ret_val;
    bool operand_2_const = dynamic_cast<LAndExpAST *>(lor_exp->land_exp.get())->is_const;
    if (lor_exp->op == "||") {
      if (operand_1_const && operand_2_const) {
        int operand_1_int = std::stoi(operand_1);
        int operand_2_int = std::stoi(operand_2);
        bool result = operand_1_int || operand_2_int;
        lor_exp->ret_val = std::to_string(result);
        lor_exp->is_const = true;
        return ret_1 + ret_2;
      }
      else {
        // short-circuit evaluation
        std::string ret = ret_1;
        std::string id_0 = ir_info.getNextID();
        std::string id_1 = ir_info.getNextID();
        std::string postfix = IfBlockPostfix();
        std::string if_block_name = "%if" + postfix;
        std::string end_block_name = "%end" + postfix;
        
        ret += "\t" + id_0 + " = alloc i32\n";
        ret += "\tstore 1, " + id_0 + '\n';
        ret += "\tbr " + operand_1 + ", " + end_block_name + ", " + if_block_name + '\n';
        ret += if_block_name + ":\n";
        ret += ret_2;
        ret += "\t" + id_1 + " = ne " + operand_2 + ", 0\n";
        ret += "\tstore " + id_1 + ", " + id_0 + '\n';
        ret += "\tjump " + end_block_name + '\n';
        ret += end_block_name + ":\n";
        std::string id_2 = ir_info.getNextID();
        ret += "\t" + id_2 + " = load " + id_0 + '\n';
        lor_exp->ret_val = id_2;
        lor_exp->is_const = false;
        return ret;
      }
    }
    else {
      return "error lor_exp op";
    }
  }
  else {
    return "error lor_exp type";
  }
}

std::string AST2IRConverter::ConvertExp(ExpAST *exp) {
  std:: string ret = Convert(exp->lor_exp.get());
  exp->ret_val = dynamic_cast<LOrExpAST *>(exp->lor_exp.get())->ret_val;
  exp->is_const = dynamic_cast<LOrExpAST *>(exp->lor_exp.get())->is_const;
  return ret;
}

std::string AST2IRConverter::ConvertConstExp(ConstExpAST *const_exp) {
  std::string ret = Convert(const_exp->exp.get());
  const_exp->ret_val = dynamic_cast<ExpAST *>(const_exp->exp.get())->ret_val;
  const_exp->is_const = dynamic_cast<ExpAST *>(const_exp->exp.get())->is_const;
  return ret;
}

std::string AST2IRConverter::getIdent(LValAST *lval_exp) {
  if (lval_exp->type == LValAST::Type::IDENT) {
    return lval_exp->ident;
  }
  else if (lval_exp->type == LValAST::Type::ARRAY) {
    return getIdent(dynamic_cast<LValAST *>(lval_exp->lval.get()));
  }
  else {
    return "error lval_exp type";
  }
}

std::string AST2IRConverter::ConvertLVal(LValAST *lval_exp) {
  std::string ident, ret;
  if (lval_exp->type == LValAST::Type::IDENT) {
    ident = lval_exp->ident;
    auto symbol_info_opt = symbol_table->getSymbolInfo(ident);
    if (symbol_info_opt) {
      SymbolInfo symbol_info = symbol_info_opt.value();
      if (std::holds_alternative<VariableInfo>(symbol_info)) {
        std::string postfix = symbol_table->getSymbolPostfix(ident);
        lval_exp->is_const = false;
        lval_exp->ret_val = ir_info.getNextID();
        // TODO: check if the symbol is a ptr/array
        bool is_ptr = std::get<VariableInfo>(symbol_info).is_pointer;
        bool is_array = std::get<VariableInfo>(symbol_info).is_array;
        if (is_array) {
          if (lval_exp->lval_ret_val == "") {
            lval_exp->lval_ret_val = "0";
          }
          ret = "\t" + lval_exp->ret_val + " = getelemptr @" + ident + postfix + ", " + lval_exp->lval_ret_val + '\n';
        }
        else if (is_ptr) {
          if (lval_exp->lval_ret_val == "") {
            lval_exp->lval_ret_val = "0";
          }
          ret = "\t" + lval_exp->ret_val + " = load @" + ident + postfix + '\n';
          std::string ret_id = ir_info.getNextID();
          ret += "\t" + ret_id + " = getptr " + lval_exp->ret_val + ", " + lval_exp->lval_ret_val + '\n';
          lval_exp->ret_val = ret_id;
        }
        else {
          ret = "\t" + lval_exp->ret_val + " = load @" + ident + postfix + '\n';
        }
      } else if (std::holds_alternative<ConstantInfo>(symbol_info)) {
        if (lval_exp->lval_ret_val != "") {
          std::string postfix = symbol_table->getSymbolPostfix(ident);
          lval_exp->is_const = false;
          lval_exp->ret_val = ir_info.getNextID();
          ret = "\t" + lval_exp->ret_val + " = getelemptr @" + ident + postfix + ", " + lval_exp->lval_ret_val + '\n';
        }
        else {
          lval_exp->is_const = true;
          lval_exp->ret_val = std::to_string(std::get<ConstantInfo>(symbol_info).value);
        }
      }
    }
    else {
      ret = "does not have this symbol: " + ident;
    }
  }
  else if (lval_exp->type == LValAST::Type::ARRAY) {
    std::string array_ret_val, ret_id, cur_id;
    ident = getIdent(dynamic_cast<LValAST *>(lval_exp->lval.get()));
    ret = Convert(lval_exp->exp.get());
    array_ret_val = dynamic_cast<ExpAST *>(lval_exp->exp.get())->ret_val;
    dynamic_cast<LValAST *>(lval_exp->lval.get())->lval_ret_val = array_ret_val;
    ret += Convert(lval_exp->lval.get());

    cur_id = ir_info.getNextID();
    ret_id = dynamic_cast<LValAST *>(lval_exp->lval.get())->ret_val;
    if (lval_exp->lval_ret_val == "") {
      std::string postfix = symbol_table->getSymbolPostfix(ident);
      int nested_layer = 0, ident_nested_layer;
      LValAST* cur_lval = lval_exp;
      while(cur_lval->type == LValAST::Type::ARRAY) {
        nested_layer += 1;
        cur_lval = dynamic_cast<LValAST *>(cur_lval->lval.get());
      }
      ident_nested_layer = symbol_table->getSymbolNestedLayer(ident);
      if (nested_layer == ident_nested_layer) {
        ret += "\t" + cur_id + " = load " + ret_id + '\n';
      }
      else {
        ret += "\t" + cur_id + " = getelemptr " + ret_id + ", 0\n";
      }
    }
    else {
      ret += "\t" + cur_id + " = getelemptr " + ret_id + ", " + lval_exp->lval_ret_val + '\n';
    }
    
    lval_exp->ret_val = cur_id;
    lval_exp->is_const = dynamic_cast<LValAST *>(lval_exp->lval.get())->is_const;
  }
  else {
    return "error lval_exp type";
  }
  return ret;
}

std::string AST2IRConverter::ConvertLValStore(LValAST *lval_exp) {
  std::string ident, ret;
  if (lval_exp->type == LValAST::Type::IDENT) {
    ident = lval_exp->ident;
    auto symbol_info_opt = symbol_table->getSymbolInfo(ident);
    if (symbol_info_opt) {
      SymbolInfo symbol_info = symbol_info_opt.value();
      if (std::holds_alternative<VariableInfo>(symbol_info)) {
        std::string postfix = symbol_table->getSymbolPostfix(ident);
        lval_exp->is_const = false;
        bool is_ptr = std::get<VariableInfo>(symbol_info).is_pointer;
        bool is_array = std::get<VariableInfo>(symbol_info).is_array;
        if (is_array) {
          lval_exp->ret_val = ir_info.getNextID();
          if (lval_exp->lval_ret_val == "") {
            lval_exp->lval_ret_val = "0";
          }
          ret = "\t" + lval_exp->ret_val + " = getelemptr @" + ident + postfix + ", " + lval_exp->lval_ret_val + '\n';
        }
        else if (is_ptr) {
          lval_exp->ret_val = ir_info.getNextID();
          if (lval_exp->lval_ret_val == "") {
            lval_exp->lval_ret_val = "0";
          }
          ret = "\t" + lval_exp->ret_val + " = load @" + ident + postfix + '\n';
          std::string ret_id = ir_info.getNextID();
          ret += "\t" + ret_id + " = getptr " + lval_exp->ret_val + ", " + lval_exp->lval_ret_val + '\n';
          lval_exp->ret_val = ret_id;
        }
        else {
          lval_exp->ret_val = "@" + ident + postfix;
        }
      } else if (std::holds_alternative<ConstantInfo>(symbol_info)) {
        if (lval_exp->lval_ret_val != "") {
          lval_exp->is_const = false;
          lval_exp->ret_val = lval_exp->lval_ret_val;
        }
        else {
          ret = "error: cannot assign to const\n";
        }
      }
    }
    else {
      ret = "does not have this symbol: " + ident;
    }
  }
  else if (lval_exp->type == LValAST::Type::ARRAY) {
    std::string array_ret_val, cur_id, ret_id;
    ret = Convert(lval_exp->exp.get());
    array_ret_val = dynamic_cast<ExpAST *>(lval_exp->exp.get())->ret_val;
    dynamic_cast<LValAST *>(lval_exp->lval.get())->lval_ret_val = array_ret_val;

    ret += ConvertLValStore(dynamic_cast<LValAST *>(lval_exp->lval.get()));
    ident = getIdent(dynamic_cast<LValAST *>(lval_exp->lval.get()));
    ret_id = dynamic_cast<LValAST *>(lval_exp->lval.get())->ret_val;
    if (lval_exp->lval_ret_val == "") {
      lval_exp->ret_val = ret_id;
    }
    else {
      cur_id = ir_info.getNextID();
      ret += "\t" + cur_id + " = getelemptr " + ret_id + ", " + lval_exp->lval_ret_val + '\n';
      lval_exp->ret_val = cur_id;
    }
    lval_exp->is_const = dynamic_cast<LValAST *>(lval_exp->lval.get())->is_const;
  }
  else {
    return "error lval_exp type";
  }
  return ret;
}

std::string AST2IRConverter::IfBlockPostfix() {
  if (if_block_num == 0) {
    if_block_num += 1;
    return "";
  }
  return "_" + std::to_string(if_block_num++);
}

std::string AST2IRConverter::WhileBlockPostfix() {
  int cur_while_block_num = while_block_stack.top();
  if (cur_while_block_num == 1) {
    return "";
  }
  return "_" + std::to_string(cur_while_block_num);
}

void AST2IRConverter::EnterWhileBlock() {
  while_block_num += 1;
  while_block_stack.push(while_block_num);
}

void AST2IRConverter::ExitWhileBlock() {
  while_block_stack.pop();
}

bool AST2IRConverter::InWhileBlock() {
  return !while_block_stack.empty();
}

void AST2IRConverter::EnterChildSymbolTable() {
  SymbolTable* child = new SymbolTable();
  child->postfix = '_' + std::to_string(symbol_table_num);
  child->setParent(symbol_table);
  symbol_table->setChild(child);
  symbol_table = child;
  symbol_table_num += 1;
}

void AST2IRConverter::ExitChildSymbolTable() {
  SymbolTable* parent = symbol_table->getParent();
  symbol_table = parent;
}

void SymbolTable::setParent(SymbolTable* parent) {
  this->parent = parent;
}

void SymbolTable::setChild(SymbolTable* child) {
  this->child = child;
}

SymbolTable* SymbolTable::getParent() {
  return parent;
}

SymbolTable* SymbolTable::getChild() {
  return child;
}

void SymbolTable::insertConstant(const std::string& symbol, int value) {
  if (constSymbolExists(symbol, true)) {
    printf("Symbol already exists %s in const\n", symbol.c_str());
    throw std::runtime_error("Symbol already exists");
  }
  table[symbol] = ConstantInfo{value};
}

void SymbolTable::insertVariable(const std::string& symbol, int nested_layer, bool is_uninitialized, bool is_array) {
  if (varSymbolExists(symbol, true)) {
    printf("Symbol already exists %s in var\n", symbol.c_str());
    throw std::runtime_error("Symbol already exists");
  }
  table[symbol] = VariableInfo{0, nested_layer, false, is_uninitialized, false, is_array};
}

void SymbolTable::insertParams(const std::string& symbol, int nested_layer, bool is_pointer) {
  if (varSymbolExists(symbol, true)) {
    printf("Symbol already exists %s in params\n", symbol.c_str());
    throw std::runtime_error("Symbol already exists");
  }
  table[symbol] = VariableInfo{0, nested_layer, true, false, is_pointer, false};
}

void SymbolTable::insertFunction(const std::string& symbol, const std::string& type) {
  if (funcSymbolExists(symbol, true)) {
    throw std::runtime_error("Symbol already exists");
  }
  table[symbol] = FunctionInfo{type};
}

bool SymbolTable::symbolExists(const std::string& symbol, bool in_cur_block) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
        return true;
    }
    current = current->getParent();
    if (in_cur_block) {
      current = nullptr;
    }
  }
  SymbolTable* root = const_cast<SymbolTable*>(this);
  while(root->getParent() != nullptr) {
    root = root->getParent();
  }
  auto it = root->table.find(symbol);
  if (it != root->table.end()) {
    return true;
  }
  return false;
}

bool SymbolTable::constSymbolExists(const std::string& symbol, bool in_cur_block) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
        return std::holds_alternative<ConstantInfo>(it->second);
    }
    current = current->getParent();
    if (in_cur_block) {
      current = nullptr;
    }
  }
  // SymbolTable* root = const_cast<SymbolTable*>(this);
  // while(root->getParent() != nullptr) {
  //   root = root->getParent();
  // }
  // auto it = root->table.find(symbol);
  // if (it != root->table.end()) {
  //   return std::holds_alternative<ConstantInfo>(it->second);
  // }
  return false;
}

bool SymbolTable::varSymbolExists(const std::string& symbol, bool in_cur_block) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
      return std::holds_alternative<VariableInfo>(it->second);
    }
    current = current->getParent();
    if (in_cur_block) {
      current = nullptr;
    }
  }
  // SymbolTable* root = const_cast<SymbolTable*>(this);
  // while(root->getParent() != nullptr) {
  //   root = root->getParent();
  // }
  // auto it = root->table.find(symbol);
  // if (it != root->table.end()) {
  //   return std::holds_alternative<VariableInfo>(it->second);
  // }
  return false;
}

bool SymbolTable::funcSymbolExists(const std::string& symbol, bool in_cur_block) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
      if (std::holds_alternative<FunctionInfo>(it->second)) {
        return true;
      }
    }
    current = current->getParent();
    if (in_cur_block) {
      current = nullptr;
    }
  }
  // SymbolTable* root = const_cast<SymbolTable*>(this);
  // while(root->getParent() != nullptr) {
  //   root = root->getParent();
  // }
  // auto it = root->table.find(symbol);
  // if (it != root->table.end()) {
  //   return std::holds_alternative<FunctionInfo>(it->second);
  // }
  return false;
}

std::string SymbolTable::getSymbolPostfix(const std::string& symbol) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
        if (std::holds_alternative<VariableInfo>(it->second)) {
          bool is_param = std::get<VariableInfo>(it->second).is_param;
          if (is_param) {
            return current->postfix + "_param";
          }
          else if (current->getParent() == nullptr) {
            return "_glob";
          }
        }
        else if (std::holds_alternative<FunctionInfo>(it->second)) {
          return "";
        }
        return current->postfix;
    }
    current = current->getParent();
  }
  return "";
}

int SymbolTable::getSymbolNestedLayer(const std::string& symbol) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
      if (std::holds_alternative<VariableInfo>(it->second)) {
        return std::get<VariableInfo>(it->second).nested_layer;
      }
    }
    current = current->getParent();
  }
  return -1;
}

std::string SymbolTable::getFunctionType(const std::string& symbol) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
        if (std::holds_alternative<FunctionInfo>(it->second)) {
          return std::get<FunctionInfo>(it->second).type;
        }
    }
    current = current->getParent();
  }
  return "";
}

std::optional<SymbolInfo> SymbolTable::getSymbolInfo(const std::string& symbol) const {
  SymbolTable* current = const_cast<SymbolTable*>(this);
  while (current != nullptr) {
    auto it = current->table.find(symbol);
    if (it != current->table.end()) {
        return it->second;
    }
    current = current->getParent();
  }
  return std::nullopt;
}

int NestedArray::getSize() const {
  return size;
}

void NestedArray::addInt(int value) {
  elements.push_back(std::to_string(value));
}

void NestedArray::addArray(std::shared_ptr<NestedArray> nestedArray) {
  elements.push_back(nestedArray);
}

void NestedArray::print() const{
    std::cout << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
      if (i > 0) {
          std::cout << ", ";
      }
      std::visit([](auto&& arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::string>) {
          std::cout << arg;
        } else {
          arg->print();
        }
      }, elements[i]);
    }
    std::cout << "]";
  }

void NestedArray::getArraySize(std::string type, std::vector<int>& array_size) {
  // search from back to front
  size_t pos = type.size() - 1;
  while (pos != std::string::npos) {
    size_t end = type.find_last_of(']', pos);
    if (end != std::string::npos) {
      size_t start = type.find_last_not_of("0123456789", end - 1);
      if (start != std::string::npos && type[start] == ' ') {
        array_size.push_back(std::stoi(type.substr(start + 1, end - start - 1)));
      }
    }
    pos = type.find_last_of(']', pos - 1);
  }
}

void NestedArray::fromString(const std::string& ori_str) {
  // Convert {1, 2, {3, 4}, {{5, 6}, 7}}-like-string to NestedArray
  std::string str = ori_str;
  if (str.size() < 2) {
    return;
  }

  size_t pos = 0;
  if (str[0] == '{') {
    pos = 1;
  }
  
  while (pos < str.size()) {
    if (str[pos] == '{') {
      std::shared_ptr<NestedArray> nestedArray = std::make_shared<NestedArray>();
      nestedArray->fromString(str.substr(pos));
      elements.push_back(nestedArray);
      pos += nestedArray->getSize();
    } 
    else if (str[pos] == ',' || str[pos] == ' ') {
      pos++;
    } 
    else if (str[pos] == '}') {
      size = pos + 1;
      break;
    } 
    else {
      size_t next_pos = str.find_first_of(",}", pos);
      elements.push_back(str.substr(pos, next_pos - pos));
      pos = next_pos;
    }
  }
}

std::string AST2IRConverter::zeroPadding(NestedArray* nested_array, std::vector<int>& array_size, std::string ptr_name) {
  std::string ret = "";
  int max_size, array_width, cur_id = 0;
  if (array_size.size() == 0) {
    return ret;
  }
  // [2, 3, 4] -> [12, 4]
  // [4, 2, 1] -> [2, 1]
  std::vector<int> acc_array_size;
  for (int i = 0; i < array_size.size(); i++) {
    int acc_size = 1;
    for (int j = i; j < array_size.size(); j++) {
      acc_size *= array_size[j];
    }
    if (i == 0) {
      max_size = acc_size;
    }
    else {
      acc_array_size.push_back(acc_size);
    }
    if (i == array_size.size() - 1) {
      array_width = array_size[i];
    }
  }
  std::stack<std::string> ptr_stack;
  ValueType* ele_ptr = &(*nested_array->elements.begin());
  ptr_stack.push(ptr_name);
  
  // if the array is not fully filled, then we need to fill the rest with 0
  while (cur_id < max_size) {
    int cur_offset = cur_id % array_width, nested_array_size_alignment = -1, new_array_num = 0;
    bool is_zero_padding = ele_ptr == &(*nested_array->elements.end());
    bool is_nested_array;
    if (is_zero_padding) {
      is_nested_array = false;
    } 
    else {
      is_nested_array = std::holds_alternative<std::shared_ptr<NestedArray>>(*ele_ptr);
    }
    // register prepare
    // for nested array, we only need to find the max size alignment
    int tmp_cur_id = cur_id;
    for (auto size_align: acc_array_size) {
      new_array_num++;
      if (tmp_cur_id % size_align == 0) {
        std::string cur_ptr = ptr_stack.top();
        std::string new_ptr = ir_info.getNextID();
        ret += "\t" + new_ptr + " = getelemptr " + cur_ptr + ", " + std::to_string(tmp_cur_id / size_align) + "\n";
        ptr_stack.push(new_ptr);

        if (is_nested_array) {
          nested_array_size_alignment = size_align;
          break;
        }
      }
      tmp_cur_id = tmp_cur_id % size_align;
    }
    // check nested array size alignment
    if (is_nested_array && nested_array_size_alignment == -1) {
      return "error: nested array size alignment not match\n";
    }
    // if it is a int, then we need to store the value
    if (is_zero_padding) {
      std::string cur_ptr = ptr_stack.top();
      std::string new_ptr = ir_info.getNextID();
      ret += "\t" + new_ptr + " = getelemptr " + cur_ptr + ", " + std::to_string(cur_offset) + "\n";
      ret += "\tstore 0, " + new_ptr + "\n";
      cur_id++;
    }
    else if (!is_nested_array) {
      std::string cur_ptr = ptr_stack.top();
      std::string new_ptr = ir_info.getNextID();
      ret += "\t" + new_ptr + " = getelemptr " + cur_ptr + ", " + std::to_string(cur_offset) + "\n";
      std::string value = std::get<std::string>(*ele_ptr);
      ret += "\tstore " + value + ", " + new_ptr + "\n";
      cur_id++;
    }
    else {
      std::string cur_ptr = ptr_stack.top();
      std::vector<int> new_array_size;
      for (int i = new_array_num; i < array_size.size(); i++) {
        new_array_size.push_back(array_size[i]);
      }
      // recursive call
      NestedArray* nested_array_ptr = std::get<std::shared_ptr<NestedArray>>(*ele_ptr).get();
      ret += zeroPadding(nested_array_ptr, new_array_size, cur_ptr);
      cur_id += nested_array_size_alignment;
    }
    // register clean
    for (auto size_align = acc_array_size.rbegin(); size_align != acc_array_size.rend(); size_align++) {
      if (is_nested_array) {
        if (*size_align < nested_array_size_alignment) continue;
        if (*size_align == nested_array_size_alignment) {
          auto next_size_align = size_align + 1;
          if (next_size_align != acc_array_size.rend()) {
            if (*next_size_align == *size_align) {
              continue;
            }
          }
        }
      }
      if (cur_id % *size_align == 0) {
        if (!ptr_stack.empty()) {
          ptr_stack.pop();
        }
        else {
          return "error: ptr_stack empty\n";
        }
      }
    }
    if (!is_zero_padding) {
      ele_ptr++;
    }
  }
  
  return ret;
}

std::string AST2IRConverter::zeroPaddingGlobal(NestedArray* nested_array, std::vector<int>& array_size) {
  std::string ret = "";
  int max_size, cur_id = 0;
  if (array_size.size() == 0) {
    return ret;
  }
  // [2,3,4] -> [12, 4]
  std::vector<int> acc_array_size;
  for (int i = 0; i < array_size.size(); i++) {
    int acc_size = 1;
    for (int j = i; j < array_size.size(); j++) {
      acc_size *= array_size[j];
    }
    if (i == 0) {
      max_size = acc_size;
    } else {
      acc_array_size.push_back(acc_size);
    }
  }
  ret = "{";
  ValueType* ele_ptr = &(*nested_array->elements.begin());
  bool is_first_ele = true;
  
  // if the array is not fully filled, then we need to fill the rest with 0
  while (cur_id < max_size) {
    int nested_array_size_alignment = -1, new_array_num = 0;
    bool is_zero_padding = ele_ptr == &(*nested_array->elements.end());
    bool is_nested_array;
    if (is_zero_padding) {
      is_nested_array = false;
    } else {
      is_nested_array = std::holds_alternative<std::shared_ptr<NestedArray>>(*ele_ptr);
    }
    if (!is_first_ele) {
      ret += ", ";
    }
    // register prepare
    // for nested array, we only need to find the max size alignment
    for (auto size_align: acc_array_size) {
      new_array_num++;
      if (cur_id % size_align == 0) {
        if (is_nested_array) {
          nested_array_size_alignment = size_align;
          break;
        }
        ret += "{";
      }
    }
    // check nested array size alignment
    if (is_nested_array && nested_array_size_alignment == -1) {
      return "error: nested array size alignment not match\n";
    }
    // if it is a int, then we need to store the value
    if (is_zero_padding) {
      ret += "0";
      cur_id++;
    }
    else if (!is_nested_array) {
      std::string value = std::get<std::string>(*ele_ptr);
      ret += value;
      cur_id++;
    }
    else {
      std::vector<int> new_array_size;
      for (int i = new_array_num; i < array_size.size(); i++) {
        new_array_size.push_back(array_size[i]);
      }
      // recursive call
      NestedArray* nested_array_ptr = std::get<std::shared_ptr<NestedArray>>(*ele_ptr).get();
      std::string nested_array_result = zeroPaddingGlobal(nested_array_ptr, new_array_size);
      ret += nested_array_result;
      cur_id += nested_array_size_alignment;
    }
    // register clean
    int tmp_array_num = array_size.size() - new_array_num;
    for (auto size_align = acc_array_size.rbegin(); size_align != acc_array_size.rend(); size_align++) {
      if (is_nested_array && tmp_array_num > 0) {
        tmp_array_num--;
        continue;
      }
      if (cur_id % *size_align == 0) {
        ret += "}";
      }
    }
    if (!is_zero_padding) {
      ele_ptr++;
    }
    is_first_ele = false;
  }
  ret += "}";
  
  return ret;
}