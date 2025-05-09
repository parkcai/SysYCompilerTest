pub mod builtin;
pub mod context;
pub mod eval;
pub mod initial_list;
pub mod scope;

use crate::front::ast::*;
use crate::front::ident::Identifier;
use crate::util::logger::show_error;
use crate::util::remove_pointer;
use crate::{add_bb, add_inst, new_value};
use context::Context;
use eval::Eval;
use initial_list::InitializeList;
use koopa::ir::builder::{GlobalInstBuilder, LocalInstBuilder, ValueBuilder};
use koopa::ir::{BinaryOp, FunctionData, Type, TypeKind, Value};
use scope::Scope;
use std::rc::Rc;

type Return = Option<Expr>;

fn get_type(value: Value, ctx: &Context) -> Result<Type, ParseError> {
    let ty = if ctx.func_data().is_ok() && ctx.func_data()?.dfg().values().contains_key(&value) {
        ctx.func_data()?.dfg().value(value).ty().clone()
    } else {
        ctx.program.borrow_value(value).ty().clone()
    };
    Ok(ty)
}

fn get_array_type<T: Eval>(shape: &[T], scope: &mut Scope) -> Type {
    let mut param_type = Type::get_i32();
    for i in shape.iter().rev() {
        let v = i.eval(scope).unwrap_or(0);
        if v <= 0 {
            show_error("Array size must be greater than 0", 2);
        }
        param_type = Type::get_array(param_type, v as usize);
    }
    param_type
}

fn get_array_pos(array_elem: &ArrayElem, ctx: &mut Context) -> Result<Value, ParseError> {
    let indices = array_elem
        .indices
        .iter()
        .map(|index| index.generate_ir(ctx))
        .collect::<Result<Vec<_>, _>>()?;
    let array = ctx
        .scope
        .get_identifier(&array_elem.name)
        .ok_or(ParseError::UnknownIdentifier)?
        .clone();
    let array = match array {
        Identifier::Variable(var) => var.koopa_def,
        Identifier::ConstArray(const_array) => const_array.koopa_def,
        _ => return Err(ParseError::InvalidExpr),
    };

    // Get offset
    let mut result = array;
    for index in &indices {
        let index = index.clone();
        let bb = ctx.get_bb()?;
        let result_type = get_type(result, ctx)?;
        let result_type = remove_pointer(result_type);
        let array_elem = if let TypeKind::Pointer(_) = result_type.kind() {
            let load = new_value!(ctx.func_data_mut()?).load(result);
            add_inst!(ctx.func_data_mut()?, bb, load);
            new_value!(ctx.func_data_mut()?).get_ptr(load, index)
        } else {
            new_value!(ctx.func_data_mut()?).get_elem_ptr(result, index)
        };
        add_inst!(ctx.func_data_mut()?, bb, array_elem);
        result = array_elem;
    }

    Ok(result)
}

#[derive(Debug)]
pub enum ParseError {
    InvalidExpr,
    FunctionNotFound,
    BasicBlockNotFound,
    UnknownIdentifier,
    ConstExprError,
    BreakOutsideLoop,
    ContinueOutsideLoop,
    MultipleDefinition,
}

pub trait GenerateIR {
    type Output;
    fn generate_ir(&self, ctx: &mut Context) -> Result<Self::Output, ParseError>;
}

impl GenerateIR for i32 {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        Ok(new_value!(ctx.func_data_mut()?).integer(*self))
    }
}

impl GenerateIR for ConstExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        let val = self
            .0
            .eval(&mut ctx.scope)
            .map_err(|_| ParseError::InvalidExpr)?;
        if let Ok(_) = ctx.get_func() {
            let func_data = ctx.func_data_mut()?;
            Ok(new_value!(func_data).integer(val))
        } else {
            Ok(ctx.program.new_value().integer(val))
        }
    }
}

impl GenerateIR for Expr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        self.0.generate_ir(ctx)
    }
}

impl GenerateIR for VarDef {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        let scope_id = ctx.scope.current_scope_id();
        match self {
            VarDef::NormalVarDef(normal_var_def) => {
                let var_name = format!("@_{}_{}", scope_id, normal_var_def.name);
                if !ctx.is_global() {
                    // local variable.
                    let bb = ctx.get_bb()?;
                    let func_data = ctx.func_data_mut()?;
                    // allocate variable
                    let var_alloc = new_value!(func_data).alloc(Type::get_i32());

                    func_data
                        .dfg_mut()
                        .set_value_name(var_alloc, Some(var_name));
                    add_inst!(func_data, bb, var_alloc);

                    if let Some(init) = &normal_var_def.value {
                        // store initial value
                        let init = init.generate_ir(ctx)?;
                        let store = new_value!(ctx.func_data_mut()?).store(init, var_alloc);
                        let current_bb = ctx.get_bb()?;
                        add_inst!(ctx.func_data_mut()?, current_bb, store);
                    }
                    // add identifier to scope
                    ctx.scope
                        .add_identifier(
                            normal_var_def.name.clone(),
                            Identifier::from_variable(var_alloc),
                        )
                        .map_err(|_| ParseError::MultipleDefinition)?;
                    Ok(var_alloc)
                } else {
                    // global variable
                    let val = normal_var_def
                        .value
                        .as_ref()
                        .map(|x| x.eval(&mut ctx.scope).unwrap_or(0))
                        .unwrap_or(0);
                    let val = ctx.program.new_value().integer(val);
                    let var_alloc = ctx.program.new_value().global_alloc(val);
                    ctx.program.set_value_name(var_alloc, Some(var_name));
                    ctx.scope
                        .add_identifier(
                            normal_var_def.name.clone(),
                            Identifier::from_variable(var_alloc),
                        )
                        .map_err(|_| ParseError::MultipleDefinition)?;
                    Ok(var_alloc)
                }
            }

            VarDef::ArrayVarDef(array_var) => {
                let var_name = format!("@_{}_{}", scope_id, array_var.name);
                let shape = array_var
                    .shape
                    .iter()
                    .map(|x| x.eval(&mut ctx.scope).unwrap_or(0))
                    .collect::<Vec<_>>();

                if ctx.is_global() {
                    // global array
                    let initial_list = if let Some(initial) = &array_var.values {
                        match initial {
                            ExprArray::Val(_) => {
                                show_error("Invalid array initialization", 2);
                            }
                            ExprArray::Array(array) => {
                                InitializeList::from_expr_array(&shape, array, ctx)
                            }
                        }
                    } else {
                        InitializeList::zero(&shape)
                    };

                    let init_value = initial_list.to_global_value(ctx);
                    let alloc = ctx.program.new_value().global_alloc(init_value);
                    ctx.program.set_value_name(alloc, Some(var_name));
                    ctx.scope
                        .add_identifier(array_var.name.clone(), Identifier::from_variable(alloc))
                        .map_err(|_| ParseError::MultipleDefinition)?;

                    Ok(alloc)
                } else {
                    // local array
                    let array_type = get_array_type(&shape, &mut ctx.scope);
                    let bb = ctx.get_bb()?;
                    let func_data = ctx.func_data_mut()?;
                    let alloc = new_value!(func_data).alloc(array_type);
                    func_data.dfg_mut().set_value_name(alloc, Some(var_name));
                    add_inst!(func_data, bb, alloc);
                    if let Some(initial) = &array_var.values {
                        let initial_list = match initial {
                            ExprArray::Val(_) => {
                                show_error("Invalid array initialization", 2);
                            }
                            ExprArray::Array(array) => {
                                InitializeList::from_expr_array(&shape, array, ctx)
                            }
                        };
                        // Get initial value and store it
                        let init_value = initial_list.to_local_value(ctx);
                        let store = new_value!(ctx.func_data_mut()?).store(init_value, alloc);
                        add_inst!(ctx.func_data_mut()?, bb, store);
                    }
                    ctx.scope
                        .add_identifier(array_var.name.clone(), Identifier::from_variable(alloc))
                        .map_err(|_| ParseError::MultipleDefinition)?;
                    Ok(alloc)
                }
            }
        }
    }
}

impl GenerateIR for ConstDef {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        match self {
            ConstDef::NormalConstDef(normal) => {
                let val = normal
                    .value
                    .eval(&mut ctx.scope)
                    .map_err(|_| ParseError::ConstExprError)?;
                ctx.scope
                    .add_identifier(normal.name.clone(), Identifier::from_constant(val))
                    .unwrap_or_else(|e| {
                        show_error(&format!("{:?}", e), 2);
                    });
                Ok(())
            }
            ConstDef::ArrayConstDef(const_array) => {
                let init = match &const_array.values {
                    ConstArray::Val(_) => {
                        show_error("Invalid array initialization", 2);
                    }
                    ConstArray::Array(array) => array,
                };
                let initial_list = InitializeList::from_const_array(&const_array.shape, init, ctx);

                // Add const array to IR
                let scope_id = ctx.scope.current_scope_id();
                let array_name = format!("@_{}_{}", scope_id, const_array.name);

                let koopa_def = if ctx.is_global() {
                    let init_value = initial_list.to_global_value(ctx);
                    let alloc = ctx.program.new_value().global_alloc(init_value);
                    ctx.program.set_value_name(alloc, Some(array_name));
                    alloc
                } else {
                    let array_type = get_array_type(&const_array.shape, &mut ctx.scope);
                    let bb = ctx.get_bb()?;
                    let func_data = ctx.func_data_mut()?;
                    // allocate array
                    let alloc = new_value!(func_data).alloc(array_type);
                    func_data.dfg_mut().set_value_name(alloc, Some(array_name));
                    add_inst!(func_data, bb, alloc);
                    // store initial value
                    let init_value = initial_list.to_local_value(ctx);
                    let store = new_value!(ctx.func_data_mut()?).store(init_value, alloc);
                    add_inst!(ctx.func_data_mut()?, bb, store);
                    alloc
                };

                // Add const array to identifier table
                ctx.scope
                    .add_identifier(
                        const_array.name.clone(),
                        Identifier::from_const_array(koopa_def, initial_list),
                    )
                    .unwrap_or_else(|e| {
                        show_error(&format!("{:?}", e), 2);
                    });

                Ok(())
            }
        }
    }
}

impl GenerateIR for LVal {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            // Normal variable
            LVal::Var(var) => {
                let ident = ctx
                    .scope
                    .get_identifier(var)
                    .ok_or(ParseError::UnknownIdentifier)?
                    .clone();

                let val = match ident {
                    Identifier::Variable(ref var) => {
                        let bb = ctx.get_bb()?;
                        let var_def = var.koopa_def;
                        let ty = get_type(var_def, ctx)?;
                        let ty = remove_pointer(ty);
                        let load = match ty.kind() {
                            TypeKind::Array(_, _) => {
                                let zero = 0.generate_ir(ctx)?;
                                let load =
                                    new_value!(ctx.func_data_mut()?).get_elem_ptr(var_def, zero);
                                add_inst!(ctx.func_data_mut()?, bb, load);
                                load
                            }
                            _ => {
                                let load = new_value!(ctx.func_data_mut()?).load(var_def);
                                add_inst!(ctx.func_data_mut()?, bb, load);
                                load
                            }
                        };
                        load
                    }
                    Identifier::Constant(ref constant) => constant.value.generate_ir(ctx)?,
                    _ => return Err(ParseError::InvalidExpr),
                };
                Ok(val)
            }
            // Array element
            LVal::ArrayElem(array_elem) => {
                let pos = get_array_pos(array_elem, ctx)?;
                let pos_type = ctx.func_data_mut()?.dfg().value(pos).ty().clone();
                let target_type = remove_pointer(pos_type);

                match target_type.kind() {
                    TypeKind::Int32 => {
                        let load = new_value!(ctx.func_data_mut()?).load(pos);
                        let bb = ctx.get_bb()?;
                        add_inst!(ctx.func_data_mut()?, bb, load);
                        Ok(load)
                    }
                    _ => {
                        let zero = 0.generate_ir(ctx)?;
                        let load = new_value!(ctx.func_data_mut()?).get_elem_ptr(pos, zero);
                        let bb = ctx.get_bb()?;
                        add_inst!(ctx.func_data_mut()?, bb, load);
                        Ok(load)
                    }
                }
            }
        }
    }
}

impl GenerateIR for PrimaryExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            PrimaryExpr::Expr(expr) => expr.generate_ir(ctx),
            PrimaryExpr::LVal(lval) => lval.generate_ir(ctx),
            PrimaryExpr::Number(n) => n.generate_ir(ctx),
        }
    }
}

impl GenerateIR for UnaryExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            UnaryExpr::PrimaryExpr(expr) => expr.generate_ir(ctx),
            UnaryExpr::FuncCall(func_call) => func_call.generate_ir(ctx),
            UnaryExpr::Unary(op, expr) => {
                let expr = expr.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let func_data = ctx.func_data_mut()?;
                let zero = new_value!(func_data).integer(0);
                match op {
                    UnaryOp::Pos => Ok(expr),
                    UnaryOp::Neg => {
                        let value = new_value!(func_data).binary(BinaryOp::Sub, zero, expr);
                        add_inst!(func_data, current_bb, value);
                        Ok(value)
                    }
                    UnaryOp::Not => {
                        // !x = (x == 0)
                        let value = new_value!(func_data).binary(BinaryOp::Eq, expr, zero);
                        add_inst!(func_data, current_bb, value);
                        Ok(value)
                    }
                }
            }
        }
    }
}

impl GenerateIR for FuncCall {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        let param_values = self
            .args
            .iter()
            .map(|arg| arg.generate_ir(ctx))
            .collect::<Result<Vec<Value>, ParseError>>()?;
        let func_name = &self.name;
        let func = ctx
            .func_table
            .get(func_name)
            .copied()
            .ok_or(ParseError::FunctionNotFound)?;
        let ret_val = new_value!(ctx.func_data_mut()?).call(func, param_values);
        let current_bb = ctx.get_bb()?;
        add_inst!(ctx.func_data_mut()?, current_bb, ret_val);
        Ok(ret_val)
    }
}

impl GenerateIR for MulExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            MulExpr::UnaryExpr(expr) => expr.generate_ir(ctx),
            MulExpr::Mul(lhs, op, rhs) => {
                let lhs = lhs.generate_ir(ctx)?;
                let rhs = rhs.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let func_data = ctx.func_data_mut()?;
                let value = new_value!(func_data).binary((*op).into(), lhs, rhs);
                add_inst!(func_data, current_bb, value);
                Ok(value)
            }
        }
    }
}

impl GenerateIR for AddExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            AddExpr::MulExpr(expr) => expr.generate_ir(ctx),
            AddExpr::Add(lhs, op, rhs) => {
                let lhs = lhs.generate_ir(ctx)?;
                let rhs = rhs.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let func_data = ctx.func_data_mut()?;
                let value = new_value!(func_data).binary((*op).into(), lhs, rhs);
                add_inst!(func_data, current_bb, value);
                Ok(value)
            }
        }
    }
}

impl GenerateIR for RelExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            RelExpr::AddExpr(expr) => expr.generate_ir(ctx),
            RelExpr::Rel(lhs, op, rhs) => {
                let lhs = lhs.generate_ir(ctx)?;
                let rhs = rhs.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let func_data = ctx.func_data_mut()?;
                let value = new_value!(func_data).binary((*op).into(), lhs, rhs);
                add_inst!(func_data, current_bb, value);
                Ok(value)
            }
        }
    }
}

impl GenerateIR for EqExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            EqExpr::RelExpr(expr) => expr.generate_ir(ctx),
            EqExpr::Eq(lhs, op, rhs) => {
                let lhs = lhs.generate_ir(ctx)?;
                let rhs = rhs.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let func_data = ctx.func_data_mut()?;
                let value = new_value!(func_data).binary((*op).into(), lhs, rhs);
                add_inst!(func_data, current_bb, value);
                Ok(value)
            }
        }
    }
}

impl GenerateIR for LAndExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            LAndExpr::EqExpr(eq_expr) => eq_expr.generate_ir(ctx),
            LAndExpr::And(lhs, rhs) => {
                let lhs = lhs.generate_ir(ctx)?;

                // alloc a new space to store the result
                let result = new_value!(ctx.func_data_mut()?).alloc(Type::get_i32());
                let result_name = ctx.temp_value_name();
                ctx.func_data_mut()?
                    .dfg_mut()
                    .set_value_name(result, Some(result_name));
                let current_bb = ctx.get_bb()?;
                add_inst!(ctx.func_data_mut()?, current_bb, result);
                let zero = 0.generate_ir(ctx)?;
                let lhs = new_value!(ctx.func_data_mut()?).binary(BinaryOp::NotEq, lhs, zero);
                add_inst!(ctx.func_data_mut()?, current_bb, lhs);
                let store = new_value!(ctx.func_data_mut()?).store(lhs, result);
                add_inst!(ctx.func_data_mut()?, current_bb, store);

                let end_bb = ctx.new_bb()?;
                let true_bb = ctx.new_bb()?;

                // If lhs is false, jump to end_bb
                // Otherwise, evaluate rhs
                let load = new_value!(ctx.func_data_mut()?).load(result);
                add_inst!(ctx.func_data_mut()?, current_bb, load);
                let branch = new_value!(ctx.func_data_mut()?).branch(load, true_bb, end_bb);
                add_inst!(ctx.func_data_mut()?, current_bb, branch);

                add_bb!(ctx.func_data_mut()?, true_bb);
                ctx.current_bb = Some(true_bb);
                let rhs = rhs.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let zero = 0.generate_ir(ctx)?;
                let rhs = new_value!(ctx.func_data_mut()?).binary(BinaryOp::NotEq, rhs, zero);
                add_inst!(ctx.func_data_mut()?, current_bb, rhs);
                let store = new_value!(ctx.func_data_mut()?).store(rhs, result);
                add_inst!(ctx.func_data_mut()?, current_bb, store);
                // Jump to end_bb
                let jump = new_value!(ctx.func_data_mut()?).jump(end_bb);
                add_inst!(ctx.func_data_mut()?, current_bb, jump);

                add_bb!(ctx.func_data_mut()?, end_bb);
                ctx.current_bb = Some(end_bb);
                let load_res = new_value!(ctx.func_data_mut()?).load(result);
                add_inst!(ctx.func_data_mut()?, end_bb, load_res);
                Ok(load_res)
            }
        }
    }
}

impl GenerateIR for LOrExpr {
    type Output = Value;

    fn generate_ir(&self, ctx: &mut Context) -> Result<Value, ParseError> {
        match self {
            LOrExpr::LAndExpr(and_expr) => and_expr.generate_ir(ctx),
            LOrExpr::Or(lhs, rhs) => {
                let lhs = lhs.generate_ir(ctx)?;
                // alloc a new space to store the result
                let result = new_value!(ctx.func_data_mut()?).alloc(Type::get_i32());
                let result_name = ctx.temp_value_name();
                ctx.func_data_mut()?
                    .dfg_mut()
                    .set_value_name(result, Some(result_name));
                let current_bb = ctx.get_bb()?;
                add_inst!(ctx.func_data_mut()?, current_bb, result);
                let zero = 0.generate_ir(ctx)?;
                let lhs = new_value!(ctx.func_data_mut()?).binary(BinaryOp::NotEq, lhs, zero);
                add_inst!(ctx.func_data_mut()?, current_bb, lhs);
                let store = new_value!(ctx.func_data_mut()?).store(lhs, result);
                add_inst!(ctx.func_data_mut()?, current_bb, store);

                let end_bb = ctx.new_bb()?;
                let false_bb = ctx.new_bb()?;

                // If lhs is true, jump to end_bb
                // Otherwise, evaluate rhs
                let load = new_value!(ctx.func_data_mut()?).load(result);
                add_inst!(ctx.func_data_mut()?, current_bb, load);
                let branch = new_value!(ctx.func_data_mut()?).branch(load, end_bb, false_bb);
                add_inst!(ctx.func_data_mut()?, current_bb, branch);

                add_bb!(ctx.func_data_mut()?, false_bb);
                ctx.current_bb = Some(false_bb);
                let rhs = rhs.generate_ir(ctx)?;
                let current_bb = ctx.get_bb()?;
                let zero = 0.generate_ir(ctx)?;
                let rhs = new_value!(ctx.func_data_mut()?).binary(BinaryOp::NotEq, rhs, zero);
                add_inst!(ctx.func_data_mut()?, current_bb, rhs);
                let store = new_value!(ctx.func_data_mut()?).store(rhs, result);
                add_inst!(ctx.func_data_mut()?, current_bb, store);
                // Jump to end_bb
                let jump = new_value!(ctx.func_data_mut()?).jump(end_bb);
                add_inst!(ctx.func_data_mut()?, current_bb, jump);

                add_bb!(ctx.func_data_mut()?, end_bb);
                ctx.current_bb = Some(end_bb);
                let load_res = new_value!(ctx.func_data_mut()?).load(result);
                add_inst!(ctx.func_data_mut()?, end_bb, load_res);
                Ok(load_res)
            }
        }
    }
}

impl GenerateIR for FuncDef {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        let ret_type = self.ret_type.into();
        let func_params = get_func_param(&self.params, &mut ctx.scope);
        let func_data =
            FunctionData::with_param_names("@".to_string() + &self.name, func_params, ret_type);
        let func = ctx.program.new_func(func_data);
        ctx.func_table.insert(self.name.clone(), func);
        ctx.func = Some(func);

        // Restore parameters because they may be modified in function body
        let store_bb = ctx.new_bb()?;
        let func_data = ctx.program.func_mut(func);
        ctx.scope.go_into_scoop(self.body.id);
        add_bb!(func_data, store_bb);
        let params = func_data.params().iter().copied().collect::<Vec<_>>();
        for param in params {
            // TODO: When a parameter is not reassigned, we don't need to allocate a new space
            let param_data = func_data.dfg().value(param);
            let param_name = param_data
                .name()
                .clone()
                .map(|s| s[1..].to_string())
                .unwrap()
                .to_string();
            let param_type = param_data.ty().clone();
            let alloc_param = new_value!(func_data).alloc(param_type);
            func_data
                .dfg_mut()
                .set_value_name(alloc_param, Some(format!("@_p_{}", param_name)));
            let store = new_value!(func_data).store(param, alloc_param);
            add_inst!(func_data, store_bb, alloc_param);
            add_inst!(func_data, store_bb, store);
            ctx.scope
                .add_identifier(param_name, Identifier::from_variable(alloc_param))
                .map_err(|e| show_error(&format!("{:?}", e), 2))?;
        }

        self.body.generate_ir(ctx)?;
        ctx.scope.go_out_scoop();
        ctx.func = None;
        Ok(())
    }
}

impl GenerateIR for Stmt {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<Self::Output, ParseError> {
        match self {
            Stmt::Assign(assign) => assign.generate_ir(ctx),
            Stmt::Expr(expr) => expr.generate_ir(ctx).map(|_| ()),
            Stmt::Block(block) => {
                ctx.scope.go_into_scoop(block.id);
                block.generate_ir(ctx)?;
                ctx.scope.go_out_scoop();
                Ok(())
            }
            Stmt::If(if_stmt) => if_stmt.generate_ir(ctx),
            Stmt::While(while_stmt) => while_stmt.generate_ir(ctx),
            Stmt::Return(ret) => ret.generate_ir(ctx),
            Stmt::Break(break_stmt) => break_stmt.generate_ir(ctx),
            Stmt::Continue(continue_stmt) => continue_stmt.generate_ir(ctx),
            Stmt::Empty => Ok(()),
        }
    }
}

impl GenerateIR for Break {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        let end_bb = ctx
            .get_while_info()
            .ok_or(ParseError::BreakOutsideLoop)?
            .end_bb;
        let jump = new_value!(ctx.func_data_mut()?).jump(end_bb);
        let bb = ctx.get_bb()?;
        add_inst!(ctx.func_data_mut()?, bb, jump);
        Ok(())
    }
}

impl GenerateIR for Continue {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        let start_bb = ctx
            .get_while_info()
            .ok_or(ParseError::ContinueOutsideLoop)?
            .start_bb;
        let jump = new_value!(ctx.func_data_mut()?).jump(start_bb);
        let bb = ctx.get_bb()?;
        add_inst!(ctx.func_data_mut()?, bb, jump);
        Ok(())
    }
}

impl GenerateIR for While {
    type Output = ();
    fn generate_ir(&self, ctx: &mut Context) -> Result<Self::Output, ParseError> {
        let body_bb = ctx.new_bb()?;
        let end_bb = ctx.new_bb()?;
        let start_bb = ctx.new_bb()?;
        add_bb!(ctx.func_data_mut()?, start_bb);
        ctx.current_bb = Some(start_bb);
        let cond_value = self.cond.generate_ir(ctx)?;
        let start_branch_bb = ctx.get_bb()?;
        // branch to body or end
        let branch = new_value!(ctx.func_data_mut()?).branch(cond_value, body_bb, end_bb);
        add_inst!(ctx.func_data_mut()?, start_branch_bb, branch);
        add_bb!(ctx.func_data_mut()?, body_bb);

        // generate body
        ctx.add_while_info(start_bb, end_bb);
        ctx.current_bb = Some(body_bb);
        self.body.generate_ir(ctx)?;

        // jump to start_bb
        let body_bb_end = ctx.get_bb()?;
        let jump_start = new_value!(ctx.func_data_mut()?).jump(start_bb);
        if !ctx.block_ended(body_bb_end)? {
            add_inst!(ctx.func_data_mut()?, body_bb_end, jump_start);
        } else {
            let body_bb_end = ctx.new_bb()?;
            add_bb!(ctx.func_data_mut()?, body_bb_end);
            add_inst!(ctx.func_data_mut()?, body_bb_end, jump_start);
        }
        add_bb!(ctx.func_data_mut()?, end_bb);
        ctx.current_bb = Some(end_bb);
        ctx.pop_while_info();
        Ok(())
    }
}

impl GenerateIR for Block {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        let bb = ctx.new_bb()?;
        let func_data = ctx.func_data_mut()?;
        add_bb!(func_data, bb);
        ctx.current_bb = Some(bb);
        for block_item in &self.items {
            match block_item {
                BlockItem::Stmt(stmt) => match stmt {
                    Stmt::Return(_) | Stmt::Break(_) | Stmt::Continue(_) => {
                        stmt.generate_ir(ctx)?;
                        // Once return, break or continue is called, we don't need to generate any more IR
                        break;
                    }
                    _ => stmt.generate_ir(ctx)?,
                },
                BlockItem::Decl(decl) => decl.generate_ir(ctx)?,
            }
        }
        let bb = ctx.new_bb()?;
        let func_data = ctx.func_data_mut()?;
        add_bb!(func_data, bb);
        ctx.current_bb = Some(bb);
        Ok(())
    }
}

impl GenerateIR for Decl {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        match self {
            Decl::ConstDecl(const_decl) => {
                for const_def in const_decl {
                    const_def.generate_ir(ctx)?;
                }
            }
            Decl::VarDecl(var_decl) => {
                for var_def in var_decl {
                    var_def.generate_ir(ctx)?;
                }
            }
        }
        Ok(())
    }
}

impl GenerateIR for If {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<Self::Output, ParseError> {
        // TODO: Modify cond
        let cond = self.cond.generate_ir(ctx)?;
        let current_bb = ctx.get_bb()?;
        let then_bb = ctx.new_bb()?;
        add_bb!(ctx.func_data_mut()?, then_bb);

        // environment for then block
        ctx.current_bb = Some(then_bb);
        self.then_stmt.generate_ir(ctx)?;
        let then_bb_end = ctx.get_bb()?;
        let end_bb = ctx.new_bb()?;

        let branch = if let Some(else_stmt) = &self.else_stmt {
            let else_bb = ctx.new_bb()?;
            add_bb!(ctx.func_data_mut()?, else_bb);

            // environment for else block
            ctx.current_bb = Some(else_bb);
            else_stmt.generate_ir(ctx)?;
            let else_bb_end = ctx.get_bb()?;
            add_bb!(ctx.func_data_mut()?, end_bb);
            ctx.end_block(then_bb_end, end_bb)?;
            ctx.end_block(else_bb_end, end_bb)?;
            new_value!(ctx.func_data_mut()?).branch(cond, then_bb, else_bb)
        } else {
            add_bb!(ctx.func_data_mut()?, end_bb);
            ctx.end_block(then_bb_end, end_bb)?;
            new_value!(ctx.func_data_mut()?).branch(cond, then_bb, end_bb)
        };
        add_inst!(ctx.func_data_mut()?, current_bb, branch);
        ctx.current_bb = Some(end_bb);
        Ok(())
    }
}

impl GenerateIR for Return {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        if let Some(expr) = self {
            if let Ok(ret_val) = expr.eval(&mut ctx.scope) {
                let ret_val = new_value!(ctx.func_data_mut()?).integer(ret_val);
                let ret = new_value!(ctx.func_data_mut()?).ret(Some(ret_val));
                let bb = ctx.get_bb()?;
                add_inst!(ctx.func_data_mut()?, bb, ret);
            } else {
                let val = expr.generate_ir(ctx)?;
                let ret = new_value!(ctx.func_data_mut()?).ret(Some(val));
                let bb = ctx.get_bb()?;
                add_inst!(ctx.func_data_mut()?, bb, ret);
            }
        } else {
            let ret = ctx.func_data_mut()?.dfg_mut().new_value().ret(None);
            let bb = ctx.get_bb()?;
            add_inst!(ctx.func_data_mut()?, bb, ret);
        }
        Ok(())
    }
}

impl GenerateIR for Assign {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        match self.target {
            LVal::Var(ref var) => {
                let val = self.value.generate_ir(ctx)?;
                let var_decl = ctx
                    .scope
                    .get_identifier(var)
                    .map(|x| x.koopa_def())
                    .ok_or(ParseError::UnknownIdentifier)?
                    .ok_or(ParseError::UnknownIdentifier)?;
                let store = new_value!(ctx.func_data_mut()?).store(val, var_decl);
                let bb = ctx.get_bb()?;
                add_inst!(ctx.func_data_mut()?, bb, store);
                Ok(())
            }
            LVal::ArrayElem(ref array_elem) => {
                let pos = get_array_pos(array_elem, ctx)?;

                // Store value
                let val = self.value.generate_ir(ctx)?;
                let store = new_value!(ctx.func_data_mut()?).store(val, pos);
                let bb = ctx.get_bb()?;
                add_inst!(ctx.func_data_mut()?, bb, store);
                Ok(())
            }
        }
    }
}

impl GenerateIR for CompUnit {
    type Output = ();

    fn generate_ir(&self, ctx: &mut Context) -> Result<(), ParseError> {
        for item in &self.items {
            match item {
                GlobalItem::Decl(decl) => {
                    decl.generate_ir(ctx)?;
                }
                GlobalItem::FuncDef(func_def) => {
                    func_def.generate_ir(ctx)?;
                }
            }
        }
        Ok(())
    }
}

fn get_func_param(params: &Vec<Rc<FuncFParam>>, scope: &mut Scope) -> Vec<(Option<String>, Type)> {
    let mut func_params = vec![];
    for param in params {
        match param.as_ref() {
            FuncFParam::NormalFParam(normal_param) => {
                func_params.push((Some("@".to_string() + &normal_param.name), Type::get_i32()));
            }
            FuncFParam::ArrayFParam(array_param) => {
                let shape = if array_param.placeholder {
                    &array_param.shape[..]
                } else {
                    &array_param.shape[1..]
                };
                let param_type = Type::get_pointer(get_array_type(shape, scope));
                func_params.push((Some("@".to_string() + &array_param.name), param_type));
            }
        }
    }

    func_params
}
