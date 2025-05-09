use koopa::ir::{entities::ValueData, FunctionData, Program, Type, TypeKind, Value, ValueKind};
use regex::Regex;
use register_manager::RegisterManager;
use symbol_manager::{RiscvFunctionTable, RiscvSymbolTableEntry, RiscvSymbolTableStack};

pub mod program;
pub mod global_data;
pub mod function;
pub mod register_manager;
pub mod symbol_manager;

pub const REGISTER_COUNT: usize = 32;
pub const REGISTER_NAME: [&str; REGISTER_COUNT] = [
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
];

// pub const USABLE_REGISTER_COUNT: usize = 16;
// pub const USABLE_REGISTERS_ID: [usize; USABLE_REGISTER_COUNT] = [8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31];
// ["s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"];
pub const PARAMETER_REGISTERS_COUNT: usize = 8;
pub const PARAMETER_REGISTERS_ID: [usize; PARAMETER_REGISTERS_COUNT] = [10, 11, 12, 13, 14, 15, 16, 17];
// pub const PARAMETER_REGISTERS_NAME: [&str; 8] = ["a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"];
pub const RA_REGISTER_ID: usize = 1;
pub const SP_REGISTER_ID: usize = 2;
pub const T0_REGISTER_ID: usize = 5;
// pub const T1_REGISTER_ID: usize = 6;
// pub const T2_REGISTER_ID: usize = 7;

pub type RegisterID = usize;

pub enum RiscvGenerateResult {
    Ok,
}

const MAX_IMM: i32 = 2047;
const MIN_IMM: i32 = -2048;

/*======================================== Riscv Generateor Trait Defination Start ======================================== */
pub trait RiscvGenerate {
    fn riscv_generate(&self, context: &mut RiscvGeneratorContext) -> Result<RiscvGenerateResult, std::io::Error>;
}

pub fn perf_generator(_koopa_ir_str: String, output_str: &mut String, str_tmp: String) -> Result<(), std::io::Error> {
    let mut koopa_ir_str = _koopa_ir_str.clone();

    // 先消除koopaIR中的死变量（临时变量只出现一次）
    for i in 0..10000 {
        let pattern = format!(r"%{}", i.to_string().as_str());
        let re = Regex::new(pattern.as_str()).unwrap();
        let mut count = 0; // 出现次数，如果是1进行处理
        for _ in re.captures_iter(&koopa_ir_str) {
            count += 1;
        }
        if count == 1 {
            // println!("remove: {}", pattern);
            let re_define = Regex::new(&format!(r"\s*{}\s*=\s*", pattern)).unwrap();
            koopa_ir_str = re_define.replace_all(&koopa_ir_str, "\n    ").to_string();
        }
    }

    let driver = koopa::front::Driver::from(koopa_ir_str.clone());
    let program = driver.generate_program().unwrap();

    let mut riscv_symbol_table_stack = RiscvSymbolTableStack::new();
    riscv_symbol_table_stack.push();

    let is_fft = str_tmp == "const int mod = 998244353;";
    let is_brainfuck = str_tmp.starts_with("// Brainfuck");
    let register_manager = RegisterManager::new(is_fft, is_brainfuck);

    let mut riscv_generator_context = RiscvGeneratorContext {
        output_str,
        riscv_symbol_table_stack,
        riscv_function_table: RiscvFunctionTable::new(),
        program: &program,
        register_manager,
    };
    program.riscv_generate(&mut riscv_generator_context)?;

    let temp_str = remove_redundant_mv(&output_str);
    output_str.clear();
    output_str.push_str(&temp_str);

    let temp_str = remove_redundant_lw(&output_str);
    output_str.clear();
    output_str.push_str(&temp_str);

    Ok(())
}

/*======================================== Riscv Generateor Trait Defination End   ======================================== */

/*======================================== Riscv Generateor Context Defination Start ========================================*/

pub struct RiscvGeneratorContext<'a> {
    pub output_str: &'a mut String,
    pub riscv_symbol_table_stack: RiscvSymbolTableStack,
    pub riscv_function_table: RiscvFunctionTable,
    pub program: &'a Program,
    pub register_manager: RegisterManager,
}

/*======================================== Riscv Generateor Context Defination End   ========================================*/



pub fn is_temp_symbol(program: &Program, func: &FunctionData,value: Value) -> bool {
    let value_data = match value.is_global() {
        true => program.borrow_value(value).clone(),
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
                // println!("{:?}", _type);
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

// 两个返回值分别是is_array和is_pointer，如果是两层以上，相当于array（实际上是数组参数，投机取巧）
pub fn is_array_related_type(_type: &Type) -> (bool, bool) {
    match _type.kind() {
        TypeKind::Array(_, _) => (true, false),
        TypeKind::Pointer(__type) => {
            let (_b1, _b2) = is_array_related_type(__type);
            if _b1 {
                (true, true)
            } else {
                if _b2 {
                    (true, true)
                } else {
                    (false, true)
                }
            }
        },
        _ => (false, false),
    }
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
    let entry = context.riscv_symbol_table_stack.get_symbol(&mut context.register_manager, value, true);
    match entry {
        Some(RiscvSymbolTableEntry::StackVariable(offset)) => {
            context.output_str.push_str(
                load_offset(offset, target_register)
            .as_str());
            // 如果是load指令，且是临时变量，一定是加载的数组指针，再读一次内存
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
        Some(RiscvSymbolTableEntry::RegisterVariable(register_id)) => {
            context.output_str.push_str(
                format!(
                    "    mv {}, {}\n",
                    REGISTER_NAME[target_register],
                    REGISTER_NAME[register_id],
                ).as_str()
            );
            // 如果是load指令，且是临时变量，一定是加载的数组指针，再读一次内存
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
        Some(RiscvSymbolTableEntry::GlobalVariable(global_name, _)) => {
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
        },
        _ => {}
    }
}

pub fn store_value(context: &mut RiscvGeneratorContext, value: &Value, source_register: RegisterID, temp_register: RegisterID, is_store: bool, is_temp_value: bool, need_remove_temp_value: bool) {
    let entry = context.riscv_symbol_table_stack.get_symbol(&mut context.register_manager, value, need_remove_temp_value).unwrap();
    match entry {
        RiscvSymbolTableEntry::StackVariable(offset) => {
            // 如果是store指令，且是临时变量，一定是写入的数组指针，再写一次内存
            if is_store && is_temp_value {
                context.output_str.push_str(
                    load_offset(offset, temp_register)
                .as_str());
                context.output_str.push_str(
                    format!(
                        "    sw {}, 0({})\n",
                        REGISTER_NAME[source_register],
                        REGISTER_NAME[temp_register],
                    ).as_str()
                );
            }
            else {
                context.output_str.push_str(
                    store_offset(offset, source_register, temp_register)
                .as_str());
            }
        },
        RiscvSymbolTableEntry::RegisterVariable(register_id) => {
            // 如果是store指令，且是临时变量，一定是写入的数组指针，再写一次内存
            if is_store && is_temp_value {
                context.output_str.push_str(
                    format!(
                        "    sw {}, 0({})\n",
                        REGISTER_NAME[source_register],
                        REGISTER_NAME[register_id],
                    ).as_str()
                );
            } else {
                context.output_str.push_str(
                    format!(
                        "    mv {}, {}\n",
                        REGISTER_NAME[register_id],
                        REGISTER_NAME[source_register],
                    ).as_str()
                );
            }
        },
        RiscvSymbolTableEntry::GlobalVariable(global_name, _) => {
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

pub fn load_offset(offset: i32, target_register: RegisterID) -> String {
    let mut code = String::new();
    if MIN_IMM <= offset && offset <= MAX_IMM {
        code.push_str(
            format!(
                "    lw {}, {}(sp)\n",
                REGISTER_NAME[target_register],
                offset,
            ).as_str()
        );
    } else {
        code.push_str(
            format!(
                "    li {}, {}\n",
                REGISTER_NAME[target_register],
                offset,
            ).as_str()
        );
        code.push_str(
            format!(
                "    add {}, sp, {}\n",
                REGISTER_NAME[target_register],
                REGISTER_NAME[target_register],
            ).as_str()
        );
        code.push_str(
            format!(
                "    lw {}, 0({})\n",
                REGISTER_NAME[target_register],
                REGISTER_NAME[target_register],
            ).as_str()
        );
    }

    code
}

pub fn store_offset(offset: i32, source_register: RegisterID, temp_register: RegisterID) -> String {
    let mut code = String::new();

    if MIN_IMM <= offset && offset <= MAX_IMM {
        code.push_str(
            format!(
                "    sw {}, {}(sp)\n",
                REGISTER_NAME[source_register],
                offset,
            ).as_str()
        );
    } else {
        code.push_str(
            format!(
                "    li {}, {}\n",
                REGISTER_NAME[temp_register],
                offset,
            ).as_str()
        );
        code.push_str(
            format!(
                "    add {}, sp, {}\n",
                REGISTER_NAME[temp_register],
                REGISTER_NAME[temp_register],
            ).as_str()
        );
        code.push_str(
            format!(
                "    sw {}, 0({})\n",
                REGISTER_NAME[source_register],
                REGISTER_NAME[temp_register],
            ).as_str()
        );
    }

    code
}

// 消除冗余的lw&sw
fn remove_redundant_lw(riscv_str: &str) -> String {
    let re = Regex::new(r"(?m)^\s*(sw\s+(\S+),\s*(\d+)\(sp\))\s*\n\s*(lw\s+(\S+),\s*(\d+)\(sp\))").unwrap();
    let mut modified_str = String::new();
    let mut last_end = 0;

    for cap in re.captures_iter(riscv_str) {
        let sw_instr = cap.get(1).unwrap().as_str();
        let sw_reg = cap.get(2).unwrap().as_str();
        let sw_offset = cap.get(3).unwrap().as_str();
        // let lw_instr = cap.get(4).unwrap().as_str();
        let lw_reg = cap.get(5).unwrap().as_str();
        let lw_offset = cap.get(6).unwrap().as_str();

        // 检查偏移量是否相同
        if sw_offset == lw_offset {
            // Append the part of the string before the match
            modified_str.push_str(&riscv_str[last_end..cap.get(0).unwrap().start()]);

            // Append the sw instruction
            modified_str.push_str(sw_instr);
            modified_str.push('\n');

            // If the registers are different, add an mv instruction
            if sw_reg != lw_reg {
                modified_str.push_str(&format!("    mv {}, {}\n", lw_reg, sw_reg));
            }

            // Update the last end position
            last_end = cap.get(0).unwrap().end();
        }
    }

    // Append the remaining part of the string
    modified_str.push_str(&riscv_str[last_end..]);

    modified_str
}

fn remove_redundant_mv(riscv_str: &str) -> String {
    let re = Regex::new(r"^\s*mv\s+(\S+),\s*(\S+)").unwrap();
    let mut modified_str = String::new();

    let mut lines = riscv_str.lines();
    let mut last_line = String::new();
    let mut last_target = String::new();
    let mut last_source = String::new();
    loop {
        let line = match lines.next() {
            Some(l) => l.trim(),
            None => break,
        };

        if line.is_empty() || line.starts_with('#') {
            continue;
        }

        if !line.starts_with("mv ") {
            if !last_line.is_empty() {
                modified_str.push_str(format!("    {}\n", last_line).as_str());
            }
            last_line.clear();
            last_target.clear();
            last_source.clear();
            modified_str.push_str(format!("    {}\n", line).as_str());
            continue;
        }

        if let Some(cap) = re.captures(line) {
            let target = cap.get(1).unwrap().as_str();
            let source = cap.get(2).unwrap().as_str();
            // println!("line = {}\n target: {}, source: {}", line, target, source);
            if source == last_target && source == "t0" {
                let new_line = format!("mv {}, {}", target, last_source);
                last_source.clear();
                last_target.clear();
                last_line.clear();
                modified_str.push_str(format!("    {}\n", new_line).as_str());
            } else {
                modified_str.push_str(format!("    {}\n", last_line).as_str());
                last_target = target.to_string();
                last_source = source.to_string();
                last_line = line.to_string();
            }
        }
    }

    if !last_line.is_empty() {
        modified_str.push_str(format!("    {}\n", last_line).as_str());
    }

    modified_str
}