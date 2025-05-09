use koopa::ir::builder::ValueBuilder;
use koopa::ir::{BinaryOp, Function, FunctionData, Type, ValueKind};
use koopa::opt::FunctionPass;

#[derive(Default)]
pub(crate) struct ConstFold;

impl FunctionPass for ConstFold {
    fn run_on(&mut self, _: Function, data: &mut FunctionData) {
        while self.eval(data) {}
    }
}

impl ConstFold {
    fn eval(&self, data: &mut FunctionData) -> bool {
        // from koopa example code
        // find all evaluable binary instructions and evaluate
        let mut evaluated = Vec::new();
        for (v, value) in data.dfg().values() {
            let ans = match value.kind() {
                ValueKind::Binary(bin) => {
                    let lhs = data.dfg().value(bin.lhs()).kind();
                    let rhs = data.dfg().value(bin.rhs()).kind();
                    match (lhs, rhs) {
                        (ValueKind::Integer(l), ValueKind::Integer(r)) => match bin.op() {
                            BinaryOp::NotEq => Some((l.value() != r.value()) as i32),
                            BinaryOp::Eq => Some((l.value() == r.value()) as i32),
                            BinaryOp::Gt => Some((l.value() > r.value()) as i32),
                            BinaryOp::Lt => Some((l.value() < r.value()) as i32),
                            BinaryOp::Ge => Some((l.value() >= r.value()) as i32),
                            BinaryOp::Le => Some((l.value() <= r.value()) as i32),
                            BinaryOp::Add => Some(l.value() + r.value()),
                            BinaryOp::Sub => Some(l.value() - r.value()),
                            BinaryOp::Mul => Some(l.value() * r.value()),
                            BinaryOp::Div => (r.value() != 0).then(|| l.value() / r.value()),
                            BinaryOp::Mod => (r.value() != 0).then(|| l.value() % r.value()),
                            BinaryOp::And => Some(l.value() & r.value()),
                            BinaryOp::Or => Some(l.value() | r.value()),
                            BinaryOp::Xor => Some(l.value() ^ r.value()),
                            BinaryOp::Shl => Some(l.value() << r.value()),
                            BinaryOp::Shr => Some((l.value() as u32 >> r.value()) as i32),
                            BinaryOp::Sar => Some(l.value() >> r.value()),
                        },
                        (ValueKind::Undef(_), _) => unreachable!(),
                        (_, ValueKind::Undef(_)) => unreachable!(),
                        _ => continue,
                    }
                }
                _ => continue,
            };
            evaluated.push((*v, ans, data.layout().parent_bb(*v).unwrap()));
        }
        // updated values
        let changed = !evaluated.is_empty();
        // replace the evaluated instructions
        for (v, ans, _) in &evaluated {
            let builder = data.dfg_mut().replace_value_with(*v);
            if let Some(v) = ans {
                builder.integer(*v);
            } else {
                builder.undef(Type::get_i32());
            }
        }
        // remove constant values in instruction layout
        for (v, _, bb) in evaluated {
            data.layout_mut().bb_mut(bb).insts_mut().remove(&v);
        }
        changed
    }
}
