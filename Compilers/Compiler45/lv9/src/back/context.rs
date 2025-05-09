use crate::back::inst::{Inst, LoadImm, LoadLabel, Lw, Sw};
use crate::back::register::*;
use koopa::ir::{BasicBlock, Function, Program, Value, ValueKind};
use std::collections::HashMap;

macro_rules! hashmap {
    ($($key:expr => $value:expr),* $(,)?) => {{
        let mut map = HashMap::new();
        $(map.insert($key, $value);)*
        map
    }};
}

pub fn variable_name(func_name: &str, var_name: &str) -> String {
    format!("{}{}", &func_name[1..], &var_name[1..])
}

#[derive(Debug)]
pub enum AsmError {
    /// Function is not set in the current context.
    UnknownFunction,
    /// No symbol is found in the symbol table.
    NoSymbol,
    /// Invalid store operation(Store a value to an immediate value or a parameter).
    InvalidStore,
    /// No basic block is set in the current context.
    NoBasicBlock,
    /// No branch target is set for a branch operation.
    UnknownBranchTarget,
    /// The jump target is not set for a jump operation.
    UnknownJumpTarget,
    /// Invalid global value.
    InvalidGlobalValue,
    /// An invalid get element pointer operation.
    InvalidGetElemPtr,
}

#[derive(Default)]
pub struct StackAllocator {
    pub stack_size: i32,
}

impl StackAllocator {
    pub fn allocate(&mut self, size: i32) -> i32 {
        self.stack_size += size;
        self.stack_size - size
    }

    pub fn align(&mut self) {
        self.stack_size = (self.stack_size + 15) & !15;
    }

    pub fn func_default(&mut self, param_num: usize) {
        let initial_stack_size = if param_num <= PARAMETER_REGISTERS.len() {
            0
        } else {
            (param_num - PARAMETER_REGISTERS.len()) as i32 * 4
        };
        self.stack_size = initial_stack_size;
    }
}

#[derive(Default)]
pub struct SymbolTable {
    symbols: HashMap<String, ValueLocation>,
}

impl SymbolTable {
    pub fn insert(&mut self, name: String, location: ValueLocation) {
        self.symbols.insert(name, location);
    }

    pub fn get_symbol_from_loc(&self, location: &ValueLocation) -> Option<&str> {
        self.symbols.iter().find_map(|(name, loc)| {
            if loc == location {
                Some(name.as_str())
            } else {
                None
            }
        })
    }

    pub fn get(&self, name: &str) -> Option<&ValueLocation> {
        self.symbols.get(name)
    }

    fn clear(&mut self) {
        let mut to_remove = vec![];
        for (name, loc) in self.symbols.iter() {
            match loc {
                ValueLocation::GlobalValue(_) => continue,
                _ => to_remove.push(name.clone()),
            }
        }
        for name in to_remove {
            self.symbols.remove(&name);
        }
    }
}

/// Register allocator.
///
/// This is used to allocate registers for temp values.
pub struct RegisterAllocator {
    /// Register usage.
    ///
    /// The first element is a boolean value indicating whether the register is used.
    /// The second element is the offset from the stack pointer if the register is stored in the stack.
    used: HashMap<Register, (bool, Option<i32>)>,

    /// Callee-saved registers.
    ///
    /// The first element is a boolean value indicating whether the register is used in the current function.
    /// The second element is the offset from the stack pointer if the register is stored in the stack.
    callee_saved_registers: HashMap<Register, (bool, i32)>,
}

impl Default for RegisterAllocator {
    fn default() -> Self {
        Self::new()
    }
}

pub const CALLEE_SAVED_REGISTERS: [Register; 11] = [S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11];

pub const ALL_REGISTERS: [Register; 25] = [
    A0, A1, A2, A3, A4, A5, A6, A7, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, T1, T2, T3, T4,
    T5, T6,
];

pub const CALLER_SAVED_REGISTERS: [Register; 14] =
    [A0, A1, A2, A3, A4, A5, A6, A7, T1, T2, T3, T4, T5, T6];

pub const PARAMETER_REGISTERS: [Register; 8] = [A0, A1, A2, A3, A4, A5, A6, A7];

impl RegisterAllocator {
    pub fn new() -> Self {
        let hashmap = hashmap! {
            A0 => (false, None),
            A1 => (false, None),
            A2 => (false, None),
            A3 => (false, None),
            A4 => (false, None),
            A5 => (false, None),
            A6 => (false, None),
            A7 => (false, None),
            S1 => (false, None),
            S2 => (false, None),
            S3 => (false, None),
            S4 => (false, None),
            S5 => (false, None),
            S6 => (false, None),
            S7 => (false, None),
            S8 => (false, None),
            S9 => (false, None),
            S10 => (false, None),
            S11 => (false, None),
            T1 => (false, None),
            T2 => (false, None),
            T3 => (false, None),
            T4 => (false, None),
            T5 => (false, None),
            T6 => (false, None),
        };
        let callee_saved_registers = hashmap! {
            S1 => (false, 0),
            S2 => (false, 0),
            S3 => (false, 0),
            S4 => (false, 0),
            S5 => (false, 0),
            S6 => (false, 0),
            S7 => (false, 0),
            S8 => (false, 0),
            S9 => (false, 0),
            S10 => (false, 0),
            S11 => (false, 0),
        };
        Self {
            used: hashmap,
            callee_saved_registers,
        }
    }

    /// Setup environment for a function.
    pub fn function_default(&mut self, param_num: usize) {
        let param_regs = if param_num < PARAMETER_REGISTERS.len() {
            &PARAMETER_REGISTERS[..param_num]
        } else {
            &PARAMETER_REGISTERS
        };
        for (reg, used) in &mut self.used {
            if CALLEE_SAVED_REGISTERS.contains(reg) || param_regs.contains(reg) {
                *used = (true, None);
            } else {
                *used = (false, None);
            }
        }
    }

    /// Allocate a random unused register.
    pub fn try_allocate(&mut self, used_registers: &[Register]) -> Option<Register> {
        for (reg, used) in &mut self.used {
            if !used.0 && !used_registers.contains(reg) {
                *used = (true, used.1);
                return Some(*reg);
            }
        }
        None
    }

    pub fn set_used(&mut self, reg: Register) {
        self.used
            .insert(reg, (true, self.used.get(&reg).unwrap().1));
    }

    pub fn set_unused(&mut self, reg: Register) {
        self.used
            .insert(reg, (false, self.used.get(&reg).unwrap().1));
    }

    pub fn used_callee_saved_registers(&self) -> Vec<Register> {
        self.callee_saved_registers
            .iter()
            .filter(|(_, (used, _))| *used)
            .map(|(reg, _)| *reg)
            .collect()
    }

    pub fn used_caller_saved_registers(&self) -> Vec<Register> {
        self.used
            .iter()
            .filter(|(_, (used, _))| *used)
            .filter(|(reg, _)| CALLER_SAVED_REGISTERS.contains(reg))
            .map(|(reg, _)| *reg)
            .collect()
    }

    pub fn insert_callee_saved_register(&mut self, reg: Register, offset: i32) {
        self.callee_saved_registers.insert(reg, (true, offset));
    }

    pub fn get_callee_saved_register_offset(&self, reg: Register) -> Option<i32> {
        self.callee_saved_registers.get(&reg).map(|r| r.1)
    }
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub enum ValueLocation {
    /// Temp value is stored in register.
    Register(Register),
    /// Temp value is stored in stack, the i32 is the offset from the stack pointer.
    Stack(i32),
    /// Temp value is a parameter, and it is stored in stack.
    /// The i32 is the offset from the frame pointer.
    Parameter(i32),
    /// Temp value is an immediate value.
    Immediate(i32),
    /// Global value.
    GlobalValue(String),
}

#[derive(Default)]
pub struct NameGenerator {
    counter: i32,
}

impl NameGenerator {
    pub fn generate_indent_name(&mut self) -> String {
        let name = format!("#{}", self.counter);
        self.counter += 1;
        name
    }

    pub fn generate_label_name(&mut self) -> String {
        let name = format!(".L{}", self.counter);
        self.counter += 1;
        name
    }
}

#[derive(Default)]
pub struct TempValueTable {
    table: HashMap<Value, ValueLocation>,
}

impl TempValueTable {
    pub fn insert(&mut self, value: Value, location: ValueLocation) {
        self.table.insert(value, location);
    }

    pub fn get(&self, value: &Value) -> Option<&ValueLocation> {
        self.table.get(value)
    }

    pub fn remove(&mut self, value: &Value) {
        self.table.remove(value);
    }

    pub fn get_val_from_loc(&self, location: &ValueLocation) -> Option<Value> {
        self.table
            .iter()
            .find_map(|(val, loc)| if loc == location { Some(*val) } else { None })
    }

    fn clear(&mut self) {
        self.table.clear();
    }
}

#[derive(Default)]
pub struct Context {
    pub func: Option<Function>,

    pub temp_value_table: TempValueTable,

    /// Register allocator.
    ///
    /// This is used to allocate registers for temp values.
    pub reg_allocator: RegisterAllocator,

    pub stack_allocator: StackAllocator,

    pub symbol_table: SymbolTable,

    pub name_generator: NameGenerator,

    pub current_bb: Option<BasicBlock>,
}

impl Context {
    /// # Setup environment for a function.
    pub fn function_default(&mut self, param_num: usize, program: &Program) {
        self.reg_allocator.function_default(param_num);
        if let Some(max_param_num) = self.max_param_num(program) {
            self.stack_allocator.func_default(max_param_num);
        }
        self.symbol_table.clear();
        self.temp_value_table.clear();
        self.current_bb = None;
    }

    /// # Move a register to the stack.
    ///
    /// Move a value stored in a register to the stack and update the symbol table, temp value table, and register allocator.
    fn move_to_stack(&mut self, reg: Register) -> Vec<Box<dyn Inst>> {
        let offset = self.stack_allocator.allocate(4);

        let reg_loc = ValueLocation::Register(reg);
        let stack_loc = ValueLocation::Stack(offset);

        // If a symbol is found, update the symbol table.
        if let Some(name) = self.symbol_table.get_symbol_from_loc(&reg_loc) {
            self.symbol_table
                .insert(name.to_string(), stack_loc.clone());
        }
        // If a temp value is found, update the temp value location.
        if let Some(temp_val) = self.temp_value_table.get_val_from_loc(&reg_loc) {
            self.temp_value_table.insert(temp_val, stack_loc);
        }
        // Update the register allocator.
        if let Some(None) = self.reg_allocator.used.get(&reg).map(|r| r.1) {
            self.reg_allocator.used.insert(reg, (true, Some(offset)));
        } else {
            self.reg_allocator.used.insert(reg, (true, None));
        }
        vec![Box::new(Sw {
            rs1: reg,
            offset,
            rs2: SP,
        })]
    }

    /// Force allocate certain register.
    pub fn alloc_reg_from_name(&mut self, register: Register) -> Vec<Box<dyn Inst>> {
        if let Some((used, _)) = self.reg_allocator.used.get(&register) {
            if !used {
                self.reg_allocator.set_used(register);
                return vec![];
            }
            return self.move_to_stack(register);
        }
        unreachable!()
    }

    /// # Allocate a register.
    ///
    /// If no register is available, it will save a register to the stack and return the saved register.
    ///
    /// ## Panics
    ///
    /// If no parameter `used_registers` is all registers, it will panic.
    pub fn allocate_reg(&mut self, used_regs: &[Register]) -> (Register, Vec<Box<dyn Inst>>) {
        self.reg_allocator
            .try_allocate(used_regs)
            .map(|r| (r, vec![]))
            .unwrap_or_else(|| {
                let reg = *ALL_REGISTERS
                    .iter()
                    .find(|r| !used_regs.contains(r))
                    .unwrap();

                if CALLEE_SAVED_REGISTERS.contains(&reg) {
                    self.reg_allocator
                        .callee_saved_registers
                        .insert(reg, (true, 0));
                }
                (reg, self.move_to_stack(reg))
            })
    }

    /// # Deallocate a register.
    pub fn deallocate_reg(&mut self, reg: Register) {
        if reg == ZERO {
            return;
        }

        // If a symbol is found, do nothing
        if self
            .symbol_table
            .get_symbol_from_loc(&ValueLocation::Register(reg))
            .is_some()
        {
            return;
        }

        // If a temp value is found, remove it from the temp value table.
        let temp_value = self
            .temp_value_table
            .get_val_from_loc(&ValueLocation::Register(reg));
        if let Some(value) = temp_value {
            self.temp_value_table.remove(&value);
        }
        let offset = self.reg_allocator.used.get(&reg).unwrap().1;
        self.reg_allocator.used.insert(reg, (false, offset));
    }

    /// # Load a value from a location.
    ///
    /// ## Parameters
    ///
    /// - `value_location`: The location of the value.
    /// - `used_regs`: The registers that are used, these registers will not be used in the loading process.
    pub fn load_value(
        &mut self,
        value_location: &ValueLocation,
        used_regs: &[Register],
    ) -> (Vec<Box<dyn Inst>>, Register) {
        match value_location {
            ValueLocation::Register(reg) => (vec![], *reg),
            ValueLocation::Stack(offset) => {
                let (reg, mut insts) = self.allocate_reg(used_regs);
                insts.push(Box::new(Lw {
                    rd: reg,
                    offset: *offset,
                    rs: SP,
                }));
                (insts, reg)
            }
            ValueLocation::Parameter(offset) => {
                let (reg, mut insts) = self.allocate_reg(used_regs);
                insts.push(Box::new(Lw {
                    rd: reg,
                    offset: *offset,
                    rs: FP,
                }));
                (insts, reg)
            }
            ValueLocation::Immediate(imm) => {
                if *imm == 0 {
                    // 0 is a special case
                    return (vec![], ZERO);
                }
                let (reg, mut insts) = self.allocate_reg(used_regs);
                insts.push(Box::new(LoadImm { rd: reg, imm: *imm }));
                (insts, reg)
            }
            ValueLocation::GlobalValue(name) => {
                let (reg, mut insts) = self.allocate_reg(used_regs);
                insts.push(Box::new(LoadLabel {
                    rd: reg,
                    label: name.clone(),
                }));
                insts.push(Box::new(Lw {
                    rd: reg,
                    offset: 0,
                    rs: reg,
                }));
                (insts, reg)
            }
        }
    }

    pub fn next_block(&self, bb: BasicBlock, program: &Program) -> Option<BasicBlock> {
        let func = self.func?;
        let func_data = program.func(func);
        let bbs = func_data.layout().bbs();
        let mut last_bb = None;
        for (bb_id, _) in bbs {
            if last_bb == Some(bb) {
                return Some(*bb_id);
            }
            last_bb = Some(*bb_id);
        }
        None
    }

    fn label_name(func_name: &str, label: &str) -> String {
        format!(".{}_{}", &func_name[1..], &label[1..])
    }

    pub fn get_label_name(&self, bb: BasicBlock, program: &Program) -> Option<String> {
        let func_data = program.func(self.func?);
        program
            .func(self.func?)
            .dfg()
            .bb(bb)
            .name()
            .clone()
            .map(|name| Self::label_name(func_data.name(), &name))
    }

    /// Get the location of a value.
    pub fn get_location(&self, value: Value, program: &Program) -> Result<ValueLocation, AsmError> {
        let func = self.func.ok_or(AsmError::UnknownFunction)?;
        let func_data = program.func(func);
        let is_global = !func_data.dfg().values().contains_key(&value);
        let value_data = if is_global {
            program.borrow_value(value).clone()
        } else {
            func_data.dfg().value(value).clone()
        };
        match value_data.kind() {
            ValueKind::Integer(c) => Ok(ValueLocation::Immediate(c.value())),
            _ => {
                // If the value is a temp value, get the location from the temp value table.
                let temp_val = self.temp_value_table.get(&value).cloned();
                if let Some(temp_val) = temp_val {
                    Ok(temp_val)
                } else {
                    // Otherwise, get the location from the symbol table.
                    let value_name = value_data.name().as_ref().unwrap();
                    let name = if is_global {
                        value_name[4..].to_string()
                    } else {
                        variable_name(func_data.name(), value_name)
                    };
                    self.symbol_table
                        .get(&name)
                        .cloned()
                        .ok_or(AsmError::NoSymbol)
                }
            }
        }
    }

    /// Get the maximum parameter number of all functions called in the current function.
    fn max_param_num(&self, program: &Program) -> Option<usize> {
        let func = self.func?;
        let func_data = program.func(func);
        let mut max_param_num = 0usize;
        for (_, bb_node) in func_data.layout().bbs() {
            for (inst, _) in bb_node.insts() {
                let inst_data = func_data.dfg().value(*inst);
                if let ValueKind::Call(call) = inst_data.kind() {
                    max_param_num = max_param_num.max(call.args().len());
                }
            }
        }
        Some(max_param_num)
    }
}
