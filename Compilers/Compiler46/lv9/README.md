# 编译原理课程实践报告：质量好的编译器

信息科学技术学院 2200013214 张立恒

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：

1. 前端：通过词法分析、语法分析和中间代码生成等技术，生成文本形式的Koopa IR中间代码。
2. 后端：通过DFS遍历内存形式的Koopa IR，生成RISC-V目标代码。
3. 优化：*TODO*

### 1.2 主要特点

本编译器的主要特点是代码风格良好、前后端解耦。

- 代码风格良好：使用C++17标准，除少量Yacc代码外，均使用智能指针、C++风格类型转换等特性，以保证内存安全；各文件、类、函数的功能划分合理，注释详细，使用doxigen规范，可以自动生成文档，可读性和可维护性好。
- 前后端解耦：编译器前端和后端代码完全分离，后端仅使用前端输出的Koopa IR。

## 二、编译器设计

### 2.1 主要模块组成

编译器由5个主要模块组成：

- 词法分析部分 `sysy.l`负责将SysY源程序转换为token流
- 语法分析部分 `sysy.y`负责用token流建立语法分析树
- 符号表部分 `symtab.hpp`负责记录源程序符号信息，如与Koopa IR符号的对应关系
- 语义分析和中间代码生成部分 `ast.hpp, ast.cpp`负责遍历语法分析树，维护符号表，生成Koopa IR
- 目标代码生成部分 `riscv.hpp, riscv.cpp`负责用libkoopa将文本形式的Koopa IR转换为内存形式，通过DFS遍历内存形式的Koopa IR，生成RISC-V目标代码。

### 2.2 主要数据结构

本编译器最核心的数据结构是 `ast.hpp`中定义的各AST节点类。在实现过程中，设计了 `BaseAST`作为基类：

```cpp
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    /**
     * @brief   语义分析，生成Koopa IR，维护符号表
     *
     * 1. 生成Koopa IR，即先以递归形式后序遍历AST树，调用成员AST的IR方法，
     * 然后生成本节点的Koopa IR
     * 2. 把对应的Koopa IR符号插入符号表
     */
    virtual void IR() = 0;
};
```

为所有Exp相关的AST设计了基类 `ExpBaseAST`，为了方便其它AST访问本AST的Koopa IR符号以进行运算，设计成员变量symbol和is_const成员变量：

```cpp
class ExpBaseAST : public BaseAST
{
public:
    // AST对应的Koopa IR符号，若能求值，则为其值，若不能，则为符号
    std::string symbol;
    // 是否为常量
    bool is_const;
};
```

这些AST的 `IR`方法会设置其 `symbol`和 `is_const`成员变量。

**symbol的含义**

| AST类型 | symbol的类型                      | e.g. SysY        | Koopa IR             | symbol |
| ------- | --------------------------------- | ---------------- | -------------------- | ------ |
| 常量    | 常量的值                          | const int a = 0; |                      | 0      |
| 变量    | 指向变量的指针                    | int a;           | @a = alloc i32       | @a     |
| LVal    | 值，另用loc_sym存放指向变量的指针 | a = 10;          | store 10, @a         | 10     |
| 数组    | 数组指针                          | int a[2];        | @a = alloc [i32, 2]  | @a     |
| 指针    | 指向数组指针的指针                | int a[][2];      | @a = alloc *[i32, 2] | @a     |

AST类型的概念是为了描述方便，编译器中并不存在这样的数据结构，一个相近的数据结构是下文的 `SymbolInfo::tag`。根据AST对应的SysY符号，一种AST可能对应不同的AST类型，例如，当PrimaryExpAST对应SysY中的常量时，其成员变量 `symbol`为常量的值，而当其对应SysY中的变量时，`symbol`为`load`语句的结果。这样，上级AST就可以用该symbol直接输出Koopa IR，如`std::cout << "  " << symbol << " = sub 0, " << unary_exp->symbol << std::endl;`。

**symbol的命名**

- 为避免符号重名，维护全局递增计数器sym_cnt，每个symbol符号都命名为 `(@|%)[SysY变量符号_]<序号>`的形式，如 `@a_0`, `%1`等。
- `main`函数和库函数除外，因为Koopa规范规定 `main`函数的Koopa IR符号必须是main。
- 容易证明所有符号都不会重名，而采用分类计数等方法均有可能重名。

`if...else...`语句涉及到二义性问题，因此，本编译器使用Yacc的 `%prec`标记，以使 `else`和最近未匹配的 `if`匹配：

```cpp
//规定if-else匹配优先级，以解决空悬else问题
%precedence IFX
%precedence ELSE
```

*TODO*

除此之外，为了代码编写的便利性，在编译器的前端和后端部分都各有一个 `GlobalInfo`的数据结构。设计 `global_info`的目的是……

```rust
pub struct GlobalInfo<'p> {
……
}
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑

以symtab.hpp中的代码来说明符号表的设计：

```cpp
/**
 * 符号表symbol table.
 * 符号表符号的定义与ast中的Koopa IR符号symbol不同.
 * Koopa IR符号包括源程序中的常量对应的符号（以数字表示）、
 * 变量对应的符号（以@<变量符号>_<序号>表示），
 * 以及为表达式生成的、以"%"开头的符号. 
 * 符号表符号只包括源程序中的变量和常量对应的符号.
 */

enum class SymbolTag
{
    CONST,
    VAR,
    ARRAY,
    PTR,
    VOID, // 无返回值的函数
    INT   // 返回类型为int的函数
};

class SymbolInfo
{
public:
    SymbolTag tag;
    std::string symbol;    // koopa IR符号
    std::vector<int> dims; // 数组或数组指针的维数
    // 成员函数...
};

// 一个作用域有一个作用域符号表ScopeSymbolTable
class ScopeSymbolTable
{
public:
    std::unordered_map<std::string, std::shared_ptr<SymbolInfo>> scope_tab;
    // 成员函数...
}

// 不同作用域的ScopeSymbolTable栈式进出
class SymbolTable
{
public:
    std::deque<std::unique_ptr<ScopeSymbolTable>> table;
    // 成员函数...
}
```

**符号的管理**

用下表来分析符号表tag与符号表symbol内容的对应关系：

| SymbolInfo::tag | SymbolInfo::symbol的类型 | e.g. SysY        | Koopa IR             | symbol |
| --------------- | ------------------------ | ---------------- | -------------------- | ------ |
| CONST           | 常量值                   | const int a = 0; |                      | 0      |
| VAR             | 指针                     | int a;           | @a = alloc i32       | @a     |
| ARRAY           | 数组指针                 | int a[2];        | @a = alloc [i32, 2]  | @a     |
| PTR             | 指向数组指针的指针       | int a[][2]       | @a = alloc *[i32, 2] | @a     |
| VOID            | 函数名                   | void foo()       | fun @foo()           | @foo   |
| INT             | 函数名                   | int main()       | fun main()           | main   |

**变量作用域的管理**

1. 在SymbolTable的构造函数插入全局符号表：

```cpp
SymbolTable()
    {
        table.emplace_back(std::make_unique<ScopeSymbolTable>());
        // 其它操作...
    }
```

2. 进入/退出一个作用域时，调用push/pop函数：

```cpp
// 插入一个作用域符号表
void push()
{
    table.emplace_back(std::make_unique<ScopeSymbolTable>());
}

// 退出一个作用域符号表
void pop()
{
    table.pop_back();
}
```

3. 插入符号时，向最上层作用域符号表插入符号：

```cpp
/**
 * @brief 向符号表中添加一个符号, 同时记录这个符号的值
 *
 * @param ident     SysY标识符
 * @param tag       符号类型
 * @param symbol    Koopa IR符号
 */
void insert(const std::string &ident,
            const SymbolTag tag,
            const std::string &symbol,
            const std::vector<int> &dims = {})
{
    assert(!(table.back()->contains(ident)));
    table.back()->insert(ident, tag, symbol, dims);
}
```

4. 查找符号时，从最上层作用域符号表开始查找：

```cpp
/**
 * @brief 给定一个符号表中已经存在的符号, 返回这个符号对应的SymbolInfo
 *
 * @param ident SysY标识符
 * @return std::shared_ptr<SymbolInfo>
 */
std::shared_ptr<SymbolInfo> operator[](const std::string &ident) const
{
    for (auto it = table.rbegin(); it != table.rend(); ++it)
    {
        if ((*it)->contains(ident))
        {
            return (**it)[ident];
        }
    }
    assert(false);
}
```

这样我们就实现了变量的作用域管理。

#### 2.3.2 寄存器分配策略

谈谈如何完成寄存器分配的，没分配（扔栈上也给个说法）也谈谈。不超过200字。

#### 2.3.3 采用的优化策略

谈谈做了哪些优化，不超过200字，没有的话就不用写。

#### 2.3.4 其它补充设计考虑

## 三、编译器实现

### 3.1 各阶段编码细节

介绍各阶段实现的时候自己认为有价值的信息，本部分内容**不做特别要求和限制**。可按如下所示的各阶段组织。

#### Lv1. main函数和Lv2. 初试目标代码生成

- 需要注意Flex和Bison的语法规则，如 `%union`中的 `str_val`, `int_val`, `ast_val`等将出现在 `%type`标识后，声明返回值的类型。
- 需要阅读koopa.h头文件，熟悉各数据结构的定义。

#### Lv3. 表达式

注意语法规范中已经蕴含了优先级，高优先级运算结果在语法树中更靠近叶子，ast递归调用 `IR`时自然会先处理高优先级运算的Koopa IR，无需额外处理。

#### Lv4. 常量和变量

需要实现符号表

解释symbol

#### Lv5. 语句块和作用域

需要实现作用域符号表，使其在符号表中栈式进出

#### Lv6. if语句

**if/else二义性**

`if...else...`语句涉及到二义性问题，因此，本编译器使用Yacc的 `%prec`标记，以使 `else`和最近未匹配的 `if`匹配：

```cpp
//规定if-else匹配优先级，以解决空悬else问题
%precedence IFX
%precedence ELSE
```

**短路求值**

以LOrExpAST为例，代码注释说明了生成中间代码的思路，LAndExpAST完全类似：

```cpp
void LOrExpAST::IR()
{
    dbg_printf("in LOrExpAST\n");
    if (tag == Tag::LAND)
    {
        land_exp->IR();
        is_const = land_exp->is_const;
        symbol = land_exp->symbol;
    }
    else
    {
        /**
         * 本质上做了这个操作:
         * int result = 1;
         * if (lhs == 0)
         * {
         *      result = rhs != 0;
         * }
         * 表达式的结果即是 result
         */
        lor_exp->IR();
        if (lor_exp->is_const)
        {
            bool lor_true = atoi(lor_exp->symbol.c_str());
            if (lor_true)
            {
                is_const = true;
                symbol = "1";
            }
            else
            {
                land_exp->IR();
                is_const = land_exp->is_const;
                if (is_const)
                {
                    bool land_true = atoi(land_exp->symbol.c_str());
                    symbol = std::to_string(land_true);
                }
                else
                {
                    symbol = "%" + std::to_string(sym_cnt++);
                    std::cout << "  " << symbol << " = ne "
                              << land_exp->symbol << ", 0" << std::endl;
                }
            }
        }
        else
        {
            is_const = false;
            auto cur_sym_cnt = sym_cnt++;
            auto res_sym = "%lor_res_" + std::to_string(sym_cnt++);
            sym_tab.insert(res_sym, SymbolTag::VAR, res_sym);
            std::cout << "  " << res_sym << " = alloc i32" << std::endl;
            std::cout << "  store 1, " << res_sym << std::endl;
            std::cout << "  br " << lor_exp->symbol
                      << ", %lor_end_" << cur_sym_cnt
                      << ", %left_false_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
            std::cout << "%left_false_" << cur_sym_cnt << ":" << std::endl;
            land_exp->IR();
            std::cout << "  store " << land_exp->symbol
                      << ", " << res_sym << std::endl;
            std::cout << "  jump %lor_end_" << cur_sym_cnt << std::endl;
            std::cout << std::endl;
            std::cout << "%lor_end_" << cur_sym_cnt << ":" << std::endl;
            symbol = "%" + std::to_string(sym_cnt++);
            std::cout << "  " << symbol << " = load "
                      << res_sym << std::endl;
        }
    }
}
```

#### Lv7. while语句

在StmtAST中维护 `whlie_cnt_stk`，保存当前 `while`语句的序号，以使 `break`和 `continue`语句能够跳转到对应的 `label`.

维护has_jp以确保基本块和跳转语句一一对应 *TODO*

#### Lv8. 函数和全局变量

函数的目标代码生成，栈

全局变量

*TODO*

#### Lv9. 数组

**多维数组初始化**

在 `ConstDefAST`中定义一系列成员函数以实现数组初始化：

```cpp
/**
 * @brief 进到待编译程序给出的初始化列表的一个大括号里，填充初始化列表
 *
 * @param init_vals         待编译程序给出的初始化列表的一个大括号对应的AST数组
 * @param full_init_vals    待填充的初始化列表
 * @param is_first          是否是第一个大括号
 * @return int              该大括号负责初始化的长度
 */
int fill_init_vals(const std::vector<std::unique_ptr<ConstInitValAST>>
                        &init_vals,
                    std::vector<std::string> &full_init_vals, bool is_first = false);

/**
 * @brief 当前已填充长度的对齐值，即当前大括号负责初始化的长度
 *
 * @param len       当前已填充长度
 * @return int      对齐值
 */
int aligned_len(int len);

/**
 * @brief 生成取数组指针和存初始值的Koopa IR
 *
 * @param full_init_vals    初始化列表
 * @param symbol            指针的Koopa IR符号
 * @param dim               当前维度
 */
void get_ptr_store_val(const std::vector<std::string> &full_init_vals,
                        const std::string &symbol, int dim);

/**
 * @brief 输出当前大括号的初始化列表
 *
 * @param full_init_vals    初始化列表
 * @param dim               当前维度
 */
void print_aggr(const std::vector<std::string> &full_init_vals, int dim);
```

生成中间代码时，`IR`函数会调用 `fill_init_vals`得到完整的初始化列表，接着调用 `print_aggr`实现 `alloc`语句，最后调用 `get_ptr_store_val`实现 `getelemptr`和 `store`等语句，完成数组初始化的Koopa IR代码。

**数组参数**

数组参数部分的困难很大程度上是由于SysY和Koopa IR对应符号的语义存在mismatch，由此产生一些细微之处需要特别处理，而文档中的简单示例不足以使我们注意到这点。通过分析，总结以下两条规则：

1. 阅读文档可知，与数组不同，SysY中的指针第一次解引用，对应的Koopa IR需要使用 `getptr`，而不是 `getelemptr`。例如，`a[][2]`用 `a[1]`解引用时，对应的Koopa IR应该使用 `<next_symbol> = getptr <current_symbol>, 1`而不是 `<next_symbol> = getelemptr <current_symbol>, 1`。
2. 显然，遇到解引用时，需要按照下标顺序，逐步使用 `<next_symbol> = getelemptr <current_symbol>, <index>`解引用，直到解引用完毕，得到一个值或指针。类似：

```cpp
auto ptr_sym = sym_info->symbol;
for (auto &exp : exps)
{
    auto next_sym = "%ptr_" + std::to_string(sym_cnt++);
    std::cout << "  " << next_sym << " = getelemptr " << ptr_sym << ", " << exp->symbol << std::endl;
    ptr_sym = next_sym;
}
```

但是，接着还需要根据数组和解引用的次数进一步处理，增加相应的Koopa IR语句。例如，`int a[][2]`用a[1]解引用时（会作为 `b[]`形参的实参），对应的Koopa IR得到一个 `*[i32, 2]`类型的符号，此时还需要加一个 `<next_symbol> = getelemptr <current_symbol>, 0`语句，得到 `*i32`类型的符号（以与 `b[]`的Koopa IR符号类型相匹配）。

分析数组和指针被解引用时的用途和需要添加的语句，如下表所示：

|            | 数组                   | 指针                   |
| ---------- | ---------------------- | ---------------------- |
| 不解引用   | 用作参数，加getelemptr | 用作参数               |
| 部分解引用 | 用作参数，加getelemptr | 用作参数，加getelemptr |
| 完全解引用 | 加load                 | 加load                 |

完成这样的分析后，代码只是按照表格分类讨论而已。

### 3.2 工具软件介绍（若未使用特殊软件或库，则本部分可略过）

1. `flex/bison`：词法分析和语法分析；
2. `libkoopa`：将文本形式的Koopa IR转换为内存形式的Koopa IR。

### 3.3 测试情况说明（如果进行过额外的测试，可增加此部分内容）

简述如何构造用例，测出过哪些不一样的错误，怎么发现和解决的。为课程提供优质测试用例会获得bonus。

## 四、实习总结

请至少谈1点，多谈谈更好。有机会获得奶茶或咖啡一杯。可以考虑按下面的几点讨论。

### 4.1 收获和体会

### 4.2 学习过程中的难点，以及对实习过程和内容的建议

### 4.3 对老师讲解内容与方式的建议
