# 编译原理课程实践报告：高效的 Sysy 编译器

数学科学学院 2200010848 陈润璘

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 生成(并不高效的) IR
2. 生成(高效的) RISC-V 汇编代码
3. 提供一些简单的错误提示

### 1.2 主要特点

本编译器的主要特点是 **使用 Rust 编程语言实现**、**分离编译器的前端和后端**。

## 二、编译器设计

### 2.1 主要模块组成

编译器由 2 个 crate，3 个主要部分组成。Crate `compiler_macro` 用于编译期的代码生成，自动生成汇编指令结构体到字符串的转换函数，以减少重复代码。Crate `compiler` 是编译器的主体部分，包含前端和后端。前端负责词法分析、语法分析、语义分析，生成 IR；后端负责将 IR 转换为 RISC-V 汇编代码。

### 2.2 主要数据结构

首先，我们设计了一些数据结构来表示不同的语法单元。例如，`CompUnit`表示一个完整的程序单元，`FuncDef`表示一个函数定义，`Stmt`表示一个语句等等。Lalrpop 生成的语法分析器把源代码解析成这些数据结构。随后，我们根据这些数据结构生成 IR。一个简单的语法树节点如下：

```rust
pub enum GlobalItem {
    Decl(Decl),
    FuncDef(Rc<FuncDef>),
}
```

为了正确地处理作用域，我们在语法分析时对每个作用域自动编号。同时，为了在语法分析阶段报告错误，我们需要一些有关源代码的元数据。为了方便期间，我们把这些信息都放在一个 `ParserContext` 结构体中：

```rust
#[derive(Clone)]
pub struct BlockIdGenerator {
    max_id: i32,
    id_stack: Vec<i32>,
}

impl BlockIdGenerator {
    pub fn new() -> Self {
        ...
    }

    pub fn generate(&mut self) -> i32 {
        ...
    }

    pub fn get_current_id(&self) -> i32 {
        self.id_stack[self.id_stack.len() - 1]
    }

    pub fn pop(&mut self) {
        self.id_stack.pop();
    }
}

#[derive(Clone)]
pub struct ParserContext<'a> {
    pub generator: BlockIdGenerator,
    pub file_path: &'a str,
    pub input: &'a str,
}
```

在生成 IR 时，我们遍历语法树，根据语法树的结构生成 IR。为了使代码更加简洁，我们声明了一个 trait `GenerateIR`，并为语法树的各节点实现这个 trait：

```rust
pub trait GenerateIR {
    type Output;
    fn generate_ir(&self, ctx: &mut Context) -> Result<Self::Output, ParseError>;
}
```

其中的 `Context` 结构体包含了生成 IR 时需要的一些信息，例如当前作用域的编号、当前函数的参数列表等等：

```rust
pub struct Context {
    pub program: Program,
    pub func: Option<Function>,
    pub scope: Scope,
    pub current_bb: Option<BasicBlock>,
    pub max_basic_block_id: usize,
    pub max_temp_value_id: usize,
    pub while_info: Vec<WhileInfo>,
    pub func_table: HashMap<String, Function>,
}
```

在生成机器代码时，我们同样声明了一个 trait `ToAsm`，并为 IR 的各节点实现这个 trait：

```rust
trait ToAsm {
    type Output;
    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError>;
}
```

同时，为了表示一个汇编程序，我们使用了以下结构体/枚举：

```rust
enum AsmProgramItem {
    VarDecl(AsmVarDecl),
    FuncDecl(AsmFunc),
}

pub struct AsmVarDecl {
    name: String,
    size: usize,
    init: Option<Vec<i32>>,
}

pub struct AsmFunc {
    name: String,
    body: Vec<AsmBlock>,
}

pub struct AsmBlock {
    name: String,
    items: Vec<Box<dyn Inst>>,
}

pub struct AsmProgram {
    items: Vec<AsmProgramItem>,
}
```

类似于 IR 生成，在生成汇编代码时，我们也使用了一个 `Context` 结构体，其中包含了生成汇编代码时需要的一些信息，例如已经使用的寄存器、局部变量和临时值的存放位置等等：

```rust
#[derive(Default)]
pub struct Context {
    pub func: Option<Function>,
    pub temp_value_table: TempValueTable,

    /// Register allocator.
    ///
    /// This is used to allocate registers for temp values.
    pub reg_allocator: RegisterAllocator,
    pub stack_allocator: StackAllocator,
    pub symbol_table: SymbolTable,
    pub name_generator: NameGenerator,
    pub current_bb: Option<BasicBlock>,
}
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑

我们使用一个栈来记录各个作用域。每当进入一个新的作用域时，我们就在栈顶压入一个新的作用域。当离开一个作用域时，我们就从栈顶弹出一个作用域。这样，我们就可以在栈顶快速找到当前作用域。同时，我们使用一个哈希表来记录每个作用域中的变量。当要查找一个变量时，我们就从栈顶开始查找，直到找到为止。这样，我们就可以正确地处理作用域。此外，我们对于不同作用域中的同名变量会在 IR 中生成不同的名称，以避免冲突。

```rust
type IdentifierMap = HashMap<String, Identifier>;

#[derive(Debug)]
pub struct Scope {
    stack: Vec<(i32, IdentifierMap)>,
}
```

#### 2.3.2 寄存器分配策略

我的编译器采取了一个非常简单的寄存器分配策略：当需要一个寄存器时就查找空闲的寄存器，如果没有空闲的寄存器就将一个寄存器中的值写回内存，并更新相应的符号表；当不再需要一个寄存器时就释放这个寄存器。这样，我们就可以保证每个寄存器都被充分利用，同时不会出现寄存器溢出的情况。

#### 2.3.3 采用的优化策略

由于在中间代码生成、目标代码生成时进行的比较仔细，因此我们的编译器生成的代码已经比较高效，因此我只额外添加了一些简单的优化策略，如常量折叠、消减运算强度等。这些优化策略可以在生成的 IR 上进行，不需要对编译器的其他部分进行修改。

#### 2.3.4 其它补充设计考虑
<!-- TODO -->
谈谈做了哪些设计上的考虑，不超过200字，没有的话就不用写。

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和 Lv2. 初试目标代码生成

在这一部分我们仅需识别 `main` 函数并去除注释，生成目标代码。简单地写几行代码即可。

#### Lv3. 表达式

这一部分是表达式的计算，但是由于此时还没有变量，因此只需把所有表达式当作常量表达式在编译器计算出来即可。

#### Lv4. 常量和变量

从这一部分开始，编译器的实现开始变得复杂。对于常量，我们并不需要为其分配空间，只需要在符号表中记录其值即可。对于变量，我们需要为其分配空间，并在符号表中记录其地址。此外，我们还需要处理变量的初始化和赋值。由于 IR 中的计算可能会生成一些临时值，我们还需要为这些临时值分配寄存器。

#### Lv5. 语句块和作用域

这一部分只需略微修改编译器的前端即可。我们使用前述的作用域设计来处理作用域，并在生成 IR 时对变量名按照一定的规则进行 mangle，这样就可以避免不同作用域中的同名变量冲突。

#### Lv6. if 语句

对于 if 语句，直接使用现有的文法规则描述会产生二义性，例如

```c
if (a) if (b) c = 1; else c = 2;
```

这段代码的含义是不明确的。为了解决这个问题，我们需要对文法进行一些修改，把 If 语句分为 IfMatchStmt 和 IfOpenStmt，最终的 Lalrpop 代码如下：

```lalrpop
IfMatchStmt: Stmt = {
    ...
    "if" "(" <cond: LOrExpr> ")" <stmt: IfMatchStmt> "else" <else_stmt:  IfMatchStmt> =>
         Stmt::If(If { cond, else_stmt: Some(Rc::new(else_stmt)), then_stmt: Rc::new(stmt) }),
}

IfOpenStmt: Stmt = {
    "while" "(" <cond: LOrExpr> ")" <stmt: IfOpenStmt> => Stmt::While(While { cond, body: Rc::new(stmt) }),
    "if" "(" <cond: LOrExpr> ")" <stmt: Stmt> => Stmt::If(If { cond, then_stmt: Rc::new(stmt), else_stmt: None }),
    "if" "(" <cond: LOrExpr> ")" <stmt: IfMatchStmt> "else" <else_stmt: IfOpenStmt> =>
           Stmt::If(If { cond, then_stmt: Rc::new(stmt), else_stmt: Some(Rc::new(else_stmt)) }),
}
```

在目标代码生成阶段，我们需要处理 IR 中的 `branch` 和 `jump` 指令，以正确地实现 if 语句。

#### Lv7. while 语句

对弈一个 while 语句，可以被转换为以下的语句：

```c
while (cond) {
    body;
}

// 转换为
start:
if (!cond) goto end;
body;
goto start;
end:
```

因此，我们只需在生成 IR 时按照这个规则转换即可。同时，我们还需要处理 break 和 continue 语句。为了处理这两个语句，我们需要在生成 IR 时记录当前的循环信息，以便在遇到 break 和 continue 语句时正确地跳转。

这一部分的目标代码生成不需要修改。

#### Lv8. 函数和全局变量

这一部分主要处理函数调用，全局变量是非常简单的。在生成 IR 时，我们只需为函数调用生成一个 Call 指令即可。需要注意的一点是，由于 koopa IR 是单赋值的，因此如果函数的参数需要被修改，我们要分配一个新的变量来存储这个参数。

在生成目标代码时，我们需要处理函数调用的参数传递、返回值传递等等。在函数调用前，我们需要把参数传递到相应的寄存器或栈上，并保存调用者保存的寄存器；在函数调用后，我们需要恢复调用者保存的寄存器。在生成函数调用的目标代码时要非常小心，以避免出现错误。

#### Lv9. 数组

由于 koopa IR 直接提供了多维数组的支持。对于多维数组的初始化，我们可以使用 koopa IR 中的聚合初始化；对于多维数组的访问，我们可以使用 koopa IR 中的 `getelementptr` 或 `getptr` 指令。

在生成目标代码时，我们只需根据 koopa IR 中的 `getelementptr` 或 `getprt` 指令生成相应的目标代码即可。由于 koopa IR 提供了类型系统，可以直接根据类型的大小得到偏移量。

### 3.2 工具软件介绍（若未使用特殊软件或库，则本部分可略过）
1. `lalrpop`：`lalrpop` 是一个 Rust 的 LALR 语法分析器的生成器，它可以根据给定的文法规则生成相应的 Rust 代码。我们使用 `lalrpop` 来生成语法分析器。
2. `koopa`：`koopa` 是一个使用 Rust 实现的 IR 框架，它仿照 LLVM IR 设计，提供了一些基本的 IR 指令和数据结构，并同时提供了内存形式和文本形式的 IR 表示。我们使用 `koopa` 来生成 IR 和目标代码。

<!-- ### 3.3 测试情况说明（如果进行过额外的测试，可增加此部分内容）

简述如何构造用例，测出过哪些不一样的错误，怎么发现和解决的。为课程提供优质测试用例会获得bonus。 -->

## 四、实习总结

<!-- 请至少谈1点，多谈谈更好。有机会获得奶茶或咖啡一杯。可以考虑按下面的几点讨论。 -->

### 4.1 收获和体会

- 掌握了实现一个编译器的基本方法和流程，实现了课堂上讲过的部分算法，对编译器的工作原理有了更深的理解。
- 学习里如何使用 Rust 构建一个大型项目。这是我第一次使用 Rust 编写一个较大的项目，我学会了如何组织代码、如何使用 Rust 的模块系统等。

### 4.2 学习过程中的难点，以及对实习过程和内容的建议

- docker 环境中的 Rust 版本过于老旧。Rust 社区中更倾向使用最新版本的 Rust 工具链，很多的 crate 都有最低 Rust 版本 (MSRV) 的要求，因此在老旧的 Rust 版本中编译这些 crate 可能会出现问题。
- 测试用例的设计不够充分，经常出现通过了某一个 Level 但是在后面的 Level 实现时发现前面某个 Level 的实现有错误，这在实现寄存器分配时尤为明显。

### 4.3 对老师讲解内容与方式的建议

- 减少一些文法部分的讲授，增加一些实际的编译器实现的内容。例如，可以讲解一些现代编译器的实现技术，例如 LLVM 的 IR 设计、LLVM 的优化器等等。
- 增加一些优化部分的内容。