use koopa::ir::{BasicBlock, Value};

#[derive(Debug)]
pub struct IrCtx {
    symbol_vec: Vec<IrSymbol>,
    block_num: i32,
    pub(crate) if_returned: bool,
    label: Option<BasicBlock>,
    pub(crate) label_num: i32,
    pub result_num: i32,
    pub loop_info: Vec<LoopInfo>,
}

#[derive(Debug)]
struct IrSymbol {
    symbol: SymbolItem,
    alloc: Option<Value>,
}

#[derive(Debug)]
enum SymbolItem {
    BlockMark(i32),
    VarSymbol(VarSymbol),
    ConstSymbol(ConstSymbol),
}

#[derive(Debug, Eq, PartialEq)]
struct VarSymbol {
    id: String,
    if_ptr: bool,
}

#[derive(Debug, Eq, PartialEq)]
struct ConstSymbol {
    id: String,
    value: i32,
}

#[derive(Debug, Eq, PartialEq)]
pub struct LoopInfo {
    pub(crate) entry: BasicBlock,
    pub(crate) end: BasicBlock,
}

impl IrCtx {
    pub fn new() -> Self {
        IrCtx {
            symbol_vec: Vec::new(),
            block_num: 0,
            if_returned: false,
            label: None,
            label_num: 0,
            result_num: 0,
            loop_info: Vec::new(),
        }
    }

    pub fn into_block(&mut self) {
        self.block_num += 1;
        self.symbol_vec.push(IrSymbol {
            symbol: SymbolItem::BlockMark(self.block_num),
            alloc: None,
        });
    }

    pub fn out_block(&mut self) {
        while let Some(symbol) = self.symbol_vec.pop() {
            if let SymbolItem::BlockMark(_) = symbol.symbol {
                break;
            }
        }
    }

    pub fn get_block_num(&self) -> i32 {
        for symbol in self.symbol_vec.iter().rev() {
            if let SymbolItem::BlockMark(block_num) = symbol.symbol {
                return block_num;
            }
        }
        0
    }

    pub fn add_var(&mut self, id: String, alloc: Value, if_ptr: bool) {
        self.symbol_vec.push(IrSymbol {
            symbol: SymbolItem::VarSymbol(VarSymbol { id, if_ptr }),
            alloc: Some(alloc),
        });
    }

    pub fn add_const(&mut self, id: String, value: i32) {
        self.symbol_vec.push(IrSymbol {
            symbol: SymbolItem::ConstSymbol(ConstSymbol { id, value }),
            alloc: None,
        });
    }

    pub fn find_var(&self, id: &str) -> Option<Value> {
        for symbol in self.symbol_vec.iter().rev() {
            if let SymbolItem::VarSymbol(var_symbol) = &symbol.symbol {
                if var_symbol.id == id {
                    return symbol.alloc;
                }
            }
            if let SymbolItem::ConstSymbol(const_symbol) = &symbol.symbol {
                if const_symbol.id == id {
                    return None;
                }
            }
        }
        None
    }

    pub fn is_ptr(&self, id: &str) -> Option<bool> {
        for symbol in self.symbol_vec.iter().rev() {
            if let SymbolItem::VarSymbol(var_symbol) = &symbol.symbol {
                if var_symbol.id == id {
                    return Some(var_symbol.if_ptr);
                }
            }
            if let SymbolItem::ConstSymbol(const_symbol) = &symbol.symbol {
                if const_symbol.id == id {
                    return None;
                }
            }
        }
        None
    }

    pub fn find_const(&self, id: &str) -> Option<i32> {
        for symbol in self.symbol_vec.iter().rev() {
            if let SymbolItem::ConstSymbol(const_symbol) = &symbol.symbol {
                if const_symbol.id == id {
                    return Some(const_symbol.value);
                }
            }
        }
        None
    }

    pub fn set_basic_block(&mut self, label: BasicBlock) {
        self.label = Option::from(label);
    }

    pub fn get_basic_block(&self) -> Option<BasicBlock> {
        self.label
    }
}
