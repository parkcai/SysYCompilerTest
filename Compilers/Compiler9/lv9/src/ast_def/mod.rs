pub mod declaration;
pub mod expression;
pub mod function;
pub mod statement;
pub mod terminal_symbol;

use declaration::*;
use function::FuncDef;
use statement::Stmt;

pub fn print_type_of<T>(_: &T) {
    println!("{}", std::any::type_name::<T>());
}

#[derive(Debug)]
pub enum CompUnit {
    CompUnit(Vec<BasicUnit>),
}

#[derive(Debug)]
pub enum BasicUnit {
    FuncDef(FuncDef),
    Decl(Decl),
}

#[derive(Debug)]
pub enum Block {
    Block(Vec<BlockItem>),
}

#[derive(Debug)]
pub enum BlockItem {
    Stmt(Stmt),
    Decl(Decl),
}

