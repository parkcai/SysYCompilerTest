use crate::ast_def::statement::*;
use super::{IRGenerate, IRGenerateResult, IRGeneratorContext, BLOCK_TEMPORARY_SYMBOL_INDEX};
use super::expression::{ExpIRGenerate, ExpIRGenerateResult};

impl IRGenerate for Stmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            // Stmt::EmptyStmt => Ok(IRGenerateResult::Ok),
            Stmt::ExpStmt(exp_stmt) => exp_stmt.generate(context),
            Stmt::AssignStmt(assign_stmt) => assign_stmt.generate(context),
            Stmt::BlockStmt(block_stmt) => block_stmt.generate(context),
            Stmt::IfStmt(if_stmt) => if_stmt.generate(context),
            Stmt::WhileStmt(while_stmt) => while_stmt.generate(context),
            Stmt::BreakStmt(break_stmt) => break_stmt.generate(context),
            Stmt::ContinueStmt(continue_stmt) => continue_stmt.generate(context),
            Stmt::ReturnStmt(return_stmt) => return_stmt.generate(context),
        }
    }
}

impl IRGenerate for ReturnStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            ReturnStmt::ReturnStmt(exp) => {
                if let Some(exp) = exp {
                    let generate_result = exp.generate(context)?;
                    match generate_result {
                        ExpIRGenerateResult::ExpResult(str_result) => {
                            context.output_str.push_str(format!("    ret {}\n", str_result).as_str());
                        }
                        ExpIRGenerateResult::Const(int_result) => {
                            context.output_str.push_str(format!("    ret {}\n", int_result).as_str());
                        }
                    };
                }
                else {
                    context.output_str.push_str("    ret\n");
                }
            }
        }
        Ok(IRGenerateResult::EarlyTermination)
    }
}

// 当LVal在等式左侧时用这个trait
impl IRGenerate for AssignStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            AssignStmt::AssignStmt(lval, exp) => {
                let exp_result = exp.generate(context)?;
                match lval.another_generate(context, 1, Some(exp_result)) {
                    Ok(_) => Ok(IRGenerateResult::Ok),
                    Err(err) => { return Err(err); }
                }
            }
        }
    }
}

impl IRGenerate for ExpStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            ExpStmt::ExpStmt(exp) => {
                if let Some(exp) = exp {
                    exp.generate(context)?;
                }
            }
        }
        Ok(IRGenerateResult::Ok)
    }
}

impl IRGenerate for IfStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            IfStmt::IfStmt(exp, if_stmt, else_stmt) => {
                let exp_result = exp.generate(context)?;
                let has_else = else_stmt.is_some();
                let mut lock = BLOCK_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                let block_index = *lock;
                *lock += 1;
                drop(lock);

                // 构造if条件判断，br %exp_result, %then_{block_index}, %else_{block_index}
                match exp_result {
                    ExpIRGenerateResult::ExpResult(exp_str) => {
                        context.output_str.push_str(
                            format!(
                                "    br {}, %then_{},", 
                                exp_str, 
                                block_index
                            ).as_str()
                        );
                    }
                    ExpIRGenerateResult::Const(const_val) => {
                        context.output_str.push_str(
                            format!(
                                "    br {}, %then_{},", 
                                const_val, 
                                block_index
                            ).as_str()
                        );
                    }
                }
                if has_else {
                    context.output_str.push_str(
                        format!(
                            " %else_{}\n", 
                            block_index
                        ).as_str()
                    );
                } else {
                    context.output_str.push_str(
                        format!(
                            " %end_{}\n", 
                            block_index
                        ).as_str()
                    );
                }

                // 构造if条件为true的block
                context.output_str.push_str(
                    format!(
                        "%then_{}:\n", 
                        block_index
                    ).as_str()
                );
                match if_stmt.generate(context)? {
                    IRGenerateResult::Ok => {
                        context.output_str.push_str(
                            format!(
                                "    jump %end_{}\n", 
                                block_index
                            ).as_str()
                        );
                    },
                    IRGenerateResult::EarlyTermination => {},
                }

                if has_else {
                    context.output_str.push_str(
                        format!(
                            "%else_{}:\n", 
                            block_index
                        ).as_str()
                    );
                    match else_stmt.as_ref().unwrap().generate(context)? {
                        IRGenerateResult::Ok => {
                            context.output_str.push_str(
                                format!(
                                    "    jump %end_{}\n", 
                                    block_index
                                ).as_str()
                            );
                        },
                        IRGenerateResult::EarlyTermination => {},
                    }
                }

                context.output_str.push_str(format!("%end_{}:\n", block_index).as_str());
            }
        }
        
        Ok(IRGenerateResult::Ok)
    }
}

impl IRGenerate for WhileStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            WhileStmt::WhileStmt(exp, stmt) => {
                let mut lock = BLOCK_TEMPORARY_SYMBOL_INDEX.lock().unwrap();
                let block_index = *lock;
                *lock += 1;
                drop(lock);

                context.output_str.push_str(
                    format!(
                        "    jump %while_entry_{}\n", 
                        block_index
                    ).as_str()
                );
                context.break_and_continue_target_stack.push(
                    format!("%while_end_{}\n", block_index),
                    format!("%while_entry_{}\n", block_index)
                );
                context.output_str.push_str(
                    format!(
                        "%while_entry_{}:\n", 
                        block_index
                    ).as_str()
                );
                let exp_result = exp.generate(context)?;
                match exp_result {
                    ExpIRGenerateResult::ExpResult(exp_str) => {
                        context.output_str.push_str(
                            format!(
                                "    br {}, %while_body_{}, %while_end_{}\n", 
                                exp_str, 
                                block_index, 
                                block_index
                            ).as_str()
                        );
                    }
                    ExpIRGenerateResult::Const(const_val) => {
                        context.output_str.push_str(
                            format!(
                                "    br {}, %while_body_{}, %while_end_{}\n", 
                                const_val, 
                                block_index, 
                                block_index
                            ).as_str()
                        );
                    }
                }

                context.output_str.push_str(
                    format!(
                        "%while_body_{}:\n", 
                        block_index
                    ).as_str()
                );
                match stmt.generate(context)? {
                    IRGenerateResult::Ok => {
                        context.output_str.push_str(
                            format!(
                                "    jump %while_entry_{}\n", 
                                block_index
                            ).as_str()
                        );
                    },
                    IRGenerateResult::EarlyTermination => {},
                }

                context.output_str.push_str(
                    format!(
                        "%while_end_{}:\n", 
                        block_index
                    ).as_str()
                );
                context.break_and_continue_target_stack.pop();
                Ok(IRGenerateResult::Ok)
            }
        }
    }
}

impl IRGenerate for BlockStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            BlockStmt::BlockStmt(block) => {
                context.ir_symbol_table_stack.push();
                block.generate(context)
            }
        }
    }
}

impl IRGenerate for BreakStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match context.break_and_continue_target_stack.get_break_target() {
            Some(break_target) => {
                context.output_str.push_str(
                    format!(
                        "    jump {}\n", 
                        break_target
                    ).as_str()
                );
            }
            None => {
                Err("Break statement not in loop".to_string())?;
            }
        }
        Ok(IRGenerateResult::EarlyTermination)
    }
}

impl IRGenerate for ContinueStmt {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match context.break_and_continue_target_stack.get_continue_target() {
            Some(continue_target) => {
                context.output_str.push_str(format!("    jump {}\n", continue_target).as_str());
            }
            None => {
                Err("Continue statement not in loop".to_string())?;
            }
        }
        Ok(IRGenerateResult::EarlyTermination)
    }
}