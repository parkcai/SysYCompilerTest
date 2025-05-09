//! Optimize multiplication division and modulo

use koopa::ir::builder::{LocalInstBuilder, ValueBuilder};
use koopa::ir::{BinaryOp, Function, FunctionData, ValueKind};
use koopa::opt::FunctionPass;

#[derive(Default)]
pub(crate) struct MulDiv;

impl FunctionPass for MulDiv {
    fn run_on(&mut self, _: Function, data: &mut FunctionData) {
        let mut rhs_replaced = vec![];
        let mut lhs_replaced = vec![];
        for (v, value) in data.dfg().values() {
            match value.kind() {
                ValueKind::Binary(bin) => {
                    let lhs = data.dfg().value(bin.lhs()).kind();
                    let rhs = data.dfg().value(bin.rhs()).kind();
                    if let ValueKind::Integer(rhs) = rhs {
                        let ord = if let Some(ord) = power_of_two(rhs.value()) {
                            ord
                        } else {
                            continue;
                        };
                        match bin.op() {
                            BinaryOp::Mul => {
                                rhs_replaced.push((*v, ord, BinaryOp::Shl, bin.lhs(), bin.rhs()));
                            }
                            BinaryOp::Div => {
                                rhs_replaced.push((*v, ord, BinaryOp::Shr, bin.lhs(), bin.rhs()));
                            }
                            BinaryOp::Mod => {
                                rhs_replaced.push((
                                    *v,
                                    rhs.value() - 1,
                                    BinaryOp::And,
                                    bin.lhs(),
                                    bin.rhs(),
                                ));
                            }
                            _ => continue,
                        }
                    } else if let ValueKind::Integer(lhs) = lhs {
                        let ord = if let Some(ord) = power_of_two(lhs.value()) {
                            ord
                        } else {
                            continue;
                        };
                        match bin.op() {
                            BinaryOp::Mul => {
                                lhs_replaced.push((*v, ord, BinaryOp::Shl, bin.lhs(), bin.rhs()));
                            }
                            _ => continue,
                        }
                    }
                }
                _ => continue,
            };
        }

        for (v, val, op, lhs, rhs) in &rhs_replaced {
            let rhs_builder = data.dfg_mut().replace_value_with(*rhs);
            let rhs = rhs_builder.integer(*val);
            let builder = data.dfg_mut().replace_value_with(*v);
            builder.binary(*op, *lhs, rhs);
        }

        for (v, val, op, lhs, rhs) in &lhs_replaced {
            let lhs_builder = data.dfg_mut().replace_value_with(*lhs);
            let lhs = lhs_builder.integer(*val);
            let builder = data.dfg_mut().replace_value_with(*v);
            builder.binary(*op, *rhs, lhs);
        }
    }
}

fn power_of_two(mut n: i32) -> Option<i32> {
    if n == 0 {
        return None;
    }
    let mut ord = 0;
    while n % 2 == 0 {
        n /= 2;
        ord += 1;
    }
    if n == 1 {
        Some(ord)
    } else {
        None
    }
}
