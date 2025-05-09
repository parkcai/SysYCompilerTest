#[derive(Debug)]
pub struct CompUnit {
    pub items: Vec<GlobalItem>,
}

#[derive(Debug)]
pub enum GlobalItem {
    Decl(Decl),
    FuncDef(FuncDef),
}

#[derive(Debug)]
pub enum Decl {
    VarDecl(Vec<VarDef>),
    ConstDecl(Vec<ConstDef>),
}

#[derive(Debug)]
pub struct ConstDef {
    pub id: String,
    pub shape: Vec<ConstExp>,
    pub val: ConstInitVal,
}

#[derive(Debug)]
pub enum ConstInitVal {
    ConstExp(ConstExp),
    ConstInitList(Vec<ConstInitVal>),
}

#[derive(Debug)]
pub struct VarDef {
    pub id: String,
    pub shape: Vec<ConstExp>,
    pub init: Option<InitVal>,
}

#[derive(Debug)]
pub enum InitVal {
    Exp(Exp),
    InitList(Vec<InitVal>),
}

#[derive(Debug)]
pub struct FuncDef {
    pub ret_type: FuncType,
    pub id: String,
    pub params: FuncFParams,
    pub body: Block,
}

#[derive(Debug)]
pub enum FuncType {
    Void,
    Int,
}

#[derive(Debug)]
pub struct FuncFParams {
    pub params: Vec<FuncFParam>,
}

#[derive(Debug)]
pub enum FuncFParam {
    NormalParam(NormalParam),
    ArrayParam(ArrayParam),
}

#[derive(Debug)]
pub struct NormalParam {
    pub id: String,
}

#[derive(Debug)]
pub struct ArrayParam {
    pub id: String,
    pub shape: Vec<Exp>,
}

#[derive(Debug)]
pub struct Block {
    pub items: Vec<BlockItem>,
}

#[derive(Debug)]
pub enum BlockItem {
    Decl(Decl),
    Stmt(Stmt),
}

#[derive(Debug)]
pub enum Stmt {
    Assign(Assign),
    Exp(ExpStmt),
    Block(Block),
    If(Box<If>),
    While(Box<While>),
    Break,
    Continue,
    Return(Return),
}

#[derive(Debug)]
pub struct Assign {
    pub lhs: LVal,
    pub rhs: Exp,
}

#[derive(Debug)]
pub struct ExpStmt {
    pub exp: Option<Exp>,
}

#[derive(Debug)]
pub struct If {
    pub cond: Cond,
    pub then: Box<Stmt>,
    pub else_: Option<Stmt>,
}

#[derive(Debug)]
pub struct While {
    pub cond: Cond,
    pub body: Box<Stmt>,
}

#[derive(Debug)]
pub struct Return {
    pub exp: Option<Exp>,
}

#[derive(Debug, Clone)]
pub struct Exp {
    pub exp: Box<LOrExp>,
}

#[derive(Debug)]
pub struct Cond {
    pub lor: LOrExp,
}

#[derive(Debug, Clone)]
pub struct LVal {
    pub id: String,
    pub indices: Vec<Exp>,
}

#[derive(Debug, Clone)]
pub struct FuncCall {
    pub id: String,
    pub args: Vec<Exp>,
}

#[derive(Debug)]
pub struct ConstExp {
    pub exp: LOrExp,
}

#[derive(Debug, Clone)]
pub enum AddExp {
    MulExp(Box<MulExp>),
    AddMul(Box<AddExp>, AddOp, MulExp),
}

#[derive(Debug, Clone)]
pub enum MulExp {
    UnaryExp(Box<UnaryExp>),
    MulUnary(Box<MulExp>, MulOp, UnaryExp),
}

#[derive(Debug, Clone)]
pub enum LOrExp {
    LAndExp(LAndExp),
    Or(Box<LOrExp>, LAndExp),
}

#[derive(Debug, Clone)]
pub enum LAndExp {
    EqExp(EqExp),
    And(Box<LAndExp>, EqExp),
}

#[derive(Debug, Clone)]
pub enum EqExp {
    RelExp(RelExp),
    Eq(Box<EqExp>, EqOp, RelExp),
}

#[derive(Debug, Clone)]
pub enum RelExp {
    AddExp(AddExp),
    Rel(Box<RelExp>, RelOp, AddExp),
}

#[derive(Debug, Clone)]
pub enum UnaryExp {
    PrimaryExp(Box<PrimaryExp>),
    Unary(UnaryOp, Box<UnaryExp>),
    Call(FuncCall),
}

#[derive(Debug, Clone)]
pub enum PrimaryExp {
    Exp(Box<Exp>),
    LVal(LVal),
    IntConst(i32),
}

// #[derive(Debug)]
// pub struct IntConst {
//     pub val: i32,
// }

#[derive(Debug, PartialEq, Clone)]
pub enum AddOp {
    Add,
    Sub,
}

#[derive(Debug, PartialEq, Clone)]
pub enum MulOp {
    Mul,
    Div,
    Mod,
}

#[derive(Debug, PartialEq, Clone)]
pub enum UnaryOp {
    Pos,
    Neg,
    Not,
}

#[derive(Debug, PartialEq, Clone)]
pub enum EqOp {
    Eq,
    Neq,
}

#[derive(Debug, PartialEq, Clone)]
pub enum RelOp {
    Lt,
    Gt,
    Le,
    Ge,
}
