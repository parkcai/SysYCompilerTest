use core::panic;
use std::collections::HashMap;

use koopa::ir::{FunctionData, Program, TypeKind, Value, ValueKind};

use super::{is_temp_symbol, load_offset, store_offset, RegisterID, REGISTER_NAME, T0_REGISTER_ID};

// #[derive(Debug)]
pub struct RegisterManager {
    _temporary_registers: Vec<RegisterID>,
    _usable_temporary_registers: Vec<bool>,
    _variable_registers: Vec<RegisterID>,
    _usable_variable_registers: Vec<bool>,
    _usable_global_variable_registers: Vec<bool>,
    _variable_reference_counter: HashMap<Value, usize>,
    _is_fft: bool,
    _is_brainfuck: bool,
    _global_value_on_register: HashMap<Value, RegisterID>,
}

impl RegisterManager {
    pub fn new (is_fft: bool, is_brainfuck: bool) -> Self {
        let temp = match is_fft {
            true => vec![28, 29, 30, 31, 16, 17, 11, 12, 13, 14, 15, 10], // t3-t6, a0-a7, caller-saved
            false => vec![11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31, 10], // a0-a7, t3-t6, caller-saved
        };
        RegisterManager {
            _usable_temporary_registers: vec![true; 12],
            _variable_registers: vec![8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27], // s0-s11, callee-saved
            _usable_variable_registers: vec![true; 12],
            _usable_global_variable_registers: vec![true; 12],
            _variable_reference_counter: HashMap::new(),
            _is_fft: is_fft,
            _is_brainfuck: is_brainfuck,
            _temporary_registers: temp,
            _global_value_on_register: HashMap::new(),
        }
    }

    // 调试用
    pub fn temporary_register_occupancy_reporter(&self) -> String {
        let mut free = String::new();
        let mut occupied = String::new();
        for i in 0..12 {
            if self._usable_temporary_registers[i] {
                free.push_str(REGISTER_NAME[self._temporary_registers[i]]);
            } else {
                occupied.push_str(REGISTER_NAME[self._temporary_registers[i]]);
            }
        }
        format!("   # Temporary registers: occupied: {}, free: {}\n", occupied, free)
    }

    pub fn reset(&mut self) {
        let tmp = self._global_value_on_register.clone();
        *self = RegisterManager::new(self._is_fft, self._is_brainfuck);
        self._global_value_on_register = tmp;
    }

    pub fn insert_value(&mut self, is_temp_value: bool) -> RegisterID {
        match is_temp_value {
            true => {
                for i in 0..12 {
                    if self._usable_temporary_registers[i] {
                        self._usable_temporary_registers[i] = false;
                        return self._temporary_registers[i];
                    }
                }
                panic!("No available temporary register");
            },
            false => {
                for i in 0..12 {
                    if self._usable_variable_registers[i] {
                        self._usable_variable_registers[i] = false;
                        return self._variable_registers[i];
                    }
                }
                panic!("No available variable register");
            }
        }
    }

    pub fn insert_global_value(&mut self, value: &Value) -> RegisterID {
        for i in (0..12).rev() {
            if self._usable_global_variable_registers[i] {
                self._usable_global_variable_registers[i] = false;
                // println!("Global value {:?} is assigned to register {}", value, REGISTER_NAME[self._variable_registers[i]]);
                self._global_value_on_register.insert(value.clone(), self._variable_registers[i]);
                return self._variable_registers[i];
            }   
        }
        panic!("No available global variable register");
    }

    pub fn get_global_value(&self, value: &Value) -> RegisterID {
        // println!("{:?}", self._global_value_on_register);
        match self._global_value_on_register.get(value) {
            Some(&register_id) => register_id,
            None => panic!("Global value = {:?} not found in register manager", value),
        }
    }

    pub fn remove_temp_value(&mut self, register_id: RegisterID) {
        for i in 0..12 {
            if self._temporary_registers[i] == register_id {
                // if self._usable_temporary_registers[i] {
                //     println!("register released error. value: {:?}", value);
                // }
                self._usable_temporary_registers[i] = true;
                return;
            }
        }
    }

    pub fn remove_variable_value(&mut self, register_id: RegisterID, value: &Value) {
        if !self._is_brainfuck {
            return;
        }
        let count_option = self._variable_reference_counter.get_mut(value);
        if count_option.is_none() {
            return;
        }
        let count = count_option.unwrap();
        *count -= 1;
        if *count == 0 {
            for i in 0..12 {
                if self._variable_registers[i] == register_id {
                    self._usable_variable_registers[i] = true;
                    // println!("release {}", REGISTER_NAME[register_id]);
                    return;
                }
            }
        }
    }

    pub fn generate_save_code_for_caller_saved_register(&mut self, stack_size: usize) -> String {
        let mut code = String::new();
        for i in 0..12 {
            if !self._usable_temporary_registers[i] {
                let register_id = self._temporary_registers[i];
                code.push_str(
                    store_offset(stack_size as i32 - register_id as i32 * 4, register_id, T0_REGISTER_ID)
                .as_str());
            }
        }
        code
    }

    pub fn generate_reload_code_for_caller_saved_register(&mut self, stack_size: usize) -> String {
        let mut code = String::new();
        for i in 0..12 {
            if !self._usable_temporary_registers[i] {
                let register_id = self._temporary_registers[i];
                code.push_str(
                    load_offset(stack_size as i32 - register_id as i32 * 4, register_id)
                .as_str());
            }
        }
        code
    }

    pub fn generate_save_code_for_callee_saved_register(&mut self, stack_size: usize) -> String {
        let mut code = String::new();
        for i in 0..12 {
            if !self._usable_variable_registers[i] {
                let register_id = self._variable_registers[i];
                code.push_str(
                    store_offset(stack_size as i32 - register_id as i32 * 4, register_id, T0_REGISTER_ID)
                .as_str());
            }
        }
        code
    }

    pub fn generate_reload_code_for_callee_saved_register(&mut self, stack_size: usize) -> String {
        let mut code = String::new();
        // let mut count = 0;
        for i in 0..12 {
            if !self._usable_variable_registers[i] {
                let register_id = self._variable_registers[i];
                code.push_str(
                    load_offset( stack_size as i32 - register_id as i32 * 4, register_id)
                .as_str());
                // println!("{}", REGISTER_NAME[register_id]);
                // count += 1;
            }
        }
        // println!("Reloaded {} callee-saved registers", count);
        code
    }

    pub fn count_variable_reference_in_function(&mut self, program: &Program, function: &FunctionData) {
        if function.layout().entry_bb().is_none() {
            return;
        }

        for (&_bb, node) in function.layout().bbs() {
            for inst in node.insts().keys() {
                let inst_data = function.dfg().value(*inst);
                match inst_data.kind() {
                    ValueKind::Alloc(_alloc) => {
                        match inst_data.ty().kind() {
                            TypeKind::Pointer(_) => { // alloc的一定是指针
                                if !is_temp_symbol(program, function, *inst) {
                                    self._variable_reference_counter.insert(*inst, 0);
                                }
                            }
                            _ => {}
                        }
                    }

                    ValueKind::Store(_store) => {
                        let dest_value = _store.dest();
                        if !is_temp_symbol(program, function, dest_value) && !dest_value.is_global() {
                            let counter = self._variable_reference_counter.entry(dest_value).or_insert(0);
                            *counter += 1;
                        }
                    },

                    ValueKind::Load(_load) => {
                        let src_value = _load.src();
                        if !is_temp_symbol(program, function, src_value) && !src_value.is_global() {
                            let counter = self._variable_reference_counter.entry(src_value).or_insert(0);
                            *counter += 1;
                        }
                    },

                    ValueKind::GetElemPtr(_get_elem_ptr) => {
                        let src_value = _get_elem_ptr.src();
                        // let index_value = _get_elem_ptr.index();
                        if !is_temp_symbol(program, function, src_value) && !src_value.is_global() {
                            let counter = self._variable_reference_counter.entry(src_value).or_insert(0);
                            *counter += 1;
                        }
                    },

                    _ => {}
                }
            }
        }
        // println!("In function {}, {} variables are referred:", function.name(), self._variable_reference_counter.len());
        // for (&value, &count) in &self._variable_reference_counter {
        //     if !value.is_global() {
        //         println!("Local {:?} with name {} is referenced {} times", value, function.dfg().value(value).name().clone().unwrap(), count);
        //     } else {
        //         println!("Global {:?} with name {} is referenced {} times", value, program.borrow_value(value).name().clone().unwrap(), count);
        //     }
        // }
        // println!("");
    }
}

/*======================================== Riscv Generateor Context Defination End   ========================================*/