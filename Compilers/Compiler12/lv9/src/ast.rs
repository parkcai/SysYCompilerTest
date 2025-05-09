use std::vec::Vec;
use std::collections::HashMap;
use std::fmt;

impl fmt::Display for Exp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // 这里假设 Exp 只包含 LOrExp，递归调用其 Display 实现
        write!(f, "{}", self.lor)
    }
}

// 递归为 LOrExp 及其相关子类型实现 Display
impl fmt::Display for LOrExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LOrExp::LAnd(exp) => write!(f, "{}", exp),
            LOrExp::LOrLAnd(lhs, rhs) => write!(f, "{} || {}", lhs, rhs),
        }
    }
}

impl fmt::Display for LAndExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LAndExp::Eq(exp) => write!(f, "{}", exp),
            LAndExp::LAndEq(lhs, rhs) => write!(f, "{} && {}", lhs, rhs),
        }
    }
}


impl fmt::Display for EqExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EqExp::Rel(exp) => write!(f, "{}", exp),
            EqExp::EqRel(lhs, op, rhs) => {
                let op_str = match op {
                    EqOp::Eq => "==",
                    EqOp::Neq => "!=",
                };
                write!(f, "({} {} {})", lhs, op_str, rhs)
            }
        }
    }
}

impl fmt::Display for RelExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            RelExp::Add(exp) => write!(f, "{}", exp),
            RelExp::RelAdd(lhs, op, rhs) => {
                let op_str = match op {
                    RelOp::Lt => "<",
                    RelOp::Gt => ">",
                    RelOp::Le => "<=",
                    RelOp::Ge => ">=",
                };
                write!(f, "({} {} {})", lhs, op_str, rhs)
            }
        }
    }
}

impl fmt::Display for AddExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            AddExp::Mul(exp) => write!(f, "{}", exp),
            AddExp::AddMul(lhs, op, rhs) => {
                let op_str = match op {
                    AddOp::Add => "+",
                    AddOp::Sub => "-",
                };
                write!(f, "({} {} {})", lhs, op_str, rhs)
            }
        }
    }
}

impl fmt::Display for MulExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            MulExp::Unary(exp) => write!(f, "{}", exp),
            MulExp::MulUnary(lhs, op, rhs) => {
                let op_str = match op {
                    MulOp::Mul => "*",
                    MulOp::Div => "/",
                    MulOp::Mod => "%",
                };
                write!(f, "({} {} {})", lhs, op_str, rhs)
            }
        }
    }
}

impl fmt::Display for UnaryExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            UnaryExp::Primary(exp) => write!(f, "{}", exp),
            UnaryExp::Unary(op, exp) => {
                let op_str = match op {
                    UnaryOp::Neg => "-",
                    UnaryOp::LNot => "!",
                };
                write!(f, "({}{})", op_str, exp)
            }
            UnaryExp::Call { .. } => {
                panic!("Function call evaluation not supported in this stage");
            }
        }
    }
}

impl fmt::Display for PrimaryExp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            PrimaryExp::Exp(exp) => write!(f, "({})", exp),
            PrimaryExp::Lval(lval) => write!(f, "{}", lval),
            PrimaryExp::Number(value) => write!(f, "{}", value),
        }
    }
}

impl fmt::Display for Lval {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.ident)
    }
}

#[derive(Debug)]
pub enum Symbol {
    Constant(i32),               // 单一常量值
    Variable(String),            // 变量名对应的内存分配地址
    Array { // 数组
        element_type: BType,
        size: Vec<usize>,
        alloc: String, // 数组名对应的内存分配地址
    },
    // 尝试引入指针，也就是数组函数参数。比如int a[],a为*i32;int a[][10],a为*[i32,10]
    // size: 例如a[][10],size为[10]，只有1维
    Pointer {
        element_type: BType,
        size: Vec<usize>,
        alloc: String, // 指针名对应的内存分配地址
    },
    Function {                   // 函数信息
        return_type: Option<FuncType>,
        params: Vec<FuncFParam>,
    },
}

impl Symbol {
    pub fn is_pointer_to_i32(&self, symbol_table: &SymbolTable) -> bool {
        match self {
            Symbol::Variable(_) => true, // 普通变量可能是指针，继续检查类型
            Symbol::Array { size,..} => {
                 // 如果是数组类型，只有在没有维度时才认为是 *i32 类型
                 size.is_empty() 
            }
            _ => false, // 非变量或数组的类型不可能是 *i32
        }
    }
}

// 更新符号表以支持嵌套作用域
#[derive(Debug)]
pub struct SymbolTable {
    scopes: Vec<HashMap<String, Symbol>>, // 使用栈存储作用域
    counter: HashMap<String, usize>,      // 用于生成唯一名称的计数器
}

impl SymbolTable {
    pub fn new() -> Self {
        SymbolTable {
            scopes: vec![HashMap::new()], // 初始化全局作用域
            counter: HashMap::new(),
        }
    }

    // 进入新作用域
    pub fn enter_scope(&mut self) {
        self.scopes.push(HashMap::new());
    }

    // 退出当前作用域
    pub fn exit_scope(&mut self) {
        self.scopes.pop();
    }

    // 插入常量定义
    pub fn insert_constant(&mut self, name: String, value: i32) -> bool {
        if let Some(current_scope) = self.scopes.last_mut() {
            if current_scope.contains_key(&name) {
                return false; // 符号重定义
            }
            current_scope.insert(name, Symbol::Constant(value));
            true
        } else {
            false
        }
    }

    // 插入变量定义
    pub fn insert_variable(&mut self, name: String, alloc: String) -> bool {
        if let Some(current_scope) = self.scopes.last_mut() {
            if current_scope.contains_key(&name) {
                return false; // 符号重定义
            }
            current_scope.insert(name, Symbol::Variable(alloc));
            true
        } else {
            false
        }
    }

    // 插入变量数组
    pub fn insert_array(
        &mut self,
        name: String,
        element_type: BType,
        size: Vec<usize>,
        alloc: String,
    ) -> bool {
        if let Some(current_scope) = self.scopes.last_mut() {
            if current_scope.contains_key(&name) {
                return false; // 符号重定义
            }
            current_scope.insert(
                name,
                Symbol::Array {
                    element_type,
                    size,
                    alloc,
                },
            );
            true
        } else {
            false
        }
    }

    // 插入数组指针
    pub fn insert_pointer(
        &mut self,
        name: String,
        element_type: BType,
        size: Vec<usize>,
        alloc: String,
    ) -> bool {
        if let Some(current_scope) = self.scopes.last_mut() {
            if current_scope.contains_key(&name) {
                return false; // 符号重定义
            }
            current_scope.insert(
                name,
                Symbol::Pointer {
                    element_type,
                    size,
                    alloc,
                },
            );
            true
        } else {
            false
        }
    }

    pub fn insert_function(
        &mut self,
        name: String,
        return_type: Option<FuncType>,
        params: Vec<FuncFParam>,
    ) -> bool {
        if let Some(current_scope) = self.scopes.last_mut() {
            if current_scope.contains_key(&name) {
                return false; // 符号重定义
            }
            current_scope.insert(
                name,
                Symbol::Function {
                    return_type,
                    params,
                },
            );
            true
        } else {
            false
        }
    }

    // 查询符号定义（支持跨作用域查询）
    pub fn get(&self, name: &str) -> Option<&Symbol> {
        for scope in self.scopes.iter().rev() {
            if let Some(symbol) = scope.get(name) {
                return Some(symbol);
            }
        }
        None
    }

    pub fn is_array(&self, ident: &str) -> bool {
        self.get(ident)
            .map(|symbol| matches!(symbol, Symbol::Array { .. }))
            .unwrap_or(false)
    }
}

#[derive(Debug)]
pub struct CompUnit {
    pub items: Vec<CompUnitItem>,
}

#[derive(Debug)]
pub enum CompUnitItem {
    Decl(Decl),
    FuncDef(FuncDef),
}

#[derive(Debug)]
pub struct FuncDef {
    pub func_type: FuncType,
    pub ident: String,
    pub params: Option<FuncFParams>, // 函数形式参数
    pub block: Block,
}

#[derive(Debug)]
pub struct FuncHeader {
    pub func_type: FuncType,
    pub ident: String,
}

#[derive(Clone, Debug)]
pub struct FuncFParams {
    pub func_fparam: FuncFParam,            // 第一个形式参数
    pub params: Option<Vec<FuncFParam>>,   // 其他形式参数（可选）
}

#[derive(Debug)]
pub struct FuncRParams {
    pub exp: Exp,                    // 第一个参数
    pub exps: Option<Vec<Exp>>,      // 剩余的参数（可选）
}

impl FuncRParams {
    // 转换为 Vec<Exp>
    pub fn to_vec(self) -> Vec<Exp> {
        let mut all_exps = vec![self.exp];
        if let Some(more_exps) = self.exps {
            all_exps.extend(more_exps);
        }
        all_exps
    }
}

#[derive(Clone, Debug)]
pub struct FuncFParam {
    pub btype: BType,             // 参数的基本类型
    pub ident: String,            // 参数名
    pub dims: Option<Vec<ConstExp>>, // 数组参数的维度信息（第一维省略）
    pub is_array: bool,           // 是否是数组参数
}

impl FuncFParam {
    pub fn eval_dims(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> Vec<usize> {
        if let Some(dims) = &self.dims {
            dims.iter()
                .map(|dim| dim.eval(symbol_table,global_symbol_table) as usize)
                .collect()
        } else {
            vec![] // 非数组参数返回空维度
        }
    }
}

// 更新 FuncType，支持 void
#[derive(Clone, Debug)]
pub enum FuncType {
    Void,
    Int,
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
pub enum Decl {
    ConstDecl(ConstDecl),
    VarDecl(VarDecl),
}

// 常量声明
#[derive(Debug)]
pub struct ConstDecl {
    pub btype: BType,
    pub def: ConstDef,
    pub defs: Option<Vec<ConstDef>>,
}

// 变量声明
#[derive(Debug)]
pub struct VarDecl {
    pub btype: BType,
    pub def: VarDef,
    pub defs: Option<Vec<VarDef>>,
}

#[derive(Clone, Debug)]
pub enum BType {
    Int,
}

#[derive(Debug)]
pub struct ConstDef {
    pub ident: String,
    pub sizes: Option<Vec<ConstExp>>, // 多维数组的尺寸
    pub constinitval: ConstInitVal, // 常量初始化值
}


#[derive(Debug)]
pub enum ConstInitVal {
    Single(ConstExp),          // 单值初始化
    Array(Vec<ConstInitVal>),  // 数组初始化
}

impl ConstInitVal {
    pub fn eval<'a>(
        &self, 
        dimensions: &[usize], 
        symbol_table: &SymbolTable,
        align_product:&Vec<usize>,
        flat_values:&'a mut Vec<i32>,
        global_symbol_table:&SymbolTable
) ->&'a mut Vec<i32> {
        // 思路是把初始化值展平为1维，然后根据维度信息填充
        let len=dimensions.len();

        match self {
            ConstInitVal::Single(exp) => {
                flat_values.push(exp.eval(symbol_table,global_symbol_table));
                flat_values
            }
            ConstInitVal::Array(subvalues) => {
                let mut now_num=flat_values.len();
                for val in subvalues {
                    val.eval(&dimensions, symbol_table,align_product,flat_values,global_symbol_table);
                }
                let mut align_size=0;
                if now_num==0{
                    align_size=dimensions.iter().product();
                    if len>1{
                        align_size/=dimensions[0];
                    }
                }else{
                    for i in align_product.iter().rev(){
                        if now_num%(*i)==0{
                            align_size=*i;
                            break;
                        }
                    }
                }
                println!("flat_values_before={:?}",flat_values);
                println!("align_size=={}, now_num={},now_product[0]={}",align_size,now_num,align_product[0]);
                if align_size==0{
                    panic!("align_size==0, now_num={},now_product[0]={}",now_num,align_product[0]);
                }
                // 计算对齐后的长度,比如align_size=4,now_num=10,则now_size=12
                now_num=flat_values.len();
                let mut now_size=0;
                if subvalues.len()==0{
                    println!("subvalues.len()==0");
                    //如果这个array是{},必须前进一个当前的对齐量
                    now_size=now_num+align_size;
                }
                else{
                    now_size=(now_num+align_size-1)/align_size*align_size;
                }
                flat_values.resize(now_size, 0); // 补零填满
                println!("flat_values_after={:?}",flat_values);
                flat_values
            }
        }
    }
}

#[derive(Clone, Debug)]
pub struct ConstExp {
    pub exp: Exp,
}

impl ConstExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        self.exp.eval(symbol_table,global_symbol_table)
    }
}

#[derive(Debug)]
pub struct VarDef {
    pub ident: String,
    pub sizes: Option<Vec<ConstExp>>, // 多维数组的尺寸
    pub initval: Option<InitVal>, // 初始化值
}

#[derive(Debug)]
pub enum InitVal {
    Single(Exp),          // 单值初始化
    Array(Vec<InitVal>),  // 数组初始化
}

impl InitVal {
    pub fn eval<'a>(
        &self, 
        dimensions: &[usize], 
        symbol_table: &SymbolTable,
        align_product:&Vec<usize>,
        flat_values:&'a mut Vec<i32>,
        global_symbol_table:&SymbolTable
) ->&'a mut Vec<i32> {
        // 思路是把初始化值展平为1维，然后根据维度信息填充
        let len=dimensions.len();

        match self {
            InitVal::Single(exp) => {
                flat_values.push(exp.eval(symbol_table,global_symbol_table));
                flat_values
            }
            InitVal::Array(subvalues) => {
                let mut now_num=flat_values.len();
                for val in subvalues {
                    val.eval(&dimensions, symbol_table,align_product,flat_values,global_symbol_table);
                }
                let mut align_size=0;
                if now_num==0{
                    align_size=dimensions.iter().product();
                    if len>1{
                        align_size/=dimensions[0];
                    }
                }else{
                    for i in align_product.iter().rev(){
                        if now_num%(*i)==0{
                            align_size=*i;
                            break;
                        }
                    }
                }
                println!("flat_values_before={:?}",flat_values);
                println!("align_size=={}, now_num={},now_product[0]={}",align_size,now_num,align_product[0]);
                if align_size==0{
                    panic!("align_size==0, now_num={},now_product[0]={}",now_num,align_product[0]);
                }
                // 计算对齐后的长度,比如align_size=4,now_num=10,则now_size=12
                now_num=flat_values.len();
                let mut now_size=0;
                if subvalues.len()==0{
                    println!("subvalues.len()==0");
                    //如果这个array是{},必须前进一个当前的对齐量
                    now_size=now_num+align_size;
                }
                else{
                    now_size=(now_num+align_size-1)/align_size*align_size;
                }
                flat_values.resize(now_size, 0); // 补零填满
                println!("flat_values_after={:?}",flat_values);
                flat_values
            }
        }
    }
}

#[derive(Debug)]
pub enum Stmt {
    Assign { lval: Lval, exp: Exp },
    Return { exp: Option<Exp> },
    Block(Block),       // 新增 Block 类型语句
    ExpStmt { exp: Option<Exp> }, // 新增 ExpStmt 类型语句
    IfElse { cond: Exp, then_branch: Box<Stmt>, else_branch: Option<Box<Stmt>> }, // 新增 IfElse 类型
    While { cond: Exp, body: Box<Stmt> }, // 新增 While 变种
    Break,
    Continue,
    Matched(Box<Stmt>), // 新增：已匹配语句
    Open(Box<Stmt>),    // 新增：未匹配语句
}// 更新符号表以支持嵌套作用域

// 表达式结构
#[derive(Clone, Debug)]
pub struct Exp {
    pub lor: Box<LOrExp>,
}

impl Exp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        self.lor.eval(symbol_table,global_symbol_table)
    }
}

#[derive(Clone, Debug)]
pub enum PrimaryExp {
    Exp(Box<Exp>),
    Lval(Lval),
    Number(i32),
}


impl PrimaryExp {
    pub fn eval(&self, symbol_table: &SymbolTable, global_symbol_table: &SymbolTable) -> i32 {
        match self {
            // 如果是嵌套表达式，递归求值
            PrimaryExp::Exp(exp) => exp.eval(symbol_table,global_symbol_table),
            
            // 如果是变量 Lval，需要从符号表中查找
            PrimaryExp::Lval(lval) => match symbol_table.get(&lval.ident) {
                Some(Symbol::Constant(value)) => *value, // 如果是单一常量，直接返回值
                Some(Symbol::Array { .. }) => {
                    panic!("Cannot evaluate variable array '{}' at compile time", lval.ident);
                }
                Some(Symbol::Pointer { .. }) => {
                    panic!("Cannot evaluate pointer '{}' as a constant", lval.ident);
                }
                Some(Symbol::Variable(_alloc)) => panic!("Cannot evaluate variable '{}' as a constant", lval.ident),
                Some(Symbol::Function { .. }) => panic!("Cannot evaluate a function '{}' as a constant", lval.ident), // 函数调用不应该出现在这里
                None => match global_symbol_table.get(&lval.ident){
                    Some(Symbol::Constant(value)) => *value,
                    Some(Symbol::Array { .. }) => panic!("Cannot evaluate variable array '{}' at compile time", lval.ident),
                    Some(Symbol::Pointer { .. }) => panic!("Cannot evaluate pointer '{}' as a constant", lval.ident),
                    Some(Symbol::Variable(_alloc)) => panic!("Cannot evaluate variable '{}' as a constant", lval.ident),
                    Some(Symbol::Function { .. }) => panic!("Cannot evaluate a function '{}' as a constant", lval.ident), // 函数调用不应该出现在这里
                    None => panic!("Undefined variable: {}", lval.ident), // 未定义，报错
                }
            },
            
            // 如果是数字，直接返回值
            PrimaryExp::Number(value) => *value,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Lval {
    pub ident: String,
    pub indices: Option<Vec<Exp>>, // 多维数组的索引
}
  
// 更新 UnaryExp，支持函数调用
#[derive(Clone, Debug)]
pub enum UnaryExp {
    Primary(PrimaryExp),
    Unary(UnaryOp, Box<UnaryExp>),
    Call {
        ident: String,     // 函数名
        args: Vec<Exp>,    // 实参
    },
}

impl UnaryExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            UnaryExp::Primary(exp) => exp.eval(symbol_table,global_symbol_table),
            UnaryExp::Unary(op, exp) => {
                let operand = exp.eval(symbol_table,global_symbol_table);
                match op {
                    UnaryOp::Neg => -operand,
                    UnaryOp::LNot => (operand == 0) as i32,
                }
            }
            UnaryExp::Call { .. } => {
                panic!("Function call evaluation not supported in this stage");
            }
        }
    }
}

#[derive(Clone, Debug)]
pub enum MulExp {
    Unary(UnaryExp),
    MulUnary(Box<MulExp>, MulOp, UnaryExp),
}

impl MulExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            MulExp::Unary(exp) => exp.eval(symbol_table,global_symbol_table),
            MulExp::MulUnary(lhs, op, rhs) => {
                let lhs_val = lhs.eval(symbol_table,global_symbol_table);
                let rhs_val = rhs.eval(symbol_table,global_symbol_table);
                match op {
                    MulOp::Mul => lhs_val * rhs_val,
                    MulOp::Div => lhs_val / rhs_val,
                    MulOp::Mod => lhs_val % rhs_val,
                }
            }
        }
    }
}
  
#[derive(Clone, Debug)]
pub enum AddExp {
    Mul(MulExp),
    AddMul(Box<AddExp>, AddOp, MulExp),
}

impl AddExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            AddExp::Mul(exp) => exp.eval(symbol_table,global_symbol_table),
            AddExp::AddMul(lhs, op, rhs) => {
                let lhs_val = lhs.eval(symbol_table,global_symbol_table);
                let rhs_val = rhs.eval(symbol_table,global_symbol_table);
                match op {
                    AddOp::Add => lhs_val + rhs_val,
                    AddOp::Sub => lhs_val - rhs_val,
                }
            }
        }
    }
}

#[derive(Clone, Debug)]
pub enum RelExp {
    Add(AddExp),
    RelAdd(Box<RelExp>, RelOp, AddExp),
}

impl RelExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            RelExp::Add(exp) => exp.eval(symbol_table,global_symbol_table),
            RelExp::RelAdd(lhs, op, rhs) => {
                let lhs_val = lhs.eval(symbol_table,global_symbol_table);
                let rhs_val = rhs.eval(symbol_table,global_symbol_table);
                match op {
                    RelOp::Lt => (lhs_val < rhs_val) as i32,
                    RelOp::Gt => (lhs_val > rhs_val) as i32,
                    RelOp::Le => (lhs_val <= rhs_val) as i32,
                    RelOp::Ge => (lhs_val >= rhs_val) as i32,
                }
            }
        }
    }
}
  
#[derive(Clone, Debug)]
pub enum EqExp {
    Rel(RelExp),
    EqRel(Box<EqExp>, EqOp, RelExp),
}

impl EqExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            EqExp::Rel(exp) => exp.eval(symbol_table,global_symbol_table),
            EqExp::EqRel(lhs, op, rhs) => {
                let lhs_val = lhs.eval(symbol_table,global_symbol_table);
                let rhs_val = rhs.eval(symbol_table,global_symbol_table);
                match op {
                    EqOp::Eq => (lhs_val == rhs_val) as i32,
                    EqOp::Neq => (lhs_val != rhs_val) as i32,
                }
            }
        }
    }
}

  
#[derive(Clone, Debug)]
pub enum LAndExp {
    Eq(EqExp),
    LAndEq(Box<LAndExp>, EqExp),
}

impl LAndExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            LAndExp::Eq(exp) => exp.eval(symbol_table,global_symbol_table),
            LAndExp::LAndEq(lhs, rhs) => {
                let lhs_val = lhs.eval(symbol_table,global_symbol_table);
                let rhs_val = rhs.eval(symbol_table,global_symbol_table);
                (lhs_val != 0 && rhs_val != 0) as i32
            }
        }
    }
}

#[derive(Clone, Debug)]
pub enum LOrExp {
    LAnd(LAndExp),
    LOrLAnd(Box<LOrExp>, LAndExp),
}

impl LOrExp {
    pub fn eval(&self, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> i32 {
        match self {
            LOrExp::LAnd(exp) => exp.eval(symbol_table,global_symbol_table),
            LOrExp::LOrLAnd(lhs, rhs) => {
                let lhs_val = lhs.eval(symbol_table,global_symbol_table);
                let rhs_val = rhs.eval(symbol_table,global_symbol_table);
                (lhs_val != 0 || rhs_val != 0) as i32
            }
        }
    }
}

#[derive(Clone, Debug)]
pub enum UnaryOp {
    Neg,
    LNot,
}
  
#[derive(Clone, Debug)]
pub enum MulOp {
    Mul,
    Div,
    Mod,
}
  
#[derive(Clone, Debug)]
pub enum AddOp {
    Add,
    Sub,
}
  
#[derive(Clone, Debug)]
pub enum RelOp {
    Lt,
    Gt,
    Le,
    Ge,
}
  
#[derive(Clone, Debug)]
pub enum EqOp {
    Eq,
    Neq,
}

