use crate::back::array::get_init_list;
use crate::back::asm::*;
use crate::back::context::{FuncContext, GlobalContext, MAX_REG_PARAM};
use crate::back::inst::*;
use crate::back::o1::is_pow_of_two;
use koopa::ir::layout::BasicBlockNode;
use koopa::ir::{BinaryOp, FunctionData, Program, TypeKind, ValueKind};
use std::cmp::max;

pub trait GenerateAsm {
    type Output;
    fn generate(&self, func_ctx: &mut Option<FuncContext>, program: &Program) -> Self::Output;
}

impl GenerateAsm for Program {
    type Output = AsmProgram;

    fn generate(&self, func_ctx: &mut Option<FuncContext>, program: &Program) -> Self::Output {
        let mut asm_program = AsmProgram::new();
        let mut global_ctx = GlobalContext::new();
        for &data in self.inst_layout() {
            let value_data = self.borrow_value(data);
            match value_data.kind() {
                ValueKind::GlobalAlloc(alloc) => {
                    let init = alloc.init();
                    let init_data = self.borrow_value(init);
                    let label = value_data.name().clone().unwrap();
                    let label = label.strip_prefix("@").unwrap().to_string();
                    match init_data.kind() {
                        ValueKind::Integer(c) => {
                            let init_value = c.value();
                            global_ctx.alloc_slot(&*value_data, label.clone());
                            asm_program.add_data(AsmData {
                                name: label.clone(),
                                size: 4,
                                init: vec![init_value],
                            });
                        }
                        ValueKind::ZeroInit(z) => {
                            global_ctx.alloc_slot(&*value_data, label.clone());
                            let size = init_data.ty().size();
                            asm_program.add_data(AsmData {
                                name: label.clone(),
                                size: size as i32,
                                init: Vec::new(),
                            });
                        }
                        ValueKind::Aggregate(agg) => {
                            let init_list = get_init_list(program, agg.clone());
                            global_ctx.alloc_slot(&*value_data, label.clone());
                            let size = init_data.ty().size();
                            asm_program.add_data(AsmData {
                                name: label.clone(),
                                size: size as i32,
                                init: init_list,
                            });
                        }
                        _ => unreachable!(),
                    };
                }
                _ => {
                    unreachable!()
                }
            }
        }
        for &func in self.func_layout() {
            let mut func_ctx = FuncContext::new(func);
            func_ctx.add_global_context(global_ctx.clone());
            match self.func(func).generate(&mut Some(func_ctx), &self) {
                Some(asm_func) => {
                    asm_program.add_func(asm_func);
                }
                None => continue,
            }
        }
        asm_program
    }
}

impl GenerateAsm for FunctionData {
    type Output = Option<AsmFunc>;
    fn generate(&self, func_ctx: &mut Option<FuncContext>, program: &Program) -> Self::Output {
        let func_name = self.name().to_string();
        let args: Vec<String> = Vec::new();
        let block_list = self.layout().bbs();

        let mut asm_func = AsmFunc {
            name: func_name.clone(),
            args,
            block: Vec::new(),
        };

        let entry_bb = self.layout().entry_bb();

        if entry_bb.is_none() {
            return None;
        }

        for (_block, node) in block_list {
            let inst_list = node.insts().keys();
            for inst in inst_list {
                let value_data = self.dfg().value(*inst);
                match value_data.kind() {
                    ValueKind::Call(call) => {
                        let arg_num = call.args().len() as i32;
                        func_ctx.as_mut().unwrap().max_arg_num =
                            max(func_ctx.as_ref().unwrap().max_arg_num, arg_num);
                    }
                    _ => {}
                }
            }
        }

        let if_call = func_ctx.as_ref().unwrap().max_arg_num >= 0;
        func_ctx.as_mut().unwrap().set_slot_for_args();

        for (_block, node) in block_list {
            let inst_list = node.insts().keys();
            for inst in inst_list {
                let value_data = self.dfg().value(*inst);
                match value_data.kind() {
                    ValueKind::Load(_)
                    | ValueKind::Binary(_)
                    | ValueKind::Call(_)
                    | ValueKind::GetElemPtr(_)
                    | ValueKind::GetPtr(_) => {
                        func_ctx.as_mut().unwrap().alloc_slot(value_data);
                    }
                    ValueKind::Alloc(_) => {
                        func_ctx.as_mut().unwrap().alloc_slot_alloc(value_data);
                    }
                    _ => {}
                }
            }
        }

        if func_ctx.as_mut().unwrap().max_arg_num > -1 {
            func_ctx.as_mut().unwrap().alloc_size += 4usize;
        }

        func_ctx.as_mut().unwrap().alloc_size =
            (func_ctx.as_mut().unwrap().alloc_size + 15) / 16 * 16;

        let params = self.params();
        for (i, param) in params.iter().enumerate() {
            let value_data = self.dfg().value(*param);
            func_ctx
                .as_mut()
                .unwrap()
                .alloc_slot_for_arg(value_data, i as i32);
        }

        for (block, node) in block_list {
            let name = self.dfg().bb(*block).name().clone().unwrap();
            if let Some(ctx) = func_ctx.as_mut() {
                ctx.block_name = name;
            } else {
                panic!("func_ctx is None"); // This should never happen
            }

            asm_func.add_block(node.generate(func_ctx, program));
        }

        let prologue = func_ctx.as_ref().unwrap().alloc_size as i32;

        asm_func.block[0].add_prologue();

        asm_func.block[0].name = func_name.strip_prefix("@").unwrap().to_string();

        if prologue > 0 {
            for asm_block in &mut asm_func.block {
                asm_block.set_prologue(prologue, if_call);
                asm_block.set_epilogue(prologue, if_call);
            }
        };

        Some(asm_func)
    }
}

impl GenerateAsm for BasicBlockNode {
    type Output = AsmBlock;
    fn generate(&self, func_ctx: &mut Option<FuncContext>, program: &Program) -> Self::Output {
        let mut name = func_ctx.as_ref().unwrap().block_name.clone();
        let name = name.strip_prefix("%").unwrap().to_string();

        let mut asm_block = AsmBlock::new(name);

        let inst_list = self.insts().keys();
        let func = func_ctx.as_ref().unwrap().func;
        let func_data = program.func(func);
        for inst in inst_list {
            let value_data = func_data.dfg().value(*inst);
            match value_data.kind() {
                ValueKind::Return(ret) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    match ret.value() {
                        Some(value) => {
                            let value_data = func_data.dfg().value(value);
                            match value_data.kind() {
                                ValueKind::Integer(c) => {
                                    inst_vec.push(Inst::Li {
                                        rd: A0,
                                        imm: c.value(),
                                    });
                                    asm_block.add_inst(inst_vec);

                                    asm_block.add_inst(vec![Inst::Epilogue]);

                                    asm_block.add_inst(vec![Inst::Ret]);
                                }
                                ValueKind::GlobalAlloc(ga) => {
                                    let address = func_ctx
                                        .as_ref()
                                        .unwrap()
                                        .get_address(value_data)
                                        .unwrap_or_else(|| {
                                            panic!("No address found for {:?}", value_data)
                                        });
                                    inst_vec.push(Inst::La {
                                        rd: T0,
                                        label: address.label.clone(),
                                    });
                                    inst_vec.push(Inst::Lw {
                                        rd: A0,
                                        rs: T0,
                                        offset: 0,
                                        tmp: T1,
                                    });
                                }
                                _ => {
                                    let value = func_ctx
                                        .as_ref()
                                        .unwrap()
                                        .get_address(value_data)
                                        .unwrap_or_else(|| {
                                            panic!("No address found for {:?}", value_data)
                                        });
                                    inst_vec.push(Inst::Lw {
                                        rd: A0,
                                        rs: value.base.clone(),
                                        offset: value.offset as i32,
                                        tmp: T1,
                                    });
                                    asm_block.add_inst(inst_vec);

                                    asm_block.add_inst(vec![Inst::Epilogue]);
                                    asm_block.add_inst(vec![Inst::Ret]);
                                }
                            }
                        }
                        None => {
                            asm_block.add_inst(vec![Inst::Epilogue]);
                            inst_vec.push(Inst::Ret);
                            asm_block.add_inst(inst_vec);
                        }
                    }
                }
                ValueKind::Load(load) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    let src_data;
                    let borrowed_value;
                    if load.src().is_global() {
                        borrowed_value = program.borrow_value(load.src());
                        src_data = &*borrowed_value;
                    } else {
                        src_data = func_data.dfg().value(load.src());
                    }
                    let ptr_flag = match src_data.kind() {
                        ValueKind::GetPtr(gp) => true,
                        ValueKind::GetElemPtr(gep) => true,
                        _ => false,
                    };
                    let dest = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(value_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", value_data)); //临时分配
                    let src = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(src_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", src_data));
                    if ptr_flag {
                        inst_vec.push(Inst::Lw {
                            rd: T0,
                            rs: src.base.clone(),
                            offset: src.offset as i32,
                            tmp: T1,
                        });
                        inst_vec.push(Inst::Lw {
                            rd: T0,
                            rs: T0,
                            offset: 0,
                            tmp: T1,
                        });
                    } else {
                        if src.is_global {
                            inst_vec.push(Inst::La {
                                rd: T0,
                                label: src.label.clone(),
                            });
                            inst_vec.push(Inst::Lw {
                                rd: T0,
                                rs: T0,
                                offset: 0,
                                tmp: T1,
                            });
                        } else {
                            inst_vec.push(Inst::Lw {
                                rd: T0,
                                rs: src.base.clone(),
                                offset: src.offset as i32,
                                tmp: T1,
                            });
                        }
                    }
                    inst_vec.push(Inst::Sw {
                        rd: dest.base.clone(),
                        rs: T0,
                        offset: dest.offset as i32,
                        tmp: T1,
                    });

                    asm_block.add_inst(inst_vec);
                }
                ValueKind::GetPtr(gp) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    let src_data;
                    let borrowed_value;
                    if gp.src().is_global() {
                        borrowed_value = program.borrow_value(gp.src());
                        src_data = &*borrowed_value;
                    } else {
                        src_data = func_data.dfg().value(gp.src());
                    }
                    let elem_size;
                    match src_data.ty().kind() {
                        TypeKind::Pointer(ptr) => {
                            elem_size = ptr.size();
                        }
                        _ => unreachable!(),
                    }
                    let src = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(src_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", src_data));
                    let dest = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(value_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", value_data));
                    //dest是临时分配
                    // let ptr_flag = matches!(
                    //     src_data.kind(),
                    //     ValueKind::GetPtr(_) | ValueKind::GetElemPtr(_)
                    // );

                    inst_vec.push(Inst::Lw {
                        rd: T0,
                        rs: src.base.clone(),
                        offset: src.offset as i32,
                        tmp: T1,
                    });

                    let index_data = func_data.dfg().value(gp.index());
                    match index_data.kind() {
                        ValueKind::Integer(i) => {
                            inst_vec.push(Inst::Li {
                                rd: T1,
                                imm: i.value() * elem_size as i32,
                            });
                        }
                        _ => {
                            let index = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(index_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", index_data));
                            inst_vec.push(Inst::Lw {
                                rd: T1,
                                rs: index.base.clone(),
                                offset: index.offset as i32,
                                tmp: T2,
                            });
                            match is_pow_of_two(elem_size as i32) {
                                Some(shift) => {
                                    inst_vec.push(Inst::Slli {
                                        rd: T1,
                                        rs: T1,
                                        imm: shift,
                                    });
                                }
                                None => {
                                    inst_vec.push(Inst::Li {
                                        rd: T2,
                                        imm: elem_size as i32,
                                    });
                                    inst_vec.push(Inst::OP2 {
                                        op: "mul".to_string(),
                                        rd: T1,
                                        rs1: T1,
                                        rs2: T2,
                                    });
                                }
                            }
                        }
                    }
                    inst_vec.push(Inst::OP2 {
                        op: "add".to_string(),
                        rd: T0,
                        rs1: T0,
                        rs2: T1,
                    });
                    inst_vec.push(Inst::Sw {
                        rd: dest.base.clone(),
                        rs: T0,
                        offset: dest.offset as i32,
                        tmp: T1,
                    });
                    asm_block.add_inst(inst_vec);
                }
                ValueKind::GetElemPtr(gep) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    let src_data;
                    let borrowed_value;
                    if gep.src().is_global() {
                        borrowed_value = program.borrow_value(gep.src());
                        src_data = &*borrowed_value;
                    } else {
                        src_data = func_data.dfg().value(gep.src());
                    }
                    let elem_size;
                    match src_data.ty().kind() {
                        TypeKind::Pointer(ptr) => {
                            match ptr.kind() {
                                TypeKind::Array(base, _) => {
                                    elem_size = base.size();
                                }
                                _ => unreachable!(),
                            };
                        }
                        _ => unreachable!(),
                    }
                    let src = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(src_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", src_data));
                    let dest = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(value_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", value_data));
                    //dest是临时分配
                    let ptr_flag = matches!(
                        src_data.kind(),
                        ValueKind::GetPtr(_) | ValueKind::GetElemPtr(_)
                    );
                    if ptr_flag {
                        inst_vec.push(Inst::Lw {
                            rd: T0,
                            rs: src.base.clone(),
                            offset: src.offset as i32,
                            tmp: T1,
                        });
                    } else {
                        if src.is_global {
                            inst_vec.push(Inst::La {
                                rd: T0,
                                label: src.label.clone(),
                            });
                        } else {
                            inst_vec.push(Inst::Addi {
                                rd: T0,
                                rs: src.base.clone(),
                                imm: src.offset as i32,
                                tmp: T1,
                            });
                        }
                    }
                    let index_data = func_data.dfg().value(gep.index());
                    match index_data.kind() {
                        ValueKind::Integer(i) => {
                            inst_vec.push(Inst::Li {
                                rd: T1,
                                imm: i.value() * elem_size as i32,
                            });
                        }
                        _ => {
                            let index = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(index_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", index_data));
                            inst_vec.push(Inst::Lw {
                                rd: T1,
                                rs: index.base.clone(),
                                offset: index.offset as i32,
                                tmp: T2,
                            });
                            match is_pow_of_two(elem_size as i32) {
                                Some(shift) => {
                                    inst_vec.push(Inst::Slli {
                                        rd: T1,
                                        rs: T1,
                                        imm: shift,
                                    });
                                }
                                None => {
                                    inst_vec.push(Inst::Li {
                                        rd: T2,
                                        imm: elem_size as i32,
                                    });
                                    inst_vec.push(Inst::OP2 {
                                        op: "mul".to_string(),
                                        rd: T1,
                                        rs1: T1,
                                        rs2: T2,
                                    });
                                }
                            }
                        }
                    }
                    inst_vec.push(Inst::OP2 {
                        op: "add".to_string(),
                        rd: T0,
                        rs1: T0,
                        rs2: T1,
                    });
                    inst_vec.push(Inst::Sw {
                        rd: dest.base.clone(),
                        rs: T0,
                        offset: dest.offset as i32,
                        tmp: T1,
                    });
                    asm_block.add_inst(inst_vec);
                }
                ValueKind::Store(store) => {
                    //src 和 dest 都可能是全局变量
                    let mut inst_vec: Vec<Inst> = vec![];
                    let src_data;
                    let borrowed_value_src;
                    if store.value().is_global() {
                        borrowed_value_src = program.borrow_value(store.value());
                        src_data = &*borrowed_value_src;
                    } else {
                        src_data = func_data.dfg().value(store.value());
                    }
                    let dest_data;
                    let borrowed_value_dest;
                    if store.dest().is_global() {
                        borrowed_value_dest = program.borrow_value(store.dest());
                        dest_data = &*borrowed_value_dest;
                    } else {
                        dest_data = func_data.dfg().value(store.dest());
                    }
                    let ptr_flag = match dest_data.kind() {
                        ValueKind::GetPtr(gp) => true,
                        ValueKind::GetElemPtr(gep) => true,
                        _ => false,
                    };
                    match src_data.kind() {
                        ValueKind::Integer(c) => {
                            let dest = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(dest_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", dest_data));
                            inst_vec.push(Inst::Li {
                                rd: T0,
                                imm: c.value(),
                            });
                            if ptr_flag {
                                inst_vec.push(Inst::Lw {
                                    rd: T1,
                                    rs: dest.base.clone(),
                                    offset: dest.offset as i32,
                                    tmp: T2,
                                });
                                inst_vec.push(Inst::Sw {
                                    rd: T1,
                                    rs: T0,
                                    offset: 0,
                                    tmp: T2,
                                });
                            } else {
                                if dest.is_global {
                                    inst_vec.push(Inst::La {
                                        rd: T1,
                                        label: dest.label.clone(),
                                    });
                                    inst_vec.push(Inst::Sw {
                                        rd: T1,
                                        rs: T0,
                                        offset: 0,
                                        tmp: T2,
                                    });
                                } else {
                                    inst_vec.push(Inst::Sw {
                                        rd: dest.base.clone(),
                                        rs: T0,
                                        offset: dest.offset as i32,
                                        tmp: T1,
                                    });
                                }
                            }
                        }
                        ValueKind::GlobalAlloc(ga) => {
                            let src = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(src_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", src_data));
                            let dest = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(dest_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", dest_data));
                            inst_vec.push(Inst::La {
                                rd: T0,
                                label: src.label.clone(),
                            });
                            inst_vec.push(Inst::Lw {
                                rd: T0,
                                rs: T0,
                                offset: 0,
                                tmp: T1,
                            });
                            if ptr_flag {
                                inst_vec.push(Inst::Lw {
                                    rd: T1,
                                    rs: dest.base.clone(),
                                    offset: dest.offset as i32,
                                    tmp: T2,
                                });
                                inst_vec.push(Inst::Sw {
                                    rd: T1,
                                    rs: T0,
                                    offset: 0,
                                    tmp: T2,
                                });
                            } else {
                                if dest.is_global {
                                    inst_vec.push(Inst::La {
                                        rd: T1,
                                        label: dest.label.clone(),
                                    });
                                    inst_vec.push(Inst::Sw {
                                        rd: T1,
                                        rs: T0,
                                        offset: 0,
                                        tmp: T2,
                                    });
                                } else {
                                    inst_vec.push(Inst::Sw {
                                        rd: dest.base.clone(),
                                        rs: T0,
                                        offset: dest.offset as i32,
                                        tmp: T1,
                                    });
                                }
                            }
                        }
                        _ => {
                            let src = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(src_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", src_data));
                            let dest = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(dest_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", dest_data));
                            if src.is_in_reg {
                                inst_vec.push(Inst::Sw {
                                    rd: dest.base.clone(),
                                    rs: src.base.clone(),
                                    offset: dest.offset as i32,
                                    tmp: T1,
                                });
                            } else {
                                inst_vec.push(Inst::Lw {
                                    rd: T0,
                                    rs: src.base.clone(),
                                    offset: src.offset as i32,
                                    tmp: T1,
                                });
                                if ptr_flag {
                                    inst_vec.push(Inst::Lw {
                                        rd: T1,
                                        rs: dest.base.clone(),
                                        offset: dest.offset as i32,
                                        tmp: T2,
                                    });
                                    inst_vec.push(Inst::Sw {
                                        rd: T1,
                                        rs: T0,
                                        offset: 0,
                                        tmp: T2,
                                    });
                                } else {
                                    if dest.is_global {
                                        inst_vec.push(Inst::La {
                                            rd: T1,
                                            label: dest.label.clone(),
                                        });
                                        inst_vec.push(Inst::Sw {
                                            rd: T1,
                                            rs: T0,
                                            offset: 0,
                                            tmp: T2,
                                        });
                                    } else {
                                        inst_vec.push(Inst::Sw {
                                            rd: dest.base.clone(),
                                            rs: T0,
                                            offset: dest.offset as i32,
                                            tmp: T1,
                                        });
                                    }
                                }
                            }
                        }
                    }
                    asm_block.add_inst(inst_vec);
                }
                ValueKind::Binary(binary) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    // func_ctx.as_mut().unwrap().alloc_slot(value_data);
                    let dest = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(value_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", value_data)); //dest是临时分配
                    let lhs_data;
                    let borrowed_value_lhs;
                    if binary.lhs().is_global() {
                        borrowed_value_lhs = program.borrow_value(binary.lhs());
                        lhs_data = &*borrowed_value_lhs;
                    } else {
                        lhs_data = func_data.dfg().value(binary.lhs());
                    }
                    let rhs_data;
                    let borrowed_value_rhs;
                    if binary.rhs().is_global() {
                        borrowed_value_rhs = program.borrow_value(binary.rhs());
                        rhs_data = &*borrowed_value_rhs;
                    } else {
                        rhs_data = func_data.dfg().value(binary.rhs());
                    }
                    let lhs = match lhs_data.kind() {
                        ValueKind::Integer(c) => {
                            let lhs = c.value();
                            inst_vec.push(Inst::Li { rd: T0, imm: lhs });
                            T0
                        }
                        ValueKind::GlobalAlloc(ga) => {
                            let lhs = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(lhs_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", lhs_data));
                            inst_vec.push(Inst::La {
                                rd: T0,
                                label: lhs.label.clone(),
                            });
                            inst_vec.push(Inst::Lw {
                                rd: T0,
                                rs: T0,
                                offset: 0,
                                tmp: T1,
                            });
                            T0
                        }
                        _ => {
                            let lhs = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(lhs_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", lhs_data));
                            inst_vec.push(Inst::Lw {
                                rd: T0,
                                rs: lhs.base.clone(),
                                offset: lhs.offset as i32,
                                tmp: T1,
                            });
                            T0
                        }
                    };
                    let rhs = match rhs_data.kind() {
                        ValueKind::Integer(c) => {
                            let rhs = c.value();
                            inst_vec.push(Inst::Li { rd: T1, imm: rhs });
                            T1
                        }
                        ValueKind::GlobalAlloc(ga) => {
                            let rhs = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(rhs_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", rhs_data));
                            inst_vec.push(Inst::La {
                                rd: T1,
                                label: rhs.label.clone(),
                            });
                            inst_vec.push(Inst::Lw {
                                rd: T1,
                                rs: T1,
                                offset: 0,
                                tmp: T1,
                            });
                            T1
                        }
                        _ => {
                            let rhs = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(rhs_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", rhs_data));
                            inst_vec.push(Inst::Lw {
                                rd: T1,
                                rs: rhs.base.clone(),
                                offset: rhs.offset as i32,
                                tmp: T1,
                            });
                            T1
                        }
                    };
                    match binary.op() {
                        BinaryOp::Add => inst_vec.push(Inst::OP2 {
                            op: "add".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Sub => inst_vec.push(Inst::OP2 {
                            op: "sub".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Mul => inst_vec.push(Inst::OP2 {
                            op: "mul".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Div => inst_vec.push(Inst::OP2 {
                            op: "div".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Mod => inst_vec.push(Inst::OP2 {
                            op: "rem".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::And => inst_vec.push(Inst::OP2 {
                            op: "and".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Or => inst_vec.push(Inst::OP2 {
                            op: "or".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Xor => inst_vec.push(Inst::OP2 {
                            op: "xor".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Shl => inst_vec.push(Inst::OP2 {
                            op: "sll".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Shr => inst_vec.push(Inst::OP2 {
                            op: "srl".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Sar => inst_vec.push(Inst::OP2 {
                            op: "sra".to_string(),
                            rd: T0,
                            rs1: lhs,
                            rs2: rhs,
                        }),
                        BinaryOp::Eq => {
                            inst_vec.push(Inst::OP2 {
                                op: "xor".to_string(),
                                rd: T0,
                                rs1: lhs,
                                rs2: rhs,
                            });
                            inst_vec.push(Inst::OP1 {
                                op: "seqz".to_string(),
                                rd: T0,
                                rs: T0,
                            });
                        }
                        BinaryOp::NotEq => {
                            inst_vec.push(Inst::OP2 {
                                op: "xor".to_string(),
                                rd: T0,
                                rs1: lhs,
                                rs2: rhs,
                            });
                            inst_vec.push(Inst::OP1 {
                                op: "snez".to_string(),
                                rd: T0,
                                rs: T0,
                            });
                        }
                        BinaryOp::Lt => {
                            inst_vec.push(Inst::OP2 {
                                op: "slt".to_string(),
                                rd: T0,
                                rs1: lhs,
                                rs2: rhs,
                            });
                        }
                        BinaryOp::Gt => {
                            inst_vec.push(Inst::OP2 {
                                op: "sgt".to_string(),
                                rd: T0,
                                rs1: lhs,
                                rs2: rhs,
                            });
                        }
                        BinaryOp::Le => {
                            inst_vec.push(Inst::OP2 {
                                op: "sgt".to_string(),
                                rd: T0,
                                rs1: lhs,
                                rs2: rhs,
                            });
                            inst_vec.push(Inst::OP1 {
                                op: "seqz".to_string(),
                                rd: T0,
                                rs: T0,
                            });
                        }
                        BinaryOp::Ge => {
                            inst_vec.push(Inst::OP2 {
                                op: "slt".to_string(),
                                rd: T0,
                                rs1: lhs,
                                rs2: rhs,
                            });
                            inst_vec.push(Inst::OP1 {
                                op: "seqz".to_string(),
                                rd: T0,
                                rs: T0,
                            });
                        }
                    }
                    inst_vec.push(Inst::Sw {
                        rd: dest.base.clone(),
                        rs: T0,
                        offset: dest.offset as i32,
                        tmp: T1,
                    });
                    asm_block.add_inst(inst_vec);
                }
                ValueKind::Alloc(alloc) => {
                    // func_ctx.as_mut().unwrap().alloc_slot(value_data);
                }
                ValueKind::Branch(branch) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    let cond_data;
                    let borrowed_value_cond;
                    if branch.cond().is_global() {
                        borrowed_value_cond = program.borrow_value(branch.cond());
                        cond_data = &*borrowed_value_cond;
                    } else {
                        cond_data = func_data.dfg().value(branch.cond());
                    }
                    let cond = match cond_data.kind() {
                        ValueKind::Integer(c) => {
                            let rhs = c.value();
                            inst_vec.push(Inst::Li { rd: T1, imm: rhs });
                            T1
                        }
                        ValueKind::GlobalAlloc(ga) => {
                            let rhs = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(cond_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", cond_data));
                            inst_vec.push(Inst::La {
                                rd: T1,
                                label: rhs.label.clone(),
                            });
                            inst_vec.push(Inst::Lw {
                                rd: T1,
                                rs: T1,
                                offset: 0,
                                tmp: T2,
                            });
                            T1
                        }
                        _ => {
                            let rhs = func_ctx
                                .as_ref()
                                .unwrap()
                                .get_address(cond_data)
                                .unwrap_or_else(|| panic!("No address found for {:?}", cond_data));
                            inst_vec.push(Inst::Lw {
                                rd: T1,
                                rs: rhs.base.clone(),
                                offset: rhs.offset as i32,
                                tmp: T2,
                            });
                            T1
                        }
                    };

                    let then = func_data.dfg().bb(branch.true_bb()).name().clone().unwrap();
                    let then = then.strip_prefix("%").unwrap().to_string();
                    let else_ = func_data
                        .dfg()
                        .bb(branch.false_bb())
                        .name()
                        .clone()
                        .unwrap();
                    let else_ = else_.strip_prefix("%").unwrap().to_string();
                    inst_vec.push(Inst::Bnez {
                        rs: cond,
                        label: then,
                    });
                    inst_vec.push(Inst::J { label: else_ });

                    asm_block.add_inst(inst_vec);
                }
                ValueKind::Jump(jump) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    let target = func_data.dfg().bb(jump.target()).name().clone().unwrap();
                    let target = target.strip_prefix("%").unwrap().to_string();
                    inst_vec.push(Inst::J { label: target });
                    asm_block.add_inst(inst_vec);
                }
                ValueKind::Call(call) => {
                    let mut inst_vec: Vec<Inst> = vec![];
                    let args = call.args();
                    for (i, arg) in args.iter().enumerate() {
                        let arg_data;
                        let borrowed_value_arg;
                        if (*arg).is_global() {
                            borrowed_value_arg = program.borrow_value(*arg);
                            arg_data = &*borrowed_value_arg;
                        } else {
                            arg_data = func_data.dfg().value(*arg);
                        }
                        let address = func_ctx.as_ref().unwrap().get_address_for_arg(i);
                        match arg_data.kind() {
                            ValueKind::Integer(c) => {
                                inst_vec.push(Inst::Li {
                                    rd: T0,
                                    imm: c.value(),
                                });
                            }
                            ValueKind::GlobalAlloc(ga) => {
                                inst_vec.push(Inst::La {
                                    rd: T0,
                                    label: address.label.clone(),
                                });
                                inst_vec.push(Inst::Lw {
                                    rd: T0,
                                    rs: T0,
                                    offset: 0,
                                    tmp: T1,
                                });
                            }
                            _ => {
                                let src = func_ctx
                                    .as_ref()
                                    .unwrap()
                                    .get_address(arg_data)
                                    .unwrap_or_else(|| {
                                        panic!("No address found for {:?}", arg_data)
                                    });
                                inst_vec.push(Inst::Lw {
                                    rd: T0,
                                    rs: src.base.clone(),
                                    offset: src.offset as i32,
                                    tmp: T1,
                                });
                            }
                        };
                        if i < MAX_REG_PARAM as usize {
                            inst_vec.push(Inst::Mv {
                                rd: address.base.clone(),
                                rs: T0,
                            });
                        } else {
                            inst_vec.push(Inst::Sw {
                                rd: address.base.clone(),
                                rs: T0,
                                offset: address.offset as i32,
                                tmp: T1,
                            });
                        };
                    }
                    let callee = call.callee();
                    let callee_name = program.func(callee).name().to_string().clone();
                    let callee_name = callee_name.strip_prefix('@').unwrap_or(&*callee_name);
                    inst_vec.push(Inst::Call {
                        label: callee_name.to_string(),
                    });
                    let dest = func_ctx
                        .as_ref()
                        .unwrap()
                        .get_address(value_data)
                        .unwrap_or_else(|| panic!("No address found for {:?}", value_data));
                    inst_vec.push(Inst::Sw {
                        rd: dest.base.clone(),
                        rs: A0,
                        offset: dest.offset as i32,
                        tmp: T1,
                    });
                    asm_block.add_inst(inst_vec);
                }
                _ => {
                    unreachable!()
                }
            }
        }
        asm_block
    }
}
