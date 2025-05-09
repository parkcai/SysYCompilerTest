use std::collections::HashMap;

use koopa::ir::{entities::ValueData, Function, Value};

use super::{get_type_size, is_array_related_type, register_manager::RegisterManager, RegisterID};

/*======================================== Riscv Symbol Table Stack Defination Start ========================================*/

pub struct RiscvSymbolTableStack {
    pub riscv_symbol_table_stack: Vec<RiscvSymbolTable>,
}

impl RiscvSymbolTableStack {
    pub fn new() -> RiscvSymbolTableStack {
        RiscvSymbolTableStack {
            riscv_symbol_table_stack: Vec::new(),
        }
    }

    // 在调用时，减引用次数，根据entry种类跟register manager交互。由于具名变量数量实际不多，所以暂时不考虑具名变量的引用计数
    pub fn get_symbol(&self, register_manager: &mut RegisterManager, value: &Value, need_remove_temp_value: bool) -> Option<RiscvSymbolTableEntry> {
        for table in self.riscv_symbol_table_stack.iter().rev() {
            if let Some(entry) = table.riscv_symbol_table.get(value) {
                match &entry {
                    // 当获得一个寄存器变量时，如果是临时符号说明可以将寄存器释放了，需要特殊处理
                    RiscvSymbolTableEntry::RegisterVariable(register_id) => {
                        if need_remove_temp_value {
                            register_manager.remove_temp_value(*register_id);
                        }
                        register_manager.remove_variable_value(*register_id, value);
                    },
                    // 当获得一个全局变量时，需要根据它是不是数组判断是否在寄存器上
                    // 是数组，直接返回GlobalVariable枚举
                    // 不是数组，新造一个RegisterVariable枚举返回
                    RiscvSymbolTableEntry::GlobalVariable(_, is_global_array) => {
                        if *is_global_array {
                            return Some(entry.clone());
                        } else {
                            let register_id = register_manager.get_global_value(value);
                            return Some(RiscvSymbolTableEntry::RegisterVariable(register_id));
                        }
                    },
                    _ => {}
                };
                return Some(entry.clone());
            }
        }
        None
    }

    // 只有当entry为None时，才会考虑is_temp_value的值
    // is_temp_value为true时，直接往寄存器上分配，此时忽略extra_data的值
    // is_temp_value为false时，考虑extra_data的内容。extra_data是一个元组，第一个是value_data，用于计算size，第二项是栈中的传参空间（a）。
    //      根据value_data的大小，判断是数组还是普通变量，数组分配到栈，普通变量分配到寄存器
    pub fn insert_new_symbol(&mut self, register_manager: &mut RegisterManager, value: &Value, riscv_symbol_table_entry: Option<RiscvSymbolTableEntry>, is_temp_value: bool, extra_data: Option<(&ValueData, usize)>) -> Option<RiscvSymbolTableEntry> {
        let _riscv_symbol_table: &mut _ = self.riscv_symbol_table_stack.last_mut().unwrap();
        match riscv_symbol_table_entry {
            Some(entry) => {
                match &entry {
                    RiscvSymbolTableEntry::GlobalVariable(_, is_global_array) => {
                        if !*is_global_array {
                            register_manager.insert_global_value(value);
                        }
                    },
                    _ => {}
                };
                _riscv_symbol_table.riscv_symbol_table.insert(*value, entry) // 只有在调用函数，生成call指令准备参数的时候才会用到返回值
            },
            None => {
                match is_temp_value {
                    true => {
                        let register_id = register_manager.insert_value(true);
                        _riscv_symbol_table.riscv_symbol_table.insert(*value, RiscvSymbolTableEntry::RegisterVariable(register_id));
                        None
                    },
                    false => {
                        let (value_data, a) = extra_data.unwrap();
                        let (is_array, _) = is_array_related_type(value_data.ty());
                        // println!("value_data: {:?}, is array = {}", value_data, is_array);
                        if is_array { // is array
                            let new_offset = (_riscv_symbol_table.current_size + a) as i32;
                            _riscv_symbol_table.riscv_symbol_table.insert(*value, RiscvSymbolTableEntry::StackVariable(new_offset));
                            self.set_size_by_delta(get_type_size(value_data, true));
                        } else {
                            let register_id = register_manager.insert_value(false);
                            _riscv_symbol_table.riscv_symbol_table.insert(*value, RiscvSymbolTableEntry::RegisterVariable(register_id));
                        }
                        None
                    }
                }
            }
        }        
    }

    pub fn push(&mut self) {
        self.riscv_symbol_table_stack.push(RiscvSymbolTable {
            riscv_symbol_table: HashMap::new(),
            current_size: 0,
        });
    }

    pub fn pop(&mut self) {
        self.riscv_symbol_table_stack.pop();
    }

    pub fn get_size(&self) -> usize {
        self.riscv_symbol_table_stack.last().unwrap().current_size
    }

    pub fn set_size_by_delta(&mut self, delta: usize) {
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
    pub fn new() -> RiscvFunctionTable {
        RiscvFunctionTable {
            riscv_function_table: HashMap::new(),
        }
    }

    pub fn insert(&mut self, func_id: Function, name: String, param_num: usize) {
        self.riscv_function_table.insert(func_id, (name, param_num));
    }

    pub fn get(&self, func_id: &Function) -> Option<&(String, usize)> {
        self.riscv_function_table.get(&func_id)
    }
}

/*======================================== Riscv Function Table Defination End   ========================================*/

/*======================================== Riscv Symbol Table Entry Defination Start ========================================*/

#[derive(Clone)]
pub enum RiscvSymbolTableEntry {
    StackVariable(i32), // offset
    RegisterVariable(RegisterID), // 寄存器id
    GlobalVariable(String, bool), // 全局变量名，是否是数组（不是数组直接永久放到寄存器里）这个分量只需要在get_symbol函数中考虑，外界能够获得这个枚举说明一定是全局数组，可以直接忽略
}

impl std::fmt::Display for RiscvSymbolTableEntry {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            RiscvSymbolTableEntry::StackVariable(offset) => write!(f, "StackVariable({})", offset)?,
            RiscvSymbolTableEntry::RegisterVariable(register_id) => write!(f, "RegisterVariable({})", register_id)?,
            RiscvSymbolTableEntry::GlobalVariable(name, is_array) => write!(f, "GlobalVariable({}, is array = {})", name, is_array)?,
        };
        Ok(())
    }
}

/*======================================== Riscv Symbol Table Entry Defination End   ========================================*/