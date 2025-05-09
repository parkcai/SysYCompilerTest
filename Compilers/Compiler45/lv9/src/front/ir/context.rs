use crate::front::ir::scope::Scope;
use crate::front::ir::ParseError;
use crate::{add_inst, new_bb, new_value};
use koopa::ir::builder::{BasicBlockBuilder, LocalInstBuilder, ValueBuilder};
use koopa::ir::{BasicBlock, Function, FunctionData, Program, TypeKind, ValueKind};
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct WhileInfo {
    pub start_bb: BasicBlock,
    pub end_bb: BasicBlock,
}

/// Context of the IR generation
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

impl Context {
    pub fn new(scope: Scope) -> Self {
        Self {
            program: Program::new(),
            func: None,
            scope,
            current_bb: None,
            max_basic_block_id: 0,
            max_temp_value_id: 0,
            while_info: vec![],
            func_table: HashMap::new(),
        }
    }

    pub fn new_bb(&mut self) -> Result<BasicBlock, ParseError> {
        self.max_basic_block_id += 1;
        self.func.ok_or(ParseError::FunctionNotFound).map(|func| {
            let func_data = self.program.func_mut(func);
            new_bb!(func_data).basic_block(Some(format!("%bb{}", self.max_basic_block_id)))
        })
    }

    pub fn get_func(&self) -> Result<Function, ParseError> {
        self.func.ok_or(ParseError::FunctionNotFound)
    }

    pub fn get_bb(&self) -> Result<BasicBlock, ParseError> {
        self.current_bb.ok_or(ParseError::BasicBlockNotFound)
    }

    pub fn program(self) -> Program {
        self.program
    }

    pub fn func_data(&self) -> Result<&FunctionData, ParseError> {
        let func = self.get_func()?;
        Ok(self.program.func(func))
    }

    pub fn func_data_mut(&mut self) -> Result<&mut FunctionData, ParseError> {
        let func = self.get_func()?;
        Ok(self.program.func_mut(func))
    }

    pub fn is_global(&self) -> bool {
        self.func.is_none()
    }

    /// Delete unused basic block and link the basic block
    pub fn delete_and_link(&mut self) {
        for (_, func_data) in self.program.funcs_mut() {
            if func_data.dfg().bbs().is_empty() {
                continue;
            }

            let return_type = match func_data.ty().kind() {
                TypeKind::Function(_, ret) => ret.clone(),
                _ => unreachable!(),
            };

            // get all jump and branch target
            let mut target = vec![];
            for (_, bb_node) in func_data.layout().bbs() {
                for value in bb_node.insts().keys() {
                    let value_data = func_data.dfg().value(*value);
                    match value_data.kind() {
                        ValueKind::Branch(branch) => {
                            target.push(branch.true_bb());
                            target.push(branch.false_bb());
                        }
                        ValueKind::Jump(jump) => {
                            target.push(jump.target());
                        }
                        _ => {}
                    }
                }
            }
            let entry_bb = func_data.layout().entry_bb().unwrap();
            if !target.contains(&entry_bb) {
                target.push(entry_bb);
            }

            // delete empty basic block
            let mut bb_to_delete = vec![];
            for (bb_id, bb_node) in func_data.layout().bbs() {
                if bb_node.insts().is_empty() && !target.contains(&bb_id) {
                    bb_to_delete.push(*bb_id);
                }
            }
            for bb_id in bb_to_delete {
                func_data.layout_mut().bbs_mut().remove(&bb_id);
            }

            let bbs = func_data.layout().bbs();
            let mut jumps = vec![];
            let mut rets = vec![];

            // add jump and ret instruction
            for i in 0..bbs.len() {
                let bb = bbs.keys().nth(i).unwrap();
                let bb_node = bbs.node(bb).unwrap();
                if let Some(last_inst) = bb_node.insts().keys().last() {
                    let last_inst = func_data.dfg().value(*last_inst);
                    match last_inst.kind() {
                        ValueKind::Branch(_) | ValueKind::Return(_) | ValueKind::Jump(_) => {
                            continue;
                        }
                        _ => {}
                    }
                }
                if let Some(next_bb) = bbs.keys().nth(i + 1) {
                    jumps.push((*bb, *next_bb));
                } else {
                    rets.push(*bb);
                }
            }

            jumps.iter().for_each(|(a, b)| {
                let jump = new_value!(func_data).jump(*b);
                add_inst!(func_data, *a, jump);
            });
            rets.iter().for_each(|bb| {
                let ret = match return_type.kind() {
                    TypeKind::Int32 => {
                        let default_value = new_value!(func_data).integer(0);
                        new_value!(func_data).ret(Some(default_value))
                    }
                    TypeKind::Unit => new_value!(func_data).ret(None),
                    _ => unreachable!(),
                };
                add_inst!(func_data, *bb, ret);
            });

            let entry_bb = func_data.layout().entry_bb().unwrap();
            let new_entry_bb = new_bb!(func_data).basic_block(Some("%entry".to_string()));
            func_data
                .layout_mut()
                .bbs_mut()
                .push_key_front(new_entry_bb)
                .unwrap();
            let jump = new_value!(func_data).jump(entry_bb);
            add_inst!(func_data, new_entry_bb, jump);
        }
    }

    /// Whether the block is end with a jump, branch or return instruction
    pub fn block_ended(&mut self, bb: BasicBlock) -> Result<bool, ParseError> {
        let inst = self
            .func_data_mut()?
            .layout_mut()
            .bb_mut(bb)
            .insts()
            .back_key()
            .copied();
        if let Some(inst) = inst {
            let inst = self.func_data()?.dfg().value(inst);
            match inst.kind() {
                ValueKind::Branch(_) | ValueKind::Jump(_) | ValueKind::Return(_) => Ok(true),
                _ => Ok(false),
            }
        } else {
            Ok(false)
        }
    }

    /// If a block is not ended, end the block with a jump instruction
    pub fn end_block(&mut self, bb: BasicBlock, target: BasicBlock) -> Result<(), ParseError> {
        if !self.block_ended(bb)? {
            let func_data = self.func_data_mut()?;
            let jump = new_value!(func_data).jump(target);
            add_inst!(func_data, bb, jump);
        }
        Ok(())
    }

    pub fn temp_value_name(&mut self) -> String {
        self.max_temp_value_id += 1;
        format!("@__t{}", self.max_temp_value_id)
    }

    pub fn add_while_info(&mut self, start: BasicBlock, end: BasicBlock) {
        self.while_info.push(WhileInfo {
            start_bb: start,
            end_bb: end,
        });
    }

    pub fn get_while_info(&self) -> Option<WhileInfo> {
        self.while_info.last().cloned()
    }

    pub fn pop_while_info(&mut self) {
        self.while_info.pop();
    }
}
