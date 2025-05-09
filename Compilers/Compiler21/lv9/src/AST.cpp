#include "AST.hpp"


// 赋值符号的编号
int num_calculate = 0;
// if语句的计数
int if_num = -1;
// while语句的计数
int while_num = -1;
// 为break和continue的while_block
// while_num仅用于标签的命名，不能显示层级关系
int while_block = -1;
std::unordered_map<int, int> whileMap;
bool in_global = false;

std::vector<std::unordered_map<std::string, SymValue>*> table;
int num_block = 0;
// 每个块的父块
std::unordered_map<int, int> father;

int BaseAST::calculateValue() { assert(0); }
int BaseAST::ConstInitType() { assert(0); }
int BaseAST::InitType() { assert(0); }
int ConstInitValType_1_AST::ConstInitType() { return 1; }
int ConstInitValType_2_AST::ConstInitType() { return 2; }
int InitValType_1_AST::InitType() { return 1; }
int InitValType_2_AST::InitType() { return 2; }
std::string BaseAST::LValIDENT() { return ""; }
std::string LValAST::LValIDENT() { return ident; }
std::vector<bool> ifHasReturn;

std::pair<SymValue, int> search(const std::string& ident){
    int cur = num_block;
    while(cur != -1){
        if((*table[cur]).find(ident) == (*table[cur]).end()){
            cur = father[cur];
        }
        else{
            return std::make_pair((*table[cur]).find(ident)->second, cur);
        }
    }
    assert(0);
}

void printAgg(std::vector<int> agg, int pos, std::vector<int> vec_len, std::vector<int> multiple_len, int layer){
    if(layer == vec_len.size()){
        koopa_str = koopa_str + std::to_string(agg[pos]);
        std::cout << agg[pos];
    }
    else{
        koopa_str = koopa_str + "{";
        std::cout << "{";
        int size = multiple_len[layer] / vec_len[layer];
        for (int i = 0; i < vec_len[layer]; i++){
            printAgg(agg, pos + i * size, vec_len, multiple_len, layer + 1);
            if(i != vec_len[layer] - 1){
                koopa_str = koopa_str + ", ";
                std::cout << ", ";
            }
        }
        koopa_str = koopa_str + "}";
        std::cout << "}";
    }
}

void printStoreAgg(std::vector<int> agg, int pos, std::vector<int> vec_len, std::vector<int> multiple_len, int layer, std::string ident, int num_block){
    if(layer == vec_len.size()){
        koopa_str = koopa_str + "  store " + std::to_string(agg[pos]) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  store " << agg[pos] << ", %" << num_calculate - 1 << std::endl;
    }
    else{
        int size = multiple_len[layer] / vec_len[layer];
        int nowcnt = num_calculate - 1;
        for (int i = 0; i < vec_len[layer]; i++){
            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr ";
            std::cout << "  %" << num_calculate << " = getelemptr ";
            if(layer == 0){
                koopa_str = koopa_str + "@" + ident + "_" + std::to_string(num_block);
                std::cout << "@" << ident << "_" << num_block;
            }
            else{
                koopa_str = koopa_str + "%" + std::to_string(nowcnt);
                std::cout << "%" << nowcnt;
            }
            koopa_str = koopa_str + ", " + std::to_string(i) + "\n";
            std::cout << ", " << i << std::endl;
            num_calculate++;
            printStoreAgg(agg, pos + i * size, vec_len, multiple_len, layer + 1, ident, num_block);
        }
    }
}

void printStoreVar(std::vector<int> agg, int pos, std::vector<int> vec_len, std::vector<int> multiple_len, int layer, std::string ident, int num_block){
    if(layer == vec_len.size()){
        int judge = agg[pos];
        if(judge != -1){
            koopa_str = koopa_str + "  store %" + std::to_string(agg[pos]) + ", %" + std::to_string(num_calculate - 1) + "\n";
            std::cout << "  store %" << agg[pos] << ", %" << num_calculate - 1 << std::endl;
        }
        else{
            koopa_str = koopa_str + "  store 0, %" + std::to_string(num_calculate - 1) + "\n";
            std::cout << "  store 0, %" << num_calculate - 1 << std::endl;
        }
    }
    else{
        int size = multiple_len[layer] / vec_len[layer];
        int nowcnt = num_calculate - 1;
        for (int i = 0; i < vec_len[layer]; i++){
            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr ";
            std::cout << "  %" << num_calculate << " = getelemptr ";
            if(layer == 0){
                koopa_str = koopa_str + "@" + ident + "_" + std::to_string(num_block);
                std::cout << "@" << ident << "_" << num_block;
            }
            else{
                koopa_str = koopa_str + "%" + std::to_string(nowcnt);
                std::cout << "%" << nowcnt;
            }
            koopa_str = koopa_str + ", " + std::to_string(i) + "\n";
            std::cout << ", " << i << std::endl;
            num_calculate++;
            printStoreVar(agg, pos + i * size, vec_len, multiple_len, layer + 1, ident, num_block);
        }
    }
}


// 关于Dump()
void CompUnitAST::Dump() const {
    std::cout << "CompUnitAST { ";
    // func_def->Dump();
    std::cout << " }";
}

void FuncDefAST::Dump() const {
    std::cout << "FuncDefAST { ";
    //func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}


void BlockAST::Dump() const {
    std::cout << "BlockAST { ";
    for (auto iter = blockItemVec.begin(); iter != blockItemVec.end(); iter++){
        (*iter)->Dump();
    }
    std::cout << " }";
}

void StmtType_1_AST::Dump() const{
    std::cout << "StmtAST { ";
    exp->Dump();
    std::cout << " }";
}
void StmtType_2_AST::Dump() const{}

void ExpAST::Dump() const{
    std::cout << "ExpAST { ";
    lOrExp->Dump();
    std::cout << " }";
}

void PrimaryExpType_1_AST::Dump() const{
    std::cout << "PrimaryExpType_1_AST { ";
    exp->Dump();
    std::cout << " }";
}

void PrimaryExpType_2_AST::Dump() const{
    std::cout << "PrimaryExpType_1_AST { " << number << " }";
}

void UnaryExpType_1_AST::Dump() const{
    std::cout << "UnaryExpType_1_AST {";
    primaryExp->Dump();
    std::cout << " }";
}

void UnaryExpType_2_AST::Dump() const{
    std::cout << "UnaryExpType_2_AST {" << unaryOp << ", ";
    unaryExp->Dump();
    std::cout << " }";
}

void MulExpType_1_AST::Dump() const{}
void MulExpType_2_AST::Dump() const{}
void AddExpType_1_AST::Dump() const{}
void AddExpType_2_AST::Dump() const{}
void RelExpType_1_AST::Dump() const{}
void RelExpType_2_AST::Dump() const{}
void EqExpType_1_AST::Dump() const{}
void EqExpType_2_AST::Dump() const{}
void LAndExpType_1_AST::Dump() const{}
void LAndExpType_2_AST::Dump() const{}
void LOrExpType_1_AST::Dump() const{}
void LOrExpType_2_AST::Dump() const{}

void DeclType_1_AST::Dump() const{}
void DeclType_2_AST::Dump() const{}
void ConstDeclAST::Dump() const{}
// void BTypeAST::Dump() const{}
void ConstDefAST::Dump() const{}
void ConstInitValType_1_AST::Dump() const{}
void BlockItemType_1_AST::Dump() const{}
void BlockItemType_2_AST::Dump() const{}
void LValAST::Dump() const{}
void PrimaryExpType_3_AST::Dump() const{}
void ConstExpAST::Dump() const{}

void VarDeclAST::Dump() const{}
void VarDefType_1_AST::Dump() const{}
void VarDefType_2_AST::Dump() const{}
void InitValType_1_AST::Dump() const{}

void StmtType_3_AST::Dump() const{}
void StmtType_4_AST::Dump() const{}

void StmtType_5_AST::Dump() const{}

void StmtType_6_AST::Dump() const{}
void StmtType_7_AST::Dump() const{}
void StmtType_8_AST::Dump() const{}

void CompUnitItemType_1_AST::Dump() const{}
void FuncFParamAST::Dump() const{}
void UnaryExpType_3_AST::Dump() const{}
void CompUnitItemType_2_AST::Dump() const{}

void ConstInitValType_2_AST::Dump() const{}
void InitValType_2_AST::Dump() const{}
// 关于generateKoopaIR
void CompUnitAST::generateKoopaIR() const{
    std::unordered_map<std::string, SymValue> nowtable;
    table.push_back(&nowtable);
    ifHasReturn.push_back(false);
    father[0] = -1;

    koopa_str = koopa_str + "decl @getint(): i32\n";
    std::cout << "decl @getint(): i32\n";
    koopa_str = koopa_str + "decl @getch(): i32\n";
    std::cout << "decl @getch(): i32\n";
    koopa_str = koopa_str + "decl @getarray(*i32): i32\n";
    std::cout << "decl @getarray(*i32): i32\n";
    koopa_str = koopa_str + "decl @putint(i32)\n";
    std::cout << "decl @putint(i32)\n";
    koopa_str = koopa_str + "decl @putch(i32)\n";
    std::cout << "decl @putch(i32)\n";
    koopa_str = koopa_str + "decl @putarray(i32, *i32)\n";
    std::cout << "decl @putarray(i32, *i32)\n";
    koopa_str = koopa_str + "decl @starttime()\n";
    std::cout << "decl @starttime()\n";
    koopa_str = koopa_str + "decl @stoptime()\n";
    std::cout << "decl @stoptime()\n\n";

    (*table[num_block]).insert(std::make_pair("getint", SymValue(SYM_FUNC_INT, 0)));
    (*table[num_block]).insert(std::make_pair("getch", SymValue(SYM_FUNC_INT, 0)));
    (*table[num_block]).insert(std::make_pair("getarray", SymValue(SYM_FUNC_INT, 0)));
    (*table[num_block]).insert(std::make_pair("putint", SymValue(SYM_FUNC_VOID, 0)));
    (*table[num_block]).insert(std::make_pair("putch", SymValue(SYM_FUNC_VOID, 0)));
    (*table[num_block]).insert(std::make_pair("putarray", SymValue(SYM_FUNC_VOID, 0)));
    (*table[num_block]).insert(std::make_pair("starttime", SymValue(SYM_FUNC_VOID, 0)));
    (*table[num_block]).insert(std::make_pair("stoptime", SymValue(SYM_FUNC_VOID, 0)));


    for (auto iter = compUnitItemVec.begin(); iter != compUnitItemVec.end(); iter++){
        (*iter)->generateKoopaIR();
    }
}

void CompUnitItemType_1_AST::generateKoopaIR() const{
    funcDef->generateKoopaIR();
}
void CompUnitItemType_2_AST::generateKoopaIR() const{
    in_global = true;
    decl->generateKoopaIR();
    in_global = false;
}

void FuncDefAST::generateKoopaIR() const{
    
    num_calculate = 0;
    if(func_type == "int"){
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_FUNC_INT, 0)));
    }
    else if (func_type == "void"){
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_FUNC_VOID, 0)));
    }

    std::unordered_map<std::string, SymValue> nowtable;
    int father_block = num_block;
    num_block = table.size();
    int nowb = num_block;
    father[num_block] = father_block;
    table.push_back(&nowtable);

    ifHasReturn.push_back(false);

    koopa_str = koopa_str + "fun @" + ident + "(";
    std::cout << "fun @" << ident << "(";
    for (auto iter = funcFParamsVec.begin(); iter != funcFParamsVec.end(); iter++){
        if(iter != funcFParamsVec.begin()){
            koopa_str = koopa_str + ", ";
            std::cout << ", ";
        }
        (*iter)->generateKoopaIR();
    }
    koopa_str = koopa_str + ")";
    std::cout << ")";
    if(func_type == "int"){
        koopa_str += ": i32";
        std::cout << ": i32";
    }
    else if(func_type == "void"){

    }
    koopa_str += " {\n";
    std::cout << " {" << std::endl;
    koopa_str = koopa_str + "%" + "entry:\n";
    std::cout << "%" << "entry:" << std::endl;

    for (auto iter = funcFParamsVec.begin(); iter != funcFParamsVec.end(); iter++){
        if(dynamic_cast<FuncFParamAST*>((*iter).get())->type == 1){
            //std::cout << "i'm 1" << std::endl;
            std::string name = dynamic_cast<FuncFParamAST *>((*iter).get())->ident;
            koopa_str = koopa_str + "  @" + name + "_" + std::to_string(num_block) + " = alloc i32\n";
            std::cout << "  @" << name << "_" << num_block << " = alloc i32\n";
            koopa_str = koopa_str + "  store @" + name + ", @" + name + "_" + std::to_string(num_block) + "\n";
            std::cout << "  store @" << name << ", @" << name << "_" << num_block << std::endl;
            (*table[num_block]).insert(std::make_pair(name, SymValue(SYM_VARIABLE, 0)));
        }
        else{
            //std::cout << "i'm not 1" << std::endl;
            std::string name = dynamic_cast<FuncFParamAST *>((*iter).get())->ident;
            koopa_str = koopa_str + "  @" + name + "_" + std::to_string(num_block) + " = alloc *";
            std::cout << "  @" << name << "_" << num_block << " = alloc *";
            auto funcfparam = dynamic_cast<FuncFParamAST *>((*iter).get());
            for (int i = 0; i < funcfparam->constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32";
            std::cout << "i32";
            for (int i = funcfparam->constExpVec.size() - 1; i >= 0; i--){
                koopa_str = koopa_str + ", ";
                std::cout << ", ";
                int expValue = dynamic_cast<ConstExpAST *>(funcfparam->constExpVec[i].get())->calculateValue();
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
            }
            koopa_str = koopa_str + "\n";
            std::cout << std::endl;
            koopa_str = koopa_str + "  store @" + name + ", @" + name + "_" + std::to_string(num_block) + "\n";
            std::cout << "  store @" << name << ", @" << name << "_" << num_block << std::endl;
            (*table[num_block]).insert(std::make_pair(name, SymValue(SYM_PTR, funcfparam->constExpVec.size() + 1)));
            //std::cout << name << "  " << SYM_PTR << std::endl;
        }
    }

    block->generateKoopaIR();
    if(ident == "main" && !ifHasReturn[nowb]){
        koopa_str = koopa_str + "  ret 0\n";
        std::cout << "  ret 0\n";
    }
    else if(func_type == "void" && !ifHasReturn[nowb]){
        koopa_str = koopa_str + "  ret\n";
        std::cout << "  ret\n";
    }
    else if(func_type == "int" && !ifHasReturn[nowb]){
        koopa_str = koopa_str + "  ret 0\n";
        std::cout << "  ret 0\n";
    }
    koopa_str += "}\n\n";
    std::cout << "}\n\n";

    num_block = father[num_block];
}


// FuncFParam ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];
void FuncFParamAST::generateKoopaIR() const{
    if(type == 1){
        koopa_str = koopa_str + "@" + ident + ": i32";
        std::cout << "@" << ident << ": i32";
    }
    else{
        koopa_str = koopa_str + "@" + ident + ": *";
        std::cout << "@" << ident << ": *";
        for (int i = 0; i < constExpVec.size(); i++){
            koopa_str = koopa_str + "[";
            std::cout << "[";
        }
        koopa_str = koopa_str + "i32";
        std::cout << "i32";
        for (int i = constExpVec.size() - 1; i >= 0; i--){
            koopa_str = koopa_str + ", ";
            std::cout << ", ";
            int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
            koopa_str = koopa_str + std::to_string(expValue) + "]";
            std::cout << expValue << "]";
        }
    }
}


void BlockAST::generateKoopaIR() const{
    //std::cout << "block\n";

    std::unordered_map<std::string, SymValue> nowtable;
    int father_block = num_block;
    num_block = table.size();
    father[num_block] = father_block;
    table.push_back(&nowtable);

    ifHasReturn.push_back(false);
    //std::cout << "ifhasreturn=" << ifHasReturn[num_block];
    //int i = 0;

    for (auto iter = blockItemVec.begin(); iter != blockItemVec.end(); iter++){
        if(ifHasReturn[num_block])
            break;
            
        //std::cout << ++i << std::endl;
        (*iter)->generateKoopaIR();
    }

    if(father_block != -1){
        ifHasReturn[father_block] = ifHasReturn[num_block];
    }

    num_block = father[num_block];
}

void BlockItemType_1_AST::generateKoopaIR() const{
    decl->generateKoopaIR();
}

void BlockItemType_2_AST::generateKoopaIR() const{
    //std::cout << "stmt" << std::endl;
    stmt->generateKoopaIR();
}

void DeclType_1_AST::generateKoopaIR() const{
    constDecl->generateKoopaIR();
}
void DeclType_2_AST::generateKoopaIR() const{
    varDecl->generateKoopaIR();
}

void ConstDeclAST::generateKoopaIR() const{
    for (auto iter = constDefVec.begin(); iter != constDefVec.end(); iter++){
        (*iter)->generateKoopaIR();
    }
}

void ConstDefAST::generateKoopaIR() const{
    if(constExpVec.empty()){
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_CONST, constInitVal->calculateValue())));
    }
    else{
        if(in_global){
            // 全局常量数组
            koopa_str = koopa_str + "global @" + ident + "_" + std::to_string(num_block) + " = alloc ";
            std::cout << "global @" << ident << "_" << num_block << " = alloc ";
            for (int i = 0; i < constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32, ";
            std::cout << "i32, ";

            // 计算各个维度及各长度
            std::vector<int> vec_len;
            std::vector<int> multiple_len;
            for (int i = constExpVec.size() - 1; i >= 0; i--){
                int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
                vec_len.push_back(expValue);
                if(multiple_len.empty()){
                    multiple_len.push_back(expValue);
                }
                else{
                    multiple_len.push_back(multiple_len.back() * expValue);
                }
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
                if(i != 0){
                    koopa_str = koopa_str + ", ";
                    std::cout << ", ";
                }
            }
            std::reverse(multiple_len.begin(), multiple_len.end());
            std::reverse(vec_len.begin(), vec_len.end());

            std::vector<int> param = dynamic_cast<ConstInitValType_2_AST *>(constInitVal.get())->Aggregate(multiple_len);

            koopa_str = koopa_str + ", ";
            std::cout << ", ";
            printAgg(param, 0, vec_len, multiple_len, 0);
            koopa_str = koopa_str + "\n";
            std::cout << std::endl;
        }
        else{
            //局部常量数组
            koopa_str = koopa_str + "  @" + ident + "_" + std::to_string(num_block) + " = alloc ";
            std::cout << "  @" << ident << "_" << num_block << " = alloc ";
            for (int i = 0; i < constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32, ";
            std::cout << "i32, ";

            // 计算各个维度及各长度
            std::vector<int> vec_len;
            std::vector<int> multiple_len;
            for (int i = constExpVec.size() - 1; i >= 0; i--){
                int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
                vec_len.push_back(expValue);
                if(multiple_len.empty()){
                    multiple_len.push_back(expValue);
                }
                else{
                    multiple_len.push_back(multiple_len.back() * expValue);
                }
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
                if(i != 0){
                    koopa_str = koopa_str + ", ";
                    std::cout << ", ";
                }
            }
            std::reverse(multiple_len.begin(), multiple_len.end());
            std::reverse(vec_len.begin(), vec_len.end());

            std::vector<int> param = dynamic_cast<ConstInitValType_2_AST *>(constInitVal.get())->Aggregate(multiple_len);
            //改
            koopa_str = koopa_str + "\n";
            std::cout << std::endl;
            printStoreAgg(param, 0, vec_len, multiple_len, 0, ident, num_block);
        }
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_ARRAY_CONST, constExpVec.size())));
    }
}

std::vector<int> ConstInitValType_2_AST::Aggregate(std::vector<int> vec){
    std::vector<int> param;
    for (auto iter = constInitValVec.begin(); iter != constInitValVec.end(); iter++){
        if((*iter)->ConstInitType() == 1){
            param.push_back(dynamic_cast<ConstInitValType_1_AST *>((*iter).get())->calculateValue());
        }
        else if((*iter)->ConstInitType() == 2){
            auto iter2 = vec.begin();
            iter2++;
            for (; iter2 != vec.end(); iter2++){
                if(param.size() % (*iter2) == 0){
                    std::vector<int> sub_vec;
                    std::copy(iter2, vec.end(), std::back_inserter(sub_vec));
                    std::vector<int> tmpAgg = dynamic_cast<ConstInitValType_2_AST *>((*iter).get())->Aggregate(sub_vec);
                    param.insert(param.end(), tmpAgg.begin(), tmpAgg.end());
                    break;
                }
            }
        }
    }
    param.insert(param.end(), vec[0] - param.size(), 0);
    return param;
}

void ConstInitValType_2_AST::generateKoopaIR() const{}

// void BTypeAST::generateKoopaIR() const{}
void ConstInitValType_1_AST::generateKoopaIR() const{}
void ConstExpAST::generateKoopaIR() const{}

void LValAST::generateKoopaIR() const{
    //std::cout << "lval " << ident << std::endl;
    if(search(ident).first.type == SYM_CONST){
        //std::cout << "i'm const" << std::endl;
        int number = search(ident).first.value;
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = add 0, " + std::to_string(number) + "\n";
        std::cout << "  %" << num_calculate << " = add 0, " << number << std::endl;
        num_calculate++;
    }
    else if(search(ident).first.type == SYM_VARIABLE){
        //std::cout << "i'm var" << std::endl;
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load @" + ident + "_" + std::to_string(search(ident).second) + "\n";
        std::cout << "  %" << num_calculate << " = load @" << ident << "_" << search(ident).second << std::endl;
        num_calculate++;
    }
    else if(search(ident).first.type == SYM_ARRAY_CONST || search(ident).first.type == SYM_ARRAY_VAR){
        //std::cout << "i'm array";
        // arr[2][4]
        for (int i = 0; i < expVec.size(); i++){
            int last = num_calculate - 1;
            expVec[i]->generateKoopaIR();
            int expcnt = num_calculate - 1;

            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr ";
            std::cout << "  %" << num_calculate << " = getelemptr ";
            if(i == 0){
                koopa_str = koopa_str + "@" + ident + "_" + std::to_string(search(ident).second);
                std::cout << "@" << ident << "_" << search(ident).second;
            }
            else{
                koopa_str = koopa_str + "%" + std::to_string(last);
                std::cout << "%" << last;
            }
            koopa_str = koopa_str + ", %" + std::to_string(expcnt) + "\n";
            std::cout << ", %" << expcnt << std::endl;
            num_calculate++;
        }
        if(search(ident).first.value == expVec.size()){
            //arr[1][3]
            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load %" + std::to_string(num_calculate - 1) + "\n";
            std::cout << "  %" << num_calculate << " = load %" << num_calculate - 1 << std::endl;
            num_calculate++;
        }
        else{
            //arr[1]
            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr ";
            std::cout << "  %" << num_calculate << " = getelemptr ";
            if(expVec.size() == 0){
                //arr
                koopa_str = koopa_str + "@" + ident + "_" + std::to_string(search(ident).second);
                std::cout << "@" << ident << "_" << search(ident).second;
            }
            else{
                //arr[1]
                koopa_str = koopa_str + "%" + std::to_string(num_calculate - 1);
                std::cout << "%" << num_calculate - 1;
            }
            koopa_str = koopa_str + ", 0\n";
            std::cout << ", 0\n";
            num_calculate++;
        }
    }
    else if(search(ident).first.type == SYM_PTR){
        //std::cout << "i'm ptr" << std::endl;
        //(*arr)[3]
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load @" + ident + "_" + std::to_string(search(ident).second) + "\n";
        std::cout << "  %" << num_calculate << " = load @" << ident << "_" << search(ident).second << std::endl;
        //std::cout << "numc=" << num_calculate << std::endl;
        num_calculate++;
        //std::cout << "numc=" << num_calculate << std::endl;
        for (int i = 0; i < expVec.size(); i++){
            int last = num_calculate - 1;
            expVec[i]->generateKoopaIR();
            int expcnt = num_calculate - 1;

            if(i == 0){
                koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getptr %";
                std::cout << "  %" << num_calculate << " = getptr %";
            }
            else{
                koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr %";
                std::cout << "  %" << num_calculate << " = getelemptr %";
            }
            koopa_str = koopa_str + std::to_string(last) + ", %" + std::to_string(expcnt) + "\n";
            std::cout << last << ", %" << expcnt << std::endl;
            num_calculate++;
        }
        if(search(ident).first.value == expVec.size()){
            //arr[1][2]
            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load %" + std::to_string(num_calculate - 1) + "\n";
            std::cout << "  %" << num_calculate << " = load %" << num_calculate - 1 << std::endl;
            num_calculate++;
        }
        else{
            //arr[1]
            if(expVec.size() == 0){
                koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getptr %";
                std::cout << "  %" << num_calculate << " = getptr %";
            }
            else{
                koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr %";
                std::cout << "  %" << num_calculate << " = getelemptr %";
            }
            koopa_str = koopa_str + std::to_string(num_calculate - 1) + ", 0\n";
            std::cout << num_calculate - 1 << ", 0\n";
            num_calculate++;
        }
    }
}

// Stmt          ::=  "return" [Exp] ";" 
                    // | LVal "=" Exp ";" 
                    // | [Exp] ";" 
                    // | Block 
                    // | "if" "(" Exp ")" Stmt ["else" Stmt] 
                    // | "while" "(" Exp ")" Stmt;
                    // | "break" ";"                          7
                    // | "continue" ";";                      8
void StmtType_1_AST::generateKoopaIR() const{
    if(exp){
        exp->generateKoopaIR();
        koopa_str = koopa_str + "  ret %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  ret %" << num_calculate - 1 << std::endl;
    }
    else{
        koopa_str = koopa_str + "  ret\n";
        std::cout << "  ret" << std::endl;
    }
    ifHasReturn[num_block] = true;
}
// don't need to call function lVal->generateKoopaIR()
void StmtType_2_AST::generateKoopaIR() const{
    std::string name = lVal->LValIDENT();
    if(search(name).first.type == SYM_VARIABLE){
        exp->generateKoopaIR();
        std::string ident = lVal->LValIDENT();
        koopa_str = koopa_str + "  store %" + std::to_string(num_calculate - 1) + ", @" + ident + "_" + std::to_string(search(ident).second) + "\n";
        std::cout << "  store %" << num_calculate - 1 << ", @" << ident << "_" << search(ident).second << std::endl;
    }
    else if(search(name).first.type == SYM_ARRAY_VAR){
        auto lvalson = dynamic_cast<LValAST *>(lVal.get());
        exp->generateKoopaIR();
        int expcnt = num_calculate - 1;
        for (int i = 0; i < lvalson->expVec.size(); i++){
            int last = num_calculate - 1;
            dynamic_cast<ExpAST *>((lvalson->expVec)[i].get())->generateKoopaIR();
            int tmpexpcnt = num_calculate - 1;
            koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr ";
            std::cout << "  %" << num_calculate << " = getelemptr ";
            if(i == 0){
                koopa_str = koopa_str + "@" + lvalson->ident + "_" + std::to_string(search(lvalson->ident).second);
                std::cout << "@" << lvalson->ident << "_" << search(lvalson->ident).second;
            }
            else{
                koopa_str = koopa_str + "%" + std::to_string(last);
                std::cout << "%" << last;
            }
            koopa_str = koopa_str + ", %" + std::to_string(tmpexpcnt) + "\n";
            std::cout << ", %" << tmpexpcnt << std::endl;
            num_calculate++;
        }
        koopa_str = koopa_str + "  store %" + std::to_string(expcnt) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  store %" << expcnt << ", %" << num_calculate - 1 << std::endl;
    }
    else if(search(name).first.type == SYM_PTR){
        //std::cout << "STMTPTR" << std::endl;
        exp->generateKoopaIR();
        int expcnt = num_calculate - 1;
        auto lvalson = dynamic_cast<LValAST *>(lVal.get());
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load @" + name + "_" + std::to_string(search(name).second) + "\n";
        std::cout << "  %" << num_calculate << " = load @" << name << "_" << search(name).second << std::endl;
        num_calculate++;
        for(int i = 0; i < lvalson->expVec.size(); i++){
            int last = num_calculate - 1;
            dynamic_cast<ExpAST *>((lvalson->expVec)[i].get())->generateKoopaIR();
            int tmpexpcnt = num_calculate - 1;
            if(i == 0){
                koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getptr %";
                std::cout << "  %" << num_calculate << " = getptr %";
            }
            else{
                koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = getelemptr %";
                std::cout << "  %" << num_calculate << " = getelemptr %";
            }
            num_calculate++;
            koopa_str = koopa_str + std::to_string(last) + ", %" + std::to_string(tmpexpcnt) + "\n";
            std::cout << last << ", %" << tmpexpcnt << std::endl;
        }
        koopa_str = koopa_str + "  store %" + std::to_string(expcnt) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  store %" << expcnt << ", %" << num_calculate - 1 << std::endl;
    }
}
void StmtType_3_AST::generateKoopaIR() const{
    if(exp){
        exp->generateKoopaIR();
    }
}
void StmtType_4_AST::generateKoopaIR() const{
    block->generateKoopaIR();
}
void StmtType_5_AST::generateKoopaIR() const{
    if_num++;
    int now_if_num = if_num;
    if(elseStmt){
        exp->generateKoopaIR();
        koopa_str = koopa_str + "  br %" + std::to_string(num_calculate - 1) + ", %yyx_then_" + std::to_string(now_if_num) + ", %yyx_else_" + std::to_string(now_if_num) + "\n";
        std::cout << "  br %" << num_calculate - 1 << ", %yyx_then_" << now_if_num << ", %yyx_else_" << now_if_num << std::endl;

        koopa_str = koopa_str + "%yyx_then_" + std::to_string(now_if_num) + ":\n";
        std::cout << "%yyx_then_" << now_if_num << ":\n";
        ifStmt->generateKoopaIR();
        if(!ifHasReturn[num_block]){
            koopa_str = koopa_str + "  jump %yyx_end_" + std::to_string(now_if_num) + "\n";
            std::cout << "  jump %yyx_end_" << now_if_num << std::endl;
        }
        bool if_ret = ifHasReturn[num_block];
        ifHasReturn[num_block] = false;

        koopa_str = koopa_str + "%yyx_else_" + std::to_string(now_if_num) + ":\n";
        std::cout << "%yyx_else_" << now_if_num << ":\n";

        elseStmt->generateKoopaIR();

        if(!ifHasReturn[num_block]){
            koopa_str = koopa_str + "  jump %yyx_end_" + std::to_string(now_if_num) + "\n";
            std::cout << "  jump %yyx_end_" << now_if_num << std::endl;
        }
        bool else_ret = ifHasReturn[num_block];
        ifHasReturn[num_block] = false;

        if(if_ret && else_ret){
            ifHasReturn[num_block] = true;
        }
        else{
            koopa_str = koopa_str + "%yyx_end_" + std::to_string(now_if_num) + ":\n";
            std::cout << "%yyx_end_" << now_if_num << ":\n";
        }
    }
    else{
        exp->generateKoopaIR();
        koopa_str = koopa_str + "  br %" + std::to_string(num_calculate - 1) + ", %yyx_then_" + std::to_string(now_if_num) + ", %yyx_end_" + std::to_string(now_if_num) + "\n";
        std::cout << "  br %" << num_calculate - 1 << ", %yyx_then_" << now_if_num << ", %yyx_end_" << now_if_num << std::endl;

        koopa_str = koopa_str + "%yyx_then_" + std::to_string(now_if_num) + ":\n";
        std::cout << "%yyx_then_" << now_if_num << ":\n";
        ifStmt->generateKoopaIR();
        if(!ifHasReturn[num_block]){
            koopa_str = koopa_str + "  jump %yyx_end_" + std::to_string(now_if_num) + "\n";
            std::cout << "  jump %yyx_end_" << now_if_num << std::endl;
        }
        ifHasReturn[num_block] = false;
        koopa_str = koopa_str + "%yyx_end_" + std::to_string(now_if_num) + ":\n";
        std::cout << "%yyx_end_" << now_if_num << ":\n";
    }
}
void StmtType_6_AST::generateKoopaIR() const{
    while_num++;
    int now_while = while_num;
    while_block++;
    whileMap[while_block] = while_num;

    koopa_str = koopa_str + "  jump %yyxwhile_entry_" + std::to_string(now_while) + "\n";
    std::cout << "  jump %yyxwhile_entry_" << now_while << std::endl;
    // while(i<10)
    koopa_str = koopa_str + "%yyxwhile_entry_" + std::to_string(now_while) + ":\n";
    std::cout << "%yyxwhile_entry_" << now_while << ":\n";
    //(i<10)
    exp->generateKoopaIR();

    koopa_str = koopa_str + "  br %" + std::to_string(num_calculate - 1) + ", %yyxwhile_body_" + std::to_string(now_while) + ", %yyxwhile_end_" + std::to_string(now_while) + "\n";
    std::cout << "  br %" << num_calculate - 1 << ", %yyxwhile_body_" << now_while << ", %yyxwhile_end_" << now_while << std::endl;

    // whilebody
    koopa_str = koopa_str + "%yyxwhile_body_" + std::to_string(now_while) + ":\n";
    std::cout << "%yyxwhile_body_" << now_while << ":\n";
    stmt->generateKoopaIR();

    if(!ifHasReturn[num_block]){
        koopa_str = koopa_str + "  jump %yyxwhile_entry_" + std::to_string(now_while) + "\n";
        std::cout << "  jump %yyxwhile_entry_" << now_while << std::endl;
    }
    ifHasReturn[num_block] = false;

    // while end
    koopa_str = koopa_str + "%yyxwhile_end_" + std::to_string(now_while) + ":\n";
    std::cout << "%yyxwhile_end_" << now_while << ":\n";

    whileMap.erase(while_block);
    while_block--;
}
// break
void StmtType_7_AST::generateKoopaIR() const{
    koopa_str = koopa_str + "  jump %yyxwhile_end_" + std::to_string(whileMap[while_block]) + "\n";
    std::cout << "  jump %yyxwhile_end_" << whileMap[while_block] << std::endl;
    ifHasReturn[num_block] = true;
}
// continue
void StmtType_8_AST::generateKoopaIR() const{
    koopa_str = koopa_str + "  jump %yyxwhile_entry_" + std::to_string(whileMap[while_block]) + "\n";
    std::cout << "  jump %yyxwhile_entry_" << whileMap[while_block] << std::endl;
    ifHasReturn[num_block] = true;
}

void VarDeclAST::generateKoopaIR() const{
    for (auto iter = varDefVec.begin(); iter != varDefVec.end(); iter++){
        (*iter)->generateKoopaIR();
    }
}

void VarDefType_1_AST::generateKoopaIR() const{
    if(constExpVec.empty()){
        if(in_global){
            koopa_str = koopa_str + "global @" + ident + "_" + std::to_string(num_block) + " = alloc i32, zeroinit\n\n";
            std::cout << "global @" << ident << "_" << num_block << " = alloc i32, zeroinit\n\n";
        }
        else{
            koopa_str = koopa_str + "  @" + ident + "_" + std::to_string(num_block) + " = alloc i32\n";
            std::cout << "  @" << ident << "_" << num_block << " = alloc i32" << std::endl;
        }
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_VARIABLE, 0)));
    }
    else{
        if(in_global){
            // 全局变量数组
            koopa_str = koopa_str + "global @" + ident + "_" + std::to_string(num_block) + " = alloc ";
            std::cout << "global @" << ident << "_" << num_block << " = alloc ";
            for (int i = 0; i < constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32, ";
            std::cout << "i32, ";

            // 计算各个维度及各长度
            std::vector<int> vec_len;
            std::vector<int> multiple_len;
            for (int i = constExpVec.size() - 1; i >= 0; i--){
                int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
                vec_len.push_back(expValue);
                if(multiple_len.empty()){
                    multiple_len.push_back(expValue);
                }
                else{
                    multiple_len.push_back(multiple_len.back() * expValue);
                }
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
                if(i != 0){
                    koopa_str = koopa_str + ", ";
                    std::cout << ", ";
                }
            }
            std::reverse(multiple_len.begin(), multiple_len.end());
            std::reverse(vec_len.begin(), vec_len.end());

            koopa_str = koopa_str + ", zeroinit\n";
            std::cout << ", zeroinit\n";
        }
        else{
            // 局部变量数组
            koopa_str = koopa_str + "  @" + ident + "_" + std::to_string(num_block) + " = alloc ";
            std::cout << "  @" << ident << "_" << num_block << " = alloc ";
            for (int i = 0; i < constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32, ";
            std::cout << "i32, ";

            // 计算各个维度及各长度
            std::vector<int> vec_len;
            std::vector<int> multiple_len;
            for (int i = constExpVec.size() - 1; i >= 0; i--){
                // std::cout << std::endl;
                // std::cout << i << std::endl;
                int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
                vec_len.push_back(expValue);
                if(multiple_len.empty()){
                    multiple_len.push_back(expValue);
                }
                else{
                    multiple_len.push_back(multiple_len.back() * expValue);
                }
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
                if(i != 0){
                    koopa_str = koopa_str + ", ";
                    std::cout << ", ";
                }
                // std::cout << "aaa\n";
            }
            // std::cout << "out\n";
            koopa_str = koopa_str + "\n";
            std::cout << "\n";
            // std::cout << "before\n";
            std::reverse(multiple_len.begin(), multiple_len.end());
            std::reverse(vec_len.begin(), vec_len.end());
            // std::cout << "after\n";
        }
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_ARRAY_VAR, constExpVec.size())));

    }
}

void VarDefType_2_AST::generateKoopaIR() const{
    if(constExpVec.empty()){
        // @x = alloc i32
        if(in_global){
            int result = initVal->calculateValue();
            koopa_str = koopa_str + "global @" + ident + "_" + std::to_string(num_block) + " = alloc i32, " + std::to_string(result) + "\n\n";
            std::cout << "global @" << ident << "_" << num_block << " = alloc i32, " << result << "\n\n";
            (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_VARIABLE, 0)));
        }
        else{
            initVal->generateKoopaIR();
            koopa_str = koopa_str + "  @" + ident + "_" + std::to_string(num_block) + " = alloc i32\n";
            std::cout << "  @" << ident << "_" << num_block << " = alloc i32" << std::endl;
            (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_VARIABLE, 0)));
            // store 10, @x
            koopa_str = koopa_str + "  store %" + std::to_string(num_calculate - 1) + ", @" + ident + "_" + std::to_string(search(ident).second) + "\n";
            std::cout << "  store %" << num_calculate - 1 << ", @" << ident << "_" << search(ident).second << std::endl;
        }
    }
    else{
        if(in_global){
            // 全局变量数组
            koopa_str = koopa_str + "global @" + ident + "_" + std::to_string(num_block) + " = alloc ";
            std::cout << "global @" << ident << "_" << num_block << " = alloc ";
            for (int i = 0; i < constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32, ";
            std::cout << "i32, ";

            // 计算各个维度及各长度
            std::vector<int> vec_len;
            std::vector<int> multiple_len;
            for (int i = constExpVec.size() - 1; i >= 0; i--){
                int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
                vec_len.push_back(expValue);
                if(multiple_len.empty()){
                    multiple_len.push_back(expValue);
                }
                else{
                    multiple_len.push_back(multiple_len.back() * expValue);
                }
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
                if(i != 0){
                    koopa_str = koopa_str + ", ";
                    std::cout << ", ";
                }
            }
            koopa_str = koopa_str + ", ";
            std::cout << ", ";
            std::reverse(multiple_len.begin(), multiple_len.end());
            std::reverse(vec_len.begin(), vec_len.end());

            
            std::vector<int> param = dynamic_cast<InitValType_2_AST *>(initVal.get())->Aggregate(multiple_len);
            printAgg(param, 0, vec_len, multiple_len, 0);
            koopa_str = koopa_str + "\n";
            std::cout << "\n";
        }
        else{
            // 局部变量数组
            koopa_str = koopa_str + "  @" + ident+"_"+std::to_string(num_block) + " = alloc ";
            std::cout << "  @" << ident << "_" << num_block << " = alloc ";
            for (int i = 0; i < constExpVec.size(); i++){
                koopa_str = koopa_str + "[";
                std::cout << "[";
            }
            koopa_str = koopa_str + "i32, ";
            std::cout << "i32, ";

            // 计算各个维度及各长度
            std::vector<int> vec_len;
            std::vector<int> multiple_len;
            for (int i = constExpVec.size() - 1; i >= 0; i--){
                int expValue = dynamic_cast<ConstExpAST *>(constExpVec[i].get())->calculateValue();
                vec_len.push_back(expValue);
                if(multiple_len.empty()){
                    multiple_len.push_back(expValue);
                }
                else{
                    multiple_len.push_back(multiple_len.back() * expValue);
                }
                koopa_str = koopa_str + std::to_string(expValue) + "]";
                std::cout << expValue << "]";
                if(i != 0){
                    koopa_str = koopa_str + ", ";
                    std::cout << ", ";
                }
            }
            koopa_str = koopa_str + "\n";
            std::cout << "\n";
            std::reverse(multiple_len.begin(), multiple_len.end());
            std::reverse(vec_len.begin(), vec_len.end());

            
            std::vector<int> param = dynamic_cast<InitValType_2_AST *>(initVal.get())->Aggregate(multiple_len);
            printStoreVar(param, 0, vec_len, multiple_len, 0, ident, num_block);
        }
        (*table[num_block]).insert(std::make_pair(ident, SymValue(SYM_ARRAY_VAR, constExpVec.size())));
    }

}

void InitValType_1_AST::generateKoopaIR() const{
    exp->generateKoopaIR();
}

void InitValType_2_AST::generateKoopaIR() const{

}

std::vector<int> InitValType_2_AST::Aggregate(std::vector<int> vec){
    if(!in_global){
        //std::cout << "enter\n";
        std::vector<int> param;
        //int i = 0;
        for (auto iter = initValVec.begin(); iter != initValVec.end(); iter++){
            //std::cout << ++i << std::endl;
            if((*iter)->InitType() == 1){
                dynamic_cast<InitValType_1_AST *>((*iter).get())->generateKoopaIR();
                param.push_back(num_calculate - 1);
            }
            else if((*iter)->InitType() == 2){
                auto iter2 = vec.begin();
                iter2++;
                for (; iter2 != vec.end(); iter2++){
                    if(param.size() % (*iter2) == 0){
                        std::vector<int> sub_vec;
                        std::copy(iter2, vec.end(), std::back_inserter(sub_vec));
                        std::vector<int> tmpAgg = dynamic_cast<InitValType_2_AST *>((*iter).get())->Aggregate(sub_vec);
                        param.insert(param.end(), tmpAgg.begin(), tmpAgg.end());
                        break;
                    }
                }
            }
        }
        //std::cout << "insertlen = " << vec[0] - param.size() << std::endl;
        param.insert(param.end(), vec[0] - param.size(), -1);
        return param;
    }
    else if(in_global){
        std::vector<int> param;
        for (auto iter = initValVec.begin(); iter != initValVec.end(); iter++){
            if((*iter)->InitType() == 1){
                param.push_back(dynamic_cast<InitValType_1_AST *>((*iter).get())->calculateValue());
            }
            else if((*iter)->InitType() == 2){
                auto iter2 = vec.begin();
                iter2++;
                for (; iter2 != vec.end(); iter2++){
                    if(param.size() % (*iter2) == 0){
                        std::vector<int> sub_vec;
                        std::copy(iter2, vec.end(), std::back_inserter(sub_vec));
                        std::vector<int> tmpAgg = dynamic_cast<InitValType_2_AST *>((*iter).get())->Aggregate(sub_vec);
                        param.insert(param.end(), tmpAgg.begin(), tmpAgg.end());
                        break;
                    }
                }
            }
        }
        param.insert(param.end(), vec[0] - param.size(), 0);
        return param;
    }
}

void ExpAST::generateKoopaIR() const{
    lOrExp->generateKoopaIR();
}

void PrimaryExpType_1_AST::generateKoopaIR() const{
    exp->generateKoopaIR();
}

void PrimaryExpType_2_AST::generateKoopaIR() const{
    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = add 0, " + std::to_string(number) + "\n";
    std::cout << "  %" << num_calculate << " = add 0, " << number << std::endl;
    num_calculate++;
}

void PrimaryExpType_3_AST::generateKoopaIR() const{
    lVal->generateKoopaIR();
}

void UnaryExpType_1_AST::generateKoopaIR() const{
    primaryExp->generateKoopaIR();
}

void UnaryExpType_2_AST::generateKoopaIR() const{
    unaryExp->generateKoopaIR();
    if(unaryOp == '+'){
        return;
    }
    else if(unaryOp == '-'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = sub 0, %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = sub 0, %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(unaryOp == '!'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = eq 0, %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = eq 0, %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
}

void UnaryExpType_3_AST::generateKoopaIR() const{
    std::vector<int> *params = new std::vector<int>();
    for (auto iter = expVec.begin(); iter != expVec.end(); iter++){
        (*iter)->generateKoopaIR();
        params->push_back(num_calculate - 1);
    }
    if(search(ident).first.type == SYM_FUNC_INT){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = ";
        std::cout << "  %" << num_calculate << " = ";
        num_calculate++;
    }
    else if(search(ident).first.type == SYM_FUNC_VOID){
        koopa_str = koopa_str + "  ";
        std::cout << "  ";
    }
    koopa_str = koopa_str + "call @" + ident + "(";
    std::cout << "call @" << ident << "(";

    for (auto iter = params->begin(); iter != params->end(); iter++){
        if(iter != params->begin()){
            koopa_str = koopa_str + ", ";
            std::cout << ", ";
        }
        koopa_str = koopa_str + "%" + std::to_string((*iter));
        std::cout << "%" << (*iter);
    }

    koopa_str = koopa_str + ")\n";
    std::cout << ")\n";
}

void MulExpType_1_AST::generateKoopaIR() const{
    unaryExp->generateKoopaIR();
}

void MulExpType_2_AST::generateKoopaIR() const{
    mulExp->generateKoopaIR();
    int num1 = num_calculate - 1;
    unaryExp->generateKoopaIR();
    if(mulOp == '*'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = mul %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = mul %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(mulOp == '/'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = div %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = div %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(mulOp == '%'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = mod %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = mod %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
}

void AddExpType_1_AST::generateKoopaIR() const{
    mulExp->generateKoopaIR();
}

void AddExpType_2_AST::generateKoopaIR() const{
    addExp->generateKoopaIR();
    int num1 = num_calculate - 1;
    mulExp->generateKoopaIR();
    if(addOp == '+'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = add %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = add %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(addOp == '-'){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = sub %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = sub %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
}

void RelExpType_1_AST::generateKoopaIR() const{
    addExp->generateKoopaIR();
}

void RelExpType_2_AST::generateKoopaIR() const{
    relExp->generateKoopaIR();
    int num1 = num_calculate - 1;
    addExp->generateKoopaIR();
    if(relOp == "<"){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = lt %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = lt %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(relOp == ">"){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = gt %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = gt %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(relOp == ">="){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = ge %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = ge %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(relOp == "<="){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = le %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = le %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
}

void EqExpType_1_AST::generateKoopaIR() const{
    relExp->generateKoopaIR();
}

void EqExpType_2_AST::generateKoopaIR() const{
    eqExp->generateKoopaIR();
    int num1 = num_calculate - 1;
    relExp->generateKoopaIR();
    if(eqOp == "=="){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = eq %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = eq %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
    else if(eqOp == "!="){
        koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = ne %" + std::to_string(num1) + ", %" + std::to_string(num_calculate - 1) + "\n";
        std::cout << "  %" << num_calculate << " = ne %" << num1 << ", %" << num_calculate - 1 << std::endl;
        num_calculate++;
    }
}

void LAndExpType_1_AST::generateKoopaIR() const{
    eqExp->generateKoopaIR();
}

void LAndExpType_2_AST::generateKoopaIR() const{
    // int result = 0;
    if_num++;
    int nowif = if_num;
    koopa_str = koopa_str + "  @yyx_result_" + std::to_string(nowif) + " = alloc i32\n";
    std::cout << "  @yyx_result_" << nowif << " = alloc i32\n";
    koopa_str = koopa_str + "  store 0, @yyx_result_" + std::to_string(nowif) + "\n";
    std::cout << "  store 0, @yyx_result_" << nowif << std::endl;

    lAndExp->generateKoopaIR();
    // 计算第一个表达式得出来的值是否为真（为真存1，为假存0）
    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = ne 0, %" + std::to_string(num_calculate - 1) + "\n";
    std::cout << "  %" << num_calculate << " = ne 0, %" << num_calculate - 1 << std::endl;

    // if(lhs == 1)...
    koopa_str = koopa_str + "  br %" + std::to_string(num_calculate) + ", %yyx_then_" + std::to_string(nowif) + ", %yyx_end_" + std::to_string(nowif) + "\n";
    std::cout << "  br %" << num_calculate << ", %yyx_then_" << nowif << ", %yyx_end_" << nowif << std::endl;

    //int num1 = num_calculate;
    num_calculate++;

    // %then:
    koopa_str = koopa_str + "%yyx_then_" + std::to_string(nowif) + ":\n";
    std::cout << "%yyx_then_" << nowif << ":\n";

    eqExp->generateKoopaIR();
    // 计算第二个表达式得出来的值是否为真（为真存1，为假存0）
    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = ne 0, %" + std::to_string(num_calculate - 1) + "\n";
    std::cout << "  %" << num_calculate << " = ne 0, %" << num_calculate - 1 << std::endl;

    koopa_str = koopa_str + "  store %" + std::to_string(num_calculate) + ", @yyx_result_" + std::to_string(nowif) + "\n";
    std::cout << "  store %" << num_calculate << ", @yyx_result_" << nowif << std::endl;
    koopa_str = koopa_str + "  jump %yyx_end_" + std::to_string(nowif) + "\n";
    std::cout << "  jump %yyx_end_" << nowif << std::endl;

    num_calculate++;

    koopa_str = koopa_str + "%yyx_end_" + std::to_string(nowif) + ":\n";
    std::cout << "%yyx_end_" << nowif << ":\n";

    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load @yyx_result_" + std::to_string(nowif) + "\n";
    std::cout << "  %" << num_calculate << " = load @yyx_result_" << nowif << std::endl;
    num_calculate++;
}

void LOrExpType_1_AST::generateKoopaIR() const{
    lAndExp->generateKoopaIR();
}

void LOrExpType_2_AST::generateKoopaIR() const{
    // int result = 1;
    if_num++;
    int nowif = if_num;
    koopa_str = koopa_str + "  @yyx_result_" + std::to_string(nowif) + " = alloc i32\n";
    std::cout << "  @yyx_result_" << nowif << " = alloc i32\n";
    koopa_str = koopa_str + "  store 1, @yyx_result_" + std::to_string(nowif) + "\n";
    std::cout << "  store 1, @yyx_result_" << nowif << std::endl;

    lOrExp->generateKoopaIR();
    // 计算第一个表达式得出来的值是否为真（为真存1，为假存0）
    // 为0存1，为1存0
    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = eq 0, %" + std::to_string(num_calculate - 1) + "\n";
    std::cout << "  %" << num_calculate << " = eq 0, %" << num_calculate - 1 << std::endl;

    //if (lhs == 0)...
    koopa_str = koopa_str + "  br %" + std::to_string(num_calculate) + ", %yyx_then_" + std::to_string(nowif) + ", %yyx_end_" + std::to_string(nowif) + "\n";
    std::cout << "  br %" << num_calculate << ", %yyx_then_" << nowif << ", %yyx_end_" << nowif << std::endl;

    //int num1 = num_calculate;
    num_calculate++;

    // %then:
    koopa_str = koopa_str + "%yyx_then_" + std::to_string(nowif) + ":\n";
    std::cout << "%yyx_then_" << nowif << ":\n";

    lAndExp->generateKoopaIR();
    // 计算第二个表达式得出来的值是否为真（为真存1，为假存0）
    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = ne 0, %" + std::to_string(num_calculate - 1) + "\n";
    std::cout << "  %" << num_calculate << " = ne 0, %" << num_calculate - 1 << std::endl;

    // result = rhs != 0;
    koopa_str = koopa_str + "  store %" + std::to_string(num_calculate) + ", @yyx_result_" + std::to_string(nowif) + "\n";
    std::cout << "  store %" << num_calculate << ", @yyx_result_" << nowif << std::endl;
    koopa_str = koopa_str + "  jump %yyx_end_" + std::to_string(nowif) + "\n";
    std::cout << "  jump %yyx_end_" << nowif << std::endl;

    num_calculate++;

    koopa_str = koopa_str + "%yyx_end_" + std::to_string(nowif) + ":\n";
    std::cout << "%yyx_end_" << nowif << ":\n";

    koopa_str = koopa_str + "  %" + std::to_string(num_calculate) + " = load @yyx_result_" + std::to_string(nowif) + "\n";
    std::cout << "  %" << num_calculate << " = load @yyx_result_" << nowif << std::endl;
    num_calculate++;
}


// calculateValue
int ExpAST::calculateValue(){
    return lOrExp->calculateValue();
}
int LOrExpType_1_AST::calculateValue(){
    return lAndExp->calculateValue();
}
int LOrExpType_2_AST::calculateValue(){
    return (lOrExp->calculateValue() || lAndExp->calculateValue());
}
int LAndExpType_1_AST::calculateValue(){
    return eqExp->calculateValue();
}
int LAndExpType_2_AST::calculateValue(){
    return (lAndExp->calculateValue() && eqExp->calculateValue());
}
int EqExpType_1_AST::calculateValue(){
    return relExp->calculateValue();
}
int EqExpType_2_AST::calculateValue(){
    if(eqOp == "=="){
        return (eqExp->calculateValue() == relExp->calculateValue());
    }
    else if(eqOp == "!="){
        return (eqExp->calculateValue() != relExp->calculateValue());
    }
    assert(0);
}
int RelExpType_1_AST::calculateValue(){
    return addExp->calculateValue();
}
int RelExpType_2_AST::calculateValue(){
    if(relOp == "<"){
        return (relExp->calculateValue() < addExp->calculateValue());
    }
    else if(relOp == ">"){
        return (relExp->calculateValue() > addExp->calculateValue());
    }
    else if(relOp == "<="){
        return (relExp->calculateValue() <= addExp->calculateValue());
    }
    else if(relOp == ">="){
        return (relExp->calculateValue() >= addExp->calculateValue());
    }
    assert(0);
}
int AddExpType_1_AST::calculateValue(){
    return mulExp->calculateValue();
}
int AddExpType_2_AST::calculateValue(){
    if(addOp == '+'){
        return (addExp->calculateValue() + mulExp->calculateValue());
    }
    else if(addOp == '-'){
        return (addExp->calculateValue() - mulExp->calculateValue());
    }
    assert(0);
}
int MulExpType_1_AST::calculateValue(){
    return unaryExp->calculateValue();
}
int MulExpType_2_AST::calculateValue(){
    if(mulOp == '*'){
        return (mulExp->calculateValue() * unaryExp->calculateValue());
    }
    else if(mulOp == '/'){
        return (mulExp->calculateValue() / unaryExp->calculateValue());
    }
    else if(mulOp == '%'){
        return (mulExp->calculateValue() % unaryExp->calculateValue());
    }
    assert(0);
}
int UnaryExpType_1_AST::calculateValue(){
    return primaryExp->calculateValue();
}
int UnaryExpType_2_AST::calculateValue(){
    if(unaryOp == '+'){
        return unaryExp->calculateValue();
    }
    else if(unaryOp == '-'){
        return -unaryExp->calculateValue();
    }
    else if(unaryOp == '!'){
        return !unaryExp->calculateValue();
    }
    assert(0);
}
int PrimaryExpType_1_AST::calculateValue(){
    return exp->calculateValue();
}
int PrimaryExpType_2_AST::calculateValue(){
    return number;
}
int PrimaryExpType_3_AST::calculateValue(){
    return lVal->calculateValue();
}
int ConstExpAST::calculateValue(){
    return exp->calculateValue();
}
int ConstInitValType_1_AST::calculateValue(){
    return constExp->calculateValue();
}
int LValAST::calculateValue(){
    return search(ident).first.value;
}
int InitValType_1_AST::calculateValue(){
    return exp->calculateValue();
}