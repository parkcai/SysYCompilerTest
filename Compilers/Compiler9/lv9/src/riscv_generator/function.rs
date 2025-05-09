use koopa::ir::{entities::ValueKind, BasicBlock, BinaryOp, FunctionData, TypeKind};
use super::{get_type_size, is_temp_symbol, load_offset, load_value, store_value, add_offset, store_offset, PARAMETER_REGISTERS_ID, RiscvGenerate, RiscvGenerateResult, RiscvGeneratorContext, RiscvSymbolTableEntry, PARAMETER_REGISTERS_NAME};

fn basic_block_2_str(bb: &BasicBlock) -> String {
    let mut block_name = format!("{:?}", bb).to_string();
    block_name = block_name.replace("(", "_");
    block_name = block_name.replace(")", "");
    block_name
}

impl RiscvGenerate for FunctionData {
    fn riscv_generate(&self, context: &mut RiscvGeneratorContext) -> Result<RiscvGenerateResult, std::io::Error> {
        if self.layout().entry_bb().is_none() {
            return Ok(RiscvGenerateResult::Ok);
        }

        // context.current_function.push(self);
        let function_name = self.name()[1..].to_string(); // 去掉前缀@
        context.output_str.push_str(format!("    .globl {}\n", function_name).as_str());
        context.output_str.push_str(format!("{}:\n", function_name).as_str());
        let mut stack_size;

        // 计算栈帧大小
        let mut r: usize = 0; // 是否含有call指令，有则需要保存ra寄存器，r = 4;
        let mut a: usize = 0; // 为所有call指令传参预留的空间（以最大的参数个数为准）
        let mut s: usize = 0; // 局部变量个数
        for (&_bb, node) in self.layout().bbs() {
            for inst in node.insts().keys() {
                let inst_data = self.dfg().value(*inst);
                match inst_data.kind() {
                    ValueKind::Call(_call) => { // 检测是否有call指令
                        if r == 0 {
                            r = 4;
                        }
                        let (_, callee_function_params_count) = *context.riscv_function_table.get(&_call.callee()).unwrap();
                        if callee_function_params_count > a {
                            a = callee_function_params_count;
                        }
                    },
                    _ => {}
                }
                s += get_type_size(inst_data, false);
            }
        }
        a = (i32::max((a as i32) - 8, 0) * 4) as usize; // 传参空间
        // println!("r = {}, a = {}, s = {}", r, a, s);
        stack_size = r + a + s;
        stack_size = (stack_size + 15) / 16 * 16; // 向16字节对齐

        if stack_size > 0 {
            add_offset(context, -(stack_size as i32), 2, 2, 5);
        }

        // 保存寄存器
        if r != 0 {
            store_offset(context, stack_size as i32 - 4, 1, 5);
        }

        // 将参数放到符号表中
        for (index, param) in self.params().iter().enumerate() {
            if index < 8 {
                context.riscv_symbol_table_stack.insert_new_symbol(param, RiscvSymbolTableEntry::RegisterVariable(PARAMETER_REGISTERS_NAME[index]));
            }
            else {
                let offset = (index - 8) as i32 * 4;
                context.riscv_symbol_table_stack.insert_new_symbol(param, RiscvSymbolTableEntry::StackVariable(offset + stack_size as i32));
            }
        }

        // BasicBlock是block的id，BasicBlockNode才是block本身
        for (&_bb, node) in self.layout().bbs() {
            let name = self.dfg().bb(_bb).name().as_ref().unwrap();
            let block_name = basic_block_2_str(&_bb);
            context.output_str.push_str(
                format!(
                    "\n{}: # {}\n", 
                    block_name, 
                    name
                ).as_str()
            );

            for inst in node.insts().keys() {
                let inst_data = self.dfg().value(*inst);
                // context.output_str.push_str(format!("\n   # {:?}\n", inst_data).as_str());
                match inst_data.kind() { // kind of instruction
                    ValueKind::Alloc(_alloc) => {
                        match inst_data.ty().kind() {
                            TypeKind::Pointer(_) => { // alloc的一定是指针
                                let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                                context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                                context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, true));
                            }
                            _ => {}
                        }
                    }

                    ValueKind::Store(_store) => {
                        let src_value = _store.value();
                        let dest_value = _store.dest();
                        let src_value_data = self.dfg().value(src_value);
                        let is_dest_temp_symbol = is_temp_symbol(context, self, dest_value);
                        
                        load_value(context, &src_value, Some(src_value_data), 5, false, false);
                        store_value(context, &dest_value, 5, 6, true, is_dest_temp_symbol);
                    },

                    ValueKind::Load(_load) => {
                        let src_value = _load.src();
                        let is_src_temp_symbol = is_temp_symbol(context, self, src_value);
                        let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                        context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        
                        // load 的src一定不会是integer，所以value_data不用给，且当value是global时，无法在函数中获得value_data
                        load_value(context, &src_value, None, 5, true, is_src_temp_symbol);
                        store_value(context, &inst, 5, 6, false, false);
                    },

                    ValueKind::Return(_return) => {
                        if let Some(return_value) = _return.value() {
                            let return_value_data = self.dfg().value(return_value);
                            load_value(context, &return_value, Some(return_value_data), 10, false, false);
                        }
                        context.output_str.push_str(format!("    j {}_ret\n", function_name).as_str());
                    },

                    ValueKind::Branch(_branch) => {
                        let cond_value = _branch.cond();
                        let cond_data = self.dfg().value(cond_value);
                        let true_block = _branch.true_bb();
                        let false_block = _branch.false_bb();
                        let true_block_str = basic_block_2_str(&true_block);
                        let false_block_str = basic_block_2_str(&false_block);

                        load_value(context, &cond_value, Some(&cond_data), 5, false, false);
                        context.output_str.push_str(
                            format!(
                                "    bnez t0, {}_{}\n", 
                                true_block_str, 
                                false_block_str
                            ).as_str()
                        );
                        context.output_str.push_str(
                            format!(
                                "    j {}\n", 
                                false_block_str
                            ).as_str()
                        );
                        context.output_str.push_str(
                            format!(
                                "{}_{}:\n", 
                                true_block_str,
                                false_block_str
                            ).as_str()
                        );
                        context.output_str.push_str(
                            format!(
                                "    j {}\n", 
                                true_block_str,
                            ).as_str()
                        );
                    },

                    ValueKind::Jump(_jump) => {
                        let target_block = _jump.target();
                        context.output_str.push_str(format!("    j {}\n", basic_block_2_str(&target_block)).as_str());
                    },

                    ValueKind::Call(_call) => {
                        // 准备参数
                        for (index, param_value) in _call.args().iter().enumerate() {
                            let param_value_data = self.dfg().value(*param_value);
                            if index < 8 {
                                load_value(context, param_value, Some(param_value_data), PARAMETER_REGISTERS_ID[index], false, false);
                            }
                            else {
                                load_value(context, param_value, Some(param_value_data), 5, false, false);

                                let new_offset = (index as i32 - 8) * 4;
                                let old_entry = context.riscv_symbol_table_stack.insert_new_symbol(param_value, RiscvSymbolTableEntry::StackVariable(new_offset));

                                store_value(context, param_value, 5, 6, false, false);

                                if old_entry.is_some() {
                                    context.riscv_symbol_table_stack.insert_new_symbol(param_value, old_entry.unwrap());
                                }
                            }
                        }

                        // 调用函数
                        let (callee_function_name, _) = context.riscv_function_table.get(&_call.callee()).unwrap();
                        context.output_str.push_str(
                            format!(
                                "    call {}\n", 
                                callee_function_name
                            ).as_str()
                        );

                        // 处理返回值
                        if !inst_data.ty().is_unit() {
                            let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                            context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                            context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));

                            store_value(context, &inst, 10, 5, false, false);
                        }
                    },

                    ValueKind::GetElemPtr(_get_elem_ptr) => {
                        let src_value = _get_elem_ptr.src();
                        let index_value = _get_elem_ptr.index();
                        let src_is_temp_symbol = is_temp_symbol(context, self, src_value);
                        let index_value_data = self.dfg().value(index_value);
                        let base_size = get_type_size(inst_data, true);

                        // 将index的值放到t1中
                        load_value(context, &index_value, Some(index_value_data), 6, false, false);

                        // 将src(base)的值放到t0中
                        if src_is_temp_symbol {
                            load_value(context, &src_value, None, 5, false, false);
                        }
                        else {
                            let src_entry = context.riscv_symbol_table_stack.get_symbol(&src_value).unwrap();
                            match src_entry {
                                RiscvSymbolTableEntry::StackVariable(offset) => {
                                    add_offset(context, *offset, 2, 5, 5);
                                },
                                RiscvSymbolTableEntry::GlobalVariable(global_variable_name) => {
                                    context.output_str.push_str(
                                        format!(
                                            "    la t0, {}\n", 
                                            global_variable_name
                                        ).as_str()
                                    );
                                },
                                RiscvSymbolTableEntry::RegisterVariable(_) => {} // 寄存器的情况待完成
                            }
                        }

                        context.output_str.push_str(
                            format!(
                                "    li t2, {}\n",
                                base_size
                            ).as_str()
                        );
                        context.output_str.push_str("    mul t1, t1, t2\n");
                        context.output_str.push_str("    add t0, t0, t1\n");
                        let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                        context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        store_value(context, &inst, 5, 6, false, false);
                    },

                    ValueKind::GetPtr(_get_ptr) => {
                        let src_value = _get_ptr.src();
                        // let src_value_data = self.dfg().value(src_value);
                        let index_value = _get_ptr.index();
                        let index_value_data = self.dfg().value(index_value);
                        let base_size = get_type_size(inst_data, true);

                        // 将index的值放到t1中
                        load_value(context, &index_value, Some(index_value_data), 6, false, false);

                        // 将src(base)的值放到t0中
                        load_value(context, &src_value, None, 5, false, false);

                        context.output_str.push_str(
                            format!(
                                "    li t2, {}\n",
                                base_size
                            ).as_str()
                        );
                        context.output_str.push_str("    mul t1, t1, t2\n");
                        context.output_str.push_str("    add t0, t0, t1\n");
                        let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                        context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        store_value(context, &inst, 5, 6, false, false);
                    },

                    ValueKind::Binary(_binary) => { // 左操作数一定在t0，右操作数一定在t1

                        let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                        context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        
                        let lhs_value = _binary.lhs();
                        let rhs_value = _binary.rhs();

                        let lhs_value_data = self.dfg().value(lhs_value);
                        let rhs_value_data = self.dfg().value(rhs_value);

                        load_value(context, &lhs_value, Some(lhs_value_data), 5, false, false);
                        load_value(context, &rhs_value, Some(rhs_value_data), 6, false, false);

                        match _binary.op() {
                            BinaryOp::Eq => {
                                context.output_str.push_str("    sub t2, t0, t1\n");
                                context.output_str.push_str("    seqz t2, t2\n");
                            },

                            BinaryOp::Sub => {
                                context.output_str.push_str("    sub t2, t0, t1\n");
                            },

                            BinaryOp::Add => {
                                context.output_str.push_str("    add t2, t0, t1\n");
                            },

                            BinaryOp::Mul => {
                                context.output_str.push_str("    mul t2, t0, t1\n");
                            },

                            BinaryOp::Div => {
                                context.output_str.push_str("    div t2, t0, t1\n");
                            },

                            BinaryOp::Mod => {
                                context.output_str.push_str("    rem t2, t0, t1\n");
                            },

                            BinaryOp::Lt => {
                                context.output_str.push_str("    slt t2, t0, t1\n");
                            },

                            BinaryOp::Le => {
                                context.output_str.push_str("    slt t2, t1, t0\n");
                                context.output_str.push_str("    xori t2, t2, 1\n");
                            },

                            BinaryOp::Gt => {
                                context.output_str.push_str("    slt t2, t1, t0\n");
                            },

                            BinaryOp::Ge => {
                                context.output_str.push_str("    slt t2, t0, t1\n");
                                context.output_str.push_str("    xori t2, t2, 1\n");
                            },

                            BinaryOp::NotEq => {
                                context.output_str.push_str("    sub t2, t0, t1\n");
                                context.output_str.push_str("    snez t2, t2\n");
                            },

                            BinaryOp::And => {
                                context.output_str.push_str("    snez t0, t0\n");
                                context.output_str.push_str("    snez t1, t1\n");
                                context.output_str.push_str("    and t2, t0, t1\n");
                            },

                            BinaryOp::Or => {
                                context.output_str.push_str("    snez t0, t0\n");
                                context.output_str.push_str("    snez t1, t1\n");
                                context.output_str.push_str("    or t2, t0, t1\n");
                            },

                            _ => {

                            }
                        }
                        store_value(context, &inst, 7, 5, false, false);
                    },
                    _ => {}
                }
            }
        }

        context.output_str.push_str(format!("{}_ret:\n", function_name).as_str());
        if r != 0 {
            load_offset(context, stack_size as i32 - 4, 1);
        }

        if stack_size > 0 {
            add_offset(context, stack_size as i32, 2, 2, 5);
        }
        context.output_str.push_str("    ret\n");
        Ok(RiscvGenerateResult::Ok)
    }
}
