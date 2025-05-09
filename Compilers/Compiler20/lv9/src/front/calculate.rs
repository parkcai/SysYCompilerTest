use crate::ast::*;
use crate::front::ctx::IrCtx;
use std::process::exit;

pub trait Eval {
    fn calculate(&self, ctx: &mut IrCtx) -> i32;
}

impl Eval for Exp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        self.exp.calculate(ctx)
    }
}

impl Eval for LOrExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            LOrExp::LAndExp(exp) => exp.calculate(ctx),
            LOrExp::Or(exp1, exp2) => {
                if exp1.calculate(ctx) == 0 && exp2.calculate(ctx) == 0 {
                    0
                } else {
                    1
                }
            }
        }
    }
}

impl Eval for LAndExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            LAndExp::EqExp(exp) => exp.calculate(ctx),
            LAndExp::And(exp1, exp2) => {
                if exp1.calculate(ctx) == 0 || exp2.calculate(ctx) == 0 {
                    0
                } else {
                    1
                }
            }
        }
    }
}

impl Eval for EqExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            EqExp::RelExp(exp) => exp.calculate(ctx),
            EqExp::Eq(exp1, op, exp2) => match op {
                EqOp::Eq => {
                    if exp1.calculate(ctx) == exp2.calculate(ctx) {
                        1
                    } else {
                        0
                    }
                }
                EqOp::Neq => {
                    if exp1.calculate(ctx) != exp2.calculate(ctx) {
                        1
                    } else {
                        0
                    }
                }
            },
        }
    }
}

impl Eval for RelExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            RelExp::AddExp(exp) => exp.calculate(ctx),
            RelExp::Rel(exp1, op, exp2) => match op {
                RelOp::Lt => {
                    if exp1.calculate(ctx) < exp2.calculate(ctx) {
                        1
                    } else {
                        0
                    }
                }
                RelOp::Le => {
                    if exp1.calculate(ctx) <= exp2.calculate(ctx) {
                        1
                    } else {
                        0
                    }
                }
                RelOp::Gt => {
                    if exp1.calculate(ctx) > exp2.calculate(ctx) {
                        1
                    } else {
                        0
                    }
                }
                RelOp::Ge => {
                    if exp1.calculate(ctx) >= exp2.calculate(ctx) {
                        1
                    } else {
                        0
                    }
                }
            },
        }
    }
}

impl Eval for AddExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            AddExp::MulExp(exp) => exp.calculate(ctx),
            AddExp::AddMul(exp1, op, exp2) => match op {
                AddOp::Add => exp1.calculate(ctx) + exp2.calculate(ctx),
                AddOp::Sub => exp1.calculate(ctx) - exp2.calculate(ctx),
            },
        }
    }
}

impl Eval for MulExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            MulExp::UnaryExp(exp) => exp.calculate(ctx),
            MulExp::MulUnary(exp1, op, exp2) => match op {
                MulOp::Mul => exp1.calculate(ctx) * exp2.calculate(ctx),
                MulOp::Div => exp1.calculate(ctx) / exp2.calculate(ctx),
                MulOp::Mod => exp1.calculate(ctx) % exp2.calculate(ctx),
            },
        }
    }
}

impl Eval for UnaryExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            UnaryExp::PrimaryExp(exp) => exp.calculate(ctx),
            UnaryExp::Unary(op, exp) => match op {
                UnaryOp::Pos => exp.calculate(ctx),
                UnaryOp::Neg => -exp.calculate(ctx),
                UnaryOp::Not => {
                    if exp.calculate(ctx) == 0 {
                        1
                    } else {
                        0
                    }
                }
            },
            UnaryExp::Call(_) => unreachable!(),
        }
    }
}

impl Eval for PrimaryExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        match self {
            PrimaryExp::Exp(exp) => exp.calculate(ctx),
            PrimaryExp::LVal(lval) => {
                let id = lval.id.clone();
                let symbol = ctx.find_const(&id);
                symbol.unwrap_or_else(|| {
                    println!("Symbol not found: {}", id);
                    exit(1)
                })
            }
            PrimaryExp::IntConst(val) => *val,
        }
    }
}

impl Eval for ConstExp {
    fn calculate(&self, ctx: &mut IrCtx) -> i32 {
        self.exp.calculate(ctx)
    }
}
