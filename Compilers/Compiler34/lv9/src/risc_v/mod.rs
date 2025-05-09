use koopa::ir::{
    dfg::DataFlowGraph, entities::ValueData, values::Aggregate, BasicBlock, BinaryOp, Function,
    FunctionData, Program, Type, TypeKind, Value, ValueKind,
};
use std::collections::HashMap;

fn load_from_stack(bias: u32, dest: String) -> String {
    if bias > 2000 {
        format!(
            "\tli {}, {bias}\n\tadd {}, {}, sp\n\tlw {}, ({})\n",
            &dest, &dest, &dest, &dest, &dest
        )
    } else {
        format!("\tlw {dest}, {bias}(sp)\n")
    }
}

fn store_to_stack(bias: u32, source: String, helper: String) -> String {
    if bias > 2000 {
        format!(
            "\tli {}, {bias}\n\tadd {}, {}, sp\n\tsw {source}, ({})\n",
            &helper, &helper, &helper, &helper
        )
    } else {
        format!("\tsw {source}, {bias}(sp)\n")
    }
}

struct RiscGlobalData<'a> {
    basic_block_table: HashMap<BasicBlock, String>,
    bnez_step_counter: u32,
    program: &'a Program,
}

impl<'a> RiscGlobalData<'a> {
    fn new(basic_block_table: HashMap<BasicBlock, String>, program: &'a Program) -> RiscGlobalData {
        RiscGlobalData {
            basic_block_table,
            bnez_step_counter: 0,
            program,
        }
    }

    pub fn get_func_name(&self, func: Function) -> String {
        String::from(&self.program.func(func).name()[1..])
    }

    fn request_basic_block(&mut self, basic_block: BasicBlock) -> String {
        match self.basic_block_table.get(&basic_block) {
            Some(res) => res.clone(),
            None => {
                let id = self.basic_block_table.len();
                let res = format!("L{id}");
                self.basic_block_table.insert(basic_block, res.clone());
                res
            }
        }
    }

    fn request_bnez_count(&mut self) -> String {
        let res = self.bnez_step_counter;
        self.bnez_step_counter = res + 1;
        format!("Step{res}")
    }
}

struct RiscStack {
    size: u32,
    value_table: HashMap<Value, u32>,
    sp: u32,
}

impl RiscStack {
    fn new(size: u32, sp: u32) -> Self {
        RiscStack {
            size,
            value_table: HashMap::new(),
            sp,
        }
    }

    pub fn incr_sp(&mut self, incr: u32) {
        self.sp = self.sp + incr;
    }
}

struct RiscFunctionData<'a> {
    dfg: &'a DataFlowGraph,
    stack: RiscStack,
    param_table: HashMap<Value, ParamType>,
    global_value_table: HashMap<Value, ValueData>,
    id: u32,
    call_involved: bool,
}

enum SourceType {
    Instant(i32),
    Stack { bias: u32 },
    Array { bias: u32 },
    Pointer { bias: u32 },
    Reg(u8),
    RegArray(u8),
    Global(String),
}

enum DestType {
    Stack { bias: u32 },
    Global(String),
    Array { bias: u32 },
}

enum ParamType {
    Reg(u8),
    RegArray(u8),
    Stack { bias: u32 },
    Array { bias: u32 },
}

impl<'a> RiscFunctionData<'a> {
    pub fn new(
        global_value_table: HashMap<Value, ValueData>,
        func_data: &FunctionData,
        id: u32,
    ) -> RiscFunctionData {
        let mut size = 0;
        let mut sp = 0;
        let mut call_involved = false;
        let mut max_arg_number = 0;
        for (&_bb, node) in func_data.layout().bbs() {
            for &inst in node.insts().keys() {
                let kind = func_data.dfg().value(inst).kind().clone();
                match kind {
                    ValueKind::Alloc(_) => {
                        if let TypeKind::Pointer(ty) = func_data.dfg().value(inst).ty().kind() {
                            size = size + ty.size() as u32;
                        } else {
                            unreachable!();
                        }
                    }
                    ValueKind::Call(call) => {
                        call_involved = true;
                        max_arg_number = u32::max(max_arg_number, call.args().len() as u32);
                        if !func_data.dfg().value(inst).ty().is_unit() {
                            size = size + 4;
                        }
                    }
                    _ => size = size + func_data.dfg().value(inst).ty().size() as u32,
                }
            }
        }
        if call_involved {
            size = size + 4;
        }
        if max_arg_number > 8 {
            let bias = (max_arg_number - 8) * 4;
            size = size + bias;
            sp = sp + bias;
        }
        if size % 16 != 0 {
            size = (size / 16) * 16 + 16;
        }
        let mut param_table = HashMap::new();
        let mut param_counter = 0;
        for param in func_data.params() {
            if param_counter < 8 {
                match func_data.dfg().value(*param).ty().kind() {
                    TypeKind::Pointer(_) => {
                        param_table.insert(param.clone(), ParamType::RegArray(param_counter as u8))
                    }
                    TypeKind::Int32 => {
                        param_table.insert(param.clone(), ParamType::Reg(param_counter as u8))
                    }
                    _ => unreachable!(),
                };
            } else {
                let bias = (param_counter - 8) * 4 + size;
                match func_data.dfg().value(*param).ty().kind() {
                    TypeKind::Pointer(_) => {
                        param_table.insert(param.clone(), ParamType::Array { bias })
                    }
                    TypeKind::Int32 => param_table.insert(param.clone(), ParamType::Stack { bias }),
                    _ => unreachable!(),
                };
            }
            param_counter = param_counter + 1;
        }
        RiscFunctionData {
            dfg: func_data.dfg(),
            stack: RiscStack::new(size, sp),
            param_table,
            global_value_table,
            id,
            call_involved,
        }
    }

    pub fn generate_prologue(&self) -> String {
        let bias = self.get_stack_size();
        if bias == 0 {
            String::new()
        } else if self.call_involved {
            let sw = store_to_stack(bias - 4, String::from("ra"), String::from("t0"));
            format!("\tli t0, -{bias}\n\tadd sp, sp, t0\n{sw}")
        } else {
            format!("\tli t0, -{bias}\n\tadd sp, sp, t0\n")
        }
    }

    pub fn generate_epilogue(&self) -> String {
        let bias = self.get_stack_size();
        if bias == 0 {
            format!("\tret\n\n")
        } else if self.call_involved {
            let lw = load_from_stack(bias - 4, String::from("ra"));
            format!("{lw}\tli t0, {bias}\n\tadd sp, sp, t0\n\tret\n")
        } else {
            format!("\tli t0, {bias}\n\tadd sp, sp, t0\n\tret\n")
        }
    }

    pub fn get_value_kind(&self, value: Value) -> &ValueKind {
        self.dfg.value(value).kind()
    }

    pub fn get_ret(&self) -> String {
        format!("Ret_{}", self.id)
    }

    pub fn get_stack_size(&self) -> u32 {
        self.stack.size
    }

    pub fn get_value_type(&self, value: Value) -> &Type {
        match self.global_value_table.get(&value) {
            Some(data) => data.ty(),
            None => self.dfg.value(value).ty(),
        }
    }

    pub fn alloc_stack(&mut self, value: Value, size: u32) -> Result<(), &'static str> {
        match self.stack.value_table.insert(value, self.stack.sp) {
            None => (),
            _ => return Err("Realloc Error"),
        }
        self.stack.incr_sp(size);
        Ok(())
    }

    pub fn get_source(&self, value: Value) -> Option<SourceType> {
        match self.param_table.get(&value) {
            Some(param) => match param {
                ParamType::Reg(reg) => Some(SourceType::Reg(*reg)),
                ParamType::RegArray(reg) => Some(SourceType::RegArray(*reg)),
                ParamType::Stack { bias } => Some(SourceType::Stack { bias: *bias }),
                ParamType::Array { bias } => Some(SourceType::Array { bias: *bias }),
            },
            None => match self.stack.value_table.get(&value) {
                Some(bias) => match self.get_value_kind(value) {
                    ValueKind::GetPtr(_) => Some(SourceType::Array { bias: *bias }),
                    ValueKind::GetElemPtr(_) => Some(SourceType::Array { bias: *bias }),
                    ValueKind::Alloc(_) => match self.get_value_type(value).kind() {
                        TypeKind::Pointer(ty) => match ty.kind() {
                            TypeKind::Pointer(_) => Some(SourceType::Pointer { bias: *bias }),
                            _ => Some(SourceType::Stack { bias: *bias }),
                        },
                        _ => unreachable!(),
                    },
                    ValueKind::Load(_) => match self.get_value_type(value).kind() {
                        TypeKind::Pointer(_) => Some(SourceType::Pointer { bias: *bias }),
                        _ => Some(SourceType::Stack { bias: *bias }),
                    },
                    _ => Some(SourceType::Stack { bias: *bias }),
                },
                None => match self.global_value_table.get(&value) {
                    Some(data) => Some(SourceType::Global(String::from(
                        &data.name().clone().unwrap()[1..],
                    ))),
                    None => match self.get_value_kind(value) {
                        ValueKind::Integer(int) => Some(SourceType::Instant(int.value())),
                        _ => None,
                    },
                },
            },
        }
    }

    pub fn get_dest(&self, value: Value) -> Option<DestType> {
        match self.stack.value_table.get(&value) {
            Some(bias) => match self.get_value_kind(value) {
                ValueKind::GetPtr(_) => Some(DestType::Array { bias: *bias }),
                ValueKind::GetElemPtr(_) => Some(DestType::Array { bias: *bias }),
                _ => Some(DestType::Stack { bias: *bias }),
            },
            None => match self.global_value_table.get(&value) {
                Some(data) => Some(DestType::Global(String::from(
                    &data.name().clone().unwrap()[1..],
                ))),
                None => None,
            },
        }
    }
}

trait ToRisc {
    fn to_risc(
        &self,
        global: &mut RiscGlobalData,
        risc_function: &mut RiscFunctionData,
    ) -> Result<String, &'static str>;
}

impl ToRisc for Value {
    fn to_risc(
        &self,
        global: &mut RiscGlobalData,
        risc_function: &mut RiscFunctionData,
    ) -> Result<String, &'static str> {
        let kind = risc_function.get_value_kind(*self);
        let mut res = match kind {
            ValueKind::Integer(_) => String::new(),
            ValueKind::Alloc(_) => return Ok(String::new()),
            ValueKind::GetPtr(get_ptr) => {
                let src = get_ptr.src();
                let index = get_ptr.index();
                let index = risc_function.get_source(index).unwrap();
                let mut ikp = match index {
                    SourceType::Global(var) => format!("\tla t2, {var}\n\tlw t2, 0(t2)\n"),
                    SourceType::Stack { bias } => load_from_stack(bias, String::from("t2")),
                    SourceType::Instant(int) => format!("\tli t2, {int}\n"),
                    _ => unreachable!(),
                };
                let elem_size =
                    if let TypeKind::Pointer(ty) = risc_function.get_value_type(src).kind() {
                            ty.size()
                    } else {
                        return Err("Try to index a non-array type.");
                    };

                ikp.push_str(&format!("\tli t1, {elem_size}\n\tmul t1, t1, t2\n",));
                let src = risc_function.get_source(src).unwrap();
                let skp = match src {
                    SourceType::Global(var) => format!("\tla t0, {var}\n"),
                    SourceType::Stack { bias } => format!("\tli t0, {bias}\n\tadd t0, sp, t0\n"),
                    SourceType::Array { bias } => load_from_stack(bias, String::from("t0")),
                    SourceType::Pointer { bias } => load_from_stack(bias, String::from("t0")),
                    _ => unreachable!(),
                };
                format!("{skp}{ikp}\tadd t0, t0, t1\n")
            }
            ValueKind::GetElemPtr(get_elem_ptr) => {
                let src = get_elem_ptr.src();
                let index = get_elem_ptr.index();
                let index = risc_function.get_source(index).unwrap();
                let mut ikp = match index {
                    SourceType::Global(var) => format!("\tla t2, {var}\n\tlw t2, 0(t2)\n"),
                    SourceType::Stack { bias } => load_from_stack(bias, String::from("t2")),
                    SourceType::Instant(int) => format!("\tli t2, {int}\n"),
                    _ => unreachable!(),
                };
                let elem_size =
                    if let TypeKind::Pointer(ty) = risc_function.get_value_type(src).kind() {
                        if let TypeKind::Array(ty, _size) = ty.kind() {
                            ty.size()
                        } else {
                            return Err("Try to index a non-array type.");
                        }
                    } else {
                        return Err("Try to index a non-array type.");
                    };

                ikp.push_str(&format!("\tli t1, {elem_size}\n\tmul t1, t1, t2\n",));
                let src = risc_function.get_source(src).unwrap();
                let skp = match src {
                    SourceType::Global(var) => format!("\tla t0, {var}\n"),
                    SourceType::Stack { bias } => format!("\tli t0, {bias}\n\tadd t0, sp, t0\n"),
                    SourceType::Array { bias } => load_from_stack(bias, String::from("t0")),
                    SourceType::Pointer { bias } => load_from_stack(bias, String::from("t0")),
                    _ => unreachable!(),
                };
                format!("{skp}{ikp}\tadd t0, t0, t1\n")
            }
            ValueKind::Call(call) => {
                let args = call.args();
                let func_name = global.get_func_name(call.callee());
                let mut res = String::new();
                let mut arg_counter = 0u32;
                for arg in args {
                    if arg_counter < 8 {
                        let arg = match risc_function.get_source(arg.clone()).unwrap() {
                            SourceType::Instant(int) => format!("\tli a{arg_counter}, {int}\n"),
                            SourceType::Stack { bias } => {
                                load_from_stack(bias, String::from(format!("a{arg_counter}")))
                            }
                            SourceType::Global(var) => {
                                format!("\tla t0, {var}\n\tlw a{arg_counter}, 0(t0)\n")
                            }
                            SourceType::Array { bias } => {
                                load_from_stack(bias, String::from(format!("a{arg_counter}")))
                            }
                            SourceType::Pointer { bias } => {
                                load_from_stack(bias, String::from(format!("a{arg_counter}")))
                            }
                            _ => unreachable!(),
                        };
                        res.push_str(&arg);
                    } else {
                        let abias = (arg_counter - 8) * 4;
                        let arg = match risc_function.get_source(arg.clone()).unwrap() {
                            SourceType::Instant(int) => format!(
                                "\tli t0, {int}\n{}",
                                store_to_stack(abias, String::from("t0"), String::from("t1"))
                            ),
                            SourceType::Stack { bias } => format!(
                                "{}{}",
                                load_from_stack(bias, String::from("t0")),
                                store_to_stack(abias, String::from("t0"), String::from("t1"))
                            ),
                            SourceType::Global(var) => format!(
                                "\tla t0, {var}\n\tlw t0, 0(t0)\n{}",
                                store_to_stack(abias, String::from("t0"), String::from("t1"))
                            ),
                            SourceType::Array { bias } => format!(
                                "{}{}",
                                load_from_stack(bias, String::from("t0")),
                                store_to_stack(abias, String::from("t0"), String::from("t1"))
                            ),
                            SourceType::Pointer { bias } => format!(
                                "{}{}",
                                load_from_stack(bias, String::from("t0")),
                                store_to_stack(abias, String::from("t0"), String::from("t1"))
                            ),
                            _ => unreachable!(),
                        };
                        res.push_str(&arg);
                    }
                    arg_counter = arg_counter + 1;
                }
                format!("{res}\tcall {func_name}\n")
            }
            ValueKind::Branch(branch) => {
                let cond = risc_function.get_source(branch.cond()).unwrap();
                let mut res = match cond {
                    SourceType::Stack { bias } => load_from_stack(bias, String::from("t0")),
                    SourceType::Array { bias } => format!(
                        "{}\tlw t0, 0(t0)\n",
                        load_from_stack(bias, String::from("t0"))
                    ),
                    SourceType::Instant(int) => format!("\tli t0, {int}\n"),
                    SourceType::Global(var) => format!("\tla t0, {var}\n\tlw t0, 0(t0)\n"),
                    _ => unreachable!(),
                };
                let true_branch = global.request_basic_block(branch.true_bb());
                let false_branch = global.request_basic_block(branch.false_bb());
                let bnez_step = global.request_bnez_count();
                res.push_str(&format!(
                    "\tbnez t0, {}\n\tj {false_branch}\n{}:\n\tj {true_branch}\n",
                    bnez_step.clone(),
                    bnez_step
                ));
                res
            }
            ValueKind::Jump(jump) => {
                let target = global.request_basic_block(jump.target());
                format!("\tj {target}\n")
            }
            ValueKind::Load(load) => match risc_function.get_source(load.src()).unwrap() {
                SourceType::Instant(_) => return Err("Couldn't load an instant number."),
                SourceType::Stack { bias } => load_from_stack(bias, String::from("t0")),
                SourceType::Global(var) => format!("\tla t0, {var}\n\tlw t0, 0(t0)\n"),
                SourceType::Array { bias } => format!(
                    "{}\tlw t0, 0(t0)\n",
                    load_from_stack(bias, String::from("t0"))
                ),
                SourceType::Pointer { bias } => load_from_stack(bias, String::from("t0")),
                _ => unreachable!(),
            },
            ValueKind::Store(store) => {
                let res = match risc_function.get_dest(store.dest()).unwrap() {
                    DestType::Stack { bias } => {
                        store_to_stack(bias, String::from("t0"), String::from("t1"))
                    }
                    DestType::Global(var) => {
                        format!("\tla t1, {var}\n\tsw t0, 0(t1)\n")
                    }
                    DestType::Array { bias } => {
                        format!(
                            "{}\tsw t0, 0(t1)\n",
                            load_from_stack(bias, String::from("t1"))
                        )
                    }
                };
                match risc_function.get_source(store.value()).unwrap() {
                    SourceType::Instant(int) => format!("\tli t0, {int}\n{res}"),
                    SourceType::Stack { bias } => {
                        format!("{}{res}", load_from_stack(bias, String::from("t0")))
                    }
                    SourceType::Reg(reg) => match risc_function.get_dest(store.dest()).unwrap() {
                        DestType::Stack { bias } => store_to_stack(
                            bias,
                            String::from(format!("a{reg}")),
                            String::from("t0"),
                        ),
                        _ => unreachable!(),
                    },
                    SourceType::RegArray(reg) => {
                        match risc_function.get_dest(store.dest()).unwrap() {
                            DestType::Stack { bias } => store_to_stack(
                                bias,
                                String::from(format!("a{reg}")),
                                String::from("t0"),
                            ),
                            _ => unreachable!(),
                        }
                    }
                    SourceType::Global(var) => {
                        format!("\tla t0, {var}\n\tlw t0, 0(t0)\n{res}")
                    }
                    SourceType::Array { bias } => {
                        format!("{}{res}", load_from_stack(bias, String::from("t0")))
                    }
                    _ => unreachable!(),
                }
            }
            ValueKind::Binary(binary) => {
                let mut risc = String::new();
                let lret = match risc_function.get_source(binary.lhs()).unwrap() {
                    SourceType::Instant(int) => {
                        if int == 0 {
                            String::from("x0")
                        } else {
                            risc.push_str(&format!("\tli t0, {int}\n"));
                            String::from("t0")
                        }
                    }
                    SourceType::Stack { bias } => {
                        risc.push_str(&load_from_stack(bias, String::from("t0")));
                        String::from("t0")
                    }
                    SourceType::Global(var) => {
                        risc.push_str(&format!("\tla t0, {var}\n\tlw t0, 0(t0)\n"));
                        String::from("t0")
                    }
                    SourceType::Array { bias } => {
                        risc.push_str(&format!(
                            "{}\tlw t0, 0(t0)\n",
                            load_from_stack(bias, String::from("t0"))
                        ));
                        String::from("t0")
                    }
                    _ => unreachable!(),
                };
                let rret = match risc_function.get_source(binary.rhs()).unwrap() {
                    SourceType::Instant(int) => {
                        if int == 0 {
                            String::from("x0")
                        } else {
                            risc.push_str(&format!("\tli t1, {int}\n"));
                            String::from("t1")
                        }
                    }
                    SourceType::Stack { bias } => {
                        risc.push_str(&load_from_stack(bias, String::from("t1")));
                        String::from("t1")
                    }
                    SourceType::Global(var) => {
                        risc.push_str(&format!("\tla t1, {var}\n\tlw t1, 0(t1)\n"));
                        String::from("t1")
                    }
                    SourceType::Array { bias } => {
                        risc.push_str(&format!(
                            "{}\tlw t0, 0(t0)\n",
                            load_from_stack(bias, String::from("t0"))
                        ));
                        String::from("t1")
                    }
                    _ => unreachable!(),
                };
                match binary.op() {
                    BinaryOp::Eq => {
                        risc.push_str(&format!("\txor t0, {}, {}\n\tseqz t0, t0\n", lret, rret));
                        risc
                    }
                    BinaryOp::NotEq => {
                        risc.push_str(&format!("\txor t0, {}, {}\n\tsnez t0, t0\n", lret, rret));
                        risc
                    }
                    BinaryOp::Lt => {
                        risc.push_str(&format!("\tslt t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Gt => {
                        risc.push_str(&format!("\tsgt t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Le => {
                        risc.push_str(&format!("\tsgt t0, {}, {}\n\tseqz t0, t0\n", lret, rret));
                        risc
                    }
                    BinaryOp::Ge => {
                        risc.push_str(&format!("\tslt t0, {}, {}\n\tseqz t0, t0\n", lret, rret));
                        risc
                    }
                    BinaryOp::Or => {
                        risc.push_str(&format!("\tor t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::And => {
                        risc.push_str(&format!("\tand t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Sub => {
                        risc.push_str(&format!("\tsub t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Add => {
                        risc.push_str(&format!("\tadd t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Mul => {
                        risc.push_str(&format!("\tmul t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Div => {
                        risc.push_str(&format!("\tdiv t0, {}, {}\n", lret, rret));
                        risc
                    }
                    BinaryOp::Mod => {
                        risc.push_str(&format!("\trem t0, {}, {}\n", lret, rret));
                        risc
                    }
                    _ => unreachable!(),
                }
            }
            ValueKind::Return(ret) => {
                let ret_bb = risc_function.get_ret();
                match ret.value() {
                    None => format!("\tj {ret_bb}\n"),
                    Some(ret_value) => match risc_function.get_source(ret_value).unwrap() {
                        SourceType::Instant(int) => format!("\tli a0, {int}\n\tj {ret_bb}\n"),
                        SourceType::Stack { bias } => {
                            if bias > 2000 {
                                format!(
                                "\tli t0, {bias}\n\tadd t0, t0, sp\n\tlw a0, (t0)\n\tj {ret_bb}\n"
                            )
                            } else {
                                format!("\tlw a0, {bias}(sp)\n\tj {ret_bb}\n")
                            }
                        }
                        SourceType::Global(var) => {
                            format!("\tla t0, {var}\n\tlw a0, 0(t0)\n\tj {ret_bb}\n")
                        }
                        SourceType::Array { bias } => {
                            format!(
                                "{}\tlw a0, 0(t0)\n",
                                load_from_stack(bias, String::from("t0"))
                            )
                        }
                        _ => unreachable!(),
                    },
                }
            }
            _ => unreachable!(),
        };
        if risc_function.get_value_type(*self).is_unit() {
            Ok(res)
        } else {
            if let DestType::Stack { bias } = risc_function.get_dest(*self).unwrap() {
                if let ValueKind::Call(_) = kind {
                    res.push_str(&store_to_stack(
                        bias,
                        String::from("a0"),
                        String::from("t0"),
                    ));
                } else {
                    res.push_str(&store_to_stack(
                        bias,
                        String::from("t0"),
                        String::from("t1"),
                    ));
                }
            } else if let DestType::Array { bias } = risc_function.get_dest(*self).unwrap() {
                res.push_str(&store_to_stack(
                    bias,
                    String::from("t0"),
                    String::from("t1"),
                ));
            }
            Ok(res)
        }
    }
}

pub fn parse_ir(program: &Program) -> Result<String, &'static str> {
    Type::set_ptr_size(4);
    let mut res = String::new();
    let mut global = RiscGlobalData::new(HashMap::new(), program);
    let mut func_id = 0;
    for &value in program.inst_layout() {
        let value_data = program.borrow_value(value);
        let value_id = &value_data.name().clone().unwrap()[1..];
        let value_init = match value_data.kind() {
            ValueKind::GlobalAlloc(global_alloc) => global_alloc.init(),
            _ => unreachable!(),
        };
        let value_init = program.borrow_value(value_init);
        let value_init = match value_init.kind() {
            ValueKind::ZeroInit(_) => {
                match value_data.ty().kind(){
                    TypeKind::Pointer(ty) => format!("\t.zero {}\n", ty.size()),
                    _ => unreachable!()
                }
            },
            ValueKind::Integer(int) => format!("\t.word {}\n", int.value()),
            ValueKind::Aggregate(aggregate) => {
                fn parse_aggregate(agg: &Aggregate, program: &Program) -> String {
                    let mut res = String::new();
                    for elem in agg.elems() {
                        let elem = program.borrow_value(*elem);
                        match elem.kind() {
                            ValueKind::Integer(int) => {
                                res.push_str(&format!("\t.word {}\n", int.value()))
                            }
                            ValueKind::Aggregate(agg) => {
                                res.push_str(&parse_aggregate(agg, program))
                            }
                            _ => unreachable!(),
                        }
                    }
                    res
                }
                parse_aggregate(aggregate, program)
            }
            _ => unreachable!(),
        };
        res.push_str(&format!(
            "\t.data\n\t.globl {}\n{}:\n{value_init}\n",
            value_id, value_id
        ));
    }
    for &func in program.func_layout() {
        //iterate all the risc_function
        let func_data = program.func(func); //get the data of the risc_function
        if let None = func_data.layout().entry_bb() {
            continue;
        }
        let func_name = &func_data.name()[1..];
        let mut risc_function =
            RiscFunctionData::new(program.borrow_values().clone(), func_data, func_id);
        let head = format!("\t.text\n\t.globl {}\n", func_name);
        let mut body = vec![];

        for (&bb, node) in func_data.layout().bbs() {
            //iterate all the basic block
            let bb_name = format!("{}:\n", global.request_basic_block(bb));
            let mut bb_body = String::new();
            for &inst in node.insts().keys() {
                let ty = risc_function.get_value_type(inst);
                let kind = risc_function.get_value_kind(inst);
                //iterate all the instruction
                if !ty.is_unit() {
                    match kind {
                        ValueKind::Alloc(_) => {
                            if let TypeKind::Pointer(ty) = ty.kind() {
                                risc_function.alloc_stack(inst, ty.size() as u32)?;
                            }
                        }
                        _ => risc_function.alloc_stack(inst, ty.size() as u32)?,
                    }
                }
                bb_body.push_str(&inst.to_risc(&mut global, &mut risc_function)?);
            }
            body.push((bb_name, bb_body));
        }
        let prologue = risc_function.generate_prologue();
        let epilogue = risc_function.generate_epilogue();
        body.first_mut().unwrap().0 = format!("{}:\n", func_name);
        body.first_mut().unwrap().0.push_str(&prologue);
        body.push((format!("{}:\n", risc_function.get_ret()), epilogue));
        res.push_str(&head);
        for (name, body) in body {
            res.push_str(&name);
            res.push_str(&body);
        }
        func_id = func_id + 1;
    }
    Ok(res)
}
