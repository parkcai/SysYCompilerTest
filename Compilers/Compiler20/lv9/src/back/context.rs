use crate::back::inst::{Register, A0, A1, A2, A3, A4, A5, A6, A7, SP};
use koopa::ir::entities::ValueData;
use koopa::ir::{Function, TypeKind};
use std::collections::HashMap;

pub const MAX_REG_PARAM: i32 = 8;

pub struct FuncContext {
    pub func: Function,
    pub value_map: HashMap<*const ValueData, Address>,
    pub alloc_size: usize,
    pub block_name: String,
    pub max_arg_num: i32,
}

#[derive(Clone)]
pub struct GlobalContext {
    pub value_map: HashMap<*const ValueData, Address>,
}

#[derive(Clone)]
pub struct Address {
    pub offset: usize,
    pub base: Register,
    pub is_in_reg: bool,
    pub is_global: bool,
    pub label: String,
}

impl GlobalContext {
    pub fn new() -> Self {
        GlobalContext {
            value_map: HashMap::new(),
        }
    }

    pub fn alloc_slot(&mut self, value_data: &ValueData, label: String) {
        let address = Address {
            offset: 0,
            base: SP,
            is_in_reg: false,
            is_global: true,
            label,
        };
        self.value_map.insert(value_data, address);
    }
}

impl FuncContext {
    pub fn new(func: Function) -> Self {
        FuncContext {
            func,
            value_map: HashMap::new(),
            alloc_size: 0,
            block_name: String::new(),
            max_arg_num: -1,
        }
    }

    pub fn add_global_context(&mut self, global_context: GlobalContext) {
        self.value_map.extend(global_context.value_map);
    }

    pub fn alloc_slot_for_arg(&mut self, value_data: &ValueData, order: i32) {
        let base;
        let address;
        if order < MAX_REG_PARAM {
            if order == 0 {
                base = A0
            } else if order == 1 {
                base = A1
            } else if order == 2 {
                base = A2
            } else if order == 3 {
                base = A3
            } else if order == 4 {
                base = A4
            } else if order == 5 {
                base = A5
            } else if order == 6 {
                base = A6
            } else {
                base = A7
            }
            address = Address {
                offset: 0,
                base,
                is_in_reg: order < MAX_REG_PARAM,
                is_global: false,
                label: String::new(),
            };
        } else {
            address = Address {
                offset: 4 * (order - MAX_REG_PARAM) as usize + self.alloc_size,
                base: SP,
                is_in_reg: false,
                is_global: false,
                label: String::new(),
            };
        }

        self.value_map.insert(value_data, address);
    }

    pub fn get_address_for_arg(&self, order: usize) -> Address {
        let base;
        let address;
        if order < MAX_REG_PARAM as usize {
            if order == 0 {
                base = A0
            } else if order == 1 {
                base = A1
            } else if order == 2 {
                base = A2
            } else if order == 3 {
                base = A3
            } else if order == 4 {
                base = A4
            } else if order == 5 {
                base = A5
            } else if order == 6 {
                base = A6
            } else {
                base = A7
            }
            address = Address {
                offset: 0,
                base,
                is_in_reg: order < MAX_REG_PARAM as usize,
                is_global: false,
                label: String::new(),
            };
        } else {
            address = Address {
                offset: 4 * (order - MAX_REG_PARAM as usize),
                base: SP,
                is_in_reg: false,
                is_global: false,
                label: String::new(),
            };
        }
        address
    }

    pub fn alloc_slot_alloc(&mut self, value_data: &ValueData) {
        let address = Address {
            offset: self.alloc_size,
            base: SP,
            is_in_reg: false,
            is_global: false,
            label: String::new(),
        };
        match value_data.ty().kind() {
            TypeKind::Pointer(base) => match base.kind() {
                TypeKind::Array(_, ..) => {
                    self.value_map.insert(value_data, address);
                    self.alloc_size += base.size();
                }
                _ => {
                    self.value_map.insert(value_data, address);
                    self.alloc_size += 4;
                }
            },
            _ => {
                self.value_map.insert(value_data, address);
                self.alloc_size += 4;
            }
        }
    }

    pub fn alloc_slot(&mut self, value_data: &ValueData) {
        let address = Address {
            offset: self.alloc_size,
            base: SP,
            is_in_reg: false,
            is_global: false,
            label: String::new(),
        };
        self.value_map.insert(value_data, address);
        self.alloc_size += 4;
    }

    pub fn get_address(&self, value_data: &ValueData) -> Option<&Address> {
        self.value_map.get(&(value_data as *const ValueData))
    }

    pub fn set_slot_for_args(&mut self) {
        // slot for storing arguments
        if self.max_arg_num > MAX_REG_PARAM {
            self.alloc_size += 4 * (self.max_arg_num - MAX_REG_PARAM) as usize;
        }
    }
}
