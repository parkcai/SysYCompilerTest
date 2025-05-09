use koopa::ir::*;
use koopa::front::driver::Driver;
use std::collections::HashMap;
use std::fmt::Write;
use koopa::ir::values::Aggregate;


pub fn generate_riscv(koopa_ir: &str) -> String {
    Type::set_ptr_size(4);
    let driver = Driver::from(koopa_ir);
    let program = driver.generate_program().unwrap();

    let mut riscv_code = String::new();

     // 处理全局变量和常量
     writeln!(riscv_code, "  .data").unwrap();

     fn handle_aggregate(
        riscv_code: &mut String,
        aggregate: &Aggregate,
        program: &Program,
    ) {
        for value in aggregate.elems() {
            let elem_value = program.borrow_value(*value);
            match elem_value.kind() {
                ValueKind::Integer(int) => {
                    // 单个整数初始化
                    writeln!(riscv_code, "  .word {}", int.value()).unwrap();
                }
                ValueKind::ZeroInit(_) => {
                    // 数组部分为 zeroinit 的情况
                    writeln!(riscv_code, "  .zero {}", elem_value.ty().size()).unwrap();
                }
                ValueKind::Aggregate(inner_aggregate) => {
                    // 递归处理嵌套的 Aggregate
                    handle_aggregate(riscv_code, inner_aggregate, program);
                }
                _ => panic!("Unsupported aggregate element initializer!"),
            }
        }
    }

     for global_value in program.inst_layout() {
        let value_data = program.borrow_value(*global_value);
        let global_name = value_data.name().as_ref().unwrap().replace("@", "");
        writeln!(riscv_code, "  .globl {}", global_name).unwrap();
    
        if let ValueKind::GlobalAlloc(alloc) = value_data.kind() {
            writeln!(riscv_code, "{}:", global_name).unwrap();
            match alloc.init() {
                init_value => match program.borrow_value(init_value).kind() {
                    ValueKind::Integer(int) => {
                        writeln!(riscv_code, "  .word {}", int.value()).unwrap();
                    }
                    ValueKind::Aggregate(aggregate) => {
                        // 处理多维数组 Aggregate 的递归初始化
                        handle_aggregate(&mut riscv_code, aggregate, &program);
                    }
                    ValueKind::ZeroInit(_) => {
                        // 处理全局变量为 zeroinit 的情况
                        if let TypeKind::Pointer(base_type) = value_data.ty().kind() {
                            writeln!(riscv_code, "  .zero {}", base_type.size()).unwrap();
                        }
                        else{
                            writeln!(riscv_code, "  .zero {}", value_data.ty().size()).unwrap();
                        }
                    }
                    _ => panic!("Unsupported global initializer!"),
                },
            }
        }
    }

    for &func in program.func_layout() {
        let func_data = program.func(func);
        // 跳过函数声明
        if func_data.layout().entry_bb().is_none() {
            continue;
        }
        let func_name = func_data.name().replace("@", ""); // 移除 @ 符号

        // 每个函数前生成 .text 和 .globl
        writeln!(riscv_code, "  .text").unwrap();
        writeln!(riscv_code, "  .globl {}", func_name).unwrap();
        writeln!(riscv_code, "{}:", func_name).unwrap();


        // 函数入口 - Prologue
        let mut stack_offset = 0;
        let mut value_offset: HashMap<Value, (usize, bool)> = HashMap::new();
        let mut max_call_args = 0;
        let mut has_call = false;

        // 遍历所有指令，计算栈大小
        for (&bb, bb_data) in func_data.layout().bbs() {
            for &inst in bb_data.insts().keys() {
                let value_data = func_data.dfg().value(inst);
                if let ValueKind::Call(call) = value_data.kind() {
                    has_call = true;
                    let num_args = call.args().len();
                    max_call_args = max_call_args.max(num_args.saturating_sub(8));
                }
            }
        }
        let param_stack_size = max_call_args * 4;

        // **处理形式参数**
        let mut param_offset = 0;
        for (i, &param) in func_data.params().iter().enumerate() {
            let param_size = func_data.dfg().value(param).ty().size(); // 获取参数大小
            value_offset.insert(param, (param_offset+param_stack_size, false));
            param_offset += param_size; // 每个参数占用 4 字节
        }
        stack_offset = param_offset;

        // 遍历所有指令，计算栈大小
        for (&bb, bb_data) in func_data.layout().bbs() {
            for &inst in bb_data.insts().keys() {
                let value_data = func_data.dfg().value(inst);
                if !value_data.ty().is_unit() {
                    let mut value_size = value_data.ty().size();
                     // 如果是 Alloc 指令，计算实际分配的空间大小
                    if let ValueKind::Alloc(_) = value_data.kind() {
                        value_size=4;
                        if let TypeKind::Pointer(base_type) = value_data.ty().kind() {
                            let mut base_type = base_type;
                            while let TypeKind::Array(base_type1, length) = base_type.kind() {
                                base_type = base_type1;
                                value_size *= length;
                            }
                        }
                    }
                     // 判断是否是 GetElemPtr 或 GetPtr 指令
                    let mut is_address = matches!(value_data.kind(), ValueKind::GetElemPtr(_) | ValueKind::GetPtr(_));
                    if let TypeKind::Pointer(base_type) = value_data.ty().kind(){
                        if let TypeKind::Pointer(_) = base_type.kind(){
                            is_address = true;
                        }
                    }
                    // 插入偏移量和是否为地址的标志
                    value_offset.insert(inst, (stack_offset + param_stack_size, is_address));

                    // 更新栈大小
                    stack_offset += value_size; 
                }
            }
        }

        // 栈对齐和计算
        let ra_size = if has_call { 4 } else { 0 };
        let aligned_stack_size = ((stack_offset + ra_size + param_stack_size + 15) / 16) * 16;
        
        // 调整局部变量和保存寄存器的存储偏移

        let mut param_offset1 = 0;
        for (i, &param) in func_data.params().iter().enumerate() {
            if i>=8 {
                // 参数超出 a0-a7 的部分，从调用者的栈帧中传递,所以趁着sp还没被修改，先存入栈
                let arg_stack_offset = (i - 8) * 4;
                let offset_minus_stack = (param_offset1 as i32 - aligned_stack_size as i32+ param_stack_size as i32) as i32;
                if arg_stack_offset<=2047 {
                    writeln!(riscv_code, "  lw t0, {}(sp)", arg_stack_offset).unwrap();
                }
                else{
                    writeln!(riscv_code, "  li t2, {}", arg_stack_offset).unwrap();
                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                    writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                }
                if offset_minus_stack <= 2047 && -offset_minus_stack<=2047 {
                    writeln!(riscv_code, "  sw t0, {}(sp)", offset_minus_stack).unwrap();
                } else {
                    writeln!(riscv_code, "  li t2, {}", offset_minus_stack).unwrap();
                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                    writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                }
            }
            param_offset1 += 4; // 每个参数占用 4 字节
        }

        // 检查 aligned_stack_size 是否超出 addi 指令的立即数范围
        if aligned_stack_size > 0 {
            if aligned_stack_size <= 2047 {
                writeln!(riscv_code, "  addi sp, sp, -{}", aligned_stack_size).unwrap();
            } else {
                writeln!(riscv_code, "  li t2, -{}", aligned_stack_size).unwrap(); // 加载立即数到 t0
                writeln!(riscv_code, "  add sp, sp, t2").unwrap();                // 更新 sp
            }
        }

        for (i, &param) in func_data.params().iter().enumerate() {
            if i < 8 {
                // 参数在 a0-a7 中
                if value_offset[&param].0<=2047 {
                    writeln!(riscv_code, "  sw a{}, {}(sp)", i, value_offset[&param].0).unwrap();
                }
                else{
                    writeln!(riscv_code, "  li t2, {}", value_offset[&param].0).unwrap();
                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                    writeln!(riscv_code, "  sw a{}, 0(t2)",i).unwrap();
                }
            } 
        }

        // 如果有call,得保存ra
        if has_call {
            if aligned_stack_size <= 2047 {
                writeln!(riscv_code, "  sw ra, {}(sp)", aligned_stack_size - 4).unwrap();
            } else {
                writeln!(riscv_code, "  li t2, {}", aligned_stack_size - 4).unwrap();
                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                writeln!(riscv_code, "  sw ra, 0(t2)").unwrap();
            }
        }

        for (&bb, bb_data) in func_data.layout().bbs() {
            if let Some(bb_name) = func_data.dfg().bb(bb).name() {
                // 跳过生成 entry: 标签
                if bb_name != "%entry" {
                    writeln!(riscv_code, "{}:", bb_name.replace("%", "")).unwrap();
                }
            }

            for &inst in bb_data.insts().keys() {
                let value_data = func_data.dfg().value(inst);
                match value_data.kind() {
                    ValueKind::Call(call) => {
                        let args = call.args();
                        for (i, &arg) in args.iter().enumerate() {
                            let arg_value = func_data.dfg().value(arg);
                            match arg_value.kind() {
                                // 如果参数是立即数
                                ValueKind::Integer(int) => {
                                    if i < 8 {
                                        // 加载立即数到 a0-a7
                                        writeln!(riscv_code, "  li a{}, {}", i, int.value()).unwrap();
                                    } else {
                                        // 加载立即数到 t0，然后存入栈
                                        writeln!(riscv_code, "  li t0, {}", int.value()).unwrap();
                                        if (i-8)*4 <= 2047{
                                            writeln!(riscv_code, "  sw t0, {}(sp)", (i - 8) * 4).unwrap();
                                        }
                                        else{
                                            writeln!(riscv_code, "  li t2, {}", (i-8)*4).unwrap();
                                            writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                            writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                                        }
                                    }
                                }
                                // 如果参数是变量
                                _ => {
                                    if let Some(&offset) = value_offset.get(&arg) {
                                        if i < 8 {
                                            // 参数在 a0-a7 中
                                            if offset.0 <= 2047 {
                                                writeln!(riscv_code, "  lw a{}, {}(sp)", i, offset.0).unwrap();
                                            } else {
                                                writeln!(riscv_code, "  li t2, {}", offset.0).unwrap();
                                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                                writeln!(riscv_code, "  lw a{}, 0(t2)", i).unwrap();
                                            }
                                        } else {
                                            // 参数超出 a0-a7，从调用者栈帧中读取
                                            let stack_offset = (i - 8) * 4;
                                            if offset.0 <= 2047 {
                                                writeln!(riscv_code, "  lw t0, {}(sp)", offset.0).unwrap();
                                            } else {
                                                writeln!(riscv_code, "  li t2, {}", offset.0).unwrap();
                                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                                writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                            }
                                            if stack_offset<=2047 {
                                                writeln!(riscv_code, "  sw t0, {}(sp)", stack_offset).unwrap();
                                            }
                                            else{
                                                writeln!(riscv_code, "  li t2, {}", stack_offset).unwrap();
                                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                                writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                                            }
                                        }
                                    } else {
                                        panic!("Argument value not found in value_offset: {:?}", arg);
                                    }
                                }
                            }
                        }

                        // 调用函数
                        let callee_name = program.func(call.callee()).name().replace("@", "");
                        writeln!(riscv_code, "  call {}", callee_name).unwrap();

                        // 处理返回值
                        if !value_data.ty().is_unit() {
                            let offset = value_offset[&inst];
                            if offset.0 <= 2047 {
                                writeln!(riscv_code, "  sw a0, {}(sp)", offset.0).unwrap();
                            } else {
                                writeln!(riscv_code, "  li t2, {}", offset.0).unwrap();
                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                writeln!(riscv_code, "  sw a0, 0(t2)").unwrap();
                            }
                        }
                    }
                    ValueKind::Integer(int) => {
                        let offset = *value_offset.get(&inst).unwrap();
                        writeln!(riscv_code, "  li t0, {}", int.value()).unwrap();
                        if offset.0 <= 2047 {
                            writeln!(riscv_code, "  sw t0, {}(sp)", offset.0).unwrap();
                        } else {
                            writeln!(riscv_code, "  li t2, {}", offset.0).unwrap();
                            writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                            writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                        }
                    }
                    
                    
                    ValueKind::Load(load) => {
                        let src = load.src();
                    
                        if let Some((src_offset,is_address)) = value_offset.get(&src) {
                            // 如果 src 在 value_offset 中，说明是局部变量
                            let (dst_offset,_) = *value_offset.get(&inst).unwrap();
                            // lw 指令生成
                            let src_name = func_data.dfg().value(src).name().clone();
                            if src_name.is_some(){
                                if src_name.unwrap().starts_with("@"){
                                    if *src_offset <= 2047 {
                                        writeln!(riscv_code, "  lw t0, {}(sp)", src_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", src_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                    }
                                }
                                else{ // %开头，是用户自己定义的
                                    if *src_offset <= 2047 {
                                        writeln!(riscv_code, "  lw t1, {}(sp)", src_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", src_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                    }
                                    writeln!(riscv_code, "  lw t0, 0(t1)").unwrap();
                                }
                            }
                        
                            // 存储加载结果
                            if dst_offset <= 2047 {
                                writeln!(riscv_code, "  sw t0, {}(sp)", dst_offset).unwrap();
                            } else {
                                writeln!(riscv_code, "  li t2, {}", dst_offset).unwrap();
                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                            }
                        } else {
                            // 如果 src 不在 value_offset 中，说明是全局变量
                            /* 
                            let src_value_data = program.inst_layout().iter().find_map(|&global| {
                                let global_data = program.borrow_value(global);
                                if global == src {
                                    Some(global_data)
                                } else {
                                    None
                                }   
                            }).expect("Global variable not found in program.inst_layout()");
                            */
                            if let Some(src_name) = program.borrow_value(src).name() {
                                let global_name = src_name.replace("@", "");
                                writeln!(riscv_code, "  la t0, {}", global_name).unwrap(); // 加载全局变量地址到 t0
                                writeln!(riscv_code, "  lw t1, 0(t0)").unwrap();         // 加载全局变量值到 t1
                                let (dst_offset,_) = *value_offset.get(&inst).unwrap();
                                if dst_offset <= 2047 {
                                    writeln!(riscv_code, "  sw t1, {}(sp)", dst_offset).unwrap();
                                } else {
                                    writeln!(riscv_code, "  li t2, {}", dst_offset).unwrap();
                                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                    writeln!(riscv_code, "  sw t1, 0(t2)").unwrap();
                                }
                            }
                            else{
                                panic!("Global variable not found in program.inst_layout()");
                            }
                        }
                    }

                    ValueKind::Store(store) => {
                        let value = store.value();
                        let dest = store.dest();
                    
                        if let Some((dest_offset,_)) = value_offset.get(&dest) {
                            // 如果 dest 在 value_offset 中，说明是局部变量
                            
                            let value_kind = func_data.dfg().value(value).kind();
                            // 加载值
                            match value_kind {
                                // 如果值是常量
                                ValueKind::Integer(int) => {
                                    writeln!(riscv_code, "  li t0, {}", int.value()).unwrap();
                                }
                                // 如果值是变量
                                _ => {
                                    let (value_offset1,_) = *value_offset.get(&value).unwrap();
                                    if value_offset1 <= 2047 {
                                        writeln!(riscv_code, "  lw t0, {}(sp)", value_offset1).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", value_offset1).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                    }
                                }
                            }
                            // 存储值
                            let dest_name = func_data.dfg().value(dest).name().clone();
                            if dest_name.is_some() {
                                if dest_name.unwrap().starts_with("@"){
                                    if *dest_offset <= 2047{
                                        writeln!(riscv_code, "  addi t1, sp, {}", dest_offset).unwrap();
                                    }
                                    else{
                                        writeln!(riscv_code, "  li t2, {}", dest_offset).unwrap();
                                        writeln!(riscv_code, "  add t1, sp, t2").unwrap();
                                    }
                                }
                                else{
                                    if *dest_offset <= 2047 {
                                        writeln!(riscv_code, "  lw t1, {}(sp)", dest_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", dest_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                    }
                                }
                            }
                            // 存储值到目标地址
                            writeln!(riscv_code, "  sw t0, 0(t1)").unwrap();

                        } else {
                            // 如果 dest 不在 value_offset 中，说明是全局变量
                            if let Some(dest_name) = program.borrow_value(dest).name() {

                                let global_name = dest_name.replace("@", "");
                                let value_kind = func_data.dfg().value(value).kind();
                                // 加载值
                                match value_kind {
                                    // 如果值是常量
                                    ValueKind::Integer(int) => {
                                        writeln!(riscv_code, "  li t0, {}", int.value()).unwrap();
                                    }
                                    // 如果值是变量
                                    _ => {
                                        let (value_offset1,_) = *value_offset.get(&value).unwrap();
                                        if value_offset1 <= 2047 {
                                            writeln!(riscv_code, "  lw t0, {}(sp)", value_offset1).unwrap();
                                        } else {
                                            writeln!(riscv_code, "  li t2, {}", value_offset1).unwrap();
                                            writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                            writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                        }
                                    }
                                }
                                // 存储值到全局变量
                                writeln!(riscv_code, "  la t1, {}", global_name).unwrap();
                                writeln!(riscv_code, "  sw t0, 0(t1)").unwrap();

                            }
                            else{
                                panic!("Global variable not found in program.inst_layout()");
                            }
                        }
                    }
                    
                
                    ValueKind::Binary(bin) => {
                        // 检查左操作数
                        let lhs_value = func_data.dfg().value(bin.lhs());
                        match lhs_value.kind() {
                            // 如果左操作数是常数
                            ValueKind::Integer(int) => {
                                writeln!(riscv_code, "  li t0, {}", int.value()).unwrap();
                            }
                            // 如果左操作数是变量
                            _ => {
                                let (lhs_offset,is_address) = *value_offset.get(&bin.lhs()).unwrap();
                                if lhs_offset <= 2047 {
                                    writeln!(riscv_code, "  lw t0, {}(sp)", lhs_offset).unwrap();
                                } else {
                                    writeln!(riscv_code, "  li t2, {}", lhs_offset).unwrap();
                                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                    writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                }
                            }
                        }
                    
                        // 检查右操作数
                        let rhs_value = func_data.dfg().value(bin.rhs());
                        match rhs_value.kind() {
                            // 如果右操作数是常数
                            ValueKind::Integer(int) => {
                                writeln!(riscv_code, "  li t1, {}", int.value()).unwrap();
                            }
                            // 如果右操作数是变量
                            _ => {
                                let (rhs_offset, is_address) = *value_offset.get(&bin.rhs()).unwrap();
                                if rhs_offset <= 2047 {
                                    writeln!(riscv_code, "  lw t1, {}(sp)", rhs_offset).unwrap();
                                } else {
                                    writeln!(riscv_code, "  li t2, {}", rhs_offset).unwrap();
                                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                    writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                }
                            }
                        }
                    
                        // 生成二元运算指令
                        match bin.op() {
                            BinaryOp::Add => writeln!(riscv_code, "  add t0, t0, t1").unwrap(),
                            BinaryOp::Sub => writeln!(riscv_code, "  sub t0, t0, t1").unwrap(),
                            BinaryOp::Mul => writeln!(riscv_code, "  mul t0, t0, t1").unwrap(),
                            BinaryOp::Div => writeln!(riscv_code, "  div t0, t0, t1").unwrap(),
                            BinaryOp::Mod => writeln!(riscv_code, "  rem t0, t0, t1").unwrap(),
                            BinaryOp::And => writeln!(riscv_code, "  and t0, t0, t1").unwrap(),
                            BinaryOp::Or => writeln!(riscv_code, "  or t0, t0, t1").unwrap(),
                            BinaryOp::Eq => {
                                writeln!(riscv_code, "  sub t0, t0, t1").unwrap();
                                writeln!(riscv_code, "  seqz t0, t0").unwrap();
                            }
                            BinaryOp::NotEq => {
                                writeln!(riscv_code, "  sub t0, t0, t1").unwrap();
                                writeln!(riscv_code, "  snez t0, t0").unwrap();
                            }
                            BinaryOp::Lt => writeln!(riscv_code, "  slt t0, t0, t1").unwrap(),
                            BinaryOp::Gt => writeln!(riscv_code, "  sgt t0, t0, t1").unwrap(),
                            BinaryOp::Le => {
                                writeln!(riscv_code, "  sgt t0, t0, t1").unwrap();
                                writeln!(riscv_code, "  seqz t0, t0").unwrap();
                            }
                            BinaryOp::Ge => {
                                writeln!(riscv_code, "  slt t0, t0, t1").unwrap();
                                writeln!(riscv_code, "  seqz t0, t0").unwrap();
                            }
                            _ => unimplemented!(),
                        }
                    
                        // 存储结果
                        let (result_offset,_) = *value_offset.get(&inst).unwrap();
                        if result_offset <= 2047 {
                            writeln!(riscv_code, "  sw t0, {}(sp)", result_offset).unwrap();
                        } else {
                            writeln!(riscv_code, "  li t2, {}", result_offset).unwrap();
                            writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                            writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                        }
                    }
                    
                    ValueKind::Branch(branch) => {
                        let cond = branch.cond();
                        let cond_value = func_data.dfg().value(cond);

                        // 判断条件是常量还是变量
                        match cond_value.kind() {
                            // 如果是常量值
                            ValueKind::Integer(int) => {
                                let const_value = int.value();
                                writeln!(riscv_code, "  li t0, {}", const_value).unwrap(); // 将常量加载到 t0 寄存器
                            }
                            // 如果是变量
                            _ => {
                                let (cond_offset,is_address) = *value_offset.get(&cond).expect("Undefined condition value");
                                if cond_offset <= 2047 {
                                    writeln!(riscv_code, "  lw t0, {}(sp)", cond_offset).unwrap(); // 从栈中加载变量值到 t0
                                } else {
                                    writeln!(riscv_code, "  li t2, {}", cond_offset).unwrap();
                                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                    writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                }
                            }
                        }
                        let true_bb = branch.true_bb();
                        let false_bb = branch.false_bb();
                        let true_label = func_data.dfg().bb(true_bb).name().as_ref().unwrap() as &str;
                        let true_label = true_label.replace("%", "");
                        let false_label = func_data.dfg().bb(false_bb).name().as_ref().unwrap() as &str;
                        let false_label = false_label.replace("%", "");
                        //writeln!(riscv_code, "  lw t0, {}(sp)", cond_offset).unwrap();
                        // 创建一个中间块名
                        let near_label = format!("{}_near", true_label);

                        // 先跳转到中间块
                        writeln!(riscv_code, "  bnez t0, {}", near_label).unwrap();

                        // 跳转到 false_label
                        writeln!(riscv_code, "  j {}", false_label).unwrap();
                        // 插入空行
                        writeln!(riscv_code, "").unwrap();

                        // 定义中间块 near_label
                        writeln!(riscv_code, "{}:", near_label).unwrap();

                        // 在中间块跳转到真正的 true_label
                        writeln!(riscv_code, "  j {}", true_label).unwrap();
                    }
                    ValueKind::Jump(jump) => {
                        let target_bb = jump.target();
                        let target_label = func_data.dfg().bb(target_bb).name().as_ref().unwrap() as &str;
                        let target_label = target_label.replace("%", "");
                        writeln!(riscv_code, "  j {}", target_label).unwrap();
                    }
                    ValueKind::GetElemPtr(getelemptr) => {
                        let base = getelemptr.src();
                        let index = getelemptr.index();
                        //看看是不是局部变量
                        if let Some((base_offset,is_address)) = value_offset.get(&base) {
                            // 加载基地址
                            let base_name = func_data.dfg().value(base).name().clone();
                            if base_name.is_some(){
                                if base_name.unwrap().starts_with("%"){
                                    if *base_offset <= 2047{
                                        writeln!(riscv_code, "  lw t0, {}(sp)", base_offset).unwrap();
                                    }
                                    else{
                                        writeln!(riscv_code, "  li t2, {}", base_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                    }
                                }
                                else{
                                    if *base_offset <= 2047 {
                                        writeln!(riscv_code, "  addi t0, sp, {}", base_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", base_offset).unwrap();
                                        writeln!(riscv_code, "  add t0, sp, t2").unwrap();
                                    }
                                }
                            }
                            // 计算偏移量
                            let index_value = func_data.dfg().value(index);
                            let mut elem_size = func_data.dfg().value(base).ty().size();
                            if let TypeKind::Pointer(base_type) =func_data.dfg().value(inst).ty().kind() {
                                elem_size = base_type.size();
                            }
                            match index_value.kind() {
                                ValueKind::Integer(int) => {
                                    let offset = int.value() as i32;
                                    writeln!(riscv_code, "  li t1, {}", offset).unwrap();
                                    writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                    writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                    writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                    
                                }
                                _ => {
                                    let (index_offset,is_address) = *value_offset.get(&index).unwrap();
                                    if index_offset <= 2047 {
                                        writeln!(riscv_code, "  lw t1, {}(sp)", index_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", index_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                    }
                                    writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                    writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                    writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                }
                            }
                        
                            // 保存结果
                            let (result_offset,_) = *value_offset.get(&inst).unwrap();
                            if result_offset <= 2047 {
                                writeln!(riscv_code, "  sw t0, {}(sp)", result_offset).unwrap();
                            } else {
                                writeln!(riscv_code, "  li t2, {}", result_offset).unwrap();
                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                            }
                        }
                        else {
                            // 如果是全局变量
                            if let Some(src_name) = program.borrow_value(base).name() {
                                
                                let global_name = src_name.replace("@", "");
                                writeln!(riscv_code, "  la t0, {}", global_name).unwrap(); // 加载全局变量地址到 t0
                                //writeln!(riscv_code, "  lw t1, 0(t0)").unwrap();         // 加载全局变量值到 t1
                            
                                // 计算偏移量
                                let index_value = func_data.dfg().value(index);
                                let mut elem_size=0;
                                if let TypeKind::Pointer(pointer_type) =func_data.dfg().value(inst).ty().kind() {
                                    elem_size = pointer_type.size();
                                }
                                match index_value.kind() {
                                    ValueKind::Integer(int) => {
                                        let offset = int.value()  as i32;
                                        writeln!(riscv_code, "  li t1, {}", offset).unwrap();
                                        writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                        writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                        writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                        
                                    }
                                    _ => {
                                        let (index_offset,is_address) = *value_offset.get(&index).unwrap();
                                        if index_offset <= 2047 {
                                            writeln!(riscv_code, "  lw t1, {}(sp)", index_offset).unwrap();
                                        } else {
                                            writeln!(riscv_code, "  li t2, {}", index_offset).unwrap();
                                            writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                            writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                        }
                                        writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                        writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                        writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                    }
                                }
                            
                                // 保存结果
                                let (result_offset,_) = *value_offset.get(&inst).unwrap();
                                if result_offset <= 2047 {
                                    writeln!(riscv_code, "  sw t0, {}(sp)", result_offset).unwrap();
                                } else {
                                    writeln!(riscv_code, "  li t2, {}", result_offset).unwrap();
                                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                    writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                                }
                                
                            }
                            else{
                                panic!("Global array not found in program.inst_layout()");
                            }
                            
                        }
                    }
                    
                    ValueKind::GetPtr(getptr) => {
                        let base = getptr.src();
                        let index = getptr.index();
                        //看看是不是局部变量
                        if let Some((base_offset,is_address)) = value_offset.get(&base) {
                            // 加载基地址
                            let base_name = func_data.dfg().value(base).name().clone();
                            if base_name.is_some(){
                                if base_name.unwrap().starts_with("%"){
                                    if *base_offset <= 2047{
                                        writeln!(riscv_code, "  lw t0, {}(sp)", base_offset).unwrap();
                                    }
                                    else{
                                        writeln!(riscv_code, "  li t2, {}", base_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t0, 0(t2)").unwrap();
                                    }
                                }
                                else{
                                    if *base_offset <= 2047 {
                                        writeln!(riscv_code, "  addi t0, sp, {}", base_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", base_offset).unwrap();
                                        writeln!(riscv_code, "  add t0, sp, t2").unwrap();
                                    }
                                }
                            }
                            // 计算偏移量
                            let index_value = func_data.dfg().value(index);
                            //let elem_size = func_data.dfg().value(base).ty().elems().unwrap().size();
                            let mut elem_size = func_data.dfg().value(base).ty().size();
                            if let TypeKind::Pointer(base_type) =func_data.dfg().value(inst).ty().kind() {
                                elem_size = base_type.size();
                            }
                            match index_value.kind() {
                                ValueKind::Integer(int) => {
                                    let offset = int.value() as i32;
                                    writeln!(riscv_code, "  li t1, {}", offset).unwrap();
                                    writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                    writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                    writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                    
                                }
                                _ => {
                                    let (index_offset,is_address) = *value_offset.get(&index).unwrap();
                                    if index_offset <= 2047 {
                                        writeln!(riscv_code, "  lw t1, {}(sp)", index_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", index_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                    }
                                    writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                    writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                    writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                }
                            }
                        
                            // 保存结果
                            let (result_offset,_) = *value_offset.get(&inst).unwrap();
                            if result_offset <= 2047 {
                                writeln!(riscv_code, "  sw t0, {}(sp)", result_offset).unwrap();
                            } else {
                                writeln!(riscv_code, "  li t2, {}", result_offset).unwrap();
                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                            }
                        }
                        else {
                            // 如果是全局变量
                            if let Some(src_name) = program.borrow_value(base).name() {
                                
                                let global_name = src_name.replace("@", "");
                                writeln!(riscv_code, "  la t0, {}", global_name).unwrap(); // 加载全局变量地址到 t0
                                //writeln!(riscv_code, "  lw t1, 0(t0)").unwrap();         // 加载全局变量值到 t1
                            
                                // 计算偏移量
                                let index_value = func_data.dfg().value(index);
                                let mut elem_size=0;
                                if let TypeKind::Pointer(pointer_type) =func_data.dfg().value(inst).ty().kind() {
                                    elem_size = pointer_type.size();
                                }
                                match index_value.kind() {
                                    ValueKind::Integer(int) => {
                                        let offset = int.value()  as i32;
                                        writeln!(riscv_code, "  li t1, {}", offset).unwrap();
                                        writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                        writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                        writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                        
                                    }
                                    _ => {
                                        let (index_offset,is_address) = *value_offset.get(&index).unwrap();
                                        if index_offset <= 2047 {
                                            writeln!(riscv_code, "  lw t1, {}(sp)", index_offset).unwrap();
                                        } else {
                                            writeln!(riscv_code, "  li t2, {}", index_offset).unwrap();
                                            writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                            writeln!(riscv_code, "  lw t1, 0(t2)").unwrap();
                                        }
                                        writeln!(riscv_code, "  li t2, {}", elem_size).unwrap();
                                        writeln!(riscv_code, "  mul t1, t1, t2").unwrap();
                                        writeln!(riscv_code, "  add t0, t0, t1").unwrap();
                                    }
                                }
                            
                                // 保存结果
                                let (result_offset,_) = *value_offset.get(&inst).unwrap();
                                if result_offset <= 2047 {
                                    writeln!(riscv_code, "  sw t0, {}(sp)", result_offset).unwrap();
                                } else {
                                    writeln!(riscv_code, "  li t2, {}", result_offset).unwrap();
                                    writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                    writeln!(riscv_code, "  sw t0, 0(t2)").unwrap();
                                }
                                
                            }
                            else{
                                panic!("Global array not found in program.inst_layout()");
                            }
                            
                        }
                    }

                    ValueKind::Return(ret) => {
                        if let Some(ret_value) = ret.value() {
                            let ret_value_data = func_data.dfg().value(ret_value);
                    
                            match ret_value_data.kind() {
                                // 如果返回值是一个整数常量
                                ValueKind::Integer(int) => {
                                    writeln!(riscv_code, "  li a0, {}", int.value()).unwrap();
                                }
                                // 如果返回值是从栈中加载的值
                                _ => {
                                    let (ret_offset,is_address) = *value_offset.get(&ret_value).unwrap();
                                    if ret_offset <= 2047 {
                                        writeln!(riscv_code, "  lw a0, {}(sp)", ret_offset).unwrap();
                                    } else {
                                        writeln!(riscv_code, "  li t2, {}", ret_offset).unwrap();
                                        writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                        writeln!(riscv_code, "  lw a0, 0(t2)").unwrap();
                                    }
                                }
                            }
                        }
                        if has_call {
                            if aligned_stack_size <= 2047 {
                                writeln!(riscv_code, "  lw ra, {}(sp)", aligned_stack_size - 4).unwrap();
                            } else {
                                writeln!(riscv_code, "  li t2, {}", aligned_stack_size - 4).unwrap();
                                writeln!(riscv_code, "  add t2, sp, t2").unwrap();
                                writeln!(riscv_code, "  lw ra, 0(t2)").unwrap();
                            }
                        }
                        // 函数退出 - Epilogue
                        if aligned_stack_size <= 2047 {
                            writeln!(riscv_code, "  addi sp, sp, {}", aligned_stack_size).unwrap();
                        } else {
                            writeln!(riscv_code, "  li t2, {}", aligned_stack_size).unwrap();
                            writeln!(riscv_code, "  add sp, sp, t2").unwrap();
                        }
                        writeln!(riscv_code, "  ret").unwrap();

                        // 在每个函数结束后添加一个空行
                        writeln!(riscv_code, "").unwrap();
                        break; // 确保一个函数只生成一个 return
                    }
                    
                    _ => {}
                }
            }
        }
    }

    riscv_code
}
