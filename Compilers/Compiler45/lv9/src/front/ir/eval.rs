use crate::front::ast::*;
use crate::front::ident::Identifier;
use crate::front::ir::scope::Scope;

#[derive(Debug)]
pub enum EvalError {
    DivisionByZero,
    Overflow,
    NotSupportedVariable,
    FunctionNotSupported,
}

fn to_bool(x: i32) -> bool {
    x != 0
}

type EvalResult = Result<i32, EvalError>;

pub trait Eval {
    fn eval(&self, scope: &mut Scope) -> EvalResult;
}

impl Eval for i32 {
    fn eval(&self, _: &mut Scope) -> EvalResult {
        Ok(*self)
    }
}

impl Eval for ConstExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        self.0.eval(scope)
    }
}

impl Eval for LOrExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            LOrExpr::LAndExpr(and) => and.eval(scope),
            LOrExpr::Or(left, right) => {
                Ok((left.eval(scope).map(to_bool)? || right.eval(scope).map(to_bool)?) as i32)
            }
        }
    }
}

impl Eval for LAndExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            LAndExpr::EqExpr(eq) => eq.eval(scope),
            LAndExpr::And(left, right) => {
                Ok((left.eval(scope).map(to_bool)? && right.eval(scope).map(to_bool)?) as i32)
            }
        }
    }
}

impl Eval for EqExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            EqExpr::RelExpr(rel) => rel.eval(scope),
            EqExpr::Eq(left, op, right) => match op {
                EqOp::Eq => Ok((left.eval(scope)? == right.eval(scope)?) as i32),
                EqOp::Ne => Ok((left.eval(scope)? != right.eval(scope)?) as i32),
            },
        }
    }
}

impl Eval for RelExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            RelExpr::AddExpr(add) => add.eval(scope),
            RelExpr::Rel(left, op, right) => match op {
                RelOp::Lt => Ok((left.eval(scope)? < right.eval(scope)?) as i32),
                RelOp::Gt => Ok((left.eval(scope)? > right.eval(scope)?) as i32),
                RelOp::Le => Ok((left.eval(scope)? <= right.eval(scope)?) as i32),
                RelOp::Ge => Ok((left.eval(scope)? >= right.eval(scope)?) as i32),
            },
        }
    }
}

impl Eval for AddExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            AddExpr::MulExpr(mul_expr) => mul_expr.eval(scope),
            AddExpr::Add(left, op, right) => match op {
                AddOp::Add => left
                    .eval(scope)?
                    .checked_add(right.eval(scope)?)
                    .ok_or(EvalError::Overflow),
                AddOp::Sub => left
                    .eval(scope)?
                    .checked_sub(right.eval(scope)?)
                    .ok_or(EvalError::Overflow),
            },
        }
    }
}

impl Eval for MulExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            MulExpr::UnaryExpr(unary_expr) => unary_expr.eval(scope),
            MulExpr::Mul(left, op, right) => match op {
                MulOp::Div => left
                    .eval(scope)?
                    .checked_div(right.eval(scope)?)
                    .ok_or(EvalError::DivisionByZero),
                MulOp::Mod => left
                    .eval(scope)?
                    .checked_rem(right.eval(scope)?)
                    .ok_or(EvalError::DivisionByZero),
                MulOp::Mul => left
                    .eval(scope)?
                    .checked_mul(right.eval(scope)?)
                    .ok_or(EvalError::Overflow),
            },
        }
    }
}

impl Eval for UnaryExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            UnaryExpr::PrimaryExpr(primary_expr) => primary_expr.eval(scope),
            UnaryExpr::FuncCall(_) => Err(EvalError::FunctionNotSupported),
            UnaryExpr::Unary(op, unary_expr) => match op {
                UnaryOp::Neg => unary_expr.eval(scope).map(|x| -x),
                UnaryOp::Not => unary_expr.eval(scope).map(|x| if x == 0 { 1 } else { 0 }),
                UnaryOp::Pos => unary_expr.eval(scope),
            },
        }
    }
}

impl Eval for PrimaryExpr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            PrimaryExpr::Expr(expr) => expr.eval(scope),
            PrimaryExpr::LVal(lval) => lval.eval(scope),
            PrimaryExpr::Number(num) => num.eval(scope),
        }
    }
}

impl Eval for Expr {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        self.0.eval(scope)
    }
}

impl Eval for LVal {
    fn eval(&self, scope: &mut Scope) -> EvalResult {
        match self {
            LVal::Var(var) => {
                let id = scope
                    .get_identifier(var)
                    .ok_or(EvalError::NotSupportedVariable)?;
                let id = id.clone();
                match id {
                    Identifier::Constant(constant) => Ok(constant.value),
                    _ => Err(EvalError::NotSupportedVariable),
                }
            }
            LVal::ArrayElem(array_elem) => {
                let id = scope
                    .get_identifier(&array_elem.name)
                    .ok_or(EvalError::NotSupportedVariable)?
                    .clone();
                match id {
                    Identifier::ConstArray(const_array) => {
                        let indices = array_elem
                            .indices
                            .iter()
                            .map(|x| x.eval(scope))
                            .collect::<Result<Vec<_>, _>>()?;
                        Ok(const_array.values.get_element(&indices))
                    }
                    _ => Err(EvalError::NotSupportedVariable),
                }
            }
        }
    }
}
