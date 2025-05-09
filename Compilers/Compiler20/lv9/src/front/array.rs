use crate::ast::{ConstInitVal, InitVal};
use crate::front::calculate::Eval;
use crate::front::ctx::IrCtx;
use koopa::ir::builder::{LocalInstBuilder, ValueBuilder};
use koopa::ir::{Function, Program, Value};

pub fn get_init_list(shape: &Vec<i32>, init: &InitVal, ctx: &mut IrCtx) -> Vec<i32> {
    let mut result = Vec::new();
    let length: i32 = shape.iter().product();
    match init {
        InitVal::Exp(exp) => {
            let value = exp.calculate(ctx);
            result.push(value);
        }
        InitVal::InitList(init_list) => {
            for init_val in init_list {
                match init_val {
                    InitVal::Exp(exp) => {
                        let value = exp.calculate(ctx);
                        result.push(value);
                    }
                    InitVal::InitList(_) => {
                        let mut new_shape = Vec::new();
                        let mut len_now = result.len() as i32;
                        for len in shape.iter().rev() {
                            let res = len_now % *len;
                            len_now = len_now / *len;
                            if res == 0 {
                                new_shape.insert(0, *len);
                            }
                            if len_now == 0 {
                                break;
                            }
                            if res != 0 && len_now != 0 {
                                unreachable!()
                            }
                        }
                        let sub_result = get_init_list(&new_shape, init_val, ctx);
                        result.extend(sub_result);
                    }
                }
            }
            for _ in result.len() as i32..length {
                result.push(0);
            }
        }
    }
    result
}

pub fn get_global_aggr(shape: &[i32], init: Vec<Value>, program: &mut Program) -> Value {
    if shape.len() == 1 {
        program.new_value().aggregate(init)
    } else {
        let mut value_vec = Vec::new();
        let new_shape = &shape[1..];
        let sub_length: i32 = new_shape.iter().product();
        for i in 0..shape[0] {
            let mut sub_init = Vec::new();
            for j in 0..sub_length {
                sub_init.push(init[(i * sub_length + j) as usize]);
            }
            value_vec.push(get_global_aggr(new_shape, sub_init, program));
        }
        program.new_value().aggregate(value_vec)
    }
}

pub fn get_init_list_const(shape: &Vec<i32>, init: &ConstInitVal, ctx: &mut IrCtx) -> Vec<i32> {
    let mut result = Vec::new();
    let length: i32 = shape.iter().product();
    match init {
        ConstInitVal::ConstExp(exp) => {
            let value = exp.calculate(ctx);
            result.push(value);
        }
        ConstInitVal::ConstInitList(init_list) => {
            for init_val in init_list {
                match init_val {
                    ConstInitVal::ConstExp(exp) => {
                        let value = exp.calculate(ctx);
                        result.push(value);
                    }
                    ConstInitVal::ConstInitList(_) => {
                        let mut new_shape = Vec::new();
                        let mut len_now = result.len() as i32;
                        for len in shape.iter().rev() {
                            let res = len_now % *len;
                            len_now = len_now / *len;
                            if res == 0 {
                                new_shape.insert(0, *len);
                            }
                            if len_now == 0 {
                                break;
                            }
                            if res != 0 && len_now != 0 {
                                unreachable!()
                            }
                        }
                        let sub_result = get_init_list_const(&new_shape, init_val, ctx);
                        result.extend(sub_result);
                    }
                }
            }
            for _ in result.len() as i32..length {
                result.push(0);
            }
        }
    }
    result
}

pub fn init_local_array(
    shape: &Vec<i32>,
    init: Vec<Value>,
    program: &mut Program,
    func: Function,
    ctx: &IrCtx,
    base: Value,
) {
    if shape.len() == 1 {
        for i in 0..shape[0] {
            let func_data = program.func_mut(func);
            let index = func_data.dfg_mut().new_value().integer(i);
            let get_elem_ptr = func_data.dfg_mut().new_value().get_elem_ptr(base, index);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![get_elem_ptr]);
            let store = func_data
                .dfg_mut()
                .new_value()
                .store(init[i as usize], get_elem_ptr);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![store]);
        }
    } else {
        for i in 0..shape[0] {
            let func_data = program.func_mut(func);
            let index = func_data.dfg_mut().new_value().integer(i);
            let get_elem_ptr = func_data.dfg_mut().new_value().get_elem_ptr(base, index);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![get_elem_ptr]);
            let mut new_init = Vec::new();
            let sub_length: i32 = shape[1..].iter().product();
            for j in 0..sub_length {
                new_init.push(init[(i * sub_length + j) as usize]);
            }
            init_local_array(
                &shape[1..].to_vec(),
                new_init,
                program,
                func,
                ctx,
                get_elem_ptr,
            );
        }
    }
}
