#include "ast.h"


void show_table();
int ident_floor(std::string ident);
int ident_id(std::string ident);
void init2array(BaseExprAST** start, int* len_start, int* len_end, const VarInitValAST* var_init_val);
std::string array2Aggregate(BaseExprAST** init_array, int* idx, int* dim, int* dim_end);
std::string array2ptr(BaseExprAST** init_array, int* idx, std::string base_ptr, int* dim, int* dim_end);
bool all_zero(BaseExprAST** init_array, int* idx, int* dim, int* dim_end);

void CompUnitAST::Dump() const  {
    std::cout << "CompUnitAST { ";
    std::cout << " }";
}

void FuncDefAST::Dump() const  {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}

void FuncParamAST::Dump() const {

}

void FuncTypeAST::Dump() const  {
    std::cout << "FuncTypeAST { ";
    std::cout << type ;
    std::cout << " }";
}

void BlockAST::Dump() const  {
    std::cout << "BlockAST { ";
    for (auto& item : block_item_list) {
        item->Dump();
        std::cout << " ,";
    }
    std::cout << " }";
}

void BlockItemAST::Dump() const  {
    std::cout << "BlockItemAST { ";
    switch(flag){
        case DECL:
            decl->Dump();
            break;
        case STMT:
            stmt->Dump();
            break;
    }
    std::cout << " }";
}

void StmtAST::Dump() const  {
    std::cout << "StmtAST { ";
    expr->Dump();
    std::cout << " }";
}

void ExprAST::Dump() const {
    std::cout << "ExprAST { ";
    lor_expr->Dump();
    std::cout << " }";
}

void UnaryExprAST::Dump() const {
    std::cout << "UnaryExprAST { ";
    switch(flag){
        case PRIMARY_EXPR:
            primary_expr->Dump();
            break;
        case OP_UNARY:
            std::cout << " " << op << " ";
            unary_expr->Dump();
            break;
        case CALL:
            break;
    }
    std::cout << " }";
}

void CallParamAST::Dump() const{

}

void PrimaryExprAST::Dump() const {
    std::cout << "PrimaryExprAST { ";
    switch(flag){
        case NUMBER:
            std::cout << number;
            break;
        case WITH_BRACKETS:
            expr->Dump();
            break;
        case LVAL:
            break;
    }
    std::cout << " }";
}

void AddExprAST::Dump() const {
    std::cout << "AddExprAST { ";
    switch(flag){
        case ONLY_MUL:
            mul_expr->Dump();
            break;
        case ADD_MUL:
            add_expr->Dump();
            std::cout << " " << op << " ";
            mul_expr->Dump();
            break;
    }
    std::cout << " }";
}

void MulExprAST::Dump() const {
    std::cout << "MulExprAST { ";
    switch(flag){
        case ONLY_UNARY:
            unary_expr->Dump();
            break;
        case MUL_UNARY:
            mul_expr->Dump();
            std::cout << " " << op << " ";
            unary_expr->Dump();
            break;
    }
    std::cout << " }";
}

void LOrExprAST::Dump() const {
    land_expr->Dump();
}

void LAndExprAST::Dump() const {
    eq_expr->Dump();
}

void EqExprAST::Dump() const {
    rel_expr->Dump();   
}

void RelExprAST::Dump() const {
    add_expr->Dump();
}

void LValAST::Dump() const {
    
}

void LeftLValAST::Dump() const {
}

void DeclAST::Dump() const {
    const_decl->Dump();
}

void ConstDeclAST::Dump() const {
    for(auto& const_def : const_def_list)
        const_def->Dump();
}

void ConstDefAST::Dump() const {
    const_init_val->Dump();
}

void ConstInitValAST::Dump() const {
    const_expr->Dump();
}

void ConstExprAST::Dump() const {
    expr->Dump();
}

void VarDeclAST::Dump() const {
    for(auto& var_def : var_def_list)
        var_def->Dump();
}

void VarDefAST::Dump() const {
    var_init_val->Dump();
}


void VarInitValAST::Dump() const {
    expr->Dump();
}

std::string CompUnitAST::koopa_ir() const {
    str+="decl @getint(): i32\ndecl @getch(): i32\ndecl @getarray(*i32): i32\ndecl @putint(i32)\ndecl @putch(i32)\ndecl @putarray(i32, *i32)\ndecl @starttime()\ndecl @stoptime()\n\n";
    func_ret["getint"]=true;
    func_ret["getch"]=true;
    func_ret["getarray"]=true;
    for(auto& item : unit_list){
        item->koopa_ir();
    }
    return "";
}

std::string FuncDefAST::koopa_ir() const {
    std::unordered_map<std::string, MyVar> next_table;
    symbol_tables.push_back(next_table);
    block_num++;
    now_func=ident;
    str += "fun @"+ident+"(";
    std::vector<std::string> ident_list;
    if(func_param_list.size()!= 0){
        for(int i=0;i<func_param_list.size();++i){
            std::string t = func_param_list[i]->koopa_ir();
            ident_list.push_back(t);
            if(i!=func_param_list.size()-1)
                str += ", ";
        }
    }
    str += ")";
    std::string ttype=func_type->koopa_ir();
    if(ttype=="int"){
        func_ret[ident] = true;
    }
    str += " {\n";
    std::string block_name = "%entry";
    str += block_name+":\n";
    for(auto ident : ident_list){
        if(symbol_tables[block_num][ident].type=="int"){
            str+="\t%"+ident+"_"+std::to_string(ident_id(ident))+" = alloc i32\n";
            str+="\tstore @"+ident+", %"+ident+"_"+std::to_string(ident_id(ident))+"\n";
        }
        else{
            str+="\t%"+ident+"_"+std::to_string(ident_id(ident))+" = alloc *";
            for(int i=0;i<symbol_tables[block_num][ident].dim_len.size();++i){
                if(symbol_tables[block_num][ident].dim_len[i]!=-1)
                    str+="[";
            }
            str+="i32";
            for(int i=1;i<symbol_tables[block_num][ident].dim_len.size();++i){
                str+=", "+std::to_string(symbol_tables[block_num][ident].dim_len[i])+"]";
            }
            str += "\n";
            str+="\tstore @"+ident+", %"+ident+"_"+std::to_string(ident_id(ident))+"\n";
        }
    }
    now_block = block_name;
    block_end[now_block] = false;
    block->koopa_ir();
    if(!has_ret[ident]){
        str += "\tret ";
        if(ttype=="int"){
            str+="0\n";
        }
        else{
            str+="\n";
        }
    }
    str += "}\n";
    symbol_tables.pop_back();
    block_num--;
    str+="\n";
    now_block = "";
    return "";
}

std::string FuncParamAST::koopa_ir() const{
    str+="@"+ident;
    std::string ret = ident;
    if(flag==INT){
        symbol_tables[block_num][ident] = MyVar("int", 0, false, ident_cnt++, true);
        str+=": i32";
    }
    else if(flag==ARRAY){
        symbol_tables[block_num][ident] = MyVar("array", 0, true, ident_cnt++, true);
        symbol_tables[block_num][ident].dim_len.push_back(-1);
        str+=": *";
        for(int i=0;i<more_array_size.size();++i){
            str+="[";
        }
        str += "i32";
        for(int i=0;i<more_array_size.size();++i){
            str+=", "+std::to_string(more_array_size[i]->getVal())+"]";
            symbol_tables[block_num][ident].dim_len.push_back(more_array_size[i]->getVal());
        }
    }
    return ret;
}

std::string FuncTypeAST::koopa_ir() const {
    if(type == "int"){
        str += ": i32";
        return "int";
    } 
    return "void";
}

std::string BlockAST::koopa_ir() const {
    for (auto& item: block_item_list){
        item->koopa_ir();
    }
    return "";
}

std::string BlockItemAST::koopa_ir() const {
    switch(flag){
        case DECL:
            decl->koopa_ir();
            break;
        case STMT:
            stmt->koopa_ir();
            break;
    }
    return "";
}

std::string StmtAST::koopa_ir() const {
    if(block_end[now_block])
        return "";
    std::string ret="";
    if(flag == RETURN){
        auto res = expr->koopa_ir();
        str += "\tret " + res + "\n";
        block_end[now_block] = true;
        has_ret[now_func]=true;
        if(now_block.find("else")!=std::string::npos||now_block.find("then")!=std::string::npos||now_block.find("while_body")!=std::string::npos){
            has_ret[now_func]=false;
        }
        if(nested_while_stack.size()!=0){
            has_ret[now_func]=false;
        }
    }
    else if (flag == ASSIGN){
        std::string ident = lval->koopa_ir();
        std::string id = std::to_string(ident_id(ident));
        // std::cout<<"assign_ident: "<<ident<<std::endl;        //debug
        // std::cout<<"assign_id: "<<id<<std::endl;        //debug
        // std::cout<<"assign_expr"<<std::endl;        //debug
        std::string val = expr->koopa_ir();
        // std::cout<<val<<std::endl;        //debug
        auto p=(LeftLValAST*)lval.get();
        if(p->flag==LeftLValAST::IDENT){
            str += "\tstore " + val + ", %" + ident + "_" + id + "\n";
        }
        else{
            str += "\tstore " + val + ", " + ident + "\n";
            int idx = 0;
            std::vector<int> dim_len = symbol_tables[ident_floor(p->ident)][p->ident].dim_len;
            // std::cout<<"hello"<<std::endl;        //debug   
            for(int i=p->idx_lst.size()-1;i>0;--i){
                if(i!=p->idx_lst.size()-1){
                    idx*=dim_len[i+1];
                }
                // std::cout<<p->idx_lst[i]->getVal()<<std::endl;        //debug
                idx+=p->idx_lst[i]->getVal();
            }
            // std::cout<<"hello"<<std::endl;        //debug
            while(symbol_tables[ident_floor(p->ident)][p->ident].array.size()<=idx){
                symbol_tables[ident_floor(p->ident)][p->ident].array.push_back(0);
            }
        }
    }
    else if(flag == BLOCK){
        std::unordered_map<std::string, MyVar> next_table;
        symbol_tables.push_back(next_table);
        block_num++;
        block->koopa_ir();
        symbol_tables.pop_back();
        block_num--;
    }
    else if(flag==EXPR){
        expr->koopa_ir();
    }
    else if(flag==RETURN_VOID){
        if(func_ret[now_func]){
            str += "\tret 0\n";
        }
        else{
            str += "\tret\n";
        }
        block_end[now_block] = true;
        has_ret[now_func]=true;
        if(now_block.find("else")!=std::string::npos||now_block.find("then")!=std::string::npos||now_block.find("while_body")!=std::string::npos){
            has_ret[now_func]=false;
        }
        if(nested_while_stack.size()!=0){
            has_ret[now_func]=false;
        }
    }
    else if(flag==IF){
        std::string val = cond->koopa_ir();
        std::string now_if = std::to_string(block_cnt);
        block_cnt ++;
        std::string then_block_name = "%then_"+now_if+"_b";
        std::string end_block_name = "%end_"+now_if+"_b";
        str += "\tbr " + val + ", " + then_block_name +", "+ end_block_name+"\n";
        block_end[now_block] = true;
        str += "\n";
        str += then_block_name+":\n";
        now_block = then_block_name;
        block_end[then_block_name] = false;
        if_stmt->koopa_ir();
        if(!block_end[now_block]){
            str += "\tjump %end_"+now_if+"_b\n";
            block_end[now_block] = true;
        }
        str += "\n";
        now_block = end_block_name;
        block_end[end_block_name] = false;
        str += end_block_name+":\n";
    }
    else if(flag==IF_ELSE){
        std::string val = cond->koopa_ir();
        std::string now_if= std::to_string(block_cnt);
        block_cnt++;
        std::string then_block_name = "%then_"+now_if+"_b";
        std::string else_block_name = "%else_"+now_if+"_b";
        std::string end_block_name = "%end_"+now_if+"_b";
        str += "\tbr " + val + ", "+then_block_name+", "+else_block_name+"\n";
        str += "\n";
        str += then_block_name+":\n";
        now_block=then_block_name;
        block_end[then_block_name] = false;
        if_stmt->koopa_ir();
        if(!block_end[now_block]){
            str += "\tjump %end_"+now_if+"_b\n";
            block_end[now_block] = true;
        }
        str += "\n";
        str += else_block_name+":\n";
        now_block = else_block_name;
        block_end[else_block_name] = false;
        else_stmt->koopa_ir();

        if(!block_end[now_block]){
            str += "\tjump %end_"+now_if+"_b\n";
            block_end[now_block] = true;
        }
        str += "\n";
        now_block = end_block_name;
        block_end[end_block_name] = false;
        str += "%end_"+now_if+"_b:\n";
    }
    else if(flag==WHILE){
        std::string now_while = std::to_string(block_cnt);
        nested_while_stack.push_back(now_while);
        block_cnt++;
        std::string entry_name = "%while_entry_"+now_while+"_b";
        std::string body_name = "%while_body_"+now_while+"_b";
        std::string end_name = "%while_end_"+now_while+"_b";
        str += "\tjump "+entry_name+"\n";
        str += "\n";
        str += entry_name+":\n";
        std::string val = cond->koopa_ir();
        str += "\tbr " + val + ", " + body_name + ", " + end_name + "\n";
        str += "\n";
        str += body_name+":\n";
        now_block = body_name;
        block_end[body_name] = false;
        while_stmt->koopa_ir();
        if(!block_end[now_block]){
            str += "\tjump "+entry_name+"\n";
            block_end[now_block] = true;
        }
        str += "\n";
        now_block = end_name;
        block_end[end_name] = false;
        str += end_name+":\n";
        nested_while_stack.pop_back();
    }
    else if(flag==BREAK){
        str += "\tjump %while_end_" + nested_while_stack.back() + "_b\n";
        block_end[now_block] = true;
    }
    else if(flag==CONTINUE){
        str += "\tjump %while_entry_" + nested_while_stack.back() + "_b\n";
        block_end[now_block] = true;
    }
    return ret;
}

std::string ExprAST::koopa_ir() const {
    auto res = lor_expr->koopa_ir();
    return res;
}

std::string UnaryExprAST::koopa_ir() const {
    std::string ret;
    std::vector<std::string> tmp;
    switch(flag){
        case PRIMARY_EXPR:
            ret = primary_expr->koopa_ir();  
            break;
        case CALL:
            for(auto& param : call_expr_list){
                tmp.push_back(param->koopa_ir());
            }
            str+="\t";
            if(func_ret[call_ident]){
                ret = "%"+std::to_string(cnt++);
                str+=ret+" = ";
            }
            str += "call @"+call_ident+"(";
            for(int i=0;i<tmp.size();++i){
                str+=tmp[i];
                if(i!=tmp.size()-1)
                    str+=", ";
            }
            str += ")\n";
            break;  
        case OP_UNARY:
            auto res = unary_expr->koopa_ir();
            if(op == "+"){
                ret = res;
                break;
            }
            ret = "%"+std::to_string(cnt++);
            if(op == "-"){
                str += "\t" + ret + " = sub 0, " + res + "\n";  
            }
            else if(op == "!"){
                str += "\t" + ret + " = eq 0, " + res + "\n";  
            }
            break; 
    }
    return ret;
}

std::string CallParamAST::koopa_ir() const{
    return expr->koopa_ir();
}

std::string PrimaryExprAST::koopa_ir() const {
    std::string ret;
    switch(flag){
        case NUMBER:
            ret = number;
            break;
        case WITH_BRACKETS:
            ret = expr->koopa_ir();
            break;
        case LVAL:
            ret = lval->koopa_ir();
            break;
    }
    return ret;
}

std::string AddExprAST::koopa_ir() const {
    std::string ret;
    switch(flag){
        case ONLY_MUL:
            ret = mul_expr->koopa_ir();
            break;
        case ADD_MUL:
            auto res1 = add_expr->koopa_ir();
            auto res2 = mul_expr->koopa_ir();
            ret = "%"+std::to_string(cnt++);
            if(op == "+"){
                str += "\t" + ret + " = add " + res1 + ", " + res2 + "\n";  
            }
            else if(op == "-"){
                str += "\t" + ret + " = sub " + res1 + ", " + res2 + "\n";  
            }
            break;
    }
    return ret;
}

std::string MulExprAST::koopa_ir() const { 
    std::string ret;
    switch(flag){
        case ONLY_UNARY:
            ret = unary_expr->koopa_ir();
            break;
        case MUL_UNARY:
            auto res1 = mul_expr->koopa_ir();
            auto res2 = unary_expr->koopa_ir();
            ret = "%"+std::to_string(cnt++);
            if(op == "*"){
                str += "\t" + ret + " = mul " + res1 + ", " + res2 + "\n";  
            }
            else if(op == "/"){
                str += "\t" + ret + " = div " + res1 + ", " + res2 + "\n";  
            }
            else if(op == "%"){
                str += "\t" + ret + " = mod " + res1 + ", " + res2 + "\n";  
            }
            break;
    }
    return ret;
}

std::string LOrExprAST::koopa_ir() const {
    std::string ret;
    switch(flag){
        case ONLY_LAND:
            ret = land_expr->koopa_ir();
            break;
        case LOR_LAND:
            std::string now_if = std::to_string(block_cnt);
            block_cnt++;

            ret = "@result_" + now_if;
            str += "\t" + ret + " = alloc i32\n";
            str += "\tstore 1, " + ret + "\n"; 

            auto res1 = lor_expr->koopa_ir();
            std::string then_block_name = "%then_"+now_if+"_b";
            std::string end_block_name = "%end_"+now_if+"_b";
            std::string cond = "%" + std::to_string(cnt++);
            str += "\t" + cond + " = eq " + res1 + ", 0\n";
            str += "\tbr " + cond + ", " + then_block_name + ", " + end_block_name + "\n";
            str += "\n";
            
            str += then_block_name+":\n";
            now_block = then_block_name;
            block_end[then_block_name] = false;
            auto res2 = land_expr->koopa_ir();
            std::string tmp = "%" + std::to_string(cnt++);
            str += "\t" + tmp + " = ne " + res2 + ", 0\n";
            str += "\tstore " + tmp + ", "+ ret + "\n";
            str += "\tjump "+end_block_name+"\n";
            block_end[now_block] = true;
            str += "\n";

            now_block = end_block_name;
            block_end[end_block_name] = false;
            str += end_block_name+":\n";
            auto old_ret = ret;
            ret = "%" + std::to_string(cnt++);
            str += "\t" + ret + " = load " + old_ret + "\n";
            break;
    }
    return ret;
}

std::string LAndExprAST::koopa_ir() const { 
    std::string ret;
    switch(flag){
        case ONLY_EQ:
            ret = eq_expr->koopa_ir();
            break;
        case LAND_EQ:
            std::string now_if = std::to_string(block_cnt);
            block_cnt++;

            ret = "@result_" + now_if;
            str += "\t" + ret + " = alloc i32\n";
            str += "\tstore 0, " + ret + "\n"; 

            auto res1 = land_expr->koopa_ir();
            std::string then_block_name = "%then_"+now_if+"_b";
            std::string end_block_name = "%end_"+now_if+"_b";
            std::string cond = "%" + std::to_string(cnt++);
            str += "\t" + cond + " = ne " + res1 + ", 0\n";
            str += "\tbr " + cond + ", " + then_block_name + ", " + end_block_name + "\n";
            str += "\n";
            
            str += then_block_name+":\n";
            now_block = then_block_name;
            block_end[then_block_name] = false;
            auto res2 = eq_expr->koopa_ir();
            std::string tmp = "%" + std::to_string(cnt++);
            str += "\t" + tmp + " = ne " + res2 + ", 0\n";
            str += "\tstore " + tmp + ", "+ ret + "\n";
            str += "\tjump "+end_block_name+"\n";
            block_end[now_block] = true;
            str += "\n";

            now_block = end_block_name;
            block_end[end_block_name] = false;
            str += end_block_name+":\n";
            auto old_ret = ret;
            ret = "%" + std::to_string(cnt++);
            str += "\t" + ret + " = load " + old_ret + "\n";
            break;      
    }
    return ret;
}

std::string EqExprAST::koopa_ir() const { 
    std::string ret;
    switch(flag){
        case ONLY_REL:
            ret = rel_expr->koopa_ir();
            break;
        case EQ_REL:
            auto res1 = eq_expr->koopa_ir();
            auto res2 = rel_expr->koopa_ir();
            ret = "%"+std::to_string(cnt++);
            if(op == "=="){
                str += "\t" + ret + " = eq " + res1 + ", " + res2 + "\n";  
            }
            else if(op == "!="){
                str += "\t" + ret + " = ne " + res1 + ", " + res2 + "\n";  
            }
            break;
    }
    return ret;
}

std::string RelExprAST::koopa_ir() const {
    std::string ret;
    switch(flag){
        case ONLY_ADD:
            ret = add_expr->koopa_ir();
            break;
        case REL_ADD:
            auto res1 = rel_expr->koopa_ir();
            auto res2 = add_expr->koopa_ir();
            ret = "%"+std::to_string(cnt++);
            if(op == "<"){
                str += "\t" + ret + " = lt " + res1 + ", " + res2 + "\n";  
            }
            else if(op == ">"){
                str += "\t" + ret + " = gt " + res1 + ", " + res2 + "\n";  
            }
            else if(op == "<="){
                str += "\t" + ret + " = le " + res1 + ", " + res2 + "\n";  
            }
            else if(op == ">="){
                str += "\t" + ret + " = ge " + res1 + ", " + res2 + "\n";  
            }
            break;
    }
    return ret;
}

std::string LValAST::koopa_ir() const {
    if(flag==IDENT){
        if(symbol_tables[ident_floor(ident)][ident].dim_len.size()){
            std::string ret = "%" + std::to_string(cnt++);
            if(symbol_tables[ident_floor(ident)][ident].dim_len[0]!=-1)
                str += "\t" + ret + " = getelemptr %" + ident + "_" + std::to_string(ident_id(ident)) + ", 0" + "\n";
            else{
                std::string tmp = "%" + std::to_string(cnt++);
                str += "\t" + tmp + " = load %" + ident + "_" + std::to_string(ident_id(ident)) + "\n";
                str += "\t" + ret + " = getptr " + tmp + ", 0\n";
            }
            return ret;
        }
        else{
            if(symbol_tables[ident_floor(ident)][ident].is_const==true){
                return std::to_string(symbol_tables[ident_floor(ident)][ident].val);
            }
            else{
                std::string ret = "%" + std::to_string(cnt++);
                str += "\t" + ret + " = load %" + ident + "_" + std::to_string(ident_id(ident)) + "\n"; 
                return ret;
            }
        }
    }
    else if(flag==LVAL_IDX){
        std::string base_ptr = "%" + ident + "_" + std::to_string(ident_id(ident));
        std::string ret = "";
        for(int i=0;i<idx_lst.size();i++){
            std::string idx_ret = idx_lst[i]->koopa_ir();
            ret = "%ptr" + std::to_string(ptr_cnt++);
            if(symbol_tables[ident_floor(ident)][ident].dim_len[i]!=-1)
                str += "\t" + ret + " = getelemptr " + base_ptr + ", " + idx_ret + "\n";
            else{
                std::string tmp = "%" + std::to_string(cnt++);
                str += "\t" + tmp + " = load " + base_ptr + "\n";
                str += "\t" + ret + " = getptr " + tmp + ", " + idx_ret + "\n";
            }
            base_ptr = ret;
        }
        if(symbol_tables[ident_floor(ident)][ident].dim_len.size()!=idx_lst.size()){
            std::string tmp = "%" + std::to_string(cnt++);
            str += "\t" + tmp + " = getelemptr " + ret + ", 0\n";
            ret = tmp;
        }
        else{
            ret = "%" + std::to_string(cnt++);
            str += "\t" + ret + " = load " + base_ptr + "\n";
        }
        return ret;
    }
    return "";
}

std::string LeftLValAST::koopa_ir() const {
    if(flag==IDENT)
        return ident;
    else{
        std::string base_ptr = "%" + ident + "_" + std::to_string(ident_id(ident));
        std::string ret = "";
        for(int i=0;i<idx_lst.size();i++){
            std::string idx_ret = idx_lst[i]->koopa_ir();
            ret = "%ptr" + std::to_string(ptr_cnt++);
            if(symbol_tables[ident_floor(ident)][ident].dim_len[i]!=-1)
                str += "\t" + ret + " = getelemptr " + base_ptr + ", " + idx_ret + "\n";
            else{
                std::string tmp = "%" + std::to_string(cnt++);
                str += "\t" + tmp + " = load " + base_ptr + "\n";
                str += "\t" + ret + " = getptr " + tmp + ", " + idx_ret + "\n";
            }
            base_ptr = ret;
        }
        return ret;
    }
}

std::string DeclAST::koopa_ir() const {
    if(block_end[now_block]){
        return "";
    }
    if(flag==CONST_DECL){
        const_decl->koopa_ir();
    }
    else if(flag==VAR_DECL){
        var_decl->koopa_ir();
    }
    return "";
}

std::string ConstDeclAST::koopa_ir() const {
    for(auto& item: const_def_list){
        item->koopa_ir();
    }
    return "";
}

std::string ConstDefAST::koopa_ir() const {
    if(flag==VAR){
        symbol_tables[block_num][ident] = MyVar("int", const_init_val->getVal(), true, ident_cnt++);
    }
    else if(flag==ARRAY){
        symbol_tables[block_num][ident] = MyVar("array", 0, true, ident_cnt++);
        int total_len = 1;
        int* dim_len = new int[len.size()];
        int iter=0;
        for(auto& it:len){
            total_len *= it->getVal();
            dim_len[iter++]=it->getVal();
        }
        BaseExprAST** init_array = new BaseExprAST*[total_len];
        memset(init_array, 0, total_len*sizeof(BaseExprAST*));
        init2array(init_array, dim_len, dim_len+len.size(), (VarInitValAST*)const_init_val.get());

        for(int i=0;i<total_len;i++){
            if(init_array[i]){
                symbol_tables[block_num][ident].array.push_back(init_array[i]->getVal());
            }
            else{
                symbol_tables[block_num][ident].array.push_back(0);
            }
        }
        symbol_tables[block_num][ident].dim_len.assign(dim_len, dim_len+len.size());

        if(ident_floor(ident)!=0){
            str += "\t%" + ident + "_" + std::to_string(ident_id(ident)) + " = alloc ";
            for(int* it = dim_len; it!=dim_len+len.size(); it++){
                str += "[";
            }
            str += "i32";
            for(int*it = dim_len+len.size()-1; it!=dim_len-1; it--){
                str += ", " + std::to_string(*it) + "]";
            }
            str += "\n";

            std::string base_ptr = "%" + ident + "_" + std::to_string(ident_id(ident));
            int idx = 0;
            str += array2ptr(init_array, &idx, base_ptr, dim_len, dim_len+len.size());
        }
        else{
            str += "global %" + ident + "_" + std::to_string(ident_id(ident)) + " = alloc ";
            for(int* it = dim_len; it!=dim_len+len.size(); it++){
                str += "[";
            }
            str += "i32";
            for(int*it = dim_len+len.size()-1; it!=dim_len-1; it--){
                str += ", " + std::to_string(*it) + "]";
            }
            str += ", ";
            int idx = 0;
            str += array2Aggregate(init_array, &idx, dim_len, dim_len+len.size());
            str += "\n";
        }
    }
    return "";
}

std::string ConstInitValAST::koopa_ir() const {
    return "";
}

std::string ConstExprAST::koopa_ir() const {
    std::string ret = expr->koopa_ir();
    return ret;
}

std::string VarDeclAST::koopa_ir() const {
    for(auto& item: var_def_list){
        item->koopa_ir();
    }
    return "";
}

std::string VarDefAST::koopa_ir() const {
    if(flag==VAR){
        if(is_init){
            int val = var_init_val->getVal();
            symbol_tables[block_num][ident] = MyVar("int", val, false, ident_cnt++); 
        }
        else{
            symbol_tables[block_num][ident] = MyVar("int", 0, false, ident_cnt++, true);
        }

        if(ident_floor(ident)!=0){
            std::string type = "i32";
            str += "\t%" + ident + "_" + std::to_string(ident_id(ident)) + " = alloc " + type + '\n';
            if(is_init){
                str += "\tstore " + var_init_val->koopa_ir() + ", %" + ident + "_" + std::to_string(ident_id(ident)) + "\n";
            }
        }
        else{
            str += "global %" + ident + "_" + std::to_string(ident_id(ident)) + " = alloc i32, ";
            if(is_init){
                str+= var_init_val->koopa_ir() + '\n';
            } 
            else{
                str+= "0\n";
            }
        }
    }
    else if(flag==ARRAY){
        symbol_tables[block_num][ident] = MyVar("array", 0, false, ident_cnt++);
        int total_len = 1;
        int* dim_len = new int[len.size()];
        int iter=0;
        for(auto& it:len){
            total_len *= it->getVal();
            dim_len[iter++]=it->getVal();
        }
        BaseExprAST** init_array = new BaseExprAST*[total_len];
        memset(init_array, 0, total_len*sizeof(BaseExprAST*));
        init2array(init_array, dim_len, dim_len+len.size(), (VarInitValAST*)var_init_val.get());

        for(int i=0;i<total_len;i++){
            if(init_array[i]){
                symbol_tables[block_num][ident].array.push_back(init_array[i]->getVal());
            }
            else{
                symbol_tables[block_num][ident].array.push_back(0);
            }
        }
        symbol_tables[block_num][ident].dim_len.assign(dim_len, dim_len+len.size());

        if(ident_floor(ident)!=0){
            str += "\t%" + ident + "_" + std::to_string(ident_id(ident)) + " = alloc ";
            for(int* it = dim_len; it!=dim_len+len.size(); it++){
                str += "[";
            }
            str += "i32";
            for(int*it = dim_len+len.size()-1; it!=dim_len-1; it--){
                str += ", " + std::to_string(*it) + "]";
            }
            str += "\n";

            std::string base_ptr = "%" + ident + "_" + std::to_string(ident_id(ident));
            int idx = 0;
            str += array2ptr(init_array, &idx, base_ptr, dim_len, dim_len+len.size());
        }
        else{
            str += "global %" + ident + "_" + std::to_string(ident_id(ident)) + " = alloc ";
            for(int* it = dim_len; it!=dim_len+len.size(); it++){
                str += "[";
            }
            str += "i32";
            for(int*it = dim_len+len.size()-1; it!=dim_len-1; it--){
                str += ", " + std::to_string(*it) + "]";
            }
            str += ", ";
            int idx=0;
            if(all_zero(init_array, &idx, dim_len, dim_len+len.size())){
                str+="zeroinit\n";
            }
            else{
                idx = 0;
                str += array2Aggregate(init_array, &idx, dim_len, dim_len+len.size());
            }
            str += "\n";
        }
    }
    return "";
}

std::string array2ptr(BaseExprAST** init_array, int* idx,std::string base_ptr, int* dim, int* dim_end){
    std::string ret = "";
    for(int i=0;i<*dim;i++){
        std::string now_ptr = "%ptr" + std::to_string(ptr_cnt++);
        ret += "\t" + now_ptr + " = getelemptr " + base_ptr + ", " + std::to_string(i) + "\n";
        if(dim+1==dim_end){
            if(init_array[*idx]){
                ret += "\tstore " + init_array[*idx]->koopa_ir() + ", " + now_ptr + "\n";
            }
            else{
                ret += "\tstore 0, " + now_ptr + "\n";
            }
            *idx+=1;
        }
        else{
            ret += array2ptr(init_array, idx, now_ptr, dim+1, dim_end);
        }
    }
    return ret;
}

bool all_zero(BaseExprAST** init_array, int* idx, int* dim, int* dim_end){
    for(int i=0;i<*dim;i++){
        if(dim+1==dim_end){
            if(init_array[*idx]){
                if(init_array[*idx]->getVal()!=0) return false;
            }
            *idx+=1;
        }
        else{
            if(!all_zero(init_array, idx, dim+1, dim_end))  return false;
        }
    }
    return true;
}

std::string array2Aggregate(BaseExprAST** init_array, int* idx, int* dim, int* dim_end){
    std::string ret = "{";
    for(int i=0;i<*dim;i++){
        if(dim+1==dim_end){
            if(init_array[*idx]){
                ret += std::to_string(init_array[*idx]->getVal());
            }
            else{
                ret+= "0";
            }
            *idx+=1;
            if(i!=*dim-1) ret += ", ";
        }
        else{
            ret += array2Aggregate(init_array, idx, dim+1, dim_end);
            if(i!=*dim-1) ret += ", ";
        }
    }
    ret += "}";
    return ret;
}

void init2array(BaseExprAST** start, int* len_start, int* len_end, const VarInitValAST* var_init_val){
    BaseExprAST** idx=start;
    for(auto& it: var_init_val->expr_list){
        auto ptr=(VarInitValAST*)it.get();
        if(ptr->flag==VarInitValAST::EXPR){
            *idx = (BaseExprAST*)(ptr->expr).get();
            idx++;
        }
        else if(ptr->flag==VarInitValAST::ARRAY){
            init2array(idx, len_start+1, len_end, ptr);
            int jump=1;
            int delta = idx-start;
            for(int* it=len_end-1; it!=len_start; it--){
                if(delta%((*it)*jump)!=0){
                    break;
                }
                jump*=*it;
            }
            idx+=jump;
        }
    }
}

std::string VarInitValAST::koopa_ir() const {
    return expr->koopa_ir();
}

int ident_floor(std::string ident){

    for(int i = block_num; i>=0; i--){
        if(symbol_tables[i].find(ident)!=symbol_tables[i].end()){
            return i;
        }
    }
    return block_num;
}

int ident_id(std::string ident){
    for(int i = block_num; i>=0; i--){
        if(symbol_tables[i].find(ident)!=symbol_tables[i].end()){
            return symbol_tables[i][ident].id;
        }
    }
    return 0;
}

void show_table(){
    for(int i=0;i<=block_num;i++){
        for(auto it = symbol_tables[i].begin(); it!= symbol_tables[i].end(); it++){
            std::cout<<it->first<<" : "<<it->second.val<<"  "<<it->second.type<<"  "<<it->second.is_const<<std::endl;
        }
    }
    std::cout<<std::endl;
}