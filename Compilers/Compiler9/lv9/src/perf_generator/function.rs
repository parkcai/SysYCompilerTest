use koopa::ir::{entities::ValueKind, BasicBlock, BinaryOp, FunctionData, TypeKind};
use regex::Regex;
use super::{add_offset, get_type_size, is_temp_symbol, load_offset, load_value, store_offset, store_value, symbol_manager::RiscvSymbolTableEntry, RiscvGenerate, RiscvGenerateResult, RiscvGeneratorContext, PARAMETER_REGISTERS_COUNT, PARAMETER_REGISTERS_ID, RA_REGISTER_ID, REGISTER_COUNT, REGISTER_NAME, SP_REGISTER_ID, T0_REGISTER_ID};

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

        let function_name = self.name()[1..].to_string(); // 去掉前缀@
        // if function_name != "fft" {
        //     return Ok(RiscvGenerateResult::Ok);
        // }
        context.output_str.push_str(format!("    .globl {}\n", function_name).as_str());
        context.output_str.push_str(format!("{}:\n", function_name).as_str());
        let mut stack_size;

        context.register_manager.reset();
        context.register_manager.count_variable_reference_in_function(context.program, self);

        // 计算栈帧大小
        /*
         * 我对栈帧的安排的改进：
         * 为所有寄存器都保留栈上位置，无论是caller-save还是callee-save，都在对应的固定位置存储
         * 我留的空间是31个的，zero不用存，计算存储位置时直接从栈大小减4乘寄存器编号即可
         */
        let mut have_call = false; // 是否含有call指令，有则需要保存ra寄存器
        let r: usize = (REGISTER_COUNT - 1) * 4; // 保存寄存器的空间
        let mut a: usize = 0; // 为所有call指令传参预留的空间（以最大的参数个数为准）
        let mut s: usize = 0; // 局部变量个数
        for (&_bb, node) in self.layout().bbs() {
            for inst in node.insts().keys() {
                let inst_data = self.dfg().value(*inst);
                match inst_data.kind() {
                    ValueKind::Call(_call) => { // 检测是否有call指令
                        have_call = true;
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
        stack_size = r + a + s;
        stack_size = (stack_size + 15) / 16 * 16; // 向16字节对齐

        if stack_size > 0 {
            add_offset(context, -(stack_size as i32), SP_REGISTER_ID, SP_REGISTER_ID, T0_REGISTER_ID);
        }

        // 保存寄存器
        if have_call {
            context.output_str.push_str(
                store_offset(stack_size as i32 - RA_REGISTER_ID as i32 * 4, RA_REGISTER_ID, T0_REGISTER_ID)
            .as_str());
        }

        context.output_str.push_str(format!("#{}_stqstqstqstq\n", function_name).as_str()); // 保存 callee-saved register 代码占位符

        // 将参数放到符号表中
        for (index, param) in self.params().iter().enumerate() {
            if index < PARAMETER_REGISTERS_COUNT {
                context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, param, Some(RiscvSymbolTableEntry::RegisterVariable(PARAMETER_REGISTERS_ID[index])), false, None);
            }
            else {
                let offset = (index - PARAMETER_REGISTERS_COUNT) as i32 * 4;
                context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, param, Some(RiscvSymbolTableEntry::StackVariable(offset + stack_size as i32)), false, None);
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
                                // let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                                // context.riscv_symbol_table_stack.insert_new_symbol(inst, RiscvSymbolTableEntry::StackVariable(new_offset));
                                context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, inst, None, false, Some((inst_data, a))); // 在insert中考虑具名符号的size决定放在哪
                                // context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, true));
                            }
                            _ => {}
                        }
                    }

                    ValueKind::Store(_store) => {
                        let src_value = _store.value();
                        let dest_value = _store.dest();
                        let src_value_data = self.dfg().value(src_value);
                        let is_dest_temp_symbol = is_temp_symbol(context.program, self, dest_value);
                        
                        load_value(context, &src_value, Some(src_value_data), 5, false, false);
                        store_value(context, &dest_value, 5, 6, true, is_dest_temp_symbol, true);
                    },

                    ValueKind::Load(_load) => {
                        let src_value = _load.src();
                        let is_src_temp_symbol = is_temp_symbol(context.program, self, src_value);
                        // let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, inst, None, true, None);
                        // context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        
                        // load 的src一定不会是integer，所以value_data不用给，且当value是global时，无法在函数中获得value_data
                        load_value(context, &src_value, None, 5, true, is_src_temp_symbol);
                        store_value(context, &inst, 5, 6, false, false, false);
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

                        // load_value(context, &cond_value, Some(&cond_data), 5, false, false);
                        let mut tmp_str = String::new();
                        match cond_data.kind() {
                            ValueKind::Integer(int) => {
                                tmp_str.push_str(int.value().to_string().as_str());
                            },
                            _ => {}
                        };
                        if tmp_str.is_empty() {
                            let entry = context.riscv_symbol_table_stack.get_symbol(&mut context.register_manager, &cond_value, true);
                            match entry {
                                Some(RiscvSymbolTableEntry::RegisterVariable(register_id)) => {
                                    tmp_str.push_str(REGISTER_NAME[register_id]);
                                },
                                _ => {}
                            };
                        }
                        context.output_str.push_str(
                            format!(
                                "    bnez {}, {}_{}\n", 
                                tmp_str,
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
                            if index < PARAMETER_REGISTERS_COUNT {
                                load_value(context, param_value, Some(param_value_data), PARAMETER_REGISTERS_ID[index], false, false);
                            }
                            else {
                                load_value(context, param_value, Some(param_value_data), 5, false, false);

                                let new_offset = (index - PARAMETER_REGISTERS_COUNT) as i32 * 4;
                                let old_entry = context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, param_value, Some(RiscvSymbolTableEntry::StackVariable(new_offset)), false, None);

                                store_value(context, param_value, 5, 6, false, false, false); // need remove temp varialbe参数：变量在栈上不受影响，写啥都行

                                if old_entry.is_some() {
                                    context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, param_value, old_entry, false, None);
                                }
                            }
                        }

                        context.output_str.push_str(&context.register_manager.generate_save_code_for_caller_saved_register(stack_size));

                        // 调用函数
                        let (callee_function_name, _) = context.riscv_function_table.get(&_call.callee()).unwrap();
                        context.output_str.push_str(
                            format!(
                                "    call {}\n", 
                                callee_function_name
                            ).as_str()
                        );

                        context.output_str.push_str(&context.register_manager.generate_reload_code_for_caller_saved_register(stack_size));
                        // if callee_function_name == "MemMove" {
                        //     println!("{:?}", inst_data);
                        // }
                        if inst_data.used_by().len() == 0 { // 如果没有使用返回值，就不用存储返回值
                            continue;
                        }
                        // 处理返回值
                        if !inst_data.ty().is_unit() {
                            // let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                            context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, inst, None, true, None);
                            // context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));

                            store_value(context, &inst, 10, 5, false, false, false);
                        }
                    },

                    ValueKind::GetElemPtr(_get_elem_ptr) => {
                        let src_value = _get_elem_ptr.src();
                        let index_value = _get_elem_ptr.index();
                        let src_is_temp_symbol = is_temp_symbol(context.program, self, src_value);
                        let index_value_data = self.dfg().value(index_value);
                        let base_size = get_type_size(inst_data, true);

                        // 将index的值放到t1中
                        load_value(context, &index_value, Some(index_value_data), 6, false, false);

                        // 将src(即base)的值放到t0中
                        if src_is_temp_symbol {
                            load_value(context, &src_value, None, 5, false, false);
                        }
                        else {
                            let src_entry = context.riscv_symbol_table_stack.get_symbol(&mut context.register_manager, &src_value, false).unwrap();
                            match src_entry {
                                RiscvSymbolTableEntry::StackVariable(offset) => {
                                    add_offset(context, offset, 2, 5, 5);
                                },
                                // 在get_elem_ptr中，src一定数组，不会是全局普通变量，直接忽略枚举中的第二项即可
                                RiscvSymbolTableEntry::GlobalVariable(global_variable_name, _) => {
                                    context.output_str.push_str(
                                        format!(
                                            "    la t0, {}\n", 
                                            global_variable_name
                                        ).as_str()
                                    );
                                },
                                RiscvSymbolTableEntry::RegisterVariable(_) => {
                                    panic!("未完成");
                                } // 寄存器的情况待完成
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
                        // let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, inst, None, true, None);
                        // context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        store_value(context, &inst, 5, 6, false, false, false);
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
                        // let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, inst, None, true, None);
                        // context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        store_value(context, &inst, 5, 6, false, false, false);
                    },

                    ValueKind::Binary(_binary) => { // 左操作数一定在t0，右操作数一定在t1

                        // let new_offset = (context.riscv_symbol_table_stack.get_size() + a) as i32;
                        context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, inst, None, true, None);
                        // context.riscv_symbol_table_stack.set_size_by_delta(get_type_size(inst_data, false));
                        
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
                        store_value(context, &inst, 7, 5, false, false, false);
                    },
                    _ => {}
                }
                // context.output_str.push_str(context.register_manager.temporary_register_occupancy_reporter().as_str());
            }
        }

        context.output_str.push_str(format!("{}_ret:\n", function_name).as_str());

        let callee_saved_register_str = context.register_manager.generate_save_code_for_callee_saved_register(stack_size);
        
        let pattern = format!(r"#{}_stqstqstqstq\n", regex::escape(function_name.as_str()));
        let re = Regex::new(pattern.as_str()).unwrap();
        let tmp = re.replace_all(&context.output_str, callee_saved_register_str.as_str()).to_string();
        context.output_str.clear();
        context.output_str.push_str(&tmp);

        // println!("in function: {}", function_name);
        context.output_str.push_str(context.register_manager.generate_reload_code_for_callee_saved_register(stack_size).as_str());

        if have_call {
            context.output_str.push_str(
                load_offset(stack_size as i32 - RA_REGISTER_ID as i32 * 4, RA_REGISTER_ID)
            .as_str());
        }

        if stack_size > 0 {
            add_offset(context, stack_size as i32, 2, 2, 5);
        }
        context.output_str.push_str("    ret\n");
        Ok(RiscvGenerateResult::Ok)
    }
}