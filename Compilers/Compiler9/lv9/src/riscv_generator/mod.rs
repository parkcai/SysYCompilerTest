use std::collections::HashMap;
use koopa::ir::{entities::ValueData, Function, FunctionData, Program, TypeKind, Value, ValueKind};
// use regex::Regex;

pub mod program;
pub mod global_data;
pub mod function;

pub const REGISTER_COUNT: usize = 32;
pub const REGISTER_NAME: [&str; REGISTER_COUNT] = [
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
];

// pub const USABLE_REGISTER_COUNT: usize = 16;
// pub const USABLE_REGISTERS_NAME: [&str; USABLE_REGISTER_COUNT] = ["s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"];
// pub const USABLE_REGISTERS_ID: [usize; USABLE_REGISTER_COUNT] = [8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31];
pub const PARAMETER_REGISTERS_ID: [usize; 8] = [10, 11, 12, 13, 14, 15, 16, 17];
pub const PARAMETER_REGISTERS_NAME: [&str; 8] = ["a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"];

pub type RegisterID = usize;
pub type RegisterName = &'static str;

pub enum RiscvGenerateResult {
    Ok,
}

const MAX_IMM: i32 = 2047;
const MIN_IMM: i32 = -2048;

/*======================================== Riscv Generateor Trait Defination Start ======================================== */
pub trait RiscvGenerate {
    fn riscv_generate(&self, context: &mut RiscvGeneratorContext) -> Result<RiscvGenerateResult, std::io::Error>;
}

pub fn riscv_generator(koopa_ir_str: String, output_str: &mut String) -> Result<(), std::io::Error> {
    let driver = koopa::front::Driver::from(koopa_ir_str.clone());
    let program = driver.generate_program().unwrap();

    let mut riscv_symbol_table_stack = RiscvSymbolTableStack::new();
    riscv_symbol_table_stack.push();

    let mut riscv_generator_context = RiscvGeneratorContext {
        output_str: output_str,
        riscv_symbol_table_stack: riscv_symbol_table_stack,
        riscv_function_table: RiscvFunctionTable::new(),
        program: &program,
    };
    program.riscv_generate(&mut riscv_generator_context)?;
    
    // let temp_str = remove_redundant_lw(&output_str);
    // output_str.clear();
    // output_str.push_str(&temp_str);

    Ok(())
}

/*======================================== Riscv Generateor Trait Defination End   ======================================== */

/*======================================== Riscv Generateor Context Defination Start ========================================*/

pub struct RiscvGeneratorContext<'a> {
    pub output_str: &'a mut String,
    pub riscv_symbol_table_stack: RiscvSymbolTableStack,
    pub riscv_function_table: RiscvFunctionTable,
    pub program: &'a Program,
}

/*======================================== Riscv Generateor Context Defination End   ========================================*/

/*======================================== Riscv Symbol Table Stack Defination Start ========================================*/

pub struct RiscvSymbolTableStack {
    pub riscv_symbol_table_stack: Vec<RiscvSymbolTable>,
}

impl RiscvSymbolTableStack {
    fn new() -> RiscvSymbolTableStack {
        RiscvSymbolTableStack {
            riscv_symbol_table_stack: Vec::new(),
        }
    }

    fn get_symbol(&self, value: &Value) -> Option<&RiscvSymbolTableEntry> {
        // let _riscv_symbol_table: &RiscvSymbolTable = self.riscv_symbol_table_stack.last().unwrap();
        // _riscv_symbol_table.riscv_symbol_table.get(&value)
        for table in self.riscv_symbol_table_stack.iter().rev() {
            if let Some(entry) = table.riscv_symbol_table.get(value) {
                return Some(entry);
            }
        }
        None
    }

    fn insert_new_symbol(&mut self, value: &Value, riscv_symbol_table_entry: RiscvSymbolTableEntry) -> Option<RiscvSymbolTableEntry> {
        let _riscv_symbol_table: &mut _ = self.riscv_symbol_table_stack.last_mut().unwrap();
        _riscv_symbol_table.riscv_symbol_table.insert(*value, riscv_symbol_table_entry)
    }

    fn push(&mut self) {
        self.riscv_symbol_table_stack.push(RiscvSymbolTable {
            riscv_symbol_table: HashMap::new(),
            current_size: 0,
        });
    }

    fn pop(&mut self) {
        self.riscv_symbol_table_stack.pop();
    }

    fn get_size(&self) -> usize {
        self.riscv_symbol_table_stack.last().unwrap().current_size
    }

    fn set_size_by_delta(&mut self, delta: usize) {
        let _riscv_symbol_table: &mut RiscvSymbolTable = self.riscv_symbol_table_stack.last_mut().unwrap();
        _riscv_symbol_table.current_size += delta;
    }
}

impl std::fmt::Display for RiscvSymbolTableStack {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (index, table) in self.riscv_symbol_table_stack.iter().enumerate() {
            write!(f, "SymbolTableStack[{}]:\n", index)?;
            for (value, entry) in table.riscv_symbol_table.iter() {
                write!(f, "    {:?}: {}\n", value, entry)?;
            }
        }
        Ok(())
    }
}

/*======================================== Riscv Symbol Table Stack Defination End   ========================================*/

pub struct RiscvSymbolTable {
    pub riscv_symbol_table: HashMap<Value, RiscvSymbolTableEntry>,
    pub current_size: usize,
}

/*======================================== Riscv Function Table Defination Start ========================================*/

pub struct RiscvFunctionTable {
    pub riscv_function_table: HashMap<Function, (String, usize)>, // 函数名（不含@）, 函数参数个数
}

impl RiscvFunctionTable {
    fn new() -> RiscvFunctionTable {
        RiscvFunctionTable {
            riscv_function_table: HashMap::new(),
        }
    }

    fn insert(&mut self, func_id: Function, name: String, param_num: usize) {
        self.riscv_function_table.insert(func_id, (name, param_num));
    }

    fn get(&self, func_id: &Function) -> Option<&(String, usize)> {
        self.riscv_function_table.get(&func_id)
    }
}

/*======================================== Riscv Function Table Defination End   ========================================*/

/*======================================== Riscv Symbol Table Entry Defination Start ========================================*/

pub enum RiscvSymbolTableEntry {
    StackVariable(i32), // offset
    RegisterVariable(RegisterName),
    GlobalVariable(String), // 全局变量名
}

impl std::fmt::Display for RiscvSymbolTableEntry {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            RiscvSymbolTableEntry::StackVariable(offset) => write!(f, "StackVariable({})", offset)?,
            RiscvSymbolTableEntry::RegisterVariable(register_name) => write!(f, "RegisterVariable({})", register_name)?,
            RiscvSymbolTableEntry::GlobalVariable(global_name) => write!(f, "GlobalVariable({})", global_name)?,
        };
        Ok(())
    }
}

/*======================================== Riscv Symbol Table Entry Defination End   ========================================*/

pub fn is_temp_symbol(context: &mut RiscvGeneratorContext, func: &FunctionData,value: Value) -> bool {
    let value_data = match value.is_global() {
        true => context.program.borrow_value(value).clone(),
        false => func.dfg().value(value).clone(),
    };
    match value_data.name() {
        Some(name) => name.starts_with("%"),
        None => true,
    }
}

// alloc类型的指令、de_address为true时 返回指针指向内容的大小，
// 其余返回指针的大小
pub fn get_type_size (value_data: &ValueData, de_address: bool) -> usize {
    let mut s = 0;
    match value_data.ty().kind() {
        TypeKind::Pointer(_type) => {
            if de_address {
                s += _type.size();
            } else {
                match value_data.kind() {
                    ValueKind::Alloc(_) => {
                        s += _type.size();
                    },
                    _ => {
                        s += value_data.ty().size();
                    }
                }
            }
        },
        _ => {
            s += value_data.ty().size();
        }
    }
    s
}

pub fn load_value(context: &mut RiscvGeneratorContext, value: &Value, value_data_option: Option<&ValueData>, target_register: RegisterID, is_load: bool, is_temp_value: bool) {
    if !is_load && value_data_option.is_some() {
        match value_data_option.unwrap().kind() {
            ValueKind::Integer(int) => {
                context.output_str.push_str(
                    format!(
                        "    li {}, {}\n", 
                        REGISTER_NAME[target_register],
                        int.value(),
                    ).as_str()
                );
                return;
            },
            _ => {}
        }
    }
    let entry = context.riscv_symbol_table_stack.get_symbol(value).unwrap();
    match entry {
        RiscvSymbolTableEntry::StackVariable(offset) => {
            load_offset(context, *offset, target_register);
            if is_load && is_temp_value {
                context.output_str.push_str(
                    format!(
                        "    lw {}, 0({})\n",
                        REGISTER_NAME[target_register],
                        REGISTER_NAME[target_register],
                    ).as_str()
                );
            }
        },
        RiscvSymbolTableEntry::RegisterVariable(register_name) => {
            context.output_str.push_str(
                format!(
                    "    mv {}, {}\n",
                    REGISTER_NAME[target_register],
                    register_name,
                ).as_str()
            );
        },
        RiscvSymbolTableEntry::GlobalVariable(global_name) => {
            context.output_str.push_str(
                format!(
                    "    la {}, {}\n",
                    REGISTER_NAME[target_register],
                    global_name,
                ).as_str()
            );
            context.output_str.push_str(
                format!(
                    "    lw {}, 0({})\n",
                    REGISTER_NAME[target_register],
                    REGISTER_NAME[target_register],
                ).as_str()
            );
        }
    }
}

pub fn store_value(context: &mut RiscvGeneratorContext, value: &Value, source_register: RegisterID, temp_register: RegisterID, is_store: bool, is_temp_value: bool) {
    let entry = context.riscv_symbol_table_stack.get_symbol(value).unwrap();
    match entry {
        RiscvSymbolTableEntry::StackVariable(offset) => {
            if is_store && is_temp_value {
                load_offset(context, *offset, temp_register);
                context.output_str.push_str(
                    format!(
                        "    sw {}, 0({})\n",
                        REGISTER_NAME[source_register],
                        REGISTER_NAME[temp_register],
                    ).as_str()
                );
            }
            else {
                store_offset(context, *offset, source_register, temp_register);
            }
        },
        RiscvSymbolTableEntry::RegisterVariable(register_name) => {
            context.output_str.push_str(
                format!(
                    "    mv {}, {}\n",
                    register_name,
                    REGISTER_NAME[source_register],
                ).as_str()
            );
        },
        RiscvSymbolTableEntry::GlobalVariable(global_name) => {
            context.output_str.push_str(
                format!(
                    "    la {}, {}\n",
                    REGISTER_NAME[temp_register],
                    global_name,
                ).as_str()
            );
            context.output_str.push_str(
                format!(
                    "    sw {}, 0({})\n",
                    REGISTER_NAME[source_register],
                    REGISTER_NAME[temp_register],
                ).as_str()
            );
        }
    }
}

pub fn add_offset(context: &mut RiscvGeneratorContext, offset: i32, base_register: RegisterID, target_register: RegisterID, temp_register: RegisterID) {
    if MIN_IMM <= offset && offset <= MAX_IMM {
        context.output_str.push_str(
            format!(
                "    addi {}, {}, {}\n",
                REGISTER_NAME[target_register],
                REGISTER_NAME[base_register],
                offset,
            ).as_str()
        );
    } else {
        context.output_str.push_str(
            format!(
                "    li {}, {}\n",
                REGISTER_NAME[temp_register],
                offset,
            ).as_str()
        );
        context.output_str.push_str(
            format!(
                "    add {}, {}, {}\n",
                REGISTER_NAME[target_register],
                REGISTER_NAME[base_register],
                REGISTER_NAME[temp_register],
            ).as_str()
        );
    }
}

pub fn load_offset(context: &mut RiscvGeneratorContext, offset: i32, target_register: RegisterID) {
    if MIN_IMM <= offset && offset <= MAX_IMM {
        context.output_str.push_str(
            format!(
                "    lw {}, {}(sp)\n",
                REGISTER_NAME[target_register],
                offset,
            ).as_str()
        );
    } else {
        context.output_str.push_str(
            format!(
                "    li {}, {}\n",
                REGISTER_NAME[target_register],
                offset,
            ).as_str()
        );
        context.output_str.push_str(
            format!(
                "    add {}, sp, {}\n",
                REGISTER_NAME[target_register],
                REGISTER_NAME[target_register],
            ).as_str()
        );
        context.output_str.push_str(
            format!(
                "    lw {}, 0({})\n",
                REGISTER_NAME[target_register],
                REGISTER_NAME[target_register],
            ).as_str()
        );
    }
}

pub fn store_offset(context: &mut RiscvGeneratorContext, offset: i32, source_register: RegisterID, temp_register: RegisterID) {
    if MIN_IMM <= offset && offset <= MAX_IMM {
        context.output_str.push_str(
            format!(
                "    sw {}, {}(sp)\n",
                REGISTER_NAME[source_register],
                offset,
            ).as_str()
        );
    } else {
        context.output_str.push_str(
            format!(
                "    li {}, {}\n",
                REGISTER_NAME[temp_register],
                offset,
            ).as_str()
        );
        context.output_str.push_str(
            format!(
                "    add {}, sp, {}\n",
                REGISTER_NAME[temp_register],
                REGISTER_NAME[temp_register],
            ).as_str()
        );
        context.output_str.push_str(
            format!(
                "    sw {}, 0({})\n",
                REGISTER_NAME[source_register],
                REGISTER_NAME[temp_register],
            ).as_str()
        );
    }
}

// 消除冗余的lw&sw
// fn remove_redundant_lw(riscv_str: &str) -> String {
//     let re = Regex::new(r"(?m)^\s*(sw\s+(\S+),\s*(\d+)\(sp\))\s*\n\s*(lw\s+(\S+),\s*(\d+)\(sp\))").unwrap();
//     let mut modified_str = String::new();
//     let mut last_end = 0;

//     for cap in re.captures_iter(riscv_str) {
//         let sw_instr = cap.get(1).unwrap().as_str();
//         let sw_reg = cap.get(2).unwrap().as_str();
//         let sw_offset = cap.get(3).unwrap().as_str();
//         let lw_instr = cap.get(4).unwrap().as_str();
//         let lw_reg = cap.get(5).unwrap().as_str();
//         let lw_offset = cap.get(6).unwrap().as_str();

//         // 检查偏移量是否相同
//         if sw_offset == lw_offset {
//             // Append the part of the string before the match
//             modified_str.push_str(&riscv_str[last_end..cap.get(0).unwrap().start()]);

//             // Append the sw instruction
//             modified_str.push_str(sw_instr);
//             modified_str.push('\n');

//             // If the registers are different, add an mv instruction
//             if sw_reg != lw_reg {
//                 modified_str.push_str(&format!("    mv {}, {}\n", lw_reg, sw_reg));
//             }

//             // Update the last end position
//             last_end = cap.get(0).unwrap().end();
//         }
//     }

//     // Append the remaining part of the string
//     modified_str.push_str(&riscv_str[last_end..]);

//     modified_str
// }