#include <cassert>
#include "visit_koopa.hpp"
#include <cstring>

using namespace std;

static bool isZero = false;
static unordered_map<void *, string> registers;
static unordered_map<void *, int> used_time;
static bool reg[6] = {false, false, false, false, false, false};
static int last_reg = -1;
static set<koopa_raw_value_t> spill;
static string current_func;
void visit(ofstream &out, const koopa_raw_program_t raw) {
    visit(out, raw.values);
    visit(out, raw.funcs);
}

void visit(ofstream &out, const koopa_raw_slice_t &slice) {
    for(int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch(slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            visit(out, reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            visit(out, reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
}

void visit(ofstream &out, const koopa_raw_slice_t &slice, StackFrame &stackFrame) {
    for(int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch(slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            visit(out, reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            visit(out, reinterpret_cast<koopa_raw_value_t>(ptr), stackFrame);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            visit(out, reinterpret_cast<koopa_raw_basic_block_t>(ptr), stackFrame);
            break;
        default:
            assert(false);
        }
    }
}

void visit(ofstream &out, const koopa_raw_value_t &value) {
    out << "    .data\n";
    out << "    .globl " << &(value->name[1]) << endl;
    out << &(value->name[1]) << ":\n";

    auto v = value->kind.data.global_alloc.init;
    stack<koopa_raw_value_t> travel_stack;
    travel_stack.push(v);
    while(!travel_stack.empty()) {
        auto p = travel_stack.top();
        travel_stack.pop();
        if(p->kind.tag == KOOPA_RVT_AGGREGATE) {
            for(int i = p->kind.data.aggregate.elems.len - 1; i >= 0; i--) {
                auto ptr = p->kind.data.aggregate.elems.buffer[i];
                travel_stack.push((koopa_raw_value_t)ptr);
            }
        }
        else if(p->kind.tag == KOOPA_RVT_ZERO_INIT) {
            out << "    .zero " << getArraySize(p->ty) << endl;
        }
        else if(p->kind.tag == KOOPA_RVT_INTEGER) {
            out << "    .word " << p->kind.data.integer.value << endl;
        }
    }
    out << endl;
}

void visit(ofstream &out, const koopa_raw_function_t &func) {
    if(func->bbs.len == 0)
        return;
    current_func = &(func->name[1]);
    out << "    .text\n";
    out << "    .globl " << current_func << "\n";
    out << current_func << ":\n";
    
    StackFrame stackFrame = StackFrame();
    preprocess(func->bbs, stackFrame);
    getStackLength(func, stackFrame);
    

    prologue(out, stackFrame);
    visit(out, func->params, stackFrame);
    visit(out, func->bbs, stackFrame);
    out << endl;
}

void visit(ofstream &out, const koopa_raw_basic_block_t &bb, StackFrame &stackFrame) {
    if(strcmp(bb->name, "%entry") != 0 ) {
        out << current_func << "_" << &(bb->name[1]) << ":\n"; 
    }
    visit(out, bb->params, stackFrame);
    visit(out, bb->insts, stackFrame);
}

void visit(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame) {
    const auto &kind = value->kind;
    void *ptr = (void *)&kind;
    switch(kind.tag) {
    case KOOPA_RVT_RETURN: // unit
        visit(out, kind.data.ret, stackFrame);
        break;
    case KOOPA_RVT_INTEGER:// int32(好像这个函数不会访问到这里？)
        visit(out, kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:// int32
        visit(out, kind.data.binary, stackFrame);
        registers[ptr] = "t" + to_string(last_reg);
        break;
    case KOOPA_RVT_ALLOC:// pointer xxx
        {
            auto ty = value->ty->data.pointer.base;
            int len = 1;
            while(ty->tag == KOOPA_RTT_ARRAY) {
                len *= ty->data.array.len;
                ty = ty->data.array.base;
            }
            len *= 4;
            stackFrame.add(ptr, len);
        }
        break;
    case KOOPA_RVT_LOAD:// int32
        visit(out, kind.data.load, stackFrame);
        registers[ptr] = "t" + to_string(last_reg);
        break;
    case KOOPA_RVT_STORE:// unit
        visit(out, kind.data.store, stackFrame);
        break;
    case KOOPA_RVT_BLOCK_ARG_REF:
        if(kind.data.block_arg_ref.index >= 8)
            stackFrame.add(ptr, 4);
        else {
            registers[ptr] = "a" + to_string(kind.data.block_arg_ref.index);
            if(spill.find(value) != spill.end()) {
                stackFrame.add(ptr, 4);
                int pos = stackFrame.find(ptr);
                if(pos >= 2048) {
                    out << "    addi t6, sp, 2047\n";
                    pos -= 2047;
                    while(pos >= 2048) {
                        out << "    addi t6, t6, 2047\n";
                        pos -= 2047;
                    }
                    out << "    sw a" << kind.data.block_arg_ref.index << ", " << pos << "(t6)\n";
                }
                else {
                    out << "    sw a" << kind.data.block_arg_ref.index << ", " << pos << "(sp)\n";
                }
                registers.erase(ptr);
            }

        }

        break;
    case KOOPA_RVT_FUNC_ARG_REF:
        if(kind.data.func_arg_ref.index >= 8) {
            stackFrame.addToLastFrame(ptr, 4);
        }
        else
            registers[ptr] = "a" + to_string(kind.data.func_arg_ref.index);
        break;
    case KOOPA_RVT_BRANCH:
        visit(out, kind.data.branch, stackFrame);
        break;
    case KOOPA_RVT_JUMP:
        visit(out, kind.data.jump, stackFrame);
        break;
    case KOOPA_RVT_CALL:
        visit(out, kind.data.call, stackFrame);
        if(value->ty->tag != KOOPA_RTT_UNIT) {
            last_reg = newReg();
            out << "    mv t" << last_reg << ", a0\n";
            registers[ptr] = "t" + to_string(last_reg);
        }
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        visit(out, kind.data.get_elem_ptr, stackFrame);
        registers[ptr] = "t" + to_string(last_reg);
        break;
    case KOOPA_RVT_GET_PTR:
        visit(out, kind.data.get_ptr, stackFrame);
        registers[ptr] = "t" + to_string(last_reg);
        break;
    default:
        assert(false);
    }

    if(value->ty->tag != KOOPA_RTT_UNIT && kind.tag != KOOPA_RVT_BLOCK_ARG_REF && kind.tag != KOOPA_RVT_FUNC_ARG_REF) {
        if(value->used_by.len == 0) {
            registers.erase(ptr);
            reg[last_reg] = false;
        }
        // 这两者应该不会同时出现，一个不被用到的value等同于被当场use，不会轮到它来spill
        if(spill.find(value) != spill.end()) {
            stackFrame.add(ptr, 4);
            int pos = stackFrame.find(ptr);
            if(pos >= 2048) {
                out << "    addi t6, sp, 2047\n";
                pos -= 2047;
                while(pos >= 2048) {
                    out << "    addi t6, t6, 2047\n";
                    pos -= 2047;
                }
                out << "    sw t" << last_reg << ", " << pos << "(t6)\n";
            }
            else {
                out << "    sw t" << last_reg << ", " << pos << "(sp)\n";
            }
            reg[last_reg] = false;
            registers.erase(ptr);
        }
    }
}

void visit(ofstream &out, const koopa_raw_return_t &ret, StackFrame &stackFrame) {
    if(ret.value != nullptr) {
        string regv = getReg(out, ret.value, stackFrame);
        out << "    mv a0, " << regv << endl;
    }
    epilogue(out, stackFrame);
    out << "    ret\n";
}

void visit(ofstream &out, const koopa_raw_integer_t &integer) {
    if(integer.value == 0) {
        isZero = true;
    }
    else {
        last_reg = newReg();
        out << "    li t" << last_reg << ", " << integer.value << "\n";
    }
}

void visit(ofstream &out, const koopa_raw_binary_t &binary, StackFrame &stackFrame) {
    
    auto lhs = binary.lhs;
    auto rhs = binary.rhs;

    string regl = getReg(out, lhs, stackFrame, false);
    string regr = getReg(out, rhs, stackFrame, false);
    if(lhs->kind.tag == KOOPA_RVT_INTEGER || used_time[(void *)&lhs->kind] >= lhs->used_by.len) {
        registers.erase((void *)&lhs->kind);
        if(regl[0] == 't') {
            reg[regl[1] - '0'] = false;
        }
    }
    if(rhs->kind.tag == KOOPA_RVT_INTEGER || used_time[(void *)&rhs->kind] >= rhs->used_by.len) {
        registers.erase((void *)&rhs->kind);
        if(regr[0] == 't') {
            reg[regr[1] - '0'] = false;
        }
    }
    last_reg = newReg();
    string reg_dest = "t" + to_string(last_reg);
    
    switch(binary.op) {
    case KOOPA_RBO_ADD:
        out << "    add " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_SUB:
        out << "    sub " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_MUL:
        out << "    mul " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_DIV:
        out << "    div " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_MOD:
        out << "    rem " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_LT:
        out << "    slt " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_GT:
        out << "    sgt " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_LE:
        out << "    sgt " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    xori "<< reg_dest << ", " << reg_dest << ", " << 1 << "\n";
        break;
    case KOOPA_RBO_GE:
        out << "    slt " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    xori "<< reg_dest << ", " << reg_dest << ", " << 1 << "\n";
        break;
    case KOOPA_RBO_EQ:
        out << "    xor " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    seqz "<< reg_dest << ", " << reg_dest << "\n";
        break;
    case KOOPA_RBO_NOT_EQ:
        out << "    xor " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    snez "<< reg_dest << ", " << reg_dest << "\n";
        break;
    case KOOPA_RBO_AND:
        out << "    and " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_OR:
        out << "    or "  << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    }
}

void visit(ofstream &out, const koopa_raw_load_t &load, StackFrame &stackFrame) {
    void *ptr = (void *)&load.src->kind;
    if(load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        last_reg = newReg();
        out << "    la t" << last_reg << ", " << &(load.src->name[1]) << endl;
        out << "    lw t" << last_reg << ", 0(t" << last_reg << ")\n"; 
    }
    else {
        int pos = stackFrame.find(ptr);
        if(pos != -1 && load.src->kind.tag == KOOPA_RVT_ALLOC) {
            if(pos >= 2048) {
                out << "    addi t6, sp, 2047\n";
                pos -= 2047;
                while(pos >= 2048) {
                    out << "    addi t6, t6, 2047\n";
                    pos -= 2047;
                }
                last_reg = newReg();
                out << "    lw t" << last_reg << ", " << pos << "(t6)\n";
            }
            else {
                last_reg = newReg();
                out << "    lw t" << last_reg << ", " << pos << "(sp)\n";
            }
        }
        else {
            string regd = getReg(out, load.src, stackFrame);
            last_reg = newReg();
            out << "    lw t" << last_reg << ", " << "0(" << regd << ")\n";
        }
    }
}

void visit(ofstream &out, const koopa_raw_store_t &store, StackFrame &stackFrame) {
    auto dest = store.dest;
    auto value = store.value;

    if(value->kind.tag == KOOPA_RVT_AGGREGATE) {
        int origin = stackFrame.find((void *)&dest->kind);
        int bias = 0;
        stack<koopa_raw_value_t> travel_stack;
        travel_stack.push(value);
        while(!travel_stack.empty()) {
            auto p = travel_stack.top();
            travel_stack.pop();
            if(p->kind.tag == KOOPA_RVT_AGGREGATE) {
                for(int i = p->kind.data.aggregate.elems.len - 1; i >= 0; i--) {
                    travel_stack.push((koopa_raw_value_t)p->kind.data.aggregate.elems.buffer[i]);
                }
            }
            else if(p->kind.tag == KOOPA_RVT_ZERO_INIT) {
                bias += getArraySize(p->ty);
            }
            else if(p->kind.tag == KOOPA_RVT_INTEGER) {
                int value = p->kind.data.integer.value;
                int r = newReg();
                out << "    li t" << r << ", " << value << endl;
                int pos = origin + bias;
                if(pos >= 2048) {
                    out << "    addi t6, sp, 2047\n";
                    pos -= 2047;
                    while(pos >= 2048) {
                        out << "    addi t6, t6, 2047\n";
                        pos -= 2047;
                    }
                    out << "    sw t" << r << ", " << pos << "(t6)\n";
                }
                else {
                    out << "    sw t" << r << ", " << pos << "(sp)\n";
                }
                bias += 4;
                reg[r] = false;
            }
        }
    }
    else if(value->kind.tag == KOOPA_RVT_ZERO_INIT) {
        return;
    }
    else {
        string regv = getReg(out, value, stackFrame, false);
        if(dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            int r = newReg();
            out << "    la t" << r << ", " << &(dest->name[1]) << endl;
            out << "    sw " << regv << ", 0(t" << r << ")\n"; 
            reg[r] = false;
            if(regv[0] == 't') {
                reg[regv[1]-'0'] = false;
            }
        }
        else {
            int pos = stackFrame.find((void *)&dest->kind);
            if(pos != -1 && dest->kind.tag == KOOPA_RVT_ALLOC) {
                if(pos >= 2048) {
                    out << "    addi t6, sp, 2047\n";
                    pos -= 2047;
                    while(pos >= 2048) {
                        out << "    addi t6, t6, 2047\n";
                        pos -= 2047;
                    }
                    out << "    sw " << regv << ", " << pos << "(t6)\n";
                }
                else {
                    out << "    sw " << regv << ", " << pos << "(sp)\n";
                }
                if(regv[0] == 't') {
                }
                    reg[regv[1]-'0'] = false;
            }
            else {
                string regd = getReg(out, dest, stackFrame);
                out << "    sw " << regv << ", " << "0(" << regd << ")\n";
                if(regv[0] == 't') {
                    reg[regv[1]-'0'] = false;
                }
            }
        }
    }
}

void visit(ofstream &out, const koopa_raw_branch_t &branch, StackFrame &stackFrame) {
    auto cond = branch.cond;
    auto true_bb = branch.true_bb;
    auto true_args = branch.true_args;
    auto false_bb = branch.false_bb;
    auto false_args = branch.false_args;

    string regc = getReg(out, cond, stackFrame);
    if(regc[0] == 'a') {
        int r;
        r = newReg();
        out << "    mv t" << r << ", " << regc << endl;
        regc = "t" + to_string(r);
    }
    visitParams(out, true_args, stackFrame);
    out << "    bnez " << regc << ", j" << current_func << "_" << &(true_bb->name[1]) << endl;
    if(regc[0] == 't') {
        reg[regc[1]-'0'] = false;
    }
    visitParams(out, false_args, stackFrame);
    out << "    j " << current_func << "_" << &(false_bb->name[1]) << endl;
    out << "j" << current_func << "_" << &(true_bb->name[1]) << ":\n";
    out << "    j " << current_func << "_" << &(true_bb->name[1]) << endl;
}

void visit(ofstream &out, const koopa_raw_jump_t &jump, StackFrame &stackFrame) {
    auto target = jump.target;
    auto args = jump.args;

    visitParams(out, args, stackFrame);
    out << "    j " << current_func << "_" << &(target->name[1]) << endl;
}

void visit(ofstream &out, const koopa_raw_call_t &call, StackFrame &stackFrame) {
    auto callee = call.callee;
    auto args = call.args;

    visitParams(out, args, stackFrame);
    // for(auto iter = registers.begin(); iter != registers.end(); iter++) {
    //     if(iter->second[0] == 't') {
    //         out << "    mv s" << iter->second[1] << ", " << iter->second << endl;
    //     }
    // }
    out << "    call " << &(callee->name[1]) << endl;
    // for(auto iter = registers.begin(); iter != registers.end(); iter++) {
    //     if(iter->second[0] == 't') {
    //         out << "    mv " << iter->second << ", s" << iter->second[1] << endl;
    //     }
    // }
}

void visit(ofstream &out, const koopa_raw_get_elem_ptr_t &getelem, StackFrame &stackFrame) {
    auto src = getelem.src;
    auto index = getelem.index;

    auto ty = src->ty->data.pointer.base->data.array.base;
    int bias = getArraySize(ty);
    int r = newReg();
    out << "    li t" << r << ", " << bias << endl;
    string regi = getReg(out, index, stackFrame);
    string regb = "t" + to_string(r);
    out << "    mul " << regb << ", t" << r << ", " << regi << endl;
    if(regi[0] == 't')
        reg[regi[1]-'0'] = false;
    if(src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        int nr = newReg();
        out << "    la t" << nr << ", " << &(src->name[1]) << endl;
        out << "    add t" << r << ", t" << nr << ", " << regb << endl;
        reg[nr] = false;
    }
    else {
        int origin = stackFrame.find((void *)&src->kind);
        if(origin == -1) {
            string regs= getReg(out, src, stackFrame);
            out << "    add t" << r<< ", " << regs << ", " << regb << endl;
        }
        else if(src->kind.tag == KOOPA_RVT_ALLOC){
            out << "    add t" << r << ", sp" << ", " << regb << endl;
            while(origin >= 2048) {
                out << "    addi t" << r << ", t" << r << ", " << 2047 << endl;
                origin -= 2047;
            }
            out << "    addi t" << r << ", t" << r << ", " << origin << endl;
        }
        else {
            string regs = getReg(out, src, stackFrame);
            out << "    add t" << r << ", " << regs << ", " << regb << endl;
        }
    }
    last_reg = r;
}

void visit(ofstream &out, const koopa_raw_get_ptr_t &getptr, StackFrame &stackFrame) {
    auto src = getptr.src;
    auto index = getptr.index;

    auto ty = src->ty->data.pointer.base;
    int bias = getArraySize(ty);
    int r = newReg();
    out << "    li t" << r << ", " << bias << endl;
    string regi = getReg(out, index, stackFrame);
    string regb = "t" + to_string(r);
    out << "    mul " << regb << ", t" << r << ", " << regi << endl;
    if(regi[0] == 't')
        reg[regi[1]-'0'] = false;
    if(src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        int nr = newReg();
        out << "    la t" << nr << ", " << &(src->name[1]) << endl;
        out << "    add t" << r << ", t" << nr << ", " << regb << endl;
        reg[nr] = false;
    }
    else {
        int origin = stackFrame.find((void *)&src->kind);
        if(origin == -1) {
            string regs= getReg(out, src, stackFrame);
            out << "    add t" << r<< ", " << regs << ", " << regb << endl;
        }
        else if(src->kind.tag == KOOPA_RVT_ALLOC){
            out << "    add t" << r << ", sp" << ", " << regb << endl;
            while(origin >= 2048) {
                out << "    addi t" << r << ", t" << r << ", " << 2047 << endl;
                origin -= 2047;
            }
            out << "    addi t" << r << ", t" << r << ", " << origin << endl;
        }
        else {
            string regs = getReg(out, src, stackFrame);
            out << "    add t" << r << ", " << regs << ", " << regb << endl;
        }
    }
    last_reg = r;
}

void getStackLength(const koopa_raw_function_t &func, StackFrame &stackFrame) {
    bool called = false;
    int ret = 0;
    unsigned int max_args = 0; // to solve error of max(int, uint32_t)
    for(int i = 0; i < func->bbs.len; i++) {
        auto ptr_bb = (koopa_raw_basic_block_data_t *)func->bbs.buffer[i];
        for(int j = 0; j < ptr_bb->insts.len; j++) {
            auto value = (koopa_raw_value_data_t *)ptr_bb->insts.buffer[j];
            if(value->kind.tag == KOOPA_RVT_CALL) {
                called = true;
                if(value->kind.data.call.args.len > 8) {
                    max_args = max(max_args, value->kind.data.call.args.len - 8);
                }
            }
            
            if(value->kind.tag == KOOPA_RVT_ALLOC)
            {
                int incr = 1;
                auto ty = value->ty->data.pointer.base;
                while(true) {
                    if(ty->tag == KOOPA_RTT_ARRAY) {
                        incr *= ty->data.array.len;
                        ty = ty->data.array.base;
                    }
                    else {
                        break;
                    }
                }
                incr *= 4;
                ret += incr;
            }
        }
    }
    ret += max_args * 4;
    if(called)
        ret += 4;
    
    stackFrame.saved_ra = called;
    stackFrame.paramsLength = max_args * 4;
    stackFrame.length += ret;
    stackFrame.align();
}

int getArraySize(const koopa_raw_type_t &ty) {
    auto ty_ = ty;
    int ret = 1;
    while(ty_->tag == KOOPA_RTT_ARRAY) {
        ret *= ty_->data.array.len;
        ty_ = ty_->data.array.base;
    }
    ret *= 4;
    return ret;
}

void prologue(ofstream &out, StackFrame &stackFrame) {
    if(stackFrame.length > 0) {
        int l = stackFrame.length;
        while(l > 2048) {
            out << "    addi sp, sp, -2048\n";
            l -= 2048;
        }
        out << "    addi sp, sp, " << -l << '\n';
    }

    if(stackFrame.saved_ra) {
        int pos = stackFrame.length - 4;
        if(pos >= 2048) {
            out << "    addi t6, sp, 2047\n";
            pos -= 2047;
            while(pos >= 2048) {
                out << "    addi t6, t6, 2047\n";
                pos -= 2047;
            }
            out << "    sw ra, " << pos << "(t6)\n";
        }
        else {
            out << "    sw ra, " << pos << "(sp)\n";
        }
    }
    
}

void epilogue(ofstream &out, StackFrame &stackFrame) {
    if(stackFrame.saved_ra) {
        int pos = stackFrame.length - 4;
        if(pos >= 2048) {
            out << "    addi t6, sp, 2047\n";
            pos -= 2047;
            while(pos >= 2048) {
                out << "    addi t6, t6, 2047\n";
                pos -= 2047;
            }
            out << "    lw ra, " << pos << "(t6)\n";
        }
        else {
            out << "    lw ra, " << pos << "(sp)\n";
        }
    }
    
    if(stackFrame.length > 0) {
        int l = stackFrame.length;
        while(l >= 2048) {
            out << "    addi sp, sp, 2047\n";
            l -= 2047;
        }
        out << "    addi sp, sp, " << l << '\n';
    }
}

string getReg(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame, bool checkNum) {
    string regv;

    if(value->kind.tag == KOOPA_RVT_INTEGER) {
        visit(out, value->kind.data.integer);
        if(isZero) {
            isZero = false;
            regv = "x0";
        }
        else {
            regv = "t" + to_string(last_reg);
        }
    }
    else if (((
                value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF 
                || value->kind.tag == KOOPA_RVT_FUNC_ARG_REF
            ) && value->kind.data.block_arg_ref.index >= 8
        ) || spill.find(value) != spill.end()){
        void *ptr = (void *)&value->kind;
        last_reg = newReg();
        regv = "t" + to_string(last_reg);
        int pos = stackFrame.find(ptr);
        if(pos >= 2048) {
            out << "    addi t6, sp, 2047\n";
            pos -= 2047;
            while(pos >= 2048) {
                out << "    addi t6, t6, 2047\n";
                pos -= 2047;
            }
            out << "    lw " << regv << ", " << pos << "(t6)\n";
        }
        else {
            out << "    lw " << regv << ", " << pos << "(sp)\n";
        }
        if(used_time.find(ptr) == used_time.end()) {
            used_time[ptr] = 1;
        }
        else {
            used_time[ptr]++;
        }
        
        if(value->used_by.len <= used_time[ptr] && checkNum) {
            registers.erase(ptr);
            reg[regv[1]-'0'] = false;
        }
    }
    else {
        void *ptr = (void *)&value->kind;
        regv = registers[ptr];
        if(regv[0] != 'a') {
            if(used_time.find(ptr) == used_time.end()) {
                used_time[ptr] = 1;
            }
            else {
                used_time[ptr]++;
            }
            
            if(value->used_by.len <= used_time[ptr] && checkNum) {
                registers.erase(ptr);
                reg[regv[1]-'0'] = false;
            }
        }
    }

    return regv;
}

int newReg() {
    for(int i = 0; i < 6; i++) {
        if(!reg[i]) {
            reg[i] = true;
            return i;
        }
    }
    return 6;
}

void visitParams(ofstream &out, const koopa_raw_slice_t &params, StackFrame &stackFrame) {
    if(params.kind != KOOPA_RSIK_VALUE)
        assert(false);
    for(int i = 0; i < params.len; i++) {
        auto param = (koopa_raw_value_data_t *)params.buffer[i];
        auto ptr = (void *)&param->kind;
        if(param->kind.tag != KOOPA_RVT_INTEGER) {
            auto r = registers.find(ptr);
            if(r != registers.end() && r->second[0] == 'a') {
                last_reg = newReg();
                out << "    mv t" << last_reg << ", " << r->second << endl;
                registers[ptr] = "t" + to_string(last_reg);
            }
        }
    }
    for(int i = 0; i < params.len; i++) {
        auto ptr = (koopa_raw_value_data_t *)params.buffer[i];
        if(i < 8) {
            switch(ptr->kind.tag) {
            case KOOPA_RVT_INTEGER:
                out << "    li a" << i << ", " << ptr->kind.data.integer.value << endl;
                break;
            default:
                string s = getReg(out, ptr, stackFrame);
                out << "    mv a" << i << ", " << s << endl;
                if(s[0] == 't') {
                    reg[s[1]-'0'] = false;
                }
                break;
            }
        }
        else {
            string s = getReg(out, ptr, stackFrame);
            int pos = (i - 8) * 4;
            if(pos >= 2048) {
                out << "    addi t6, sp, 2047\n";
                pos -= 2047;
                while(pos >= 2048) {
                    out << "    addi t6, t6, 2047\n";
                    pos -= 2047;
                }
                out << "    sw " << s << ", " << pos << "(t6)\n";
            }
            else {
                out << "    sw " << s << ", " << pos << "(sp)\n";
            }
            if(s[0] == 't') {
                reg[s[1] - '0'] = false;
            }
        }
    }
}

void preprocess(const koopa_raw_slice_t &bbs, StackFrame &stackFrame) {
    int spill_bytes = 0;
    spill.clear();
    for(int i = 0; i < bbs.len; i++) {
        auto block = (koopa_raw_basic_block_data_t *)bbs.buffer[i];
        unordered_map<koopa_raw_value_t, int> start, end;
        unordered_map<int, set<koopa_raw_value_t> > use;
        unordered_map<int, koopa_raw_value_t> def;
        unordered_map<int, set<koopa_raw_value_t> > extra_def;
        set<koopa_raw_value_t> def_but_not_used;
        for(int j = 0; j < block->params.len; j++) {
            auto param = (koopa_raw_value_data_t *)block->params.buffer[j];
            def_but_not_used.insert(param);
        }
        for(int j = 0; j < block->insts.len; j++) {
            auto inst = (koopa_raw_value_data_t *)block->insts.buffer[j];
            switch(inst->kind.tag) {
            case KOOPA_RVT_RETURN: 
                {
                    auto v = inst->kind.data.ret.value;
                    if(v != nullptr) {
                        if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0)) {       
                            end[v] = j;
                            use[j].insert(v);
                        }
                        if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                            extra_def[j].insert(v);
                        else 
                            def_but_not_used.erase(v);
                    }
                }
                break;
            case KOOPA_RVT_BINARY:
                {
                    auto l = inst->kind.data.binary.lhs;
                    auto r = inst->kind.data.binary.rhs;
                    if(!(l->kind.tag == KOOPA_RVT_INTEGER && l->kind.data.integer.value == 0)) {
                        end[l] = j;
                        use[j].insert(l);
                    }
                    if(!(r->kind.tag == KOOPA_RVT_INTEGER && r->kind.data.integer.value == 0)) {
                        end[r] = j;
                        use[j].insert(r);
                    }
                    if(l->kind.tag == KOOPA_RVT_INTEGER && l->kind.data.integer.value != 0)
                        extra_def[j].insert(l);
                    else if(l->kind.tag != KOOPA_RVT_INTEGER)
                        def_but_not_used.erase(l);
                    if(r->kind.tag == KOOPA_RVT_INTEGER && r->kind.data.integer.value != 0)
                        extra_def[j].insert(r);
                    else if(r->kind.tag != KOOPA_RVT_INTEGER)
                        def_but_not_used.erase(r);
                }
                break;
            case KOOPA_RVT_LOAD:
                {
                    auto v = inst->kind.data.load.src;
                    if(v->kind.tag == KOOPA_RVT_ALLOC)
                        break;
                    if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0) && v->kind.tag != KOOPA_RVT_ALLOC) {       
                        end[v] = j;
                        use[j].insert(v);
                    }
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else if(v->kind.tag != KOOPA_RVT_ALLOC)
                        def_but_not_used.erase(v);
                }
                break;
            case KOOPA_RVT_STORE:
                {
                    auto v = inst->kind.data.store.value;
                    auto d = inst->kind.data.store.dest;
                    if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0)) {       
                        end[v] = j;
                        use[j].insert(v);
                    }
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);

                    if(!(d->kind.tag == KOOPA_RVT_INTEGER && d->kind.data.integer.value == 0) && d->kind.tag != KOOPA_RVT_ALLOC) {       
                        end[d] = j;
                        use[j].insert(d);
                    }
                    if(d->kind.tag == KOOPA_RVT_INTEGER && d->kind.data.integer.value != 0)
                        extra_def[j].insert(d);
                    else if(d->kind.tag != KOOPA_RVT_ALLOC)
                        def_but_not_used.erase(d);
                }
                break;
            case KOOPA_RVT_BRANCH:
                {
                    auto c = inst->kind.data.branch.cond;
                    if(!(c->kind.tag == KOOPA_RVT_INTEGER && c->kind.data.integer.value == 0)) {       
                        end[c] = j;
                        use[j].insert(c);
                    }
                    if(c->kind.tag == KOOPA_RVT_INTEGER && c->kind.data.integer.value != 0)
                        extra_def[j].insert(c);
                    else
                        def_but_not_used.erase(c);
                }
                for(int k = 0; k < inst->kind.data.branch.true_args.len; k++) {
                    auto v = (koopa_raw_value_t)inst->kind.data.branch.true_args.buffer[k];
                    if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0)) {       
                        end[v] = j;
                        use[j].insert(v);
                    }
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);
                }
                for(int k = 0; k < inst->kind.data.branch.false_args.len; k++) {
                    auto v = (koopa_raw_value_t)inst->kind.data.branch.false_args.buffer[k];
                    if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0)) {       
                        end[v] = j;
                        use[j].insert(v);
                    }
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);
                }
                break;
            case KOOPA_RVT_CALL:
                for(int k = 0; k < inst->kind.data.call.args.len; k++) {
                    auto v = (koopa_raw_value_t)inst->kind.data.call.args.buffer[k];
                    if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0)) {       
                        end[v] = j;
                        use[j].insert(v);
                    }
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);
                }
                if(inst->ty->tag == KOOPA_RTT_INT32) {
                    for(auto iter = def_but_not_used.begin(); iter != def_but_not_used.end(); iter++) {
                        auto s = start.find(*iter);
                        if(s != start.end())
                            def.erase(s->second);
                        spill.insert(*iter);
                        spill_bytes += 4;
                    }
                }
                break;
            case KOOPA_RVT_JUMP:
                for(int k = 0; k < inst->kind.data.jump.args.len; k++) {
                    auto v = (koopa_raw_value_t)inst->kind.data.jump.args.buffer[k];
                    if(!(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value == 0)) {       
                        end[v] = j;
                        use[j].insert(v);
                    }
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);
                }
                break;
            case KOOPA_RVT_GET_ELEM_PTR:
                {
                    auto s = inst->kind.data.get_elem_ptr.src;
                    auto x = inst->kind.data.get_elem_ptr.index;
                    auto b = createIntegerValueData(0); // 占位符，代表数组大小所用的寄存器
                    end[b] = j;
                    use[j].insert(b);
                    extra_def[j].insert(b);
                    if(!(s->kind.tag == KOOPA_RVT_INTEGER && s->kind.data.integer.value == 0) && s->kind.tag != KOOPA_RVT_ALLOC) {
                        end[s] = j;
                        use[j].insert(s);
                    }
                    if(!(x->kind.tag == KOOPA_RVT_INTEGER && x->kind.data.integer.value == 0)) {
                        end[x] = j;
                        use[j].insert(x);
                    }
                    if(s->kind.tag == KOOPA_RVT_INTEGER && s->kind.data.integer.value != 0)
                        extra_def[j].insert(s);
                    else if(s->kind.tag != KOOPA_RVT_INTEGER && s->kind.tag != KOOPA_RVT_ALLOC)
                        def_but_not_used.erase(s);
                    if(x->kind.tag == KOOPA_RVT_INTEGER && x->kind.data.integer.value != 0)
                        extra_def[j].insert(x);
                    else if(x->kind.tag != KOOPA_RVT_INTEGER)
                        def_but_not_used.erase(x);
                }
                break;
            case KOOPA_RVT_GET_PTR:
                {
                    auto s = inst->kind.data.get_ptr.src;
                    auto x = inst->kind.data.get_ptr.index;
                    auto b = createIntegerValueData(0); // 占位符，代表数组大小所用的寄存器
                    end[b] = j;
                    use[j].insert(b);
                    extra_def[j].insert(b);
                    if(!(s->kind.tag == KOOPA_RVT_INTEGER && s->kind.data.integer.value == 0) && s->kind.tag != KOOPA_RVT_ALLOC) {
                        end[s] = j;
                        use[j].insert(s);
                    }
                    if(!(x->kind.tag == KOOPA_RVT_INTEGER && x->kind.data.integer.value == 0)) {
                        end[x] = j;
                        use[j].insert(x);
                    }
                    if(s->kind.tag == KOOPA_RVT_INTEGER && s->kind.data.integer.value != 0)
                        extra_def[j].insert(s);
                    else if(s->kind.tag != KOOPA_RVT_INTEGER &&s->kind.tag != KOOPA_RVT_ALLOC)
                        def_but_not_used.erase(s);
                    if(x->kind.tag == KOOPA_RVT_INTEGER && x->kind.data.integer.value != 0)
                        extra_def[j].insert(x);
                    else if(x->kind.tag != KOOPA_RVT_INTEGER)
                        def_but_not_used.erase(x);
                }
                break;
            default:
                break;
            }
            if(inst->ty->tag != KOOPA_RTT_UNIT && inst->kind.tag != KOOPA_RVT_ALLOC) {
                def[j] = inst;
                start[inst] = j;
                def_but_not_used.insert(inst);
            }
            if(inst->ty->tag != KOOPA_RTT_UNIT && inst->used_by.len == 0) {
                use[j].insert(inst);
                end[inst] = j;
                def_but_not_used.erase(inst);
            }
        }
        for(auto iter = def_but_not_used.begin(); iter != def_but_not_used.end(); iter++) {
            spill.insert(*iter);
            spill_bytes += 4;
        }
        // params不参与t0-t5的分配

        set<koopa_raw_value_t> active;
        for(int j = 0; j < block->insts.len; j++) {
            //auto inst = (koopa_raw_value_data_t *)block->insts.buffer[j];
            active.insert(extra_def[j].begin(), extra_def[j].end());

            while(active.size() > 6) {
                auto spill_value = active.begin();
                for(auto k = active.begin(); k != active.end(); k++) {
                    if(end[*k] > end[*spill_value]) {
                        spill_value = k;
                    }
                    else if(end[*k] == end[*spill_value]) {
                        if(start[*k] < start[*spill_value]) {
                            spill_value = k;
                        }
                    }
                }
                spill.insert(*spill_value);
                active.erase(spill_value);
                if((*spill_value)->ty->tag == KOOPA_RTT_INT32)
                    spill_bytes += 4;
            }
            set<koopa_raw_value_t> temp;
            set_difference(active.begin(), active.end(), use[j].begin(), use[j].end(), inserter(temp, temp.end()));
            active = temp;
            auto d = def.find(j);
            if(d != def.end())
                active.insert(d->second);
        }
    }
    stackFrame.length += spill_bytes;
}