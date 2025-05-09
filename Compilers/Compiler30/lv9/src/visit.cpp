#include "visit.h"
using namespace std;

static string reg_names[16] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                               "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0"};
static vector<int> reg_stats(16, 0);

static koopa_raw_value_t registers[16];
static koopa_raw_value_t current_value = 0;
static map<koopa_raw_value_t, Reg> value_register;

static map<koopa_raw_value_t, string> global_var;
static int global_var_num = 0;

static int stack_size = 0;
static int stack_top = 0;
static bool ra = false;

inline ostream &tab(ostream &os) {
    return os << "  ";
}

inline string getName(const char *name) {
    return &name[1];
}

void immSpInst(string dest, int offset, string op) {
    if (offset >= -2048 && offset <= 2047) {
        instOperation(op, dest, "sp", offset);
    } else {
        instOperation("li", "s2", offset);
        instOperation("add", "s2", "s2", "sp");
        instOperation(op, dest, "s2", 0);
    }
}

int getLength(const koopa_raw_type_t &base) {
    if (base->tag == KOOPA_RTT_ARRAY) {
        return base->data.array.len * getLength(base->data.array.base);
    }
    return 4;
}

int findReg(int stat) {
    for (int i = 0; i < 15; i++) {
        if (reg_stats[i] == 0) {
            registers[i] = current_value;
            reg_stats[i] = stat;
            return i;
        }
    }
    for (int i = 0; i < 15; i++) {
        if (reg_stats[i] == 1) {
            value_register[registers[i]].index = -1;
            int offset = value_register[registers[i]].offset;
            if (offset == -1) {
                offset = stack_top;
                stack_top += 4;
                value_register[registers[i]].offset = offset;
            }
            immSpInst(reg_names[i], offset, "sw");
            registers[i] = current_value;
            reg_stats[i] = stat;
            return i;
        }
    }
    return -1;
}

void clearReg(bool save_stack) {
    for (int i = 0; i < 15; i++) {
        if (reg_stats[i] > 0) {
            value_register[registers[i]].index = -1;
            int offset = value_register[registers[i]].offset;
            if (offset == -1) {
                offset = stack_top;
                stack_top += 4;
                value_register[registers[i]].offset = offset;
                if (save_stack) {
                    immSpInst(reg_names[i], offset, "sw");
                }
            }
            reg_stats[i] = 0;
        }
    }
}

void aggregateInit(const koopa_raw_value_t &value) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        cout << tab << ".word " << value->kind.data.integer.value << endl;
        return;
    }
    for (int i = 0; i < value->kind.data.aggregate.elems.len; i++) {
        aggregateInit(reinterpret_cast<koopa_raw_value_t>(value->kind.data.aggregate.elems.buffer[i]));
    }
}

void Visit(const koopa_raw_program_t &program) {
    Visit(program.values);
    Visit(program.funcs);
}

void Visit(const koopa_raw_slice_t &slice) {
    for (size_t i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
}

void Visit(const koopa_raw_function_t &func) {
    if (!func->bbs.len)
        return;
    cout << tab << ".text" << endl;
    cout << tab << ".globl " << getName(func->name) << endl;
    cout << getName(func->name) << ":" << endl;
    uint32_t max_args_len = 0;
    size_t bbs_len = func->bbs.len;
    for (size_t i = 0; i < bbs_len; i++) {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        size_t bb_len = bb->insts.len;
        for (size_t j = 0; j < bb_len; j++) {
            koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->ty->tag != KOOPA_RTT_UNIT) {
                if (inst->kind.tag == KOOPA_RVT_ALLOC)
                    stack_size += getLength(inst->ty->data.pointer.base);
                else stack_size += 4;
            }
            if (inst->kind.tag == KOOPA_RVT_CALL) {
                ra = true;
                uint32_t args_len = inst->kind.data.call.args.len;
                if (args_len > 8 && (args_len - 8) * 4 > max_args_len) {
                    max_args_len = (args_len - 8) * 4;
                }
            }
        }
    }
    stack_size += max_args_len;
    stack_top += max_args_len;
    if (ra)
        stack_size += 4;
    stack_size = (stack_size + 15) & ~15;
    if (stack_size > 0 && stack_size <= 2048) {
        instOperation("addi", "sp", "sp", -stack_size);
    }
    else if (stack_size > 2048) {
        instOperation("li", "s2", -stack_size);
        instOperation("add", "sp", "sp", "s2");
    }
    if (ra) {
        immSpInst("ra", stack_size - 4, "sw");
    }
    for (size_t i = 0; i < func->params.len; i++) {
        auto param = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);
        if (i < 8) {
            value_register[param] = Reg(i + 7, -1);
        }
        else {
            int offset = stack_size + (i - 8) * 4;
            value_register[param] = Reg(-1, offset);
        }
    }
    Visit(func->bbs);
    cout << endl;
    stack_size = 0;
    stack_top = 0;
    reg_stats.resize(16, 0);
    value_register.clear();
    ra = false;
}

void Visit(const koopa_raw_basic_block_t &bb) {
    cout << getName(bb->name) << ":" << endl;
    Visit(bb->insts);
}

Reg Visit(const koopa_raw_value_t &value) {
    auto old_value = current_value;
    current_value = value;
    if (value_register.count(value)) {
        if (value_register[value].index == -1) {
            int reg_name = findReg(1);
            value_register[value].index = reg_name;
            immSpInst(reg_names[reg_name], value_register[value].offset, "lw");
        }
        current_value = old_value;
        return value_register[value];
    }

    const auto &kind = value->kind;
    Reg dest_reg = Reg(-1, -1);
    switch (kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        dest_reg = Visit(kind.data.integer);
        break;
    case KOOPA_RVT_ALLOC:
        dest_reg.offset = stack_top;
        stack_top += getLength(value->ty->data.pointer.base);
        value_register[value] = dest_reg;
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        global_var[value] = Visit(kind.data.global_alloc);
        break;
    case KOOPA_RVT_LOAD:
        dest_reg = Visit(kind.data.load);
        value_register[value] = dest_reg;
        break;
    case KOOPA_RVT_STORE:
        Visit(kind.data.store);
        break;
    case KOOPA_RVT_GET_PTR:
        dest_reg = Visit(kind.data.get_ptr);
        value_register[value] = dest_reg;
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        dest_reg = Visit(kind.data.get_elem_ptr);
        value_register[value] = dest_reg;
        break;
    case KOOPA_RVT_BINARY:
        dest_reg = Visit(kind.data.binary);
        value_register[value] = dest_reg;
        break;
    case KOOPA_RVT_BRANCH:
        Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        Visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
        dest_reg = Visit(kind.data.call);
        value_register[value] = dest_reg;
        if (value->ty->tag != KOOPA_RTT_UNIT) {
            registers[dest_reg.index] = value;
            reg_stats[dest_reg.index] = 1;
        }
        break;
    case KOOPA_RVT_RETURN:
        Visit(kind.data.ret);
        break;
    default:
        assert(false);
    }
    current_value = old_value;
    return dest_reg;
}

void Visit(const koopa_raw_return_t &ret) {
    koopa_raw_value_t ret_value = ret.value;
    if (ret_value) {
        Reg result_var = Visit(ret_value);
        if (result_var.index != 7) {
            instOperation("mv", "a0", reg_names[result_var.index]);
        }
    }
    clearReg(false);
    if (ra) {
        immSpInst("ra", stack_size - 4, "lw");
    }
    if (stack_size > 0 && stack_size <= 2047) {
        instOperation("addi", "sp", "sp", stack_size);
    }
    else if (stack_size > 2047) {
        instOperation("li", "t0", stack_size);
        instOperation("add", "sp", "sp", "t0");
    }
    cout << tab << "ret" << endl;
}

Reg Visit(const koopa_raw_integer_t &integer) {
    Reg dest_reg = Reg(-1, -1);
    if (integer.value == 0) { 
        dest_reg.index = 15; 
        return dest_reg; 
    }
    dest_reg.index = findReg(0);
    instOperation("li", reg_names[dest_reg.index], integer.value);
    return dest_reg;
}

Reg Visit(const koopa_raw_binary_t &binary) {
    Reg left_reg = Visit(binary.lhs);
    int lidx = left_reg.index;
    int old_stat = reg_stats[lidx];
    reg_stats[lidx] = 2;

    Reg right_val = Visit(binary.rhs);
    int ridx = right_val.index;
    reg_stats[lidx] = old_stat;
    old_stat = reg_stats[ridx];

    reg_stats[ridx] = 2;
    Reg dest_reg = Reg(findReg(1), -1);
    reg_stats[ridx] = old_stat;

    string left_name = reg_names[lidx];
    string right_name = reg_names[ridx];
    string dest_name = reg_names[dest_reg.index];

    switch (binary.op)
    {
    case KOOPA_RBO_NOT_EQ:
        if (right_name == "x0") {
            instOperation("snez", dest_name, left_name);
            break;
        }
        if (left_name == "x0") {
            instOperation("snez", dest_name, right_name);
            break;
        }
        instOperation("xor", dest_name, left_name, right_name);
        instOperation("snez", dest_name, dest_name);
        break;
    case KOOPA_RBO_EQ:
        if (right_name == "x0") {
            instOperation("seqz", dest_name, left_name);
            break;
        }
        if (left_name == "x0") {
            instOperation("seqz", dest_name, right_name);
            break;
        }
        instOperation("xor", dest_name, left_name, right_name);
        instOperation("seqz", dest_name, dest_name);
        break;
    case KOOPA_RBO_GT:
        instOperation("sgt", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_LT:
        instOperation("slt", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_GE:
        instOperation("slt", dest_name, left_name, right_name);
        instOperation("seqz", dest_name, dest_name);
        break;
    case KOOPA_RBO_LE:
        instOperation("sgt", dest_name, left_name, right_name);
        instOperation("seqz", dest_name, dest_name);
        break;
    case KOOPA_RBO_ADD:
        instOperation("add", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_SUB:
        instOperation("sub", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_MUL:
        instOperation("mul", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_DIV:
        instOperation("div", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_MOD:
        instOperation("rem", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_AND:
        instOperation("and", dest_name, left_name, right_name);
        break;
    case KOOPA_RBO_OR:
        instOperation("or", dest_name, left_name, right_name);
        break;
    default:
        assert(false);
    }
    return dest_reg;
}

Reg Visit(const koopa_raw_load_t &load) {
    auto src = load.src;
    Reg dest_reg;
    if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        int reg_idx = findReg(1);
        dest_reg = Reg(reg_idx, -1);
        instOperation("la", reg_names[reg_idx], global_var[src]);
        instOperation("lw", reg_names[reg_idx], reg_names[reg_idx], 0);
        return dest_reg;
    } else if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src->kind.tag == KOOPA_RVT_GET_PTR) {
        dest_reg = Reg(findReg(2), -1);
        Reg src_reg = Visit(load.src);
        reg_stats[dest_reg.index] = 1;
        instOperation("lw", reg_names[dest_reg.index], reg_names[src_reg.index], 0);
        return dest_reg;
    }
    if (value_register[src].index >= 0)
        return value_register[src];
    int reg_idx = findReg(1);
    int reg_offset = value_register[src].offset;
    dest_reg = Reg(reg_idx, reg_offset);
    immSpInst(reg_names[reg_idx], reg_offset, "lw");
    return dest_reg;
}

void Visit(const koopa_raw_store_t &store) {
    Reg value = Visit(store.value);
    auto dest = store.dest;
    if (dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        instOperation("la", "s2", global_var[dest]);
        instOperation("sw", reg_names[value.index], "s2", 0);
        return;
    }
    else if (dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || dest->kind.tag == KOOPA_RVT_GET_PTR) {
        int old_stat = reg_stats[value.index];
        reg_stats[value.index] = 2;
        Reg dest_var = Visit(dest);
        reg_stats[value.index] = old_stat;
        instOperation("sw", reg_names[value.index], reg_names[dest_var.index], 0);
        return;
    }
    if (value_register[dest].offset == -1) {
        value_register[dest].offset = stack_top;
        stack_top += 4;
    }
    else {
        for (int i = 0; i < 16; i++)
            if (i == value.index)
                continue;
            else if (reg_stats[i] > 0 && value_register[registers[i]].offset == value_register[dest].offset) {
                reg_stats[i] = 0;
                value_register[registers[i]].index = value.index;
            }
    }
    int reg_idx = value.index;
    int reg_offset = value_register[dest].offset;
    immSpInst(reg_names[reg_idx], reg_offset, "sw");
}

void Visit(const koopa_raw_branch_t &branch) {
    int cond_idx = Visit(branch.cond).index;
    clearReg(false);
    std::cout << tab << "bnez " << reg_names[cond_idx] << ", " << getName(branch.true_bb->name) << std::endl;
    std::cout << tab << "j " << getName(branch.false_bb->name) << std::endl;
}

void Visit(const koopa_raw_jump_t &jump) {
    clearReg(false);
    std::cout << tab << "j " << getName(jump.target->name) << std::endl;
}

Reg Visit(const koopa_raw_call_t &call) {
    Reg dest_reg = Reg(7, -1);
    clearReg(true);
    vector<int> old_stats;
    for (size_t i = 0; i < call.args.len; i++) {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        Reg arg_var = Visit(arg);
        if (i < 8) {
            if (arg_var.index != i + 7)
                instOperation("mv", reg_names[i + 7], reg_names[arg_var.index]);
            old_stats.push_back(reg_stats[i + 7]);
            reg_stats[i + 7] = 2;
        }
        else 
            immSpInst(reg_names[arg_var.index], (i - 8) * 4, "sw");
    }
    for (int i = 0; i < old_stats.size(); i++)
        reg_stats[i + 7] = old_stats[i];
    cout << tab << "call " << getName(call.callee->name) << endl;
    clearReg(false);
    return dest_reg;
}

string Visit(const koopa_raw_global_alloc_t &global) {
    string name = "var_" + to_string(global_var_num++);
    //string name = getName(global.init->name);
    cout << tab << ".data" << endl;
    cout << tab << ".globl " << name << endl;
    cout << name << ":" << endl;
    switch (global.init->kind.tag) {
    case KOOPA_RVT_ZERO_INIT:
        cout << tab << ".zero " << getLength(global.init->ty) << endl;
        break;
    case KOOPA_RVT_INTEGER:
        cout << tab << ".word " << global.init->kind.data.integer.value << endl;
        break;
    case KOOPA_RVT_AGGREGATE:
        aggregateInit(global.init);
        break;
    default:
        assert(false);
    }
    cout << endl;
    return name;
}

Reg Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr) {
    Reg dest_reg;
    auto arr = get_elem_ptr.src->ty->data.pointer.base;
    int elem_size = getLength(arr) / arr->data.array.len;
    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        dest_reg = Reg(findReg(2), -1);
        Reg index_reg = Visit(get_elem_ptr.index);
        reg_stats[dest_reg.index] = 1;
        instOperation("la", reg_names[dest_reg.index], global_var[get_elem_ptr.src]);
        instOperation("li", "s2", elem_size);
        instOperation("mul", "s2", "s2", reg_names[index_reg.index]);
        instOperation("add", reg_names[dest_reg.index], reg_names[dest_reg.index], "s2");
        return dest_reg;
    }
    Reg src_reg = value_register[get_elem_ptr.src];
    dest_reg = Reg(findReg(2), -1);
    int src_idx, src_old_stat;
    if (get_elem_ptr.src->name && get_elem_ptr.src->name[0] == '@') {
        int offset = src_reg.offset;
        if (offset >= -2048 && offset <= 2047) {
            instOperation("addi", reg_names[dest_reg.index], "sp", offset);
        }
        else {
            instOperation("li", "s2", offset);
            instOperation("add", reg_names[dest_reg.index], "sp", "s2");
        }
    }
    else {
        src_reg = Visit(get_elem_ptr.src);
        src_idx = src_reg.index;
        src_old_stat = reg_stats[src_idx];
        reg_stats[src_idx] = 2;
    }
    Reg index_reg = Visit(get_elem_ptr.index);
    Reg temp_reg;
    if (elem_size != 0 && index_reg.index != 15) {
        int index_idx = index_reg.index;
        int ind_old_stat = reg_stats[index_idx];
        reg_stats[index_idx] = 2;
        temp_reg = Reg(findReg(0), -1);
        reg_stats[index_idx] = ind_old_stat;
        instOperation("li", reg_names[temp_reg.index], elem_size);
        instOperation("mul", reg_names[temp_reg.index], reg_names[temp_reg.index], reg_names[index_idx]);
    }
    else 
        temp_reg = Reg(15, -1);
    reg_stats[dest_reg.index] = 1;
    if (get_elem_ptr.src->name && get_elem_ptr.src->name[0] == '@') {
        instOperation("add", reg_names[dest_reg.index], reg_names[dest_reg.index], reg_names[temp_reg.index]);
    }
    else {
        instOperation("add", reg_names[dest_reg.index], reg_names[src_idx], reg_names[temp_reg.index]);
        reg_stats[src_idx] = src_old_stat;
    }
    return dest_reg;
}

Reg Visit(const koopa_raw_get_ptr_t &get_ptr) {
    Reg src_reg = value_register[get_ptr.src];
    Reg dest_reg = Reg(findReg(2), -1);
    Reg index_reg = Visit(get_ptr.index);
    Reg temp_reg;
    auto arr = get_ptr.src->ty->data.pointer.base;
    int arr_size = getLength(arr);
    if (arr_size != 0 && index_reg.index != 15) {
        int index = index_reg.index;
        int old_stat = reg_stats[index];
        reg_stats[index] = 2;
        temp_reg = Reg(findReg(0), -1);
        reg_stats[index] = old_stat;
        instOperation("li", reg_names[temp_reg.index], arr_size);
        instOperation("mul", reg_names[temp_reg.index], reg_names[temp_reg.index], reg_names[index]);
    } else {
        temp_reg = Reg(15, -1);
    }
    reg_stats[dest_reg.index] = 1;
    instOperation("add", reg_names[dest_reg.index], reg_names[src_reg.index], reg_names[temp_reg.index]);
    return dest_reg;
}

void instOperation(string inst, string reg, int32_t imm) {
    cout << tab << "li " + reg + ", " << imm << endl;
}

void instOperation(string inst, string reg1, string reg2, int32_t imm) {
    if (inst == "addi") {
        cout << tab << "addi " + reg1 + ", " + reg2 + ", " << imm << endl;
    } else if (inst == "sw" || inst == "lw") {
        cout << tab << inst + " " + reg1 + ", " << imm << "(" + reg2 + ")" << endl;
    }
}

void instOperation(string inst, string reg1, string reg2) {
    cout << tab << inst + " " + reg1 + ", " + reg2 << endl;
}

void instOperation(string inst, string reg, string reg1, string reg2) {
    cout << tab << inst + " " + reg + ", " + reg1 + ", " + reg2 << endl;
}