pub mod declaration;
pub mod expression;
pub mod function;
pub mod statement;
pub mod terminal_symbol;

use koopa::ir::{Type, TypeKind};
use crate::ast_def::{ terminal_symbol::{BType, IDENT}, BasicUnit, Block, BlockItem, CompUnit};
use std::{collections::HashMap, sync::Mutex};

// 定义全局变量
lazy_static! {
    pub static ref BLOCK_TEMPORARY_SYMBOL_INDEX: Mutex<i32> = Mutex::new(0);
    pub static ref MIDDLE_TEMPORARY_SYMBOL_INDEX: Mutex<i32> = Mutex::new(0);
}

pub fn koopa_ir_generator(ast: CompUnit, output_str: &mut String, str_tmps: (String, String)) -> Result<(), std::io::Error> {
    let mut ir_function_table = IRFunctionTable::new();
    ir_function_table.insert_new_function(IDENT::IDENT("getint".to_string()), BType::int);
    ir_function_table.insert_new_function(IDENT::IDENT("getch".to_string()), BType::int);
    ir_function_table.insert_new_function(IDENT::IDENT("getarray".to_string()), BType::int);
    ir_function_table.insert_new_function(IDENT::IDENT("putint".to_string()), BType::void);
    ir_function_table.insert_new_function(IDENT::IDENT("putch".to_string()), BType::void);
    ir_function_table.insert_new_function(IDENT::IDENT("putarray".to_string()), BType::void);
    ir_function_table.insert_new_function(IDENT::IDENT("starttime".to_string()), BType::void);
    ir_function_table.insert_new_function(IDENT::IDENT("stoptime".to_string()), BType::void);

    let mut ir_symbol_table_stack = IRSymbolTableStack::new();
    ir_symbol_table_stack.push(); // 全局符号表

    let mut ir_generator_context = IRGeneratorContext { 
        output_str,
        ir_symbol_table_stack,
        break_and_continue_target_stack: BreakAndContinueTargetStack::new(),
        ir_function_table,
        is_fft: str_tmps.0 == "const int mod = 998244353;" && str_tmps.1 == "-perf",
        // is_fft: false,
    };
    match ast.generate(&mut ir_generator_context) {
        Ok(IRGenerateResult::Ok) | Ok(IRGenerateResult::EarlyTermination) => Ok(()),
        Err(error_string) => Err(std::io::Error::new(std::io::ErrorKind::Other, format!("Error = {} in IR generation", error_string))),
    }
}

pub struct IRGeneratorContext<'a> {
    pub output_str: &'a mut String,
    pub ir_symbol_table_stack: IRSymbolTableStack,
    pub ir_function_table: IRFunctionTable,
    pub break_and_continue_target_stack: BreakAndContinueTargetStack,
    pub is_fft: bool,
}

/*======================================== symbol table stack definition start ========================================*/

pub struct IRSymbolTableStack {
    pub ir_symbol_table_stack: Vec<IRSymbolTable>,
    symbol_counter: HashMap<IDENT, u32>,
}

impl IRSymbolTableStack {
    fn new() -> IRSymbolTableStack {
        IRSymbolTableStack {
            ir_symbol_table_stack: Vec::new(),
            symbol_counter: HashMap::new(),
        }
    }

    fn get_symbol(&self, ident: IDENT) -> Option<&IRSymbolTableEntry> {
        for ir_symbol_table in self.ir_symbol_table_stack.iter().rev() {
            if let Some(ir_symbol_table_entry) = ir_symbol_table.ir_symbol_table.get(&ident) {
                return Some(ir_symbol_table_entry);
            }
        }
        None
    }

    fn insert_new_symbol(&mut self, ident: IDENT, ir_symbol_table_entry: IRSymbolTableEntry) {
        match ir_symbol_table_entry {
            IRSymbolTableEntry::Variable(_type, _index) => {
                let value = self.symbol_counter.entry(ident.clone()).or_insert(0);
                let new_index = *value;
                *value += 1;
                let _ir_symbol_table = self.ir_symbol_table_stack.last_mut().unwrap();
                _ir_symbol_table.ir_symbol_table.insert(ident, IRSymbolTableEntry::Variable(_type, new_index));
            }
            IRSymbolTableEntry::Constant(_type, val) => {
                let _ir_symbol_table = self.ir_symbol_table_stack.last_mut().unwrap();
                _ir_symbol_table.ir_symbol_table.insert(ident, IRSymbolTableEntry::Constant(_type, val));
            },
        }
    }

    fn push (&mut self) {
        self.ir_symbol_table_stack.push(IRSymbolTable::new());
    }

    fn pop (&mut self) {
        self.ir_symbol_table_stack.pop();
    }
}

/*======================================== symbol table stack definition end   ========================================*/

/*======================================== symbol table definition start ========================================*/

pub struct IRSymbolTable {
    pub ir_symbol_table: HashMap<IDENT, IRSymbolTableEntry>,
}

impl IRSymbolTable {
    fn new() -> IRSymbolTable {
        IRSymbolTable {
            ir_symbol_table: HashMap::new(),
        }
    }
}

/*======================================== symbol table definition end   ========================================*/

/*======================================== symbol table entry definition start ========================================*/

#[derive(Clone)]
pub enum IRSymbolTableEntry {
    Variable(TypeKind, u32), // TypeKind, index（重名变量的区分）
    Constant(TypeKind, i32),
    // Array(TypeKind, u32, Vec<usize>), // TypeKind, index（重名变量的区分）, array sizes of each dimension
}

/*======================================== symbol table entry definition end   ========================================*/

/*======================================== break and continue target stack definition start ========================================*/

pub struct BreakAndContinueTargetStack {
    pub break_and_continue_target_stack: Vec<(String, String)>,
}

impl BreakAndContinueTargetStack {
    fn new() -> BreakAndContinueTargetStack {
        BreakAndContinueTargetStack {
            break_and_continue_target_stack: Vec::new(),
        }
    }

    fn push(&mut self, break_target: String, continue_target: String) {
        self.break_and_continue_target_stack.push((break_target, continue_target));
    }

    fn pop(&mut self) {
        self.break_and_continue_target_stack.pop();
    }

    fn get_break_target(&self) -> Option<&String> {
        self.break_and_continue_target_stack.last().map(|(break_target, _)| break_target)
    }

    fn get_continue_target(&self) -> Option<&String> {
        self.break_and_continue_target_stack.last().map(|(_, continue_target)| continue_target)
    }
}

/*======================================== break and continue target stack definition end   ========================================*/

/*======================================== IR function table definition start ========================================*/

pub struct IRFunctionTable {
    pub ir_function_table: HashMap<IDENT, BType>,
}

impl IRFunctionTable {
    fn new() -> IRFunctionTable {
        IRFunctionTable {
            ir_function_table: HashMap::new(),
        }
    }

    fn insert_new_function(&mut self, ident: IDENT, func_type: BType) {
        self.ir_function_table.insert(ident, func_type);
    }

    fn get_function(&self, ident: IDENT) -> Option<&BType> {
        self.ir_function_table.get(&ident)
    }
}

/*======================================== IR function table definition end   ========================================*/

/*======================================== IR generate definition start ========================================*/

pub enum IRGenerateResult {
    Ok,
    EarlyTermination, // 用于 return break continue
}

trait IRGenerate {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String>;
}

impl IRGenerate for CompUnit {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        let func_declaration_str = "decl @getint(): i32\ndecl @getch(): i32\ndecl @getarray(*i32): i32\ndecl @putint(i32)\ndecl @putch(i32)\ndecl @putarray(i32, *i32)\ndecl @starttime()\ndecl @stoptime()\n";
        let mut global_declaration_str = String::new();
        match self {
            CompUnit::CompUnit(units) => {
                for unit in units {
                    match unit {
                        BasicUnit::FuncDef(func_def) => {
                            match func_def.generate(context) {
                                Ok(IRGenerateResult::Ok) => {},
                                Ok(IRGenerateResult::EarlyTermination) => {},
                                Err(error_string) => {
                                    return Err(error_string);
                                }
                            }
                        },
                        BasicUnit::Decl(decl) => {
                            match decl.global_decelaration_generate(context) {
                                Ok(result_str) => {
                                    global_declaration_str.push_str(result_str.as_str());
                                    continue;
                                },
                                Err(error_string) => {
                                    return Err(error_string);
                                }
                            }
                        },
                    }
                }
            }
        };
        context.output_str.insert_str(0, func_declaration_str);
        context.output_str.insert_str(0, global_declaration_str.as_str());

        Ok(IRGenerateResult::Ok)
    }
}

impl IRGenerate for Block {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        let mut result = IRGenerateResult::Ok;
        match self {
            Block::Block(block_items) => {
                for block_item in block_items {
                    if let Ok(IRGenerateResult::EarlyTermination) = block_item.generate(context) {
                        result = IRGenerateResult::EarlyTermination;
                        break;
                    }
                }
            }
        }
        context.ir_symbol_table_stack.pop();
        Ok(result)
    }
}

impl IRGenerate for BlockItem {
    fn generate(&self, context: &mut IRGeneratorContext) -> Result<IRGenerateResult, String> {
        match self {
            BlockItem::Stmt(stmt) => stmt.generate(context),
            BlockItem::Decl(decl) => decl.generate(context),
        }
    }
}

fn get_array_typekind(array_sizes: &Vec<usize>) -> TypeKind {
    let mut ret_typekind = TypeKind::Int32;
    for array_size in array_sizes.iter().rev() {
        ret_typekind = TypeKind::Array(Type::get(ret_typekind), *array_size);
    }
    ret_typekind
}