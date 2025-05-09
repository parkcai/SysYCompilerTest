use crate::ast::*;
use crate::front::array::{get_global_aggr, get_init_list, get_init_list_const, init_local_array};
use crate::front::calculate::Eval;
use crate::front::ctx::{IrCtx, LoopInfo};
use koopa::ir::builder::{BasicBlockBuilder, GlobalInstBuilder, LocalInstBuilder, ValueBuilder};
use koopa::ir::{Function, FunctionData, Program, Type, TypeKind, Value, ValueKind};
use std::process::exit;

pub(crate) fn generate_ir(comp_unit: &CompUnit) -> Program {
    let mut program = Program::new();
    generate_lib(&mut program);
    let mut ctx = IrCtx::new();
    for item in comp_unit.items.iter() {
        match item {
            GlobalItem::Decl(decl) => match decl {
                Decl::VarDecl(var_decl) => {
                    for var_def in var_decl.iter() {
                        if var_def.shape.is_empty() {
                            let init = match &var_def.init {
                                Some(init) => match init {
                                    InitVal::Exp(exp) => exp.calculate(&mut ctx),
                                    _ => unreachable!(),
                                },
                                None => 0,
                            };
                            let init_value;
                            let alloc;
                            if init == 0 {
                                init_value = program.new_value().zero_init(Type::get_i32());
                                alloc = program.new_value().global_alloc(init_value);
                            } else {
                                init_value = program.new_value().integer(init);
                                alloc = program.new_value().global_alloc(init_value);
                            }
                            program.set_value_name(
                                alloc,
                                Some("@".to_string() + &*var_def.id.clone()),
                            );
                            ctx.add_var(var_def.id.clone(), alloc, false);
                        } else {
                            let mut shape = Vec::new();
                            for const_exp in var_def.shape.iter() {
                                let value = const_exp.calculate(&mut ctx);
                                shape.push(value);
                            }
                            match &var_def.init {
                                Some(init) => match init {
                                    InitVal::InitList(il) => {
                                        if il.is_empty() {
                                            let mut array_type = Type::get_i32();
                                            for s in shape.iter().rev() {
                                                array_type =
                                                    Type::get_array(array_type, *s as usize);
                                            }
                                            let zero_init =
                                                program.new_value().zero_init(array_type);
                                            let alloc = program.new_value().global_alloc(zero_init);
                                            program.set_value_name(
                                                alloc,
                                                Some("@".to_string() + &*var_def.id.clone()),
                                            );
                                            ctx.add_var(var_def.id.clone(), alloc, false);
                                        } else {
                                            let init_list = get_init_list(&shape, init, &mut ctx);
                                            let init_value_list = init_list
                                                .iter()
                                                .map(|&x| program.new_value().integer(x))
                                                .collect::<Vec<Value>>();
                                            let aggr = get_global_aggr(
                                                &shape,
                                                init_value_list,
                                                &mut program,
                                            );
                                            let alloc = program.new_value().global_alloc(aggr);
                                            program.set_value_name(
                                                alloc,
                                                Some("@".to_string() + &*var_def.id.clone()),
                                            );
                                            ctx.add_var(var_def.id.clone(), alloc, false);
                                        }
                                    }
                                    _ => unreachable!(),
                                },
                                None => {
                                    let mut array_type = Type::get_i32();
                                    for s in shape.iter().rev() {
                                        array_type = Type::get_array(array_type, *s as usize);
                                    }
                                    let zero_init = program.new_value().zero_init(array_type);
                                    let alloc = program.new_value().global_alloc(zero_init);
                                    program.set_value_name(
                                        alloc,
                                        Some("@".to_string() + &*var_def.id.clone()),
                                    );
                                    ctx.add_var(var_def.id.clone(), alloc, false);
                                }
                            }
                        }
                    }
                }
                Decl::ConstDecl(const_decl) => {
                    for const_def in const_decl.iter() {
                        if const_def.shape.is_empty() {
                            match &const_def.val {
                                ConstInitVal::ConstExp(exp) => {
                                    let value = exp.calculate(&mut ctx);
                                    ctx.add_const(const_def.id.clone(), value);
                                }
                                _ => {
                                    unreachable!()
                                }
                            }
                        } else {
                            let mut shape = Vec::new();
                            for const_exp in const_def.shape.iter() {
                                let value = const_exp.calculate(&mut ctx);
                                shape.push(value);
                            }
                            match &const_def.val {
                                ConstInitVal::ConstInitList(_) => {
                                    let init_list =
                                        get_init_list_const(&shape, &const_def.val, &mut ctx);
                                    let init_value_list = init_list
                                        .iter()
                                        .map(|&x| program.new_value().integer(x))
                                        .collect::<Vec<Value>>();
                                    let aggr =
                                        get_global_aggr(&shape, init_value_list, &mut program);
                                    let alloc = program.new_value().global_alloc(aggr);
                                    program.set_value_name(
                                        alloc,
                                        Some("@".to_string() + &*const_def.id.clone()),
                                    );
                                    ctx.add_var(const_def.id.clone(), alloc, false);
                                }
                                _ => {
                                    unreachable!()
                                }
                            }
                        }
                    }
                }
            },
            GlobalItem::FuncDef(func_def) => generate_func_def(func_def, &mut program, &mut ctx),
        }
    }
    program
}

pub fn generate_lib(program: &mut Program) {
    program.new_func(FunctionData::new_decl(
        "@getint".to_string(),
        Vec::new(),
        Type::get_i32(),
    ));
    program.new_func(FunctionData::new_decl(
        "@getch".to_string(),
        Vec::new(),
        Type::get_i32(),
    ));
    program.new_func(FunctionData::new_decl(
        "@getarray".to_string(),
        vec![Type::get_pointer(Type::get_i32())],
        Type::get_i32(),
    ));
    program.new_func(FunctionData::new_decl(
        "@putint".to_string(),
        vec![Type::get_i32()],
        Type::get_unit(),
    ));
    program.new_func(FunctionData::new_decl(
        "@putch".to_string(),
        vec![Type::get_i32()],
        Type::get_unit(),
    ));
    program.new_func(FunctionData::new_decl(
        "@putarray".to_string(),
        vec![Type::get_i32(), Type::get_pointer(Type::get_i32())],
        Type::get_unit(),
    ));
    program.new_func(FunctionData::new_decl(
        "@starttime".to_string(),
        Vec::new(),
        Type::get_unit(),
    ));
    program.new_func(FunctionData::new_decl(
        "@stoptime".to_string(),
        Vec::new(),
        Type::get_unit(),
    ));
}

fn generate_decl(decl: &Decl, program: &mut Program, ctx: &mut IrCtx, func: Function) {
    match decl {
        Decl::VarDecl(var_decl) => {
            for var_def in var_decl.iter() {
                if var_def.shape.is_empty() {
                    let func_data = program.func_mut(func);
                    let alloc = func_data.dfg_mut().new_value().alloc(Type::get_i32());
                    func_data
                        .dfg_mut()
                        .set_value_name(alloc, Some("@".to_string() + &*var_def.id.clone()));
                    ctx.add_var(var_def.id.clone(), alloc, false);
                    func_data
                        .layout_mut()
                        .bb_mut(ctx.get_basic_block().unwrap())
                        .insts_mut()
                        .extend(vec![alloc]);
                    match &var_def.init {
                        Some(init) => match init {
                            InitVal::Exp(exp) => {
                                let r = generate_exp(exp, program, func, ctx).unwrap();
                                let func_data = program.func_mut(func);
                                let store = func_data.dfg_mut().new_value().store(r, alloc);
                                func_data
                                    .layout_mut()
                                    .bb_mut(ctx.get_basic_block().unwrap())
                                    .insts_mut()
                                    .extend(vec![store]);
                            }
                            _ => {
                                unreachable!()
                            }
                        },
                        None => {}
                    };
                } else {
                    let mut shape = Vec::new();
                    for const_exp in var_def.shape.iter() {
                        let value = const_exp.calculate(ctx);
                        shape.push(value);
                    }
                    match &var_def.init {
                        Some(init) => {
                            let init_list = get_init_list(&shape, init, ctx);
                            let func_data = program.func_mut(func);
                            let init_value_list = init_list
                                .iter()
                                .map(|&x| func_data.dfg_mut().new_value().integer(x))
                                .collect::<Vec<Value>>();
                            let mut array_type = Type::get_i32();
                            for s in shape.iter().rev() {
                                array_type = Type::get_array(array_type, *s as usize);
                            }
                            let func_data = program.func_mut(func);
                            let alloc = func_data.dfg_mut().new_value().alloc(array_type);
                            func_data.dfg_mut().set_value_name(
                                alloc,
                                Some("@".to_string() + &*var_def.id.clone()),
                            );
                            ctx.add_var(var_def.id.clone(), alloc, false);
                            func_data
                                .layout_mut()
                                .bb_mut(ctx.get_basic_block().unwrap())
                                .insts_mut()
                                .extend(vec![alloc]);
                            init_local_array(&shape, init_value_list, program, func, ctx, alloc);
                        }
                        None => {
                            let length: i32 = shape.iter().product();
                            let func_data = program.func_mut(func);
                            let zero = func_data.dfg_mut().new_value().integer(0);
                            let init_value_list = vec![zero; length as usize];
                            let mut array_type = Type::get_i32();
                            for s in shape.iter().rev() {
                                array_type = Type::get_array(array_type, *s as usize);
                            }
                            let func_data = program.func_mut(func);
                            let alloc = func_data.dfg_mut().new_value().alloc(array_type);
                            func_data.dfg_mut().set_value_name(
                                alloc,
                                Some("@".to_string() + &*var_def.id.clone()),
                            );
                            ctx.add_var(var_def.id.clone(), alloc, false);
                            func_data
                                .layout_mut()
                                .bb_mut(ctx.get_basic_block().unwrap())
                                .insts_mut()
                                .extend(vec![alloc]);
                            // init_local_array(&shape, init_value_list, program, func, ctx, alloc);
                        }
                    }
                }
            }
        }
        Decl::ConstDecl(const_decl) => {
            for const_def in const_decl.iter() {
                if const_def.shape.is_empty() {
                    match &const_def.val {
                        ConstInitVal::ConstExp(exp) => {
                            let value = exp.calculate(ctx);
                            ctx.add_const(const_def.id.clone(), value);
                            ctx.add_const(const_def.id.clone(), value);
                        }
                        ConstInitVal::ConstInitList(list) => {
                            unreachable!()
                        }
                    }
                } else {
                    let mut shape = Vec::new();
                    for const_exp in const_def.shape.iter() {
                        let value = const_exp.calculate(ctx);
                        shape.push(value);
                    }
                    let init_list = get_init_list_const(&shape, &const_def.val, ctx);
                    let func_data = program.func_mut(func);
                    let init_value_list = init_list
                        .iter()
                        .map(|&x| func_data.dfg_mut().new_value().integer(x))
                        .collect::<Vec<Value>>();
                    let mut array_type = Type::get_i32();
                    for s in shape.iter().rev() {
                        array_type = Type::get_array(array_type, *s as usize);
                    }
                    let func_data = program.func_mut(func);
                    let alloc = func_data.dfg_mut().new_value().alloc(array_type);
                    func_data
                        .dfg_mut()
                        .set_value_name(alloc, Some("@".to_string() + &*const_def.id.clone()));
                    ctx.add_var(const_def.id.clone(), alloc, false);
                    func_data
                        .layout_mut()
                        .bb_mut(ctx.get_basic_block().unwrap())
                        .insts_mut()
                        .extend(vec![alloc]);
                    init_local_array(&shape, init_value_list, program, func, ctx, alloc);
                }
            }
        }
    }
}

fn generate_func_def(func_def: &FuncDef, program: &mut Program, ctx: &mut IrCtx) {
    ctx.into_block();
    let func_type = match func_def.ret_type {
        FuncType::Void => Type::get_unit(),
        FuncType::Int => Type::get_i32(),
    };
    let func_name = "@".to_string() + &func_def.id;
    let params = generate_params(&func_def.params, ctx);
    let func = program.new_func(FunctionData::with_param_names(
        func_name,
        params.clone(),
        func_type,
    ));
    let func_data = program.func_mut(func);

    let entry = func_data
        .dfg_mut()
        .new_bb()
        .basic_block(Some("%entry".into()));
    let func_data = program.func_mut(func);
    func_data.layout_mut().bbs_mut().extend([entry]);
    ctx.set_basic_block(entry);

    for i in 0..program.func(func).params().len() {
        let value = program.func(func).params()[i];
        let ty = program.func(func).dfg().value(value).ty().clone();
        let func_data = program.func_mut(func);
        let alloc = func_data.dfg_mut().new_value().alloc(ty.clone());
        let mut param_id = params[i].0.clone().unwrap();
        param_id.remove(0);
        func_data
            .dfg_mut()
            .set_value_name(alloc, Some("%".to_string() + &*param_id));
        match ty.kind() {
            TypeKind::Pointer(_) => {
                ctx.add_var(param_id.clone(), alloc, true);
            }
            _ => {
                ctx.add_var(param_id.clone(), alloc, false);
            }
        }
        func_data
            .layout_mut()
            .bb_mut(ctx.get_basic_block().unwrap())
            .insts_mut()
            .extend(vec![alloc]);
        let store = func_data.dfg_mut().new_value().store(value, alloc);
        func_data
            .layout_mut()
            .bb_mut(ctx.get_basic_block().unwrap())
            .insts_mut()
            .extend(vec![store]);
    }

    generate_block(&func_def.body, program, func, ctx);

    let func_data = program.func_mut(func);
    let zero = func_data.dfg_mut().new_value().integer(0);

    let mut bbs = vec![];
    for (basic_block, _) in func_data.layout().bbs().iter() {
        let bb = func_data.layout().bbs().node(basic_block).unwrap();
        let insts = bb.insts();
        if insts.len() == 0 {
            bbs.push(*basic_block);
        }
    }
    match func_def.ret_type {
        FuncType::Void => {
            let r = func_data.dfg_mut().new_value().ret(None);
            for b in bbs {
                func_data
                    .layout_mut()
                    .bb_mut(b)
                    .insts_mut()
                    .push_key_back(r)
                    .unwrap();
            }
        }
        FuncType::Int => {
            let r = func_data.dfg_mut().new_value().ret(Option::from(zero));
            for b in bbs {
                func_data
                    .layout_mut()
                    .bb_mut(b)
                    .insts_mut()
                    .push_key_back(r)
                    .unwrap();
            }
        }
    }

    //处理void函数的返回指令缺失
    let mut bbs = vec![];
    for (basic_block, _) in func_data.layout().bbs().iter() {
        let bb = func_data.layout().bbs().node(basic_block).unwrap();
        let last_inst = bb.insts().back_key().unwrap();
        let last_inst_type = func_data.dfg().value(*last_inst).kind();
        match last_inst_type {
            ValueKind::Jump(_) => {}
            ValueKind::Branch(_) => {}
            ValueKind::Return(_) => {}
            _ => {
                bbs.push(*basic_block);
            }
        }
    }

    let r = func_data.dfg_mut().new_value().ret(None);
    for b in bbs {
        func_data
            .layout_mut()
            .bb_mut(b)
            .insts_mut()
            .push_key_back(r)
            .unwrap();
    }
    ctx.out_block();
}

fn generate_params(params: &FuncFParams, ctx: &mut IrCtx) -> Vec<(Option<String>, Type)> {
    let mut param_list = Vec::new();
    for param in params.params.iter() {
        match param {
            FuncFParam::NormalParam(param) => {
                let ty = Type::get_i32();
                param_list.push((Some("@".to_string() + &*param.id.clone()), ty));
            }
            FuncFParam::ArrayParam(param_array) => {
                let mut shape = Vec::new();
                for exp in param_array.shape.iter() {
                    let value = exp.calculate(ctx);
                    shape.push(value);
                }
                let mut array_type = Type::get_i32();
                for s in shape.iter().rev() {
                    array_type = Type::get_array(array_type, *s as usize);
                }
                let ty = Type::get_pointer(array_type);
                param_list.push((Some("@".to_string() + &*param_array.id.clone()), ty));
            }
        }
    }
    param_list
}

fn generate_block(block: &Block, program: &mut Program, func: Function, ctx: &mut IrCtx) {
    ctx.into_block();
    program.func_mut(func);
    for item in block.items.iter() {
        match item {
            BlockItem::Decl(decl) => {
                generate_decl(decl, program, ctx, func);
            }
            BlockItem::Stmt(stmt) => {
                generate_stmt(stmt, program, func, ctx);
            }
        }
    }
    ctx.out_block();
}

fn generate_stmt(stmt: &Stmt, program: &mut Program, func: Function, ctx: &mut IrCtx) {
    match stmt {
        Stmt::Return(ret) => {
            let ret_value = if let Some(exp) = &ret.exp {
                let r = generate_exp(exp, program, func, ctx).unwrap();
                Some(r)
            } else {
                None
            };
            // 创建返回指令，将 ret_value 作为返回值传递，如果为 None 则表示无返回值
            let func_data = program.func_mut(func);
            let ret_inst = func_data.dfg_mut().new_value().ret(ret_value);

            // 返回生成的返回指令
            let func_data = program.func_mut(func);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![ret_inst]);

            ctx.label_num += 1;
            let next = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%next".to_string() + "_" + &ctx.label_num.to_string()));
            func_data.layout_mut().bbs_mut().extend([next]);
            ctx.set_basic_block(next);
        }
        Stmt::Assign(assign) => {
            let r = generate_exp(&assign.rhs, program, func, ctx);
            // let alloc = generate_lval(&assign.lhs, program, func, ctx).unwrap();
            let mut alloc = ctx.find_var(&assign.lhs.id).unwrap();
            let is_ptr = ctx.is_ptr(&assign.lhs.id).unwrap();
            if !assign.lhs.indices.is_empty() {
                for (i, index) in assign.lhs.indices.iter().enumerate() {
                    let r = generate_exp(index, program, func, ctx).unwrap();
                    let func_data = program.func_mut(func);
                    if i == 0 && is_ptr {
                        alloc = func_data.dfg_mut().new_value().load(alloc);
                        func_data
                            .layout_mut()
                            .bb_mut(ctx.get_basic_block().unwrap())
                            .insts_mut()
                            .extend(vec![alloc]);
                        alloc = func_data.dfg_mut().new_value().get_ptr(alloc, r);
                        func_data
                            .layout_mut()
                            .bb_mut(ctx.get_basic_block().unwrap())
                            .insts_mut()
                            .extend(vec![alloc]);
                    } else {
                        alloc = func_data.dfg_mut().new_value().get_elem_ptr(alloc, r);
                        func_data
                            .layout_mut()
                            .bb_mut(ctx.get_basic_block().unwrap())
                            .insts_mut()
                            .extend(vec![alloc]);
                    }
                }
            }
            let func_data = program.func_mut(func);
            let store = func_data.dfg_mut().new_value().store(r.unwrap(), alloc);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![store]);
        }
        Stmt::Exp(exp) => match &exp.exp {
            Some(e) => {
                generate_exp(e, program, func, ctx);
            }
            None => {}
        },
        Stmt::Block(block) => {
            generate_block(block, program, func, ctx);
        }
        Stmt::If(if_) => {
            let cond = &if_.cond.lor;
            let r = generate_lor_exp(cond, program, func, ctx).unwrap();
            let func_data = program.func_mut(func);
            ctx.label_num += 1;
            let then = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%then".to_string() + "_" + &ctx.label_num.to_string()));
            ctx.label_num += 1;
            let else_ = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%else".to_string() + "_" + &ctx.label_num.to_string()));
            ctx.label_num += 1;
            let end = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%end".to_string() + "_" + &ctx.label_num.to_string()));

            let br;

            match &if_.else_ {
                Some(_) => {
                    br = func_data.dfg_mut().new_value().branch(r, then, else_);
                }
                None => {
                    br = func_data.dfg_mut().new_value().branch(r, then, end);
                }
            }

            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![br]);

            func_data.layout_mut().bbs_mut().extend([then]);
            ctx.set_basic_block(then);

            generate_stmt(if_.then.as_ref(), program, func, ctx);

            let func_data = program.func_mut(func);
            let j1 = func_data.dfg_mut().new_value().jump(end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j1]);

            match &if_.else_ {
                Some(stmt) => {
                    let func_data = program.func_mut(func);
                    func_data.layout_mut().bbs_mut().extend([else_]);
                    ctx.set_basic_block(else_);

                    generate_stmt(stmt, program, func, ctx);

                    let func_data = program.func_mut(func);
                    let j2 = func_data.dfg_mut().new_value().jump(end);
                    func_data
                        .layout_mut()
                        .bb_mut(ctx.get_basic_block().unwrap())
                        .insts_mut()
                        .extend(vec![j2]);
                }
                None => {}
            }
            let func_data = program.func_mut(func);
            func_data.layout_mut().bbs_mut().extend([end]);
            ctx.set_basic_block(end);
        }
        Stmt::While(while_) => {
            let func_data = program.func_mut(func);
            ctx.label_num += 1;
            let while_entry = func_data.dfg_mut().new_bb().basic_block(Some(
                "%while_entry".to_string() + "_" + &ctx.label_num.to_string(),
            ));
            ctx.label_num += 1;
            let while_body = func_data.dfg_mut().new_bb().basic_block(Some(
                "%while_body".to_string() + "_" + &ctx.label_num.to_string(),
            ));
            ctx.label_num += 1;
            let end = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%end".to_string() + "_" + &ctx.label_num.to_string()));
            let j_while_entry = func_data.dfg_mut().new_value().jump(while_entry);

            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j_while_entry]);

            func_data.layout_mut().bbs_mut().extend([while_entry]);
            ctx.set_basic_block(while_entry);

            let cond = &while_.cond.lor;
            let r = generate_lor_exp(cond, program, func, ctx).unwrap();
            let func_data = program.func_mut(func);
            let br = func_data.dfg_mut().new_value().branch(r, while_body, end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![br]);

            func_data.layout_mut().bbs_mut().extend([while_body]);
            ctx.set_basic_block(while_body);
            ctx.loop_info.push(LoopInfo {
                entry: while_entry,
                end: end,
            });
            generate_stmt(&while_.body, program, func, ctx);
            ctx.loop_info.pop();

            let func_data = program.func_mut(func);
            let j = func_data.dfg_mut().new_value().jump(while_entry);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j]);
            // }

            let func_data = program.func_mut(func);
            func_data.layout_mut().bbs_mut().extend([end]);
            ctx.set_basic_block(end);
        }
        Stmt::Continue => {
            let loop_info = ctx.loop_info.last().unwrap_or_else(|| exit(5));
            let func_data = program.func_mut(func);
            let j = func_data.dfg_mut().new_value().jump(loop_info.entry);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j]);

            ctx.label_num += 1;
            let while_body_ = func_data.dfg_mut().new_bb().basic_block(Some(
                "%while_body".to_string() + "_" + &ctx.label_num.to_string(),
            ));
            func_data.layout_mut().bbs_mut().extend([while_body_]);
            ctx.set_basic_block(while_body_);
        }
        Stmt::Break => {
            let loop_info = ctx.loop_info.last().unwrap_or_else(|| exit(5));
            let func_data = program.func_mut(func);
            let j = func_data.dfg_mut().new_value().jump(loop_info.end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j]);

            ctx.label_num += 1;
            let while_body_ = func_data.dfg_mut().new_bb().basic_block(Some(
                "%while_body".to_string() + "_" + &ctx.label_num.to_string(),
            ));
            func_data.layout_mut().bbs_mut().extend([while_body_]);
            ctx.set_basic_block(while_body_);
        }
    };
}

fn generate_exp(
    exp: &Exp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    let r = generate_lor_exp(&exp.exp, program, func, ctx);
    r
}

fn generate_lor_exp(
    lor_exp: &LOrExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match lor_exp {
        LOrExp::LAndExp(land_exp) => {
            let r = generate_land_exp(land_exp, program, func, ctx);
            r
        }
        LOrExp::Or(lhs, rhs) => {
            //短路求值
            //@result = alloc i32
            let func_data = program.func_mut(func);
            let result = func_data.dfg_mut().new_value().alloc(Type::get_i32());
            func_data.dfg_mut().set_value_name(
                result,
                Some("@".to_string() + "result___" + &ctx.result_num.to_string()),
            );
            ctx.add_var(
                "result___".to_string() + &ctx.result_num.to_string(),
                result,
                false,
            );
            ctx.result_num += 1;
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            //store 1, @result
            let func_data = program.func_mut(func);
            let one = func_data.dfg_mut().new_value().integer(1);
            let store = func_data.dfg_mut().new_value().store(one, result);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![store]);

            //%0 = load @lhs
            let r_l = generate_lor_exp(lhs, program, func, ctx);
            let lhs_value = r_l.unwrap();
            //%1 = eq %0, 0
            let func_data = program.func_mut(func);
            let zero = func_data.dfg_mut().new_value().integer(0);
            let lhs_eq_0 =
                func_data
                    .dfg_mut()
                    .new_value()
                    .binary(koopa::ir::BinaryOp::Eq, lhs_value, zero);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![lhs_eq_0]);
            //br %1, %then, %end
            ctx.label_num += 1;
            let then = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%then".to_string() + "_" + &ctx.label_num.to_string()));
            ctx.label_num += 1;
            let end = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%end".to_string() + "_" + &ctx.label_num.to_string()));
            let br = func_data.dfg_mut().new_value().branch(lhs_eq_0, then, end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![br]);
            //%then_1:
            func_data.layout_mut().bbs_mut().extend([then]);
            ctx.set_basic_block(then);
            //%2 = load @rhs
            let r_r = generate_land_exp(rhs, program, func, ctx);
            let rhs_value = r_r.unwrap();
            //%3 = ne %2, 0
            let func_data = program.func_mut(func);
            let rhs_ne_0 =
                func_data
                    .dfg_mut()
                    .new_value()
                    .binary(koopa::ir::BinaryOp::NotEq, rhs_value, zero);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![rhs_ne_0]);
            //store %3, @result
            let store = func_data.dfg_mut().new_value().store(rhs_ne_0, result);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![store]);
            //jump %end
            let j = func_data.dfg_mut().new_value().jump(end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j]);
            //%end:
            func_data.layout_mut().bbs_mut().extend([end]);
            ctx.set_basic_block(end);
            //%4 = load @result
            let result = func_data.dfg_mut().new_value().load(result);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);

            Some(result)
        }
    }
}

fn generate_land_exp(
    land_exp: &LAndExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match land_exp {
        LAndExp::EqExp(eq_exp) => {
            let r = generate_eq_exp(eq_exp, program, func, ctx);
            r
        }
        LAndExp::And(lhs, rhs) => {
            let func_data = program.func_mut(func);
            let result = func_data.dfg_mut().new_value().alloc(Type::get_i32());
            func_data.dfg_mut().set_value_name(
                result,
                Some("@".to_string() + "result___" + &ctx.result_num.to_string()),
            );
            ctx.add_var(
                "result___".to_string() + &ctx.result_num.to_string(),
                result,
                false,
            );
            ctx.result_num += 1;
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            //store 0, @result
            let func_data = program.func_mut(func);
            let zero = func_data.dfg_mut().new_value().integer(0);
            let store = func_data.dfg_mut().new_value().store(zero, result);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![store]);

            //%0 = load @lhs
            let r_l = generate_land_exp(lhs, program, func, ctx);
            let lhs_value = r_l.unwrap();
            //%1 = ne %0, 0
            let func_data = program.func_mut(func);
            let zero = func_data.dfg_mut().new_value().integer(0);
            let lhs_neq_0 =
                func_data
                    .dfg_mut()
                    .new_value()
                    .binary(koopa::ir::BinaryOp::NotEq, lhs_value, zero);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![lhs_neq_0]);
            //br %1, %then, %end
            ctx.label_num += 1;
            let then = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%then".to_string() + "_" + &ctx.label_num.to_string()));
            ctx.label_num += 1;
            let end = func_data
                .dfg_mut()
                .new_bb()
                .basic_block(Some("%end".to_string() + "_" + &ctx.label_num.to_string()));
            let br = func_data.dfg_mut().new_value().branch(lhs_neq_0, then, end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![br]);
            //%then_1:
            func_data.layout_mut().bbs_mut().extend([then]);
            ctx.set_basic_block(then);
            //%2 = load @rhs
            let r_r = generate_eq_exp(rhs, program, func, ctx);
            let rhs_value = r_r.unwrap();
            //%3 = ne %2, 0
            let func_data = program.func_mut(func);
            let rhs_ne_0 =
                func_data
                    .dfg_mut()
                    .new_value()
                    .binary(koopa::ir::BinaryOp::NotEq, rhs_value, zero);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![rhs_ne_0]);
            //store %3, @result
            let store = func_data.dfg_mut().new_value().store(rhs_ne_0, result);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![store]);
            //jump %end
            let j = func_data.dfg_mut().new_value().jump(end);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![j]);
            //%end:
            func_data.layout_mut().bbs_mut().extend([end]);
            ctx.set_basic_block(end);
            //%4 = load @result
            let result = func_data.dfg_mut().new_value().load(result);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);

            Some(result)
        }
    }
}

fn generate_eq_exp(
    eq_exp: &EqExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match eq_exp {
        EqExp::RelExp(rel_exp) => {
            let r = generate_rel_exp(rel_exp, program, func, ctx);
            r
        }
        EqExp::Eq(lhs, op, rhs) => {
            // 递归生成左操作数和右操作数的 IR 值
            let r_l = generate_eq_exp(lhs, program, func, ctx).unwrap();
            let lhs_value = r_l;
            let r_r = generate_rel_exp(rhs, program, func, ctx).unwrap();
            let rhs_value = r_r;

            // 生成等于运算指令
            let func_data = program.func_mut(func);
            let result = match op {
                EqOp::Eq => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Eq,
                    lhs_value,
                    rhs_value,
                ),
                EqOp::Neq => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::NotEq,
                    lhs_value,
                    rhs_value,
                ),
            };
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            Some(result)
        }
    }
}

fn generate_rel_exp(
    rel_exp: &RelExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match rel_exp {
        RelExp::AddExp(add_exp) => {
            let r = generate_add_exp(add_exp, program, func, ctx);
            r
        }
        RelExp::Rel(lhs, op, rhs) => {
            // 递归生成左操作数和右操作数的 IR 值
            let r_l = generate_rel_exp(lhs, program, func, ctx);
            let lhs_value = r_l.unwrap();
            let r_r = generate_add_exp(rhs, program, func, ctx);
            let rhs_value = r_r.unwrap();

            // 生成关系运算指令
            let func_data = program.func_mut(func);
            let result = match op {
                RelOp::Lt => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Lt,
                    lhs_value,
                    rhs_value,
                ),
                RelOp::Le => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Le,
                    lhs_value,
                    rhs_value,
                ),
                RelOp::Gt => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Gt,
                    lhs_value,
                    rhs_value,
                ),
                RelOp::Ge => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Ge,
                    lhs_value,
                    rhs_value,
                ),
            };
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            Some(result)
        }
    }
}

fn generate_add_exp(
    add_exp: &AddExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match add_exp {
        AddExp::MulExp(mul_exp) => {
            let r = generate_mul_exp(mul_exp, program, func, ctx);
            r
        }
        AddExp::AddMul(lhs, op, rhs) => {
            // 递归生成左操作数和右操作数的 IR 值
            let r_l = generate_add_exp(lhs, program, func, ctx);
            let lhs_value = r_l.unwrap();
            let r_r = generate_mul_exp(rhs, program, func, ctx);
            let rhs_value = r_r.unwrap();

            // 生成加法运算指令
            let func_data = program.func_mut(func);
            let result = match op {
                AddOp::Add => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Add,
                    lhs_value,
                    rhs_value,
                ),
                AddOp::Sub => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Sub,
                    lhs_value,
                    rhs_value,
                ),
            };
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            Some(result)
        }
    }
}

fn generate_mul_exp(
    mul_exp: &MulExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match mul_exp {
        MulExp::UnaryExp(unary_exp) => {
            let r = generate_unary_exp(unary_exp, program, func, ctx);
            r
        }
        MulExp::MulUnary(lhs, op, rhs) => {
            // 递归生成左操作数和右操作数的 IR 值
            let r_l = generate_mul_exp(lhs, program, func, ctx);
            let lhs_value = r_l.unwrap();
            let r_r = generate_unary_exp(rhs, program, func, ctx);
            let rhs_value = r_r.unwrap();

            // 生成乘法运算指令
            let func_data = program.func_mut(func);
            let result = match op {
                MulOp::Mul => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Mul,
                    lhs_value,
                    rhs_value,
                ),
                MulOp::Div => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Div,
                    lhs_value,
                    rhs_value,
                ),
                MulOp::Mod => func_data.dfg_mut().new_value().binary(
                    koopa::ir::BinaryOp::Mod,
                    lhs_value,
                    rhs_value,
                ),
            };
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            Some(result)
        }
    }
}

fn generate_unary_exp(
    unary_exp: &UnaryExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match unary_exp {
        UnaryExp::PrimaryExp(primary_exp) => {
            let r = generate_primary_exp(primary_exp, program, func, ctx);
            r
        }
        UnaryExp::Unary(op, exp) => {
            let r = generate_unary_exp(exp, program, func, ctx);
            let exp_value = r.unwrap();
            let func_data = program.func_mut(func);
            let result = match op {
                UnaryOp::Pos => exp_value,
                UnaryOp::Neg => {
                    let zero = func_data.dfg_mut().new_value().integer(0);
                    func_data.dfg_mut().new_value().binary(
                        koopa::ir::BinaryOp::Sub,
                        zero,
                        exp_value,
                    )
                }
                UnaryOp::Not => {
                    let zero = func_data.dfg_mut().new_value().integer(0);
                    func_data
                        .dfg_mut()
                        .new_value()
                        .binary(koopa::ir::BinaryOp::Eq, zero, exp_value)
                }
            };
            match func_data.dfg().value(result).kind() {
                ValueKind::Integer(i) => {}
                _ => {
                    func_data
                        .layout_mut()
                        .bb_mut(ctx.get_basic_block().unwrap())
                        .insts_mut()
                        .extend(vec![result]);
                }
            }
            Some(result)
        }
        UnaryExp::Call(func_call) => {
            let func_id = func_call.id.clone();
            let args_exp = func_call.args.clone();
            let mut args = Vec::new();
            for exp in args_exp {
                let value = generate_exp(&exp, program, func, ctx).unwrap();
                args.push(value);
            }
            let mut callee: Option<Function> = None;
            for (func, funcdata) in program.funcs() {
                if funcdata.name() == "@".to_string() + &*func_id {
                    callee = Some(*func)
                }
            }
            let func_data = program.func_mut(func);
            let call = func_data.dfg_mut().new_value().call(callee.unwrap(), args);
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![call]);
            let is_void = program.func(callee.unwrap()).ty().is_unit();
            if is_void {
                None
            } else {
                Some(call)
            }
        }
    }
}

fn generate_primary_exp(
    primary_exp: &PrimaryExp,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    match primary_exp {
        PrimaryExp::Exp(exp) => {
            let r = generate_exp(exp, program, func, ctx);
            r
        }
        PrimaryExp::LVal(lval) => {
            let r = generate_lval(lval, program, func, ctx);
            r
        }
        PrimaryExp::IntConst(val) => {
            let func_data = program.func_mut(func);
            let result = func_data.dfg_mut().new_value().integer(*val);
            Some(result)
        }
    }
}

fn generate_lval(
    lval: &LVal,
    program: &mut Program,
    func: Function,
    ctx: &mut IrCtx,
) -> Option<Value> {
    let func_data = program.func_mut(func);
    let id = lval.id.clone();
    let alloc = ctx.find_var(&id);
    let is_ptr = ctx.is_ptr(&id);
    if let Some(alloc) = alloc {
        if lval.indices.is_empty() {
            let if_local = func_data.dfg().values().contains_key(&alloc);
            let value_data = if if_local {
                func_data.dfg().value(alloc).clone()
            } else {
                program.borrow_value(alloc).clone()
            };
            let func_data = program.func_mut(func);
            let mut result;
            match value_data.ty().kind() {
                TypeKind::Pointer(ty) => {
                    if ty.is_i32() {
                        result = func_data.dfg_mut().new_value().load(alloc);
                    } else {
                        if is_ptr.unwrap() {
                            result = func_data.dfg_mut().new_value().load(alloc);
                        } else {
                            let zero = func_data.dfg_mut().new_value().integer(0);
                            result = func_data.dfg_mut().new_value().get_elem_ptr(alloc, zero);
                        }
                    }
                }
                _ => {
                    result = func_data.dfg_mut().new_value().load(alloc);
                }
            }
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            Some(result)
        } else {
            let mut base = alloc;
            for (i, index) in lval.indices.iter().enumerate() {
                let r = generate_exp(index, program, func, ctx).unwrap();
                let func_data = program.func_mut(func);
                if i == 0 && is_ptr.unwrap() {
                    base = func_data.dfg_mut().new_value().load(base);
                    func_data
                        .layout_mut()
                        .bb_mut(ctx.get_basic_block().unwrap())
                        .insts_mut()
                        .extend(vec![base]);
                    base = func_data.dfg_mut().new_value().get_ptr(base, r);
                } else {
                    base = func_data.dfg_mut().new_value().get_elem_ptr(base, r);
                }
                func_data
                    .layout_mut()
                    .bb_mut(ctx.get_basic_block().unwrap())
                    .insts_mut()
                    .extend(vec![base]);
            }
            let func_data = program.func_mut(func);
            let value_data = func_data.dfg().value(base);
            let mut result;
            match value_data.ty().kind() {
                TypeKind::Pointer(ty) => {
                    if ty.is_i32() {
                        result = func_data.dfg_mut().new_value().load(base);
                    } else {
                        let zero = func_data.dfg_mut().new_value().integer(0);
                        result = func_data.dfg_mut().new_value().get_elem_ptr(base, zero);
                    }
                }
                _ => {
                    result = func_data.dfg_mut().new_value().load(base);
                }
            }
            func_data
                .layout_mut()
                .bb_mut(ctx.get_basic_block().unwrap())
                .insts_mut()
                .extend(vec![result]);
            Some(result)
        }
    } else {
        let result_value = ctx.find_const(&id).unwrap_or_else(|| {
            println!("Symbol not found: {}", id);
            exit(4)
        });
        let result = func_data.dfg_mut().new_value().integer(result_value);
        Some(result)
    }
}
