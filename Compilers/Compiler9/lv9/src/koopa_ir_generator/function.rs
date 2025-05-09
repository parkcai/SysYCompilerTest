use koopa::ir::{Type, TypeKind};

use crate::ast_def::{function::*, terminal_symbol::BType};
use super::{expression::{ExpIRGenerate, ExpIRGenerateResult}, get_array_typekind, IRGenerate, IRGenerateResult, IRGeneratorContext, IRSymbolTableEntry};

impl IRGenerate for FuncDef {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            FuncDef::FuncDef(func_type, ident, func_fparams, block) => {
                context.ir_function_table.insert_new_function(ident.clone(), func_type.clone());

                // 创建符号表
                context.ir_symbol_table_stack.push();

                context.output_str.push_str(
                    format!("fun @{}", ident).as_str()
                );

                context.output_str.push_str("(");
                if let Some(func_fparams) = func_fparams {
                    func_fparams.generate(context)?;
                }
                context.output_str.push_str(")");

                match func_type {
                    BType::void => {
                        context.output_str.push_str(" ");
                    }
                    BType::int => {
                        context.output_str.push_str(": i32 ");
                    }
                }

                context.output_str.push_str("{\n");
                context.output_str.push_str(
                    format!(
                        "%{}_entry:\n",
                        ident
                    ).as_str()
                );

                if let Some(func_fparams) = func_fparams {
                    func_fparams.another_generate(context)?;
                }

                match block.generate(context)? {
                    IRGenerateResult::Ok => {
                        let default_return_value = match func_type {
                            BType::void => "",
                            BType::int => " 0",
                        };
                        context.output_str.push_str(
                            format!(
                                "    ret{}\n",
                                default_return_value
                            ).as_str()
                        );
                    },
                    IRGenerateResult::EarlyTermination => {},
                }
                context.output_str.push_str("}\n");
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

impl IRGenerate for FuncFParams {
    // 只生成函数定义的形参表，然后返回，再调用another_generate
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            FuncFParams::FuncFParams(func_fparams) => {
                let mut iter = func_fparams.iter();
                if let Some(func_fparam) = iter.next() {
                    func_fparam.generate(context)?;
                    for func_fparam in iter {
                        context.output_str.push_str(", ");
                        func_fparam.generate(context)?;
                    }
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

impl FuncFParams {
    // 在函数体中为形参分配空间
    fn another_generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            FuncFParams::FuncFParams(func_fparams) => {
                for func_fparam in func_fparams {
                    match func_fparam {
                        FuncFParam::FuncFParam(_, ident, const_exps_option) => {

                            // 插入符号表
                            if const_exps_option.is_some() {
                                let const_exps = const_exps_option.as_ref().unwrap();
                                let mut const_exp_results = Vec::new();
                                for const_exp in const_exps.iter() {
                                    let const_exp_result = const_exp.generate(context)?;
                                    match const_exp_result {
                                        ExpIRGenerateResult::Const(const_val) => {
                                            const_exp_results.push(const_val as usize);
                                        }
                                        _ => {
                                            return Err("Non constant array size in function formal parameter".to_string());
                                        }
                                    }
                                }
                                let mut array_typekind = get_array_typekind(&const_exp_results);
                                array_typekind = TypeKind::Pointer(Type::get(array_typekind));
                                context.ir_symbol_table_stack.insert_new_symbol(
                                    ident.clone(), 
                                    IRSymbolTableEntry::Variable(array_typekind, 0)
                                );

                            } else {
                                context.ir_symbol_table_stack.insert_new_symbol(
                                    ident.clone(), 
                                    IRSymbolTableEntry::Variable(TypeKind::Int32, 0)
                                );
                            }

                            // 生成IR
                            match context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap() {
                                IRSymbolTableEntry::Variable(typekind, index) => {
                                    context.output_str.push_str(
                                        format!(
                                            "    @{}_{} = alloc {}\n", 
                                            ident,
                                            index,
                                            typekind,
                                        ).as_str()
                                    );
                                    context.output_str.push_str(
                                        format!(
                                            "    store %{}, @{}_{}\n", 
                                            ident,
                                            ident,
                                            index,
                                        ).as_str()
                                    );
                                }
                                _ => panic!("SymbolTableEntry is not Variable"),
                            };
                        }
                    }
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

impl IRGenerate for FuncFParam {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            FuncFParam::FuncFParam(_type, ident, const_exps) => {
                context.output_str.push_str(
                    format!("%{}: ", ident).as_str()
                );
                if const_exps.is_some() {
                    context.output_str.push('*');
                }
                if let Some(const_exps) = const_exps {
                    for _ in 0..const_exps.len() {
                        context.output_str.push('[');
                    }
                }
                context.output_str.push_str("i32");
                if let Some(const_exps) = const_exps {
                    for const_exp in const_exps.iter().rev() {
                        let const_exp_result = const_exp.generate(context)?;
                        match const_exp_result {
                            ExpIRGenerateResult::Const(const_val) => {
                                context.output_str.push_str(
                                    format!(
                                        ", {}]", 
                                        const_val
                                    ).as_str()
                                );
                            }
                            _ => {
                                return Err("Non constant array size in function formal parameter".to_string());
                            }
                        }
                    }
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}
