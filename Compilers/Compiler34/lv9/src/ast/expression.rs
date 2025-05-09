use super::operator::{parse_operator, Operator, Parameter};
use super::variable::LVal;
use super::KoopaFunctionData;
use super::{ToKoopa, TypedValue};
use std::cell::RefCell;
use std::rc::Rc;

pub trait Evaluate {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32;
}

#[derive(Debug, Clone)]
pub enum PrimaryExp {
    Parenthese(Box<Exp>),
    Num(i32),
    LVal(LVal),
}

#[derive(Debug, Clone)]
pub enum Exp {
    LOr(Box<LOrExp>),
}

#[derive(Debug, Clone)]
pub enum ConstExp {
    Exp(Box<Exp>),
}

#[derive(Debug, Clone)]
pub enum LOrExp {
    LAnd(Box<LAndExp>),
    LOrOp(Box<LOrExp>, Box<LAndExp>),
}

#[derive(Debug, Clone)]
pub enum LAndExp {
    Eq(Box<EqExp>),
    LAndOp(Box<LAndExp>, Box<EqExp>),
}

#[derive(Debug, Clone)]
pub enum EqExp {
    Rel(Box<RelExp>),
    EqOp(EqOp, Box<EqExp>, Box<RelExp>),
}

#[derive(Debug, Clone)]
pub enum RelExp {
    Add(Box<AddExp>),
    RelOp(RelOp, Box<RelExp>, Box<AddExp>),
}

#[derive(Debug, Clone)]
pub enum AddExp {
    Mul(Box<MulExp>),
    AddOp(AddOp, Box<AddExp>, Box<MulExp>),
}

#[derive(Debug, Clone)]
pub enum MulExp {
    Unary(Box<UnaryExp>),
    MulOp(MulOp, Box<MulExp>, Box<UnaryExp>),
}

#[derive(Debug, Clone)]
pub enum UnaryExp {
    Primary(Box<PrimaryExp>),
    FuncCall(String, Vec<Exp>),
    UnaryOp(UnaryOp, Box<UnaryExp>),
}

#[derive(Debug, Clone)]
pub enum EqOp {
    Eq,
    Ne,
}

#[derive(Debug, Clone)]
pub enum RelOp {
    Gt,
    Lt,
    Ge,
    Le,
}

#[derive(Debug, Clone)]
pub enum MulOp {
    Mul,
    Div,
    Mod,
}

#[derive(Debug, Clone)]
pub enum AddOp {
    Add,
    Sub,
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    Pos,
    Neg,
    Opp,
}

impl Evaluate for ConstExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        let ConstExp::Exp(exp) = self;
        exp.eval(koopa_function)
    }
}

impl Evaluate for Exp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        let Exp::LOr(lor) = self;
        lor.eval(koopa_function)
    }
}

impl Evaluate for LOrExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            LOrExp::LAnd(land) => land.eval(koopa_function),
            LOrExp::LOrOp(lor, land) => {
                (lor.eval(Rc::clone(&koopa_function)) != 0
                    || land.eval(Rc::clone(&koopa_function)) != 0) as i32
            }
        }
    }
}

impl Evaluate for LAndExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            LAndExp::Eq(eq) => eq.eval(koopa_function),
            LAndExp::LAndOp(land, eq) => {
                (land.eval(Rc::clone(&koopa_function)) != 0
                    && eq.eval(Rc::clone(&koopa_function)) != 0) as i32
            }
        }
    }
}

impl Evaluate for EqExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            EqExp::Rel(rel) => rel.eval(koopa_function),
            EqExp::EqOp(op, eq, rel) => match op {
                EqOp::Eq => {
                    (eq.eval(Rc::clone(&koopa_function)) == rel.eval(Rc::clone(&koopa_function)))
                        as i32
                }
                EqOp::Ne => {
                    (eq.eval(Rc::clone(&koopa_function)) != rel.eval(Rc::clone(&koopa_function)))
                        as i32
                }
            },
        }
    }
}

impl Evaluate for RelExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            RelExp::Add(add) => add.eval(koopa_function),
            RelExp::RelOp(op, rel, add) => match op {
                RelOp::Ge => {
                    (rel.eval(Rc::clone(&koopa_function)) >= add.eval(Rc::clone(&koopa_function)))
                        as i32
                }
                RelOp::Gt => {
                    (rel.eval(Rc::clone(&koopa_function)) > add.eval(Rc::clone(&koopa_function)))
                        as i32
                }
                RelOp::Le => {
                    (rel.eval(Rc::clone(&koopa_function)) <= add.eval(Rc::clone(&koopa_function)))
                        as i32
                }
                RelOp::Lt => {
                    (rel.eval(Rc::clone(&koopa_function)) < add.eval(Rc::clone(&koopa_function)))
                        as i32
                }
            },
        }
    }
}

impl Evaluate for AddExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            AddExp::Mul(mul) => mul.eval(koopa_function),
            AddExp::AddOp(op, add, mul) => match op {
                AddOp::Add => {
                    add.eval(Rc::clone(&koopa_function)) + mul.eval(Rc::clone(&koopa_function))
                }
                AddOp::Sub => {
                    add.eval(Rc::clone(&koopa_function)) - mul.eval(Rc::clone(&koopa_function))
                }
            },
        }
    }
}

impl Evaluate for MulExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            MulExp::Unary(u) => u.eval(koopa_function),
            MulExp::MulOp(op, mul, u) => match op {
                MulOp::Mul => {
                    mul.eval(Rc::clone(&koopa_function)) * u.eval(Rc::clone(&koopa_function))
                }
                MulOp::Div => {
                    mul.eval(Rc::clone(&koopa_function)) / u.eval(Rc::clone(&koopa_function))
                }
                MulOp::Mod => {
                    mul.eval(Rc::clone(&koopa_function)) % u.eval(Rc::clone(&koopa_function))
                }
            },
        }
    }
}

impl Evaluate for UnaryExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            UnaryExp::Primary(p) => p.eval(koopa_function),
            UnaryExp::UnaryOp(op, u) => match op {
                UnaryOp::Neg => -u.eval(koopa_function),
                UnaryOp::Opp => (u.eval(koopa_function) == 0) as i32,
                UnaryOp::Pos => u.eval(koopa_function),
            },
            _ => unreachable!(),
        }
    }
}

impl Evaluate for PrimaryExp {
    fn eval(&self, koopa_function: Rc<RefCell<KoopaFunctionData>>) -> i32 {
        match self {
            PrimaryExp::Num(n) => *n,
            PrimaryExp::Parenthese(exp) => exp.eval(koopa_function),
            PrimaryExp::LVal(lv) => {
                let LVal::Ident(id, _index) = lv;
                let tv = koopa_function.borrow().get_symbol(id);
                match tv {
                    Some(v) => match v {
                        TypedValue::Int(n) => n,
                        TypedValue::MutInt(_) => {
                            panic!("Evaluating a mutable variable when compiling: {id}")
                        }
                        _ => unreachable!(),
                    },
                    None => panic!("Undefined Varible: {id}"),
                }
            }
        }
    }
}

impl ToKoopa for Exp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let Exp::LOr(lor) = self;
        lor.to_koopa(koopa_function)
    }
}

impl ToKoopa for ConstExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let ConstExp::Exp(exp) = self;
        exp.to_koopa(koopa_function)
    }
}

impl ToKoopa for LOrExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            LOrExp::LOrOp(lexp, rexp) => {
                let (lkp, lret) = lexp.to_koopa(Rc::clone(&koopa_function))?;
                let (rkp, rret) = rexp.to_koopa(Rc::clone(&koopa_function))?;
                let object1 = koopa_function.borrow_mut().counter.request_variable();
                let object2 = koopa_function.borrow_mut().counter.request_variable();
                let (ret, then_bb, else_bb, end_bb) =
                    koopa_function.borrow_mut().counter.request_or_count();
                let object3 = koopa_function.borrow_mut().counter.request_variable();
                let source1 = parse_operator(
                    &Operator::Or,
                    &vec![
                        Parameter::ConstStr(lret.clone()),
                        Parameter::ConstStr(rret.clone()),
                    ][..],
                );
                let source2 = parse_operator(
                    &Operator::Ne,
                    &vec![Parameter::ConstStr(object1.clone()), Parameter::ConstInt(0)][..],
                );
                Ok((
                    format!(
                        "{lkp}\t{} = alloc i32\n\tbr {}, {}, {}\n{}:\n\tstore 1, {}\n\tjump {}\n{}:\n{rkp}\t{} = {}\n\t{} = {}\n\tstore {}, {}\n\tjump {}\n{}:\n\t{} = load {}\n",
                        &ret, &lret, &then_bb, &else_bb, &then_bb, &ret, &end_bb, &else_bb, &object1, &source1, &object2, &source2, &object2, &ret, &end_bb, &end_bb, &object3, &ret
                    ),
                    object3,
                ))
            }
            LOrExp::LAnd(land) => land.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for LAndExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            LAndExp::LAndOp(lexp, rexp) => {
                let (lkp, lret) = lexp.to_koopa(Rc::clone(&koopa_function))?;
                let (rkp, rret) = rexp.to_koopa(Rc::clone(&koopa_function))?;
                let object1 = koopa_function.borrow_mut().counter.request_variable();
                let object2 = koopa_function.borrow_mut().counter.request_variable();
                let object3 = koopa_function.borrow_mut().counter.request_variable();
                let (ret, then_bb, else_bb, end_bb) =
                    koopa_function.borrow_mut().counter.request_and_count();
                let object4 = koopa_function.borrow_mut().counter.request_variable();
                let source1 = parse_operator(
                    &Operator::Ne,
                    &vec![Parameter::ConstStr(lret.clone()), Parameter::ConstInt(0)][..],
                );
                let source2 = parse_operator(
                    &Operator::Ne,
                    &vec![Parameter::ConstStr(rret.clone()), Parameter::ConstInt(0)][..],
                );
                let source3 = parse_operator(
                    &Operator::And,
                    &vec![
                        Parameter::ConstStr(object1.clone()),
                        Parameter::ConstStr(object2.clone()),
                    ][..],
                );
                Ok((
                    format!(
                        "{lkp}\t{} = alloc i32\n\tbr {}, {}, {}\n{}:\n{rkp}\t{} = {}\n\t{} = {}\n\t{} = {}\n\tstore {}, {}\n\tjump {}\n{}:\n\tstore 0, {}\n\tjump {}\n{}:\n\t{} = load {}\n",
                        &ret, &lret, &then_bb, &else_bb, &then_bb, &object1, &source1, &object2, &source2, &object3, &source3, &object3, &ret, &end_bb, &else_bb, &ret, &end_bb, &end_bb, &object4, &ret
                    ),
                    object4
                ))
            }
            LAndExp::Eq(leq) => leq.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for EqExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            EqExp::EqOp(op, lexp, rexp) => {
                let (lkp, lret) = lexp.to_koopa(Rc::clone(&koopa_function))?;
                let (rkp, rret) = rexp.to_koopa(Rc::clone(&koopa_function))?;
                let object = koopa_function.borrow_mut().counter.request_variable();
                let source = match op {
                    EqOp::Eq => parse_operator(
                        &Operator::Eq,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    EqOp::Ne => parse_operator(
                        &Operator::Ne,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                };
                Ok((
                    format!("{}{}\t{} = {}\n", lkp, rkp, &object, &source),
                    object,
                ))
            }
            EqExp::Rel(rel) => rel.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for RelExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            RelExp::RelOp(op, lexp, rexp) => {
                let (lkp, lret) = lexp.to_koopa(Rc::clone(&koopa_function))?;
                let (rkp, rret) = rexp.to_koopa(Rc::clone(&koopa_function))?;
                let object = koopa_function.borrow_mut().counter.request_variable();
                let source = match op {
                    RelOp::Ge => parse_operator(
                        &Operator::Ge,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    RelOp::Gt => parse_operator(
                        &Operator::Gt,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    RelOp::Le => parse_operator(
                        &Operator::Le,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    RelOp::Lt => parse_operator(
                        &Operator::Lt,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                };
                Ok((
                    format!("{}{}\t{} = {}\n", lkp, rkp, &object, &source),
                    object,
                ))
            }
            RelExp::Add(add) => add.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for AddExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            AddExp::AddOp(op, lexp, rexp) => {
                let (lkp, lret) = lexp.to_koopa(Rc::clone(&koopa_function))?;
                let (rkp, rret) = rexp.to_koopa(Rc::clone(&koopa_function))?;
                let object = koopa_function.borrow_mut().counter.request_variable();
                let source = match op {
                    AddOp::Add => parse_operator(
                        &Operator::Add,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    AddOp::Sub => parse_operator(
                        &Operator::Sub,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                };
                Ok((
                    format!("{}{}\t{} = {}\n", lkp, rkp, &object, &source),
                    object,
                ))
            }
            AddExp::Mul(mul) => mul.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for MulExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            MulExp::MulOp(op, lexp, rexp) => {
                let (lkp, lret) = lexp.to_koopa(Rc::clone(&koopa_function))?;
                let (rkp, rret) = rexp.to_koopa(Rc::clone(&koopa_function))?;
                let object = koopa_function.borrow_mut().counter.request_variable();
                let source = match op {
                    MulOp::Mul => parse_operator(
                        &Operator::Mul,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    MulOp::Div => parse_operator(
                        &Operator::Div,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                    MulOp::Mod => parse_operator(
                        &Operator::Mod,
                        &vec![
                            Parameter::ConstStr(lret.clone()),
                            Parameter::ConstStr(rret.clone()),
                        ][..],
                    ),
                };
                Ok((
                    format!("{}{}\t{} = {}\n", lkp, rkp, &object, &source),
                    object,
                ))
            }
            MulExp::Unary(unary) => unary.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for UnaryExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            UnaryExp::FuncCall(func_id, params) => {
                let func_id = koopa_function.borrow().get_symbol(func_id).unwrap();
                let mut kp = String::new();
                let mut param_list = String::from("(");
                for param in params {
                    let (pkp, pret) = param.to_koopa(Rc::clone(&koopa_function))?;
                    kp.push_str(&pkp);
                    param_list.push_str(&format!("{pret}, "));
                }
                if param_list.len() != 1 {
                    param_list.pop();
                    param_list.pop();
                }
                param_list.push_str(")");
                match func_id {
                    TypedValue::IntFunc(func_id) => {
                        let dest = koopa_function.borrow_mut().counter.request_variable();
                        Ok((
                            format!("{kp}\t{} = call {func_id}{param_list}\n", dest.clone()),
                            dest,
                        ))
                    }
                    TypedValue::VoidFunc(func_id) => {
                        Ok((format!("{kp}\tcall {func_id}{param_list}\n"), String::new()))
                    }
                    _ => unreachable!(),
                }
            }
            UnaryExp::UnaryOp(op, unary) => match op {
                UnaryOp::Pos => unary.to_koopa(koopa_function),
                _ => {
                    let (kp, ret) = unary.to_koopa(Rc::clone(&koopa_function))?;
                    let object = koopa_function.borrow_mut().counter.request_variable();
                    let source = match op {
                        UnaryOp::Neg => parse_operator(
                            &Operator::Sub,
                            &vec![Parameter::ConstInt(0), Parameter::ConstStr(ret.clone())][..],
                        ),
                        UnaryOp::Opp => parse_operator(
                            &Operator::Eq,
                            &vec![Parameter::ConstInt(0), Parameter::ConstStr(ret.clone())][..],
                        ),
                        _ => unreachable!(),
                    };
                    Ok((format!("{}\t{} = {}\n", kp, &object, &source), object))
                }
            },
            UnaryExp::Primary(primary) => primary.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for PrimaryExp {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            PrimaryExp::Parenthese(exp) => exp.to_koopa(koopa_function),
            PrimaryExp::Num(num) => Ok((String::new(), num.to_string())),
            PrimaryExp::LVal(lval) => lval.to_koopa(koopa_function),
        }
    }
}
