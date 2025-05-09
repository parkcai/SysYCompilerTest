use crate::front::opt::const_fold::ConstFold;
use crate::front::opt::mul_div::MulDiv;
use koopa::ir::Program;
use koopa::opt::{Pass, PassManager};

mod const_fold;
mod mul_div;

pub fn opt(program: &mut Program) {
    let mut passman = PassManager::new();
    passman.register(Pass::Function(Box::new(ConstFold::default())));
    passman.register(Pass::Function(Box::new(MulDiv::default())));
    passman.run_passes(program);
}
