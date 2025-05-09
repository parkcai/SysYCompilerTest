use expression::{ConstExp, Evaluate, Exp};
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use variable::{BType, Decl, LVal};

pub mod expression;
pub mod operator;
pub mod variable;

#[derive(Clone)]
pub struct KoopaFunctionCounter {
    variable_counter: u32,
    or_counter: u32,
    and_counter: u32,
    if_counter: u32,
    while_counter: u32,
    while_stack: Vec<(String, String, String, u32)>,
    symbol_counter: u32,
}

impl KoopaFunctionCounter {
    pub fn new() -> Self {
        KoopaFunctionCounter {
            variable_counter: 0,
            or_counter: 0,
            and_counter: 0,
            if_counter: 0,
            while_counter: 0,
            while_stack: Vec::new(),
            symbol_counter: 0,
        }
    }

    pub fn from(other: &KoopaFunctionCounter) -> Self {
        KoopaFunctionCounter { ..other.clone() }
    }

    pub fn request_variable(&mut self) -> String {
        let res = self.variable_counter;
        self.variable_counter = self.variable_counter + 1;
        format!("%{}", res.to_string())
    }
    pub fn request_or_count(&mut self) -> (String, String, String, String) {
        let res = self.or_counter;
        self.or_counter = res + 1;
        (
            format!("@r{res}_or"),
            format!("%or_then_{res}"),
            format!("%or_else_{res}"),
            format!("%or_end_{res}"),
        )
    }
    pub fn request_and_count(&mut self) -> (String, String, String, String) {
        let res = self.and_counter;
        self.and_counter = res + 1;
        (
            format!("@r{res}_and"),
            format!("%and_then_{res}"),
            format!("%and_else_{res}"),
            format!("%and_end_{res}"),
        )
    }
    pub fn request_if_count(&mut self) -> (String, String, String) {
        let res = self.if_counter;
        self.if_counter = res + 1;
        (
            format!("%if_then_{res}"),
            format!("%if_else_{res}"),
            format!("%if_end_{res}"),
        )
    }
    pub fn request_while_count(&mut self) -> (String, String, String) {
        let res = self.while_counter;
        self.while_counter = res + 1;
        let entry = format!("%while_entry_{res}");
        let body = format!("%while_body_{res}");
        let end = format!("%while_end_{res}");
        self.while_stack
            .push((entry.clone(), body.clone(), end.clone(), 0));
        (entry, body, end)
    }
    pub fn request_while_message(&mut self) -> Option<&(String, String, String, u32)> {
        let res = self.while_stack.last_mut();
        match res {
            Some((_, _, _, count)) => {
                *count = *count + 1;
            }
            None => (),
        }
        self.while_stack.last()
    }
    pub fn request_while_leave(&mut self) {
        self.while_stack.pop();
    }
    pub fn request_symbol(&mut self) -> u32 {
        let res = self.symbol_counter;
        self.symbol_counter = res + 1;
        res
    }
}

#[derive(Clone)]
pub struct KoopaFunctionData {
    symbol_table: HashMap<String, TypedValue>,
    static_chain_pointer: Option<Rc<RefCell<KoopaFunctionData>>>,
    pub counter: KoopaFunctionCounter,
}

impl KoopaFunctionData {
    pub fn new(
        symbol_table: HashMap<String, TypedValue>,
        static_chain_pointer: Option<Rc<RefCell<KoopaFunctionData>>>,
        counter: &KoopaFunctionCounter,
    ) -> KoopaFunctionData {
        KoopaFunctionData {
            symbol_table: symbol_table.clone(),
            static_chain_pointer,
            counter: KoopaFunctionCounter::from(counter),
        }
    }

    pub fn get_symbol(&self, symbol: &String) -> Option<TypedValue> {
        let self_res = self.symbol_table.get(symbol);
        match self_res {
            Some(tv) => Some(tv.clone()),
            None => match &self.static_chain_pointer {
                Some(stmt) => stmt.borrow().get_symbol(symbol),
                None => None,
            },
        }
    }
    pub fn is_global(&self) -> bool {
        if let None = self.static_chain_pointer {
            true
        } else {
            false
        }
    }

    pub fn insert_symbol(&mut self, symbol: String, ty: TypedValue) -> Option<TypedValue> {
        self.symbol_table.insert(symbol, ty)
    }
}

#[derive(Clone)]
pub enum TypedValue {
    Int(i32),
    MutInt(String),
    Array(String, u32),
    Pointer(String, u32),
    IntFunc(String),
    VoidFunc(String),
}

#[derive(Debug)]
pub enum CompUnit {
    FuncDef(FuncDef, Option<Box<CompUnit>>),
    Decl(Decl, Option<Box<CompUnit>>),
}

#[derive(Debug)]
pub struct FuncDef {
    pub func_type: FuncType,
    pub identity: String,
    pub params: Vec<FuncFParam>,
    pub block: Block,
}

#[derive(Debug)]
pub enum FuncType {
    Int,
    Void,
}

#[derive(Debug)]
pub struct FuncFParam {
    pub param_type: BType,
    pub identity: String,
    pub index: Option<Vec<ConstExp>>,
}

#[derive(Debug)]
pub enum Block {
    Items(Vec<BlockItem>),
}

#[derive(Debug)]
pub enum BlockItem {
    Decl(Decl),
    Stmt(Stmt),
}

#[derive(Debug)]
pub enum Stmt {
    Return(Option<Exp>),
    Exp(Option<Exp>),
    Block(Block),
    Assign(LVal, Exp),
    If(Exp, Box<Stmt>, Option<Box<Stmt>>),
    While(Exp, Box<Stmt>),
    Break,
    Continue,
}

pub trait ToKoopa {
    type RetTuple;
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str>;
}

impl ToKoopa for FuncDef {
    type RetTuple = String;
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let sub_koopa_function = Rc::new(RefCell::new(KoopaFunctionData::new(
            HashMap::new(),
            Some(Rc::clone(&koopa_function)),
            &KoopaFunctionCounter::new(),
        )));
        let mut param_list = String::new();
        let mut prologue = String::new();
        for param in &self.params {
            let symbol_id = koopa_function.borrow_mut().counter.request_symbol();
            let dest = format!("{}_{symbol_id}", param.identity.clone());
            match &param.index {
                None => {
                    param_list.push_str(&format!("@{}: i32, ", dest.clone()));
                    prologue.push_str(&format!(
                        "\t%{} = alloc i32\n\tstore @{}, %{}\n",
                        dest.clone(),
                        dest.clone(),
                        dest.clone()
                    ));
                    match sub_koopa_function.borrow_mut().insert_symbol(
                        param.identity.clone(),
                        TypedValue::MutInt(format!("%{dest}")),
                    ) {
                        None => (),
                        Some(_) => return Err("Redefined Error: Function param redefine"),
                    }
                }
                Some(sizes) => {
                    let mut ty = String::from("i32");
                    let dim = sizes.len() + 1;
                    let mut sizes: Vec<_> = sizes
                        .iter()
                        .map(|x| x.eval(Rc::clone(&koopa_function)))
                        .collect();
                    sizes.reverse();
                    for size in sizes {
                        ty = format!("[{ty}, {}]", size);
                    }
                    param_list.push_str(&format!("@{}: *{}, ", dest.clone(), ty.clone()));
                    prologue.push_str(&format!(
                        "\t%{} = alloc *{}\n\tstore @{}, %{}\n",
                        dest.clone(),
                        ty.clone(),
                        dest.clone(),
                        dest.clone()
                    ));
                    match sub_koopa_function.borrow_mut().insert_symbol(
                        param.identity.clone(),
                        TypedValue::Pointer(format!("%{dest}"), dim as u32),
                    ) {
                        None => (),
                        Some(_) => return Err("Redefined Error: Function param redefine"),
                    }
                }
            }
        }
        if !param_list.is_empty() {
            param_list.pop();
            param_list.pop();
        }
        match self.func_type {
            FuncType::Int => match koopa_function.borrow_mut().insert_symbol(
                self.identity.clone(),
                TypedValue::IntFunc(format!("@{}", self.identity.clone())),
            ) {
                None => (),
                Some(_) => return Err("Redefined Error: Redefine function."),
            },
            FuncType::Void => match koopa_function.borrow_mut().insert_symbol(
                self.identity.clone(),
                TypedValue::VoidFunc(format!("@{}", self.identity.clone())),
            ) {
                None => (),
                Some(_) => return Err("Redefined Error: Redefine function."),
            },
        }
        let func_type = self.func_type.to_koopa(Rc::clone(&koopa_function))?;
        let (block_kp, block_term) = self.block.to_koopa(Rc::clone(&sub_koopa_function))?;
        match block_term {
            true => Ok(format!(
                "fun @{}({param_list}){func_type} {{\n%entry:\n{prologue}{block_kp}}}\n",
                self.identity
            )),
            false => match self.func_type {
                FuncType::Int => Ok(format!(
                    "fun @{}({param_list}){func_type} {{\n%entry:\n{prologue}{block_kp}\tret 0\n}}\n",
                    self.identity
                )),
                FuncType::Void => Ok(format!(
                    "fun @{}({param_list}){func_type} {{\n%entry:\n{prologue}{block_kp}\tret\n}}\n",
                    self.identity
                )),
            },
        }
    }
}

impl ToKoopa for FuncType {
    type RetTuple = String;
    fn to_koopa(
        &self,
        _koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            FuncType::Int => Ok(String::from(": i32")),
            FuncType::Void => Ok(String::new()),
        }
    }
}

impl ToKoopa for Block {
    type RetTuple = (String, bool);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let Block::Items(items) = self;
        let mut res = String::new();
        let mut term = false;
        for item in items {
            if term {
                break;
            }
            let (kp, terminate) = item.to_koopa(Rc::clone(&koopa_function))?;
            res.push_str(&kp);
            term = terminate;
        }
        Ok((res, term))
    }
}

impl ToKoopa for BlockItem {
    type RetTuple = (String, bool);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            BlockItem::Decl(decl) => Ok((decl.to_koopa(koopa_function)?, false)),
            BlockItem::Stmt(stmt) => stmt.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for Stmt {
    type RetTuple = (String, bool);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            Stmt::Break => match koopa_function.borrow_mut().counter.request_while_message() {
                Some((_entry, _body, end, _id)) => Ok((format!("\tjump {}\n", end.clone()), true)),
                None => Err("Break without while."),
            },
            Stmt::Continue => match koopa_function.borrow_mut().counter.request_while_message() {
                Some((entry, _body, _end, _id)) => {
                    Ok((format!("\tjump {}\n", entry.clone()), true))
                }
                None => Err("Continue without while."),
            },
            Stmt::While(condition, loop_body) => {
                let (bb_entry, bb_body, bb_end) =
                    koopa_function.borrow_mut().counter.request_while_count();
                let (ckp, cret) = condition.to_koopa(Rc::clone(&koopa_function))?;
                let (lkp, lterm) = loop_body.to_koopa(Rc::clone(&koopa_function))?;
                let loop_body = match lterm {
                    false => format!("{}:\n{}\tjump {}\n", bb_body, lkp, bb_entry),
                    true => format!("{}:\n{}", bb_body, lkp),
                };
                koopa_function.borrow_mut().counter.request_while_leave();
                Ok((
                    format!(
                        "\tjump {}\n{}:\n{}\tbr {}, {}, {}\n{}{}:\n",
                        bb_entry, bb_entry, ckp, cret, bb_body, bb_end, loop_body, bb_end
                    ),
                    false,
                ))
            }
            Stmt::If(condition, true_branch, false_branch) => {
                let (bb_then, bb_else, bb_end) =
                    koopa_function.borrow_mut().counter.request_if_count();
                let (ckp, cret) = condition.to_koopa(Rc::clone(&koopa_function))?;
                let (tkp, tterm) = true_branch.to_koopa(Rc::clone(&koopa_function))?;
                let true_branch = match tterm {
                    false => format!("{}:\n{}\tjump {}\n", bb_then, tkp, bb_end),
                    true => format!("{}:\n{}", bb_then, tkp),
                };
                match false_branch {
                    Some(false_branch) => {
                        let (fkp, fterm) = false_branch.to_koopa(Rc::clone(&koopa_function))?;
                        let false_branch = match fterm {
                            false => format!("{}:\n{}\tjump {}\n", bb_else, fkp, bb_end),
                            true => format!("{}:\n{}", bb_else, fkp),
                        };
                        let term = tterm && fterm;
                        let end = match term {
                            false => format!("{}:\n", bb_end),
                            true => String::new(),
                        };
                        Ok((
                            format!(
                                "{}\tbr {}, {}, {}\n{}{}{}",
                                ckp, cret, bb_then, bb_else, true_branch, false_branch, end
                            ),
                            term,
                        ))
                    }
                    None => Ok((
                        format!(
                            "{}\tbr {}, {}, {}\n{}{}:\n",
                            ckp, cret, bb_then, bb_end, true_branch, bb_end
                        ),
                        false,
                    )),
                }
            }
            Stmt::Exp(exp) => match exp {
                Some(exp) => Ok((exp.to_koopa(koopa_function)?.0, false)),
                None => Ok((String::new(), false)),
            },
            Stmt::Block(block) => {
                let sub_koopa_function = Rc::new(RefCell::new(KoopaFunctionData::new(
                    HashMap::new(),
                    Some(Rc::clone(&koopa_function)),
                    &koopa_function.borrow().counter,
                )));
                let res = block.to_koopa(Rc::clone(&sub_koopa_function));
                koopa_function.borrow_mut().counter =
                    KoopaFunctionCounter::from(&sub_koopa_function.borrow().counter);
                res
            }
            Stmt::Return(exp) => {
                let (kp, ret) = match exp {
                    Some(exp) => exp.to_koopa(koopa_function)?,
                    None => (String::new(), String::new()),
                };
                Ok((format!("{}\tret {}\n", kp, ret), true))
            }
            Stmt::Assign(lval, exp) => {
                let LVal::Ident(id, indexs) = lval;
                let v = koopa_function.borrow().get_symbol(id).unwrap();
                match v {
                    TypedValue::MutInt(id) => {
                        let dest = id.clone();
                        let (kp, ret) = exp.to_koopa(koopa_function)?;
                        Ok((format!("{}\tstore {}, {}\n", kp, ret, dest.clone()), false))
                    }
                    TypedValue::Array(id, _) => {
                        let mut dest = id.clone();
                        let (ekp, eret) = exp.to_koopa(Rc::clone(&koopa_function))?;
                        let mut res = ekp;
                        for index in indexs {
                            let (ikp, iret) = index.to_koopa(Rc::clone(&koopa_function))?;
                            let idest = koopa_function.borrow_mut().counter.request_variable();
                            res.push_str(&format!(
                                "{ikp}\t{} = getelemptr {dest}, {iret}\n",
                                idest.clone()
                            ));
                            dest = idest;
                        }
                        res.push_str(&format!("\tstore {eret}, {dest}\n"));
                        Ok((res, false))
                    }
                    TypedValue::Pointer(id, _) => {
                        let mut dest = koopa_function.borrow_mut().counter.request_variable();
                        let mut res = format!("\t{} = load {id}\n", dest.clone());
                        let (ekp, eret) = exp.to_koopa(Rc::clone(&koopa_function))?;
                        res.push_str(&ekp);
                        for (i, index) in indexs.iter().enumerate() {
                            let (ikp, iret) = index.to_koopa(Rc::clone(&koopa_function))?;
                            let idest = koopa_function.borrow_mut().counter.request_variable();
                            if i == 0 {
                                res.push_str(&format!(
                                    "{ikp}\t{} = getptr {dest}, {iret}\n",
                                    idest.clone()
                                ));
                            } else {
                                res.push_str(&format!(
                                    "{ikp}\t{} = getelemptr {dest}, {iret}\n",
                                    idest.clone()
                                ));
                            }
                            dest = idest;
                        }
                        res.push_str(&format!("\tstore {eret}, {dest}\n"));
                        Ok((res, false))
                    }
                    _ => return Err("Try to assign an immutable variable."),
                }
            }
        }
    }
}

impl ToKoopa for CompUnit {
    type RetTuple = String;
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            CompUnit::FuncDef(func_def, next) => {
                let mut res = match next {
                    Some(comp) => comp.to_koopa(Rc::clone(&koopa_function))?,
                    None => String::from("decl @getint(): i32\ndecl @getch(): i32\ndecl @getarray(*i32): i32\ndecl @putint(i32)\ndecl @putch(i32)\ndecl @putarray(i32, *i32)\ndecl @starttime()\ndecl @stoptime()\n\n"),
                };
                res.push_str(&func_def.to_koopa(koopa_function)?);
                Ok(res)
            }
            CompUnit::Decl(decl, next) => {
                let mut res = match next {
                    Some(comp) => comp.to_koopa(Rc::clone(&koopa_function))?,
                    None => String::from("decl @getint(): i32\ndecl @getch(): i32\ndecl @getarray(*i32): i32\ndecl @putint(i32)\ndecl @putch(i32)\ndecl @putarray(i32, *i32)\ndecl @starttime()\ndecl @stoptime()\n\n"),
                };
                res.push_str(&decl.to_koopa(koopa_function)?);
                Ok(res)
            }
        }
    }
}
