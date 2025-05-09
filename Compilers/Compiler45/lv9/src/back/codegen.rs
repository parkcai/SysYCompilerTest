use crate::back::context::{variable_name, AsmError, Context, ValueLocation, PARAMETER_REGISTERS};
use crate::back::inst::*;
use crate::back::program::{AsmBlock, AsmFunc, AsmProgram, AsmVarDecl, Assembly};
use crate::back::register::*;
use crate::between;
use crate::util::logger::show_error;
use crate::util::remove_pointer;
use koopa::ir::entities::ValueData;
use koopa::ir::values::{Aggregate, Call as IRCall, GetElemPtr, GetPtr};
use koopa::ir::values::{Binary, Branch, Jump, Load, Return, Store};
use koopa::ir::{
    BasicBlock, BinaryOp, Function, FunctionData, Program, Type, TypeKind, Value, ValueKind,
};

struct GlobalValue(Value);
struct GlobalAggregate(Aggregate, usize);

fn get_return_type(func: Function, program: &Program) -> Type {
    let func_data = program.func(func);
    let ty = func_data.ty();
    if let TypeKind::Function(_, ret_ty) = ty.kind() {
        ret_ty.clone()
    } else {
        unreachable!()
    }
}

fn is_symbol(value: Value, ctx: &mut Context, program: &Program) -> Result<bool, AsmError> {
    Ok(ctx
        .symbol_table
        .get_symbol_from_loc(&ctx.get_location(value, program)?)
        .is_some())
}

fn get_type(value: Value, ctx: &Context, program: &Program) -> Result<Type, AsmError> {
    let func = ctx.func.ok_or(AsmError::UnknownFunction)?;
    let func_data = program.func(func);
    if func_data.dfg().values().contains_key(&value) {
        Ok(func_data.dfg().value(value).ty().clone())
    } else {
        Ok(program.borrow_value(value).ty().clone())
    }
}

pub fn generate_asm(program: Program) -> String {
    let functions = program.func_layout().to_vec();
    let global_vars = program.inst_layout().to_vec();
    let mut ctx = Context::default();

    let mut asm = AsmProgram::new();

    // Global variables
    for global_var in global_vars {
        let var_data = program.borrow_value(global_var);
        let name = var_data.name().clone().map_or_else(
            || ctx.name_generator.generate_indent_name(),
            // Global variable name is always start with "@_0_"
            |name| name[4..].to_string(),
        );
        let asm_var_decl = GlobalValue(global_var)
            .to_asm(&mut ctx, &program)
            .unwrap_or_else(|e| {
                show_error(&format!("{:?}", e), 3);
            });
        asm.add_var_decl(asm_var_decl);
        // Add global variable to the symbol table.
        ctx.symbol_table
            .insert(name.clone(), ValueLocation::GlobalValue(name));
    }

    // Function definitions
    for func in functions {
        ctx.func = Some(func);
        let func_data = program.func(func);
        if func_data.dfg().bbs().is_empty() {
            continue;
        }
        asm.add_func(func.to_asm(&mut ctx, &program).unwrap_or_else(|e| {
            show_error(&format!("{:?}", e), 3);
        }));
        ctx.func = None;
    }
    asm.dump()
}

fn get_init_array(agg: &ValueData, ty: &Type, func_data: &FunctionData) -> Vec<i32> {
    let size = ty.size();

    match agg.kind() {
        ValueKind::ZeroInit(_) => vec![0; size / 4],
        ValueKind::Aggregate(agg) => {
            let mut ret_val = Vec::with_capacity(size / 4);
            for elem in agg.elems() {
                let elem_data = func_data.dfg().value(*elem);
                let elem_type = elem_data.ty();
                let data = get_init_array(elem_data, elem_type, func_data);
                ret_val.extend(data);
            }
            ret_val
        }
        ValueKind::Integer(n) => vec![n.value()],
        _ => unreachable!(),
    }
}

trait ToAsm {
    type Output;
    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError>;
}

fn prologue_insts(ctx: &mut Context, program: &Program) -> Vec<Box<dyn Inst>> {
    let func = ctx.func.expect("No function context.");
    let func_data = program.func(func);
    let param_num = func_data.params().len();
    let mut insts: Vec<Box<dyn Inst>> = vec![];

    // Save used callee saved registers to stack.
    let used_callee_saved_registers = ctx.reg_allocator.used_callee_saved_registers();
    for reg in used_callee_saved_registers {
        let offset = ctx.stack_allocator.allocate(4);
        insts.push(Box::new(Sw {
            rs1: reg,
            offset,
            rs2: SP,
        }));
        ctx.reg_allocator.insert_callee_saved_register(reg, offset);
    }

    let use_fp = param_num > 8;

    if use_fp {
        // We need to use frame pointer to access parameters.
        insts.push(Box::new(Sw {
            rs1: FP,
            offset: ctx.stack_allocator.allocate(4),
            rs2: SP,
        }));
    }

    ctx.stack_allocator.align();

    let stack_size = ctx.stack_allocator.stack_size;
    if stack_size == 0 {
        return insts;
    }
    if stack_size <= 2048 {
        insts.insert(
            0,
            Box::new(Addi {
                rd: SP,
                rs: SP,
                imm: -stack_size,
            }),
        );
        if use_fp {
            insts.push(Box::new(Addi {
                rd: FP,
                rs: SP,
                imm: stack_size,
            }));
        }
    } else {
        insts.insert(
            0,
            Box::new(Add {
                rd: SP,
                rs1: SP,
                rs2: T0,
            }),
        );
        insts.insert(
            0,
            Box::new(LoadImm {
                rd: T0,
                imm: -stack_size,
            }),
        );
        if use_fp {
            insts.push(Box::new(LoadImm {
                rd: T0,
                imm: stack_size,
            }));
            insts.push(Box::new(Add {
                rd: FP,
                rs1: SP,
                rs2: T0,
            }));
        }
    }
    insts
}

fn epilogue_insts(ctx: &Context) -> Vec<Box<dyn Inst>> {
    let mut insts: Vec<Box<dyn Inst>> = vec![];

    // Restore callee saved registers.
    for reg in ctx.reg_allocator.used_callee_saved_registers() {
        if let Some(offset) = ctx.reg_allocator.get_callee_saved_register_offset(reg) {
            insts.push(Box::new(Sw {
                rs1: reg,
                offset,
                rs2: SP,
            }));
        }
    }

    let stack_size = ctx.stack_allocator.stack_size;
    if stack_size == 0 {
        return insts;
    }
    if stack_size <= 2047 {
        insts.push(Box::new(Addi {
            rd: SP,
            rs: SP,
            imm: stack_size,
        }));
    } else {
        insts.push(Box::new(LoadImm {
            rd: T0,
            imm: stack_size,
        }));
        insts.push(Box::new(Add {
            rd: SP,
            rs1: SP,
            rs2: T0,
        }))
    };
    insts
}

impl ToAsm for GlobalValue {
    type Output = AsmVarDecl;
    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let var_data = program.borrow_value(self.0);
        let original_name = var_data
            .name()
            .clone()
            .unwrap_or(ctx.name_generator.generate_indent_name());
        let name = original_name[4..].to_string();

        let size = remove_pointer(var_data.ty().clone()).size();
        if let ValueKind::GlobalAlloc(global_alloc) = var_data.kind() {
            let init = global_alloc.init();
            let init = program.borrow_value(init);
            let init = match init.kind() {
                ValueKind::Integer(n) => vec![n.value()],
                ValueKind::ZeroInit(_) => vec![0; size / 4],
                ValueKind::Aggregate(agg) => {
                    GlobalAggregate(agg.clone(), size).to_asm(ctx, program)?
                }
                _ => unreachable!("Invalid global variable initialization."),
            };
            Ok(AsmVarDecl::new(name, size, Some(init)))
        } else {
            Err(AsmError::InvalidGlobalValue)
        }
    }
}

impl ToAsm for GlobalAggregate {
    type Output = Vec<i32>;
    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let agg = &self.0;
        let mut result = Vec::with_capacity(self.1 / 4);
        for elem in agg.elems() {
            let elem_data = program.borrow_value(*elem);
            match elem_data.kind() {
                ValueKind::Integer(n) => {
                    result.push(n.value());
                }
                ValueKind::ZeroInit(_) => {
                    let init_data = program.borrow_value(*elem);
                    let size = remove_pointer(init_data.ty().clone()).size();
                    result.extend(vec![0; size / 4]);
                }
                ValueKind::Aggregate(agg) => {
                    let agg_data = program.borrow_value(*elem);
                    let size = remove_pointer(agg_data.ty().clone()).size();
                    let inner = GlobalAggregate(agg.clone(), size).to_asm(ctx, program)?;
                    result.extend(inner);
                }
                _ => {
                    return Err(AsmError::InvalidGlobalValue);
                }
            }
        }
        Ok(result)
    }
}

impl ToAsm for Function {
    type Output = AsmFunc;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let func_data = program.func(*self);
        let param_num = func_data.params().len();
        // Reset the register allocator.
        ctx.function_default(param_num, program);
        // Add parameters to the symbol table.
        for (i, param) in func_data.params().iter().enumerate() {
            let param = func_data.dfg().value(*param);
            let param_name = param
                .name()
                .clone()
                .map_or(ctx.name_generator.generate_indent_name(), |name| {
                    variable_name(&func_data.name(), &name)
                });
            if i < 8 {
                ctx.symbol_table
                    .insert(param_name, ValueLocation::Register(PARAMETER_REGISTERS[i]));
            } else {
                ctx.symbol_table
                    .insert(param_name, ValueLocation::Parameter(((i - 8) * 4) as i32));
            }
        }
        // The first character of the function name should be deleted.
        let func_name = &func_data.name()[1..];
        let mut func = AsmFunc::new(func_name.to_string());
        for (block, _) in func_data.layout().bbs() {
            func.add_block(block.to_asm(ctx, program)?);
        }
        ctx.current_bb = None;

        // Allocate stack space for the function and deallocate it when the function is done.
        let mut first_block = AsmBlock::new(format!(".{}_prologue", func_name));
        ctx.stack_allocator.align();
        let prologue_insts = prologue_insts(ctx, program);
        if !prologue_insts.is_empty() {
            first_block.add_insts(prologue_insts);
            func.add_first_block(first_block);
        }

        // Add epilogue instructions to the end of the function.
        for block in func.blocks_mut() {
            if let Some(ret_pos) = block.contains(&Ret) {
                let leave_insts = epilogue_insts(ctx);
                block.add_insts_in_pos(ret_pos, leave_insts);
            }
        }
        Ok(func)
    }
}

impl ToAsm for BasicBlock {
    type Output = AsmBlock;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        ctx.current_bb = Some(*self);
        let label_name = ctx
            .get_label_name(*self, program)
            .unwrap_or(ctx.name_generator.generate_label_name());
        let func = ctx.func.ok_or(AsmError::UnknownFunction)?;
        let func_data = program.func(func);
        let node = func_data.layout().bbs().node(self).unwrap();

        let mut asm_block = AsmBlock::new(label_name);
        for value in node.insts().keys() {
            let insts = value.to_asm(ctx, program)?;
            asm_block.add_insts(insts);
        }

        // If a temp value is used in another block, store it in the stack.
        for value in node.insts().keys() {
            let value_data = func_data.dfg().value(*value);
            let used_by = value_data.used_by();
            let mut used_by_other = false;
            for use_inst in used_by {
                if !node.insts().contains_key(&use_inst) {
                    used_by_other = true;
                    break;
                }
            }

            if used_by_other {
                if let Some(ValueLocation::Register(reg)) = ctx.temp_value_table.get(value) {
                    let reg = *reg;
                    let offset = ctx.stack_allocator.allocate(4);
                    ctx.temp_value_table
                        .insert(*value, ValueLocation::Stack(offset));
                    asm_block.add_inst_before_branch(Sw {
                        rs1: reg,
                        offset,
                        rs2: SP,
                    });
                    ctx.deallocate_reg(reg);
                }
            }
        }
        Ok(asm_block)
    }
}

impl ToAsm for Value {
    type Output = Vec<Box<dyn Inst>>;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let func = ctx.func.ok_or(AsmError::UnknownFunction)?;
        let value_data = program.func(func).dfg().value(*self);

        match value_data.kind() {
            ValueKind::Integer(_) => Ok(vec![]),
            ValueKind::ZeroInit(_)
            | ValueKind::Undef(_)
            | ValueKind::Aggregate(_)
            | ValueKind::FuncArgRef(_)
            | ValueKind::BlockArgRef(_)
            | ValueKind::GlobalAlloc(_) => unreachable!(),
            ValueKind::Alloc(_) => {
                let func_data = program.func(ctx.func.ok_or(AsmError::UnknownFunction)?);
                let data_type = remove_pointer(value_data.ty().clone());
                let data_name = value_data
                    .name()
                    .clone()
                    .map(|name| variable_name(&func_data.name(), &name))
                    .unwrap_or(ctx.name_generator.generate_indent_name());
                let data_size = data_type.size();

                let offset = ctx.stack_allocator.allocate(data_size as i32);
                ctx.symbol_table
                    .insert(data_name.clone(), ValueLocation::Stack(offset));
                Ok(vec![])
            }
            ValueKind::Load(load) => {
                let (insts, temp_value) = load.to_asm(ctx, program)?;
                ctx.temp_value_table.insert(*self, temp_value);
                Ok(insts)
            }
            ValueKind::Store(store) => store.to_asm(ctx, program),
            ValueKind::GetPtr(get_ptr) => {
                let (insts, temp_value) = get_ptr.to_asm(ctx, program)?;
                ctx.temp_value_table.insert(*self, temp_value);
                Ok(insts)
            }
            ValueKind::GetElemPtr(get_elem_ptr) => {
                let (insts, temp_value) = get_elem_ptr.to_asm(ctx, program)?;
                ctx.temp_value_table.insert(*self, temp_value);
                Ok(insts)
            }
            ValueKind::Binary(binary) => {
                let (insts, temp_value) = binary.to_asm(ctx, program)?;
                ctx.temp_value_table.insert(*self, temp_value);
                Ok(insts)
            }
            ValueKind::Branch(branch) => branch.to_asm(ctx, program),
            ValueKind::Jump(jump) => jump.to_asm(ctx, program),
            ValueKind::Call(call) => {
                let (insts, ret_value) = call.to_asm(ctx, program)?;
                let return_type = get_return_type(call.callee(), program);
                if !return_type.is_unit() {
                    ctx.temp_value_table.insert(*self, ret_value);
                }
                Ok(insts)
            }
            ValueKind::Return(ret) => ret.to_asm(ctx, program),
        }
    }
}

impl ToAsm for GetPtr {
    type Output = (Vec<Box<dyn Inst>>, ValueLocation);
    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let src = self.src();
        let idx = self.index();
        let src_type = remove_pointer(get_type(src, ctx, program)?);
        let elem_size = src_type.size();

        let src_loc = ctx.get_location(src, program)?;
        let (mut insts, src_reg) = ctx.load_value(&src_loc, &[]);
        let (imm_insts, imm_reg) =
            ctx.load_value(&ValueLocation::Immediate(elem_size as i32), &[src_reg]);
        insts.extend(imm_insts);
        let idx_loc = ctx.get_location(idx, program)?;
        let (idx_insts, idx_reg) = ctx.load_value(&idx_loc, &[src_reg, imm_reg]);
        insts.extend(idx_insts);
        let (temp_reg, alloc_insts) = ctx.allocate_reg(&[src_reg, idx_reg, imm_reg]);
        insts.extend(alloc_insts);

        if idx_reg != ZERO {
            insts.push(Box::new(Mul {
                rd: idx_reg,
                rs1: imm_reg,
                rs2: idx_reg,
            }));
        }
        insts.push(Box::new(Add {
            rd: temp_reg,
            rs1: src_reg,
            rs2: idx_reg,
        }));
        ctx.deallocate_reg(src_reg);
        ctx.deallocate_reg(idx_reg);
        ctx.deallocate_reg(imm_reg);
        Ok((insts, ValueLocation::Register(temp_reg)))
    }
}

impl ToAsm for GetElemPtr {
    type Output = (Vec<Box<dyn Inst>>, ValueLocation);
    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let src = self.src();
        let idx = self.index();
        let src_type = remove_pointer(get_type(src, ctx, program)?);
        let elem_size = match src_type.kind() {
            TypeKind::Array(ty, _) => remove_pointer(ty.clone()).size(),
            _ => return Err(AsmError::InvalidGetElemPtr),
        };

        let idx_loc = ctx.get_location(idx, program)?;
        let (mut insts, idx_reg) = ctx.load_value(&idx_loc, &[]);
        let (imm_insts, imm_reg) =
            ctx.load_value(&ValueLocation::Immediate(elem_size as i32), &[idx_reg]);
        insts.extend(imm_insts);
        let src_loc = ctx.get_location(src, program)?;

        if is_symbol(src, ctx, program)? {
            let (res_reg, alloc_insts) = ctx.allocate_reg(&[idx_reg, imm_reg]);
            insts.extend(alloc_insts);
            match src_loc {
                ValueLocation::Stack(offset) => {
                    if between!(-2048, offset, 2047) {
                        insts.push(Box::new(Addi {
                            rd: res_reg,
                            rs: SP,
                            imm: offset,
                        }));
                    } else {
                        insts.push(Box::new(LoadImm {
                            rd: res_reg,
                            imm: offset,
                        }));
                        insts.push(Box::new(Add {
                            rd: res_reg,
                            rs1: SP,
                            rs2: res_reg,
                        }));
                    }
                }
                ValueLocation::GlobalValue(name) => {
                    insts.push(Box::new(LoadLabel {
                        rd: res_reg,
                        label: name,
                    }));
                }
                _ => return Err(AsmError::InvalidGetElemPtr),
            };
            if idx_reg != ZERO {
                insts.push(Box::new(Mul {
                    rd: idx_reg,
                    rs1: imm_reg,
                    rs2: idx_reg,
                }));
            }
            insts.push(Box::new(Add {
                rd: res_reg,
                rs1: res_reg,
                rs2: idx_reg,
            }));
            ctx.deallocate_reg(idx_reg);
            ctx.deallocate_reg(imm_reg);
            Ok((insts, ValueLocation::Register(res_reg)))
        } else {
            let (src_insts, src_reg) = ctx.load_value(&src_loc, &[idx_reg, imm_reg]);
            insts.extend(src_insts);
            let (temp_reg, alloc_insts) = ctx.allocate_reg(&[idx_reg, src_reg, imm_reg]);
            insts.extend(alloc_insts);
            if idx_reg != ZERO {
                insts.push(Box::new(Mul {
                    rd: idx_reg,
                    rs1: imm_reg,
                    rs2: idx_reg,
                }));
            }
            insts.push(Box::new(Add {
                rd: temp_reg,
                rs1: src_reg,
                rs2: idx_reg,
            }));
            ctx.deallocate_reg(src_reg);
            ctx.deallocate_reg(idx_reg);
            ctx.deallocate_reg(imm_reg);
            Ok((insts, ValueLocation::Register(temp_reg)))
        }
    }
}

impl ToAsm for IRCall {
    type Output = (Vec<Box<dyn Inst>>, ValueLocation);

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let callee_name = &program.func(self.callee()).name()[1..];
        let return_type = get_return_type(self.callee(), program);

        let mut insts: Vec<Box<dyn Inst>> = vec![];
        let mut arg_regs: Vec<Register> = vec![];

        let param_reg_num = PARAMETER_REGISTERS.len();

        // Load arguments.
        for (i, arg) in self.args().iter().enumerate() {
            let arg_loc = ctx.get_location(*arg, program)?;
            let (load, value_reg) = ctx.load_value(&arg_loc, &arg_regs);
            insts.extend(load);
            if i < param_reg_num {
                // Pass the argument in register.
                let param_reg = PARAMETER_REGISTERS[i];
                insts.extend(ctx.alloc_reg_from_name(param_reg));
                arg_regs.push(param_reg);
                if param_reg != value_reg {
                    insts.push(Box::new(Move {
                        rd: param_reg,
                        rs: value_reg,
                    }));
                }
                if value_reg != param_reg {
                    ctx.deallocate_reg(value_reg);
                }
            } else {
                // Pass the argument in stack.
                insts.push(Box::new(Sw {
                    rs1: value_reg,
                    offset: 4 * (i - param_reg_num) as i32,
                    rs2: SP,
                }));
                ctx.deallocate_reg(value_reg);
            }
        }

        // Allocate register for the return value.
        if self.args().len() == 0 && !return_type.is_unit() {
            let alloc_insts = ctx.alloc_reg_from_name(A0);
            insts.extend(alloc_insts);
        }

        // Save caller saved registers.
        let mut caller_saved_reg = ctx.reg_allocator.used_caller_saved_registers();
        caller_saved_reg.push(RA);

        // If the register is used for parameter or return value, we don't need to save it.
        let arg_num = self.args().len();
        for reg in PARAMETER_REGISTERS[..arg_num.min(param_reg_num)].iter() {
            if let Some(pos) = caller_saved_reg.iter().position(|r| r == reg) {
                caller_saved_reg.remove(pos);
            }
        }
        if !return_type.is_unit() {
            if let Some(pos) = caller_saved_reg.iter().position(|r| *r == A0) {
                caller_saved_reg.remove(pos);
            }
        }

        let mut saved_reg_offset = vec![];

        for reg in &caller_saved_reg {
            let offset = ctx.stack_allocator.allocate(4);
            insts.push(Box::new(Sw {
                rs1: *reg,
                offset,
                rs2: SP,
            }));
            saved_reg_offset.push(offset);
        }

        // Call the function.
        insts.push(Box::new(Call {
            label: callee_name.to_string(),
        }));

        // Restore caller saved registers.
        for (i, reg) in caller_saved_reg.iter().enumerate() {
            insts.push(Box::new(Lw {
                rd: *reg,
                offset: saved_reg_offset[i],
                rs: SP,
            }));
        }

        // Deallocate the stack space for the arguments.
        if arg_num > 1 {
            for reg in &PARAMETER_REGISTERS[1..arg_num.min(param_reg_num)] {
                ctx.reg_allocator.set_unused(*reg);
            }
        }
        if return_type.is_unit() && arg_num >= 1 {
            ctx.reg_allocator.set_unused(A0);
        }

        Ok((insts, ValueLocation::Register(A0)))
    }
}

impl ToAsm for Branch {
    type Output = Vec<Box<dyn Inst>>;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let cond_loc = ctx.get_location(self.cond(), program)?;
        let (mut insts, reg) = ctx.load_value(&cond_loc, &[]);
        let true_target = ctx
            .get_label_name(self.true_bb(), program)
            .ok_or(AsmError::UnknownBranchTarget)?;
        let false_target = ctx
            .get_label_name(self.false_bb(), program)
            .ok_or(AsmError::UnknownBranchTarget)?;
        insts.push(Box::new(Bnez {
            rs: reg,
            label: true_target,
        }));
        insts.push(Box::new(Jmp {
            label: false_target,
        }));
        ctx.deallocate_reg(reg);
        Ok(insts)
    }
}

impl ToAsm for Jump {
    type Output = Vec<Box<dyn Inst>>;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let target_name = ctx
            .get_label_name(self.target(), program)
            .ok_or(AsmError::UnknownJumpTarget)?;
        let current_bb = ctx.current_bb.ok_or(AsmError::NoBasicBlock)?;
        let target_next_bb = ctx
            .next_block(current_bb, program)
            .map(|bb| {
                ctx.get_label_name(bb, program)
                    .map(|name| name == target_name)
                    .unwrap_or(false)
            })
            .unwrap_or(false);
        if !target_next_bb {
            Ok(vec![Box::new(Jmp { label: target_name })])
        } else {
            Ok(vec![])
        }
    }
}

impl ToAsm for Store {
    type Output = Vec<Box<dyn Inst>>;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let func = ctx.func.ok_or(AsmError::UnknownFunction)?;
        let func_data = program.func(func);
        let value = self.value();
        let dest = self.dest();
        let value_data = func_data.dfg().value(value);

        // Aggregate initialize
        if matches!(
            value_data.kind(),
            ValueKind::ZeroInit(_) | ValueKind::Aggregate(_)
        ) {
            let dest_data = func_data.dfg().value(dest);
            let dest_type = remove_pointer(dest_data.ty().clone());
            let store_value = get_init_array(value_data, &dest_type, func_data);
            let dest_loc = ctx.get_location(dest, program)?;
            return match dest_loc {
                ValueLocation::Stack(offset) => {
                    let (temp_reg, mut insts) = ctx.allocate_reg(&[]);
                    for (i, value) in store_value.iter().enumerate() {
                        let offset = offset + i as i32 * 4;
                        if *value != 0 {
                            insts.push(Box::new(LoadImm {
                                rd: temp_reg,
                                imm: *value,
                            }));
                            insts.push(Box::new(Sw {
                                rs1: temp_reg,
                                offset,
                                rs2: SP,
                            }));
                        } else {
                            insts.push(Box::new(Sw {
                                rs1: ZERO,
                                offset,
                                rs2: SP,
                            }));
                        }
                    }
                    ctx.deallocate_reg(temp_reg);
                    Ok(insts)
                }
                _ => Err(AsmError::InvalidStore),
            };
        }

        let store_value = ctx.get_location(value, program)?;
        let is_symbol = is_symbol(dest, ctx, program)?;

        // A temp value generated by GetElemPtr or GetPtr.
        if !is_symbol {
            let dest_loc = ctx.get_location(dest, program)?;
            let (mut insts, dest_reg) = ctx.load_value(&dest_loc, &[]);
            let value_loc = ctx.get_location(value, program)?;
            let (load, value_reg) = ctx.load_value(&value_loc, &[dest_reg]);
            insts.extend(load);
            insts.push(Box::new(Sw {
                rs1: value_reg,
                offset: 0,
                rs2: dest_reg,
            }));
            return Ok(insts);
        }

        // Get store destination.
        let dest = ctx.get_location(dest, program)?;
        match dest {
            ValueLocation::Register(dest) => {
                let (mut insts, reg) = ctx.load_value(&store_value, &[dest]);
                insts.push(Box::new(Move { rd: dest, rs: reg }));
                if reg != dest {
                    ctx.deallocate_reg(reg)
                }
                Ok(insts)
            }
            ValueLocation::Stack(offset) => {
                let (mut insts, reg) = ctx.load_value(&store_value, &[]);
                insts.push(Box::new(Sw {
                    rs1: reg,
                    offset,
                    rs2: SP,
                }));
                ctx.deallocate_reg(reg);
                Ok(insts)
            }
            ValueLocation::GlobalValue(name) => {
                let (mut insts, reg) = ctx.load_value(&store_value, &[]);
                let (temp_reg, alloc_insts) = ctx.allocate_reg(&[reg]);
                insts.extend(alloc_insts);
                insts.push(Box::new(LoadLabel {
                    rd: temp_reg,
                    label: name,
                }));
                insts.push(Box::new(Sw {
                    rs1: reg,
                    offset: 0,
                    rs2: temp_reg,
                }));
                ctx.deallocate_reg(reg);
                ctx.deallocate_reg(temp_reg);
                Ok(insts)
            }
            _ => Err(AsmError::InvalidStore),
        }
    }
}

impl ToAsm for Load {
    type Output = (Vec<Box<dyn Inst>>, ValueLocation);

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let is_symbol = is_symbol(self.src(), ctx, program)?;
        let val_loc = ctx.get_location(self.src(), program)?;

        // A temp value generated by GetElemPtr or GetPtr.
        if !is_symbol {
            let (mut insts, temp_reg) = ctx.load_value(&val_loc, &[]);
            let (res_reg, alloc_insts) = ctx.allocate_reg(&[temp_reg]);
            insts.extend(alloc_insts);
            insts.push(Box::new(Lw {
                rd: res_reg,
                offset: 0,
                rs: temp_reg,
            }));
            ctx.deallocate_reg(temp_reg);
            Ok((insts, ValueLocation::Register(res_reg)))
        } else {
            let (insts, reg) = ctx.load_value(&val_loc, &[]);
            Ok((insts, ValueLocation::Register(reg)))
        }
    }
}

impl ToAsm for Binary {
    type Output = (Vec<Box<dyn Inst>>, ValueLocation);

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let lhs = self.lhs();
        let rhs = self.rhs();
        let mut insts: Vec<Box<dyn Inst>> = vec![];

        fn generate_with_imm(
            lhs: Value,
            rhs: Value,
            op: BinaryOp,
            ctx: &mut Context,
            program: &Program,
        ) -> Option<(Vec<Box<dyn Inst>>, ValueLocation)> {
            let mut insts: Vec<Box<dyn Inst>> = vec![];
            if let Some(imm) = imm12(rhs, ctx, program) {
                if has_imm(op) {
                    let lhs_loc = ctx.get_location(lhs, program).ok()?;
                    let (load_lhs, rs) = ctx.load_value(&lhs_loc, &[]);
                    let (rd, temp_inst) = ctx.allocate_reg(&[rs]);
                    insts.extend(load_lhs);
                    insts.extend(temp_inst);
                    let cal_inst: Vec<Box<dyn Inst>> = match op {
                        BinaryOp::Add => vec![Box::new(Addi { rd, rs, imm })],
                        BinaryOp::Or => vec![Box::new(Ori { rd, rs, imm })],
                        BinaryOp::And => vec![Box::new(Andi { rd, rs, imm })],
                        BinaryOp::Xor => vec![Box::new(Xori { rd, rs, imm })],
                        BinaryOp::Eq => vec![
                            Box::new(Xori { rd, rs, imm }),
                            Box::new(SetZero { rd, rs: rd }),
                        ],
                        BinaryOp::NotEq => vec![
                            Box::new(Xori { rd, rs, imm }),
                            Box::new(SetNonZero { rd, rs: rd }),
                        ],
                        _ => unreachable!(),
                    };
                    insts.extend(cal_inst);
                    ctx.deallocate_reg(rs);
                    Some((insts, ValueLocation::Register(rd)))
                } else {
                    None
                }
            } else {
                None
            }
        }

        if let Some(res) = generate_with_imm(self.lhs(), self.rhs(), self.op(), ctx, program) {
            return Ok(res);
        }

        if let Some(res) = generate_with_imm(self.rhs(), self.lhs(), self.op(), ctx, program) {
            return Ok(res);
        }

        let lhs_loc = ctx.get_location(lhs, program)?;
        let (load_lhs, lhs_reg) = ctx.load_value(&lhs_loc, &[]);

        let rhs_loc = ctx.get_location(rhs, program)?;
        let (load_rhs, rhs_reg) = ctx.load_value(&rhs_loc, &[lhs_reg]);
        insts.extend(load_lhs);
        insts.extend(load_rhs);

        // Allocate a temporary register for the result.
        let (temp_reg, temp_inst) = ctx.allocate_reg(&[lhs_reg, rhs_reg]);
        insts.extend(temp_inst);

        let rs1 = lhs_reg;
        let rs2 = rhs_reg;
        let rd = temp_reg;
        // Calculate the result.
        let cal_insts: Vec<Box<dyn Inst>> = match self.op() {
            BinaryOp::NotEq => {
                vec![
                    Box::new(Xor { rd, rs1, rs2 }),
                    Box::new(SetNonZero { rd, rs: rd }),
                ]
            }
            BinaryOp::Eq => {
                vec![
                    Box::new(Xor { rd, rs1, rs2 }),
                    Box::new(SetZero { rd, rs: rd }),
                ]
            }
            BinaryOp::Gt => vec![Box::new(SetGt { rd, rs1, rs2 })],
            BinaryOp::Lt => vec![Box::new(SetLt { rd, rs1, rs2 })],
            BinaryOp::Ge => {
                vec![
                    Box::new(SetLt { rd, rs1, rs2 }),
                    Box::new(SetZero { rd, rs: rd }),
                ]
            }
            BinaryOp::Le => {
                vec![
                    Box::new(SetGt { rd, rs1, rs2 }),
                    Box::new(SetZero { rd, rs: rd }),
                ]
            }
            BinaryOp::Add => vec![Box::new(Add { rd, rs1, rs2 })],
            BinaryOp::Sub => vec![Box::new(Sub { rd, rs1, rs2 })],
            BinaryOp::Mul => vec![Box::new(Mul { rd, rs1, rs2 })],
            BinaryOp::Div => vec![Box::new(Div { rd, rs1, rs2 })],
            BinaryOp::Mod => vec![Box::new(Rem { rd, rs1, rs2 })],
            BinaryOp::And => vec![Box::new(And { rd, rs1, rs2 })],
            BinaryOp::Or => vec![Box::new(Or { rd, rs1, rs2 })],
            BinaryOp::Xor => vec![Box::new(Xor { rd, rs1, rs2 })],
            BinaryOp::Shl => vec![Box::new(Sll { rd, rs1, rs2 })],
            BinaryOp::Shr => vec![Box::new(Srl { rd, rs1, rs2 })],
            BinaryOp::Sar => vec![Box::new(Sra { rd, rs1, rs2 })],
        };

        insts.extend(cal_insts);

        if rs1 != rd {
            ctx.deallocate_reg(rs1);
        }
        if rs2 != rd {
            ctx.deallocate_reg(rs2);
        }
        let temp_val = ValueLocation::Register(rd);
        Ok((insts, temp_val))
    }
}

impl ToAsm for Return {
    type Output = Vec<Box<dyn Inst>>;

    fn to_asm(&self, ctx: &mut Context, program: &Program) -> Result<Self::Output, AsmError> {
        let mut insts: Vec<Box<dyn Inst>> = vec![];
        if let Some(val) = self.value() {
            let ret_val_loc = ctx.get_location(val, program)?;
            if let ValueLocation::Immediate(n) = ret_val_loc {
                insts.push(Box::new(LoadImm { rd: A0, imm: n }));
            } else {
                let (load, reg) = ctx.load_value(&ret_val_loc, &[]);
                insts.extend(load);
                if reg != A0 {
                    insts.push(Box::new(Move { rd: A0, rs: reg }));
                    if ctx.symbol_table.get_symbol_from_loc(&ret_val_loc).is_none() {
                        ctx.deallocate_reg(reg);
                    }
                }
            }
        }
        insts.push(Box::new(Ret));
        Ok(insts)
    }
}

fn imm12(value: Value, ctx: &mut Context, program: &Program) -> Option<i32> {
    let func = ctx.func?;
    let func_data = program.func(func);
    let value_data = func_data.dfg().value(value);
    match value_data.kind() {
        ValueKind::Integer(n) => {
            if between!(-2048, n.value(), 2047) {
                Some(n.value())
            } else {
                None
            }
        }
        _ => None,
    }
}

fn has_imm(op: BinaryOp) -> bool {
    match op {
        BinaryOp::Add
        | BinaryOp::Or
        | BinaryOp::And
        | BinaryOp::Xor
        | BinaryOp::Eq
        | BinaryOp::NotEq => true,
        _ => false,
    }
}
