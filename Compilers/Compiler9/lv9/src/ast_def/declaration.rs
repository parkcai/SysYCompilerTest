use crate::ast_def::terminal_symbol::{IDENT, BType};
use crate::ast_def::expression::{ ConstExp, Exp };

#[derive(Debug)]
pub enum Decl {
    ConstDecl(ConstDecl),
    VarDecl(VarDecl),
}

#[derive(Debug)]
pub enum ConstDecl {
    ConstDecl(BType, Vec<ConstDef>),
}

#[derive(Debug)]
pub enum ConstDef {
    ConstDef(IDENT, Vec<ConstExp>, InitVal),
    // ConstDef(IDENT, ConstInitVal),
}

// #[derive(Debug)]
// pub enum ConstInitVal {
//     ConstExp(ConstExp),
//     Arr(Vec<Box<ConstInitVal>>),
// }

#[derive(Debug)]
pub enum VarDecl {
    VarDecl(BType, Vec<VarDef>),
}

#[derive(Debug)]
pub enum VarDef {
    VarDef(IDENT, Vec<ConstExp>, Option<InitVal>),
    // VarDef(IDENT, Option<InitVal>),
}

#[derive(Debug)]
pub enum InitVal {
    Exp(Exp),
    Arr(Vec<Box<InitVal>>),
}