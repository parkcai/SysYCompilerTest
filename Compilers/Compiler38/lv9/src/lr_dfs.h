#pragma once
#include "koopa.h"
#include <string>
#include "stack_manager.h"
#define DEBUG

void Visit(const koopa_raw_function_t &func, std::string& str);
void Visit(const koopa_raw_basic_block_t &bb, std::string& str);
void Visit(const koopa_raw_value_t &value, std::string& str);

/************ 工具函数部分 ************/

inline int get_array_size(koopa_raw_type_t type)
{
    koopa_raw_type_t temp_ty = type;
    int ret = 4;
    while(temp_ty->tag != KOOPA_RTT_INT32 && temp_ty->tag != KOOPA_RTT_POINTER)
    {
        ret *= temp_ty->data.array.len;
        temp_ty = temp_ty->data.array.base;
    }
    return ret;
}

void init_array(koopa_raw_value_t value, vector<int>& ret)
{
    switch(value->kind.tag) {
        case KOOPA_RVT_AGGREGATE:
            for(int i=0;i<value->kind.data.aggregate.elems.len;++i)
            {
                init_array(reinterpret_cast<koopa_raw_value_t>(value->kind.data.aggregate.elems.buffer[i]), ret);
            }
            break;
        case KOOPA_RVT_INTEGER:
            ret.push_back(value->kind.data.integer.value);
            break;
        default:
            assert(false);
    }
}

bool inline offset_safe(int offset)
{
    return offset >= -2048 && offset <= 2047;
}

void inline get_st_value(const koopa_raw_value_t &value, std::string& str, std::string target)
{
    StackManager& st = StackManager::getInstance();
    int offset = st.get_offset((long long)&value->kind.data);
    if (offset == -1)
        offset = st.get_offset(std::string(value->name + 1));
    if(offset_safe(offset))
    {
        if(offset == -1)
            str.append("    la " + target + ", " + (value->name + 1) + "\n");
        else
            str.append("    lw " + target + ", " + std::to_string(offset) + "(sp)\n");
    } else {
        str.append("    li t6, " + std::to_string(offset) + "\n");
        str.append("    add t6, t6, sp\n");
        str.append("    lw " + target + ", 0(t6)\n");
    }
    #ifdef DEBUG
    offset = st.get_offset((long long)&value->kind.data);
    if (offset == -1)
        cout << "get_st_value: " << value->name + 1 << endl;
    else 
        cout << "get_st_value: " << (long long)&value->kind.data << endl;
    #endif
}

void inline get_arg_value(int arg_id, std::string& str, std::string target)
{
    #ifdef DEBUG
    cout << "get_arg_value: " << arg_id << endl;
    #endif
    StackManager& st = StackManager::getInstance();
    if(arg_id < 8)
    {
        str.append("    mv " + target + ", a" + std::to_string(arg_id) + "\n");
    } else {
        int offset = (arg_id - 8) * 4 + st.align();
        if(offset_safe(offset))
        {
            str.append("    lw " + target + ", " + std::to_string(offset) + "(sp)\n");
        } else {
            str.append("    li t6, " + std::to_string(offset) + "\n");
            str.append("    add t6, t6, sp\n");
            str.append("    lw " + target + ", 0(t6)\n");
        }
    }
    
}

void get_value(const koopa_raw_value_t &value, std::string& str, std::string target)
{
    switch(value->kind.tag)
    {
        case KOOPA_RVT_INTEGER:
            str.append("    li " + target + ", " + std::to_string(value->kind.data.integer.value) + "\n");
            break;
        case KOOPA_RVT_FUNC_ARG_REF:
            get_arg_value(value->kind.data.func_arg_ref.index, str, target);
            break;
        default:
            get_st_value(value, str, target);
    }
}

void put_value(const koopa_raw_value_t &value, std::string& str, std::string source)
{
    #ifdef DEBUG
    if(value->name)
        cout << "put_value: " << value->name + 1 << endl;
    else
        cout << "put_value: " << (long long)&value->kind.data << endl;
    #endif
    StackManager& st = StackManager::getInstance();
    // 地址优先
    int offset = st.get_offset((long long)&value->kind.data);
    if(offset==-1)
        offset = st.get_offset(std::string(value->name + 1));
    assert(offset!=-1);
    if(offset_safe(offset))
    {
        str.append("    sw " + source + ", " + std::to_string(offset) + "(sp)\n");
    } else {
        str.append("    li t6, " + std::to_string(offset) + "\n");
        str.append("    add t6, t6, sp\n");
        str.append("    sw " + source + ", 0(t6)\n");
    }
}

/************ 目标代码生成部分 ************/

void Root_Visit(const koopa_raw_program_t &program, std::string& str)
{
    str.append("    .data\n");
    for(int i=0;i<program.values.len;++i)
    {
        Visit(reinterpret_cast<koopa_raw_value_t>(program.values.buffer[i]), str);
    }
    str.append("    .text\n");
    for(int i=0;i<program.funcs.len;++i)
    {
        Visit(reinterpret_cast<koopa_raw_function_t>(program.funcs.buffer[i]), str);
    }
}

void Visit(const koopa_raw_function_t &func, std::string& str)
{
    if(func->bbs.len==0) return;
    #ifdef DEBUG
    cout << "Visit function: " << func->name + 1 << endl;
    #endif
    StackManager& st = StackManager::getInstance();
    st.new_func(std::string(func->name + 1));
    for (int i=0;i<func->bbs.len;++i)
    {
        for (int j=0;j < ((koopa_raw_basic_block_t)func->bbs.buffer[i])->insts.len;++j)
        {
            const koopa_raw_value_t& value = (koopa_raw_value_t)((koopa_raw_basic_block_t)func->bbs.buffer[i])->insts.buffer[j];
            if(value->kind.tag != KOOPA_RVT_ALLOC)
            {
                if(value->name)
                    st.push_in(std::string(value->name + 1));
                else
                    st.push_in((long long)&value->kind.data);
            } else {
                // 如果是alloc指令，那么name和地址都会被存入栈中
                assert(value->name);
                st.push_in(std::string(value->name + 1), get_array_size(value->ty->data.pointer.base));
                st.push_in((long long)&value->kind.data);
            }
        }
    }
    str.append("    .global "+st.cur_func+"\n");
    str.append(st.cur_func + ":\n");
    str.append("    sw ra, -4(sp)\n");
    str.append("    li t0, " + std::to_string(-st.align()) + "\n");
    str.append("    add sp, sp, t0\n");
    for(int i=0;i<func->bbs.len;++i)
    {
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]), str);
    }
}

void Visit(const koopa_raw_basic_block_t &bb, std::string& str)
{
    #ifdef DEBUG
    cout << "Visit basic block: " << bb->name + 1 << endl;
    #endif
    StackManager& st = StackManager::getInstance();
    str.append(st.cur_func + "_" + std::string(bb->name + 1) + ":\n");
    for(int i=0;i<bb->insts.len;++i)
    {
        Visit(reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]), str);
    }
}

void Visit(const koopa_raw_value_t &value, std::string& str)
{
    #ifdef DEBUG
    std::vector<std::string> stmt_type = { "KOOPA_RVT_INTEGER","KOOPA_RVT_ZERO_INIT","KOOPA_RVT_UNDEF","KOOPA_RVT_AGGREGATE","KOOPA_RVT_FUNC_ARG_REF",
    "KOOPA_RVT_BLOCK_ARG_REF","KOOPA_RVT_ALLOC","KOOPA_RVT_GLOBAL_ALLOC","KOOPA_RVT_LOAD","KOOPA_RVT_STORE",
    "KOOPA_RVT_GET_PTR","KOOPA_RVT_GET_ELEM_PTR","KOOPA_RVT_BINARY","KOOPA_RVT_BRANCH","KOOPA_RVT_JUMP","KOOPA_RVT_CALL","KOOPA_RVT_RETURN"};
    cout << "Visit value: " << stmt_type[value->kind.tag] << endl;
    #endif
    StackManager& st = StackManager::getInstance();
    const auto &kind = value->kind;
    std::vector<int> init_vec = {};
    long long self_id = (long long)&value->kind.data; 
    int offset = 0, arg_len = 0;
    switch (kind.tag)
    {
    case KOOPA_RVT_GLOBAL_ALLOC:
        str.append("    .global " + std::string(value->name + 1) + "\n");
        str.append(std::string(value->name + 1) + ":\n");
        switch(value->kind.data.global_alloc.init->kind.tag)
        {
            case KOOPA_RVT_ZERO_INIT:
                str.append("    .zero " + std::to_string(get_array_size(value->ty->data.pointer.base)) + "\n");
                break;
            case KOOPA_RVT_INTEGER:
                str.append("    .word " + std::to_string(value->kind.data.global_alloc.init->kind.data.integer.value) + "\n");
                break;
            case KOOPA_RVT_AGGREGATE:
                init_array(value->kind.data.global_alloc.init, init_vec);
                for(int i=0;i<init_vec.size();++i)
                    str.append("    .word " + std::to_string(init_vec[i]) + "\n");
                break;
            default:
                assert(false);
        }
        break;
    case KOOPA_RVT_ALLOC:
        // 取出变量地址
        offset = st.get_offset(value->name + 1);
        assert(offset!=-1);
        str.append("    li t0, " + std::to_string(offset) + "\n");
        str.append("    add t0, t0, sp\n");
        // 存放到局部变量中
        offset = st.get_offset(self_id);
        assert(offset!=-1);
        if(offset_safe(offset)) {
            str.append("    sw t0, " + std::to_string(offset) + "(sp)\n");
        } else {
            str.append("    li t1, " + std::to_string(offset) + "\n");
            str.append("    add t1, t1, sp\n");
            str.append("    sw t0, 0(t1)\n");
        }
        break;
    case KOOPA_RVT_GET_PTR:
        get_value(kind.data.get_ptr.index, str, "t0");
        offset = get_array_size(kind.data.get_ptr.src->ty->data.pointer.base);
        str.append("    li t1, " + std::to_string(offset) + "\n");
        str.append("    mul t0, t0, t1\n");
        get_value(kind.data.get_ptr.src, str, "t1");
        str.append("    add t0, t0, t1\n");
        put_value(value, str, "t0");
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        get_value(kind.data.get_elem_ptr.index, str, "t0");
        offset = get_array_size(kind.data.get_elem_ptr.src->ty->data.pointer.base->data.array.base);
        str.append("    li t1, " + std::to_string(offset) + "\n");
        str.append("    mul t0, t0, t1\n");
        get_value(kind.data.get_elem_ptr.src, str, "t1");
        str.append("    add t0, t0, t1\n");
        put_value(value, str, "t0");
        break;
    case KOOPA_RVT_LOAD:
        get_value(kind.data.load.src, str, "t0");
        str.append("    lw t0, 0(t0)\n");
        put_value(value, str, "t0");
        break;
    case KOOPA_RVT_STORE:
        if(value->kind.data.store.value->kind.tag == KOOPA_RVT_AGGREGATE)
        {
        get_value(value->kind.data.store.dest, str, "t1");
        init_array(value->kind.data.store.value, init_vec);
        offset = init_vec.size() * 4;
        for(int i=0, flag=0; i<offset; i+=4)
        {
            if(i/2044 > flag) 
            {
                str.append("    addi t1, t1, 2044\n");
                flag++;
            }
            str.append("    li t0, " + std::to_string(init_vec[i/4]) + "\n"); 
            str.append("    sw t0, " + std::to_string(i%2044) + "(t1)\n");
        }
        } else {
            get_value(value->kind.data.store.value, str, "t0");
            get_value(value->kind.data.store.dest, str, "t1");
            str.append("    sw t0, 0(t1)\n");
        }
        break;
    case KOOPA_RVT_CALL:
        arg_len = kind.data.call.args.len;
        offset = 0;
        if(arg_len<=8)
            for(int i=0;i<arg_len;++i)
                get_value(reinterpret_cast<koopa_raw_value_t>(kind.data.call.args.buffer[i]), str, "a" + std::to_string(i));
        else{
            for(int i=0;i<8;++i)
                get_value(reinterpret_cast<koopa_raw_value_t>(kind.data.call.args.buffer[i]), str, "a" + std::to_string(i));
            offset = (arg_len - 8) * 4;
            str.append("    addi sp, sp, " + std::to_string(-offset) + "\n");
            for(int i=8;i<arg_len;++i) {
                get_value(reinterpret_cast<koopa_raw_value_t>(kind.data.call.args.buffer[i]), str, "t0");
                str.append("    sw t0, " + std::to_string((i-8)*4) + "(sp)\n");
            }
        }
        str.append("    call " + std::string(kind.data.call.callee->name + 1) + "\n");
        if (arg_len > 8)
            str.append("    addi sp, sp, " + std::to_string(offset) + "\n");
        put_value(value, str, "a0");
        break;
    case KOOPA_RVT_BINARY:
        get_value(kind.data.binary.lhs, str, "t0");
        get_value(kind.data.binary.rhs, str, "t1");
        switch(kind.data.binary.op)
        {
            case KOOPA_RBO_NOT_EQ:
                str.append("    xor t0, t0, t1\n");
                str.append("    snez t0, t0\n");
                break;
            case KOOPA_RBO_EQ:
                str.append("    xor t0, t0, t1\n");
                str.append("    seqz t0, t0\n");
                break;
            case KOOPA_RBO_GT:
                str.append("    sgt t0, t0, t1\n");
                break;
            case KOOPA_RBO_LT:
                str.append("    slt t0, t0, t1\n");
                break;
            case KOOPA_RBO_GE:
                str.append("    slt t0, t0, t1\n");
                str.append("    seqz t0, t0\n");
                break;
            case KOOPA_RBO_LE:
                str.append("    sgt t0, t0, t1\n");
                str.append("    seqz t0, t0\n");
                break;
            case KOOPA_RBO_ADD:
                str.append("    add t0, t0, t1\n");
                break;
            case KOOPA_RBO_SUB:
                str.append("    sub t0, t0, t1\n");
                break;
            case KOOPA_RBO_MUL:
                str.append("    mul t0, t0, t1\n");
                break;
            case KOOPA_RBO_DIV:
                str.append("    div t0, t0, t1\n");
                break;
            case KOOPA_RBO_MOD:
                str.append("    rem t0, t0, t1\n");
                break;
            case KOOPA_RBO_AND:
                str.append("    and t0, t0, t1\n");
                break;
            case KOOPA_RBO_OR:
                str.append("    or t0, t0, t1\n");
                break;
            case KOOPA_RBO_XOR:
                str.append("    xor t0, t0, t1\n");
                break;
            case KOOPA_RBO_SHL:
                str.append("    sll t0, t0, t1\n");
                break;
            case KOOPA_RBO_SHR:
                str.append("    srl t0, t0, t1\n");
                break;
            case KOOPA_RBO_SAR:
                str.append("    sra t0, t0, t1\n");
                break;
            default:
                assert(false);
        }
        put_value(value, str, "t0");
        break;
    case KOOPA_RVT_RETURN:
        if(kind.data.ret.value)
            get_value(kind.data.ret.value, str, "a0");
        str.append("    li t0, " + std::to_string(st.align()) + "\n");
        str.append("    add sp, sp, t0\n");
        str.append("    lw ra, -4(sp)\n");
        str.append("    ret\n");
        break;
    case KOOPA_RVT_BRANCH:
        get_value(kind.data.branch.cond, str, "t0");
        str.append("    beqz t0, " + st.cur_func + "_" + std::string(kind.data.branch.false_bb->name + 1) + "\n");
        str.append("    j " + st.cur_func + "_" + std::string(kind.data.branch.true_bb->name + 1) + "\n");
        break;
    case KOOPA_RVT_JUMP:
        str.append("    j " + st.cur_func + "_" + std::string(kind.data.jump.target->name + 1) + "\n");
        break;
    default:
        assert(false);
    }
    return;
}

