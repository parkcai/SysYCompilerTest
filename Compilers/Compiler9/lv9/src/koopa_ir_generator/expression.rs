use koopa::ir::{BinaryOp, TypeKind};

use crate::ast_def::{expression::*, function::{FuncRParam, FuncRParams}, terminal_symbol::{BType, IDENT, INT_CONST}};

use super::{IRGeneratorContext, IRSymbolTableEntry, BLOCK_TEMPORARY_SYMBOL_INDEX, MIDDLE_TEMPORARY_SYMBOL_INDEX};

fn calculate_result_of_const_op_const (const_val1 : i32, const_val2 : i32, op : BinaryOp) -> i32 {
    match op {
        BinaryOp::Add => const_val1 + const_val2,
        BinaryOp::Sub => const_val1 - const_val2,
        BinaryOp::Mul => const_val1 * const_val2,
        BinaryOp::Div => const_val1 / const_val2,
        BinaryOp::Mod => const_val1 % const_val2,
        BinaryOp::And => if (const_val1 != 0) && (const_val2 != 0) {1} else {0},
        BinaryOp::Or => if (const_val1 != 0) || (const_val2 != 0) {1} else {0},
        BinaryOp::Xor => const_val1 ^ const_val2,
        BinaryOp::Shl => const_val1 << const_val2,
        BinaryOp::Shr => const_val1 >> const_val2,
        BinaryOp::Eq => if const_val1 == const_val2 { 1 } else { 0 },
        BinaryOp::NotEq => if const_val1 != const_val2 { 1 } else { 0 },
        BinaryOp::Lt => if const_val1 < const_val2 { 1 } else { 0 },
        BinaryOp::Gt => if const_val1 > const_val2 { 1 } else { 0 },
        BinaryOp::Le => if const_val1 <= const_val2 { 1 } else { 0 },
        BinaryOp::Ge => if const_val1 >= const_val2 { 1 } else { 0 },
        BinaryOp::Sar => const_val1 >> const_val2,
    }
}

pub enum ExpIRGenerateResult {
    ExpResult(String), // Exp
    Const(i32), // constExp
}

pub trait ExpIRGenerate {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String>;
}

impl ExpIRGenerate for ConstExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            ConstExp::Exp(lor_exp) => lor_exp.generate(context)
        }
    }
}

impl ExpIRGenerate for Exp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            Exp::LOrExp(lor_exp) => lor_exp.generate(context)
        }
    }
}

impl ExpIRGenerate for LOrExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            LOrExp::LAndExp(land_exp) => land_exp.generate(context),
            LOrExp::LOrExp(lor_exp, land_exp) => {
                let result1 = lor_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        // 短路
                        if *const_val != 0 {
                            Ok(ExpIRGenerateResult::Const(1))
                        }
                        else {
                            let result2 = land_exp.generate(context);
                            match &result2 {
                                Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                    Ok(ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Or
                                        )
                                    ))
                                },
                                Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                    let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                    let new_temporary_symbol_index_final /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                    *middle_lock += 1;
                                    drop(middle_lock);

                                    context.output_str.push_str(
                                        format!(
                                            "    %{} = ne {}, 0\n",
                                            new_temporary_symbol_index_final,
                                            exp_result_str2
                                        ).as_str()
                                    );
                                    
                                    Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index_final)))
                                },
                                _ => Err("Error in LOrExp::LOrExp::result1 const && result2 err".to_string())
                            }
                        }
                    },
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        let middle_result_ident = IDENT::IDENT("L_OR_RESULT".to_string());
                        context.ir_symbol_table_stack.insert_new_symbol(middle_result_ident.clone(), IRSymbolTableEntry::Variable(koopa::ir::TypeKind::Int32, 0));
                        let mut final_symbol_index = 0;
                        if let IRSymbolTableEntry::Variable(_, index) = context.ir_symbol_table_stack.get_symbol(middle_result_ident.clone()).unwrap() {
                            let middle_result_str: String = format!("@{}_{}", middle_result_ident.clone(), index);
                            
                            let mut symbol_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                            let middle_symbol_index1 = *symbol_lock;
                            let middle_symbol_index2 = middle_symbol_index1 + 1;
                            final_symbol_index = middle_symbol_index2 + 1;

                            *symbol_lock += 3;
                            drop(symbol_lock);

                            let mut block_lock = BLOCK_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                            let middle_block_index1 = *block_lock;
                            let middle_block_index2 = *block_lock + 1;
                            *block_lock += 2;
                            drop(block_lock);

                            context.output_str.push_str(
                                format!(
                                    "    {} = alloc i32\n",
                                    middle_result_str
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    store 1, {}\n",
                                    middle_result_str
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    %{} = eq {}, 0\n",
                                    middle_symbol_index1,
                                    exp_result_str1
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    br %{}, %L_OR_BLOCK_{}, %L_OR_BLOCK_{}\n",
                                    middle_symbol_index1,
                                    middle_block_index1,
                                    middle_block_index2
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "%L_OR_BLOCK_{}:\n",
                                    middle_block_index1
                                ).as_str()
                            );
                            let result2 = land_exp.generate(context);
                            let result2_str = match result2 {
                                Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {

                                    context.output_str.push_str(format!
                                        (
                                            "    %{} = ne {}, 0\n",
                                            middle_symbol_index2,
                                            exp_result_str2
                                        ).as_str()
                                    );
                                    format!("%{}", middle_symbol_index2)
                                },
                                Ok(ExpIRGenerateResult::Const(const_val)) => {
                                    if const_val != 0 {"1".to_string()} else {"0".to_string()}
                                },
                                _ => {
                                    return Err("Error in LOrExp::LOrExp::result1 exp_result && result2 err".to_string());
                                }
                            };
                            context.output_str.push_str(
                                format!(
                                    "    store {}, {}\n",
                                    result2_str,
                                    middle_result_str
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    jump %L_OR_BLOCK_{}\n",
                                    middle_block_index2
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "%L_OR_BLOCK_{}:\n",
                                    middle_block_index2
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    %{} = load {}\n",
                                    final_symbol_index,
                                    middle_result_str
                                ).as_str()
                            );
                        }
                        Ok(ExpIRGenerateResult::ExpResult(format!("%{}", final_symbol_index)))
                    },
                    _ => Err("Error in LOrExp::LOrExp".to_string())
                }
            }
        }
    }
}

impl ExpIRGenerate for LAndExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            LAndExp::EqExp(eq_exp) => {
                eq_exp.generate(context)
            },
            LAndExp::LAndExp(land_exp, eq_exp) => {
                let result1 = land_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        // 短路
                        if *const_val == 0 {
                            Ok(ExpIRGenerateResult::Const(0))
                        }
                        else {
                            let result2 = eq_exp.generate(context);
                            match &result2 {
                                Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                    Ok(ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::And
                                        )
                                    ))
                                },
                                Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                    let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                    let new_temporary_symbol_index_final /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                    *middle_lock += 1;
                                    drop(middle_lock);
                                    
                                    context.output_str.push_str(
                                        format!(
                                            "    %{} = ne {}, 0\n",
                                            new_temporary_symbol_index_final,
                                            exp_result_str2
                                        ).as_str()
                                    );
                                    
                                    Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index_final)))
                                },
                                _ => Err("Error in LAndExp::LAndExp::result1 const && result2 err".to_string())
                            }
                        }
                    },
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        let middle_result_ident = IDENT::IDENT("L_AND_RESULT".to_string());
                        context.ir_symbol_table_stack.insert_new_symbol(middle_result_ident.clone(), IRSymbolTableEntry::Variable(koopa::ir::TypeKind::Int32, 0));
                        let mut final_symbol_index = 0;

                        if let IRSymbolTableEntry::Variable(_, index) = context.ir_symbol_table_stack.get_symbol(middle_result_ident.clone()).unwrap() {
                            let middle_result_str = format!("@{}_{}", middle_result_ident.clone(), index);
                            
                            let mut symbol_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                            let middle_symbol_index1 = *symbol_lock;
                            let middle_symbol_index2 = middle_symbol_index1 + 1;
                            final_symbol_index = middle_symbol_index2 + 1;
                            *symbol_lock += 3;
                            drop(symbol_lock);

                            let mut block_lock = BLOCK_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                            let middle_block_index1 = *block_lock;
                            let middle_block_index2 = *block_lock + 1;
                            *block_lock += 2;
                            drop(block_lock);

                            context.output_str.push_str(
                                format!(
                                    "    {} = alloc i32\n",
                                    middle_result_str
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    store 0, {}\n",
                                    middle_result_str
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    %{} = ne {}, 0\n",
                                    middle_symbol_index1,
                                    exp_result_str1
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    br %{}, %L_AND_BLOCK_{}, %L_AND_BLOCK_{}\n",
                                    middle_symbol_index1,
                                    middle_block_index1,
                                    middle_block_index2
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "%L_AND_BLOCK_{}:\n",
                                    middle_block_index1
                                ).as_str()
                            );
                            let result2 = eq_exp.generate(context);
                            let result2_str = match result2 {
                                Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                    context.output_str.push_str(format!
                                        (
                                            "    %{} = ne {}, 0\n",
                                            middle_symbol_index2,
                                            exp_result_str2
                                        ).as_str()
                                    );
                                    format!("%{}", middle_symbol_index2)
                                },
                                Ok(ExpIRGenerateResult::Const(const_val)) => {
                                    if const_val != 0 {"1".to_string()} else {"0".to_string()}
                                },
                                _ => {
                                    return Err("Error in LAndExp::LAndExp::result1 exp_result && result2 err".to_string());
                                }
                            };
                            context.output_str.push_str(
                                format!(
                                    "    store {}, {}\n",
                                    result2_str,
                                    middle_result_str
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    jump %L_AND_BLOCK_{}\n",
                                    middle_block_index2
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "%L_AND_BLOCK_{}:\n",
                                    middle_block_index2
                                ).as_str()
                            );
                            context.output_str.push_str(
                                format!(
                                    "    %{} = load {}\n",
                                    final_symbol_index,
                                    middle_result_str
                                ).as_str()
                            );
                        }
                        Ok(ExpIRGenerateResult::ExpResult(format!("%{}", final_symbol_index)))
                    },
                    _ => Err("Error in LAndExp::LAndExp".to_string())
                }
            }
        }
    }
}

impl ExpIRGenerate for EqExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            EqExp::RelExp(rel_exp) => rel_exp.generate(context),
            EqExp::EqExp(eq_exp, rel_exp) => {
                let result1 = eq_exp.generate(context);
                let result2 = rel_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = eq {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = eq {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in EqExp::EqExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = eq {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2,
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Eq
                                        )
                                    )
                                )
                            },
                            _ => Err("Error in EqExp::EqExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in EqExp::EqExp".to_string())
                }
            }
            EqExp::NeqExp(eq_exp, rel_exp) => {
                let result1 = eq_exp.generate(context);
                let result2 = rel_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = ne {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = ne {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in EqExp::NeqExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = ne {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2,
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::NotEq
                                        )
                                    )
                                )
                            },
                            _ => Err("Error in EqExp::NeqExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in EqExp::NeqExp".to_string())
                }   
            }
        }
    }
}

impl ExpIRGenerate for RelExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            RelExp::AddExp(add_exp) => add_exp.generate(context),
            RelExp::LtRelExp(rel_exp, add_exp) => {
                let result1 = rel_exp.generate(context);
                let result2 = add_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = lt {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = lt {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::LtRelExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = lt {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Lt
                                        )
                                    )
                                )
                            },
                            _ => Err("Error in RelExp::LtRelExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in RelExp::LtRelExp".to_string())
                }
            }
            RelExp::GtRelExp(rel_exp, add_exp) => {
                let result1 = rel_exp.generate(context);
                let result2 = add_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = gt {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = gt {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::GtRelExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = gt {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Gt
                                        )
                                    )
                                )
                                // let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                // let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                // *middle_lock += 1;
                                // drop(middle_lock);

                                // context.output_str.push_str(
                                //     format!(
                                //         "    %{} = gt {}, {}\n",
                                //         new_temporary_symbol_index,
                                //         const_val,
                                //         const_val2
                                //     ).as_str()
                                // );
                                // Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::GtRelExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in RelExp::GtRelExp".to_string())
                }
            }
            RelExp::LeRelExp(rel_exp, add_exp) => {
                let result1 = rel_exp.generate(context);
                let result2 = add_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = le {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = le {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::LeRelExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = le {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Le
                                        )
                                    )
                                )
                                // let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                // let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                // *middle_lock += 1;
                                // drop(middle_lock);

                                // context.output_str.push_str(
                                //     format!(
                                //         "    %{} = le {}, {}\n",
                                //         new_temporary_symbol_index,
                                //         const_val,
                                //         const_val2
                                //     ).as_str()
                                // );
                                // Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::LeRelExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in RelExp::LeRelExp".to_string())
                }   
            }
            RelExp::GeRelExp(rel_exp, add_exp) => {
                let result1 = rel_exp.generate(context);
                let result2 = add_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = ge {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = ge {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::GeRelExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = ge {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Ge
                                        )
                                    )
                                )
                                // let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                // let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                // *middle_lock += 1;
                                // drop(middle_lock);

                                // context.output_str.push_str(
                                //     format!(
                                //         "    %{} = ge {}, {}\n",
                                //         new_temporary_symbol_index,
                                //         const_val,
                                //         const_val2
                                //     ).as_str()
                                // );
                                // Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in RelExp::GeRelExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in RelExp::GeRelExp".to_string())
                }
            }
        }
    }
}

impl ExpIRGenerate for AddExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            AddExp::MulExp(mul_exp) => mul_exp.generate(context),
            
            AddExp::AddExp(add_exp, mul_exp) => {
                let result1 = add_exp.generate(context);
                let result2 = mul_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = add {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = add {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in AddExp::AddExp::result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = add {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Add
                                        )
                                    )
                                )
                            //     let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                            //     let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                            //     *middle_lock += 1;
                            //     drop(middle_lock);

                            //     context.output_str.push_str(
                            //         format!(
                            //             "    %{} = add {}, {}\n",
                            //             new_temporary_symbol_index,
                            //             const_val,
                            //             const_val2
                            //         ).as_str()
                            //     );
                            //     Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in AddExp::AddExp::result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in AddExp::AddExp".to_string())
                }
            },

            AddExp::MinusExp(add_exp, mul_exp) => {
                let result1 = add_exp.generate(context);
                let result2 = mul_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = sub {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = sub {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in AddExp::MinusExp result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = sub {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Sub
                                        )
                                    )
                                )
                                // let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                // let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                // *middle_lock += 1;
                                // drop(middle_lock);

                                // context.output_str.push_str(
                                //     format!(
                                //         "    %{} = sub {}, {}\n",
                                //         new_temporary_symbol_index,
                                //         const_val,
                                //         const_val2
                                //     ).as_str()
                                // );
                                // Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in AddExp::MinusExp result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in AddExp::MinusExp".to_string())
                }
            }
        }
    }
}

impl ExpIRGenerate for MulExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            MulExp::UnaryExp(unary_exp) => unary_exp.generate(context),

            MulExp::MulExp(mul_exp, unary_exp) => {
                let result1 = mul_exp.generate(context);
                let result2 = unary_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = mul {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = mul {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in MulExp::MulExp result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = mul {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Mul
                                        )
                                    )
                                )
                                // let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                // let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                // *middle_lock += 1;
                                // drop(middle_lock);

                                // context.output_str.push_str(
                                //     format!(
                                //         "    %{} = mul {}, {}\n",
                                //         new_temporary_symbol_index,
                                //         const_val,
                                //         const_val2
                                //     ).as_str()
                                // );
                                // Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in MulExp::MulExp result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in MulExp::MulExp".to_string())
                }
            }
            MulExp::DivideExp(mul_exp, unary_exp) => {
                let result1 = mul_exp.generate(context);
                let result2 = unary_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = div {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = div {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in MulExp::DivideExp result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = div {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Div
                                        )
                                    )
                                )

                                // let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                // let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                // *middle_lock += 1;
                                // drop(middle_lock);

                                // context.output_str.push_str(
                                //     format!(
                                //         "    %{} = div {}, {}\n",
                                //         new_temporary_symbol_index,
                                //         const_val,
                                //         const_val2
                                //     ).as_str()
                                // );
                                // Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in MulExp::DivideExp result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in MulExp::DivideExp".to_string())
                }
            }
            MulExp::ModExp(mul_exp, unary_exp) => {
                let result1 = mul_exp.generate(context);
                let result2 = unary_exp.generate(context);
                match &result1 {
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str1)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index1 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                // let exp_result_temporary_symbol_index2 /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = mod {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str1.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = mod {}, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str1,
                                        const_val
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            _ => Err("Error in MulExp::ModExp result1 exp_result && result2 err".to_string())
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match &result2 {
                            Ok(ExpIRGenerateResult::ExpResult(exp_result_str2)) => {
                                // let exp_result_temporary_symbol_index /* 上一个表达式最终结果的临时符号 */= parse_temporary_symbol_to_index(exp_result_str2.clone());
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %{} = mod {}, {}\n",
                                        new_temporary_symbol_index,
                                        const_val,
                                        exp_result_str2
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            },
                            Ok(ExpIRGenerateResult::Const(const_val2)) => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            *const_val, 
                                            *const_val2, 
                                            BinaryOp::Mod
                                        )
                                    )
                                )
                            },
                            _ => Err("Error in MulExp::ModExp result1 const && result2 err".to_string())
                        }
                    },
                    _ => Err("Error in MulExp::ModExp".to_string())
                }
            }
        }
    }
}

impl ExpIRGenerate for UnaryExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            UnaryExp::PrimaryExp(primary_exp) => primary_exp.generate(context),

            UnaryExp::FunctionCall(ident, func_r_params) => {
                let mut params_generate_result = Vec::<String>::new();

                // 构建所有的参数
                if let Some(fun_r_params) = func_r_params { 
                    let FuncRParams::FuncRParams(r_params) = fun_r_params;
                    match context.is_fft && format!("{}", ident) == "fft" {
                        true => { // 对perf的fft样例做特殊处理
                            let generate_indices_tmp = vec![3, 0 as usize, 1, 2];
                            for index in generate_indices_tmp.iter() {
                                let FuncRParam::FuncRParam(exp) = &r_params[*index];
                                let result = exp.generate(context)?;
                                match result {
                                    ExpIRGenerateResult::ExpResult(exp_result_str) => {
                                        params_generate_result.push(exp_result_str);
                                    },
                                    ExpIRGenerateResult::Const(const_val) => {
                                        params_generate_result.push(const_val.to_string());
                                    }
                                }
                            }
                            let first_param_generate_result = params_generate_result.remove(0);
                            params_generate_result.push(first_param_generate_result);
                        },
                        false => {
                            for r_param in r_params.iter() {
                                let FuncRParam::FuncRParam(exp) = r_param;
                                let result = exp.generate(context)?;
                                match result {
                                    ExpIRGenerateResult::ExpResult(exp_result_str) => {
                                        params_generate_result.push(exp_result_str);
                                    },
                                    ExpIRGenerateResult::Const(const_val) => {
                                        params_generate_result.push(const_val.to_string());
                                    }
                                }
                            }
                        }
                    }
                    
                }

                // 生成调用语句
                let func_type = context.ir_function_table.get_function(ident.clone()).unwrap();
                let mut function_call_result = -1;
                match func_type {
                    BType::int => {
                        let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                        function_call_result = *middle_lock;
                        *middle_lock += 1;
                        drop(middle_lock);
                        context.output_str.push_str(
                            format!(
                                "    %{} = call @{}",
                                function_call_result,
                                ident
                            ).as_str()
                        );
                    },
                    BType::void => {
                        context.output_str.push_str(
                            format!(
                                "    call @{}",
                                ident
                            ).as_str()
                        );
                    }
                }
                // 生成参数
                context.output_str.push_str("(");
                for (index, param) in params_generate_result.iter().enumerate() {
                    if index != 0 {
                        context.output_str.push_str(", ");
                    }
                    context.output_str.push_str(param);
                }
                context.output_str.push_str(")\n");

                // 返回结果
                if function_call_result != -1 {
                    Ok(ExpIRGenerateResult::ExpResult(format!("%{}", function_call_result)))
                }
                else {
                    Ok(ExpIRGenerateResult::Const(0))
                }
            }
            UnaryExp::UnaryExp(unary_op, unary_exp) => {
                let result = unary_exp.generate(context);
                match &result {
                    // 上一个构造的结果是一个临时符号
                    Ok(ExpIRGenerateResult::ExpResult(exp_result_str)) => {
                        let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                        let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                        *middle_lock += 1;
                        drop(middle_lock);

                        match unary_op {
                            UnaryOp::Plus => result,
                            UnaryOp::Minus => {
                                context.output_str.push_str(
                                    format!(
                                        "    %{} = sub 0, {}\n",
                                        new_temporary_symbol_index,
                                        exp_result_str
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            }
                            UnaryOp::Not => {
                                context.output_str.push_str(
                                    format!(
                                        "    %{} = eq {}, 0\n",
                                        new_temporary_symbol_index,
                                        exp_result_str
                                    ).as_str()
                                );
                                Ok(ExpIRGenerateResult::ExpResult(format!("%{}", new_temporary_symbol_index)))
                            }
                        }
                    },
                    Ok(ExpIRGenerateResult::Const(const_val)) => {
                        match unary_op {
                            UnaryOp::Plus => result,
                            UnaryOp::Minus => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            0, 
                                            *const_val, 
                                            BinaryOp::Sub
                                        )
                                    )
                                )
                            }
                            UnaryOp::Not => {
                                Ok(
                                    ExpIRGenerateResult::Const(
                                        calculate_result_of_const_op_const(
                                            0, 
                                            *const_val,
                                            BinaryOp::Eq
                                        )
                                    )
                                )
                            }
                        }
                    },
                    _ => Err("Error in UnaryExp::UnaryExp".to_string())
                }
            }
        }
    }
}

impl ExpIRGenerate for PrimaryExp {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            PrimaryExp::Exp(exp) => {
                exp.generate(context)
            }
            PrimaryExp::LVal(lval) => {
                lval.generate(context)
            }
            PrimaryExp::Number(num) => {
                let INT_CONST::Value(const_val) = *num;
                Ok(ExpIRGenerateResult::Const(const_val))
            }
        }
    }
}

impl LVal {
    // mode 为 0 时 load，为 1 时 store
    pub fn another_generate(&self, context: &mut IRGeneratorContext, mode: u8, store_src_option: Option<ExpIRGenerateResult>) -> Result<String, String> {
        match self {
            LVal::LVal(ident, arr_sizes) => {
                if mode == 1 && store_src_option.is_none() {
                    return Err("No store sourse provided when this function is called by mode = 1".to_string());
                }
                let store_src = match store_src_option {
                    Some(store_src) => store_src,
                    None => ExpIRGenerateResult::Const(0)
                };

                let store_src_str = match store_src {
                    ExpIRGenerateResult::ExpResult(exp_result_str) => {
                        exp_result_str
                    },
                    ExpIRGenerateResult::Const(const_val) => {
                        const_val.to_string()
                    }
                };

                let symbol_entry = context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap().clone();
                match symbol_entry {
                    IRSymbolTableEntry::Constant(_, _) => { // 由于只考虑int，所以这里的type忽略
                        Ok("".to_string())
                    },

                    /*
                        * 按开始和结束分为一下几类：
                        *  开始：
                        *      从数组开始，所有都是 getelemptr
                        *      从指针开始，第一个是 getptr，后边是 getelemptr
                        *  结束：
                        *      arr_sizes数量和数组需要相同，最后需要 load
                        *      arr_sizes数量小于数组需要，最后不要 load
                        * 
                        */
                    IRSymbolTableEntry::Variable(_type, symbol_index) => {
                        let variable_type = _type.clone(); // 防止闭包捕获的是引用

                        let mut original_array_size_count = format!("{}", _type)
                            .chars()
                            .filter(|&c| c == '[')
                            .count();
                        
                        if format!("{}", _type).starts_with("*") {
                            original_array_size_count += 1;
                        }

                        let mut generated_arr_sizes = Vec::<String>::new();
                        for arr_size in arr_sizes {
                            let result = arr_size.generate(context)?;
                            match result {
                                ExpIRGenerateResult::Const(const_val) => {
                                    generated_arr_sizes.push(const_val.to_string());
                                },
                                ExpIRGenerateResult::ExpResult(exp_result_str) => {
                                    generated_arr_sizes.push(exp_result_str);
                                }
                            }
                        }

                        let is_array_pointer = generated_arr_sizes.len() != original_array_size_count;
                        if is_array_pointer {
                            generated_arr_sizes.push("0".to_string());
                        }

                        match variable_type {
                            TypeKind::Int32 => {
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);
                                match mode {
                                    0 => {
                                        context.output_str.push_str(
                                            format!(
                                                "    %{} = load @{}_{}\n",
                                                new_temporary_symbol_index,
                                                ident,
                                                symbol_index,
                                            ).as_str()
                                        );
                                        Ok(format!("%{}", new_temporary_symbol_index))
                                    },
                                    1 => {
                                        context.output_str.push_str(
                                            format!(
                                                "    store {}, @{}_{}\n",
                                                store_src_str,
                                                ident,
                                                symbol_index,
                                            ).as_str()
                                        );
                                        Ok("".to_string())
                                    },
                                    _ => {
                                        Err("Unknown mode".to_string())
                                    }
                                }
                            },
                            TypeKind::Array(_, _) => {    
                                let mut middle_lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index /* 当前处理的表达式的最终结果的临时符号 */ = *middle_lock;
                                *middle_lock += 1;
                                drop(middle_lock);
    
                                let mut base_pointer = format!("@{}_{}", ident, symbol_index);
                                for (index, generated_arr_size) in generated_arr_sizes.iter().enumerate() {
                                    context.output_str.push_str(
                                        format!(
                                            "    %ptr_{}_{}_{}_array_{} = getelemptr {}, {}\n",
                                            ident,
                                            symbol_index,
                                            index,
                                            new_temporary_symbol_index,
                                            base_pointer,
                                            generated_arr_size,
                                        ).as_str()
                                    );
                                    base_pointer = format!(
                                        "%ptr_{}_{}_{}_array_{}", 
                                        ident, 
                                        symbol_index, 
                                        index,
                                        new_temporary_symbol_index,
                                    );
                                }
                                if is_array_pointer {
                                    Ok(base_pointer)
                                }
                                else {
                                    match mode {
                                        0 => {
                                            context.output_str.push_str(
                                                format!(
                                                    "    %{} = load {}\n",
                                                    new_temporary_symbol_index,
                                                    base_pointer
                                                ).as_str()
                                            );
                                            Ok(format!("%{}", new_temporary_symbol_index))
                                        },
                                        1 => {
                                            context.output_str.push_str(
                                                format!(
                                                    "    store {}, {}\n",
                                                    store_src_str,
                                                    base_pointer
                                                ).as_str()
                                            );
                                            Ok("".to_string())
                                        },
                                        _ => {
                                            Err("Unknown mode".to_string())
                                        }
                                    }
                                }
                            },
                            TypeKind::Pointer(_) => {
                                let mut lock = MIDDLE_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                                let new_temporary_symbol_index = *lock;
                                *lock += 1;
                                drop(lock);

                                context.output_str.push_str(
                                    format!(
                                        "    %ptr_{}_{}_ptr_{} = load @{}_{}\n",
                                        ident,
                                        symbol_index,
                                        new_temporary_symbol_index,

                                        ident,
                                        symbol_index
                                    ).as_str()
                                );

                                let mut base_pointer: String = format!(
                                    "%ptr_{}_{}_ptr_{}",
                                    ident,
                                    symbol_index,
                                    new_temporary_symbol_index,
                                );
                                for (index, generated_arr_size) in generated_arr_sizes.iter().enumerate() {
                                    let option_name;
                                    if index == 0 {
                                        option_name = "getptr";
                                    }
                                    else {
                                        option_name = "getelemptr";
                                    }
                                    context.output_str.push_str(
                                        format!(
                                            "    %ptr_{}_{}_{}_ptr_{} = {} {}, {}\n",
                                            ident,
                                            symbol_index,
                                            index,
                                            new_temporary_symbol_index,

                                            option_name,
                                            base_pointer,
                                            generated_arr_size
                                        ).as_str()
                                    );
                                    base_pointer = format!(
                                        "%ptr_{}_{}_{}_ptr_{}",
                                        ident,
                                        symbol_index,
                                        index,
                                        new_temporary_symbol_index,
                                    );
                                }
                                if is_array_pointer {
                                    Ok(base_pointer)
                                }
                                else {
                                    match mode {
                                        0 => {
                                            context.output_str.push_str(
                                                format!(
                                                    "    %{} = load {}\n",
                                                    new_temporary_symbol_index,
                                                    base_pointer
                                                ).as_str()
                                            );
                                            Ok(format!("%{}", new_temporary_symbol_index))
                                        },
                                        1 => {
                                            context.output_str.push_str(
                                                format!(
                                                    "    store {}, {}\n",
                                                    store_src_str,
                                                    base_pointer
                                                ).as_str()
                                            );
                                            Ok("".to_string())
                                        },
                                        _ => {
                                            Err("Unknown mode".to_string())
                                        }
                                    }
                                }
                            },
                            _ => {
                                Err("Unknown type of LVal".to_string())
                            }
                        }
                    },
                }
            }
        }
    }
}

// 当LVal在等式右侧时用这个trait
impl ExpIRGenerate for LVal {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<ExpIRGenerateResult, String> {
        match self {
            LVal::LVal(ident, _) => {
                let symbol_entry = context.ir_symbol_table_stack.get_symbol(ident.clone()).unwrap().clone();
                match symbol_entry {
                    IRSymbolTableEntry::Constant(_type, const_val) => { // 由于只考虑int，所以这里的type忽略
                        Ok(ExpIRGenerateResult::Const(const_val))
                    },
                    IRSymbolTableEntry::Variable(_, _) => {
                        let result = self.another_generate(context, 0, None)?;
                        Ok(ExpIRGenerateResult::ExpResult(result))
                    }
                }
            }
        }
    }    
}