#include "ast.hpp"
#include "helper.hpp"
using namespace std;

// constructor
CompUnitAST::CompUnitAST(vector<unique_ptr<BaseAST> > *p) 
    : defs(p) {}
FuncDefAST::FuncDefAST(BaseAST *p1, const char *p2, vector<unique_ptr<BaseAST> > *p3, BaseAST *p4)
    : func_type(p1), ident(p2), fparams(p3), block(p4) {}
FuncFParamAST1::FuncFParamAST1(BaseAST *p1, const char *p2)
    : bType(p1), ident(p2) {}
FuncFParamAST2::FuncFParamAST2(BaseAST *p1, const char *p2, vector<unique_ptr<BaseAST> > *p3)
    : bType(p1), ident(p2), exp_length(p3) {}
TypeAST::TypeAST(const char *p)
    : type(p) {}
BlockAST::BlockAST(vector<unique_ptr<BaseAST> > *p)
    : blockItem(p) {}
StmtAST1::StmtAST1(BaseAST *p)
    : exp(p) {}
StmtAST2::StmtAST2(BaseAST *p1, BaseAST *p2)
    : lVal(p1), exp(p2) {}
StmtAST3::StmtAST3(BaseAST *p)
    : exp(p) {}
StmtAST4::StmtAST4(BaseAST *p)
    : block(p) {}
StmtAST5::StmtAST5() {}
StmtAST6::StmtAST6() {}
IfAST1::IfAST1(BaseAST *p1, BaseAST *p2)
    : exp(p1), stmtThen(p2) {}
IfAST2::IfAST2(BaseAST *p1, BaseAST *p2, BaseAST *p3)
    : exp(p1), stmtThen(p2), stmtElse(p3) {}
WhileAST::WhileAST(BaseAST *p1, BaseAST *p2)
    : exp(p1), stmt(p2) {}
PrimaryExpAST::PrimaryExpAST(int num)
    : num(num) {}
UnaryExpAST1::UnaryExpAST1(const char *p1, BaseAST *p2)
    : unaryOp(p1), unaryExp(p2) {}
UnaryExpAST2::UnaryExpAST2(const char *p1, vector<unique_ptr<BaseAST> > *p2)
    : ident(p1), rparams(p2) {}
MulExpAST::MulExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : mulExp(p1), mulOp(p2), unaryExp(p3) {}
AddExpAST::AddExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : addExp(p1), addOp(p2), mulExp(p3) {}
RelExpAST::RelExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : relExp(p1), relOp(p2), addExp(p3) {}
EqExpAST::EqExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : eqExp(p1), eqOp(p2), relExp(p3) {}
LAndExpAST::LAndExpAST(BaseAST *p1, BaseAST *p2)
    : lAndExp(p1), eqExp(p2) {}
LOrExpAST::LOrExpAST(BaseAST *p1, BaseAST *p2)
    : lOrExp(p1), lAndExp(p2) {} 
ConstDeclAST::ConstDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2)
    : bType(p1), constDef(p2) {}
ConstDefAST::ConstDefAST(const char *p1, BaseAST *p2)
    : ident(p1), constInitVal(p2) {}
ConstArrayDefAST::ConstArrayDefAST(const char *p1, vector<unique_ptr<BaseAST> > *p2, BaseAST *p3)
    : ident(p1), exp_length(p2), initVal(p3) {}
VarDeclAST::VarDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2)
    : bType(p1), varDef(p2) {}
VarDefAST1::VarDefAST1(const char *p)
    : ident(p) {}
VarArrayDefAST1::VarArrayDefAST1(const char *p1, vector<unique_ptr<BaseAST> > *p2)
    : ident(p1), exp_length(p2) {}
VarArrayDefAST2::VarArrayDefAST2(const char *p1, vector<unique_ptr<BaseAST> > *p2, BaseAST *p3)
    : ident(p1), exp_length(p2), initVal(p3) {}
VarDefAST2::VarDefAST2(const char *p1, BaseAST *p2)
    : ident(p1), initVal(p2) {}
LValAST1::LValAST1(const char *p)
    : ident(p) {}
LValAST2::LValAST2(const char *p1, vector<unique_ptr<BaseAST> > *p2)
    : ident(p1), indexes(p2) {}
ConstInitValAST::ConstInitValAST(vector<unique_ptr<BaseAST> > *p)
    : values(p) {}
InitValAST::InitValAST(vector<unique_ptr<BaseAST> > *p)
    : values(p) {}
// end constructor

// calculateExp
int PrimaryExpAST::calculateExp() const {
    return num;
}
int UnaryExpAST1::calculateExp() const {
    switch(unaryOp[0]) {
    case '+':
        return unaryExp->calculateExp();
    case '-':
        return -unaryExp->calculateExp();
    case '!':
        return !unaryExp->calculateExp();
    }
    return 0;
}
int MulExpAST::calculateExp() const {
    switch(mulOp[0]) {
    case '*':
        return mulExp->calculateExp() * unaryExp->calculateExp();
    case '/':
        return mulExp->calculateExp() / unaryExp->calculateExp();
    case '%':
        return mulExp->calculateExp() % unaryExp->calculateExp();
    }
    return 0;
}
int AddExpAST::calculateExp() const {
    if(addOp[0] == '+')
        return addExp->calculateExp() + mulExp->calculateExp();
    else if(addOp[0] == '-')
        return addExp->calculateExp() - mulExp->calculateExp();
    return 0;
}
int RelExpAST::calculateExp() const {
    if(relOp == "<") 
        return relExp->calculateExp() < addExp->calculateExp();
    else if(relOp == "<=")
        return relExp->calculateExp() <= addExp->calculateExp();
    else if(relOp == ">")  
        return relExp->calculateExp() > addExp->calculateExp();
    else if(relOp == ">=")
        return relExp->calculateExp() >= addExp->calculateExp();
    return 0;
}
int EqExpAST::calculateExp() const {
    if(eqOp == "==")
        return eqExp->calculateExp() == relExp->calculateExp();
    else if(eqOp == "!=")
        return eqExp->calculateExp() != relExp->calculateExp();
    return 0;
}
int LAndExpAST::calculateExp() const {
    return lAndExp->calculateExp() && eqExp->calculateExp();
}
int LOrExpAST::calculateExp() const {
    return lOrExp->calculateExp() || lAndExp->calculateExp();
}
int LValAST1::calculateExp() const {
    auto i = SymbolTable::getItem(ident);
    if(i.type == SYMBOLTABLE_ITEM_CONST) 
        return i.data.c;
    else {
        //常量求值里不能有查询到变量
        cerr << "Required no variables in const declaration" << endl;
        exit(1);
    }
}
// end calculateExp

// other
void BaseAST::endBlock() {
    auto ptr = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr->insts, bufferInsts);
    bufferInsts.clear();
}
bool BaseAST::checkBlock(koopa_raw_basic_block_data_t *dest) {
    auto last = bufferInsts.empty() ? 255 : ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
    if(last != KOOPA_RVT_RETURN && last != KOOPA_RVT_BRANCH && last != KOOPA_RVT_JUMP) {
        auto rawjmp = createValueData(KOOPA_RVT_JUMP, nullptr, createTypeKind(KOOPA_RTT_UNIT), KOOPA_RSIK_VALUE);
        rawjmp->kind.data.jump.target = dest;
        rawjmp->kind.data.jump.args = createSlice(KOOPA_RSIK_VALUE);
        bufferInsts.push_back(rawjmp);
        return true;
    }
    return false;
}
bool BaseAST::checkBlock(int value) {
    auto last = bufferInsts.empty() ? 255 : ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
    if(last != KOOPA_RVT_RETURN && last != KOOPA_RVT_BRANCH && last != KOOPA_RVT_JUMP) {
        auto rawret = createValueData(KOOPA_RVT_RETURN, nullptr, createTypeKind(KOOPA_RTT_UNIT), KOOPA_RSIK_VALUE);
        rawret->kind.data.ret.value = createIntegerValueData(value);
        bufferInsts.push_back(rawret);
        return true;
    }
    return false;
}
bool BaseAST::checkBlock() {
    auto last = bufferInsts.empty() ? 255 : ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
    if(last != KOOPA_RVT_RETURN && last != KOOPA_RVT_BRANCH && last != KOOPA_RVT_JUMP) {
        auto rawret = createValueData(KOOPA_RVT_RETURN, nullptr, createTypeKind(KOOPA_RTT_UNIT), KOOPA_RSIK_VALUE);
        rawret->kind.data.ret.value = nullptr;
        bufferInsts.push_back(rawret);
        return true;
    }
    return false;
}
void BaseAST::initLibFuncs() {
    auto ty_pint = createTypeKind(KOOPA_RTT_POINTER);
    auto ty_int = createTypeKind(KOOPA_RTT_INT32);
    auto ty_unit = createTypeKind(KOOPA_RTT_UNIT);
    ty_pint->data.pointer.base = ty_int;

    auto ty_getint = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_getint->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_getint->data.function.ret = ty_int;
    auto raw_getint = createFuncData("@getint", ty_getint, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    bufferFuncs.push_back(raw_getint);
    SymbolTable::addItem("getint", raw_getint);

    auto ty_getch = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_getch->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_getch->data.function.ret = ty_int;
    auto raw_getch = createFuncData("@getch", ty_getch, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    bufferFuncs.push_back(raw_getch);
    SymbolTable::addItem("getch", raw_getch);

    auto ty_getarray = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_getarray->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_getarray->data.function.ret = ty_int;
    auto raw_getarray = createFuncData("@getarray", ty_getarray, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    addItemToSlice(ty_getarray->data.function.params, ty_pint);
    bufferFuncs.push_back(raw_getarray);
    SymbolTable::addItem("getarray", raw_getarray);

    auto ty_putint = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_putint->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_putint->data.function.ret = ty_unit;
    auto raw_putint = createFuncData("@putint", ty_putint, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    addItemToSlice(ty_putint->data.function.params, ty_int);
    bufferFuncs.push_back(raw_putint);
    SymbolTable::addItem("putint", raw_putint);

    auto ty_putch = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_putch->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_putch->data.function.ret = ty_unit;
    auto raw_putch = createFuncData("@putch", ty_putch, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    addItemToSlice(ty_putch->data.function.params, ty_int);
    bufferFuncs.push_back(raw_putch);
    SymbolTable::addItem("putch", raw_putch);

    auto ty_putarray = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_putarray->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_putarray->data.function.ret = ty_unit;
    auto raw_putarray = createFuncData("@putarray", ty_putarray, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    addItemToSlice(ty_putarray->data.function.params, ty_int);
    addItemToSlice(ty_putarray->data.function.params, ty_pint);
    bufferFuncs.push_back(raw_putarray);
    SymbolTable::addItem("putarray", raw_putarray);

    auto ty_starttime = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_starttime->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_starttime->data.function.ret = ty_unit;
    auto raw_starttime = createFuncData("@starttime", ty_starttime, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    bufferFuncs.push_back(raw_starttime);
    SymbolTable::addItem("starttime", raw_starttime);

    auto ty_stoptime = createTypeKind(KOOPA_RTT_FUNCTION);
    ty_stoptime->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty_stoptime->data.function.ret = ty_unit;
    auto raw_stoptime = createFuncData("@stoptime", ty_stoptime, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    bufferFuncs.push_back(raw_stoptime);
    SymbolTable::addItem("stoptime", raw_stoptime);
}
void BaseAST::ArrayDecl::setType(void *p) {
    declType = (koopa_raw_type_kind_t *)p;
}
void BaseAST::ArrayDecl::addDim(int dim) {
    dims.insert(dims.begin(), dim);
}
void BaseAST::ArrayDecl::reset() {
    arr = nullptr;
    dims.clear();
    align = 0;
}
void *BaseAST::ArrayDecl::constArrayToKoopa(vector<unique_ptr<BaseAST> > *values) {
    deque<vector<void *> > elems(dims.size() - align + 1);
    auto ty = arr->ty->data.pointer.base;
    for(int i = 0; i < align; i++) {
        ty = ty->data.array.base;
    }
    if(values == nullptr) {
        auto zeroinit = createValueData(KOOPA_RVT_ZERO_INIT, nullptr, ty, KOOPA_RSIK_VALUE);
        return zeroinit;
    }
    int cur_align = 1;
    for(int i = 0; i < values->size(); i++) {
        if(dynamic_cast<ConstInitValAST*>((*values)[i].get()) || dynamic_cast<InitValAST*>((*values)[i].get())) {
            align += cur_align;
            auto ptr = (koopa_raw_value_data_t *)(*values)[i]->toKoopa();
            align -= cur_align;
            elems[cur_align].push_back(ptr);
        }
        else {
            auto ptr = createIntegerValueData((*values)[i]->calculateExp());
            elems.back().push_back(ptr);
            cur_align = elems.size() - 1;
        }
        for(int j = 0; j < elems.size() - 1; j++) {
            auto &v = elems[elems.size() - j - 1];
            int len = v.size();
            if(len == dims[dims.size() - j - 1]) {
                auto ty_base = ((koopa_raw_value_data_t *)v[0])->ty;
                koopa_raw_value_data_t *raw;
                auto ty_temp = createTypeKind(KOOPA_RTT_ARRAY);
                ty_temp->data.array.base = ty_base;
                ty_temp->data.array.len = dims[dims.size() - j - 1];
                raw = createValueData(KOOPA_RVT_AGGREGATE, nullptr, ty_temp, KOOPA_RSIK_VALUE);
                raw->kind.data.aggregate.elems = createSlice(KOOPA_RSIK_VALUE);
                addItemToSlice(raw->kind.data.aggregate.elems, v);
                v.clear();
                elems[elems.size() - j - 2].push_back(raw);
                cur_align--;
            }
        }
    }
        
    for(int i = cur_align; i > 0; i--) {
        auto &v = elems[i];
        auto ty_base = ((koopa_raw_value_data_t *)v[0])->ty;
        koopa_raw_value_data_t *raw;
        auto ty_temp = createTypeKind(KOOPA_RTT_ARRAY);
        ty_temp->data.array.base = ty_base;
        ty_temp->data.array.len = dims[align + i - 1];
        raw = createValueData(KOOPA_RVT_AGGREGATE, nullptr, ty_temp, KOOPA_RSIK_VALUE);
        raw->kind.data.aggregate.elems = createSlice(KOOPA_RSIK_VALUE);
        auto zeroinit = createValueData(KOOPA_RVT_ZERO_INIT, nullptr, ty_base, KOOPA_RSIK_VALUE);
        for(int j = v.size(); j < dims[align + i - 1]; j++) {
            v.push_back(zeroinit);
        }
        addItemToSlice(raw->kind.data.aggregate.elems, v);
        elems[i - 1].push_back(raw);
    }
    return elems[0][0];
    
}
void BaseAST::ArrayDecl::initValuesProcess(const unique_ptr<vector<unique_ptr<BaseAST> > > &values, vector<pair<int, BaseAST *> >& index, int &next) {
    if(values == nullptr) {
        int next_align = 1;
        for(int i = align; i < dims.size(); i++) {
            next_align *= dims[i];
        }
        next += next_align;
        return;
    }
    for(int i = 0; i < values->size(); i++) {
        auto &p = (*values)[i];
        auto p_ = dynamic_cast<InitValAST*>(p.get());
        if(p_) {
            int cur_align = dims.size() - align;
            if(i == 0) {
                cur_align = 1;
            }
            else {
                int next_ = next;
                for(int i = dims.size() - 1; i >= 0; i--) {
                    if(next_ % dims[i] == 0) {
                        cur_align--;
                        next_ /= dims[i];
                    }
                    else {
                        break;
                    }
                }
            }
            align += cur_align;
            initValuesProcess(p_->values, index, next);
            align -= cur_align;
        }
        else {
            index.push_back(make_pair(next++, p.get()));
        }
    }
    int next_align = 1;
    for(int i = align; i < dims.size(); i++) {
        next_align *= dims[i];
    }
    next = (next / next_align + (next % next_align == 0 ? 0 : 1)) * next_align;
}
void *BaseAST::getLVal(const BaseAST *p) {
    auto ptr1 = dynamic_cast<const LValAST1*>(p);
    if(ptr1) {
        auto i = SymbolTable::getItem(ptr1->ident);
        if(i.type == SYMBOLTABLE_ITEM_CONST) {
            cerr << "lvalue required as left operand of assignment" << endl;
            exit(1);
        }
        else if(i.type == SYMBOLTABLE_ITEM_VAR) {
            // 赋值语句里无方括号的LVal似乎不会有改变？
            return i.data.v;
        }
        exit(1);
    }
    auto ptr2 = dynamic_cast<const LValAST2*>(p);
    if(ptr2) {
        auto i = SymbolTable::getItem(ptr2->ident).data.v;
        auto ty = i->ty->data.pointer.base;
        koopa_raw_value_data_t *raw_get = i;
        int j = 0;
        while(j < ptr2->indexes->size()) {
                if(ty->tag == KOOPA_RTT_ARRAY) {
                auto old_raw_get = raw_get;
                ty = ty->data.array.base;
                auto ty_pointer = createTypeKind(KOOPA_RTT_POINTER);
                ty_pointer->data.pointer.base = ty;
                raw_get = createValueData(KOOPA_RVT_GET_ELEM_PTR, nullptr, ty_pointer, KOOPA_RSIK_VALUE);
                auto raw_index = (koopa_raw_value_data_t *)(*ptr2->indexes)[j++]->toKoopa();
                raw_get->kind.data.get_elem_ptr.index = raw_index;
                raw_get->kind.data.get_elem_ptr.src = old_raw_get;
                bufferInsts.push_back(raw_get);
            }
            else if(ty->tag == KOOPA_RTT_POINTER) {
                auto raw_load = createValueData(KOOPA_RVT_LOAD, nullptr, ty, KOOPA_RSIK_VALUE);
                raw_load->kind.data.load.src = raw_get;
                bufferInsts.push_back(raw_load);
                raw_get = raw_load;
                auto old_raw_get = raw_get;
                raw_get = createValueData(KOOPA_RVT_GET_PTR, nullptr, ty, KOOPA_RSIK_VALUE);
                ty = ty->data.pointer.base;
                auto raw_index = (koopa_raw_value_data_t *)(*ptr2->indexes)[j++]->toKoopa();
                raw_get->kind.data.get_ptr.index = raw_index;
                raw_get->kind.data.get_ptr.src = old_raw_get;
                bufferInsts.push_back(raw_get);
            }
            else {
                break;
            }
        }
        return raw_get;
    }
    exit(1);
}