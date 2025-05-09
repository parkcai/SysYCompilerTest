use koopa::ir::{BinaryOp, Type};
use std::rc::Rc;

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct CompUnit {
    pub items: Vec<GlobalItem>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum GlobalItem {
    Decl(Decl),
    FuncDef(Rc<FuncDef>),
}

/// Represent const declaration or non-const variable declaration.
///
/// One declaration statement may contains more than one declaration.
#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum Decl {
    ConstDecl(Vec<Rc<ConstDef>>),
    VarDecl(Vec<Rc<VarDef>>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum ConstDef {
    NormalConstDef(NormalConstDef),
    ArrayConstDef(ArrayConstDef),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct NormalConstDef {
    pub name: String,
    pub value: ConstExpr,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct ArrayConstDef {
    pub name: String,
    pub shape: Vec<ConstExpr>,
    pub values: ConstArray,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum ConstArray {
    Val(ConstExpr),
    Array(Vec<ConstArray>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum VarDef {
    NormalVarDef(NormalVarDef),
    ArrayVarDef(ArrayVarDef),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct NormalVarDef {
    pub name: String,
    pub value: Option<Expr>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct ArrayVarDef {
    pub name: String,
    pub shape: Vec<ConstExpr>,
    pub values: Option<ExprArray>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum ExprArray {
    Val(Expr),
    Array(Vec<ExprArray>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Expr(pub Rc<LOrExpr>);

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct FuncDef {
    pub name: String,
    pub params: Vec<Rc<FuncFParam>>,
    pub ret_type: DataType,
    pub body: Block,
}

/// Represent function parameter in declaration.
#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum FuncFParam {
    NormalFParam(NormalFParam),
    ArrayFParam(ArrayFParam),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct NormalFParam {
    pub name: String,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct ArrayFParam {
    pub name: String,
    /// Whether the array has a placeholder. For example, `int a[]` has a placeholder.
    pub placeholder: bool,
    pub shape: Vec<ConstExpr>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone, Copy)]
pub enum DataType {
    Void,
    Int,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Block {
    pub id: i32,
    pub items: Vec<BlockItem>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum BlockItem {
    Stmt(Stmt),
    Decl(Decl),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Break;

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Continue;

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum Stmt {
    Assign(Assign),
    Expr(Expr),
    Block(Block),
    If(If),
    While(While),
    Return(Option<Expr>),
    Break(Break),
    Continue(Continue),
    Empty,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Assign {
    pub target: LVal,
    pub value: Expr,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum LVal {
    Var(String),
    ArrayElem(ArrayElem),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct ArrayElem {
    pub name: String,
    pub indices: Vec<Expr>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct If {
    pub cond: LOrExpr,
    pub then_stmt: Rc<Stmt>,
    pub else_stmt: Option<Rc<Stmt>>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct While {
    pub cond: LOrExpr,
    pub body: Rc<Stmt>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum LOrExpr {
    LAndExpr(LAndExpr),
    Or(Rc<LOrExpr>, Rc<LAndExpr>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum LAndExpr {
    EqExpr(EqExpr),
    And(Rc<LAndExpr>, Rc<EqExpr>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone, Copy)]
pub enum EqOp {
    Eq,
    Ne,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum EqExpr {
    RelExpr(RelExpr),
    Eq(Rc<EqExpr>, EqOp, Rc<RelExpr>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone, Copy)]
pub enum RelOp {
    Lt,
    Gt,
    Le,
    Ge,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum RelExpr {
    AddExpr(AddExpr),
    Rel(Rc<RelExpr>, RelOp, Rc<AddExpr>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone, Copy)]
pub enum AddOp {
    Add,
    Sub,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum AddExpr {
    MulExpr(MulExpr),
    Add(Rc<AddExpr>, AddOp, Rc<MulExpr>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone, Copy)]
pub enum MulOp {
    Mul,
    Div,
    Mod,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum MulExpr {
    UnaryExpr(UnaryExpr),
    Mul(Rc<MulExpr>, MulOp, Rc<UnaryExpr>),
}

/// Unary operator.
#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum UnaryOp {
    /// Positive(+).
    Pos,
    /// Negative(-).
    Neg,
    /// Logical not(!).
    Not,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum UnaryExpr {
    PrimaryExpr(PrimaryExpr),
    FuncCall(FuncCall),
    Unary(UnaryOp, Rc<UnaryExpr>),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum PrimaryExpr {
    Expr(Expr),
    LVal(LVal),
    Number(i32),
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct FuncCall {
    pub name: String,
    pub args: Vec<Expr>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct ConstExpr(pub Rc<LOrExpr>);

impl Into<BinaryOp> for RelOp {
    fn into(self) -> BinaryOp {
        match self {
            RelOp::Lt => BinaryOp::Lt,
            RelOp::Gt => BinaryOp::Gt,
            RelOp::Le => BinaryOp::Le,
            RelOp::Ge => BinaryOp::Ge,
        }
    }
}

impl Into<BinaryOp> for EqOp {
    fn into(self) -> BinaryOp {
        match self {
            EqOp::Eq => BinaryOp::Eq,
            EqOp::Ne => BinaryOp::NotEq,
        }
    }
}

impl Into<BinaryOp> for AddOp {
    fn into(self) -> BinaryOp {
        match self {
            AddOp::Add => BinaryOp::Add,
            AddOp::Sub => BinaryOp::Sub,
        }
    }
}

impl Into<BinaryOp> for MulOp {
    fn into(self) -> BinaryOp {
        match self {
            MulOp::Mul => BinaryOp::Mul,
            MulOp::Div => BinaryOp::Div,
            MulOp::Mod => BinaryOp::Mod,
        }
    }
}

impl Into<Type> for DataType {
    fn into(self) -> Type {
        match self {
            DataType::Void => Type::get_unit(),
            DataType::Int => Type::get_i32(),
        }
    }
}

impl Default for Expr {
    fn default() -> Self {
        Expr(Rc::new(LOrExpr::LAndExpr(LAndExpr::EqExpr(
            EqExpr::RelExpr(RelExpr::AddExpr(AddExpr::MulExpr(MulExpr::UnaryExpr(
                UnaryExpr::PrimaryExpr(PrimaryExpr::Number(0)),
            )))),
        ))))
    }
}

#[cfg(test)]
mod test_ast;
