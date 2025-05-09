# Yuhui Xie's Compiler Principle lab project

## Optimization
1. original: 618.11s

## libkoopa

#### **koopa_error_code**: 

返回的错误类型，type定义为koopa_error_code_t

```c++
        /// No errors occurred.
        KOOPA_EC_SUCCESS = 0,
        /// UTF-8 st
        ring conversion error.
        KOOPA_EC_INVALID_UTF8_STRING,
        /// File operation error.
        KOOPA_EC_INVALID_FILE,
        /// Koopa IR program parsing error.
        KOOPA_EC_INVALID_KOOPA_PROGRAM,
        /// IO operation error.
        KOOPA_EC_IO_ERROR,
        /// Byte array to C string conversion error.
        KOOPA_EC_NULL_BYTE_ERROR,
        /// Insufficient buffer length.
        KOOPA_EC_INSUFFICIENT_BUFFER_LENGTH,
        /// Mismatch of item kind in raw slice.
        KOOPA_EC_RAW_SLICE_ITEM_KIND_MISMATCH,
        /// Passing null pointers to `libkoopa`.
        KOOPA_EC_NULL_POINTER_ERROR,
        /// Mismatch of type.
        KOOPA_EC_TYPE_MISMATCH,
        /// Mismatch of function parameter number.
        KOOPA_EC_FUNC_PARAM_NUM_MISMATCH,
```

### koopa_raw_slice

#### koopa_raw_slice_item_kind_t

```c++
    enum koopa_raw_slice_item_kind
    {
        /// Unknown.
        KOOPA_RSIK_UNKNOWN = 0,
        /// Type.
        KOOPA_RSIK_TYPE,
        /// Function.
        KOOPA_RSIK_FUNCTION,
        /// Basic block.
        KOOPA_RSIK_BASIC_BLOCK,
        /// Value.
        KOOPA_RSIK_VALUE,
    };
```

koopa_raw_slice是最基本的组织形式，分为五类，分别是Unkown，type，function，basic block和value。

#### koopa_raw_slice_t

```c++
        /// Buffer of slice items.
        const void **buffer;
        /// Length of slice.
        uint32_t len;
        /// Kind of slice items.
        koopa_raw_slice_item_kind_t kind;
```

每个koopa slice有三个成员，分别是slice items的buffer，slice的长度，以及slice的类别。

### koopa_raw_type_kind

#### koopa_raw_type_tag_t

```c++
        /// 32-bit integer.
        KOOPA_RTT_INT32,
        /// Unit (void).
        KOOPA_RTT_UNIT,
        /// Array (with base type and length).
        KOOPA_RTT_ARRAY,
        /// Pointer (with base type).
        KOOPA_RTT_POINTER,
        /// Function (with parameter types and return type).
        KOOPA_RTT_FUNCTION,
```

分为int, unit(void), array, pointer, function

#### koopa_raw_type_kind_t

```c++
        koopa_raw_type_tag_t tag;
        union
        {
            struct
            {
                const struct koopa_raw_type_kind *base;
                size_t len;
            } array;
            struct
            {
                const struct koopa_raw_type_kind *base;
            } pointer;
            struct
            {
                koopa_raw_slice_t params;
                const struct koopa_raw_type_kind *ret;
            } function;
        } data;
```

有一个tag，之后有一个名叫data的union，针对array，pointer和function三种tag进行不同处理，array存base pointer和len，pointer存base pointer，function存params和return pointer。每个pointer都是一个koopa_raw_type_kind的struct，params是koopa_raw_slice_t的结构。

**koopa_raw_type_t**

koopa_raw_type_kind_t的const指针

### koopa_raw_program

#### koopa_raw_program_t

```c++
        /// Global values (global allocations only).
        koopa_raw_slice_t values;
        /// Function definitions.
        koopa_raw_slice_t funcs;
```

全局变量values，以及里面含有的函数定义funcs，都是raw_slice_t的结构。

#### koopa_raw_function_data_t

```c++
/// Type of function.
koopa_raw_type_t ty;
/// Name of function.
const char *name;
/// Parameters.
koopa_raw_slice_t params;
/// Basic blocks, empty if is a function declaration.
koopa_raw_slice_t bbs;
```

**koopa_raw_function_t**

koopa_raw_function_data_t的const指针

#### koopa_raw_basic_block_data_t

```c++
        /// Name of basic block, null if no name.
        const char *name;
        /// Parameters.
        koopa_raw_slice_t params;
        /// Values that this basic block is used by.
        koopa_raw_slice_t used_by;
        /// Instructions in this basic block.
        koopa_raw_slice_t insts;
```

**koopa_raw_basic_block_t**

koopa_raw_basic_block_data_t的const指针

### Koopa_raw_value

#### koopa_raw_value_data_t

koopa_raw_value_t

```c++
        /// Type of value.
        koopa_raw_type_t ty;
        /// Name of value, null if no name.
        const char *name;
        /// Values that this value is used by.
        koopa_raw_slice_t used_by;
        /// Kind of value.
        koopa_raw_value_kind_t kind;
```

#### koopa_raw_value_kind_t

```c++
        koopa_raw_value_tag_t tag;
        union
        {
            koopa_raw_integer_t integer;
            koopa_raw_aggregate_t aggregate;
            koopa_raw_func_arg_ref_t func_arg_ref;
            koopa_raw_block_arg_ref_t block_arg_ref;
            koopa_raw_global_alloc_t global_alloc;
            koopa_raw_load_t load;
            koopa_raw_store_t store;
            koopa_raw_get_ptr_t get_ptr;
            koopa_raw_get_elem_ptr_t get_elem_ptr;
            koopa_raw_binary_t binary;
            koopa_raw_branch_t branch;
            koopa_raw_jump_t jump;
            koopa_raw_call_t call;
            koopa_raw_return_t ret;
        } data;
```

#### koopa_raw_value_tag_t

```c++
        /// Integer constant.
        KOOPA_RVT_INTEGER,
        /// Zero initializer.
        KOOPA_RVT_ZERO_INIT,
        /// Undefined value.
        KOOPA_RVT_UNDEF,
        /// Aggregate constant.
        KOOPA_RVT_AGGREGATE,
        /// Function argument reference.
        KOOPA_RVT_FUNC_ARG_REF,
        /// Basic block argument reference.
        KOOPA_RVT_BLOCK_ARG_REF,
        /// Local memory allocation.
        KOOPA_RVT_ALLOC,
        /// Global memory allocation.
        KOOPA_RVT_GLOBAL_ALLOC,
        /// Memory load.
        KOOPA_RVT_LOAD,
        /// Memory store.
        KOOPA_RVT_STORE,
        /// Pointer calculation.
        KOOPA_RVT_GET_PTR,
        /// Element pointer calculation.
        KOOPA_RVT_GET_ELEM_PTR,
        /// Binary operation.
        KOOPA_RVT_BINARY,
        /// Conditional branch.
        KOOPA_RVT_BRANCH,
        /// Unconditional jump.
        KOOPA_RVT_JUMP,
        /// Function call.
        KOOPA_RVT_CALL,
        /// Function return.
        KOOPA_RVT_RETURN,
```

### koopa_raw_value_kind

#### koopa_raw_integer_t

`int32_t value;`

#### koopa_raw_aggregate_t

`koopa_raw_slice_t elems;`

#### koopa_raw_func_arg_ref_t

`size_t index;`

Raw function argument reference.

#### koopa_raw_block_arg_ref_t

`size_t index;`

 Raw basic block argument reference.

#### koopa_raw_global_alloc_t

`koopa_raw_value_t init;`

Raw global memory allocation.

#### koopa_raw_load_t

`koopa_raw_value_t src;`

Raw memory load.

#### koopa_raw_store_t

```c++
/// Value.

koopa_raw_value_t value;

/// Destination.

koopa_raw_value_t dest;
```

Raw memory store.

#### koopa_raw_get_ptr_t

```c++
        /// Source.
        koopa_raw_value_t src;
        /// Index.
        koopa_raw_value_t index;
```



Raw pointer calculation.

#### koopa_raw_get_elem_ptr_t

```c++
        /// Source.
        koopa_raw_value_t src;
        /// Index.
        koopa_raw_value_t index;
```

Raw element pointer calculation.

#### koopa_raw_binary_op_t

```c++
		/// Not equal to.
        KOOPA_RBO_NOT_EQ,
        /// Equal to.
        KOOPA_RBO_EQ,
        /// Greater than.
        KOOPA_RBO_GT,
        /// Less than.
        KOOPA_RBO_LT,
        /// Greater than or equal to.
        KOOPA_RBO_GE,
        /// Less than or equal to.
        KOOPA_RBO_LE,
        /// Addition.
        KOOPA_RBO_ADD,
        /// Subtraction.
        KOOPA_RBO_SUB,
        /// Multiplication.
        KOOPA_RBO_MUL,
        /// Division.
        KOOPA_RBO_DIV,
        /// Modulo.
        KOOPA_RBO_MOD,
        /// Bitwise AND.
        KOOPA_RBO_AND,
        /// Bitwise OR.
        KOOPA_RBO_OR,
        /// Bitwise XOR.
        KOOPA_RBO_XOR,
        /// Shift left logical.
        KOOPA_RBO_SHL,
        /// Shift right logical.
        KOOPA_RBO_SHR,
        /// Shift right arithmetic.
        KOOPA_RBO_SAR,
```

数学运算。

#### koopa_raw_binary_t

```c++
        /// Operator.
        koopa_raw_binary_op_t op;
        /// Left-hand side value.
        koopa_raw_value_t lhs;
        /// Right-hand side value.
        koopa_raw_value_t rhs;
```

#### koopa_raw_branch_t

```c++
        /// Condition.
        koopa_raw_value_t cond;
        /// Target if condition is `true`.
        koopa_raw_basic_block_t true_bb;
        /// Target if condition is `false`.
        koopa_raw_basic_block_t false_bb;
        /// Arguments of `true` target..
        koopa_raw_slice_t true_args;
        /// Arguments of `false` target..
        koopa_raw_slice_t false_args;
```

#### koopa_raw_jump_t;

```c++
        /// Target.
        koopa_raw_basic_block_t target;
        /// Arguments of target..
        koopa_raw_slice_t args;
```

#### koopa_raw_call_t

```c++
        /// Callee.
        koopa_raw_function_t callee;
        /// Arguments.
        koopa_raw_slice_t args;
```

#### koopa_raw_return_t

```c++
/// Return value, null if no return value.
koopa_raw_value_t value;
```

## changelog

### 10.10
1. environment set up, the compiler now recognize **int main() { return 0; }**  
2. the compiler can now turn string to ast  
3. finished lv1:  
   a. redirecting cout to output file  
   b. blockcomment

### 10.22
1. visit raw program  
2. output riscv

### 10.23
1. lv3 finished  
2. change string KoopaIR into raw program supported by koopa.h  

### 11.21
1. lv4 finished
2. using stack frame to store all values.  

