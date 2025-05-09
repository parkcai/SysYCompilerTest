# 编译原理课程实践报告：StupidSysY2RV

## 一、编译器概述

### 1.1 基本功能

本编译器具备如下基本功能：
1. 将[SysY](https://pku-minic.github.io/online-doc/#/misc-app-ref/sysy-spec)语言编译为[Koopa IR](https://pku-minic.github.io/online-doc/#/misc-app-ref/koopa)形式
1. 将[SysY](https://pku-minic.github.io/online-doc/#/misc-app-ref/sysy-spec)语言编译为RISC-V代码
1. 通过自身崩溃检查代码错误

### 1.2 主要特点

包含了基本要求的功能，包括表达式编译、常量变量、语句块作用域、if, while语句、函数调用、全局变量以及多维数组、数组参数的实现。

## 二、编译器设计

整体设计：

1. 使用词法、语法分析器分析SysY代码文件，构建抽象语法树AST；
2. 调用根节点AST的函数`GenerateIR_void`，通过AST生成Koopa IR结构的内存形式；
3. 将Koopa IR结构输出为代码，重新读取后，再遍历Koopa IR结构的内存形式，生成RISC-V代码。

### 2.1 主要模块组成

该编译器由三个模块组成：

1. 分析模块：分别进行词法分析和语法分析，构建抽象语法树(AST)。```sysy.l``` ```sysy.y```。
2. AST模块：负责实现AST结构，并使用该结构生成Koopa结构。```ast.cpp``` ```ast.h```。
3. Visit模块：负责读取Koopa结构，生成RISC-V代码。`visit.cpp` `visit.h`。


### 2.2 主要数据结构

#### 2.2.1 抽象语法树

抽象语法树节点均继承自```BaseAST```类，该类规定了一些可以实现的功能，方便使用多态调用。

```c++
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual void *GetLeftValue() const { return nullptr; };
    virtual std::int32_t CalculateValue() const { return 0; };
    virtual void GenerateGlobalValues(std::vector<const void *> &vec) const { return; };
    virtual void ArrayInit(std::vector<const void *> &init_vec, std::vector<size_t> size_vec) const { return; };
    virtual void GenerateGlobalValues(std::vector<const void *> &values, koopa_raw_type_tag_t tag) const { return; };
    virtual void *GenerateIR_ret() const { return nullptr; };
    virtual void *GenerateIR_ret(std::vector<const void *> &init_vec, std::vector<size_t> size_vec, int level) const { return nullptr; };
    virtual void GenerateIR_void() const { return; };
    virtual void GenerateIR_void(koopa_raw_type_tag_t tag) const { return; };
    virtual void GenerateIR_void(std::vector<const void *> &funcs, std::vector<const void *> &values) const { return; };
    virtual void GenerateIR_void(std::vector<const void *> &funcs) const { return; };
};
```

另外定义了三个类，并分别声明其的静态实例以管理相应结构。

```c++
static SymbolTable symbol_table;
static BlockList block_list;
static LoopStack loop_stack;
```

#### 2.2.2 符号表

符号表类存入的数据可分为五类：变量、常亮、函数、数组及数组指针。符号表在每个基本块开始时会新建一个符号表加入符号表栈中，基本块结束时则会pop栈中的符号表。访问变量时从栈顶依次往里访问，若未找到变量说明代码出现错误并报错。

```c++
class SymbolTable
{
public:
    class Value
    {
    public:
        enum ValueType
        {
            Var,
            Const,
            Func,
            Array,
            Pointer
        } type;
        union Data
        {
            int const_value;
            koopa_raw_value_t var_value;
            koopa_raw_function_t func_value;
            koopa_raw_value_t array_value;
            koopa_raw_value_t pointer_value;
        } data;
        Value() = default;
        Value(ValueType type, int value) : type(type)
        {
            assert(type == Const);
            data.const_value = value;
        };
        Value(ValueType type, koopa_raw_value_t value) : type(type)
        {
            if (type == Var)
            {
                data.var_value = value;
            }
            else if (type == Array)
            {
                data.array_value = value;
            }
            else if (type == Pointer)
            {
                data.pointer_value = value;
            }
        };
        Value(ValueType type, koopa_raw_function_t value) : type(type)
        {
            assert(type == Func);
            data.func_value = value;
        };
    };

    void add_symbol(std::string name, SymbolTable::Value value);
    Value get_value(std::string name);
    void add_table();
    void del_table();

private:
    std::vector<std::unordered_map<std::string, SymbolTable::Value>> symbol_table_stack;
};
```

#### 2.2.3 基本块列表

在```BlockList```中，需要存储当前正在构建的函数、基本块、指令缓冲区等Koopa结构信息。这可以方便我们对基本块中的指令进行统一管理和整理。

```c++
class BlockList
{
private:
    std::vector<const void *> block_list;
    std::vector<const void *> tmp_inst_buf;
    std::string func_name;

public:
    void init(std::string ident);
    const char *generate_block_name(std::string ident);
    void add_block(koopa_raw_basic_block_data_t *block);
    void add_inst(const void *inst);
    void push_tmp_inst();
    bool check_return();
    void rearrange_block_list();
    std::vector<const void *> get_block_list();
};
```

#### 2.2.4 循环列表

该类使用一个vector来构建一个栈，用于表示嵌套的循环结构。这样的处理可以清晰地实现continue和break语句。

```c++
class LoopStack
{
private:
    class LoopBlocks
    {
    public:
        koopa_raw_basic_block_data_t *cond_block;
        koopa_raw_basic_block_data_t *end_block;
        LoopBlocks(koopa_raw_basic_block_data_t *cond_block, koopa_raw_basic_block_data_t *end_block)
        {
            this->cond_block = cond_block;
            this->end_block = end_block;
        }
    };
    std::vector<LoopBlocks> loop_stack;

public:
    void add_loop(koopa_raw_basic_block_data_t *cond, koopa_raw_basic_block_data_t *end);
    koopa_raw_basic_block_data_t *get_cond_block();
    koopa_raw_basic_block_data_t *get_end_block();
    void del_loop();
    bool is_inside_loop();
};
```

#### 2.2.5 Visit中的栈管理

在`Visit.cpp`和`Visit.h`中，`Stack`类用于存储Koopa指令到int的映射，管理该指令的计算结果存储在栈上的位置。

```c++
class Stack
{
public:
    int len;
    int pos;

    Stack()
    {
        len = 0;
        pos = 0;
    }
    void alloc_value(koopa_raw_value_t value, int loc);
    int get_loc(koopa_raw_value_t value);
    void init();

private:
    std::unordered_map<koopa_raw_value_t, int> value_loc;
};

static Stack stack;
```

#### 2.2.6 Visit中的数组大小存储

`PtrSizeVec`类用于存储数组的大小。这样在访问数组的时候可以通过该数据结构得到数组的偏移等信息。

```c++
class PtrSizeVec
{
public:
    int get_value_offset(koopa_raw_value_t src);
    int get_value_total_size(koopa_raw_value_t value);
    void push_size(koopa_raw_value_t value, int size);
    void copy_size_vec(koopa_raw_value_t dest, koopa_raw_value_t src);
    void copy_size_vec_ptr(koopa_raw_value_t dest, koopa_raw_value_t src);

private:
    std::unordered_map<koopa_raw_value_t, std::vector<int>> size_vec;
};

static PtrSizeVec ptr_size_vec;
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的栈式设计

符号表类里面定义了一个vector实现的栈，表示作用域的嵌套结构，新建作用域时在栈顶添加元素。栈中每个元素为一个map，将字符串映射到Koopa结构，表示该作用域的符号表。当需要查找符号时，从栈顶开始在map中查找，若没有找到，则进入栈中前一个元素继续查找，实现嵌套作用域的查找。

```c++
std::vector<std::unordered_map<std::string, SymbolTable::Value>> symbol_table_stack;
```

#### 2.3.2 寄存器分配策略
寄存器分配采用把变量全部放在栈上的策略。首先Visit函数时计算所有语句以及变量等占用的空间大小，在栈上分配足够的空间。每个语句确保自己执行的内部寄存器不会冲突，执行完毕将语句的结构存在栈上指定位置，寄存器信息不再保存，可被其他语句复用。

#### 2.3.3 koopa内存形式结构生成简化

为了优化koopa形式内存结构的生成过程，通过整理utils函数来简化。

```c
const char *generate_var_name(std::string ident);
koopa_raw_slice_t generate_slice(koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(std::vector<const void *> &vec,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(const void *data,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag);
koopa_raw_type_t generate_type_pointer(koopa_raw_type_t base);
koopa_raw_type_t generate_type_array(koopa_raw_type_t base, size_t size);
koopa_raw_type_t generate_type_func(koopa_raw_type_t func_type, koopa_raw_slice_t params);
koopa_raw_type_t generate_linked_list_type(koopa_raw_type_t base, std::vector<size_t> size_vec);
koopa_raw_value_data_t *generate_number(int32_t number);
koopa_raw_value_data_t *generate_zero_init(koopa_raw_type_t type);
koopa_raw_value_data_t *generate_aggregate(koopa_raw_type_t type, koopa_raw_slice_t elements);
koopa_raw_value_data_t *generate_func_arg_ref(koopa_raw_type_t ty, std::string ident);
koopa_raw_function_data_t *generate_function_decl(std::string ident, std::vector<const void *> &params_ty, koopa_raw_type_t func_type);
koopa_raw_function_data_t *generate_function(std::string ident, std::vector<const void *> &params, koopa_raw_type_t func_type);
koopa_raw_basic_block_data_t *generate_block(std::string name);
koopa_raw_value_data_t *generate_global_alloc(std::string ident, koopa_raw_value_t value, koopa_raw_type_t tag);
koopa_raw_value_data_t *generate_alloc_inst(std::string ident, koopa_raw_type_t base);
koopa_raw_value_data_t *generate_getelemptr_inst(koopa_raw_value_t src, koopa_raw_value_t index);
koopa_raw_value_data_t *generate_getptr_inst(koopa_raw_value_t src, koopa_raw_value_t index);
koopa_raw_value_data_t *generate_store_inst(koopa_raw_value_t dest, koopa_raw_value_t value);
koopa_raw_value_data_t *generate_load_inst(koopa_raw_value_t src);
koopa_raw_value_data_t *generate_binary_inst(koopa_raw_value_t lhs, koopa_raw_value_t rhs, koopa_raw_binary_op_t op);
koopa_raw_value_data_t *generate_return_inst(koopa_raw_value_t value);
koopa_raw_value_data_t *generate_jump_inst(koopa_raw_basic_block_data_t *target);
koopa_raw_value_data_t *generate_branch_inst(koopa_raw_value_t cond, koopa_raw_basic_block_data_t *true_bb, koopa_raw_basic_block_data_t *false_bb);
koopa_raw_value_data_t *generate_call_inst(koopa_raw_function_t func, std::vector<const void *> &args);
```

这样的设计会使koopa形式内存结构的生成极大地简化，使得代码清晰易读。

#### 2.3.4 riscv代码优化

实现了针对lw和sw指令重复调用的窥孔优化，即如果刚刚执行完的sw指令和接下来要执行的lw指令的目标栈地址一样，就可以省去lw指令的调用。

```c++
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
```

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成

简单地按照文档完成简易功能。构建了AST的类结构，选择了直接生成Koopa结构内存形式的方式。

#### Lv3. 表达式

### 3.1 各阶段编码细节

[lab文档](https://pku-minic.github.io/online-doc/#/)

#### Lv1. main函数和Lv2. 初试目标代码生成

简单地按照文档完成简易功能。构建了AST的类结构，选择了直接生成Koopa结构内存形式的方式。

使用简单函数生成RISC-V，没有构建其他复杂的数据结构。

#### Lv3. 表达式
按照文档的语法规范，对每种表达式设计AST类。

由于Koopa没有逻辑与/或，只有按位与/或，所以这两个运算之前，添加不等于运算来将结果转化为1和0，再进行按位运算。

#### Lv4. 常量和变量
对于Const，在表达式的AST中添加了CalculateValue函数用于求值。

因为变量的出现，利用SymbolTable类进行全局的符号管理。

在BaseAST中添加了新的虚函数to_left_value用于处理LValAST的左值。

在栈空间计算以及koopa与地址映射中添加了对变量的处理。

#### Lv5. 语句块和作用域
修改SymbolTable类为栈结构使其支持实现嵌套作用域。

#### Lv6. if语句
出现二义性时,首先通过把If语句拆开，再加上分析器的默认优先级解决了问题。

```
// Stmt          :: = LVal "=" Exp ";"
//                    | [Exp] ";"
//                    | Block
//                    | IfExp ["else" Stmt]
//                    | WhileExp
//                    | "return" [Exp] ";";

// IfExp         ::= "if" "(" Exp ")" Stmt;
```

由于基本块越来越多，设计了BlockList来更直接地管理基本块。

对于短路求值，直接将&&，||改为跳转形式，而不再使用按位与等运算。

#### Lv7. while语句
设计了LoopList类来维护嵌套的循环，使用栈结构来存储每个循环的基本块信息。

这样break，continue能找到当前所在的循环，从而找到跳转位置。

#### Lv8. 函数和全局变量
在函数定义FuncDefAST中读取列表构建AST。

在Visit阶段，计算函数返回地址，传参在栈上存储的位置，以及调用函数将参数放入指定寄存器或栈上位置。

#### Lv9. 数组
ConstInitValAST和InitValAST首先调用预处理ArrayInit，将初始化数据预处理成易读的数组格式。

然后根据是全局还是局部变量，生成aggerate的koopa内容或者读取某一位的值，在函数内部通过赋值初始化。

在Visit中，对get_element_ptr和get_ptr进行特判处理。针对get_element_ptr等语句，处理时会读取koopa对象中的ty以计算指针需要移动的大小，使用mul，add等语句计算得到指定位置的指针。

### 3.2 工具软件介绍
1. `flex/bison`：编写```sysy.l```和```sysy.y```，完成词法分析和语法分析，通过表达式识别代码生成了语法分析树。
2. [libkoopa](https://github.com/pku-minic/koopa)：使用该库的Koopa各种结构体，直接在AST中构建了Koopa结构的内存形式。并使用该库生成Koopa代码。
3. VSCode：代码编写。

### 3.3 测试情况说明

测试情况良好,基本AC。

## 四、实习总结

### 4.1 收获和体会

通过对编译器的实现对课程中的内容有了更深入的理解和感悟,在敲代码的过程中也学习到有关大程序开发的部分困难之处。

### 4.2 学习过程中的难点，以及对实习过程和内容的建议

参考以往课程,希望在上课过程中添加对lab的讲解部分。

### 4.3 对老师讲解内容与方式的建议

没有什么意见。