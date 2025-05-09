# 编译原理课程实践报告

信息科学技术学院 2000011158 毛嘉楷

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 将SysY程序编译为KoopaIR程序
2. 将生成的KoopaIR程序编译为RISC-V程序
3. 在生成RISC-V程序时，做最简单的优化

### 1.2 主要特点

本编译器的主要特点是**逻辑简单**、**功能完整**。逻辑简单指的是在编译程序运行过程中，各函数的功能明确，没有过多的循环调用。功能完整指的是通过了线上测评网站的全部测试样例。

## 二、编译器设计

### 2.1 主要模块组成

编译器由4个主要模块组成：
1. sysy.l 部分负责词法分析
2. sysy.y 部分负责语法分析并初始化了少量的语义信息
3. ast.hpp 部分负责KoopaIR程序的生成
4. riscv.hpp 部分负责RISC-V程序的生成

### 2.2 主要数据结构

编译器前端最核心的数据结构是`BaseAST`类型，是所有AST节点的父类。

```cpp
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual string Koopa() = 0;

  bool returned = false;

  int type_code;

  static int calcuKoopa(string koopa_result);

  static string parseIR(string koopa_result);

  static int searchTab(string ident);

  static string linkLib(); 
};
```

在符号表的实现中，引入了`symbol`和`table`两种数据结构，分别代表了符号和符号表。

```cpp
struct symbol {
  int type;
  string val;
  vector<int> dim;
  symbol (int t, string v) {
    type=t;
    val=v;
  }
  symbol (int t, vector<int> d) {
    type=t;
    dim=d;
  }
  symbol () {}
};

struct table {
  int cnt;
  map<string, symbol> tab;
  table (int c, map<string, symbol> t) {
    cnt = c;
    tab = t;
  }
  table() {}

  void newTable() {
    cnt = tab_cnt++;
    tab.clear();
  }
};
```

后端的实现引入了名为`ProInfo`的数据结构，表示prologue过程返回的数据。

```cpp
struct ProInfo {
  int frame_length;
  int max_arg_length;

  ProInfo () {}
  ProInfo (int f, int m) {
    frame_length = f;
    max_arg_length = m;
  }
};
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑
使用栈的结构，在进入一个新Block之前，将当前的符号表压入栈中并创建一个新表（有唯一标识符`tab_cnt`）。在退出Block时，丢弃当前符号表并弹出栈顶的符号表。在查找符号时，先查当前表，再从栈顶开始查到栈底。

#### 2.3.2 寄存器分配策略
只做单条KoopaIR指令的寄存器分配，保证没有溢出的情况。指令执行前，将需要用到的不同数据从栈中对应位置加载到不同寄存器中。指令结束后，释放所用到的寄存器并将指令的计算结果存在当前栈里可以储存的位置。

#### 2.3.3 采用的优化策略
本来打算将得到的RISC-V代码遍历一遍，找出类似

```
load  a, b
store a, b
```

或者

```
store a, b
load  a, b
```

的情况，删除两种情况下的后一条代码。

但是，考虑到遍历也需要时间且本人已大四，遂作罢。

#### 2.3.4 其它补充设计考虑
1. 前端：在语法分析阶段，为了避免二义性和规约冲突，引入了多个Temp节点，不同的节点对于语义分析阶段是透明的。在短路操作的实现过程中，使用了KoopaIR的SSA形式。
2. 后端：在生成RISC-V程序时，使用统一的生成函数`product_riscv`，对其进行了不同的重载，以生成不同格式的指令。

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成

课程群里助教给出的正则表达式似乎并不能解决所有的块注释问题，因为`*`后面可能只有`*`或者`/`（不妨称其为`*`在最右边的情况），例如：

```cpp
/* *****/
```

为了处理`*`的不同情况，可以将块注释分成以下几种情况：

```
BlockComment  \/\*([^*]|\*+[^*/])*\*\/
RStarComment  \/\*[^*]*\*+\/
MStarComment  \/\*[^*]+(\*+[^*/][^*]*)+\*+\/
LStarComment  \/\*(\*+[^*/][^*]*)+\*+\/
```

其中，`RStarComment`匹配`*`只在最右边的情况，`MStarComment`匹配`*`在中间和最右边的情况，`LStarComment`匹配`*`在最左边、中间和最右边的情况。

需要注意的是，yacc的匹配原则是尽可能地匹配更多字符，先选择匹配长度最长的规则，长度相同时，匹配顺序在前的规则。

另外，由于SysY语言不处理字符串，所以编译器不用考虑`"`的情况。

#### Lv3. 表达式

这一部分将通过表达式的处理介绍编译器前端和后端的基本逻辑。

**前端**   

词法分析和语法分析在同一遍中进行。语法分析过程中，编译器构建起AST结构，并返回根节点的指针。运行根节点的`Koopa()`函数，生成此AST的KoopaIR程序。

每一个`BaseAST`都定义了`type_code`变量，表示此节点的类型（在文法中，同一个节点可以推导出不同的结构）。

```cpp
RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 0;
    // ...
    $$ = ast;
  }
  | RelExp LTOP AddExp {
    auto ast = new RelExpAST();
    ast->type_code = 1;
    // ...
    $$ = ast;
  }
  | // ...
```

每一个`BaseAST`都实现了`Koopa()`函数，此函数将根据不同的`type_code`值调用不同子节点的`Koopa()`函数，获得其返回值后，通过字符串`string`的操作生成此节点的KoopaIR代码并返回。

```cpp
class RelExpAST : public BaseAST {
 public:
  string op;
  unique_ptr<BaseAST> add_exp;
  unique_ptr<BaseAST> rel_exp;

  string Koopa() override {
    if (type_code == 0)
      return add_exp->Koopa();
    else if (type_code == 1) {
      string rel_result = rel_exp->Koopa();
      // ...
    }
  }
};
```

**后端**

在得到KoopaIR程序后，将其加载到内存上，并且按照函数、基本块、指令的层次遍历整个程序。在处理指令时，根据`value->kind.tag`的值判断指令类型，针对不同指令做不同处理。若`value->kind.tag == KOOPA_RVT_BINARY`，则该指令为表达式。根据其操作符的不同调用不同的函数，生成不同的RISC-V代码。

```cpp
for (size_t i = 0; i < raw.funcs.len; ++i) {
  // ...
  for (size_t j = 0; j < func->bbs.len; ++j) {
    // ...
    for (size_t k = 0; k < bb->insts.len; ++k) { 
      koopa_raw_value_t value = (koopa_raw_value_t) bb->insts.buffer[k];
      // ...
      if (value->kind.tag == KOOPA_RVT_BINARY) {
        if (value->kind.data.binary.op == KOOPA_RBO_EQ) {
          binary_op("seqz", OP_NEEQ, value, var_point);
        }
        else if (value->kind.data.binary.op == KOOPA_RBO_NOT_EQ) {
          binary_op("snez", OP_NEEQ, value, var_point);
        }
        // ...
      }
      // ...
    }
    // ...
  }
  // ...
}
```

在解析KoopaIR过程中，往往遇到一条语句的计算需要使用另一条语句的结果，这种情况下编译器选择维护两个`map`对象来表示指令和寄存器之间的关系（此时还未使用内存栈，所有计算结果均使用寄存器保存，无法处理溢出的情况）。

```cpp
map<koopa_raw_value_t, string> ins_to_reg;
map<string, koopa_raw_value_t> reg_to_ins;
```

#### Lv4. 常量和变量

本部分将重点介绍编译器对常量和变量的处理。

***常量的处理***

常量只会出现在编译器的前端。当编译器解析到SysY语言的常量定义语句时，常量的值将被求出并连同常量的名字一并保存在符号表中，此过程对KoopaIR是透明的。常量的求值使用的是`BaseAST`的`calcuKoopa(string)`静态函数，此函数通过解析传入的KoopaIR指令计算常量的值。

```cpp
// 常量的定义
class ConstDefAST : public BaseAST {
  // ...
  string Koopa() override {
    // ...
    string koopa_result = const_init_val->Koopa();
    // ...
    symbol_tab.tab[ident] = symbol(CONST_SYM, to_string(BaseAST::calcuKoopa(koopa_result)));
    // ...
  }
};
```

在后续的程序中，如果检测到出现该常量，则用其值替代之。

***变量的处理***

1. 前端：当编译器解析到SysY语言的变量定义语句时，先在符号表中加入该变量的名字，再生成KoopaIR的alloc指令。在生成指令时，为了防止重名，在变量名后添加了`_x`，其中的x表示此时变量表的唯一标识符`tab_cnt`。若其在定义时被赋值，则需要加上赋值的KoopaIR指令。在后续的程序中，如果需要使用变量的值，则需要通过变量的查找函数`searchTab(string)`找到变量所在表的唯一标识符并生成KoopaIR的load指令。

```cpp
// 变量的定义
class VarDefAST : public BaseAST {
  // ...
  string Koopa() override {
    // ...
    string koopa_result = init_val->Koopa();
    // ...
    symbol_tab.tab[ident] = symbol(VAR_SYM, "");
    // ...
    return koopa_result + 
        "  @" + ident + "_" + to_string(symbol_tab.cnt) + " = alloc i32 \n" + 
        "  store %" + to_string(IR_cnt) + ", @" + ident + "_" + to_string(symbol_tab.cnt) + "\n";
    // ...
  }
};
```

```cpp
// 变量的使用
class PrimaryExpAST : public BaseAST {
  // ...
  string Koopa() override {
    string val_result = l_val->Koopa();
    // ...
    if (symbol_tab.tab[val_result].type == VAR_SYM)
      return "  %" + to_string(++IR_cnt) + " = load @" + val_result + "_" + to_string(symbol_tab.cnt) + "\n";
    // ...
  }
};          
```

2. 栈的引入（后端）：从这里开始，编译器的后端开始使用内存栈。在指令的分析进入一个新的函数后，编译器会立刻运行`prologue(koopa_raw_function_t)`来计算此函数栈帧的大小，并使得栈指针向低地址移动。当指令的分析离开一个函数时，编译器在离开前运行`epilogue(int)`使得栈指针指向进入函数前的位置。

```cpp
int prologue(koopa_raw_function_t func) {
  int frame_length = 0;
  for (size_t j = 0; j < func->bbs.len; ++j) {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];  
    for (size_t k = 0; k < bb->insts.len; ++k) { 
      koopa_raw_value_t value = (koopa_raw_value_t) bb->insts.buffer[k];
        if (value->ty->tag != KOOPA_RTT_UNIT) 
          frame_length += 4;
    }
  }
  // 对齐16字节
  frame_length = frame_length%16 ? (frame_length/16 + 1)*16 : frame_length;

  product_riscv("addi", getReg(REG_sp), getReg(REG_sp), -frame_length);

  return frame_length;
}

void epilogue(int frame_length) {
  product_riscv("addi", getReg(REG_sp), getReg(REG_sp), frame_length);
}
```

3. 栈的使用（后端）：在引入栈的概念之后，我们将每一条KoopaIR指令的结果存到栈上，需要使用时再取出。因此，我们需要维护一个指令到栈偏移的映射`ins_to_off`以及一个栈当前可用位置的偏移量`var_point`，在对新引入的三条指令的处理中就有体现它们的用处。

```cpp
map<koopa_raw_value_t, int> ins_to_off;

// 以下被替代
map<koopa_raw_value_t, string> ins_to_reg;
map<string, koopa_raw_value_t> reg_to_ins;
```

- `alloc`：建立当前可用位置与当前alloc指令的对应关系。

```cpp
void alloc_op(koopa_raw_value_t value, int& var_point) {
  ins_to_off[value] = var_point;
  var_point += 4;
}
```

- `load`：将src指令处的内容加载到当前位置，并建立当前可用位置与当前load指令的对应关系。

```cpp
void load_op(koopa_raw_value_t value, int& var_point) {
  koopa_raw_value_t src_value = value->kind.data.load.src;
  string reg = findAvailableReg();

  int src_offset = ins_to_off[src_value];
  product_riscv("lw", reg, src_offset, getReg(REG_sp));
  product_riscv("sw", reg, var_point, getReg(REG_sp));

  ins_to_off[value] = var_point;
  var_point +=4;
}
```

- `store`：将关联了src指令的位置处的内容存储到dest指令关联的位置

```cpp
void store_op(koopa_raw_value_t value) {
  koopa_raw_value_t src_value = value->kind.data.store.value;
  koopa_raw_value_t dest_value = value->kind.data.store.dest;
  string reg = findAvailableReg();

  if (src_value->kind.tag == KOOPA_RVT_INTEGER) {
    product_riscv("li", reg, src_value->kind.data.integer.value);
  }
  else {
    int src_offset = ins_to_off[src_value];
    product_riscv("lw", reg, src_offset, getReg(REG_sp));
  }

  int dest_offset = ins_to_off[dest_value];
  product_riscv("sw", reg, dest_offset, getReg(REG_sp));
}
```

将KoopaIR指令的结果存入栈中，结果如下图：

![alt text](photos/example-stack-frame.png)

#### Lv5. 语句块和作用域

这一部分主要围绕符号表的栈式结构展开。在进入一个新Block前，在栈`tab_stk`中储存旧的符号表并创建新符号表。在离开Block后，从栈中弹出上一个符号表并更新当前符号表。

```cpp
// BlockAST的Koopa()函数处理符号表的片段
tab_stk.push_back(symbol_tab);
symbol_tab.newTable();
// ...
symbol_tab.cnt = tab_stk[tab_stk.size() - 1].cnt;
symbol_tab.tab = tab_stk[tab_stk.size() - 1].tab;
tab_stk.pop_back();
```

符号的作用域由`BaseAST`的静态函数`searchTab(string)`实现。若当前符号表中有符号，则作用域为当前Block。若当前符号表没有符号，则从栈顶向栈底查找，直到找出符号，其作用域为从定义处语句到当前Block。函数中`global_symbol_tab`表示全局符号表。

```cpp
// 查符号表
static int searchTab(string ident) {
  if (symbol_tab.tab.count(ident))
    return tab_stk.size();
  else {
    for (int i=tab_stk.size() - 1; i >=0; i--)
      if (tab_stk[i].tab.count(ident))
        return i;
  }
  if (global_symbol_tab.tab.count(ident))
    return GLOBAL_DATA;
  return -2;
}
```

另外，在Lv4中已经提及到，为了避免KoopaIR中的变量重名问题，编译器会在每一个SysY变量后添加符号表的唯一标识符`tab_cnt`来构成KoopaIR中的变量。

#### Lv6. if语句

**二义性的解决**

给定的文法规则会导致二义性（空悬else问题）。为了解决该问题，将文法修改如下：

```ebnf
StmtMatch ::= IF '(' Exp ')' StmtMatch ELSE StmtMatch | 
              other;

StmtOpen ::= IF '(' Exp ')' Stmt | 
             IF '(' Exp ')' StmtMatch ELSE StmtOpen ;

Stmt ::= StmtMatch | 
         StmtOpen ;
```

其中，other表示除去if-else以外其它类型的Stmt节点展开形式。目前为止，文法不会有二义性，但是后续添加文法规则后会出现规约冲突。

**分支结构（前端）**

KoopaIR中将if-else语句分为四个阶段：条件判断阶段、`%if_then_x`阶段、`%if_else_x`阶段、`%if_end_x`阶段。其中，`x`为该if-else语句的唯一标识符`if_label_cnt`，防止标签重名。

```llvm
  ...
  br %1, %if_then_0, %if_else_0
%if_then_0:
  ...
  ret %2
%if_else_0:
  ...
  jump %if_end_0
%if_end_0:
  ...
```

需要重点强调的是，由于KoopaIR的基本块的结尾必须是`br`、`jump`或`ret`指令其中之一，而且这些指令只能出现在基本块的结尾，我们需要判断`if`和`else`后的Block是否返回，在已返回的基本块后加`ret`语句，在未返回的基本块后加`jump`语句。在编译器中，我们用`BaseAST`的returned参数表示该节点是否返回。判断Block是否返回只需要判断其中是否存在已返回的BlockItem，并且若存在则Block内后续的AST节点都不需要解析，代码如下：

```cpp
// BlockAST的Koopa函数片段
string block_item_koopa;
for (int i=0; i<block_item_vec.size(); i++) {
  string koopa_result = block_item_vec[i]->Koopa();
  block_item_koopa += koopa_result;
  if (block_item_vec[i]->returned) {
    this->returned = true;
    break;
  }
}
```

只有两种情况的BlockItem是已返回的：
1. `return`语句

```cpp
return;

return 0;
```

2. 两个stmt段都已返回的`if-else`语句

```cpp
if (1) 
  return 1;
else
  return 0;

// 以下情况不可视为已返回
if (0)
  return 0;
```

**分支结构（后端）**

这里主要说明`bnez`的解析过程。在RISC-V中，`bnez`和`beqz`都对跳转的目标label与当前指令的相对地址有要求，如果相对地址超出范围，则会出现 *AE* 错误。

![alt text](photos/riscv-beqz(bnez).png)

为了解决这个问题，编译器会在生成`bnez`指令时再生成一个`br_x`基本块，其中`x`表示branch语句的唯一标识符`br_cnt`。在需要跳转时，先跳转到这个新添加的基本块，再使用`j`语句跳到更远处。

```cpp
void product_riscv(string ins_type, 
                   string reg, 
                   koopa_raw_basic_block_t true_bb, 
                   koopa_raw_basic_block_t false_bb) {
  riscv += "  " + ins_type + string(6 - ins_type.size(), ' ') + 
           reg + ", br_" + to_string(br_cnt) + "\n";
  
  product_riscv("j", false_bb);
  
  product_label("%br_" + to_string(br_cnt++));
  
  product_riscv("j", true_bb);
}
```

**短路操作（前端）**

在短路操作的实现中，为了便于理解，编译器使用了KoopaIR的SSA形式，将逻辑与和逻辑或的结果通过`jump`操作传给跳转到的label。

```cpp
int main() {  
  return 1 || 0;
}
```

```llvm
fun @main(): i32 {
%entry:
  %1 = ne 0, 1
  br %1, %or_true_0, %or_false_0
%or_true_0:
  jump %or_end_0(%1)
%or_false_0:
  %2 = ne 0, 0
  jump %or_end_0(%2)
%or_end_0(%3: i32):
  ret %3
}
```

**短路操作（后端）**

为了解析前端SSA形式的KoopaIR，编译器的后端需要做以下几点额外的操作：

1. 在prologue阶段，为出现在基本块label中的参数留出栈空间

```cpp
int prologue(koopa_raw_function_t func) {
  int frame_length = 0;
  for (size_t j = 0; j < func->bbs.len; ++j) {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];
    frame_length += 4 * bb->params.len;  
    // ...
  }
  // ...
}
```

2. 在解析`jump`语句时，对可能传出的参数做处理

```cpp

```

3. 在解析新的基本块时，对可能传入的参数做处理

```cpp

```

#### Lv7. while语句

```
循环嵌套和控制流，总会有点想说的吧？
```

#### Lv8. 函数和全局变量

returned
```
传参和返回值的处理，有想分享的地方么？
```

#### Lv9. 数组

```
数组参数？多维数组？不过……如果没做到这个阶段就不用写了。
```

### 3.2 工具软件介绍
`bison`：使用以下命令生成了LALR(1)的状态和操作表，找出了移进-规约冲突和规约-规约冲突的情况

```bash
 bison -v src/sysy.y -o debug/sysy.output
```

### 3.3 测试情况说明

简述如何构造用例，测出过哪些不一样的错误，怎么发现和解决的。为课程提供优质测试用例会获得bonus。

```cpp
0;

return 0;

BType FuncType

addi, lw, sw

if while if else 

arr[arr[0]]

beqz
```

## 四、实习总结

请至少谈1点，多谈谈更好。有机会获得奶茶或咖啡一杯。可以考虑按下面的几点讨论。

### 4.1 收获和体会
### 4.2 学习过程中的难点，以及对实习过程和内容的建议
### 4.3 对老师讲解内容与方式的建议
