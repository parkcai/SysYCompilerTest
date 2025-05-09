use super::{
    expression::{ConstExp, Evaluate, Exp},
    KoopaFunctionData, ToKoopa, TypedValue,
};
use std::cell::RefCell;
use std::rc::Rc;

#[derive(Debug)]
pub enum Decl {
    ConstDecl(ConstDecl),
    VarDecl(VarDecl),
}

#[derive(Debug)]
pub enum ConstDecl {
    ConstDecl(BType, Vec<ConstDef>),
}

#[derive(Debug)]
pub enum VarDecl {
    VarDecl(BType, Vec<VarDef>),
}

#[derive(Debug)]
pub enum ConstDef {
    ConstDef(String, Vec<ConstExp>, ConstInitVal),
}

#[derive(Debug)]
pub enum VarDef {
    VarDef(String, Vec<ConstExp>),
    VarDefInit(String, Vec<ConstExp>, InitVal),
}

#[derive(Debug, Clone)]
pub enum InitVal {
    Exp(Exp),
    List(Vec<InitVal>),
}

impl InitVal {
    fn standardize(
        arr: String,
        vec: &mut Vec<InitVal>,
        sizes: Vec<u32>,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> String {
        let dim = sizes.len() as u32;
        let mut res = String::new();
        let size = sizes.first().unwrap().clone() as usize;
        println!("{dim}");
        match dim {
            0 => unreachable!(),
            1 => {
                let vec_len = vec.len();
                let vals: Vec<_> = if vec_len >= size {
                    vec.splice(..size, []).collect()
                } else {
                    vec.splice(..vec_len, []).collect()
                };
                for index in 0..size {
                    match vals.get(index) {
                        Some(val) => match val {
                            InitVal::Exp(exp) => {
                                let (ekp, eret) = exp.to_koopa(Rc::clone(&koopa_function)).unwrap();
                                let v = koopa_function.borrow_mut().counter.request_variable();
                                res.push_str(&format!(
                                    "{ekp}\t{} = getelemptr {}, {index}\n\tstore {eret}, {}\n",
                                    v.clone(),
                                    arr.clone(),
                                    v.clone()
                                ));
                            }
                            InitVal::List(_) => unreachable!(),
                        },
                        None => {
                            let v = koopa_function.borrow_mut().counter.request_variable();
                            res.push_str(&format!(
                                "\t{} = getelemptr {}, {index}\n\tstore 0, {}\n",
                                v.clone(),
                                arr.clone(),
                                v.clone()
                            ));
                        }
                    }
                }
                res
            }
            _ => {
                let mut sizes = sizes.clone();
                sizes.splice(0..1, []);
                for index in 0..size {
                    let v = koopa_function.borrow_mut().counter.request_variable();
                    let next = vec.first_mut();
                    let elem = match next {
                        Some(next) => match next {
                            InitVal::Exp(_) => InitVal::standardize(
                                v.clone(),
                                vec,
                                sizes.clone(),
                                Rc::clone(&koopa_function),
                            ),
                            InitVal::List(list) => {
                                let elem = InitVal::standardize(
                                    v.clone(),
                                    list,
                                    sizes.clone(),
                                    Rc::clone(&koopa_function),
                                );
                                vec.splice(0..1, []);
                                elem
                            }
                        },
                        None => InitVal::standardize(
                            v.clone(),
                            vec,
                            sizes.clone(),
                            Rc::clone(&koopa_function),
                        ),
                    };
                    res.push_str(&format!(
                        "\t{} = getelemptr {}, {index}\n{elem}",
                        v.clone(),
                        arr.clone(),
                    ));
                }
                res
            }
        }
    }

    fn global_standardize(
        vec: &mut Vec<InitVal>,
        sizes: Vec<u32>,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> String {
        let dim = sizes.len() as u32;
        let mut res = String::from("{");
        let size = sizes.first().unwrap().clone() as usize;
        match dim {
            0 => unreachable!(),
            1 => {
                let vec_len = vec.len();
                let vals: Vec<_> = if vec_len >= size {
                    vec.splice(..size, []).collect()
                } else {
                    vec.splice(..vec_len, []).collect()
                };
                for index in 0..size {
                    match vals.get(index) {
                        Some(val) => match val {
                            InitVal::Exp(exp) => {
                                if index != size - 1 {
                                    res.push_str(&format!(
                                        "{}, ",
                                        exp.eval(Rc::clone(&koopa_function))
                                    ));
                                } else {
                                    res.push_str(&format!(
                                        "{}}}",
                                        exp.eval(Rc::clone(&koopa_function))
                                    ));
                                }
                            }
                            InitVal::List(_) => unreachable!(),
                        },
                        None => {
                            if index != size - 1 {
                                res.push_str("0, ");
                            } else {
                                res.push_str("0}");
                            }
                        }
                    }
                }
                res
            }
            _ => {
                let mut sizes = sizes.clone();
                sizes.splice(0..1, []);
                for index in 0..size {
                    let next = vec.first_mut();
                    let elem = match next {
                        Some(next) => match next {
                            InitVal::Exp(_) => InitVal::global_standardize(
                                vec,
                                sizes.clone(),
                                Rc::clone(&koopa_function),
                            ),
                            InitVal::List(list) => {
                                let elem = InitVal::global_standardize(
                                    list,
                                    sizes.clone(),
                                    Rc::clone(&koopa_function),
                                );
                                vec.splice(0..1, []);
                                elem
                            }
                        },
                        None => InitVal::global_standardize(
                            vec,
                            sizes.clone(),
                            Rc::clone(&koopa_function),
                        ),
                    };
                    if index != size - 1 {
                        res.push_str(&format!("{elem}, "));
                    } else {
                        res.push_str(&format!("{elem}}}"));
                    }
                }
                res
            }
        }
    }
}

#[derive(Debug, Clone)]
pub enum ConstInitVal {
    ConstExp(ConstExp),
    ConstList(Vec<ConstInitVal>),
}

impl ConstInitVal {
    fn standardize(
        arr: String,
        vec: &mut Vec<ConstInitVal>,
        sizes: Vec<u32>,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> String {
        let dim = sizes.len() as u32;
        let mut res = String::new();
        let size = sizes.first().unwrap().clone() as usize;
        println!("{dim}");
        match dim {
            0 => unreachable!(),
            1 => {
                let vec_len = vec.len();
                let vals: Vec<_> = if vec_len >= size {
                    vec.splice(..size, []).collect()
                } else {
                    vec.splice(..vec_len, []).collect()
                };
                for index in 0..size {
                    match vals.get(index) {
                        Some(val) => match val {
                            ConstInitVal::ConstExp(exp) => {
                                let v = koopa_function.borrow_mut().counter.request_variable();
                                res.push_str(&format!(
                                    "\t{} = getelemptr {}, {index}\n\tstore {}, {}\n",
                                    v.clone(),
                                    arr.clone(),
                                    exp.eval(Rc::clone(&koopa_function)),
                                    v.clone()
                                ));
                            }
                            ConstInitVal::ConstList(_) => unreachable!(),
                        },
                        None => {
                            let v = koopa_function.borrow_mut().counter.request_variable();
                            res.push_str(&format!(
                                "\t{} = getelemptr {}, {index}\n\tstore 0, {}\n",
                                v.clone(),
                                arr.clone(),
                                v.clone()
                            ));
                        }
                    }
                }
                res
            }
            _ => {
                let mut sizes = sizes.clone();
                sizes.splice(0..1, []);
                for index in 0..size {
                    let v = koopa_function.borrow_mut().counter.request_variable();
                    let next = vec.first_mut();
                    let elem = match next {
                        Some(next) => match next {
                            ConstInitVal::ConstExp(_) => ConstInitVal::standardize(
                                v.clone(),
                                vec,
                                sizes.clone(),
                                Rc::clone(&koopa_function),
                            ),
                            ConstInitVal::ConstList(list) => {
                                let elem = ConstInitVal::standardize(
                                    v.clone(),
                                    list,
                                    sizes.clone(),
                                    Rc::clone(&koopa_function),
                                );
                                vec.splice(0..1, []);
                                elem
                            }
                        },
                        None => ConstInitVal::standardize(
                            v.clone(),
                            vec,
                            sizes.clone(),
                            Rc::clone(&koopa_function),
                        ),
                    };
                    res.push_str(&format!(
                        "\t{} = getelemptr {}, {index}\n{elem}",
                        v.clone(),
                        arr.clone(),
                    ));
                }
                res
            }
        }
    }
    fn global_standardize(
        vec: &mut Vec<ConstInitVal>,
        sizes: Vec<u32>,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> String {
        let dim = sizes.len() as u32;
        let mut res = String::from("{");
        let size = sizes.first().unwrap().clone() as usize;
        match dim {
            0 => unreachable!(),
            1 => {
                let vec_len = vec.len();
                let vals: Vec<_> = if vec_len >= size {
                    vec.splice(..size, []).collect()
                } else {
                    vec.splice(..vec_len, []).collect()
                };
                for index in 0..size {
                    match vals.get(index) {
                        Some(val) => match val {
                            ConstInitVal::ConstExp(exp) => {
                                if index != size - 1 {
                                    res.push_str(&format!(
                                        "{}, ",
                                        exp.eval(Rc::clone(&koopa_function))
                                    ));
                                } else {
                                    res.push_str(&format!(
                                        "{}}}",
                                        exp.eval(Rc::clone(&koopa_function))
                                    ));
                                }
                            }
                            ConstInitVal::ConstList(_) => unreachable!(),
                        },
                        None => {
                            if index != size - 1 {
                                res.push_str("0, ");
                            } else {
                                res.push_str("0}");
                            }
                        }
                    }
                }
                res
            }
            _ => {
                let mut sizes = sizes.clone();
                sizes.splice(0..1, []);
                for index in 0..size {
                    let next = vec.first_mut();
                    let elem = match next {
                        Some(next) => match next {
                            ConstInitVal::ConstExp(_) => ConstInitVal::global_standardize(
                                vec,
                                sizes.clone(),
                                Rc::clone(&koopa_function),
                            ),
                            ConstInitVal::ConstList(list) => {
                                let elem = ConstInitVal::global_standardize(
                                    list,
                                    sizes.clone(),
                                    Rc::clone(&koopa_function),
                                );
                                vec.splice(0..1, []);
                                elem
                            }
                        },
                        None => ConstInitVal::global_standardize(
                            vec,
                            sizes.clone(),
                            Rc::clone(&koopa_function),
                        ),
                    };
                    if index != size - 1 {
                        res.push_str(&format!("{elem}, "));
                    } else {
                        res.push_str(&format!("{elem}}}"));
                    }
                }
                res
            }
        }
    }
}

#[derive(Debug)]
pub enum BType {
    Int,
}

#[derive(Debug, Clone)]
pub enum LVal {
    Ident(String, Vec<Exp>),
}

impl ToKoopa for Decl {
    type RetTuple = String;
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        match self {
            Decl::ConstDecl(cd) => cd.to_koopa(koopa_function),
            Decl::VarDecl(vd) => vd.to_koopa(koopa_function),
        }
    }
}

impl ToKoopa for ConstDecl {
    type RetTuple = String;
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let ConstDecl::ConstDecl(_, defs) = self;
        let mut res = String::new();
        for def in defs {
            let ConstDef::ConstDef(id, sizes, val) = def;
            match val {
                ConstInitVal::ConstExp(exp) => {
                    let exp_val = exp.eval(Rc::clone(&koopa_function));
                    let insert_res = koopa_function
                        .borrow_mut()
                        .insert_symbol(id.clone(), TypedValue::Int(exp_val));
                    match insert_res {
                        Some(_) => panic!("Redefined Error: {id}"),
                        None => (),
                    }
                }
                ConstInitVal::ConstList(vals) => {
                    let sizes: Vec<_> = sizes
                        .iter()
                        .map(|e| e.eval(Rc::clone(&koopa_function)) as u32)
                        .collect();
                    let mut ty = String::from("i32");
                    {
                        let mut sizes = sizes.clone();
                        sizes.reverse();
                        for size in sizes {
                            ty = format!("[{ty}, {size}]");
                        }
                    }
                    if koopa_function.borrow().is_global() {
                        let mut vals = vals.clone();
                        let aggregate = ConstInitVal::global_standardize(
                            &mut vals,
                            sizes.clone(),
                            Rc::clone(&koopa_function),
                        );
                        let dest = format!("@{}_globl", id.clone());
                        koopa_function.borrow_mut().insert_symbol(
                            id.clone(),
                            TypedValue::Array(dest.clone(), sizes.len() as u32),
                        );
                        res.push_str(&format!("global {dest} = alloc {ty}, {aggregate}\n"));
                    } else {
                        let symbol_id = koopa_function.borrow_mut().counter.request_symbol();
                        let dest = format!("@{}_{symbol_id}", id.clone());
                        koopa_function.borrow_mut().insert_symbol(
                            id.clone(),
                            TypedValue::Array(dest.clone(), sizes.len() as u32),
                        );
                        res.push_str(&format!("\t{} = alloc {ty}\n", dest.clone()));
                        let mut vals = vals.clone();
                        res.push_str(&ConstInitVal::standardize(
                            dest,
                            &mut vals,
                            sizes,
                            Rc::clone(&koopa_function),
                        ));
                    }
                }
            }
        }
        Ok(res)
    }
}

impl ToKoopa for VarDecl {
    type RetTuple = String;
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let VarDecl::VarDecl(_, defs) = self;
        let mut res = String::new();
        for def in defs {
            match def {
                VarDef::VarDef(id, sizes) => {
                    let var_type = if sizes.is_empty() {
                        String::from("i32")
                    } else {
                        let sizes: Vec<_> = sizes
                            .iter()
                            .map(|e| e.eval(Rc::clone(&koopa_function)) as u32)
                            .collect();
                        let mut ty = String::from("i32");
                        {
                            let mut sizes = sizes.clone();
                            sizes.reverse();
                            for size in sizes {
                                ty = format!("[{ty}, {size}]");
                            }
                        }
                        ty
                    };
                    if koopa_function.borrow().is_global() {
                        let dest = format!("@{}_globl", id.clone());
                        let symbol = if sizes.is_empty() {
                            TypedValue::MutInt(dest.clone())
                        } else {
                            TypedValue::Array(dest.clone(), sizes.len() as u32)
                        };
                        let insert_res = koopa_function
                            .borrow_mut()
                            .insert_symbol(id.clone(), symbol);
                        match insert_res {
                            Some(_) => panic!("Redefined Error: {id}"),
                            None => {
                                res.push_str(&format!(
                                    "global {} = alloc {var_type}, zeroinit\n",
                                    dest.clone()
                                ));
                            }
                        }
                    } else {
                        let symbol_id = koopa_function.borrow_mut().counter.request_symbol();
                        let dest = format!("@{}_{symbol_id}", id.clone());
                        let symbol = if sizes.is_empty() {
                            TypedValue::MutInt(dest.clone())
                        } else {
                            TypedValue::Array(dest.clone(), sizes.len() as u32)
                        };
                        let insert_res = koopa_function
                            .borrow_mut()
                            .insert_symbol(id.clone(), symbol);
                        match insert_res {
                            Some(_) => panic!("Redefined Error: {id}"),
                            None => {
                                res.push_str(&format!("\t{} = alloc {var_type}\n", dest.clone()))
                            }
                        }
                    }
                }
                VarDef::VarDefInit(id, sizes, val) => match val {
                    InitVal::Exp(val) => {
                        if koopa_function.borrow().is_global() {
                            let dest = format!("@{}_globl", id.clone());
                            let insert_res = koopa_function
                                .borrow_mut()
                                .insert_symbol(id.clone(), TypedValue::MutInt(dest.clone()));
                            match insert_res {
                                Some(_) => panic!("Redefined Error: {id}"),
                                None => {
                                    let val = val.eval(Rc::clone(&koopa_function));
                                    res.push_str(&format!("global {dest} = alloc i32, {val}\n"));
                                }
                            }
                        } else {
                            let symbol_id = koopa_function.borrow_mut().counter.request_symbol();
                            let dest = format!("@{}_{symbol_id}", id.clone());
                            let insert_res = koopa_function
                                .borrow_mut()
                                .insert_symbol(id.clone(), TypedValue::MutInt(dest.clone()));
                            match insert_res {
                                Some(_) => panic!("Redefined Error: {id}"),
                                None => {
                                    let (kp, ret) = val.to_koopa(Rc::clone(&koopa_function))?;
                                    res.push_str(&format!(
                                        "{}\t{} = alloc i32\n\tstore {}, {}\n",
                                        kp,
                                        dest.clone(),
                                        ret,
                                        dest.clone()
                                    ));
                                }
                            }
                        }
                    }
                    InitVal::List(vals) => {
                        let sizes: Vec<_> = sizes
                            .iter()
                            .map(|e| e.eval(Rc::clone(&koopa_function)) as u32)
                            .collect();
                        let mut ty = String::from("i32");
                        {
                            let mut sizes = sizes.clone();
                            sizes.reverse();
                            for size in sizes {
                                ty = format!("[{ty}, {size}]");
                            }
                        }
                        if koopa_function.borrow().is_global() {
                            let mut vals = vals.clone();
                            let aggregate = InitVal::global_standardize(
                                &mut vals,
                                sizes.clone(),
                                Rc::clone(&koopa_function),
                            );
                            let dest = format!("@{}_globl", id.clone());
                            koopa_function.borrow_mut().insert_symbol(
                                id.clone(),
                                TypedValue::Array(dest.clone(), sizes.len() as u32),
                            );
                            res.push_str(&format!("global {dest} = alloc {ty}, {aggregate}\n"));
                        } else {
                            let symbol_id = koopa_function.borrow_mut().counter.request_symbol();
                            let dest = format!("@{}_{symbol_id}", id.clone());
                            koopa_function.borrow_mut().insert_symbol(
                                id.clone(),
                                TypedValue::Array(dest.clone(), sizes.len() as u32),
                            );
                            res.push_str(&format!("\t{} = alloc {ty}\n", dest.clone()));
                            let mut vals = vals.clone();
                            res.push_str(&InitVal::standardize(
                                dest,
                                &mut vals,
                                sizes,
                                Rc::clone(&koopa_function),
                            ));
                        }
                    }
                },
            }
        }
        Ok(res)
    }
}

impl ToKoopa for LVal {
    type RetTuple = (String, String);
    fn to_koopa(
        &self,
        koopa_function: Rc<RefCell<KoopaFunctionData>>,
    ) -> Result<Self::RetTuple, &'static str> {
        let LVal::Ident(id, indexs) = self;
        let val = koopa_function.borrow().get_symbol(id);
        match val {
            Some(v) => match v {
                TypedValue::Int(n) => Ok((String::new(), n.to_string())),
                TypedValue::MutInt(v) => {
                    let dest = koopa_function.borrow_mut().counter.request_variable();
                    Ok((format!("\t{} = load {v}\n", dest.clone()), dest))
                }
                TypedValue::Array(v, dim) => {
                    let mut res = String::new();
                    let mut dest = v;
                    let index_dim = indexs.len() as u32;
                    for index in indexs {
                        let (kp, ret) = index.to_koopa(Rc::clone(&koopa_function)).unwrap();
                        let idest = koopa_function.borrow_mut().counter.request_variable();
                        res.push_str(&format!(
                            "{kp}\t{} = getelemptr {dest}, {ret}\n",
                            idest.clone()
                        ));
                        dest = idest;
                    }
                    let ret = koopa_function.borrow_mut().counter.request_variable();
                    if index_dim == dim {
                        res.push_str(&format!("\t{} = load {dest}\n", ret.clone()));
                    } else {
                        res.push_str(&format!("\t{} = getelemptr {dest}, 0\n", ret.clone()));
                    }
                    Ok((res, ret))
                }
                TypedValue::Pointer(v, dim) => {
                    let mut dest = koopa_function.borrow_mut().counter.request_variable();
                    let mut res = format!("\t{dest} = load {v}\n");
                    let index_dim = indexs.len() as u32;
                    for (i, index) in indexs.iter().enumerate() {
                        let (kp, ret) = index.to_koopa(Rc::clone(&koopa_function)).unwrap();
                        let idest = koopa_function.borrow_mut().counter.request_variable();
                        if i == 0 {
                            res.push_str(&format!(
                                "{kp}\t{} = getptr {dest}, {ret}\n",
                                idest.clone()
                            ));
                        } else {
                            res.push_str(&format!(
                                "{kp}\t{} = getelemptr {dest}, {ret}\n",
                                idest.clone()
                            ));
                        }
                        dest = idest;
                    }
                    let mut ret = koopa_function.borrow_mut().counter.request_variable();
                    if index_dim == dim {
                        res.push_str(&format!("\t{} = load {dest}\n", ret.clone()));
                    } else if index_dim == 0 {
                        ret = dest;
                    } else {
                        res.push_str(&format!("\t{} = getelemptr {dest}, 0\n", ret.clone()));
                    }
                    Ok((res, ret))
                }
                _ => unreachable!(),
            },
            None => panic!("Undefined variable: {id}"),
        }
    }
}
