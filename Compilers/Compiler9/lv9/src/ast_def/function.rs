use super::terminal_symbol::{IDENT, BType};
use super::Block;
use super::expression::{ConstExp, Exp};

#[derive(Debug)]
pub enum FuncDef {
    FuncDef(BType, IDENT, Option<FuncFParams>, Block),
}

// #[derive(Debug)]
// pub enum FuncDecl {
    // FuncDecl(BType, IDENT, Option<FuncFParams>),
// }

#[derive(Debug)]
pub enum FuncFParams {
    FuncFParams(Vec<FuncFParam>),
}

#[derive(Debug)]
pub enum FuncFParam {
    FuncFParam(BType, IDENT, Option<Vec<ConstExp>>),
}

#[derive(Debug)]
pub enum FuncRParams {
    FuncRParams(Vec<FuncRParam>),
}

#[derive(Debug)]
pub enum FuncRParam {
    FuncRParam(Exp),
}
