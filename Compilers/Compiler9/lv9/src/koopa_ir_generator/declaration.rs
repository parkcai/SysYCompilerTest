use koopa::ir::TypeKind;

use crate::ast_def::declaration::*;
use crate::ast_def::terminal_symbol::IDENT;

use super::{get_array_typekind, IRGenerate, IRGenerateResult, IRGeneratorContext, IRSymbolTableEntry};
use super::expression::{ExpIRGenerate, ExpIRGenerateResult};

impl IRGenerate for Decl {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            Decl::ConstDecl(const_decl) => const_decl.generate(context),
            Decl::VarDecl(var_decl) => var_decl.generate(context),
        }
    }
}

// globla variable and array declaration
impl Decl {
    pub fn global_decelaration_generate(&self, context: &mut IRGeneratorContext) -> Result<String, String> {
        let mut return_str = String::new();
        match self {
            Decl::ConstDecl(const_decl) => {
                match const_decl {
                    ConstDecl::ConstDecl(_btype, const_defs) => {
                        for const_def in const_defs {
                            match const_def {
                                ConstDef::ConstDef(ident, arr_size_exps, const_init_val) => {
                                    match const_init_val {

                                        // global const variable
                                        InitVal::Exp(const_exp) => {
                                            let result = const_exp.generate(context)?;
                                            match result {
                                                ExpIRGenerateResult::Const(const_val) => {
                                                    context.ir_symbol_table_stack.insert_new_symbol(
                                                        (*ident).clone(),
                                                        IRSymbolTableEntry::Constant(
                                                            TypeKind::Int32,
                                                            const_val
                                                        )
                                                    );
                                                },
                                                _ => {
                                                    return Err("Global ConstDef found non-const expression".to_string());
                                                },
                                            }
                                        },

                                        // global const array
                                        InitVal::Arr(_) => {
                                            let mut arr_sizes = Vec::new();
                                            for arr_size_exp in arr_size_exps {
                                                let size_result = arr_size_exp.generate(context)?;
                                                match size_result {
                                                    ExpIRGenerateResult::Const(const_val) => {
                                                        arr_sizes.push(const_val as usize);
                                                    },
                                                    _ => {
                                                        return Err("Global array size def found non-const expression".to_string());
                                                    },
                                                }   
                                                
                                            }
                                            let mut result = Vec::new();
                                            array_ast_to_string_filled_with_absent_zero(
                                                const_init_val, 
                                                0, 
                                                &mut result, 
                                                &arr_sizes, 
                                                context
                                            );
                                            return_str.push_str(
                                                array_string_filled_with_absent_zero_to_ir_string(
                                                    0, 
                                                    ident, 
                                                    &result, 
                                                    &arr_sizes, 
                                                    context
                                                ).as_str()
                                            );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            },

            Decl::VarDecl(var_decl) => {
                match var_decl {
                    VarDecl::VarDecl(_btype, var_defs) => {
                        for var_def in var_defs {
                            match var_def {
                                VarDef::VarDef(ident, arr_size_exps, init_val) => {
                                    match init_val {
                                        Some(init_val) => {
                                            match init_val {

                                                // global variable
                                                InitVal::Exp(exp) => {
                                                    context.ir_symbol_table_stack.insert_new_symbol(
                                                        ident.clone(),
                                                        IRSymbolTableEntry::Variable(
                                                            TypeKind::Int32,
                                                            0
                                                        )
                                                    );
                                                    let entry = context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap();
                                                    let mut index = 0;
                                                    match entry {
                                                        IRSymbolTableEntry::Variable(_, _index) => {
                                                            index = *_index;
                                                        },
                                                        _ => {}
                                                    }

                                                    let result = exp.generate(context)?;
                                                    match result {
                                                        ExpIRGenerateResult::Const(const_val) => {
                                                            return_str.push_str(
                                                                format!(
                                                                    "global @{}_{} = alloc i32, {}\n",
                                                                    ident,
                                                                    index,
                                                                    const_val
                                                                ).as_str()
                                                            );
                                                        },
                                                        _ => {
                                                            return Err("Global VarDef found non-const expression".to_string());
                                                        },
                                                    }
                                                },

                                                // global array
                                                InitVal::Arr(_) => {
                                                    let mut arr_sizes = Vec::new();
                                                    for arr_size_exp in arr_size_exps {
                                                        let size_result = arr_size_exp.generate(context)?;
                                                        match size_result {
                                                            ExpIRGenerateResult::Const(const_val) => {
                                                                arr_sizes.push(const_val as usize);
                                                            },
                                                            _ => {
                                                                return Err("Global array size def found non-const expression".to_string());
                                                            },
                                                        }   
                                                    }
                                                    let mut result = Vec::new();
                                                    array_ast_to_string_filled_with_absent_zero(
                                                        init_val, 
                                                        0, 
                                                        &mut result, 
                                                        &arr_sizes, 
                                                        context
                                                    );
                                                    return_str.push_str(
                                                         array_string_filled_with_absent_zero_to_ir_string(
                                                            0, 
                                                            ident, 
                                                            &result, 
                                                            &arr_sizes, 
                                                            context
                                                        ).as_str()
                                                    );
                                                }
                                            }
                                        },
                                        None => {

                                            // non initialized global variable
                                            if arr_size_exps.len() == 0 {
                                                context.ir_symbol_table_stack.insert_new_symbol(
                                                    ident.clone(),
                                                    IRSymbolTableEntry::Variable(
                                                        TypeKind::Int32,
                                                        0
                                                    )
                                                );
                                                let entry = context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap();
                                                let mut index = 0;
                                                match entry {
                                                    IRSymbolTableEntry::Variable(_, _index) => {
                                                        index = *_index;
                                                    },
                                                    _ => {}
                                                }

                                                return_str.push_str(
                                                    format!(
                                                        "global @{}_{} = alloc i32, zeroinit\n",
                                                        ident,
                                                        index
                                                    ).as_str()
                                                );
                                            }

                                            // non initialized global array
                                            else {
                                                let mut _arr_sizes = Vec::new();
                                                for size in arr_size_exps {
                                                    let size_result = size.generate(context)?;
                                                    match size_result {
                                                        ExpIRGenerateResult::Const(const_val) => {
                                                            _arr_sizes.push(const_val as usize);
                                                        },
                                                        _ => {
                                                            return Err("Global array size def found non-const expression".to_string());
                                                        },
                                                    }   
                                                }
                                                let mut result = Vec::new();
                                                array_ast_to_string_filled_with_absent_zero(
                                                    &InitVal::Arr(Vec::new()), 
                                                    0, 
                                                    &mut result, 
                                                    &_arr_sizes, 
                                                    context
                                                );
                                                return_str.push_str(
                                                    array_string_filled_with_absent_zero_to_ir_string(
                                                        0, 
                                                        ident, 
                                                        &result, 
                                                        &_arr_sizes, 
                                                        context
                                                    ).as_str()
                                                );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        Ok(return_str)
    }
}

impl IRGenerate for VarDecl {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            VarDecl::VarDecl(_btype, var_defs) => {
                for var_def in var_defs {
                    var_def.generate(context)?;
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

// local variable and array declaration
impl IRGenerate for VarDef {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            VarDef::VarDef(ident, arr_size_exps, init_val) => {
                
                match init_val {
                    Some(init_val) => {
                        match init_val {
                            InitVal::Exp(exp) => { // 变量
                                context.ir_symbol_table_stack.insert_new_symbol(
                                    (*ident).clone(), 
                                    IRSymbolTableEntry::Variable(
                                        TypeKind::Int32,
                                        0
                                    )
                                );
                                let entry = context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap();
                                let mut index = 0;
                                match entry {
                                    IRSymbolTableEntry::Variable(_type, _index) => {
                                        index = *_index;
                                    },
                                    _ => {}
                                }
                                context.output_str.push_str(
                                    format!(
                                        "    @{}_{} = alloc i32\n",
                                        ident,
                                        index
                                    ).as_str()
                                );
                                let result = exp.generate(context)?;
                                match result {
                                    ExpIRGenerateResult::ExpResult(str_result) => {
                                        context.output_str.push_str(
                                            format!(
                                                "    store {}, @{}_{}\n",
                                                str_result,
                                                ident,
                                                index
                                            ).as_str()
                                        );
                                    },
                                    ExpIRGenerateResult::Const(const_val) => {
                                        context.output_str.push_str(
                                            format!(
                                                "    store {}, @{}_{}\n",
                                                const_val,
                                                ident,
                                                index
                                            ).as_str()
                                        );
                                    },
                                }
                            },
                            InitVal::Arr(_) => { // 数组
                                let mut arr_sizes = Vec::new();
                                for arr_size_exp in arr_size_exps {
                                    let size_result = arr_size_exp.generate(context)?;
                                    match size_result {
                                        ExpIRGenerateResult::Const(const_val) => {
                                            arr_sizes.push(const_val as usize);
                                        },
                                        _ => {
                                            return Err("Local array size def found non-const expression".to_string());
                                        },
                                    }   
                                    
                                }
                                let mut result = Vec::new();
                                array_ast_to_string_filled_with_absent_zero(
                                    init_val, 
                                    0, 
                                    &mut result, 
                                    &arr_sizes, 
                                    context
                                );
                                array_string_filled_with_absent_zero_to_ir_string(
                                    1, 
                                    ident, 
                                    &result, 
                                    &arr_sizes, 
                                    context
                                );
                            }
                        }
                    },
                    None => {
                        if arr_size_exps.len() == 0 { // 变量
                            context.ir_symbol_table_stack.insert_new_symbol(
                                (*ident).clone(), 
                                IRSymbolTableEntry::Variable(
                                    TypeKind::Int32, 
                                    0
                                )
                            );
                            let entry = context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap();
                            let mut index = 0;
                            match entry {
                                IRSymbolTableEntry::Variable(_type, _index) => {
                                    index = *_index;
                                },
                                _ => {}
                            }
                            context.output_str.push_str(
                                format!(
                                    "    @{}_{} = alloc i32\n",
                                    ident,
                                    index
                                ).as_str()
                            );
                        }
                        else { // 数组
                            let mut _arr_sizes = Vec::new();
                            for size in arr_size_exps {
                                let size_result = size.generate(context)?;
                                match size_result {
                                    ExpIRGenerateResult::Const(const_val) => {
                                        _arr_sizes.push(const_val as usize);
                                    },
                                    _ => {
                                        return Err("Local array size def found non-const expression".to_string());
                                    },
                                }
                            }
                            let result = Vec::new();
                            array_string_filled_with_absent_zero_to_ir_string(
                                1, 
                                ident, 
                                &result, 
                                &_arr_sizes, 
                                context
                            );
                        }
                    }
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
    
}

impl IRGenerate for ConstDecl {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            // 由于本课程只考虑BType = int，所以这里不需要处理BType，直接在constDef的build使用默认的int即可
            ConstDecl::ConstDecl(_btype, const_defs) => {
                for const_def in const_defs {
                    const_def.generate(context)?;
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

// local const variable and array declaration
impl IRGenerate for ConstDef {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            ConstDef::ConstDef(ident, arr_size_exps, const_init_val) => {
                match const_init_val {
                    InitVal::Exp(const_exp) => {
                        let result = const_exp.generate(context)?;
                        match result {
                            ExpIRGenerateResult::Const(const_val) => {
                                context.ir_symbol_table_stack.insert_new_symbol(
                                    (*ident).clone(), 
                                    IRSymbolTableEntry::Constant(
                                        koopa::ir::TypeKind::Int32, 
                                        const_val
                                    )
                                );
                            },
                            _ => {
                                Err("ConstDef found non-const expression".to_string())?;
                            },
                        }
                    },
                    InitVal::Arr(_) => {
                        let mut arr_sizes = Vec::new();
                        for arr_size_exp in arr_size_exps {
                            let size_result = arr_size_exp.generate(context)?;
                            match size_result {
                                ExpIRGenerateResult::Const(const_val) => {
                                    arr_sizes.push(const_val as usize);
                                },
                                _ => {
                                    return Err("Global array size def found non-const expression".to_string());
                                },
                            }   
                            
                        }
                        let mut result = Vec::new();
                        array_ast_to_string_filled_with_absent_zero(const_init_val, 0, &mut result, &arr_sizes, context);
                        array_string_filled_with_absent_zero_to_ir_string(1, ident, &result, &arr_sizes, context);
                    }
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

// 将一个数组的ast进行转换，将初始化数据放到```result: &mut Vec<String>```中，如果数组维度不够，则填充0
fn array_ast_to_string_filled_with_absent_zero(init_val: &InitVal, dim_to_fill: usize, result: &mut Vec<String>, arr_sizes: &Vec<usize>, context: &mut IRGeneratorContext) {
    match init_val {
        InitVal::Arr(arr) => {
            for _init_val in arr {
                match _init_val.as_ref() {
                    InitVal::Exp(exp) => {
                        let exp_result = exp.generate(context);
                        match exp_result {
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                result.push(const_val.to_string());
                            },
                            Ok(ExpIRGenerateResult::ExpResult(exp_result)) => {
                                result.push(exp_result);
                            },
                            _ => {}
                        }
                    },
                    InitVal::Arr(_) => {
                
                        let mut _dim_to_fill = arr_sizes.len() - 1;
                        while _dim_to_fill > 0 {
                            let mut full_element_cnt = 1;
                            for i in _dim_to_fill..arr_sizes.len() {
                                full_element_cnt *= arr_sizes[i];
                            }
                            if result.len() % full_element_cnt == 0 {
                                _dim_to_fill -= 1;
                                continue;
                            }
                            else {
                                _dim_to_fill += 1;
                                break;
                            }
                        }
                        if _dim_to_fill <= dim_to_fill {
                            _dim_to_fill = dim_to_fill + 1;
                        }
                        array_ast_to_string_filled_with_absent_zero(_init_val, _dim_to_fill, result, arr_sizes, context);
                    }
                }
            }
            // 如果是空的列表，result中什么都没有也会满足模数为0，所以需要特殊处理
            if result.len() == 0 {
                result.push("0".to_string());
            }

            // 剩余位置填充0
            let mut full_element_cnt = 1;
            for i in dim_to_fill..arr_sizes.len() {
                full_element_cnt *= arr_sizes[i];
            }
            while result.len() % full_element_cnt != 0 {
                result.push("0".to_string());
            }
        },
        _ => {}
    }
}

// 将上一个函数转换得到的初始化数据字符串数组转换为ir字符串，插入到全局变量中
// mode 0: global, 1: local
// global时ir在return_str中，通过函数返回值获取。
// local时ir在context.output_str中，函数返回值无效。
fn array_string_filled_with_absent_zero_to_ir_string (mode: u8, array_ident: &IDENT, array_init_values: &Vec<String>, arr_sizes: &Vec<usize>, context: &mut IRGeneratorContext) -> String {
    let mut return_str = String::new();
    let mut array_type_string = String::new();

    // 插入符号表，获得符号的index
    context.ir_symbol_table_stack.insert_new_symbol(
        array_ident.clone(),
        IRSymbolTableEntry::Variable(
            get_array_typekind(arr_sizes), 
            0
        ),
    );
    let entry = context.ir_symbol_table_stack.get_symbol(array_ident.clone()).unwrap();
    let mut symbol_index = 0;
    match entry {
        IRSymbolTableEntry::Variable(_, _index) => {
            symbol_index = *_index;
        },
        _ => {}
    }

    // 构造数组类型的字符串
    for _ in 0..arr_sizes.len() {
        array_type_string.push_str("[");
    }
    array_type_string.push_str("i32");
    for i in 0..arr_sizes.len() {
        array_type_string.push_str(
            format!(
                ", {}]",
                arr_sizes[arr_sizes.len() - 1 - i]
            ).as_str()
        );
    }
    let mut all_zero = true;
    for i in 0..array_init_values.len() {
        if array_init_values[i] != "0".to_string() {
            all_zero = false;
            break;
        }
    }
    match mode {
        0 => { // global
            if all_zero {
                return_str.push_str(
                    format!(
                        "global @{}_{} = alloc {}, zeroinit\n",
                        array_ident,
                        symbol_index,
                        array_type_string
                    ).as_str()
                );
                
            } else {
                return_str.push_str(
                    format!(
                        "global @{}_{} = alloc {}, {}\n",
                        array_ident,
                        symbol_index,
                        array_type_string,
                        array_data_to_global_declaration(array_init_values, arr_sizes)
                    ).as_str()
                );
            }
        },
        1 => { // local
            context.output_str.push_str(
                format!(
                    "    @{}_{} = alloc {}\n",
                    array_ident,
                    symbol_index,
                    array_type_string,
                ).as_str()
            );
            if array_init_values.len() == 0 || all_zero {
                return String::new();
            }
            let base_pointer = format!("@{}_{}", array_ident, symbol_index);
            array_data_to_local_declaration(array_init_values, arr_sizes, base_pointer, context);
        },
        _ => {}
    }

    return_str
}

fn array_data_to_global_declaration(array_init_values: &Vec<String>, arr_sizes: &Vec<usize>) -> String {
    let mut result = String::new();
    if arr_sizes.len() == 1 {
        result.push_str("{");
        for i in 0..array_init_values.len() {
            result.push_str(array_init_values[i].as_str());
            if i != array_init_values.len() - 1 {
                result.push_str(", ");
            }
        }
        result.push_str("}");
        result
    }
    else {
        result.push_str("{");
        let new_size = array_init_values.len() / arr_sizes[0];
        for i in 0..arr_sizes[0] {
            result.push_str(
                array_data_to_global_declaration(
                    &array_init_values[i*new_size..(i+1)*new_size].to_vec(), 
                    &arr_sizes[1..].to_vec()
                ).as_str()
            );
            if i != arr_sizes[0] - 1 {
                result.push_str(", ");
            }
        }
        result.push_str("}");
        result
    }
}

fn array_data_to_local_declaration(array_init_values: &Vec<String>, arr_sizes: &Vec<usize>, base_pointer: String, context: &mut IRGeneratorContext) {
    if arr_sizes.len() == 1 {
        for i in 0..array_init_values.len() {
            // if array_init_values[i] == 0.to_string() {
            //     continue;
            // }
            let new_base_pointer = match base_pointer.chars().next().unwrap() {
                '@' => {
                    format!(
                        "%{}_{}",
                        base_pointer[1..].to_string(),
                        i
                    )
                },
                
                _ => {
                    format!(
                        "{}_{}",
                        base_pointer,
                        i
                    )
                }
            };
            context.output_str.push_str(
                format!(
                    "    {} = getelemptr {}, {}\n",
                    new_base_pointer,
                    base_pointer,
                    i,
                ).as_str()
            );
            context.output_str.push_str(
                format!(
                    "    store {}, {}\n",
                    array_init_values[i],
                    new_base_pointer,
                ).as_str()
            );
        }
    }
    else {
        let new_size = array_init_values.len() / arr_sizes[0];
        for i in 0..arr_sizes[0] {
            
            let new_base_pointer = match base_pointer.chars().next().unwrap() {
                '@' => {
                    format!(
                        "%{}_{}",
                        base_pointer[1..].to_string(),
                        i
                    )
                },
                
                _ => {
                    format!(
                        "{}_{}",
                        base_pointer,
                        i
                    )
                }
            };
            context.output_str.push_str(
                format!(
                    "    {} = getelemptr {}, {}\n",
                    new_base_pointer,
                    base_pointer,
                    i,
                ).as_str()
            );
            array_data_to_local_declaration(
                &array_init_values[i * new_size..(i + 1) * new_size].to_vec(),
                &arr_sizes[1..].to_vec(),
                new_base_pointer,
                context
            );
        }
    }
}