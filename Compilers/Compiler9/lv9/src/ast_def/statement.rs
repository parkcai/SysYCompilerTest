use crate::ast_def::Block;
use crate::ast_def::expression::{ Exp, LVal };

#[derive(Debug)]
pub enum Stmt {
    AssignStmt(AssignStmt),
    BlockStmt(BlockStmt),
    ExpStmt(ExpStmt),
    IfStmt(IfStmt),
    WhileStmt(WhileStmt),
    BreakStmt(BreakStmt),
    ContinueStmt(ContinueStmt),
    ReturnStmt(ReturnStmt),
}

#[derive(Debug)]
pub enum AssignStmt {
    AssignStmt(LVal, Exp),
}

#[derive(Debug)]
pub enum BlockStmt {
    BlockStmt(Block),
}

#[derive(Debug)]
pub enum ExpStmt {
    ExpStmt(Option<Exp>),
}

/* If(Exp) {
    Stmt
} else {
    Option<Stmt>
} */
#[derive(Debug)]
pub enum IfStmt {
    IfStmt(Exp, Box<Stmt>, Option<Box<Stmt>>)
}

#[derive(Debug)]
pub enum WhileStmt {
    WhileStmt(Exp, Box<Stmt>),
}

#[derive(Debug)]
pub enum BreakStmt {
    BreakStmt,
}

#[derive(Debug)]
pub enum ContinueStmt {
    ContinueStmt,
}

#[derive(Debug)]
pub enum ReturnStmt {
    ReturnStmt(Option<Exp>),
}