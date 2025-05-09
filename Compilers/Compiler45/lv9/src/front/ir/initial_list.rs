use crate::front::ast::{ConstArray, Expr, ExprArray};
use crate::front::ir::context::Context;
use crate::front::ir::eval::Eval;
use crate::front::ir::initial_list::InitializeList::{NonZero, Zero};
use crate::front::ir::{get_array_type, GenerateIR};
use crate::new_value;
use koopa::ir::builder::ValueBuilder;
use koopa::ir::Value;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum InitializeList<T: Default + Clone> {
    Zero(Vec<i32>),
    NonZero(Vec<T>, Vec<i32>),
}

impl InitializeList<i32> {
    pub fn from_const_array<T: Eval>(
        shape: &[T],
        const_array: &[ConstArray],
        ctx: &mut Context,
    ) -> Self {
        let shape = shape
            .iter()
            .map(|x| x.eval(&mut ctx.scope).unwrap_or(0))
            .collect::<Vec<_>>();
        let size: i32 = shape.iter().product();
        let mut data = Vec::with_capacity(size as usize);

        for constant in const_array {
            match constant {
                ConstArray::Val(expr) => data.push(expr.eval(&mut ctx.scope).unwrap_or(0)),
                ConstArray::Array(array) => {
                    let align = Self::align(data.len() as i32, &shape);
                    let inner =
                        Self::from_const_array(&shape[shape.len() - align as usize..], array, ctx);
                    if let NonZero(inner_data, _) = inner {
                        data.extend(inner_data);
                    }
                }
            }
        }

        for _ in data.len()..size as usize {
            data.push(0);
        }

        NonZero(data, shape)
    }
}

impl InitializeList<Expr> {
    pub fn from_expr_array<T: Eval>(
        shape: &[T],
        expr_array: &[ExprArray],
        ctx: &mut Context,
    ) -> Self {
        let shape = shape
            .iter()
            .map(|x| x.eval(&mut ctx.scope).unwrap_or(0))
            .collect::<Vec<_>>();
        let size: i32 = shape.iter().product();
        let mut data = Vec::with_capacity(size as usize);

        for expr in expr_array {
            match expr {
                ExprArray::Val(expr) => data.push(expr.clone()),
                ExprArray::Array(expr_array) => {
                    let align = Self::align(data.len() as i32, &shape);
                    let inner = Self::from_expr_array(
                        &shape[shape.len() - align as usize..],
                        expr_array,
                        ctx,
                    );
                    if let NonZero(inner_data, _) = inner {
                        data.extend(inner_data);
                    }
                }
            }
        }

        for _ in data.len()..size as usize {
            data.push(Expr::default());
        }

        NonZero(data, shape)
    }
}

impl<T: Default + Clone + Eval + GenerateIR<Output = Value> + PartialEq> InitializeList<T> {
    pub fn zero(shape: &[i32]) -> Self {
        Zero(shape.to_vec())
    }

    fn align(len: i32, shape: &[i32]) -> i32 {
        let mut align = 0;
        let mut product = 1;
        if len == 0 {
            return shape.len() as i32 - 1;
        }
        for s in shape.iter().rev() {
            product *= s;
            if len % product != 0 {
                break;
            }
            align += 1;
        }
        align
    }

    pub fn to_global_value(&self, ctx: &mut Context) -> Value {
        match self {
            Zero(shape) => ctx
                .program
                .new_value()
                .zero_init(get_array_type(shape, &mut ctx.scope)),
            NonZero(data, shape) => {
                if data.iter().all(|x| *x == T::default()) {
                    return ctx
                        .program
                        .new_value()
                        .zero_init(get_array_type(shape, &mut ctx.scope));
                }
                let mut values = vec![];
                if shape.len() == 1 {
                    for expr in data {
                        let value = expr.eval(&mut ctx.scope).unwrap_or(0);
                        values.push(ctx.program.new_value().integer(value));
                    }
                } else {
                    let outer_shape = shape[0];
                    let inner_shape = shape[1..].to_vec();
                    let inner_size = inner_shape.iter().product::<i32>() as usize;
                    for i in 0..outer_shape {
                        let inner = NonZero(
                            data[i as usize * inner_size..(i + 1) as usize * inner_size].to_vec(),
                            inner_shape.clone(),
                        );
                        values.push(inner.to_global_value(ctx));
                    }
                }
                ctx.program.new_value().aggregate(values)
            }
        }
    }

    pub fn to_local_value(&self, ctx: &mut Context) -> Value {
        match self {
            Zero(shape) => {
                let ty = get_array_type(shape, &mut ctx.scope);
                new_value!(ctx.func_data_mut().unwrap()).zero_init(ty)
            }
            NonZero(data, shape) => {
                if data.iter().all(|x| *x == T::default()) {
                    let ty = get_array_type(shape, &mut ctx.scope);
                    return new_value!(ctx.func_data_mut().unwrap()).zero_init(ty);
                }
                let mut values = vec![];
                if shape.len() == 1 {
                    for expr in data {
                        let value = expr.generate_ir(ctx).unwrap();
                        values.push(value);
                    }
                } else {
                    let outer_shape = shape[0];
                    let inner_shape = shape[1..].to_vec();
                    let inner_size = inner_shape.iter().product::<i32>() as usize;
                    for i in 0..outer_shape {
                        let inner = NonZero(
                            data[i as usize * inner_size..(i + 1) as usize * inner_size].to_vec(),
                            inner_shape.clone(),
                        );
                        values.push(inner.to_local_value(ctx));
                    }
                }
                new_value!(ctx.func_data_mut().unwrap()).aggregate(values)
            }
        }
    }

    pub fn get_element(&self, index: &[i32]) -> T {
        if let NonZero(data, shape) = self {
            let mut offset = 0;
            for (i, idx) in index.iter().enumerate() {
                offset += idx;
                if i < shape.len() - 1 {
                    offset *= shape[i + 1];
                }
            }
            data[offset as usize].clone()
        } else {
            T::default()
        }
    }
}
