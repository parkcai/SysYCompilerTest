#include "visit.h"

/**********************************************************************************************************/
/************************************************Stack*****************************************************/
/**********************************************************************************************************/

void Stack::alloc_value(koopa_raw_value_t value, int loc)
{
    value_loc[value] = loc;
}

int Stack::get_loc(koopa_raw_value_t value)
{
    return value_loc[value];
}

void Stack::init()
{
    len = 0;
    pos = 0;
    value_loc.clear();
}

/**********************************************************************************************************/
/**********************************************PtrSizeVec**************************************************/
/**********************************************************************************************************/

int PtrSizeVec::get_value_total_size(koopa_raw_value_t value)
{
    int offset = 4;
    for (auto i : size_vec[value])
    {
        offset *= i;
    }
    return offset;
}

int PtrSizeVec::get_value_offset(koopa_raw_value_t src)
{
    auto it = size_vec[src].begin();
    ++it;
    int offset = 4;
    for (; it != size_vec[src].end(); it++)
    {
        offset *= (*it);
    }
    return offset;
}

void PtrSizeVec::push_size(koopa_raw_value_t value, int size)
{
    size_vec[value].push_back(size);
}

void PtrSizeVec::copy_size_vec(koopa_raw_value_t dest, koopa_raw_value_t src)
{
    if (size_vec.find(src) != size_vec.end())
    {
        size_vec[dest] = size_vec[src];
    }
}

void PtrSizeVec::copy_size_vec_ptr(koopa_raw_value_t dest, koopa_raw_value_t src)
{
    auto it = size_vec[src].begin();
    it++;
    std::vector<int> tmp;
    for (; it != size_vec[src].end(); it++)
    {
        tmp.push_back(*it);
    }
    size_vec[dest] = tmp;
}

/**********************************************************************************************************/
/************************************************Visit*****************************************************/
/**********************************************************************************************************/

// 访问 raw program
std::string Visit(const koopa_raw_program_t &program)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有全局变量
    std::string ret = "";
#ifdef DEBUG
    ret += "visit global value\n";
#endif
    ret += Visit(program.values);
    // 访问所有函数
#ifdef DEBUG
    ret += "visit functions\n";
#endif
    ret += Visit(program.funcs);

    std::string optimized_code = optimize_riscv_code(ret);

    return optimized_code;
}

// 访问 raw slice
std::string Visit(const koopa_raw_slice_t &slice)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit slice\n";
#endif
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            ret += Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            ret += Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            ret += Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
    return ret;
}

// 访问函数
std::string Visit(const koopa_raw_function_t &func)
{
    // 忽略函数声明
    std::string ret = "";
#ifdef DEBUG
    ret += "visit function\n";
#endif
    if (func->bbs.len == 0)
        return ret;

    // 执行一些其他的必要操作
    ret += "  .text\n";
    ret += "  .globl " + std::string(func->name + 1) + "\n";
    ret += std::string(func->name + 1) + ":\n";

    // 清空
    stack.init();

    // 计算栈帧长度需要的值
    // 局部变量个数
    int var_count = 0;
    // 是否需要为 ra 分配栈空间
    ra_count = 0;
    // 需要为传参预留几个变量的栈空间
    int arg_count = 0;

    // 遍历基本块
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        const auto &insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
        for (size_t j = 0; j < insts.len; ++j)
        {
            auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
            if (inst->ty->tag != KOOPA_RTT_UNIT)
            {
                var_count++;
            }
            if (inst->kind.tag == KOOPA_RVT_CALL)
            {
                ra_count = 1;
                arg_count = std::max(arg_count, std::max(0, int(inst->kind.data.call.args.len) - 8));
            }
            else if (inst->kind.tag == KOOPA_RVT_ALLOC &&
                     inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
            {
                var_count--;
                int arrmem = 1;
                auto base = inst->ty->data.pointer.base;
                while (base->tag == KOOPA_RTT_ARRAY)
                {
                    ptr_size_vec.push_size(inst, base->data.array.len);
                    arrmem *= base->data.array.len;
                    base = base->data.array.base;
                }
                var_count += arrmem;
            }
        }
    }
#ifdef DEBUG
    ret += "var_count: " + std::to_string(var_count) + "\n";
    ret += "ra_count: " + std::to_string(ra_count) + "\n";
    ret += "arg_count: " + std::to_string(arg_count) + "\n";
#endif
    stack.len = (var_count + ra_count + arg_count) * 4;
    // 将栈帧长度对齐到 16
    stack.len = (stack.len + 15) / 16 * 16;
    stack.pos = arg_count * 4;

    if (stack.len != 0)
    {
        ret += deal_offset_exceed(stack.len, "addi-", "sp");
    }

    if (ra_count)
    {
        ret += deal_offset_exceed(stack.len - 4, "sw", "ra");
    }

    // 访问所有基本块
    ret += Visit(func->bbs);
    ret += "\n";
    return ret;
}

// 访问基本块
std::string Visit(const koopa_raw_basic_block_t &bb)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit basic block\n";
#endif
    // 执行一些其他的必要操作
    // 当前块的label, %entry开头的不打印
    if (strncmp(bb->name + strlen(bb->name) - 5, "entry", 5) != 0)
    {
        ret += std::string(bb->name + 1) + ":\n";
    }
    // 访问所有指令
    ret += Visit(bb->insts);
    return ret;
}

// 访问指令
std::string Visit(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    std::string ret = "";
#ifdef DEBUG
    ret += "visit value\n";
#endif
    const auto &kind = value->kind;
    koopa_raw_type_t base;
    switch (kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        ret += Visit(kind.data.integer);
        break;
    case KOOPA_RVT_ALLOC:
        base = value->ty->data.pointer.base;
        if (base->tag == KOOPA_RTT_INT32)
        {
#ifdef DEBUG
            ret += "alloc integer\n";
#endif
            stack.alloc_value(value, stack.pos);
            stack.pos += 4;
        }
        else if (base->tag == KOOPA_RTT_ARRAY)
        {
#ifdef DEBUG
            ret += "alloc array\n";
#endif
            int arrmem = ptr_size_vec.get_value_total_size(value);
            stack.alloc_value(value, stack.pos);
            stack.pos += arrmem;
        }
        else if (base->tag == KOOPA_RTT_POINTER)
        {
#ifdef DEBUG
            ret += "alloc pointer\n";
#endif
            while (base->tag == KOOPA_RTT_POINTER)
            {
                ptr_size_vec.push_size(value, 1);
                base = base->data.array.base;
            }
            while (base->tag == KOOPA_RTT_ARRAY)
            {
                ptr_size_vec.push_size(value, base->data.array.len);
                base = base->data.array.base;
            }
            stack.alloc_value(value, stack.pos);
            stack.pos += 4;
        }
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global alloc 指令
        ret += Visit(value->kind.data.global_alloc, value);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        ret += Visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        ret += Visit(kind.data.store);
        break;
    case KOOPA_RVT_GET_PTR:
        // 访问 getptr 指令
        ret += Visit(kind.data.get_ptr, value);
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        // 访问 getelemptr 指令
        ret += Visit(kind.data.get_elem_ptr, value);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        ret += Visit(kind.data.binary, value);
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 branch 指令
        ret += Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令
        ret += Visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
        // 访问 call 指令
        ret += Visit(kind.data.call, value);
        break;
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        ret += Visit(kind.data.ret);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
        break;
    }
    return ret;
}

// 访问 return 指令
std::string Visit(const koopa_raw_return_t &ret_inst)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit return\n";
#endif
    // 返回值存入 a0
    if (ret_inst.value != nullptr)
    {
        ret += loadstack_reg(ret_inst.value, "a0");
    }
    // 从栈帧中恢复 ra 寄存器
    if (ra_count)
    {
        ret += deal_offset_exceed(stack.len - 4, "lw", "ra");
    }
    // 恢复栈帧
    if (stack.len != 0)
    {
        ret += deal_offset_exceed(stack.len, "addi+", "sp");
    }
    ret += "  ret\n";
    return ret;
}

// 访问 integer 指令
std::string Visit(const koopa_raw_integer_t &integer)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit integer\n";
#endif
    ret += loadint_reg(integer.value, "a0");
    return ret;
}

// 访问 binary 指令
std::string Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit binary\n";
#endif
    // 将运算数存入 t0 和 t1
    ret += loadstack_reg(binary.lhs, "t0");
    ret += loadstack_reg(binary.rhs, "t1");

    // 进行运算
    switch (binary.op)
    {
    case KOOPA_RBO_ADD:
        ret += "  add t0, t0, t1\n";
        break;
    case KOOPA_RBO_SUB:
        ret += "  sub t0, t0, t1\n";
        break;
    case KOOPA_RBO_MUL:
        ret += "  mul t0, t0, t1\n";
        break;
    case KOOPA_RBO_DIV:
        ret += "  div t0, t0, t1\n";
        break;
    case KOOPA_RBO_MOD:
        ret += "  rem t0, t0, t1\n";
        break;
    case KOOPA_RBO_AND:
        ret += "  and t0, t0, t1\n";
        break;
    case KOOPA_RBO_OR:
        ret += "  or t0, t0, t1\n";
        break;
    case KOOPA_RBO_XOR:
        ret += "  xor t0, t0, t1\n";
        break;
    case KOOPA_RBO_SHL:
        ret += "  sll t0, t0, t1\n";
        break;
    case KOOPA_RBO_SHR:
        ret += "  srl t0, t0, t1\n";
        break;
    case KOOPA_RBO_SAR:
        ret += "  sra t0, t0, t1\n";
        break;
    case KOOPA_RBO_EQ:
        ret += "  xor t0, t0, t1\n";
        ret += "  seqz t0, t0\n";
        break;
    case KOOPA_RBO_NOT_EQ:
        ret += "  xor t0, t0, t1\n";
        ret += "  snez t0, t0\n";
        break;
    case KOOPA_RBO_GT:
        ret += "  slt t0, t1, t0\n";
        break;
    case KOOPA_RBO_LT:
        ret += "  slt t0, t0, t1\n";
        break;
    case KOOPA_RBO_GE:
        ret += "  slt t0, t0, t1\n";
        ret += "  xori t0, t0, 1\n";
        break;
    case KOOPA_RBO_LE:
        ret += "  slt t0, t1, t0\n";
        ret += "  xori t0, t0, 1\n";
        break;
    }

    // 将结果存回
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

// 访问 load 指令
std::string Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit load\n";
#endif
    ret += loadstack_reg(load.src, "t0");

    if (value_is_ptr(load.src))
    {
        ret += "  lw t0, 0(t0)\n";
    }

    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec(value, load.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

// 访问 store 指令
std::string Visit(const koopa_raw_store_t &store)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit store\n";
#endif
    switch (store.dest->kind.tag)
    {
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += loadstack_reg(store.value, "t0");
        ret += loadaddr_reg(store.dest, "t1");
        ret += "  sw t0, 0(t1)\n";
        break;
    case KOOPA_RVT_GET_PTR:
    case KOOPA_RVT_GET_ELEM_PTR:
        ret += loadstack_reg(store.value, "t0");
        ret += loadstack_reg(store.dest, "t1");
        ret += "  sw t0, 0(t1)\n";
        break;
    default:
        ret += loadstack_reg(store.value, "t0");
        ret += save_reg(store.dest, "t0");
        break;
    };
    return ret;
}

// 访问 branch 指令
std::string Visit(const koopa_raw_branch_t &branch)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit branch\n";
#endif
    ret += loadstack_reg(branch.cond, "t0");
    ret += "  bnez t0, DOUBLE_JUMP_" + std::string(branch.true_bb->name + 1) + "\n";
    ret += "  j " + std::string(branch.false_bb->name + 1) + "\n";
    ret += "DOUBLE_JUMP_" + std::string(branch.true_bb->name + 1) + ":\n";
    ret += "  j " + std::string(branch.true_bb->name + 1) + "\n";
    return ret;
}

// 访问 jump 指令
std::string Visit(const koopa_raw_jump_t &jump)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit jump\n";
#endif
    ret += "  j " + std::string(jump.target->name + 1) + "\n";
    return ret;
}

// 访问 call 指令
std::string Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit call\n";
#endif
    // 处理参数
    for (size_t i = 0; i < call.args.len; ++i)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (i < 8)
        {
            ret += loadstack_reg(arg, "a" + std::to_string(i));
        }
        else
        {
            ret += loadstack_reg(arg, "t0");
            ret += deal_offset_exceed((i - 8) * 4, "sw", "t0");
        }
    }
    // call half
    ret += "  call " + std::string(call.callee->name + 1) + "\n";
    // 若有返回值则将 a0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "a0");
    }
    return ret;
}

// 访问 global alloc 指令
std::string Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit global alloc\n";
#endif
    ret += "  .data\n";
    ret += "  .globl " + std::string(value->name + 1) + "\n";
    ret += std::string(value->name + 1) + ":\n";
    if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
        // 初始化为 0
        auto base = value->ty->data.pointer.base;
        if (base->tag == KOOPA_RTT_INT32)
        {
            ret += "  .zero 4\n";
        }
        else if (base->tag == KOOPA_RTT_ARRAY)
        {
            int size = 4;
            while (base->tag == KOOPA_RTT_ARRAY)
            {
                ptr_size_vec.push_size(value, base->data.array.len);
                size *= base->data.array.len;
                base = base->data.array.base;
            }
            ret += "  .zero " + std::to_string(size) + "\n";
        }
    }
    if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER)
    {
        ret += "  .word " + std::to_string(global_alloc.init->kind.data.integer.value) + "\n";
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE)
    {
        // 数组，初始化为 Aggregate
        auto base = value->ty->data.pointer.base;
        while (base->tag == KOOPA_RTT_ARRAY)
        {
            ptr_size_vec.push_size(value, base->data.array.len);
            base = base->data.array.base;
        }
        ret += aggregate_init(global_alloc.init);
    }
    ret += "\n";
    return ret;
}

// 访问 getptr 指令
std::string Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit getptr\n";
#endif
    ret += loadaddr_reg(get_ptr.src, "t0");
    ret += "  lw t0, 0(t0)\n";
    int offset = ptr_size_vec.get_value_offset(get_ptr.src);
    ret += loadstack_reg(get_ptr.index, "t1");
    ret += loadint_reg(offset, "t2");
    ret += "  mul t1, t1, t2\n";
    ret += "  add t0, t0, t1\n";

    // 若有返回值则将 t0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec_ptr(value, get_ptr.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

// 访问 getelemptr 指令
std::string Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value)
{
    std::string ret = "";
#ifdef DEBUG
    ret += "visit getelemptr\n";
#endif
    ret += loadaddr_reg(get_elem_ptr.src, "t0");
    if (get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
        get_elem_ptr.src->kind.tag == KOOPA_RVT_GET_PTR)
    {
        ret += "  lw t0, 0(t0)\n";
    }

    int offset = ptr_size_vec.get_value_offset(get_elem_ptr.src);
    ret += loadstack_reg(get_elem_ptr.index, "t1");
    ret += loadint_reg(offset, "t2");
    ret += "  mul t1, t1, t2\n";
    ret += "  add t0, t0, t1\n";

    // 若有返回值则将 t0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec_ptr(value, get_elem_ptr.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

std::string loadstack_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    int index;
    std::string ret = "";
    switch (value->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        ret += loadint_reg(value->kind.data.integer.value, reg);
        break;
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        if (index < 8)
        {
            ret += "  mv " + reg + ", a" + std::to_string(index) + "\n";
        }
        else
        {
            ret += deal_offset_exceed(stack.len + (index - 8) * 4, "lw", reg);
        }
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += "  la t1, " + std::string(value->name + 1) + "\n";
        ret += "  lw " + reg + ", 0(t1)\n";
        break;
    default:
        // 一定保存在栈里, 或者是全局符号
        ret += deal_offset_exceed(stack.get_loc(value), "lw", reg);
        break;
    }
    return ret;
}

// 将 value 的值放置在标号为 reg 的寄存器中
std::string loadint_reg(int value, const std::string &reg)
{
    std::string ret = "";
    ret += "  li " + reg + ", " + std::to_string(value) + "\n";
    return ret;
}

// 将 value 的地址放置在标号为 reg 的寄存器中
std::string loadaddr_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    std::string ret = "";
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        assert(index >= 8);
        ret += loadint_reg(stack.len + (index - 8) * 4, reg);
        ret += "  add " + reg + ", " + reg + ", sp\n";
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += "  la " + reg + ", " + std::string(value->name + 1) + "\n";
        break;
    default:
        ret += deal_offset_exceed(stack.get_loc(value), "addi+", reg);
        break;
    }
    return ret;
}

// 将标号为 reg 的寄存器中的value的值保存在内存中
std::string save_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    std::string ret = "";
    assert(value->kind.tag != KOOPA_RVT_INTEGER);
    int offset;
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        assert(index >= 8);
        ret += deal_offset_exceed(stack.len + (index - 8) * 4, "sw", reg);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += "  la t1, " + std::string(value->name + 1) + "\n";
        ret += "  sw " + reg + ", 0(t1)\n";
        break;
    default:
        offset = stack.get_loc(value);
        ret += deal_offset_exceed(offset, "sw", reg);
        break;
    }
    return ret;
}

// 遍历 agg 并输出为一系列 .word 格式
std::string aggregate_init(const koopa_raw_value_t &value)
{
    std::string ret = "";
    if (value->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 到叶子了
        ret += "  .word " + std::to_string(value->kind.data.integer.value) + "\n";
    }
    else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
    {
        const auto &agg = value->kind.data.aggregate;
        for (int i = 0; i < agg.elems.len; i++)
        {
            ret += aggregate_init(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
        }
    }
    return ret;
}

// value->kind.tag 为 KOOPA_RVT_GET_PTR 或 KOOPA_RVT_GET_ELEM_PTR,
// 或者是 KOOPA_RVT_GET_LOAD 且 load 的是函数开始保存数组参数的位置
bool value_is_ptr(const koopa_raw_value_t &value)
{
    if (value->kind.tag == KOOPA_RVT_GET_PTR)
        return 1;
    if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
        return 1;
    if (value->kind.tag == KOOPA_RVT_LOAD)
    {
        const auto &load = value->kind.data.load;
        if (load.src->kind.tag == KOOPA_RVT_ALLOC)
            if (load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
                return 1;
    }
    return 0;
}

std::string deal_offset_exceed(int offset, std::string inst, std::string reg)
{
    std::string ret = "";
    if (inst == "lw" || inst == "sw")
    {
        if (offset < -2048 || offset > 2047)
        {
            int new_base_offset = offset & ~0x7FF;
            int remaining_offset = offset & 0x7FF;

            if (new_base_offset < -2048 || new_base_offset > 2047)
            {
                ret += "  li t1, " + std::to_string(new_base_offset) + "\n";
                ret += "  add t1, t1, sp\n";
            }
            else
            {
                ret += "  addi t1, sp, " + std::to_string(new_base_offset) + "\n";
            }
            ret += "  " + inst + " " + reg + ", " + std::to_string(remaining_offset) + "(t1)\n";
        }
        else
        {
            ret += "  " + inst + " " + reg + ", " + std::to_string(offset) + "(sp)\n";
        }
    }
    else if (inst == "addi-")
    {
        if (offset < -2048 || offset > 2047)
        {
            ret += "  li t0, " + std::to_string(offset) + "\n";
            ret += "  sub " + reg + ", sp, t0\n";
        }
        else
        {
            ret += "  addi " + reg + ", sp, -" + std::to_string(offset) + "\n";
        }
    }
    else if (inst == "addi+")
    {
        if (offset < -2048 || offset > 2047)
        {
            ret += "  li t0, " + std::to_string(offset) + "\n";
            ret += "  add " + reg + ", sp, t0\n";
        }
        else
        {
            ret += "  addi " + reg + ", sp, " + std::to_string(offset) + "\n";
        }
    }
    return ret;
}

// 函数：检查给定行是否是存储指令（sw）
bool is_sw(const std::string &line)
{
    return line.find("sw") == 0;
}

// 函数：检查给定行是否是加载指令（lw）
bool is_lw(const std::string &line)
{
    return line.find("lw") == 0;
}

// 函数：处理并优化 RISC-V 汇编代码
std::string optimize_riscv_code(const std::string &riscv_code)
{
    std::stringstream result;
    std::istringstream code_stream(riscv_code);
    std::string line, prev_sw;
    bool sw_detected = false;

    // 逐行处理汇编代码
    while (std::getline(code_stream, line))
    {
        std::string code_part = line;
        size_t comment_pos = line.find('#');
        std::string comment_part = "";
        if (comment_pos != std::string::npos)
        {
            code_part = line.substr(0, comment_pos); // 获取指令部分
            comment_part = line.substr(comment_pos); // 获取注释部分
        }

        // 去除空格并保留原有格式
        std::string trimmed_code_part = code_part;
        trimmed_code_part.erase(std::remove_if(trimmed_code_part.begin(), trimmed_code_part.end(), ::isspace), trimmed_code_part.end());

        // 忽略空行
        if (trimmed_code_part.empty())
        {
            result << line << std::endl;
            continue;
        }

        // 如果是存储指令 (sw)，记录当前存储的寄存器
        if (is_sw(trimmed_code_part))
        {
            prev_sw = trimmed_code_part;
            // prev_lw.clear(); // 清除之前的 lw
            sw_detected = true;
        }
        // 如果是加载指令 (lw)，尝试进行优化
        else if (is_lw(trimmed_code_part))
        {
            if (sw_detected)
            {
                std::string lw_instr, lw_dst, lw_addr, sw_instr, sw_src, sw_addr;
                size_t comma_pos;
                lw_instr = trimmed_code_part.substr(0, 2); // "lw"

                // 查找寄存器和偏移部分的分隔符 ','
                comma_pos = trimmed_code_part.find(',');

                // 提取寄存器部分（从第3个字符开始，到逗号位置）
                lw_dst = trimmed_code_part.substr(2, comma_pos - 2); // "t0"

                // 提取偏移部分（从逗号后开始）
                lw_addr = trimmed_code_part.substr(comma_pos + 1); // "4(sp)"
                // 判断 lw 载入的地址与前一个 sw 存储的地址是否一致，且寄存器一致

                sw_instr = prev_sw.substr(0, 2); // "lw"

                // 查找寄存器和偏移部分的分隔符 ','
                comma_pos = prev_sw.find(',');

                // 提取寄存器部分（从第3个字符开始，到逗号位置）
                sw_src = prev_sw.substr(2, comma_pos - 2); // "t0"

                // 提取偏移部分（从逗号后开始）
                sw_addr = prev_sw.substr(comma_pos + 1); // "4(sp)"

                if (lw_dst != "" && sw_src != "" && lw_addr == sw_addr)
                {
                    // 使用 mv 替代 lw，避免冗余的内存加载
                    if (lw_dst != sw_src)
                    {
                        result << "  mv " << lw_dst << ", " << sw_src << comment_part << std::endl;
                    }
                    sw_detected = false; // 重置标志位
                    continue;            // 跳过当前的 lw 指令
                }
            }
            sw_detected = false; // 重置标志位
        }
        else
        {
            sw_detected = false;
        }
        result << line << std::endl;
    }

    return result.str();
}