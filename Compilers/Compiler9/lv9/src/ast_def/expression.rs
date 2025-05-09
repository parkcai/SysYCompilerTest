use crate::ast_def::{function::FuncRParams, terminal_symbol::IDENT};

use crate::ast_def::terminal_symbol::INT_CONST;

#[derive(Debug)]
pub enum Exp {
    LOrExp(LOrExp),
}

#[derive(Debug)]
pub enum ConstExp {
    Exp(Exp),
}


#[derive(Debug)]
pub enum LVal {
    // LVal(IDENT, Option<Vec<Exp>>)
    LVal(IDENT, Vec<Exp>),
}

#[derive(Debug)]
pub enum LOrExp {
    LAndExp(LAndExp),
    LOrExp(Box<LOrExp>, LAndExp),
}

#[derive(Debug)]
pub enum LAndExp {
    EqExp(EqExp),
    LAndExp(Box<LAndExp>, EqExp),
}

#[derive(Debug)]
pub enum EqExp {
    RelExp(RelExp),
    EqExp(Box<EqExp>, RelExp),
    NeqExp(Box<EqExp>, RelExp),
}

#[derive(Debug)]
pub enum RelExp {
    AddExp(AddExp),
    LtRelExp(Box<RelExp>, AddExp),
    GtRelExp(Box<RelExp>, AddExp),
    LeRelExp(Box<RelExp>, AddExp),
    GeRelExp(Box<RelExp>, AddExp),
}

#[derive(Debug)]
pub enum AddExp {
    MulExp(MulExp),
    AddExp(Box<AddExp>, MulExp),
    MinusExp(Box<AddExp>, MulExp),
}

#[derive(Debug)]
pub enum MulExp {
    UnaryExp(UnaryExp),
    MulExp(Box<MulExp>, UnaryExp),
    DivideExp(Box<MulExp>, UnaryExp),
    ModExp(Box<MulExp>, UnaryExp),
}

#[derive(Debug)]
pub enum UnaryExp {
    PrimaryExp(PrimaryExp),
    FunctionCall(IDENT, Option<FuncRParams>),
    UnaryExp(UnaryOp, Box<UnaryExp>),
}

#[derive(Debug)]
pub enum PrimaryExp {
    Number(INT_CONST),
    LVal(LVal),
    Exp(Box<Exp>),
}

#[derive(Debug)]
pub enum UnaryOp {
    Plus,
    Minus,
    Not,
}